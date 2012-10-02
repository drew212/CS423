#include <stdio.h>
#include <stdlib.h>

unsigned int period = 0;
unsigned int computation = 0;

long int factorial(unsigned long int num);

//TODO Write comments for functions
void registerProcess(int myPid)
{
    printf("%d: registering...\n", myPid);
    char string[100];
    sprintf(string, "echo \'R:%d:%u:%u\' > /proc/mp2/status", myPid, period, computation);
    printf("%s\n", string);
    system(string);
}

//TODO Write comments
int amIRegistered(int myPid)
{
    int isRegistered = 0;
    char line[1024];
    FILE * procFile = fopen("/proc/mp2/status", "r");

    /* Assumpting one pid per line. */
    while(fgets(line, 1024, procFile) != NULL) 
    {
       int pid = atoi(line);
       if(pid == myPid) 
       {
           isRegistered = 1;
           break;
       }
    }
    fclose(procFile);
    return isRegistered;
}

//TODO Write comments
void yield(int myPid){
    printf("%d: yielding...\n", myPid);
    char string[100];
    sprintf(string, "echo \'Y:%d\' > /proc/mp2/status", myPid);
    system(string);
}

//TODO Write comments
void unregister(int myPid){
    printf("%d: unregistering...\n", myPid);
    char string[100];
    sprintf(string, "echo \'D:%d\' > /proc/mp2/status", myPid);
    system(string);
}

void main()
{
    int myPid = getpid();
    //TODO generate period and computation times.

    registerProcess(myPid);
    if(0 == amIRegistered(myPid))
    {
        printf("%d: I was unable to register.  ;_;\n", myPid);
        exit(1);
    }
    printf("%d: Registered! \n", myPid);
    long startTime = gettimeofday();
    yield(myPid);

    //TODO Add while loop that performs work and calls yield instead of the below for loop.

    int i;
    for(i = 1; i < 1000000000; i++)
    {
        factorial((i % 20)+1);
    }

    unregister(myPid);
}


//TODO Write comments
long int factorial(unsigned long int num)
{
    if(num == 1)
        return 1;
    else return num * factorial(num-1);
}



