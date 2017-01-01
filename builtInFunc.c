#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>

#include "proto.h"

void exitBuiltIn (char **line);
void aechoBuiltIn (char **line);
void envsetBuiltIn (char **line);
void envunsetBuiltIn (char **line);

// This compares the string at line[0] to the built in function program name in my builtIns array
// It has a return type of bool in order to make sure processline doesnt run
bool builtInFunc (char **line, int args)
{
    int n = 4;
    char *builtIns[n];
    builtIns[0] = "exit";
    builtIns[1] = "aecho";
    builtIns[2] = "envset";
    builtIns[3] = "envunset";
    typedef void (*funcBuiltIn)(char **line);
    funcBuiltIn funcBuiltInArr[n];
    funcBuiltInArr[0] = exitBuiltIn;
    funcBuiltInArr[1] = aechoBuiltIn;
    funcBuiltInArr[2] = envsetBuiltIn;
    funcBuiltInArr[3] = envunsetBuiltIn;
    
    int c;
    for (c = 0; c < n; c++)
    {
        /* If it is built in since the index of the builtIns array and the function array are at identical
        we can use the variable c in order to call the correct function pointer to call the function */
        if (strcmp(line[0], builtIns[c]) == 0 )
        {
            (*funcBuiltInArr[c])(&line[1], args - 1);
            return true;
        }
    }
    return false;
}

// This is the first built in command which exits the minishell with the value given or 0
void exitBuiltIn (char **line, int numArgs)
{
    int intStr;
    if (line[0] == 0)
    {
        exit(0);
    }
    else if (line[1] != 0)
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


void envsetBuiltIn (char **line, int numArgs)
{
    if (line[1] == 0)
    {
        printf("You have nothing to set");
    }
    else if (line[2] != 0)
    {
        printf("You have too many arguments\n");
    }
    else
    {
        setenv(line[0], line[1], 1);
    }
}


void envunsetBuiltIn (char **line, int numArgs)
{
    if (line[0] == 0)
    {
        printf("You have nothing to remove");
    }
    else if (line[1] != 0)
    {
        printf("You have too many arguments\n");
    }
    unsetenv(line[0]);
}
