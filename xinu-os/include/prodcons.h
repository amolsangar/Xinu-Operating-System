/* Global variable for producer consumer */
extern int n; /* this is just declaration */
extern sid32 can_read;
extern sid32 can_write;
extern sid32 is_prodcon_done;

/* Function Prototype */
void consumer(int count);
void producer(int count);