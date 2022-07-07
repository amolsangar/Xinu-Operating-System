/* xsh_run.c - xsh_run */

#include <xinu.h>
#include <stdio.h>
#include <stdlib.h>
#include <shprototypes.h>
#include <string.h>
#include <run.h>            // is_complete semaphore sharing
#include <runprototypes.h>
#include <future_prodcons.h>

sid32 is_complete;

/*------------------------------------------------------------------------
 * xhs_run - Run command
 *------------------------------------------------------------------------*/

shellcmd xsh_run(int nargs, char *args[])
{
    char *list[] = {
      "list",
      "hello",
      "prodcons",
      "prodcons_bb",
      "futest",
      "tscdf",
      "tscdf_fq",
      "fstest"
    };

    int listLength = 8;

    sort(list, listLength);

	// Print list of available functions
    if ((nargs == 1) || (strcmp(args[1], "list") == 0)) {

        printListOfCommands(list,listLength);
        return OK;
    }

    /* This will go past "run" and pass the function/process name and its arguments */
    args++;
    nargs--;

    // Semaphore for checking execution completion
    is_complete = semcreate(1);
    
    if(strcmp(args[0], "hello") == 0) {
        /* create a process with the function as an entry point. */
        resume (create(xsh_hello, 4096, 20, "hello", 2, nargs, args));
    }
    else if (strcmp(args[0], "futest") == 0) {
        /* create a process with the function as an entry point. */
        resume(create(future_prodcons, 4096, 20, "future_prodcons", 2, nargs, args));
    }
    else if(strcmp(args[0], "prodcons") == 0) {
        /* create a process with the function as an entry point. */
        resume (create(xsh_prodcons, 4096, 20, "prodcons", 2, nargs, args));
    }
    else if(strcmp(args[0], "prodcons_bb") == 0) {
        /* create a process with the function as an entry point. */
        resume (create(prodcons_bb, 4096, 20, "prodcons_bb", 2, nargs, args));
    }
    else if(strcmp(args[0], "tscdf") == 0) {
        /* create a process with the function as an entry point. */
        resume(create(stream_proc, 4096, 20, "stream_proc", 2, nargs, args));
    }
    else if(strcmp(args[0], "tscdf_fq") == 0) {
        /* create a process with the function as an entry point. */
        resume(create(stream_proc_futures, 4096, 20, "stream_proc_futures", 2, nargs, args));
    }
    else if(strcmp(args[0], "fstest") == 0) {
        /* create a process with the function as an entry point. */
        fstest(nargs, args);
    }
    else {
        printListOfCommands(list,listLength);
    }
    
    wait(is_complete);

	return 0;
}
