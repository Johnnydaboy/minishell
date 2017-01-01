#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include "proto.h"

int findInz (char * orig, char * Inz);
bool envpone (char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
//bool envptwo (char * origBuffLoc, char * newBuff, int * counter);

int expand (char *orig, char *new, int newsize)
{
    int n = 1;
    char *Inz[n];
    Inz[0] = "${";
    //Inz[1] = "$$";
    int whereIsInz = 0;
    typedef bool (*funcInz)(char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
    funcInz funcInzArr[n];
    funcInzArr[0] = envpone;
    //funcInzArr[1] = envptwo;
    
    while (orig[whereIsInz] != 0)
    {  
        for (int g = 0; g < n; g++)
        {
            int h = findInz(&orig[whereIsInz], Inz[g]);
            if (h != 0)
            {
                int inputC = 0;
                int inputNew = 0;
                whereIsInz = whereIsInz + h;
                bool finish = (*funcInzArr[g])(&orig[whereIsInz], new, &inputC, &inputNew);
                whereIsInz = whereIsInz + inputC;
                new = new + inputNew;
                if (finish == false)
                {
                    printf("An incident has occurred");
                    return 0;
                }
            }
        }
        *new = orig[whereIsInz];
        new++;
        whereIsInz++;
        
    }
    *new = 0;
    return 1;
}

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

bool envpone (char * origBuffLoc, char * newBuff, int * counter, int * counterNew)
{
    char * ptr2temp;
    int count = 0;
    ptr2temp = origBuffLoc;
    
    while (*origBuffLoc != '}')
    {
        origBuffLoc++;
        count++;
        if (*origBuffLoc == 0)
        {
            return false;
        }
    }
    
    *origBuffLoc = 0;
    count++;
    *counter = count;
    
    if (count == 1)
    {
        return true;
    }
    
    
    char * envstr = getenv(ptr2temp);
    
    if (envstr == NULL)
    {
        return true;
    }
    
    int much = strlen(envstr);
    *counterNew = much; 
    for (int m = 0; m < much; m++)
    {
        *newBuff = *envstr;
        newBuff++;
        envstr++;
    }
    return true;
}

/* bool envptwo (char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
{
    
}
*/
