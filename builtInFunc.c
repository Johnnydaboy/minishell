#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <sys/types.h>
#include <time.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <signal.h>

#include "proto.h"
#include "globals.h"

int fdInUse;
int counterForShift = 0;
bool enterShift = false;
void exitBuiltIn (char **line, int args);
void aechoBuiltIn (char **line, int args);
void envsetBuiltIn (char **line, int args);
void envunsetBuiltIn (char **line, int args);
void chdirBuiltIn (char **line, int args);
void shiftBuiltIn (char **line, int args);
void unShiftBuiltIn (char **line, int numArgs);
void sstat (char **line, int numArgs);
int DisplayToConsole(char *filename);
void readName(char **line, int numArgs);


// This compares the string at line[0] to the built in function program name in my builtIns array
// It has a return type of bool in order to make sure processline doesnt run
bool builtInFunc (char **line, int args, int * fd)
{
    fdInUse = fd[1];
    //After changing the 
    int n = 9;
    char *builtIns[n];
    builtIns[0] = "exit";
    builtIns[1] = "aecho";
    builtIns[2] = "envset";
    builtIns[3] = "envunset";
    builtIns[4] = "cd";
    builtIns[5] = "shift";
    builtIns[6] = "unshift";
    builtIns[7] = "sstat";
    builtIns[8] = "read";
    typedef void (*funcBuiltIn)(char **line, int args);
    funcBuiltIn funcBuiltInArr[n];
    funcBuiltInArr[0] = exitBuiltIn;
    funcBuiltInArr[1] = aechoBuiltIn;
    funcBuiltInArr[2] = envsetBuiltIn;
    funcBuiltInArr[3] = envunsetBuiltIn;
    funcBuiltInArr[4] = chdirBuiltIn;
    funcBuiltInArr[5] = shiftBuiltIn;
    funcBuiltInArr[6] = unShiftBuiltIn;
    funcBuiltInArr[7] = sstat;
    funcBuiltInArr[8] = readName;

    normalExit = true;
    int c;
    for (c = 0; c < n; c++)
    {
        /* If it is built in since the index of the builtIns array and the function array are at identical
        we can use the variable c in order to call the correct function pointer to call the function */
        if (strcmp(line[0], builtIns[c]) == 0 )
        {
            (*funcBuiltInArr[c])(&line[1], args - 2);
            return true;
        }
    }
    return false;
}

// This is the first built in command which exits the minishell with the value given or 0
void exitBuiltIn (char **line, int numArgs)
{
    int intStr;
    if (numArgs == 0)
    {
        exit(0);
    }
    else if (numArgs > 1)
    {
        normalExit = false;
        printf("You have too many arguments\n");
    }
    else
    {
        intStr = atoi (line[0]);
        exit(intStr);
    }
}

/* This is the second built in command which echo what every you type in and
if you give -n it won't echo on a new line */
void aechoBuiltIn (char **line, int numArgs)
{
    int counter = 0;
    int counterb = 1;
    bool finish = false;
    char bufferForWrite[1024];
    int lenOfStrLen = 0;
    
    if (numArgs == 0)
    {
        normalExit = false;
        return;
    }
    
    // This is the implementation for the -n for the echo
    if (strcmp(line[0], "-n") == 0)
    {
        while (finish == false)
        {
            if (line[counterb + 1] == 0)
            {
                lenOfStrLen = strlen(line[counterb]);
                snprintf(bufferForWrite, lenOfStrLen + 1, "%s", line[counterb]);
                write(fdInUse, bufferForWrite, lenOfStrLen);
                finish = true;
            }
            else if (line[counterb] != 0)
            {
                lenOfStrLen = strlen(line[counterb]);
                snprintf(bufferForWrite, lenOfStrLen + 2, "%s ", line[counterb]);
                write(fdInUse, bufferForWrite, lenOfStrLen + 1);
                counterb++;
            }
        }
    }
    
    // This is the implementation for the standard of echobodies
    else 
    {
        while (finish == false)
        {
            if (line[counter] == '\0')
            {
                snprintf(bufferForWrite, 2, "\n ");
                write(fdInUse, bufferForWrite, 2);
                finish = true;
            }
            else
            {
                lenOfStrLen = strlen(line[counter]);
                snprintf(bufferForWrite, lenOfStrLen + 2, "%s ", line[counter]);
                write(fdInUse, bufferForWrite, lenOfStrLen + 1);
                counter++;
            }
        }
        
    }
}

