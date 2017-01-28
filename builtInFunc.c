#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>

#include "proto.h"

void exitBuiltIn (char **line, int args);
void aechoBuiltIn (char **line, int args);
void envsetBuiltIn (char **line, int args);
void envunsetBuiltIn (char **line, int args);
void chdirBuiltIn (char **line, int args);

// This compares the string at line[0] to the built in function program name in my builtIns array
// It has a return type of bool in order to make sure processline doesnt run
bool builtInFunc (char **line, int args)
{
    typedef void (*funcBuiltIn)(char **line, int args);
    int n = 5;
    char *builtIns[n];
    funcBuiltIn funcBuiltInArr[n];
    
    builtIns[0] = "exit"; funcBuiltInArr[0] = exitBuiltIn;
    builtIns[1] = "aecho"; funcBuiltInArr[1] = aechoBuiltIn;
    builtIns[2] = "envset"; funcBuiltInArr[2] = envsetBuiltIn;
    builtIns[3] = "envunset"; funcBuiltInArr[3] = envunsetBuiltIn;
    builtIns[4] = "cd"; funcBuiltInArr[4] = chdirBuiltIn;
    
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
    
    if (numArgs == 0)
    {
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
            printf("Directory does not exist\n");
        }
        return;
    }
    printf("Error: Invalid number of arguments for cd\n");
    return;
    chdir(line[0]);
}