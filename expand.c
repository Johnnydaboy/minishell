#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include "proto.h"
#include <dirent.h>

extern int margc;
extern char **margv;
extern bool runFourFunctions;
extern int counterforShift;
extern bool enterShift;
extern bool normalExit;

bool normalExit = true;
char globalStrValOfInt[20];
int findInz (char * orig, char * Inz);
char * dollarSignCurlyBrace (char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
char * dollarSignDollarSign (char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
char * dollarSignPoundSign (char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
char * dollarSignN (char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
char * dollarSignQuestionMark (char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
bool match (const char *pattern, const char *candidate, int p, int c);
char **getResult(char *pattern, char *folderName, int *count);
//char * wildCardExpand (char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
// This function takes in two character arrays and writes to the new buffer by reading from the old buffer, newsize is the length of array new
// In a failure case this function will return a 0 and otherwise it returns 1
int expand (char *orig, char *new, int newsize)
{
    int lenOfFuncArr = 5;
    char *Inz[lenOfFuncArr];
    Inz[0] = "${";
    Inz[1] = "$$";
    Inz[2] = "$#";
	Inz[3] = "$?";
    Inz[4] = "$";
	//Inz[5] = " *";
	//Inz[6] = "\"\"*";
    int whereIsInz = 0;
    int whereIsNew = 0;
    typedef char *(*funcInz)(char * origBuffLoc, char * newBuff, int * counter, int * counterNew);
    funcInz funcInzArr[lenOfFuncArr];
    funcInzArr[0] = dollarSignCurlyBrace;
    funcInzArr[1] = dollarSignDollarSign;
    funcInzArr[2] = dollarSignPoundSign;
	funcInzArr[3] = dollarSignQuestionMark;
    funcInzArr[4] = dollarSignN;
	//funcInzArr[5] = wildCardExpand;
	//funcInzArr[6] = wildCardExpand;
    int lenOfParam = 0;
    
    // This while loop will continue to execute until the orig string reads at 0 
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
                // counter (orig counter is inputC) is added to the original buffer in order to skip ahead what is found in findInz
                // ^ (new counter is inputNew) same applies to counterNew but with newBuff rather
                char * copyOver = (*funcInzArr[g])(&orig[whereIsInz], new, &inputC, &inputNew);
                // If it fail and no } is found it will print an error statement
                if (inputNew == -1)
                {
                    printf("${: No matching }\n");
                    return 0;
                }
                else if (inputC == -1)
                {
                    return 0;
                }
                // This tells how far each string needs to skip by
                whereIsInz = whereIsInz + inputC;
                whereIsNew = whereIsNew + inputNew;
                if (whereIsNew > newsize)
                {
                    printf("Expand too big\n");
                    return 0; 
                }
                int i;
                // the single character copy over that is given if a function is called
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

// This function finds the next initilizing variable and returns the location of it if any
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

// This is the first expand subfunction which does and expand to ${} 
char * dollarSignCurlyBrace (char * origBuffLoc, char * newBuff, int * counter, int * counterNew)
{
    char * ptr2temp;
    int count = 0;
    ptr2temp = origBuffLoc;
    
    while (*origBuffLoc != '}')
    {
        origBuffLoc++;
        count++;
        // This is an error statement and above will check if -1 it will fail
        if (*origBuffLoc == 0)
        {
            *counterNew = -1;
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
}

// The second expand subfunction which expands the $$ environment
char * dollarSignDollarSign (char * origBuffLoc, char * newBuff, int * counter, int * counterNew)
{
    int envint = getppid();
    sprintf(globalStrValOfInt,"%d", envint);
    int cNew = strlen(globalStrValOfInt);
    *counterNew = cNew;
    return globalStrValOfInt;
}

char * dollarSignPoundSign (char * origBuffLoc, char * newBuff, int * counter, int * counterNew)
{
    int argsHere = 0;
	//printf("%d\n", margc);
	if (runFourFunctions == false)
	{
		argsHere = margc;
		sprintf(globalStrValOfInt, "%d", argsHere);
		*counterNew = strlen(globalStrValOfInt);
		return globalStrValOfInt;
	}
	else 
	{
		while (*margv != 0)
		{
			//printf("%s\n", *margv);
			//printf("%d\n", argsHere);
			margv++;
			argsHere++;
		}
		argsHere--;
	}
    sprintf(globalStrValOfInt, "%d", argsHere);
    *counterNew = strlen(globalStrValOfInt);
	margv = margv - argsHere;
	//printf("%d\n", argsHere);
    return globalStrValOfInt;
}

char * dollarSignN (char * origBuffLoc, char * newBuff, int * counter, int * counterNew)
{
    int totalNum = 0;
    int length = 0;
    bool isOver = false;
    bool isNormal = true;
    if (*origBuffLoc == ' ' || *origBuffLoc == 0)
    {	
        return NULL;
    }
    while (*origBuffLoc != ' ' && *origBuffLoc != 0)
    {
        if (*origBuffLoc >= '0' && *origBuffLoc <= '9')
        {
            totalNum = totalNum * 10 + *origBuffLoc - '0';
            //printf("%d\n", totalNum);
        }
        else
        {
            isNormal = false;
        }
        if (totalNum != 0 && totalNum >= margc - 1)
        {
            isOver = true;
        }
        length++;
        origBuffLoc++;
    } 
	/*
	printf("%d\n",counterforShift);
	if (enterShift == true)
	{
		printf("true\n");
	}
	*/
	if (counterforShift == 0 && enterShift == true)
	{
		//printf("here");
		*counter = length;
		return NULL;
	}
	else if (isNormal == false || isOver == true)
    {
        *counter = length;
		//printf("here");
        return NULL;
    }
    else if (margc != 1)
    {   
       totalNum++;
       //^probably shouldn't touch this in certain situations but you also need to touch it in order to get /0
    }
    memcpy(newBuff, margv[totalNum], strlen(margv[totalNum])+1);
    *counter = length;
    *counterNew = strlen(margv[totalNum]);
    return newBuff;
}

char * dollarSignQuestionMark (char * origBuffLoc, char * newBuff, int * counter, int * counterNew)
{
	int envint;
	/*
	if (normalExit == true)
	{
		printf("hello working\n");
	}
	else if (normalExit == false)
	{
		printf("not working\n");
	}
	else
	{
		printf("I'm broken\n");
	}
	*/
	if (normalExit == true)
	{
		envint = getpid();
		sprintf(globalStrValOfInt,"%d", envint);
		int cNew = strlen(globalStrValOfInt);
		*counterNew = cNew;
		return globalStrValOfInt;
	}
	else if (normalExit == false)
	{
		envint = 1;
	}
	else
	{
		envint = 127;
	}
	
    sprintf(globalStrValOfInt,"%d", envint);
    int cNew = strlen(globalStrValOfInt);
    *counterNew = cNew;
    return globalStrValOfInt;
}
/*
char * wildCardExpand (char * origBuffLoc, char * newBuff, int * counter, int * counterNew)
{
	char cwd[199];
	getcwd(cwd, sizeof(cwd));
	char pattern[50];
	int k = 0;
	while (origBuffLoc != 0)
	{
		if (*origBuffLoc == ' ')
		{
			break;
		}
		else if (*origBuffLoc == '\"' && *(origBuffLoc + 1) == '\"')
		{
			break;
		}
		*(pattern+k) = *origBuffLoc;
		origBuffLoc++;
		k++;
	}
	*(pattern + k) = 0;
	*counter = k;
	int *count;
	char **result = getResult(pattern, cwd, count);
	int i;
	for (i = 0; i < *count; i++)
	{
		strcpy(newBuff, result[i]);
		newBuff = newBuff + strlen(result[i]);
		*newBuff = ' ';
		newBuff++;
	}
	*newBuff = 0;
	*counterNew = strlen(newBuff);
	return newBuff;
}

char **getResult(char *pattern, char *folderName, int *count)
{
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir (folderName)) != NULL) 
	{
		// count how many string
		int counter = 0;
		while ((ent = readdir (dir)) != NULL) 
		{
			if (*pattern == 0)
			{
				counter++; 
			}
			else if ( match(pattern, ent->d_name, 0, 0) )
			{
				counter++;
			}
		}
		char ** ptrToStrArr = (char** ) malloc(sizeof (char*) * counter);
		
		*count = counter;
		counter = 0;
		while ((ent = readdir (dir)) != NULL) 
		{
			if (*pattern == 0)
			{
				memcpy(ptrToStrArr[counter],ent->d_name, sizeof (ent->d_name)); 
			}
			else if ( match(pattern, ent->d_name, 0, 0) )
			{
				memcpy(ptrToStrArr[counter],ent->d_name, sizeof (ent->d_name));
			}
		}
		closedir (dir);
	
		return ptrToStrArr;
	} 
	return NULL;
}
*/
