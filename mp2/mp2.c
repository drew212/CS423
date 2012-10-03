#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <asm/uaccess.h>
#include <linux/limits.h>

#include <linux/sched.h>
#include <linux/kthread.h>

#include "mp2_given.h"
#include "defs.h"

#define PROC_DIR_NAME "mp2"
#define PROCFS_NAME "status"
#define PROCFS_MAX_SIZE 1024
#define FULL_PROC "status/mp2"
#define THREAD_NAME "mp2_task_thread"

// Process States
#define READY 0
#define RUNNING 1
#define SLEEPING 2

#define DEBUG

#ifdef DEBUG
#define debugk(...) printk(__VA_ARGS__)
#endif

struct proc_dir_entry* mp2_proc_dir_g;
struct proc_dir_entry* proc_file_g;

struct task_struct * thread_g;


struct proc_dir_entry* mp2_proc_dir_g;
char procfs_buffer[PROCFS_MAX_SIZE]; //buffer used to store character

static ULONG procfs_buffer_size_g = 0; //size of buffer

typedef struct mp2_task_struct{
    struct task_struct* linux_task;
    struct timer_list wakeup_timer;
    int state;
    ULONG PID;
    ULONG period;
    ULONG proc_time;

    struct list_head list_node;
} task_struct_t;

task_struct_t * running_process_g;

LIST_HEAD(process_list_g);
DEFINE_MUTEX(process_list_mutex_g);

//Function Prototypes
void mp2_destroy_process_list(void);
void mp2_update_tasks(void);
UINT mp2_get_process_times(char ** process_times);
int thread_function(void * data);
void start_kthread(void);
void stop_kthread(void);
int mp2_register(ULONG pid, ULONG period, ULONG computation);
int mp2_yield(ULONG pid);
int mp2_deregister(ULONG pid);
void set_pid_ready(ULONG pid);
bool mp2_admission_control(ULONG period, ULONG proc_time);
void mp2_set_timer(task_struct_t * process);

/**
 * Delete linked list. Call after removing procfs entries
 */
void
mp2_destroy_process_list(){
    task_struct_t * task_data = NULL;
    task_struct_t * temp_task_data = NULL;
    mutex_lock(&process_list_mutex_g);
    //TODO: Do we do anything extra with the running/blocked processes?
    list_for_each_entry_safe(task_data, temp_task_data, &process_list_g, list_node)
    {
        list_del(&task_data->list_node);

        del_timer(&task_data->wakeup_timer);


        kfree(task_data);
    }
    mutex_unlock(&process_list_mutex_g);
}

void
mp2_update_tasks(void)
{
    task_struct_t * curr_process = NULL;
    task_struct_t * next_process = NULL;
    task_struct_t * process_to_remove = NULL;

    mutex_lock(&process_list_mutex_g);
    list_for_each_entry(curr_process, &process_list_g, list_node)
    {
        // Find the task with state == READY and shortest period
        if(next_process == NULL || (curr_process->state == READY && next_process->period > curr_process->period))
        {
            next_process = curr_process;
        }
        if(running_process_g->state == RUNNING)
        {
            running_process_g->state = READY;
        }
    }

    next_process->state = RUNNING;

    if(running_process_g->PID != next_process->PID)
    {
        // Preempt the currently running task
        struct sched_param sparam_remove;
        sparam_remove.sched_priority = 0;
        sched_setscheduler(running_process_g->linux_task, SCHED_NORMAL, &sparam_remove);

        // Set state of new task to running and schedule it
        struct sched_param sparam_schedule;
        wake_up_process(next_process->linux_task);
        sparam_schedule.sched_priority=MAX_USER_RT_PRIO-1;
        sched_setscheduler(next_process->linux_task, SCHED_FIFO, &sparam_schedule);
    }

    mutex_unlock(&process_list_mutex_g);
}

/**
 * Retrieves a formatted string of process info.  Returns length of string.
 * Be sure to kfree this string when you are done with it!
 */
UINT
mp2_get_process_times(char ** process_times){
    //TODO: rename this
    UINT index = 0;
    task_struct_t * task_data;

    mutex_lock(&process_list_mutex_g);

    *process_times = (char *)kmalloc(MAX_INPUT * sizeof(char), GFP_KERNEL);
    *process_times[0] = '\0';

    index += sprintf(*process_times+index, "PID:Period:Computation:State\n");
    list_for_each_entry(task_data, &process_list_g, list_node)
    {
        index += sprintf(*process_times+index, "%ld:%ld:%ld:%ld\n",
                task_data->PID, task_data->period, task_data->proc_time, task_data->state);
    }
    mutex_unlock(&process_list_mutex_g);
    return index;
}


