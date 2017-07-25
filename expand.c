#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/types.h>
#include <time.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>

#include "proto.h"
#include "globals.h"

bool normalExit = true;
int whatInz;
int whatDollar;
char globalStrValOfInt[1024];
int findInz (char * orig, char * Inz);
bool isQuote;
char * expandEnvVar (char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
char * expandPid (char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
char * expandNumArgs (char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
char * expandLocOfArg (char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
char * expandProcessId (char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
char * wildCardExpand (char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
char * wildCardPrint (char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
//char * poundSign (char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
int comparisionFunc(char * comparBuf, char * dirBuf);
char * expandHomeDir (char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
char * commandExpansion (char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
// This function takes in two character arrays and writes to the new buffer by reading from the old buffer, newsize is the length of array new
// In a failure case this function will return a 0 and otherwise it returns 1
int expand (char *orig, char *new, int newsize)
{
    int lenOfFuncArr = 8;
    char *Inz[lenOfFuncArr];
    Inz[0] = "${";
    Inz[1] = "$$";
    Inz[2] = "$#";
    Inz[3] = "$?";
    Inz[4] = "$";
    Inz[5] = "\\*";
    Inz[6] = "*";
    Inz[7] = "~";
    Inz[8] = "$(";
    int whereIsInz = 0;
    int whereIsNew = 0;
    typedef char *(*funcInz)(char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
    funcInz funcInzArr[lenOfFuncArr];
    funcInzArr[0] = expandEnvVar;
    funcInzArr[1] = expandPid;
    funcInzArr[2] = expandNumArgs;
    funcInzArr[3] = expandProcessId;
    funcInzArr[4] = expandLocOfArg;
    funcInzArr[5] = wildCardPrint;
    funcInzArr[6] = wildCardExpand;
    funcInzArr[7] = expandHomeDir;
    funcInzArr[8] = commandExpansion;
    int lenOfParam = 0;
    // This while loop will continue to execute until the orig string reads at 0 
    while (orig[whereIsInz] != 0)
    {
        for (whatInz = 0; whatInz < lenOfFuncArr; whatInz++)
        {
            lenOfParam = 0;
            lenOfParam = findInz(&orig[whereIsInz], Inz[whatInz]);
            if (lenOfParam != 0)
            {
                int inputC = 0;
                int inputNew = 0;
                whereIsInz = whereIsInz + lenOfParam;
                // counter (orig counter is inputC) is added to the original buffer in order to skip ahead what is found in findInz
                // ^ (new counter is inputNew) same applies to counterNew but with newBuff rather
                char * copyOver = (*funcInzArr[whatInz])(&orig[whereIsInz], new, &inputC, &inputNew);
                //printf("copyOver is: %s\n", copyOver);
                // If it fail and no } is found it will print an error statement
                if (inputNew == -1)
                {
                    printf("${: No matching }\n");
                    return 0;
                }
                else if (inputC == -1)
                {
                    return 0;
                }
                // This tells how far each string needs to skip by
                whereIsInz = whereIsInz + inputC;
                whereIsNew = whereIsNew + inputNew;
                if (whereIsNew > newsize)
                {
                    printf("Expand too big\n");
                    return 0; 
                }
                else
                {
                    int i;
                    // the single character copy over that is given if a function is called
                                            //printf("copyOver is: %s\n", copyOver);
                    for (i = 0; i < inputNew; i++)
                    {
                        *new = *copyOver;
                        new++;
                        copyOver++;
                    }
                    whatInz = 0;
                    break;
                }
            }
        }
        if (lenOfParam == 0)
        {
                                                            //printf("goeshere?\n");
            *new = orig[whereIsInz];
            new++;
            whereIsInz++;
            whereIsNew++;
        }
    }
    *new = '\0';
    return 1;
}

// This function finds the next initilizing variable and returns the location of it if any
int findInz (char * orig, char * Inz)
{
    int counter = 0;
    while (*Inz != '\0')
    {
        if (*orig != *Inz)
        {
            break;
        }
        orig++;
        Inz++;
        counter++;
    }
    if (*Inz == '\0')
    {
        return counter;
    }
    return 0;
}

// This is the first expand subfunction which does and expand to ${} 
char * expandEnvVar (char * origBuffLoc, char * newBuff, int * counter, int * counterNew)
{
    char * ptr2temp;
    int count = 0;
    ptr2temp = origBuffLoc;
    
    while (*origBuffLoc != '}')
    {
        origBuffLoc++;
        count++;
        // This is an error statement and above will check if -1 it will fail
        if (*origBuffLoc == 0)
        {
            *counterNew = -1;
            return NULL;
        }
    }
    
    *origBuffLoc = '\0';
    count++;
    *counter = count;
    
    if (count == 1)
    {
        return NULL;
    }
    
    
    char * envstr = getenv(ptr2temp);
    
    if (envstr == NULL)
    {
        return NULL;
    }
    
    int much = strlen(envstr);
    *counterNew = much;
    return envstr;
}

// The second expand subfunction which expands the $$ environment
char * expandPid (char * origBuffLoc, char * newBuff, int * counter, int * counterNew)
{
    int envint = getppid();
    sprintf(globalStrValOfInt,"%d", envint);
    int cNew = strlen(globalStrValOfInt);
    *counterNew = cNew;
    return globalStrValOfInt;
}

char * expandNumArgs (char * origBuffLoc, char * newBuff, int * counter, int * counterNew)
{
    int argsHere = 0;
    if (margc == 1)
    {
        argsHere = margc;
        sprintf(globalStrValOfInt, "%d", argsHere);
        *counterNew = strlen(globalStrValOfInt);
        return globalStrValOfInt;
    }
    else 
    {
        argsHere = margc - (counterForShift + 1);
    }
    //printf("%d\n", argsHere);
    sprintf(globalStrValOfInt, "%d", argsHere);
    *counterNew = strlen(globalStrValOfInt);
    return globalStrValOfInt;
}

char * expandProcessId (char * origBuffLoc, char * newBuff, int * counter, int * counterNew)
{
    int envint;
    if (normalExit == true)
    {
        sprintf(globalStrValOfInt, "%d", exitStatus);
        int cNew = strlen(globalStrValOfInt);
        *counterNew = cNew;
        return globalStrValOfInt;
    }
    else if (normalExit == false)
    {
        envint = 1;
    }
    else
    {
        envint = 127;
    }
    
    sprintf(globalStrValOfInt,"%d", envint);
    int cNew = strlen(globalStrValOfInt);
    *counterNew = cNew;
    return globalStrValOfInt;
}








char * expandLocOfArg (char * origBuffLoc, char * newBuff, int * counter, int * counterNew)
{
    int totalNum = 0;
    int length = 0;
    int locationInfo = 0;
    bool enterNum = false;
    int locForGlobalValOfInt = 0; 
    int tempArg = 0;
    if (*origBuffLoc == '\0' || *origBuffLoc == ' ' )
    {
        printf("error here\n");
        return NULL;
    }
    //This algorithm converts strings into ints using the ASCII table (look at reference from char and their decimal 
    // numbers)
    while (*origBuffLoc >= '0' && *origBuffLoc <= '9')
    {
        enterNum = true;
        totalNum = totalNum * 10 + *origBuffLoc - '0';
        length++;
        origBuffLoc++;
    } 
    if (enterNum == false)
    {
        globalStrValOfInt[locForGlobalValOfInt] = '\0';
        return globalStrValOfInt;
    }
    if (totalNum == 0)
    {
        if (margc == 1)
        {
            char tempCpyMargv[1024];
            char * ptrToTemp = tempCpyMargv;
            int tempArgLen = strlen(margv[0]);
            memcpy(tempCpyMargv, margv[0], strlen(margv[0]));
            ptrToTemp[tempArgLen] = '\0';
            while (ptrToTemp[tempArg] != '\0')
            {
                globalStrValOfInt[locForGlobalValOfInt] = ptrToTemp[tempArg];
                tempArg++;
                locForGlobalValOfInt++;
            }
            tempArg = 0;
        }
        else if (margc != 1)
        {
            char tempCpyMargv[1024];
            char * ptrToTemp = tempCpyMargv;
            int tempArgLen = strlen(margv[1]);
            memcpy(tempCpyMargv, margv[1], strlen(margv[1]));
            ptrToTemp[tempArgLen] = '\0';
            while (ptrToTemp[tempArg] != '\0')
            {
                globalStrValOfInt[locForGlobalValOfInt] = ptrToTemp[tempArg];
                tempArg++;
                locForGlobalValOfInt++;
            }
            tempArg = 0;
        }
        globalStrValOfInt[locForGlobalValOfInt] = '\0';
        *counter = length;
        *counterNew = strlen(globalStrValOfInt);
        return globalStrValOfInt;
    }
    else if (totalNum != 0)
    {
        locationInfo = totalNum + 1 + counterForShift;
        
        if (locationInfo >= margc)
        {
            while (*origBuffLoc != '\0')
            {
                globalStrValOfInt[locForGlobalValOfInt] = *origBuffLoc;
                locForGlobalValOfInt++;
                origBuffLoc++;
                length++;
            }
            globalStrValOfInt[locForGlobalValOfInt] = '\0';
            *counter = length;
            *counterNew = strlen(globalStrValOfInt);
            return globalStrValOfInt;
        }
        char tempCpyMargv[1024];
        char * ptrToTemp = tempCpyMargv;
        int tempArgLen = strlen(margv[locationInfo]);
        memcpy(tempCpyMargv, margv[locationInfo], strlen(margv[locationInfo]));
        ptrToTemp[tempArgLen] = '\0';
        while (ptrToTemp[tempArg] != '\0')
        {
            globalStrValOfInt[locForGlobalValOfInt] = ptrToTemp[tempArg];
            ptrToTemp++;
            locForGlobalValOfInt++;
        }
        tempArg = 0;
        
        while (*origBuffLoc != '\0')
        {
            globalStrValOfInt[locForGlobalValOfInt] = *origBuffLoc;
            locForGlobalValOfInt++;
            origBuffLoc++;
            length++;
        }
        globalStrValOfInt[locForGlobalValOfInt] = '\0';
        *counter = length;
        *counterNew = strlen(globalStrValOfInt);
        return globalStrValOfInt;
    }
    else
    {
        printf("Error: Something has occured\n");
        return NULL;
    }
    return globalStrValOfInt;
}

char * wildCardExpand (char * origBuffLoc, char * newBuff, int * counter, int * counterNew)
{
    struct dirent *Dirent;
    DIR *dir;
    char cwd[1024];
    int printDir;
    int loc = 0;
    int lengthOfBuf = 0;
    //printf("OrigBuff is, %s\n", origBuffLoc);
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        //printf("%s\n", cwd);
    }
    
    dir = opendir(cwd);
    if (dir == NULL)
    {
        printf ("Cannot open direcotry\n");
        return NULL;
    }
    /*
    if (whatInz == 7)
    {
        globalStrValOfInt[loc] = '"';
        loc++;
    }
    if (whatInz == 8)
    {
        globalStrValOfInt[loc] = ' ';
        loc++;
    }
    */
    bool matches = false;
    while ((Dirent = readdir(dir)) != NULL)
    {
        printDir = comparisionFunc(origBuffLoc, Dirent->d_name);
        if (printDir == 0)
        {
            matches = true;
            int len = 0; 
            /*
            while (Dirent->d_name[len] != 0)
            {
                printf("%c\n", Dirent->d_name[len]);
                len++;  
            }
            */
            
            while (Dirent->d_name[len] != '\0')
            {
                //printf("here\n");
                globalStrValOfInt[loc] = Dirent->d_name[len];
                loc++;
                len++;
            }
            globalStrValOfInt[loc] = ' ';
            loc++;
                                //printf ("%s\n", Dirent->d_name);
        }
        else if (printDir == 3)
        {
            printf("Error: / detected\n");
            break;
        }
        //i++;
    }
    if (matches == false)
    {
        bool dontRun = true;
        globalStrValOfInt[loc] = '*';
        loc++;
        while (*origBuffLoc != ' ' && *origBuffLoc != '\0')
        {
                                //printf("%c\n", *origBuffLoc);
            if (*origBuffLoc == '"')
            {
                dontRun = false;
            }
            globalStrValOfInt[loc] = *origBuffLoc;
            origBuffLoc++;
            lengthOfBuf++;
            loc++;
        }
        *counter = lengthOfBuf;
        globalStrValOfInt[loc] = '\0';
        if (dontRun == true)
        {
            if (isQuote == true)
            {
                globalStrValOfInt[loc] = '"';
                loc++;
                globalStrValOfInt[loc] = '\0';
                isQuote = false;
            }
        }
        *counterNew = strlen(globalStrValOfInt);
        closedir(dir);
                            //printf("%s\n", globalStrValOfInt);
        return globalStrValOfInt;
    }
    loc--;
    globalStrValOfInt[loc] = '\0';
    while (*origBuffLoc != ' ' && *origBuffLoc != '\0')
    {
        origBuffLoc++;
        lengthOfBuf++;
    }
    *counter = lengthOfBuf;
    if (isQuote == true)
    {
        globalStrValOfInt[loc] = '"';
        loc++;
        globalStrValOfInt[loc] = '\0';
        isQuote = false;
    }
    *counterNew = strlen(globalStrValOfInt);
    closedir(dir);
                    //printf("globalBuf is: %s\n", globalStrValOfInt);
    return globalStrValOfInt;
}





int comparisionFunc(char * comparBuf, char * dirBuf)
{
                        //printf("Comparison is,%s\n", comparBuf);
                        //printf("Directory is, %s\n", dirBuf);
    int lenOfBuff = 0;
    while (comparBuf[lenOfBuff] != '\0' && comparBuf[lenOfBuff] != '"' && comparBuf[lenOfBuff] != ' ')
    {
        lenOfBuff++;
    }
    if (comparBuf[lenOfBuff] == '"')
    {
        isQuote = true;
    }
                        //printf("lenOfBuff: %d\n", lenOfBuff);
    //= strlen(comparBuf);
    int lenOfDir = strlen(dirBuf);
                        //printf("lenOfDir: %d\n", lenOfDir);
                        
    int moveToLen = lenOfDir - lenOfBuff;
                        //printf("moveToLen: %d\n", moveToLen);
    if (moveToLen < 0)
    {
                        //printf("Returned 2\n");
        return (2);
    }
    if (*comparBuf == '"')
    {
        lenOfBuff--;
    }
    if (*comparBuf == ' ' || *comparBuf == '\0' || *comparBuf == '"')
    {
                        //printf("Returned 0 no conditions\n");
        return(0);
    }
    if (*dirBuf == '.')
    {
                        //printf("Returned 1 hidden file");
        return 1;
    }
    
                                            //printf("%c\n", dirBuf[moveToLen]);
                                            //printf("%c\n", *comparBuf);
    while ((*comparBuf != '\0' || *comparBuf != ' ') && dirBuf[moveToLen] != 0)
    {
        if (*comparBuf == '/')
        {
                        //printf("We go here?\n");
            return(3);
        }
        else if (*comparBuf == dirBuf[moveToLen])
        {
            comparBuf++;
            moveToLen++;
        }
        if (*comparBuf == '"')
        {
                        //printf("Returned 0 if quote and EOF char\n");
            return(0);
        }
        else if (*comparBuf == ' ' && dirBuf[moveToLen] == '\0')
        {
            return(0);
        }
        else if (*comparBuf != dirBuf[moveToLen])
        {
                        //printf("%c\n", dirBuf[moveToLen]);
                        //printf("%c\n", *comparBuf);
                        //printf("Returned 1 failed\n");
            return(1);
        }
    }
    if (*comparBuf == 0 && dirBuf[moveToLen] == 0)
    {
                        //printf("Returned 0 successful both zero\n");
        return(0);
    }
    return(1);
}

char * wildCardPrint (char * origBuffLoc, char * newBuff, int * counter, int * counterNew)
{
    int loc = 0;
    globalStrValOfInt[loc] = '*';
    loc++;
    globalStrValOfInt[loc] = '\0';
    *counterNew = strlen(globalStrValOfInt);
    return globalStrValOfInt;
}

char * expandHomeDir (char * origBuffLoc, char * newBuff, int * counter, int * counterNew)
{
    struct passwd *pwd;
    char buf[1024];
    char * ptrToBuf = buf;
    int loc = 0;
    int counterOld = 0;
    char buffer[1024];
    int counterf = 0;
    char * ptrToBuffer = buffer;
    if (*origBuffLoc == '~')
    {
        globalStrValOfInt[counterf] = '~';
        counterf++;
        while(*origBuffLoc != ' ' && *origBuffLoc != '\0')
        {
            globalStrValOfInt[counterf] = *origBuffLoc;
            counterf++;
            origBuffLoc++;
            counterOld++;
        }
        globalStrValOfInt[counterf] = '\0';
        *counter = counterOld;
        *counterNew = strlen(globalStrValOfInt);
        return globalStrValOfInt;
    }
    if (*origBuffLoc == ' ' || *origBuffLoc == '\0' || *origBuffLoc == '/' || *origBuffLoc == '~')
    {
        pwd = getpwuid(getuid());
        strcpy(buf, pwd->pw_dir);
        while(ptrToBuf[counterf] != '\0')
        {
            globalStrValOfInt[counterf] = ptrToBuf[counterf];
            counterf++;
        }
        while(*origBuffLoc != ' ' && *origBuffLoc != '\0')
        {
            globalStrValOfInt[counterf] = *origBuffLoc;
            counterf++;
            origBuffLoc++;
            counterOld++;
        }
        globalStrValOfInt[counterf] = '\0';
        *counter = counterOld;
        *counterNew = strlen(globalStrValOfInt);
        return globalStrValOfInt;
    }
    while (*origBuffLoc != ' ' && *origBuffLoc != '\0' && *origBuffLoc != '/' && *origBuffLoc != '~')
    {
        ptrToBuffer[loc] = *origBuffLoc;
        counterOld++;
        loc++;
        origBuffLoc++;
    }
    if (*origBuffLoc == '~')
    {
        int countera = 0;
        globalStrValOfInt[counterf] = '~';
        counterf++;
        while(ptrToBuffer[countera] != '\0')
        {
            globalStrValOfInt[counterf] = ptrToBuffer[countera];
            counterf++;
            countera++;
        }
        while(*origBuffLoc != ' ' && *origBuffLoc != '\0')
        {
            globalStrValOfInt[counterf] = *origBuffLoc;
            counterf++;
            origBuffLoc++;
            counterOld++;
        }
        globalStrValOfInt[counterf] = '\0';
        *counter = counterOld;
        *counterNew = strlen(globalStrValOfInt);
        return globalStrValOfInt;
    }
    ptrToBuffer[loc] = '\0';
    pwd = getpwnam(buffer);
    strcpy(buf, pwd->pw_dir);
    int ctrForBuf = 0;
    while(ptrToBuf[ctrForBuf] != '\0')
    {
        globalStrValOfInt[ctrForBuf] = ptrToBuf[ctrForBuf];
        ctrForBuf++;
    }
    while(*origBuffLoc != ' ' && *origBuffLoc != '\0')
    {
        globalStrValOfInt[ctrForBuf] = *origBuffLoc;
        ctrForBuf++;
        origBuffLoc++;
        counterOld++;
    }
    globalStrValOfInt[ctrForBuf] = '\0';
    *counter = counterOld;
    //printf("%s\n", globalStrValOfInt);
    *counterNew = strlen(globalStrValOfInt);
    return globalStrValOfInt;
}


char * commandExpansion (char * origBuffLoc, char * newBuff, int * counter, int * counterNew)
{
    int matchingBrace = 1;
    char cmdExpandBuf[1024];
    char ecmdExpandBuf[1024];
    char importToBuff[1024];
    int fileDescriptors[2];
    char * ptrTocmdExpandBuf = cmdExpandBuf;
    int ctrForcmdExp = 0;
    while (*origBuffLoc != '\0' || matchingBrace < 1)
    {
        ptrTocmdExpandBuf[ctrForcmdExp] = *origBuffLoc;
        if (*origBuffLoc == '(')
        {
            matchingBrace++;
        }
        else if (*origBuffLoc == ')' && matchingBrace == 1)
        {
            int functional;
            *origBuffLoc = '\0';
            if (pipe(fileDescriptors) == -1)
            {
                printf("Error\n");
            }
            functional = processLine(cmdExpandBuf, ecmdExpandBuf, fileDescriptors);
            if (functional == 1)
            {
                printf("Error \n");
            }
            close(fileDescriptors[1]);
            read (fileDescriptors[0], importToBuff, 1024);
            close(fileDescriptors[0]);
            *origBuffLoc = ')';
            origBuffLoc++;
        }
        else if (*origBuffLoc == ')' && matchingBrace > 1)
        {
            matchingBrace--;
        }
        origBuffLoc++;
    }
    if (matchingBrace != 1)
    {
        printf("Matching parentheses not found\n");
        return NULL;
    }
    /*
    if (pipe(fd) == -1)
    {
        printf("Error");
    }
    */
    return globalStrValOfInt;
}






































