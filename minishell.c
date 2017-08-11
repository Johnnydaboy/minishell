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

/* Prototypes */
void forkProcess (char **line, int fd[], int doWait);
int processLine (char *buffer, char *expandBuffer, int fd[], int doWait);
//int findChar (char * line, char * keyComparison);
int locateRedirect (char *expandBuffer, char * comparison, int *whereIsRedirectMain);

int redirection (char *expandBuffer, int doWait);

//int redirection (char *expandBuffer);
int twoGreaterThanTwice (char *bufferRedirect);
int twoGreaterThan (char *bufferRedirect);
int greaterThanTwice (char *bufferRedirect);
int greaterThan (char *bufferRedirect);
int lessThan (char *bufferRedirect);
//int locateWR (char *expandBuffer, int *check);

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
    int arr[2];
    arr[0] = 1;
    arr[1] = 1;
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
            
            functional = processLine (buffer, expandBuffer, arr, doWait);
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
                functional = processLine(buffer, expandBuffer, arr, doWait);
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
            functional = processLine(buffer, expandBuffer, arr, doWait);
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
void forkProcess (char **line, int fd[], int doWait)
{
    pid_t  cpid;
    int status;
    
    /* Start a new process to do the job. */
    cpid = fork();
    if (cpid < 0) {
      perror ("fork");
      return;
    }
    
    /* Check for who we are! */
    if (cpid == 0) {
      /* We are the child! */
      //If this is the main process then this won't run 
      if (fd[1] != 1)
      {
        dup2(fd[1], 1);
        close(fd[0]);
      }
      execvp (line[0], line);
      perror ("exec");
      exit (127);
    }
    killChild = cpid;
    /* Have the parent wait for child to complete */
    //printf("waiting?\n");
    //wait function is just waiting...
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


    /*
    int backRedirect = redirection(expandBuffer);
    if (backRedirect == -1)
    {
        printf("Error: Processline\n");
        return -1;
    }
    */

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

    
    /*
    while (expandBuffer[loop] != '\0')
    {
        int whereChar = findChar(expandBuffer, "<>");
        int whichRedirect;
        if (expandBuffer[whereChar] == '>' && expandBuffer[whereChar - 1] == '2' && expandBuffer[whereChar + 1] == '>' )
        {
            whichRedirect = 5;
        }
        else if (expandBuffer[whereChar] == '>' && expandBuffer[whereChar - 1] == '2')
        {
            whichRedirect = 4;
        }
        else if (expandBuffer[whereChar] == '>' && expandBuffer[whereChar + 1] == '>')
        {
            whichRedirect = 3;
        }
        else if (expandBuffer[whereChar] == '>')
        {
            whichRedirect = 2;
            
        }
        else if (expandBuffer[whereChar] == '<')
        {
            whichRedirect = 1;
        }
    }
    */
    bool runforkPro;
    
    /* Run it ... */
    // check to make sure that the process won't run if nothing is given to arg_parse
    if (numOfArg != 1)
    {
        runforkPro = builtInFunc(location, numOfArg, fd);
        if (runforkPro == true)
        {
            doWait = 0;
        }
        /*
        if (runforkPro == true)
        {
            dup2(fd[1], 1);
            close(fd[0]);
        }
        */
        //close(fd[0]);
        //issue with closing them is that these aren't file descriptors but to check if they are 1 makes sure they are correct
        // MAY HAVE TO CLOSE THE FILE DESCRIPTORS EVENTUALLY? IF THE FIX FOR FORK DOESN'T WORK
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
            forkProcess (location, fd, doWait);
        }
    }
    
    // Frees up the malloc'ed location by arg_parse
    free(location);
    return 0;
}


