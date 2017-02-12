/* CS 352 -- Mini Shell!  
 *
 *   Sept 21, 2000,  Phil Nelson
 *   Modified April 8, 2001 
 *   Modified January 6, 2003
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include "proto.h"

/* Constants */ 

#define LINELEN 1024

/* Prototypes */
void processline (char **line);
int runFourFunctions (char *buffer, char *expandBuffer);

/* Shell main */
int main(int mainargc, char **mainargv)
{
    char   buffer [LINELEN];
    char   expandBuffer [LINELEN];
    int    len;
    FILE * fileopener;
    int countercc;
    int functional;
    
    if (mainargc == 1)
    {
        while (1) 
        {

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
            
            functional = runFourFunctions (buffer, expandBuffer);
            if (functional == 1)
            {
                continue;
            }
        }

        if (!feof(stdin)) {
            perror ("read");
        }
    }
    else if (mainargc > 1)
    {
        countercc = 0;
        fileopener = fopen(mainargv[1],"r");
        if (fileopener == NULL)
        {
            printf("Error in opening file\n");
            exit(127);
        }
        
        char * buffptr1 = buffer;
        //int check = 1;
        //printf("%d", countercc);
        while (fgets(buffptr1, 2, fileopener)!= NULL)
        {   
            //printf("%d-%d\n",countercc,*buffptr1);
            if (countercc == 1023)
            {
                printf("buffer overflow\n");
                exit(127);
            }        
            else if (*buffptr1 == '\n')
            {  
                *buffptr1 = 0;
                functional = runFourFunctions(buffer, expandBuffer);
                if (functional == 1)
                {
                    printf("Error \n");
                }
                countercc = 0;
                buffptr1 = buffer;
                continue;
            }
            buffptr1++;
            countercc++;
        }
        if (*buffptr1 == 0 && strlen(buffer)!= 2)
        {
            functional = runFourFunctions(buffer, expandBuffer);
            if (functional == 1)
            {
                printf("Error \n");
            }
        }
    }

    return 0; /* Also known as exit (0); */
}

// Runs a library program if a built in command wasn't called
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

int runFourFunctions (char *buffer, char *expandBuffer)
{
    char ** location;
    int numOfArg = 0;
    int successfulExpand = expand(buffer, expandBuffer, LINELEN);
            
    // Running arg_parse in order to return the arguments in a seperated string array format
    if (successfulExpand != 0)
    {
        location = arg_parse(expandBuffer, &numOfArg);
    }        
    else if (successfulExpand == 0)
    {
        return 1;
    }
    
    bool runProLine;
    
    /* Run it ... */
    // Kinda unneccesary but check to make sure that the process won't run if nothing is given to arg_parse
    if (numOfArg != 1)
    {           
        runProLine = builtInFunc(location, numOfArg);
    }
    else
    {
        runProLine = false;
    }
    
    // This checks to make sure if there are no built in functions then it will call the library
    // Also checks to make sure processline won't run if nothing was given to arg_parse
    if (numOfArg != 1)
    {
        // This makes sure that processline doesn't run if a built in function was called
        if (runProLine == false)
        {
            processline (location);
        }
    }
    
    // Frees up the malloc'ed location by arg_parse
    free(location);
    return 0;
}