#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <asm/uaccess.h>

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

static struct timer_list timer_g;
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

typedef struct process_data {
    int process_id;
    ULONG cpu_time;
    struct list_head list_node;
} process_data_t;

LIST_HEAD(process_list_g);
DEFINE_MUTEX(process_list_mutex_g);

//Function Prototypes
void mp2_init_process_list(void);
void mp2_destroy_process_list(void);
void mp2_add_pid_to_list(ULONG pid, ULONG period, ULONG proc_time);
void mp2_update_process_times(void);
void mp2_update_process_times_unsafe(void);
UINT mp2_get_process_times(char ** process_times);
int thread_function(void * data);
void start_kthread(void);
void stop_kthread(void);
int mp2_register(ULONG pid, ULONG period, ULONG computation);
int mp2_yeild(ULONG pid);
int mp2_deregister(ULONG pid);

void
mp2_init_process_list(){
    //TODO: Initialize list.  May not be needed due to LIST_HEAD macro
}

/**
 * Delete linked list. Call after removing procfs entries
 */
void
mp2_destroy_process_list(){
    process_data_t * pid_data = NULL;
    process_data_t * temp_pid_data = NULL;
    mutex_lock(&process_list_mutex_g);
    list_for_each_entry_safe(pid_data, temp_pid_data, &process_list_g, list_node)
    {
        list_del(&pid_data->list_node);
        kfree(pid_data);
    }
    mutex_unlock(&process_list_mutex_g);
}

/**
 * Register a new pid when a process registers itself
 */
void
mp2_add_pid_to_list(ULONG pid, ULONG period, ULONG proc_time){
    task_struct_t * new_pid_data;

    mutex_lock(&process_list_mutex_g);
    new_pid_data = (task_struct_t *)kmalloc(sizeof(task_struct_t), GFP_KERNEL);
    new_pid_data->PID = pid;
    new_pid_data->state = SLEEPING;
    new_pid_data->period = period;
    new_pid_data->proc_time = proc_time;

    INIT_LIST_HEAD(&new_pid_data->list_node);

    list_add_tail(&new_pid_data->list_node, &process_list_g);

    mutex_unlock(&process_list_mutex_g);
}

/**
 * Called by kernel thread to update process information in linked list
 */
void
mp2_update_process_times(){
    mutex_lock(&process_list_mutex_g);
    mp2_update_process_times_unsafe();
    mutex_unlock(&process_list_mutex_g);
}


/**
 * Unsafe version of update function.
 */
void
mp2_update_process_times_unsafe(){
    process_data_t * pid_data = NULL;
    list_for_each_entry(pid_data, &process_list_g, list_node)
    {
//        ULONG cpu_time;
//        if(0 == get_cpu_use(pid_data->process_id, &cpu_time))
//        {
//            pid_data->cpu_time = cpu_time;
//            printk(KERN_INFO "pid: %d, cpu: %lu", pid_data->process_id, pid_data->cpu_time);
//        }
//        else
//        {
//            // If get_cpu returns an error, we don't know what to do
//            printk(KERN_ALERT "pid %d returns error with get_cpu_use()", pid_data->process_id);
//        }
    }
}

/**
 * Retrieves a formatted string of process info.  Returns length of string.
 * Be sure to kfree this string when you are done with it!
 */
UINT
mp2_get_process_times(char ** process_times){
    UINT index = 0;
    process_data_t * pid_data;

    mutex_lock(&process_list_mutex_g);

    *process_times = (char *)kmalloc(2048, GFP_KERNEL);
    *process_times[0] = '\0';

    list_for_each_entry(pid_data, &process_list_g, list_node)
    {
        index += sprintf(*process_times+index, "%d: %lu\n", pid_data->process_id, pid_data->cpu_time);
    }
    mutex_unlock(&process_list_mutex_g);
    return index;
}


void
timer_handler(ULONG pid)
{
    printk(KERN_INFO "TIMER RUN!!!" );

    //TODO: set the pid state to ready and call dispatch thread

    setup_timer(&timer_g, timer_handler, 0);
    mod_timer(&timer_g, jiffies + msecs_to_jiffies (5000));
}

int
mp2_register(ULONG pid, ULONG period, ULONG computation)
{
    //TODO
    printk(KERN_INFO "registering pid %ld\n", pid);
    return -EINVAL;
}
int
mp2_yeild(ULONG pid)
{
    //TODO
    printk(KERN_INFO "pid %ld is yielding \n", pid);
    return -EINVAL;
}
int
mp2_deregister(ULONG pid)
{
    //TODO
    printk(KERN_INFO "deregistering pid %ld", pid);
    return -EINVAL;
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

    debugk(KERN_INFO "reading from procfile\n");

    if (offset > 0) {
        /* we have finished to read, return 0 */
        ret  = 0;
    } else {
        /* fill the buffer, return the buffer size */
        int num_copied = mp2_get_process_times(&proc_buff);
        //int nbytes = copy_to_user(buffer, proc_buff, num_copied);
        int nbytes = sprintf(buffer, "%s", proc_buff);
        debugk(KERN_INFO "num_coppied: %d", num_copied);
        debugk(KERN_INFO "%snbytes: %d\n", proc_buff, nbytes);

        if(nbytes != 0)
        {
            printk(KERN_ALERT "procfile_read copy_to_user failed!\n");
            kfree(proc_buff);
            return -EIO;
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

    ULONG pid = simple_strtol(pid_str, NULL, 10);
    ULONG period = simple_strtol(period_str, NULL, 10);
    ULONG computation = simple_strtol(computation_str, NULL, 10);

    if(!pid)
    {
        printk(KERN_ALERT "malformed PID\n");
        return -EINVAL;
    }

    if(pid_str[0] == '\0' || (register_action[0] == 'R' && computation_str[0] == '\0'))
    {
        printk(KERN_ALERT "Malformed procfs write, not registering/yeilding/de-regsitering\n");
        return -EINVAL;
    }

    int ret = 0;
    if( register_action[0] == 'R' )
    {
        printk(KERN_INFO "Registering PID %ld, period: %ld, comp: %ld\n", pid, period, computation);
        ret = mp2_register(pid, period, computation);
    }
    else if ( register_action[0] == 'Y' )
    {
        ret = mp2_yeild(pid);
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
    debugk(KERN_INFO "debugging works!\n");

    //Setup /proc/mp2/status
    mp2_proc_dir_g = proc_mkdir(PROC_DIR_NAME, NULL);
    proc_file_g = create_proc_entry(PROCFS_NAME, 0666, mp2_proc_dir_g);

    if(proc_file_g == NULL)
    {
        remove_proc_entry(PROCFS_NAME, mp2_proc_dir_g);
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

    //SETUP TIMER
//    setup_timer ( &timer_g, timer_handler, 0);
//    mod_timer ( &timer_g, jiffies + msecs_to_jiffies (5000) );

//    start_kthread();
    return 0;
}

void __exit
my_module_exit(void)
{
    remove_proc_entry(PROCFS_NAME, mp2_proc_dir_g);
    remove_proc_entry(PROC_DIR_NAME, NULL);
    mp2_destroy_process_list();
    //stop_kthread();

    //del_timer ( &timer_g );
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
 */
int
thread_function(void * data)
{
    while(1)
    {
        debugk(KERN_INFO "Thread running!\n");
        if(kthread_should_stop())
            return 0;
        mp2_update_process_times();
        set_current_state(TASK_INTERRUPTIBLE);
        schedule();
    }
    return 0;
}


module_init(my_module_init);
module_exit(my_module_exit);

MODULE_LICENSE("GPL");
