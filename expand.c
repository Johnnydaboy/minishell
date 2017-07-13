#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include "proto.h"
#include <dirent.h>

#include "globals.h"

bool normalExit = true;
int whatInz;
char globalStrValOfInt[1024];
bool ranEnv = false;
int findInz (char * orig, char * Inz);
char * dollarSignCurlyBrace (char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
char * dollarSignDollarSign (char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
char * dollarSignPoundSign (char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
char * dollarSignN (char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
char * dollarSignQuestionMark (char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
//bool match (const char *pattern, const char *candidate, int p, int c);
//char **getResult(char *pattern, char *folderName, int *count);
char * wildCardExpand (char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
char * wildCardFail (char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
char * wildCardPrint (char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
int comparisionFunc(char * comparBuf, char * dirBuf);
// This function takes in two character arrays and writes to the new buffer by reading from the old buffer, newsize is the length of array new
// In a failure case this function will return a 0 and otherwise it returns 1
int expand (char *orig, char *new, int newsize)
{
    int lenOfFuncArr = 10;
    char *Inz[lenOfFuncArr];
    Inz[0] = "${";
    Inz[1] = "$$";
    Inz[2] = "$#";
    Inz[3] = "$?";
    Inz[4] = "$";
    Inz[5] = "\\*";
    Inz[6] = "**";
    Inz[7] = "\"*";
    Inz[8] = "\"\"*";
    Inz[9] = "*";
    int whereIsInz = 0;
    int whereIsNew = 0;
    typedef char *(*funcInz)(char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
    funcInz funcInzArr[lenOfFuncArr];
    funcInzArr[0] = dollarSignCurlyBrace;
    funcInzArr[1] = dollarSignDollarSign;
    funcInzArr[2] = dollarSignPoundSign;
    funcInzArr[3] = dollarSignQuestionMark;
    funcInzArr[4] = dollarSignN;
    funcInzArr[5] = wildCardPrint;
    funcInzArr[6] = wildCardFail;
    funcInzArr[7] = wildCardExpand;
    funcInzArr[8] = wildCardExpand;
    funcInzArr[9] = wildCardExpand;
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
                                            //printf("whatInz is: %d\n", whatInz);
                                            //printf("copyOver is: %s\n", copyOver);
                // If it fail and no } is found it will print an error statement
                
                while (inputNew >= 1000)
                {
                    printf("CopyOver: %s\n", copyOver);
                    whatInz = inputNew - 1000;
                    printf("whatInz: %d\n", whatInz);
                    int countForMe = 0;
                    int quickCpy = strlen(copyOver);
                    char * quickCpyStr = (*funcInzArr[whatInz])(&orig[whereIsInz], new, &inputC, &inputNew);
                    printf("qcs = %s\n", quickCpyStr);
                    int countDown = strlen(quickCpyStr);
                    while (countDown != 0)
                    {
                        copyOver[quickCpy] = quickCpyStr[countForMe];
                        quickCpy++;
                        countForMe++;
                        countDown--;
                    }
                }
                
                
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
                    for (i = 0; i < inputNew; i++)
                    {
                        *new = *copyOver;
                        new++;
                        copyOver++;
                    }
                                                //printf("%s\n", new);
                    whatInz = 0;
                    memcpy(orig, new, strlen(new));
                }
            }
        }
        if (lenOfParam == 0)
        {
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
    while (*Inz != 0)
    {
        if (*orig != *Inz)
        {
            break;
        }
        orig++;
        Inz++;
        counter++;
    }
    if (*Inz == 0)
    {
        return counter;
    }
    return 0;
}

// This is the first expand subfunction which does and expand to ${} 
char * dollarSignCurlyBrace (char * origBuffLoc, char * newBuff, int * counter, int * counterNew)
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
    ranEnv = true;
    return envstr;
}

// The second expand subfunction which expands the $$ environment
char * dollarSignDollarSign (char * origBuffLoc, char * newBuff, int * counter, int * counterNew)
{
    int envint = getppid();
    sprintf(globalStrValOfInt,"%d", envint);
    int cNew = strlen(globalStrValOfInt);
    *counterNew = cNew;
    return globalStrValOfInt;
}

char * dollarSignPoundSign (char * origBuffLoc, char * newBuff, int * counter, int * counterNew)
{
    int argsHere = 0;
    //printf("%d\n", margc);
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
    sprintf(globalStrValOfInt, "%d", argsHere);
    *counterNew = strlen(globalStrValOfInt);
    return globalStrValOfInt;
}

char * dollarSignQuestionMark (char * origBuffLoc, char * newBuff, int * counter, int * counterNew)
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








char * dollarSignN (char * origBuffLoc, char * newBuff, int * counter, int * counterNew)
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
    else if (*origBuffLoc == '{')
    {
        origBuffLoc++;
        *counter = 2;
        *counterNew = 1000;
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
        //i++;
    }
    if (matches == false)
    {
        globalStrValOfInt[loc] = '*';
        loc++;
        while (*origBuffLoc != ' ' && *origBuffLoc != '"' && *origBuffLoc != '\0')
        {
            globalStrValOfInt[loc] = *origBuffLoc;
            origBuffLoc++;
            lengthOfBuf++;
            loc++;
        }
        *counter = lengthOfBuf;
        *counterNew = strlen(globalStrValOfInt);
        closedir(dir);
        return globalStrValOfInt;
    }
    loc--;
    globalStrValOfInt[loc] = '\0';
    while (*origBuffLoc != ' ' && *origBuffLoc != '"' && *origBuffLoc != '\0')
    {
        origBuffLoc++;
        lengthOfBuf++;
    }
    *counter = lengthOfBuf;
    *counterNew = strlen(globalStrValOfInt);
    closedir(dir);
    return globalStrValOfInt;
}

int comparisionFunc(char * comparBuf, char * dirBuf)
{
    int lenOfBuff = strlen(comparBuf);
    int lenOfDir = strlen(dirBuf);
    if (whatInz == 7)
    {
        lenOfBuff--;
    }
    int moveToLen = lenOfDir - lenOfBuff;
    if (moveToLen < 0)
    {
        return (2);
    }
    if (*comparBuf == ' ' || *comparBuf == 0)
    {
        return(0);
    }
    if (*comparBuf == '.')
    {
        return 1;
    }
    while ((*comparBuf != 0 || *comparBuf != ' ') && dirBuf[moveToLen] != 0)
    {
        if (*comparBuf == '/')
        {
            printf("Error: / detected\n");
            return 1;
        }
        else if (*comparBuf == dirBuf[moveToLen])
        {
            comparBuf++;
            moveToLen++;
        }
        if (*comparBuf == '"' && dirBuf[moveToLen] == '\0')
        {
            return(0);
        }
        else if (*comparBuf != dirBuf[moveToLen])
        {
            return 1;
        }
    }
    if (*comparBuf == 0 && dirBuf[moveToLen] == 0)
    {
        return(0);
    }
    return(1);
}

char * wildCardFail (char * origBuffLoc, char * newBuff, int * counter, int * counterNew)
{
        int loc = 0;
        int lengthOfBuf = 0;
        globalStrValOfInt[loc] = '*';
        loc++;
        globalStrValOfInt[loc] = '*';
        loc++;
        while (*origBuffLoc != ' ' && *origBuffLoc != '"' && *origBuffLoc != '\0')
        {
            globalStrValOfInt[loc] = *origBuffLoc;
            origBuffLoc++;
            lengthOfBuf++;
            loc++;
        }
        globalStrValOfInt[loc] = '\0';
        *counter = lengthOfBuf;
        *counterNew = strlen(globalStrValOfInt);
        return globalStrValOfInt;
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

/*
char * dollarSignDollarSign (char * origBuffLoc, char * newBuff, int * counter, int * counterNew)
{
    int envint = getppid();
    sprintf(globalStrValOfInt,"%d", envint);
    int cNew = strlen(globalStrValOfInt);
    *counterNew = cNew;
    return globalStrValOfInt;
}
*/
