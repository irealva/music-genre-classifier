
 /***************************************************************************/
 /*                                                                         */
 /*       Copyright (C) 2000 Daniel Sleator and David Temperley             */
 /*           See http://www.link.cs.cmu.edu/music-analysis                 */
 /*        for information about commercial use of this system              */
 /*                                                                         */
 /***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#include "meter.h"

/* My version of malloc that keeps track of the number of calls, and
   prints a nice message when it runs out of space.
*/   

int max_allocs_in_use = 0;
int allocs_in_use = 0;

void * xalloc(int size) {
/* To allow printing of a nice error message, and keep track of the
   space allocated.
*/
    char * p = (char *) malloc(size);
    allocs_in_use ++;
    if (allocs_in_use > max_allocs_in_use) max_allocs_in_use = allocs_in_use;
    if (p == NULL) {
	fprintf(stderr, "%s: Ran out of space while allocating %d bytes.\n", this_program, size);
	fprintf(stderr, "%s: allocs_in_use = %d, max_allocs_in_use = %d.\n", this_program,
		allocs_in_use, max_allocs_in_use);
        exit(1);
    }
    return (void *) p;
}

void xfree(void * p) {
    allocs_in_use --;
    free((void *)p); 
}

void safe_strcpy(char *u, char * v, int usize) {
/* Copies as much of v into u as it can assuming u is of size usize */
/* guaranteed to terminate u with a '\0'.                           */    
    strncpy(u, v, usize-1);
    u[usize-1] = '\0';
}
