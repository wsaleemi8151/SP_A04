


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Retrieved from: https://stackoverflow.com/questions/4770985/how-to-check-if-a-string-starts-with-another-string-in-c
// How to check if a string starts with another string in C?
int checkPrefix(char *pre, char *str)
{
    return strncmp(pre, str, strlen(pre));
}