// Another built in command the third to be percise, sets environment variables
void envsetBuiltIn (char **line, int numArgs)
{
    int comparison = strcmp(line[0], "P1");
    int envCpy = 1;
    int len = 0;
    char envStr[1024];
    char tempLine[1024];
    while (envCpy != numArgs)
    {
        strcpy(tempLine, line[envCpy]);
        int len1 = 0;
        while (tempLine[len1] != '\0')
        {
            envStr[len] = tempLine[len1];
            len1++;
            len++;
        }
        envStr[len] = ' ';
        len++;
        envCpy++;
    }
    len--;
    envStr[len] = '\0';
    
    if (comparison == 0)
    {
        int lenOfPrompt = strlen(envStr);
        memcpy(prompt, envStr, lenOfPrompt);
    }
    setenv(line[0], envStr, 1);
}

// The fourth built in function unsets variables that have been set
void envunsetBuiltIn (char **line, int numArgs)
{
    if (numArgs != 1)
    {
        normalExit = false;
        printf("Error: Invalid number of arguments for envunset\n");
        return;
    }
    
    unsetenv(line[0]);
}

// The fifth built in function changes your current directory depending on the second argument
void chdirBuiltIn (char **line, int numArgs)
{   
    if (numArgs == 0)
    {
        char * homeDir = getenv("HOME");
        if (homeDir == 0)
        {
            normalExit = false;
            printf("Nothing in homeDir\n");
            return;
        }
        chdir(homeDir);
        return;
    }
    else if (numArgs == 1)
    {
        int isThere = chdir(line[0]);
        if (isThere == -1)
        {
            normalExit = false;
            printf("Directory does not exist\n");
        }
        return;
    }
    printf("Error: Invalid number of arguments for cd\n");
    return;
    chdir(line[0]);
}

void shiftBuiltIn (char **line, int numArgs)
{
    int shiftBy;
    if (margc <= 2)
    {
        normalExit = false;
        printf("Nothing to shift here\n");
        return;
    }
    if (numArgs == 0)
    {
        counterForShift++;
    }
    else if (numArgs == 1)
    {
        shiftBy = atoi (line[0]);
        if (shiftBy > ((margc - 2) - counterForShift))
        {
            printf("Overshifting please change the number to shift\n");
            normalExit = false;
            return;
        }
        counterForShift = counterForShift + shiftBy;
    }
    else 
    {
        printf("Error: Invalid number of arguments for shift\n");
        normalExit = false;
        return;
    }
}

void unShiftBuiltIn (char **line, int numArgs)
{
    int unshiftby;
    if (numArgs == 0)
    {
        counterForShift = 0;
    }
    else if (numArgs == 1)
    {
        unshiftby = atoi (line[0]);
        if (unshiftby > (counterForShift - 2))
        {
            normalExit = false;
            printf("Undershifting please change the number to shift\n");
            return;
        }
        counterForShift = counterForShift - unshiftby;
    }
    else
    { 
        normalExit = false;
        printf("Error: Invalid number of arguments for unshift\n");
        return;
    }     
}

