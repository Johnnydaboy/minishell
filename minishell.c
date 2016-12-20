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


/* Constants */ 

#define LINELEN 1024

/* Prototypes */
char ** arg_parse (char *line);
void processline (char **line);

/* Shell main */
int main()
{
    char   buffer [LINELEN];
    int    len;

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
        
        char **location = arg_parse(buffer);
        
        /* Run it ... */
        processline (location);
        
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

char ** arg_parse (char *line)
{
    int counter = 0;
    bool inArg = false;
    int len = strlen(line) + 1;
    
    for (int i = 0; i < len; i++) 
    {
        if (line[i] == ' ' || line[i] == 0)
        {
            if (inArg == true)
            {
                counter = counter + 1;
                inArg = false;
            }
        }
        else if (line[i] != ' ')
        {
            inArg = true;
        }
    }
    counter = counter + 1;
    
    char ** ptrToStrArr = (char** ) malloc(sizeof (char*) * counter);
    inArg = false;
    int ptrToStrArrCounter = 0;
    for (int i = 0; i < len; i++)
    {
       if (line[i] == ' ' || line[i] == 0)
       {
           line[i] = 0;
           inArg = false;
       }
       else if (line[i] != ' ')
       {
           if (inArg == false)
           {
               ptrToStrArr[ptrToStrArrCounter] = &line[i];
               ptrToStrArrCounter = ptrToStrArrCounter + 1;
               inArg = true;
           }
       }
    }
    ptrToStrArr[ptrToStrArrCounter] = NULL;
    
    
    return ptrToStrArr;
}


