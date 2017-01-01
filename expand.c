#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include "proto.h"

int findInz (char * orig, char * Inz);
char * envpone (char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
//bool envptwo (char * origBuffLoc, char * newBuff, int * counter);

int expand (char *orig, char *new, int newsize)
{
    int lenOfFuncArr = 1;
    char *Inz[lenOfFuncArr];
    Inz[0] = "${";
    //Inz[1] = "$$";
    int whereIsInz = 0;
    int whereIsNew = 0;
    typedef char *(*funcInz)(char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
    funcInz funcInzArr[lenOfFuncArr];
    funcInzArr[0] = envpone;
    //funcInzArr[1] = envptwo;
    int lenOfParam = 0;
    
    while (orig[whereIsInz] != 0)
    {  
        int g;
        for (g = 0; g < lenOfFuncArr; g++)
        {
            lenOfParam = 0;
            lenOfParam = findInz(&orig[whereIsInz], Inz[g]);
            if (lenOfParam != 0)
            {
                int inputC = 0;
                int inputNew = 0;
                whereIsInz = whereIsInz + lenOfParam;
                char * copyOver = (*funcInzArr[g])(&orig[whereIsInz], new, &inputC, &inputNew);
                whereIsInz = whereIsInz + inputC;
                whereIsNew = whereIsNew + inputNew;
                if (whereIsNew > newsize)
                {
                    printf("Expand too big\n");
                    return 0; 
                }
                int i;
                for (i = 0; i < inputNew; i++)
                {
                    *new = *copyOver;
                    new++;
                    copyOver++;
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

char * envpone (char * origBuffLoc, char * newBuff, int * counter, int * counterNew)
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
            return NULL;
        }
    }
    
    *origBuffLoc = 0;
    count++;
    *counter = count;
    
    if (count == 1)
    {
        return NULL;
    }
    
    
    char * envstr = getenv(ptr2temp);
    
    if (envstr == NULL)
    {
        return NULL;
    }
    
    int much = strlen(envstr);
    *counterNew = much;
    return envstr;
    /*
    for (int m = 0; m < much; m++)
    {
        *newBuff = *envstr;
        newBuff++;
        envstr++;
    }
    return true;
    */
}

/* bool envptwo (char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
{
    
}
*/