int DisplayToConsole(char *filename)
{
    struct stat fileStat;
    int fail = stat(filename,&fileStat);
    if (fail == -1)
    {
        return 1;
    }
    struct tm *info;
    struct passwd *pwd;
    struct group *grp;
    time_t get_mtime;
    char timeBuff[100];
    char bufferForWrite[1024];
    int lenOfStrLen = 0;

    lenOfStrLen = strlen(filename);
    snprintf(bufferForWrite, lenOfStrLen + 1, "%s", filename);
    write(fdInUse, bufferForWrite, lenOfStrLen);

    pwd = getpwuid(fileStat.st_uid);
    lenOfStrLen = strlen(pwd->pw_name);
    snprintf(bufferForWrite, lenOfStrLen + 2, "\t%s", pwd->pw_name);
    write(fdInUse, bufferForWrite, lenOfStrLen + 1);
    
    grp = getgrgid(fileStat.st_gid);
    lenOfStrLen = strlen(grp->gr_name);
    snprintf(bufferForWrite, lenOfStrLen + 2, "\t%s", grp->gr_name);
    write(fdInUse, bufferForWrite, lenOfStrLen + 1);
    
    snprintf(bufferForWrite, 2, (S_ISDIR(fileStat.st_mode)) ? "\td" : "\t-");
    write(fdInUse, bufferForWrite, 1);
    snprintf(bufferForWrite, 2, (fileStat.st_mode & S_IRUSR) ? "r" : "-");
    write(fdInUse, bufferForWrite, 1);
    snprintf(bufferForWrite, 2, (fileStat.st_mode & S_IWUSR) ? "w" : "-");
    write(fdInUse, bufferForWrite, 1);
    snprintf(bufferForWrite, 2, (fileStat.st_mode & S_IXUSR) ? "x" : "-");
    write(fdInUse, bufferForWrite, 1);
    snprintf(bufferForWrite, 2, (fileStat.st_mode & S_IRGRP) ? "r" : "-");
    write(fdInUse, bufferForWrite, 1);
    snprintf(bufferForWrite, 2, (fileStat.st_mode & S_IWGRP) ? "w" : "-");
    write(fdInUse, bufferForWrite, 1);
    snprintf(bufferForWrite, 2, (fileStat.st_mode & S_IXGRP) ? "x" : "-");
    write(fdInUse, bufferForWrite, 1);
    snprintf(bufferForWrite, 2, (fileStat.st_mode & S_IROTH) ? "r" : "-");
    write(fdInUse, bufferForWrite, 1);
    snprintf(bufferForWrite, 2, (fileStat.st_mode & S_IWOTH) ? "w" : "-");
    write(fdInUse, bufferForWrite, 1);
    snprintf(bufferForWrite, 2, (fileStat.st_mode & S_IXOTH) ? "x" : "-");
    write(fdInUse, bufferForWrite, 1);

    lenOfStrLen = snprintf(NULL, 0, "%ld", fileStat.st_nlink);
    snprintf(bufferForWrite, lenOfStrLen + 2, "\t%ld", fileStat.st_nlink);
    write(fdInUse, bufferForWrite, lenOfStrLen + 1);
    
    lenOfStrLen = snprintf(NULL, 0, "%ld", fileStat.st_size);
    snprintf(bufferForWrite, lenOfStrLen + 2, "\t%ld", fileStat.st_size);
    write(fdInUse, bufferForWrite, lenOfStrLen + 1);
    
    get_mtime = fileStat.st_mtime;
    info = localtime(&get_mtime);
    strftime(timeBuff, sizeof(timeBuff), "%d.%m.%Y %H:%M:%S", info);
    lenOfStrLen = strlen(timeBuff);
    snprintf(bufferForWrite, lenOfStrLen + 3, "\t%s\n", timeBuff);
    write(fdInUse, bufferForWrite, lenOfStrLen + 2);
    return 0;
}

void sstat (char **line, int numArgs)
{
    int loopArgs = 0;
    while (line[loopArgs] != '\0')
    {
        int successOrFail = DisplayToConsole(line[loopArgs]);
        if (successOrFail == 1)
        {
            printf ("File or directory does not exist\n");
        }
        loopArgs++;
    }
    return;
}


void readName (char **line, int numArgs)
{
    if (numArgs != 1)
    {
        printf("Invalid number of arguments\n");
        return;
    }
    
    char envStr[1024];
    fgets (envStr, 1024, stdin);
    int removeNewL;
    while (envStr[removeNewL] != '\0')
    {
        if (envStr[removeNewL] == '\n')
        {
            envStr[removeNewL] = '\0';
        }
        removeNewL++;
    }
    setenv(line[0], envStr, 1);
}


