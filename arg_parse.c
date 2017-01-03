#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include "proto.h"
/* This function takes in a string from the input of the GUI and returns the seperates the arguments with 
memory locations and returns those memory locations */
char ** arg_parse (char *line, int *argcp)
{
    int counter = 0;
    int quoteCount = 0;
    bool inArg = false;
    bool inQuote = false;
    int len = strlen(line) + 1;
    
    // This checks how many quotes I have in the argument
    int numOfQuotes;
    for (numOfQuotes = 0; numOfQuotes < len; numOfQuotes++)
    {
        if (line[numOfQuotes] == '"')
        {
            quoteCount = quoteCount + 1;
        }
        
    }
    
    // If there is an odd number of quotes this won't run
    // This counts the total number of arguments I have along with quotes
    if ( quoteCount % 2 == 0)
    {
        int i;
        for (i = 0; i < len; i++) 
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
    
    // If you have an odd number of quotes it won't malloc the needed amount of memory
    else
    {
        printf("You can't have an odd number of quotes \n");
        return NULL;
    }        
    counter = counter + 1;
    *argcp = counter;
    
    // This mallocs the correct number of memory based on the number of arguments you have
    char ** ptrToStrArr = (char** ) malloc(sizeof (char*) * counter);
    inArg = false;
    inQuote = false;
    int ptrToStrArrCounter = 0;
    char * ptr1 = line;
    char * ptr2 = line;
    
    /* This seeks out the beginning of each argument and sets the array for each beginning argument,
    it then proceeds to seperate each argument so that each argument won't interefer with each other
    and this finally gives you seperated arguments */
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
    
    return ptrToStrArr;
}