/*
int redirection (char *expandBuffer)
{
    int loop = 0;
    bool inQuote = false;
    int whichRedirect = 0;
    int countForWhich = 0;
    while (expandBuffer[loop] != '\0')
    {

        if (expandBuffer[loop] == '"')
        {
            if (inQuote == false)
            {
                inQuote = true;
            }
            else if (inQuote == true)
            {
                inQuote = false;
            }
            loop++;
        }
        else if (inQuote == true)
        {
            loop++;
        }
        if (inQuote == false)
        {
            if (whichRedirect == 0)
            {
                countForWhich = locateWR (expandBuffer, &whichRedirect);
            }
            expandBuffer = expandBuffer + countForWhich;
            if (whichRedirect == 5)
            {

            }
            else if (whichRedirect == 4)
            {

            }
            else if (whichRedirect == 3)
            {
                
            }
            else if (whichRedirect == 2)
            {
                countForWhich = greaterThan(expandBuffer, &countForWhich, &whichRedirect);
                if (countForWhich == 0)
                {
                    //found nothing to follow up
                    return 0;
                }
            }
            else if (whichRedirect == 1)
            {
                
            }
        }
    }
    return 0;
}

int findChar (char * line, char * keyComparison)
{
    int cycleRedirect = 0;
    while (line[cycleRedirect] != '\0')
    {
        int cycleKey;
        for(cycleKey = 0; cycleKey < strlen(keyComparison); cycleKey++)
        {
            if (line[cycleRedirect] == keyComparison[cycleKey])
            {
                return cycleRedirect;
            }
        }
        cycleRedirect++;
    }
    return 0;
}

int locateWhichRedirect (char *expandBuffer, int *check)
{
    int whereChar = findChar(expandBuffer, "<>");
            
    if (expandBuffer[whereChar] == '>' && expandBuffer[whereChar - 1] == '2' && expandBuffer[whereChar + 1] == '>')
    {
        *check = 5;
        expandBuffer[whereChar] = ' ';
        expandBuffer[whereChar - 1] = ' ';
        expandBuffer[whereChar + 1] = ' ';
        whereChar = whereChar + 2;
    }
    else if (expandBuffer[whereChar] == '>' && expandBuffer[whereChar - 1] == '2')
    {
        *check = 4;
        expandBuffer[whereChar] = ' ';
        expandBuffer[whereChar - 1] = ' ';
        whereChar = whereChar + 1;
    }
    else if (expandBuffer[whereChar] == '>' && expandBuffer[whereChar + 1] == '>')
    {
        *check = 3;
        expandBuffer[whereChar] = ' ';
        expandBuffer[whereChar + 1] = ' ';
        whereChar = whereChar + 2;
    }
    else if (expandBuffer[whereChar] == '>')
    {
        *check = 2;
        expandBuffer[whereChar] = ' ';
        whereChar++;
        //int failure = greaterThan (expandBuffer, &loop);
    }
    else if (expandBuffer[whereChar] == '<')
    {
        *check = 1;
        expandBuffer[whereChar] = ' ';
        whereChar++;
    }
    return whereChar;
}

int greaterThan (char *bufferRedirect, int *location, int *whichRedirect)
{
    int loopLoc = *location;
    //int whereNextRedirect = findChar(bufferRedirect, "<>");
    int whereNextRedirect = locateWR(bufferRedirect, whichRedirect);
    char outputTo[1024];
    while(loopLoc != whereNextRedirect)
    {
        outputTo[loopLoc] = bufferRedirect[loopLoc];
        loopLoc++;
    }
    int fdOutput = open(outputTo, O_CREAT | O_WRONLY | O_TRUNC);
    if (fdOutput == -1)
    {
        printf("Error: greaterThan in opening file\n");
        return -1;
    }
    return whereNextRedirect;
}
*/

