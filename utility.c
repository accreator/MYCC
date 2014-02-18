#include <stdio.h>
#include <stdlib.h>
#include "utility.h"

int reporterrorflag = 0;

void report_error(int line, char *str, char *text)
{
    if(!reporterrorflag)
        printf("Line %d: %s %s\n", line, str, text);
    exit(1);
}

char * random_string_gen()
{
    char *s;
    static int count = 0;
    count ++;
    s = (char *)malloc(16);
    sprintf(s, "?%d", count);
    return s;
}
