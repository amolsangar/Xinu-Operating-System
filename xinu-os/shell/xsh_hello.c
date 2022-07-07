/* xsh_hello.c - xsh_hello */

#include <xinu.h>
#include <stdio.h>
#include <run.h>

sid32 is_complete;

/*------------------------------------------------------------------------
 * xhs_hello - Welcome message
 *------------------------------------------------------------------------
 */
shellcmd xsh_hello(int nargs, char *args[])
{	
	wait(is_complete);
	
	if (nargs == 2) {
		printf("Hello %s, Welcome to the world of Xinu!!\n ", args[1]);
	}
    else {
        printf("%s\n ", "Syntax: run hello name");
    }
	printf("\n");

	signal(is_complete);
	return 0;
}
