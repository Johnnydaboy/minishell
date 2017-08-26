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
int normalizeFilenameForRedirect (char *buffer, char *tempChar);
int findRedirectFilenameEnd (char *expandedBuffer, int startPos);
int openFile (char *filename, int FLAGS);

int proLineSimple(int successfulExpand, int doWait, char *expandBuffer, int fd[]);
int locatePipe (char *expandedBuffer, int fd[], int successfulExpand, int doWait);
bool normalizedString = false;

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
    //waitpid on -1 (everything) using the WNOHANG constant
}

/* Shell main */
int main(int mainargc, char **mainargv)
{
    char   buffer [LINELEN];
    //char* buffer = (char*)malloc(sizeof(char) * LINELEN);
    char   expandBuffer [LINELEN];
    //char* expandBuffer = (char*)malloc(sizeof(char) * LINELEN);
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
            perror("Error in opening file");
            exit(127);
        }
        
        char * buffptr1 = buffer;
        while (fgets(buffptr1, 2, fileopener) != NULL)
        {
            if (countercc == LINELEN - 1)
            {
                dprintf(2, "buffer overflow");
                exit(127);
            }        
            else if (*buffptr1 == '\n')
            {  
                *buffptr1 = 0;
                functional = processLine(buffer, expandBuffer, origFdArr, doWait);
                *buffptr1 = '\n';
                if (functional == 1)
                {
                    dprintf(2, "Error: proLine");
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
                dprintf(2, "Error: proLine");
            }
        }
    }
    return 0; /* Also known as exit (0); */
}
//echo $(./gen 65537)
//65536
// Runs a library program if a built in command wasn't called

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
    if (cpid == 0) 
    {

        if (dup2(newFd[0], 0) == -1)
        {
            perror("Error dup2 on fdInput");
        }
        if (dup2(newFd[1], 1) == -1)
        {
            perror("Error dup2 on fdOutput");
        }
        if (dup2(newFd[2], 2) == -1)
        {
            perror("Error dup2 on fdError");
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

/* 
    0 = sucess
    1 = error
 */
int processLine (char *buffer, char *expandBuffer, int fd[], int doWait)
{
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

//new function starting here
    int ifSuccess = locatePipe(expandBuffer, fd, successfulExpand, doWait);
    //int ifSuccess = proLineSimple(successfulExpand, doWait, expandBuffer, fd);

    if (ifSuccess == -1)
    {
        return 1;
    }
    return 0;
}

int redirection (char *expandedBuffer, int *in_fd, int *out_fd, int *err_fd)
{
    int whereIsRedirectMain = 0; // first character of redirection symbol
    int whereIsRedirectSub; // last character of redirection symbol
    int whatOperation;

    while ((whatOperation = locateRedirect(expandedBuffer, 0, &whereIsRedirectMain, &whereIsRedirectSub)) != -1)
    {
        int endOfFilenamePos = findRedirectFilenameEnd(expandedBuffer, whereIsRedirectSub + 1);
        char tempChar = expandedBuffer[endOfFilenamePos];
        expandedBuffer[endOfFilenamePos] = '\0';
        char tempChar2;
        bool changed = false;
        int endOfFileToSpace = normalizeFilenameForRedirect(&expandedBuffer[whereIsRedirectSub + 1], &tempChar2);
        if (endOfFileToSpace != 0)
        {
            changed = true;
            expandedBuffer[endOfFilenamePos] = tempChar;
            endOfFilenamePos = whereIsRedirectSub + 1 + endOfFileToSpace;
            tempChar = tempChar2;
            expandedBuffer[endOfFilenamePos] = '\0'; 
        }
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
        if (changed == true)
        {
            expandedBuffer[endOfFilenamePos] = ' ';
        }
        else
        {
            expandedBuffer[endOfFilenamePos] = tempChar;
        }
        normalizedString = true;
    }
    normalizedString = false;
    if (*out_fd == -1 || *in_fd == -1 || *err_fd == -1)
    {
        perror("open");
    }
    else if (*out_fd == -2 || *in_fd == -2 || *err_fd == -2)
    {
        return -1;
    }
    return 0;
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
    bool enterQuote = false;
    while (expandedBuffer[counter] != '\0')
    {
        if (expandedBuffer[counter] == '"')
        {
            enterQuote = true;
            counter++;
            while (enterQuote == true)
            {
                if (expandedBuffer[counter] == '"')
                {
                    enterQuote = !enterQuote;
                }
                counter++;
            }
        }
        // case 2 "<"
        if (expandedBuffer[counter] == '<')
        {
            *whereIsRedirectMain = counter;
            *whereIsRedirectSub = counter;
            return 2;
        }
        else if (expandedBuffer[counter] == '>')
        {
            if (counter >= 2 && expandedBuffer[counter - 1] == '2' && expandedBuffer[counter - 2] == ' ')
            {
                // case 5 " 2>>"
                if (expandedBuffer[counter + 1] == '>')
                {
                    *whereIsRedirectMain = counter - 1;
                    *whereIsRedirectSub = counter + 1;
                    return 5;
                }
                // case 4 " 2>"
                *whereIsRedirectMain = counter - 2;
                *whereIsRedirectSub = counter;
                return 4;
            }
            else if (counter == 1 && expandedBuffer[counter - 1] == '2')
            {
                // case 5 "2>>"
                if (expandedBuffer[counter + 1] == '>')
                {
                    *whereIsRedirectMain = counter - 1;
                    *whereIsRedirectSub = counter + 1;
                    return 5;
                }
                // case 4 "2>"
                *whereIsRedirectMain = counter - 1;
                *whereIsRedirectSub = counter;
                return 4;
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
 2 cases where redirect filename ends:
 1: we encounter '\0'
 2: we encounter another redirect operator
 
 Returns either:
 The location of the start of the next redirection operator or the location of the '\0' (potential end of filename, 
 the actual filename may be shorter - computed later)

*/
int findRedirectFilenameEnd (char *expandedBuffer, int startPos)
{
    int redirStart, redirEnd;
    if (locateRedirect(expandedBuffer, startPos, &redirStart, &redirEnd) != -1)
    {
        return redirStart;
    }

    while (expandedBuffer[startPos] != '\0')
    {
        startPos++;
    }
    return startPos;
}

int openFile (char *filename, int FLAGS)
{
    if (*filename == '\0')
    {
        return -2;
    }
    int openFd = open(filename, FLAGS, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    return openFd;
}

int normalizeFilenameForRedirect (char *buffer, char *tempChar)
{
    int counterForEof = 0;
    bool inQuote = false;
    char * ptr1 = buffer;
    char * ptr2 = buffer;
    while (*ptr2 == ' ')
    {
        ptr2++;
        counterForEof++;
    }
    while (*ptr2 != '\0')
    {
        if (*ptr2 == '"')
        {
            *ptr2 = ' ';
            inQuote = !inQuote;
            ptr2++;
        }
        else if (*ptr2 == ' ' && inQuote == false)
        {
            //*ptr2 = '\0'
            //ptr1++;
            *tempChar = *ptr1;
            *ptr1 = '\0';
            return counterForEof;
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
    return 0;
}


int locatePipe (char *expandBuffer, int fd[], int successfulExpand, int doWait)
{
    int locationOfExpandedBuffer = 0;
    int howManyPipe = 0;
    int fdtemp = dup (fd[0]);
    while (expandBuffer[locationOfExpandedBuffer] != '\0')
    {
        printf("%c\n", expandBuffer[locationOfExpandedBuffer]);
        if(expandBuffer[locationOfExpandedBuffer] == '|')
        {
            howManyPipe++;
            expandBuffer[locationOfExpandedBuffer] = '\0';
            int fileD[2];
            if (pipe(fileD) == -1)
            {
                perror("Error: pipe");
                return -1;
            }
            printf("%s\n", expandBuffer);
            if (proLineSimple(successfulExpand, doWait, expandBuffer, fileD) == 1)
            {
                dprintf(2,"Error pipeline\n");
                return -1;
            }
            close(fdtemp);
            close(fileD[1]);
            fdtemp = fileD[0];
            setAllCharsInRange(expandBuffer, 0, locationOfExpandedBuffer, ' ');
        }
        locationOfExpandedBuffer++;
    }
    if (howManyPipe == 0)
    {
        if(proLineSimple(successfulExpand, doWait, expandBuffer, fd) == 1)
        {
            dprintf(2,"Error pipeline\n");
            return -1;
        }
    }
    return 0;
}

int proLineSimple(int successfulExpand, int doWait, char *expandBuffer, int fd[])
{
    char ** location;
    int numOfArg = 0;
    int origFds[3];
    origFds[0] = fd[0];
    origFds[1] = fd[1];
    origFds[2] = fileno(stderr);

    int redirectedFds[3];
    redirectedFds[0] = origFds[0]; 
    redirectedFds[1] = origFds[1];
    redirectedFds[2] = origFds[2];
    int errorRedirect = redirection(expandBuffer, &redirectedFds[0], &redirectedFds[1], &redirectedFds[2]);
    if (errorRedirect == -1)
    {
        dprintf(2,"No file detected\n");
        return 1;
    }
    if (successfulExpand != 0)
    {
        location = arg_parse(expandBuffer, &numOfArg);
        if (location == NULL)
        {
            return 1;
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



    