int redirection (char *expandBuffer, int doWait)
{
    int redirection = 5;
    char *redirect[redirection];
    redirect[0] = "2>>";
    redirect[1] = "2>";
    redirect[2] = ">>";
    redirect[3] = ">";
    redirect[4] = "<";
    typedef int(*redirectFunc)(char *ptrToFileName);
    redirectFunc redrectFuncArr[redirection];
    redrectFuncArr[0] = twoGreaterThanTwice;
    redrectFuncArr[1] = twoGreaterThan;
    redrectFuncArr[2] = greaterThanTwice;
    redrectFuncArr[3] = greaterThan;
    redrectFuncArr[4] = lessThan;

    int cycleRedirect;
    int returnForRedirect = 0;
    int whereIsRedirectMain = 0;
    int whereIsRedirectSub;
    //int whereIsCommand = 0;
    //char* bufferRedirect = (char *) malloc (sizeof(char) * LINELEN);
    while (expandBuffer[whereIsRedirectMain] != '\0')
    {
        for (cycleRedirect = 0; cycleRedirect < redirection; cycleRedirect++)
        {
            whereIsRedirectSub = 0;
            whereIsRedirectSub = locateRedirect(expandBuffer, redirect[cycleRedirect], &whereIsRedirectMain);
            if (whereIsRedirectSub != 0)
            {

                //int ToRedirect = 0;
                bool inRedirect = true;
                char * ptrToFileName = expandBuffer + whereIsRedirectSub;
                char replacementChar;
                while (inRedirect == true)
                {
                    if (expandBuffer[whereIsRedirectSub] == '2' && expandBuffer[whereIsRedirectSub + 1] == '>' && expandBuffer[whereIsRedirectSub + 2] == '>')
                    {
                        //inRedirect = false;
                        break;
                    }
                    else if (expandBuffer[whereIsRedirectSub] == '2' && expandBuffer[whereIsRedirectSub + 1] == '>')
                    {
                        //inRedirect = false;
                        break;
                    }
                    else if (expandBuffer[whereIsRedirectSub] == '>' && expandBuffer[whereIsRedirectSub + 1] == '>')
                    {
                        //inRedirect = false;
                        break;
                    }
                    else if (expandBuffer[whereIsRedirectSub] == '>')
                    {
                        //inRedirect = false;
                        break;
                    }
                    else if (expandBuffer[whereIsRedirectSub] == '<')
                    {
                        //inRedirect = false;
                        break;
                    }
                    /*
                    for (whereIsRedirectSub; whereIsRedirectSub < whereIsRedirectMain; whereIsRedirectSub++)
                    {
                        bufferRedirect[ToRedirect] = expandBuffer[whereIsRedirectSub];
                        ToRedirect++;
                    }
                    */
                    //bufferRedirect[ToRedirect] = expandBuffer[whereIsRedirectSub];
                    //ToRedirect++;
                    whereIsRedirectSub++;
                }
                replacementChar = expandBuffer[whereIsRedirectSub];
                expandBuffer[whereIsRedirectSub] = '\0';
                returnForRedirect = (*redrectFuncArr[cycleRedirect])(ptrToFileName);
                expandBuffer[whereIsRedirectSub] = replacementChar;
                while (whereIsRedirectSub != whereIsRedirectMain)
                {
                    expandBuffer[whereIsRedirectMain] = ' ';
                    whereIsRedirectMain++;
                } 
                cycleRedirect = 0;
            }
            /*
            else if (whereIsRedirectSub == 0)
            {
                printf("Error: redirection\n");
            }
            */
        }
        if (whereIsRedirectSub == 0)
        {
            whereIsRedirectMain++;
        }
    }
}

int locateRedirect (char *expandBuffer, char * comparison, int *whereIsRedirectMain)
{
    int counter = *whereIsRedirectMain;
    while (expandBuffer[counter] != '\0')
    {
        if (expandBuffer[counter] != *comparison)
        {
            break;
        }
        counter++;
        comparison++;
        //counter++;
    }
    if (*comparison == '\0')
    {
        return counter;
    }
    return 0;
}
/*
int locateRedirect (char *expandBuffer, char *redirect, int *whereIsRedirectMain)
{
    int counterForEB = *whereIsRedirectMain;
    int cpyOfWhereIsRedirectMain = *whereIsRedirectMain;
    while(*redirect != '\0')
    {
        if (expandBuffer[cpyOfWhereIsRedirectMain] != *redirect)
        {
            break;
        }
        cpyOfWhereIsRedirectMain++;
        redirect++;
    }
    if (*redirect == '\0')
    {
        //*whereIsRedirectMain = cpyOfWhereIsRedirectMain;
        return counterForEB;
    }
    return 0;
}
*/