void
timer_handler(ULONG pid)
{
    printk(KERN_INFO "Timer run for pid: %ld", pid);

    // Set the pid state to ready and call dispatch thread
    set_pid_ready(pid);

    wake_up_process(thread_g);
}

int
mp2_register(ULONG pid, ULONG period, ULONG proc_time)
{
    //TODO test this, check for errors and return error codes
    printk(KERN_INFO "Registering PID %ld, period: %ld, comp: %ld\n", pid, period, proc_time);
    task_struct_t * new_task_data;

    if(!mp2_admission_control(period, proc_time))
    {
        // We don't have processor time returning that we're busy
        return -EBUSY;
    }

    new_task_data = (task_struct_t *)kmalloc(sizeof(task_struct_t), GFP_KERNEL);
    new_task_data->linux_task = find_task_by_pid(pid);
    new_task_data->PID = pid;
    new_task_data->state = SLEEPING;
    new_task_data->period = period;
    new_task_data->proc_time = proc_time;

    mutex_lock(&process_list_mutex_g);

    INIT_LIST_HEAD(&new_task_data->list_node);

    list_add_tail(&new_task_data->list_node, &process_list_g);

    mp2_set_timer(new_task_data);

    mutex_unlock(&process_list_mutex_g);

    return 0;
}

void
mp2_set_timer(task_struct_t * process)
{
    printk(KERN_INFO "PID: %ld, timer reset", process->PID);
    setup_timer(&process->wakeup_timer, timer_handler, process->PID);
    mod_timer(&process->wakeup_timer, jiffies + msecs_to_jiffies (process->period));
}

int
mp2_yield(ULONG pid)
{
    printk(KERN_INFO "pid %ld is yielding \n", pid);

    task_struct_t * task_data;
    task_struct_t * temp_task_data;

    mutex_lock(&process_list_mutex_g);

    list_for_each_entry_safe(task_data, temp_task_data, &process_list_g, list_node)
    {
        if(task_data->PID == pid)
        {
            set_task_state(task_data->linux_task, TASK_UNINTERRUPTIBLE);
        }
    }
    mutex_unlock(&process_list_mutex_g);
    wake_up_process(thread_g);
    return 0;
}

int
mp2_deregister(ULONG pid)
{
    printk(KERN_INFO "deregistering pid %ld", pid);

    task_struct_t * task_data;
    task_struct_t * temp_task_data;

    mutex_lock(&process_list_mutex_g);

    list_for_each_entry_safe(task_data, temp_task_data, &process_list_g, list_node)
    {
        if(task_data->PID == pid)
        {
            list_del(&task_data->list_node);

            del_timer(&task_data->wakeup_timer);

            kfree(&task_data->wakeup_timer);
            kfree(task_data);
        }
    }
    mutex_unlock(&process_list_mutex_g);
    return 0;
}

bool
mp2_admission_control(ULONG proc_time, ULONG period)
{
    double utilization = 0;

    task_struct_t * task_data;

    mutex_lock(&process_list_mutex_g);
    list_for_each_entry(task_data, &process_list_g, list_node)
    {
        utilization += task_data->proc_time / task_data->period;
    }
    utilization += proc_time / period;
    mutex_unlock(&process_list_mutex_g);

    if(utilization < 0.693)
        return true;
    else return false;

}

void
set_pid_ready(ULONG pid)
{
    printk(KERN_INFO "setting pid %ld ready", pid);

    task_struct_t * task_data;

    mutex_lock(&process_list_mutex_g);

    list_for_each_entry(task_data, &process_list_g, list_node)
    {
        if(task_data->PID == pid)
        {
            //TODO set period for task_data accordingly
            task_data->state = READY;

            mp2_set_timer(task_data);
        }
    }
    mutex_unlock(&process_list_mutex_g);
}

/*
 * This function reads from the proc file and fills the buffer
 */
int
procfile_read(
        char * buffer,
        char ** buffer_location,
        off_t offset,
        int buffer_length,
        int * eof,
        void * data
        )
{
    int ret;
    char * proc_buff = NULL;

    printk(KERN_INFO "reading from procfile\n");

    if (offset > 0) {
        /* we have finished to read, return 0 */
        ret  = 0;
    } else {

        mp2_get_process_times(&proc_buff); // TODO: may want to check return?
        //int nbytes = copy_to_user(buffer, proc_buff, num_copied);
        int nbytes = sprintf(buffer, "%s", proc_buff);
        printk(KERN_INFO "trying to print:%s", proc_buff);

        if(nbytes != 0)
        {
            //printk(KERN_ALERT "procfile_read failed!\n");
            //TODO: i don't know what to do here, sprintf is returning 0 always, but it is writing to the buffer
        }

        kfree(proc_buff);
        ret = nbytes;
    }
    return ret;
}

