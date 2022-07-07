#include <xinu.h>
#include <stdio.h>
#include <stdlib.h>
#include <stream.h>
#include <run.h>
#include "tscdf.h"
#include <future.h>

sid32 is_prodcon_done;
sid32 print_sem;

int stream_proc_futures(int nargs, char* args[]) {
    wait(is_complete);

    print_sem = semcreate(1);
    int status;

    ulong secs, msecs, time;
    secs = clktime;
    msecs = clkticks;
    
    int32 num_streams = 0;
    int32 work_queue_depth = 0;
    int32 time_window = 0;
    int32 output_time = 0;

    // TODO: Parse arguments
    char usage[] = "Usage: run tscdf_fq -s <num_streams> -w <work_queue_depth> -t <time_window> -o <output_time>\n";

    int i;
    char *ch, c;
    if (nargs != 9) {
        printf("%s", usage);
        signal(is_complete);
        return SYSERR;
    } else {
        i = nargs - 1;
        while (i > 0) {
            ch = args[i - 1];
            c = *(++ch);

            switch (c) {
            case 's':
                num_streams = atoi(args[i]);
                break;

            case 'w':
                work_queue_depth = atoi(args[i]);
                break;

            case 't':
                time_window = atoi(args[i]);
                break;

            case 'o':
                output_time = atoi(args[i]);
                break;

            default:
                printf("%s", usage);
                signal(is_complete);
                return SYSERR;
            }

            i -= 2;
        }
    }

    // Initialize IPC ports
    ptinit(num_streams);
    
    uint ipc_port = ptcreate(num_streams);
    if (ipc_port == SYSERR) {
        printf("ERROR: PTCREATE FAILED\n");
        signal(is_complete);
        return SYSERR;
    }
    
    // TODO: Create futures
    future_t **futures;
    futures = (future_t **)getmem(num_streams * sizeof(future_t *));
    if (futures == (future_t **)SYSERR) {
        printf("ERROR: UNABLE TO ALLOCATE MEMORY\n");
        signal(is_complete);
        return SYSERR;
    }

    // TODO: Allocate futures and create consumer processes
    // Use `i` as the stream id.
    for (i = 0; i < num_streams; i++) {
        futures[i] = future_alloc(FUTURE_QUEUE, sizeof(de), work_queue_depth);

        if (futures[i] == (future_t *)SYSERR)
        {
            printf("ERROR: UNABLE TO ALLOCATE MEMORY\n");
            signal(is_complete);
            return SYSERR;
        }

        resume(create(stream_consumer_future, 4096, 20, "stream_consumer_future", 6, i, futures[i], work_queue_depth, time_window, output_time, ipc_port));
    }

    // TODO: Parse input header file data and populate work queue
    for (int i = 0; i < n_input; i++) {
        int st, ts, v;
        char *a;

        a = (char *)stream_input[i];
        st = atoi(a);
        while (*a++ != '\t');
        ts = atoi(a);
        while (*a++ != '\t');
        v = atoi(a);

        de *data = (de *)getmem(sizeof(de));
        data->time = ts;
        data->value = v;

        status = (int) future_set(futures[st], (char *)data);
        if (status < 1) {
            wait(print_sem);
            kprintf("future_set failed\n");
            signal(print_sem);
            return -1;
        }
    }
    
    // TODO: Join all launched consumer processes
    uint32 pid;
    for (int l = 0; l < num_streams; l++) {
        pid = ptrecv(ipc_port);
        kprintf("process %d exited\n", pid);
    }
    
    ptdelete(ipc_port, 0);

    // TODO: Free all futures
    for (int l = 0; l < num_streams; l++) {
        future_free(futures[i]);
    }

    // TODO: Measure the time of this entire function and report it at the end
    time = (((clktime * 1000) + clkticks) - ((secs * 1000) + msecs));
    kprintf("time in ms: %u\n", time);
    
    signal(is_complete);
    return 0;
}

void stream_consumer_future(int32 id, future_t *f, int32 work_queue_depth, int32 time_window, int32 output_time, uint ipc_port) {
    // TODO: Consume all values from the work queue of the corresponding stream
    kprintf("stream_consumer_future id:%d (pid:%d)\n", id, getpid());

    struct tscdf* tc;
    tc = tscdf_init(time_window);
    
    int current_time = 0;
    int status;
    
    while (1) {
        
        de *data = NULL;
        status = (int) future_get(f, (char *)data);
        if (status < 1) {
            wait(print_sem);
            kprintf("future_get failed\n");
            signal(print_sem);
            return;
        }

        int time_st, val;
        time_st = data->time;
        val = data->value;

        if(time_st == 0 && val == 0) {
            break;
        }
        
        tscdf_update(tc,time_st,val);

        current_time++;
        if (current_time == output_time) {
            int *qarray = (int *)getmem(6*sizeof(int32));
            char output[10];

            qarray = tscdf_quartiles(tc);

            if (qarray == NULL) {
                kprintf("tscdf_quartiles returned NULL\n");
                continue;
            }
            
            sprintf(output, "s%d: %d %d %d %d %d", id, qarray[0], qarray[1], qarray[2], qarray[3], qarray[4]);
            kprintf("%s\n", output);
            freemem((char *) qarray, (6*sizeof(int32)));

            current_time = 0;
        }
    }

    tscdf_free(tc);
    kprintf("stream_consumer_future exiting\n");
    ptsend(ipc_port, getpid());

}
