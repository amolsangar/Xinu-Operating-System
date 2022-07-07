#include <xinu.h>
#include <stdio.h>
#include <stdlib.h>
#include <stream.h>
#include <run.h>
#include "tscdf.h"

sid32 is_prodcon_done;

int stream_proc(int nargs, char *args[]) {   
    wait(is_complete);

    ulong secs, msecs, time;
    secs = clktime;
    msecs = clkticks;
    
    int32 num_streams = 0;
    int32 work_queue_depth = 0;
    int32 time_window = 0;
    int32 output_time = 0;

    // TODO: Parse arguments
    char usage[] = "Usage: run tscdf -s <num_streams> -w <work_queue_depth> -t <time_window> -o <output_time>\n";

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

    // TODO: Create streams
    struct stream **s;
    s = (struct stream **)getmem(num_streams * sizeof(struct stream *));
    if (s == (struct stream**)SYSERR) {
        printf("ERROR: UNABLE TO ALLOCATE MEMORY\n");
        signal(is_complete);
        return SYSERR;
    }

    // TODO: Create consumer processes and initialize streams
    // Use `i` as the stream id.
    for (int i = 0; i < num_streams; i++) {
        s[i] = (struct stream *) getmem((sizeof(de) * work_queue_depth) + sizeof(struct stream));
        if (s[i] == (struct stream *)SYSERR)
        {
            printf("ERROR: UNABLE TO ALLOCATE MEMORY\n");
            signal(is_complete);
            return SYSERR;
        }
        s[i]->items = semcreate(work_queue_depth);
        s[i]->mutex = semcreate(1);
        s[i]->spaces = semcreate(0);
        s[i]->head = 0;
        s[i]->tail = 0;
        s[i]->queue = sizeof(struct stream) + (char *)s[i];

        resume(create(stream_consumer, 4096, 20, "stream_consumer", 6, i, s[i], work_queue_depth, time_window, output_time, ipc_port));
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

        wait(s[st]->items);
        wait(s[st]->mutex);
        
        int head = s[st]->head;
        s[st]->queue[head].time = ts;
        s[st]->queue[head].value = v;
        head = head + 1;
        head = head % work_queue_depth;
        s[st]->head = head;

        signal(s[st]->mutex);
        signal(s[st]->spaces);
    }

    // TODO: Join all launched consumer processes
    uint32 pid;
    for (int l = 0; l < num_streams; l++) {
        pid = ptrecv(ipc_port);
        kprintf("process %d exited\n", pid);
    }
    
    ptdelete(ipc_port, 0);

    // TODO: Measure the time of this entire function and report it at the end
    time = (((clktime * 1000) + clkticks) - ((secs * 1000) + msecs));
    kprintf("time in ms: %u\n", time);

    signal(is_complete);
    return OK;
}

void stream_consumer(int32 id, struct stream *str, int32 work_queue_depth, int32 time_window, int32 output_time, uint ipc_port) {
    // TODO: Consume all values from the work queue of the corresponding stream
    kprintf("stream_consumer id:%d (pid:%d)\n", id, getpid());

    struct tscdf* tc;
    tc = tscdf_init(time_window);
    
    int current_time = 0;
    
    while (1) {
        wait(str->spaces);
        wait(str->mutex);

        int tail;
        int time_st, val;

        tail = str->tail;
        time_st = str->queue[tail].time;
        val = str->queue[tail].value;
        tail = tail + 1;
        tail = tail % work_queue_depth;
        str->tail = tail;

        if(time_st == 0 && val == 0) {
            signal(str->mutex);
            signal(str->items);
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
        
        signal(str->mutex);
        signal(str->items);
    }

    tscdf_free(tc);
    kprintf("stream_consumer exiting\n");
    ptsend(ipc_port, getpid());
}