int
procfile_write(
        struct file *file,
        const char *buffer,
        ULONG count,
        void *data
        )
{
    int pid_from_proc_file;
    const char split[] = ":";

    debugk(KERN_INFO "/proc/%s was written to!\n", FULL_PROC);
    /* get buffer size */
    procfs_buffer_size_g = count;
    if (procfs_buffer_size_g > PROCFS_MAX_SIZE ) {
        procfs_buffer_size_g = PROCFS_MAX_SIZE;
    }

    /* write data to the buffer */
    if ( copy_from_user(procfs_buffer, buffer, procfs_buffer_size_g) ) {
        return -EFAULT;
    }

    char* procfs_buffer_ptr = procfs_buffer;

    char* register_action = strsep(&procfs_buffer_ptr, split);
    char* pid_str = strsep(&procfs_buffer_ptr, split);
    char* period_str = strsep(&procfs_buffer_ptr, split);
    char* computation_str = strsep(&procfs_buffer_ptr, split);



    if(pid_str[0] == '\0' || (register_action[0] == 'R' && computation_str[0] == '\0'))
    {
        printk(KERN_ALERT "Malformed procfs write, not registering/yielding/de-regsitering\n");
        return -EINVAL;
    }

    ULONG pid = simple_strtol(pid_str, NULL, 10);

    if(!pid)
    {
        printk(KERN_ALERT "malformed PID\n");
        return -EINVAL;
    }

    int ret = 0;
    if( register_action[0] == 'R' )
    {
        ULONG period = simple_strtol(period_str, NULL, 10);
        ULONG computation = simple_strtol(computation_str, NULL, 10);
        ret = mp2_register(pid, period, computation);
    }
    else if ( register_action[0] == 'Y' )
    {
        ret = mp2_yield(pid);
    }
    else if ( register_action[0] == 'D' )
    {
        ret = mp2_deregister(pid);
    }
    else
    {
        printk(KERN_ALERT "Registration operation not valid\n");
        ret = -EINVAL;
    }
    if(ret)
    {
        return ret;
    }


    return procfs_buffer_size_g;
}


    int __init
my_module_init(void)
{
    printk(KERN_INFO "MODULE LOADED\n");

    //Setup /proc/mp2/status
    mp2_proc_dir_g = proc_mkdir(PROC_DIR_NAME, NULL);
    proc_file_g = create_proc_entry(PROCFS_NAME, 0666, mp2_proc_dir_g);

    if(proc_file_g == NULL)
    {
        remove_proc_entry(PROCFS_NAME, mp2_proc_dir_g);
        remove_proc_entry(PROC_DIR_NAME, NULL);
        printk(KERN_ALERT "Error: Could not initialize /proc/%s\n", FULL_PROC);
        return -ENOMEM;
    }

    proc_file_g->read_proc = procfile_read;
    proc_file_g->write_proc = procfile_write;
    proc_file_g->mode = S_IFREG | S_IRUGO | S_IWUSR | S_IWGRP | S_IWOTH;
    proc_file_g->uid = 0;
    proc_file_g->gid = 0;
    proc_file_g->size = 37;

    printk(KERN_INFO "/proc/%s created\n", FULL_PROC);

    return 0;
}

    void __exit
my_module_exit(void)
{
    remove_proc_entry(PROCFS_NAME, mp2_proc_dir_g);
    remove_proc_entry(PROC_DIR_NAME, NULL);
    mp2_destroy_process_list();

    printk(KERN_INFO "MODULE UNLOADED\n");
}

/*
 * Starts the kernel thread
 */
    void
start_kthread(void)
{
    debugk(KERN_INFO "Starting up kernel thread!\n");
    thread_g = kthread_run(&thread_function,  NULL, THREAD_NAME);
}

/*
 * Stops the kernel thread
 */
    void
stop_kthread(void)
{
    kthread_stop(thread_g);
}

/*
 * This is the function that executes when the thread is run.
 * After start wake up with:
 * wake_up_process(thread);
 */
    int
thread_function(void * data)
{
    while(1)
    {
        debugk(KERN_INFO "Thread running!\n");
        if(kthread_should_stop())
            return 0;
        mp2_update_tasks();

        set_current_state(TASK_INTERRUPTIBLE);
        schedule();
    }
    return 0;
}


module_init(my_module_init);
module_exit(my_module_exit);

MODULE_LICENSE("GPL");
