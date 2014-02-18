#include<stdio.h>
#include<stdlib.h>

int main(int argc, char *argv[])
{
    char tmp[100];
    sprintf(tmp, "./mycc %s -q", argv[1]);
    if(system(tmp)) exit(1);
    sprintf(tmp, "./mycc %s | ./interpreter", argv[1]);
    system(tmp);
    return 0;
}

