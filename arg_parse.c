#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include "proto.h"

char ** arg_parse (char *line, int *argcp)
{
    int counter = 0;
    bool inArg = false;
    int len = strlen(line) + 1;
    
    for (int i = 0; i < len; i++) 
    {
        if (line[i] == ' ' || line[i] == 0)
        {
            if (inArg == true)
            {
                counter = counter + 1;
                inArg = false;
            }
        }
        else if (line[i] != ' ')
        {
            inArg = true;
        }
    }
    counter = counter + 1;
    *argcp = counter; 
    
    
    char ** ptrToStrArr = (char** ) malloc(sizeof (char*) * counter);
    inArg = false;
    int ptrToStrArrCounter = 0;
    for (int i = 0; i < len; i++)
    {
       if (line[i] == ' ' || line[i] == 0)
       {
           line[i] = 0;
           inArg = false;
       }
       else if (line[i] != ' ')
       {
           if (inArg == false)
           {
               ptrToStrArr[ptrToStrArrCounter] = &line[i];
               ptrToStrArrCounter = ptrToStrArrCounter + 1;
               inArg = true;
           }
       }
    }
    ptrToStrArr[ptrToStrArrCounter] = NULL;
    
    
    return ptrToStrArr;
}

