#include <xinu.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*------------------------------------------------------------------------
 * xhs_run - Sort Function
 *------------------------------------------------------------------------*/
void sort(char *arr[], int listLength) {

    //get the length of array in the variable counter
	// int counter = strlen(*arr);
	int counter = listLength;
    
    //apply ascending sorting algorithm
    int i;
    int j;
	for(i=0; i<counter-1; i++)
	{
		for(j=i+1; j<counter;j++)
		{
			if (strcmp(arr[i],arr[j])>0)
			{
				char *arrtemp;
				arrtemp = arr[i];
				arr[i] = arr[j];
				arr[j] = arrtemp;
			}
		}
	}
}

/*------------------------------------------------------------------------
 * xhs_run - Prints lists of commands
 *------------------------------------------------------------------------*/
void printListOfCommands(char *list[], int listLength) {

    int i;
    int counter = listLength;
    for(i=0;i<=counter; i++)
    {   
        if(list[i] != NULL)
            printf("%s\n", list[i]);
    }
}