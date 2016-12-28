#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include "proto.h"

int expand (char *orig, char *new, int newsize)
{
    char * ptr1orig = orig;
    char * ptr2temp;
    char * ptr1new = new;
    bool inEnv = false;
    
    while (*ptr1orig != 0)
    {
        if (*ptr1orig == '$')
        {
            ptr1orig++;
            if (*ptr1orig == '{')
            {
                ptr1orig++;
                if (*ptr1orig != '}')
                {
                    ptr2temp = ptr1orig;
                    inEnv = true;
                    while (inEnv == true)
                    {
                        if (*ptr1orig != '}')
                        {
                            ptr1orig++;
                        }
                        else if (*ptr1orig == '}')
                        {
                            *ptr1orig = 0;
                            if (*ptr1orig + 1 != 0)
                            {
                                ptr1orig++;
                            }
                            inEnv = false;
                        }
                        else if (*ptr1orig == 0)
                        {
                            printf ("No } detected");
                            return 0;
                        }
                    }
                    char * envstr = getenv(ptr2temp);
                    int much = strlen(envstr);
                    for (int m = 0; m < much; m++)
                    {
                        *ptr1new = *envstr;
                        if (*envstr != 0)
                        {
                            ptr1new++;
                            envstr++;
                        }
                    }
                    *ptr1new = 0;
                }
            }
            else if (*ptr1orig != '{')
            {
                *ptr1new = *(ptr1orig - 1);
                ptr1new++;
            }
            
            
        }
        else
        {
            *ptr1new = *ptr1orig;
            ptr1new++;
            ptr1orig++;
        }
    }
    *ptr1new = 0;
    printf("%s\n", new);
    return 1;
}
