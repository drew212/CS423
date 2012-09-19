#include "stdio.h"

long int factorial(unsigned long int num);

void main()
{
    int pid = getpid();
    printf("pid is %d\n", pid);
    char string[1024];
    sprintf(string, "echo \'%d\' > /proc/mp1/status", pid);
    printf("%s\n", string);
    system(string);
    int i;
    for(i = 1; i < 1000000000; i++)
    {
        printf("%ld\n", factorial((i % 20)+1));
    }
}

long int factorial(unsigned long int num)
{
    if(num == 1)
        return 1;
    else return num * factorial(num-1);
}
