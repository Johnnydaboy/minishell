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

#include "proto.h"
#include "globals.h"

//int counterTillZero = 0;
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


// This compares the string at line[0] to the built in function program name in my builtIns array
// It has a return type of bool in order to make sure processline doesnt run
bool builtInFunc (char **line, int args)
{
    int n = 8;
    char *builtIns[n];
    builtIns[0] = "exit";
    builtIns[1] = "aecho";
    builtIns[2] = "envset";
    builtIns[3] = "envunset";
    builtIns[4] = "cd";
    builtIns[5] = "shift";
    builtIns[6] = "unshift";
    builtIns[7] = "sstat";
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
        //valueForExit = intStr;
        //printf("%d\n", valueForExit);
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
                printf("%s", line[counterb]);
                fflush(stdout);
                finish = true;
            }
            else if (line[counterb] != 0)
            {
                printf("%s ", line[counterb]);
                fflush(stdout);
                counterb++;
            }
        }
    }
    
    // This is the implementation for the standard of echo
    else 
    {
        while (finish == false)
        {
            if (line[counter] == 0)
            {
                printf("\n ");
                finish = true;
            }
            else
            {
                printf("%s ", line[counter]);
                counter++;
            }
        }
    }    
}

// Another built in command the third to be percise, sets environment variables
void envsetBuiltIn (char **line, int numArgs)
{
    if (numArgs != 2)
    {
        normalExit = false;
        printf("Error: Invalid number of arguments for envset\n");
        return;
    }
    
    setenv(line[0], line[1], 1);
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
    //if (enterShift == false)
    //{
    //    //counterTillZero = margc;
    //    enterShift = true;
    //}
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
        //counterTillZero--;
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
        //counterTillZero = counterTillZero - shiftBy;
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
        //counterTillZero = margc;
        counterForShift = 0;
        //printf("locationa: %s\n", margv[0]);
    }
    else if (numArgs == 1)
    {
        unshiftby = atoi (line[0]);
        //printf("unshift by is %d\n", unshiftby);
        if (unshiftby > (counterForShift - 2))
        {
            normalExit = false;
            printf("Undershifting please change the number to shift\n");
            return;
        }
        counterForShift = counterForShift - unshiftby;
        //counterTillZero = counterTillZero + unshiftby;
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
    //printf("fail is %d\n", fail);
    struct tm *info;
    struct passwd *pwd;
    struct group *grp;
    time_t get_mtime;
    char timeBuff[100];
    printf("%s\t", filename);
    printf( (S_ISDIR(fileStat.st_mode)) ? "d" : "-");
    printf( (fileStat.st_mode & S_IRUSR) ? "r" : "-");
    printf( (fileStat.st_mode & S_IWUSR) ? "w" : "-");
    printf( (fileStat.st_mode & S_IXUSR) ? "x" : "-");
    printf( (fileStat.st_mode & S_IRGRP) ? "r" : "-");
    printf( (fileStat.st_mode & S_IWGRP) ? "w" : "-");
    printf( (fileStat.st_mode & S_IXGRP) ? "x" : "-");
    printf( (fileStat.st_mode & S_IROTH) ? "r" : "-");
    printf( (fileStat.st_mode & S_IWOTH) ? "w" : "-");
    printf( (fileStat.st_mode & S_IXOTH) ? "x" : "-");
    printf("\t%ld", fileStat.st_nlink);

    pwd = getpwuid(fileStat.st_uid);
    printf("\t%s", pwd->pw_name);

    grp = getgrgid(fileStat.st_gid);
    printf("\t%s", grp->gr_name);
    printf("\t%ld", fileStat.st_size);

    get_mtime = fileStat.st_mtime;
    info = localtime(&get_mtime);
    strftime(timeBuff, sizeof(timeBuff), "%d.%m.%Y %H:%M:%S", info);
    printf("\t%s\n", timeBuff);
    return 0;
}

void sstat (char **line, int numArgs)
{
    int loopArgs = 0;
    /*
    if (numArgs != 1)
    {
        printf("External error detected, please change number of variable to something applicable\n");
        return;
    }
    */
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

/*
int fileExists (char *filename)
{
    struct stat fileStat;
    return (stat (filename, &file))
}
*/
