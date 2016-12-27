/* CS 352 -- Mini Shell!  
 *
 *   Sept 21, 2000,  Phil Nelson
 *   Modified April 8, 2001 
 *   Modified January 6, 2003
 *
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include "proto.h"


/* Constants */ 

#define LINELEN 1024

/* Prototypes */
void processline (char **line);

/* Shell main */
int main()
{
    char   buffer [LINELEN];
    int    len;
    int numOfArg = 0;

    while (1) {

        /* prompt and get line */
        fprintf (stderr, "%% ");
        if (fgets (buffer, LINELEN, stdin) != buffer){
            break;
        }
        
        /* Get rid of \n at end of buffer. */
        len = strlen(buffer);
        if (buffer[len-1] == '\n'){
            buffer[len-1] = 0;
        }
        
        char **location = arg_parse(buffer, &numOfArg);

        /* Run it ... */
        bool runProLine = builtInFunc(location);
        
        if (numOfArg != 1)
        {
            if (runProLine == false)
            {
                processline (location);
                printf("It ran \n");
            }
        }
        
        free(location);
        
    }

    if (!feof(stdin)) {
        perror ("read");
    }

    return 0; /* Also known as exit (0); */
}


void processline (char **line)
{
    pid_t  cpid;
    int    status;
    
    /* Start a new process to do the job. */
    cpid = fork();
    if (cpid < 0) {
      perror ("fork");
      return;
    }
    
    /* Check for who we are! */
    if (cpid == 0) {
      /* We are the child! */
      execvp ( line[0], &line[0]);
      perror ("exec");
      exit (127);
    }
    
    /* Have the parent wait for child to complete */
    if (wait (&status) < 0) {
      perror ("wait");
    }
}


