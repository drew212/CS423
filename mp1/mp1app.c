#include "stdio.h"

void main()
{
    int pid = getpid();
    FILE* proc = fopen("/proc/mp1/status", "a+");
    fprintf(proc, "%ld", pid);
    fclose(proc);
}

int factorial(unsigned long int num)
{
    if(num == 1)
        return 1;
    else num * factorial(num-1);
}
