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
#include <signal.h>
#include <pwd.h>
#include <grp.h>

#include "proto.h"
#include "globals.h"

typedef int(*funcInz)(char * origBuffLoc, char * newBuff, int * counter, int * counterNew);

bool normalExit = true;
int whatInz;
int whatDollar;
int findInz (char * orig, char * Inz);
bool isQuote;
int expandEnvVar (char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
int expandPid (char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
int expandNumArgs (char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
int expandLocOfArg (char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
int expandProcessId (char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
int wildCardExpand (char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
int wildCardPrint (char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
int comparisionFunc(char * comparBuf, char * dirBuf);
int expandHomeDir (char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
int commandExpansion (char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
// This function takes in two character arrays and writes to the new buffer by reading from the old buffer, newsize is the length of array new
// In a failure case this function will return a 0 and otherwise it returns 1
int expand (char *orig, char *new, int newsize)
{
    int lenOfFuncArr = 9;
    char *Inz[lenOfFuncArr];
    Inz[0] = "${";
    Inz[1] = "$$";
    Inz[2] = "$#";
    Inz[3] = "$?";
    Inz[4] = "$(";
    Inz[5] = "$";
    Inz[6] = "\\*";
    Inz[7] = "*";
    Inz[8] = "~";
    int whereIsInz = 0;
    int whereIsNew = 0;

    funcInz funcInzArr[lenOfFuncArr];
    funcInzArr[0] = expandEnvVar;
    funcInzArr[1] = expandPid;
    funcInzArr[2] = expandNumArgs;
    funcInzArr[3] = expandProcessId;
    funcInzArr[4] = commandExpansion;
    funcInzArr[5] = expandLocOfArg;
    funcInzArr[6] = wildCardPrint;
    funcInzArr[7] = wildCardExpand;
    funcInzArr[8] = expandHomeDir;
    int lenOfParam = 0;
    // This while loop will continue to execute until the orig string reads at 0 
    int moveOver = 1;
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
                moveOver = (*funcInzArr[whatInz])(&orig[whereIsInz], new, &inputC, &inputNew);
                if (moveOver == -1)
                {
                    return -1;
                }
                // If it fail and no } is found it will print an error statement
                if (inputNew == -1)
                {
                    dprintf(2, "${: No matching }");
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
                    dprintf(2, "Expand too big");
                    return 0; 
                }
                else
                {
                    new = new + inputNew;
                    whatInz = 0;
                    break;
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
    return moveOver;
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
int expandEnvVar (char * origBuffLoc, char * newBuff, int * counter, int * counterNew)
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
            return -1;
        }
    }
    *origBuffLoc = '\0';
    count++;
    *counter = count;
    
    if (count == 1)
    {
        return -1;
    }
    
    
    char * envstr = getenv(ptr2temp);
    if (envstr == NULL)
    {
        return -1;
    }
    int cpyover = 0;
    while (envstr[cpyover] != '\0')
    {
        newBuff[cpyover] = envstr[cpyover];
        cpyover++;
    }
    newBuff[cpyover] = '\0';
    *counterNew = cpyover;
    //printf("newBuff is %s\n", newBuff);
    return *counterNew;
}

// The second expand subfunction which expands the $$ environment
int expandPid (char * origBuffLoc, char * newBuff, int * counter, int * counterNew)
{
    int envint = getppid();
    sprintf(newBuff,"%d", envint);
    int cNew = strlen(newBuff);
    *counterNew = cNew;
    //printf("%s\n", newBuff);
    return *counterNew;
}

int expandNumArgs (char * origBuffLoc, char * newBuff, int * counter, int * counterNew)
{
    int argsHere = 0;
    if (margc == 1)
    {
        argsHere = margc;
        sprintf(newBuff, "%d", argsHere);
        *counterNew = strlen(newBuff);
        return *counterNew;
    }
    else
    {
        argsHere = margc - (counterForShift + 1);
    }
    sprintf(newBuff, "%d", argsHere);
    *counterNew = strlen(newBuff);
    return *counterNew;
}

int expandProcessId (char * origBuffLoc, char * newBuff, int * counter, int * counterNew)
{
    int envint;
    if (normalExit == true)
    {
        sprintf(newBuff, "%d", exitStatus);
        int cNew = strlen(newBuff);
        *counterNew = cNew;
        return *counterNew;
    }
    else if (normalExit == false)
    {
        envint = 1;
    }
    else
    {
        envint = 127;
    }
    sprintf(newBuff,"%d", envint);
    int cNew = strlen(newBuff);
    *counterNew = cNew;
    return *counterNew;
}






int expandLocOfArg (char * origBuffLoc, char * newBuff, int * counter, int * counterNew)
{
    int totalNum = 0;
    int length = 0;
    int locationInfo = 0;
    bool enterNum = false;
    int locForNewBuff = 0; 
    int tempArg = 0;
    if (*origBuffLoc == '\0' || *origBuffLoc == ' ' )
    {
        dprintf(2, "error here");
        return -1;
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
        newBuff[locForNewBuff] = '\0';
        return *counterNew;
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
                newBuff[locForNewBuff] = ptrToTemp[tempArg];
                tempArg++;
                locForNewBuff++;
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
                newBuff[locForNewBuff] = ptrToTemp[tempArg];
                tempArg++;
                locForNewBuff++;
            }
            tempArg = 0;
        }
        newBuff[locForNewBuff] = '\0';
        *counter = length;
        *counterNew = strlen(newBuff);
        return *counterNew;
    }
    else if (totalNum != 0)
    {
        locationInfo = totalNum + 1 + counterForShift;
        
        if (locationInfo >= margc)
        {
            while (*origBuffLoc != '\0')
            {
                newBuff[locForNewBuff] = *origBuffLoc;
                locForNewBuff++;
                origBuffLoc++;
                length++;
            }
            newBuff[locForNewBuff] = '\0';
            *counter = length;
            *counterNew = strlen(newBuff);
            return *counterNew;
        }
        char tempCpyMargv[1024];
        char * ptrToTemp = tempCpyMargv;
        int tempArgLen = strlen(margv[locationInfo]);
        memcpy(tempCpyMargv, margv[locationInfo], strlen(margv[locationInfo]));
        ptrToTemp[tempArgLen] = '\0';
        while (ptrToTemp[tempArg] != '\0')
        {
            newBuff[locForNewBuff] = ptrToTemp[tempArg];
            ptrToTemp++;
            locForNewBuff++;
        }
        tempArg = 0;
        
        while (*origBuffLoc != '\0')
        {
            newBuff[locForNewBuff] = *origBuffLoc;
            locForNewBuff++;
            origBuffLoc++;
            length++;
        }
        newBuff[locForNewBuff] = '\0';
        *counter = length;
        *counterNew = strlen(newBuff);
        return *counterNew;
    }
    else
    {
        dprintf(2,"Error: Something has occured");
        return -1;
    }
    return *counterNew;
}

int wildCardExpand (char * origBuffLoc, char * newBuff, int * counter, int * counterNew)
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
        perror ("Cannot open direcotry");
        return -1;
    }
    bool matches = false;
    while ((Dirent = readdir(dir)) != NULL)
    {
        printDir = comparisionFunc(origBuffLoc, Dirent->d_name);
        if (printDir == 0)
        {
            matches = true;
            int len = 0; 
            while (Dirent->d_name[len] != '\0')
            {
                newBuff[loc] = Dirent->d_name[len];
                loc++;
                len++;
            }
            newBuff[loc] = ' ';
            loc++;
        }
        else if (printDir == 3)
        {
            dprintf(2, "Error: / detected");
            //break;
            return -1;
        }
    }
    if (matches == false)
    {
        bool dontRun = true;
        newBuff[loc] = '*';
        loc++;
        while (*origBuffLoc != ' ' && *origBuffLoc != '\0')
        {
            if (*origBuffLoc == '"')
            {
                dontRun = false;
            }
            newBuff[loc] = *origBuffLoc;
            origBuffLoc++;
            lengthOfBuf++;
            loc++;
        }
        *counter = lengthOfBuf;
        newBuff[loc] = '\0';
        if (dontRun == true)
        {
            if (isQuote == true)
            {
                newBuff[loc] = '"';
                loc++;
                newBuff[loc] = '\0';
                isQuote = false;
            }
        }
        *counterNew = strlen(newBuff);
        closedir(dir);
        return *counterNew;
    }
    loc--;
    newBuff[loc] = '\0';
    while (*origBuffLoc != ' ' && *origBuffLoc != '\0')
    {
        origBuffLoc++;
        lengthOfBuf++;
    }
    *counter = lengthOfBuf;
    if (isQuote == true)
    {
        newBuff[loc] = '"';
        loc++;
        newBuff[loc] = '\0';
        isQuote = false;
    }
    *counterNew = strlen(newBuff);
    closedir(dir);
    return *counterNew;
}





int comparisionFunc(char * comparBuf, char * dirBuf)
{
    int lenOfBuff = 0;
    while (comparBuf[lenOfBuff] != '\0' && comparBuf[lenOfBuff] != '"' && comparBuf[lenOfBuff] != ' ')
    {
        lenOfBuff++;
    }
    if (comparBuf[lenOfBuff] == '"')
    {
        isQuote = true;
    }
    int lenOfDir = strlen(dirBuf);
    int moveToLen = lenOfDir - lenOfBuff;
    if (moveToLen < 0)
    {
        return(2);
    }
    if (*comparBuf == '"')
    {
        lenOfBuff--;
    }
    if (*comparBuf == ' ' || *comparBuf == '\0' || *comparBuf == '"')
    {
        return(0);
    }
    if (*dirBuf == '.')
    {
        return 1;
    }
    while ((*comparBuf != '\0' || *comparBuf != ' ') && dirBuf[moveToLen] != 0)
    {
        if (*comparBuf == '/')
        {
            return(3);
        }
        else if (*comparBuf == dirBuf[moveToLen])
        {
            comparBuf++;
            moveToLen++;
        }
        if (*comparBuf == '"')
        {
            return(0);
        }
        else if (*comparBuf == ' ' && dirBuf[moveToLen] == '\0')
        {
            return(0);
        }
        else if (*comparBuf != dirBuf[moveToLen])
        {
            return(1);
        }
    }
    if (*comparBuf == 0 && dirBuf[moveToLen] == 0)
    {
        return(0);
    }
    return(1);
}

int wildCardPrint (char * origBuffLoc, char * newBuff, int * counter, int * counterNew)
{
    int loc = 0;
    newBuff[loc] = '*';
    loc++;
    newBuff[loc] = '\0';
    *counterNew = strlen(newBuff);
    return *counterNew;
}






int expandHomeDir (char * origBuffLoc, char * newBuff, int * counter, int * counterNew)
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
        newBuff[counterf] = '~';
        counterf++;
        while(*origBuffLoc != ' ' && *origBuffLoc != '\0')
        {
            newBuff[counterf] = *origBuffLoc;
            counterf++;
            origBuffLoc++;
            counterOld++;
        }
        newBuff[counterf] = '\0';
        *counter = counterOld;
        *counterNew = strlen(newBuff);
        return *counterNew;
    }
    if (*origBuffLoc == ' ' || *origBuffLoc == '\0' || *origBuffLoc == '/' || *origBuffLoc == '~' || *origBuffLoc == '"')
    {
        pwd = getpwuid(getuid());
        strcpy(buf, pwd->pw_dir);
        while(ptrToBuf[counterf] != '\0')
        {
            newBuff[counterf] = ptrToBuf[counterf];
            counterf++;
        }
        while(*origBuffLoc != ' ' && *origBuffLoc != '\0')
        {
            newBuff[counterf] = *origBuffLoc;
            counterf++;
            origBuffLoc++;
            counterOld++;
        }
        newBuff[counterf] = '\0';
        *counter = counterOld;
        *counterNew = strlen(newBuff);
        return *counterNew;
    }
    while (*origBuffLoc != ' ' && *origBuffLoc != '\0' && *origBuffLoc != '/' && *origBuffLoc != '~' && *origBuffLoc != '"')
    {
        ptrToBuffer[loc] = *origBuffLoc;
        counterOld++;
        loc++;
        origBuffLoc++;
    }
    if (*origBuffLoc == '~')
    {
        int countera = 0;
        newBuff[counterf] = '~';
        counterf++;
        while(ptrToBuffer[countera] != '\0')
        {
            newBuff[counterf] = ptrToBuffer[countera];
            counterf++;
            countera++;
        }
        while(*origBuffLoc != ' ' && *origBuffLoc != '\0')
        {
            newBuff[counterf] = *origBuffLoc;
            counterf++;
            origBuffLoc++;
            counterOld++;
        }
        newBuff[counterf] = '\0';
        *counter = counterOld;
        *counterNew = strlen(newBuff);
        return *counterNew;
    }
    ptrToBuffer[loc] = '\0';
    pwd = getpwnam(buffer);
    if (pwd == NULL)
    {
        //printf("%s\n",buffer);
        int tocpy = 0;
        int tocpyNew = 0;
        newBuff[tocpyNew] = '~';
        tocpyNew++;
        while (buffer[tocpy] != '\0')
        {
            //printf("%c\n", buffer[tocpy]);
            newBuff[tocpyNew] = buffer[tocpy];
            //printf("%c\n", newBuff[tocpyNew]);
            tocpy++;
            tocpyNew++;
        }
        while(*origBuffLoc != '\0')
        {
            newBuff[tocpyNew] = *origBuffLoc;
            tocpyNew++;
            origBuffLoc++;
            counterOld++;
        }
        newBuff[tocpyNew] = '\0';
        *counter = counterOld;
        *counterNew = tocpyNew;
        //printf("%s\n", newBuff);
        return *counterNew;
    }
    strcpy(buf, pwd->pw_dir);
    int ctrForBuf = 0;
    while(ptrToBuf[ctrForBuf] != '\0')
    {
        newBuff[ctrForBuf] = ptrToBuf[ctrForBuf];
        ctrForBuf++;
    }
    while(*origBuffLoc != ' ' && *origBuffLoc != '\0')
    {
        newBuff[ctrForBuf] = *origBuffLoc;
        ctrForBuf++;
        origBuffLoc++;
        counterOld++;
    }
    newBuff[ctrForBuf] = '\0';
    *counter = counterOld;
    *counterNew = strlen(newBuff);
    return *counterNew;
}

int commandExpansion (char * origBuffLoc, char * newBuff, int * counter, int * counterNew)
{
    int sizeForBuf = 10;
    int matchingBrace = 1;
    int counterForOld = 0;
    int counterForNew = 0;
    char* ecmdExpandBuf = (char *) malloc (sizeof(char) * LINELEN);
    int fileDescriptors[2];
    //int ctrForcmdExp = 0;
    int findbrace = 0;
    while (origBuffLoc[findbrace] != '\0' && matchingBrace >= 1)
    {
        if (origBuffLoc[findbrace] == '(')
        {
            matchingBrace++;
        }
        else if (origBuffLoc[findbrace] == ')' && matchingBrace == 1)
        {
            origBuffLoc[findbrace] = '\0';
            if (pipe(fileDescriptors) == -1)
            {
                perror("Error: pipe");
            }
            int doWait = 1;
            int functional = processLine(origBuffLoc, ecmdExpandBuf, fileDescriptors, doWait);
            if (functional == 1)
            {
                dprintf(2, "Error: proLine");
                return -1;
            }
            close(fileDescriptors[1]);
            int closeBuffTotal = 0;
            int closeBuff;
            char * ptrToecmd = ecmdExpandBuf;
            while (closeBuff = read(fileDescriptors[0], ptrToecmd, sizeForBuf), closeBuff != 0)
            {
                closeBuffTotal = closeBuffTotal + closeBuff;
                ptrToecmd = ptrToecmd + closeBuff;
            }
            wait(NULL);
            ecmdExpandBuf[closeBuffTotal] = '\0';
            close(fileDescriptors[0]);
            
            origBuffLoc[findbrace] = ')';
            counterForOld++;
            findbrace++;
            matchingBrace--;
            break;
        }
        else if (origBuffLoc[findbrace] == ')' && matchingBrace > 1)
        {
            matchingBrace--;
        }
        findbrace++;
        counterForOld++;
    }
    if (matchingBrace > 0)
    {
        dprintf(2,"Matching parentheses not found");
        return -1;
    }
    int removeNewLine = 0;
    while (ecmdExpandBuf[removeNewLine] != '\0')
    {
        if (ecmdExpandBuf[removeNewLine] == '\n')
        {
            if (ecmdExpandBuf[removeNewLine + 1] == '\0')
            {
                ecmdExpandBuf[removeNewLine] = '\0';
            }
            else
            {
                ecmdExpandBuf[removeNewLine] = ' ';
            }
        }
        if (removeNewLine >= LINELEN)
        {
            dprintf(2,"Too many characters");
            return -1;
        }
        removeNewLine++;
    }
    //printf("%d\n", removeNewLine);
    int copyOver = 0;
    while (ecmdExpandBuf[copyOver] != '\0')
    {
        newBuff[copyOver] = ecmdExpandBuf[copyOver];
        counterForNew++;
        copyOver++;
    }
    *counterNew = counterForNew;
    *counter = counterForOld;
    newBuff[copyOver] = '\0';
    free (ecmdExpandBuf);
    return *counterNew;
}



































