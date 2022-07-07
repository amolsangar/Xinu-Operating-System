#include <xinu.h>
#include <future.h>

typedef struct data_element {
    int32 time;
    int32 value;
} de;

struct stream {
    sid32 spaces;
    sid32 items;
    sid32 mutex;
    int32 head;
    int32 tail;
    struct data_element *queue;
};

int32 stream_proc(int nargs, char *args[]);
void stream_consumer(int32 id, struct stream *str, int32 work_queue_depth, int32 time_window, int32 output_time, uint ipc_port);

int32 stream_proc_future(int nargs, char *args[]);
void stream_consumer_future(int32 id, future_t *f, int32 work_queue_depth, int32 time_window, int32 output_time, uint ipc_port);