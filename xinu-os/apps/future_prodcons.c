#include <xinu.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <future_prodcons.h>
#include <run.h>

sid32 is_complete;

 int future_fib(int nargs, char *args[]);
 int future_free_test(int nargs, char *args[]);

/*------------------------------------------------------------------------
* FUTEST
*------------------------------------------------------------------------*/
void future_prodcons(int nargs, char *args[]) {
    // Take control using semaphore from run command
    wait(is_complete);

    print_sem = semcreate(1);
    char *val;

    if(strcmp(args[1], "--free") == 0) {
        if(nargs > 2) {
            printf("Syntax: run futest [-pc [g ...] [s VALUE ...]] | [-pcq LENGTH [g ...] [s VALUE ...]] | [-f NUMBER] | [--free]\n");
        } 
        else {
            resume(create(future_free_test, 4096, 20, "future_free_test", 2, nargs, args));
        }

        signal(is_complete);
        return;
    }
    
    if ( (strcmp(args[1], "-pc") != 0 && strcmp(args[1], "-pcq") != 0 && strcmp(args[1], "-f") != 0) || nargs == 2 ) {
        printf("Syntax: run futest [-pc [g ...] [s VALUE ...]] | [-pcq LENGTH [g ...] [s VALUE ...]] | [-f NUMBER] | [--free]\n");
        signal(is_complete);
        return;
    }
    
    if(strcmp(args[1], "-f") == 0) {
        resume(create(future_fib, 4096, 20, "future_fib", 2, nargs, args));
        signal(is_complete);
        return;
    }

    // First, try to iterate through the arguments and make sure they are all valid based on the requirements
    // (you should not assume that the argument after "s" is always a number)
    int i = 3;    // Setting to max of below
    if(strcmp(args[1], "-pc") == 0) {
        i = 2;
    }
    else if(strcmp(args[1], "-pcq") == 0) {
        i = 3;
    }
    
    int total_prod_cons = 0;
    while (i < nargs) {
        // TODO: write your code here to check the validity of arguments

        if(strcmp(args[i],"g") == 0) {
            total_prod_cons++;
        }
        else if(strcmp(args[i],"s") == 0) {
            total_prod_cons++;
            
            if(args[i+1] && (strcmp(args[i+1],"s") == 0 || strcmp(args[i+1],"g") == 0 || atoi(args[i+1]))) {
                i++;
            }
            else {
                printf("Syntax: run futest [-pc [g ...] [s VALUE ...]] | [-pcq LENGTH [g ...] [s VALUE ...]] | [-f NUMBER] | [--free]\n");
                signal(is_complete);
                return;
            }
        }
        else {
            printf("Syntax: run futest [-pc [g ...] [s VALUE ...]] | [-pcq LENGTH [g ...] [s VALUE ...]] | [-f NUMBER] | [--free]\n");
            signal(is_complete);
            return;         
        }

        i++;
    }

    if(strcmp(args[1], "-pc") == 0) {
        future_t* f_exclusive;
        f_exclusive = future_alloc(FUTURE_EXCLUSIVE, sizeof(int), 1);

        int num_args = i;  // keeping number of args to create the array
        i = 2; // reseting the index
        val  =  (char *) getmem(num_args); // initializing the array to keep the "s" numbers

        // Iterate again through the arguments and create the following processes based on the passed argument ("g" or "s VALUE")
        while (i < nargs) {
            if (strcmp(args[i], "g") == 0) {
                char id[10];
                sprintf(id, "fcons%d",i);
                resume(create(future_cons, 2048, 20, id, 1, f_exclusive));
            }
            if (strcmp(args[i], "s") == 0) {
                i++;
                uint8 number = atoi(args[i]);
                val[i] = number;
                resume(create(future_prod, 2048, 20, "fprod1", 2, f_exclusive, &val[i]));
                sleepms(5);
            }
            i++;
        }
        sleepms(100);
        future_free(f_exclusive);
    }
    else if(strcmp(args[1], "-pcq") == 0) {
        
        int max_elems = 0;
        if( (atoi(args[2]) || strcmp( args[2], "0" ) == 0) && nargs > 3) {
            // GET MAX ELEMENTS
            max_elems = atoi(args[2]);
        }
        else {
            printf("Syntax: run futest [-pc [g ...] [s VALUE ...]] | [-pcq LENGTH [g ...] [s VALUE ...]] | [-f NUMBER] | [--free]\n");
            signal(is_complete);
            return;
        }
        
        future_t* f_queue;
        f_queue = future_alloc(FUTURE_QUEUE, sizeof(int), max_elems);

        int num_args = i;  // keeping number of args to create the array
        i = 3; // reseting the index
        val  =  (char *) getmem(num_args); // initializing the array to keep the "s" numbers

        // Iterate again through the arguments and create the following processes based on the passed argument ("g" or "s VALUE")
        while (i < nargs) {
            if (strcmp(args[i], "g") == 0) {
                char id[10];
                sprintf(id, "fcons%d",i);
                resume(create(future_cons, 2048, 20, id, 1, f_queue));
            }
            if (strcmp(args[i], "s") == 0) {
                char id[10];
                sprintf(id, "fprod%d",i);
                i++;
                uint8 number = atoi(args[i]);
                val[i] = number;
                resume(create(future_prod, 2048, 20, id, 2, f_queue, &val[i]));
                sleepms(5);
            }
            i++;
        }
        sleepms(100);
        future_free(f_queue);
    }

    // Signal run command of completion
    signal(is_complete);
}