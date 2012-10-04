#include <stdio.h>
#include <stdlib.h>


/**
 * Constants for the user space app to randomly choose periods and computation times.
 */
#define MIN_PERIOD 100
#define MAX_PERIOD 500
#define MIN_COMP_TIME 10
#define MAX_COMP_TIME_TO_PERIOD_RATIO .2

/**
 * Range for the number of periods the process computes factorials
 */
#define MIN_ITERATIONS 100
#define MAX_ITERATIONS 3000

long int factorial(unsigned long int num);

/**
 * Writes to the proc file to register the process with the rate-monotonic scheduler.
 */
void register_process(int my_pid, unsigned int period, unsigned int computation_time)
{
    printf("%d: registering...\n", my_pid);
    char string[100];
    sprintf(string, "echo \'R:%d:%u:%u\' > /proc/mp2/status", my_pid, period, computation_time);
    printf("%s\n", string);
    system(string);
}

/**
 * Returns 1 if the proc file shows this program successfully registered.  0 otherwise.
 */
int is_registered(int my_pid)
{
    int is_registered = 0;
    char line[1024];
    FILE * proc_file = fopen("/proc/mp2/status", "r");

    /* Assumpting one pid per line. */
    while(fgets(line, 1024, proc_file) != NULL) 
    {
        int pid = atoi(line);
        if(pid == my_pid) 
        {
            printf("Registered PID: %d\n", pid);
            is_registered = 1;
            break;
        }
    }
    fclose(proc_file);
    return is_registered;
}

/**
 * Write to the proc file signalling that we have finished processing for this period.
 */
void yield(int my_pid){
    printf("%d: yielding...\n");
    char string[100];
    sprintf(string, "echo \'Y:%d\' > /proc/mp2/status", my_pid);
    system(string);
}

/**
 * Writes to the proc file signalling that we are done with all processing.
 */
void unregister(int my_pid){
    printf("%d: unregistering...\n", my_pid);
    char string[100];
    sprintf(string, "echo \'D:%d\' > /proc/mp2/status", my_pid);
    system(string);
}


/**
 * Returns a random value between the min and max parameters
 */
unsigned int rand_in_range(unsigned int min, unsigned int max)
{
    unsigned int range = max - min + 1;
    unsigned int random_value = rand() % range;
    return random_value + min;
}


/**
 * Returns a number between MIN_PERIOD and MAX_PERIOD
 */
unsigned int get_period()
{
    return rand_in_range(MIN_PERIOD, MAX_PERIOD);
}

/**
 * Returns a number between MIN_COMP_TIME and a fraction of period time
 */
unsigned int get_comp_time(int period)
{
    unsigned int max_comp_time = (unsigned int)(period * MAX_COMP_TIME_TO_PERIOD_RATIO);
    if(max_comp_time < MIN_COMP_TIME)
    {
        max_comp_time = MIN_COMP_TIME;
    }

    return rand_in_range(MIN_COMP_TIME, max_comp_time);
}

/**
 * Returns a random number between MIN_ITERATIONS and MAX_ITERATIONS
 */
unsigned int get_iterations()
{
    return rand_in_range(MIN_ITERATIONS, MAX_ITERATIONS);
}


/**
 * The main function.
 */
int main()
{
    /* Seeding random number generator */
    srand( time(NULL) );

    int my_pid = getpid();
    unsigned int period = get_period();
    unsigned int computation_time = get_comp_time(period);

    register_process(my_pid, period, computation_time);

    if(!is_registered(my_pid))
    {
        printf("%d: I was unable to register.  ;_;\n", my_pid);
        exit(1);
    }
    printf("%d: Registered! \n", my_pid);

    unsigned int iterations = get_iterations();
    yield(my_pid);

    while(iterations > 0)
    {
        struct timeval time;
        gettimeofday(&time);
        unsigned int start_time = time.tv_usec;

        unsigned int time_elapsed = 0;
        while(time_elapsed < computation_time)
        {
            int i;
            for(i = 0; i < 100; i++)
            {
                factorial((i % 20)+1);
            }
            gettimeofday(&time);
            time_elapsed = time.tv_usec - start_time;
        }

        yield(my_pid);
        iterations--;
    }

    unregister(my_pid);
    return 0;
}


/**
 * Calculate the factorial of a number.
 */
long int factorial(unsigned long int num)
{
    if(num == 1)
        return 1;
    else return num * factorial(num-1);
}



