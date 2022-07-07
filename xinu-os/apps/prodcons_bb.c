#include <xinu.h>
#include <prodcons_bb.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <run.h>

sid32 is_complete;
sid32 is_prodcon_done;
sid32 mutex;

int arr_q[5];

int tail;   // read
int head;   // write

void prodcons_bb(int nargs, char *args[]) {
    // definition of array, semaphores and indices

    wait(is_complete);

    //initialize read and write indices for the queue
    tail = 0;
    head = 0;

    //create producer and consumer processes and put them in ready queue
    int no_of_prod = 0;
    int no_of_cons = 0;
    int prod_iters = 0;
    int cons_iters = 0;
    if(nargs == 5) {
        if((atoi(args[1]) || strcmp( args[1], "0" ) == 0) && 
            (atoi(args[2]) || strcmp( args[2], "0" ) == 0) &&
            (atoi(args[3]) || strcmp( args[3], "0" ) == 0) &&
            (atoi(args[4]) || strcmp( args[4], "0" ) == 0) ) 
        {
            no_of_prod = atoi(args[1]);
            no_of_cons = atoi(args[2]);
            prod_iters = atoi(args[3]);
            cons_iters = atoi(args[4]);
        }
        else {
            printf("Syntax: run prodcons_bb <# of producer processes> <# of consumer processes> <# of iterations the producer runs> <# of iterations the consumer runs>\n");
            signal(is_complete);
            return;
        }
    }
    else {
        printf("Syntax: run prodcons_bb <# of producer processes> <# of consumer processes> <# of iterations the producer runs> <# of iterations the consumer runs>\n");
        signal(is_complete);
        return;
    }

    // Check Iteration Counts
    if((no_of_prod * prod_iters) != (no_of_cons * cons_iters)) {
        printf("Iteration Mismatch Error: the number of producer(s) iteration does not match the consumer(s) iteration\n");
        signal(is_complete);
        return;
    }

    //create and initialize semaphores to necessary values
    can_read = semcreate(0);
    can_write = semcreate(no_of_prod);
    
    int total_prod_cons = no_of_prod + no_of_cons;
    is_prodcon_done = semcreate(total_prod_cons);
    mutex = semcreate(1);

    int prod_id;
    for(prod_id=0;prod_id<no_of_prod;prod_id++) {
        resume(create(producer_bb, 1024, 20, "producer_bb", 2, prod_id, prod_iters));
    }

    int cons_id; 
    for(cons_id=0;cons_id<no_of_cons;cons_id++) {
        resume(create(consumer_bb, 1024, 20, "consumer_bb", 2, cons_id, cons_iters));
    }

    // Wait for producer consumer to complete their work
    // wait(is_prodcon_done);
    while(semcount(is_prodcon_done) != total_prod_cons) {}

    // Signal run command of completion
    signal(is_complete);
}