#include <xinu.h>
#include <prodcons.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <run.h>

int n;                 // Definition for global variable 'n'
/* Now global variable n is accessible by all the processes i.e. consume and produce */

sid32 is_complete;      // From run command
sid32 is_prodcon_done;  // For Producer Consumer
sid32 can_read;
sid32 can_write;

shellcmd xsh_prodcons(int nargs, char *args[]) {
    // Take control using semaphore from run command
    wait(is_complete);
    
    // Argument verifications and validations
    int count = 200;    // local varible to hold count

    // TODO: check args[1], if present assign value to count    
    if(nargs == 1) {
        // Keep default count value
    }
    else if(nargs == 2) {
        if(atoi(args[1]) || strcmp( args[1], "0" ) == 0 ) {
            count = atoi(args[1]);
        }
        else {
            printf("Syntax: run prodcons [counter]\n");
            signal(is_complete);
            return (0);
        }
    }
    else {
        printf("Syntax: run prodcons [counter]\n");
        signal(is_complete);
        return (0);
    }

    // Initialize Semaphores for producer and consumer
    can_read = semcreate(0);
    can_write = semcreate(1);
    is_prodcon_done = semcreate(2);
    
    // create the process producer and consumer and put them in ready queue.
    // Look at the definations of function create and resume in the system folder for reference.
    resume(create(producer, 1024, 20, "producer", 1, count));
    resume(create(consumer, 1024, 20, "consumer", 1, count));

    // Wait for producer consumer to complete their work
    wait(is_prodcon_done);

    // Signal run command of completion
    signal(is_complete);

    return (0);
}