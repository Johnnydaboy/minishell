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
        printf("You can't have an odd number of quotes \n");
        return NULL;
    }        
    counter = counter + 1;
    *argcp = counter;
    //printf("You have this many arguments: %d\n", counter);   
    
    char ** ptrToStrArr = (char** ) malloc(sizeof (char*) * counter);
    inArg = false;
    inQuote = false;
    int ptrToStrArrCounter = 0;
    char * ptr1 = line;
    char * ptr2 = line;

    while (*ptr2 != 0)
    {
        *ptr1 = *ptr2;
        if (*ptr2 == '"')
        {
            if (inArg == false)
            {
                ptrToStrArr[ptrToStrArrCounter] = ptr1;
                ptrToStrArrCounter++;
                inArg = true;
            }
            ptr2++;
            inQuote = !inQuote;
        }
        else
        {
            if (*ptr2 != ' ' && inArg == false)
            {
                ptrToStrArr[ptrToStrArrCounter] = ptr1;
                ptrToStrArrCounter++;
                inArg = true;
            }
            
            else if (*ptr2 == ' ')
            {
                if (inQuote == false)
                {
                    *ptr1 = 0;
                    inArg = false;
                }
            }
            ptr1++;
            ptr2++;
        }
    }
    *ptr1 = *ptr2;
    ptrToStrArr[ptrToStrArrCounter] = NULL;
    //for(int g = 0; g < ptrToStrArrCounter; g++)
    //{
    //    printf("Your char is: %s\n", ptrToStrArr[g]);
    //}
    
    return ptrToStrArr;
}

