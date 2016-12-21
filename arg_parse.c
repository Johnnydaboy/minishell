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
    int quoteCount = 0;
    bool inArg = false;
    bool inQuote = false;
    int len = strlen(line) + 1;
    for (int numOfQuotes = 0; numOfQuotes < len; numOfQuotes++)
    {
        if (line[numOfQuotes] == '"')
        {
            quoteCount = quoteCount + 1;
        }
        
    }
    if ( quoteCount % 2 == 0)
    {
        for (int i = 0; i < len; i++) 
        {
            if (line[i] == '"')
            {
                if (inArg == false && inQuote == false)
                {
                    inArg = true;
                    inQuote = true;
                }
                else if (inArg == true && inQuote == true)
                {
                    inArg = true;
                    inQuote = false;
                }
                else if (inArg == true && inQuote == false)
                {
                    inQuote = true;
                }
            }
            else if (line[i] == ' ' || line[i] == 0)
            {
                if (inArg == true && inQuote == false)
                {
                    inArg = false;
                    counter = counter + 1;
                }
            }
            else if (line[i] != ' ')
            {
                if (inArg == false)
                {
                    inArg = true;
                }
            }
        }
    }
    else
    {
        perror("You can't have an odd number of quotes \n");
    }        
    counter = counter + 1;
    *argcp = counter;
    printf("You have this many arguments: %d\n", counter);
    return NULL;    
    
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

