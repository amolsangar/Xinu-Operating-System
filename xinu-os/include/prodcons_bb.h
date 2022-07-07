// globally shared array
extern int arr_q[5];

// globally shared semaphores
extern sid32 can_read;
extern sid32 can_write;
extern sid32 is_prodcon_done;
extern sid32 mutex;

// globally shared read and write indices
extern int tail;    // read
extern int head;    // write

// function prototypes
void consumer_bb(int id, int count);
void producer_bb(int id, int count);