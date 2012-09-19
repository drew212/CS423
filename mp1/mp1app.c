#include "stdio.h"

void main()
{
    int pid = getpid();
    printf("pid is %d\n", pid);
    char string[1024];
    sprintf(string, "echo \'%d\' > /proc/mp1/status", pid);
    printf("%s\n", string);
    system(string);
    factorial(10000);
}

int factorial(unsigned long int num)
{
    if(num == 1)
        return 1;
    else num * factorial(num-1);
}
