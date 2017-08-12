#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <signal.h>
#include "proto.h"
#include "globals.h"

int margc;
char **margv;
int exitStatus;
char prompt[1024];
int globalIntSigInt;
int killChild = -1;

typedef int(*redirectFunc)(char *ptrToFileName);

/* Prototypes */
void forkProcess (char **line, int origFd[], int newFd[], int doWait);
int processLine (char *buffer, char *expandBuffer, int fd[], int doWait);
int redirection (char *expandedBuffer, int *in_fd, int *out_fd, int *err_fd);
int locateRedirect (char *expandBuffer, int startPos, int *whereIsRedirectMain, int *whereIsRedirectSub);
int normalizeFilenameForRedirect (char *buffer);
int findRedirectFilenameEnd (char *expandedBuffer, int startPos);
int openFile (char *filename, int FLAGS);

void setAllCharsInRange (char *expandedBuffer, int start, int end, char c);
//int argCmdParse ();

void sigIntHandler (int signum)
{
    //sleep(1);
    if (signum == SIGINT)
    {
        globalIntSigInt = signum;
    }
    if (killChild != -1)
    {
        kill(killChild, SIGINT);
    }
}

/* Shell main */
int main(int mainargc, char **mainargv)
{
    //char   buffer [LINELEN];
    char* buffer = (char*)malloc(sizeof(char) * LINELEN);
    //char   expandBuffer [LINELEN];
    char* expandBuffer = (char*)malloc(sizeof(char) * LINELEN);
    int    len;
    FILE * fileopener; 
    int countercc;
    int functional; 
    margc = mainargc;
    margv = mainargv;
    int doWait = 0;
    int origFdArr[2];
    origFdArr[0] = fileno(stdin);
    origFdArr[1] = fileno(stdout);
     struct sigaction act;
    act.sa_handler = sigIntHandler;
    act.sa_flags = SA_RESTART;
    sigemptyset (&act.sa_mask);
    //sigaction(SIGTSTP, &act, NULL);
    if(sigaction(SIGINT, &act, NULL) == -1)
    {
        perror("SIGACTION");
    }
    
    if (mainargc == 1)
    {
        while (1) 
        {

            /* prompt and get line */
            if (prompt[0] == '\0')
            {
                fprintf (stderr, "%% ");
            }
            else
            {
                fprintf (stderr, "%s", prompt);
            }
            if (fgets (buffer, LINELEN, stdin) != buffer)
            {
                break;
            }
            /* Get rid byteof \n at end of buffer. */
            len = strlen(buffer);
            if (buffer[len-1] == '\n')
            {
                buffer[len-1] = 0;
            }
            
            functional = processLine (buffer, expandBuffer, origFdArr, doWait);
            if (functional == 1)
            {
                continue;
            }
        }

        if (!feof(stdin)) 
        {
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
        while (fgets(buffptr1, 2, fileopener) != NULL)
        {
            if (countercc == LINELEN - 1)
            {
                printf("buffer overflow\n");
                exit(127);
            }        
            else if (*buffptr1 == '\n')
            {  
                *buffptr1 = 0;
                functional = processLine(buffer, expandBuffer, origFdArr, doWait);
                *buffptr1 = '\n';
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
        if (*buffptr1 == 0)
        {
            functional = processLine(buffer, expandBuffer, origFdArr, doWait);
            if (functional == 1)
            {
                printf("Error \n");
            }
        }
    }
    return 0; /* Also known as exit (0); */
}
//echo $(./gen 65537)
//65536
// Runs a library program if a built in command wasn't called

//break it up into int fd[0] and fd[1] and send it through
void forkProcess (char **line, int origFd[], int newFd[], int doWait)
{
    pid_t  cpid;
    int status;
    
    /* Start a new process to do the job. */
    cpid = fork();
    if (cpid < 0) 
    {
      int n;
      for (n = 0; n < 3; n++)
      {
          if (origFd[n] != newFd[n])
          {
              close(newFd[n]);
          }
      }
      perror ("fork");
      return;
    }
    
    /*We are the child! */
    // If this is the main process then this won't run 
    if (cpid == 0) {

        if (dup2(newFd[0], 0) == -1)
        {
            printf("Error dup2 on fdInput\n");
        }
        if (dup2(newFd[1], 1) == -1)
        {
            printf("Error dup2 on fdOutput\n");
        }
        if (dup2(newFd[2], 2) == -1)
        {
            printf("Error dup2 on fdError\n");
        } 

        execvp (line[0], line);
        perror ("exec");
        exit (127);
    }

    /* We are the parent! */
    // Have the parent wait for child to complete */
    killChild = cpid;
    if (doWait != 1)
    {
        if (wait (&status) < 0) 
        {
          perror ("wait");
        }
    }
    if (WIFEXITED(status))
    {
        exitStatus = WEXITSTATUS(status);
        //printf("Exit status was %d\n", exitStatus);
    }
}

/* 0 = sucess
   1 = error
 */
int processLine (char *buffer, char *expandBuffer, int fd[], int doWait)
{
    char ** location;
    int numOfArg = 0;
    int checkForPoundSign = 0;
    bool inQuote = false;
    while (buffer[checkForPoundSign] != '\0')
    {
        if (buffer[checkForPoundSign] == '#' && inQuote == false)
        {
            buffer[checkForPoundSign] = '\0';
        }
        if (buffer[checkForPoundSign] == '"' && inQuote == false)
        {
            inQuote = true;
        }
        else if (buffer[checkForPoundSign] == '"' && inQuote == true)
        {
            inQuote = false;
        }
        checkForPoundSign++;
    }
    int successfulExpand = expand(buffer, expandBuffer, LINELEN);
    //printf("se%d\n", successfulExpand);
    if (successfulExpand == -1)
    {
        return 1;
    }
    // Running arg_parse in order to return the arguments in a seperated string array format

    int origFds[3];
    origFds[0] = fd[0];
    origFds[1] = fd[1];
    origFds[2] = fileno(stderr);

    int redirectedFds[3];
    redirectedFds[0] = origFds[0]; 
    redirectedFds[1] = origFds[1];
    redirectedFds[2] = origFds[2];  
    int backRedirect = redirection(expandBuffer, &redirectedFds[0], &redirectedFds[1], &redirectedFds[2]);
    if (backRedirect == -1)
    {
        printf("Error: Processline\n");
        return -1;
    }
    
    if (successfulExpand != 0)
    {
        location = arg_parse(expandBuffer, &numOfArg);
        if (location == NULL)
        {
            return 2;
        }
    }        
    else if (successfulExpand == 0)
    {
        return 1;
    }
    bool runforkPro;
    
    /* Run it ... */
    // check to make sure that the process won't run if nothing is given to arg_parse
    if (numOfArg != 1)
    {
        runforkPro = builtInFunc(location, numOfArg, redirectedFds);
        if (runforkPro == true)
        {
            doWait = 0;
        }
    }
    else
    {
        runforkPro = false;
    }
    
    // This checks to make sure if there are no built in functions then it will call the library
    // Also checks to make sure processline won't run if nothing was given to arg_parse
    if (numOfArg != 1)
    {
        // This makes sure that processline doesn't run if a built in function was called
        if (runforkPro == false)
        {
            forkProcess (location, origFds, redirectedFds, doWait);
        }
    }
    
    // Frees up the malloc'ed location by arg_parse
    free(location);
    return 0;
}

int redirection (char *expandedBuffer, int *in_fd, int *out_fd, int *err_fd)
{
    int returnForRedirect = 0; // fd to return
    int whereIsRedirectMain = 0; // first character of redirection symbol
    int whereIsRedirectSub; // last character of redirection symbol
    int whatOperation;

    while ((whatOperation = locateRedirect(expandedBuffer, 0, &whereIsRedirectMain, &whereIsRedirectSub)) != -1)
    {
        int endOfFilenamePos = findRedirectFilenameEnd(expandedBuffer, whereIsRedirectSub + 1);
        char tempChar = expandedBuffer[endOfFilenamePos];
        expandedBuffer[endOfFilenamePos] = '\0';
        normalizeFilenameForRedirect(&expandedBuffer[whereIsRedirectSub + 1]);

        if (whatOperation == 1)
        {
            *out_fd = openFile(&expandedBuffer[whereIsRedirectSub + 1], O_CREAT | O_WRONLY | O_TRUNC);
        }
        else if (whatOperation == 2)
        {
            *in_fd = openFile(&expandedBuffer[whereIsRedirectSub + 1], O_RDONLY);
        }
        else if (whatOperation == 3)
        {
            *out_fd = openFile(&expandedBuffer[whereIsRedirectSub + 1], O_APPEND | O_WRONLY | O_CREAT);
        }
        else if (whatOperation == 4)
        {
            *err_fd = openFile(&expandedBuffer[whereIsRedirectSub + 1], O_CREAT | O_WRONLY | O_TRUNC);
        }
        else if (whatOperation == 5)
        {
            *err_fd = openFile(&expandedBuffer[whereIsRedirectSub + 1], O_APPEND | O_WRONLY | O_CREAT);
        }
        setAllCharsInRange (expandedBuffer, whereIsRedirectMain, endOfFilenamePos, ' ');
        expandedBuffer[endOfFilenamePos] = tempChar;
    }
    return returnForRedirect;
}

void setAllCharsInRange (char *expandedBuffer, int start, int end, char c)
{
    for(;start <= end; start++)
    {
        expandedBuffer[start] = c;
    }
}
/* Returns:
 1 if >
 2 if <
 3 if >>
 4 if 2>
 5 if 2>>
 -1 if no operators found

 Sets:
 whereIsRedirectMain to the position of the first character of the redirect operator
 whereIsRedirectSub to the position of the last character of the redirect operator
*/
int locateRedirect (char *expandedBuffer, int startPos, int *whereIsRedirectMain, int *whereIsRedirectSub)
{
    int counter = startPos;
    while (expandedBuffer[counter] != '\0')
    {
        // case 2 "<"
        if (expandedBuffer[counter] == '<')
        {
            *whereIsRedirectMain = counter;
            *whereIsRedirectSub = counter;
            return 2;
        }
        else if (expandedBuffer[counter] == '>')
        {
            if (counter != 0 && expandedBuffer[counter - 1] == '2')
            {
                // case 5 "2>>"
                if (expandedBuffer[counter + 1] == '>')
                {
                    *whereIsRedirectMain = counter -1;
                    *whereIsRedirectSub = counter + 1;
                    return 5;
                }
                // case 4 "2>"
                else
                {
                    *whereIsRedirectMain = counter - 1;
                    *whereIsRedirectSub = counter;
                    return 4;
                }
            }
            else 
            {
                // case 3 ">>"
                if (expandedBuffer[counter + 1] == '>') 
                {
                    *whereIsRedirectMain = counter;
                    *whereIsRedirectSub = counter + 1;
                    return 3;
                }
                // case 1 ">"
                else
                {
                    *whereIsRedirectMain = counter;
                    *whereIsRedirectSub = counter;
                    return 1;
                }
            }
        }
        counter++;
    }
    return -1;
}

/*
 3 cases where redirect filename ends:
 1: we encounter a space
 2: we encounter '\0'
 3: we encounter another redirect operator
 4: filename is in quotes

 Returns:
 1 if we successfully found a filename
 0 if unsuccessful (i.e. missing quote in filename)
*/
int findRedirectFilenameEnd (char *expandedBuffer, int startPos)
{
    int redirStart, redirEnd;
    if (locateRedirect(expandedBuffer, startPos, &redirStart, &redirEnd) != -1)
    {
        return redirEnd;
    }

    while (expandedBuffer[startPos] != '\0')
    {
        startPos++;
    }
    return startPos;
}

int openFile (char *filename, int FLAGS)
{
    int openFd = open(filename, FLAGS, S_IRUSR | S_IWUSR | S_IRGRP | S_IRGRP);
    if(openFd == -1)
    {
        perror("open");
    }
    return openFd;
}

int normalizeFilenameForRedirect (char *buffer)
{
    int counterForEof = 0;
    bool inQuote = false;
    char * ptr1 = buffer;
    char * ptr2 = buffer;
    while (*ptr2 != '\0')
    {
        if (*ptr2 == '"' || *ptr2 == ' ')
        {
            if (*ptr2 == '"')
            {
                *ptr2 = ' ';
                inQuote = !inQuote;
            }
            ptr2++;
        }
        else if (*ptr2 == ' ' && inQuote == false)
        {
            //*ptr2 = '\0';
            break;
        }
        else if (inQuote == false)
        {
            *ptr1 = *ptr2;
            ptr1++;
            ptr2++;
            counterForEof++;
        }
        else if (inQuote == true)
        {
            *ptr1 = *ptr2;
            ptr1++;
            ptr2++;
            counterForEof++;
        }
    }
    *ptr1 = '\0';
    return counterForEof;
}

