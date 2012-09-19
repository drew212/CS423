#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/timer.h>
#include <linux/list.h>
#include "mp1_given.h"
#include <asm/uaccess.h>

#define PROC_DIR_NAME "mp1"
#define PROCFS_NAME "status"
#define PROCFS_MAX_SIZE 1024
#define FULL_PROC "status/mp1"
#define THREAD_NAME "mp1_task_thread"

#define DEBUG

#ifdef DEBUG
#define debugk(...) printk(__VA_ARGS__)
#endif

struct proc_dir_entry* mp1_proc_dir_g;
struct proc_dir_entry* proc_file_g;

struct semaphore thread_sem;

static struct timer_list timer;
struct task_struct * thread;

struct proc_dir_entry* mp1_proc_dir_g;
static char procfs_buffer[PROCFS_MAX_SIZE]; //buffer used to store character

static unsigned long procfs_buffer_size = 0; //size of buffer

typedef struct process_data{
    int process_id;
    unsigned long cpu_time;
    struct list_head list_node;
} process_data_t;

LIST_HEAD(process_list_g);

//Function Prototypes
void mp1_init_process_list(void);
void mp1_destroy_process_list(void);
void mp1_add_pid_to_list(int pid);
void mp1_update_process_times(void);
void mp1_update_process_times_unsafe(void);
unsigned int mp1_get_process_times(char ** process_times);

void
mp1_init_process_list(){
    //TODO: Initialize list.  May not be needed due to LIST_HEAD macro
}

/**
 * Delete linked list. Not thread safe so call after removing procfs entries
 */
void
mp1_destroy_process_list(){
    process_data_t * pid_data = NULL;
    process_data_t * temp_pid_data = NULL;
    list_for_each_entry_safe(pid_data, temp_pid_data, &process_list_g, list_node) 
    {
        list_del(&pid_data->list_node);
        kfree(pid_data);
    }
}

/**
 * Register a new pid when a process registers itself
 */
void
mp1_add_pid_to_list(int pid){
    //TODO: Make this thread safe.
    process_data_t * new_pid_data = (process_data_t *)kmalloc(sizeof(process_data_t), GFP_KERNEL);
    new_pid_data->process_id = pid;
    new_pid_data->cpu_time = 0;
    INIT_LIST_HEAD(&new_pid_data->list_node);

    list_add_tail(&new_pid_data->list_node, &process_list_g);

    mp1_update_process_times_unsafe();
}

/**
 * Called by kernel thread to update process information in linked list
 */
void
mp1_update_process_times(){
    //TODO: Make this thread safe
    mp1_update_process_times_unsafe();
}


/**
 * Unsafe version of update function.
 */
void
mp1_update_process_times_unsafe(){
    process_data_t * pid_data = NULL;
    list_for_each_entry(pid_data, &process_list_g, list_node)
    {
        unsigned long cpu_time;
        if(0 == get_cpu_use(pid_data->process_id, &cpu_time))
        {
            pid_data->cpu_time = cpu_time;
        }
    }
}

/**
 * Retrieves a formatted string of process info.  Returns length of string.
 * Be sure to kfree this string when you are done with it!
 */
unsigned int
mp1_get_process_times(char ** process_times){
    //TODO: Make this thread safe.
    int index = 0;
    process_data_t * pid_data;

    *process_times = (char *)kmalloc(2048, GFP_KERNEL);
    *process_times[0] = '\0';

    list_for_each_entry(pid_data, &process_list_g, list_node)
    {
        index += sprintf(*process_times+index, "%d: %lu/n", pid_data->process_id, pid_data->cpu_time);
    }
    return index;
}


void
timer_handler(unsigned long data)
{
    printk(KERN_INFO "TIMER RUN!!!" );

    setup_timer(&timer, timer_handler, 0);
    mod_timer(&timer, jiffies + msecs_to_jiffies (5000));
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

    debugk(KERN_INFO "reading from procfile\n");

    if (offset > 0) {
        /* we have finished to read, return 0 */
        ret  = 0;
    } else {
        /* fill the buffer, return the buffer size */
        int nbytes = copy_to_user(buffer, procfs_buffer, procfs_buffer_size);
        if(nbytes > 0)
        {
            printk(KERN_ALERT "procfile_read copy_to_user failed!\n");
        }
        ret = procfs_buffer_size;
    }
    return ret;
}

int
procfile_write(
    struct file *file,
    const char *buffer,
    unsigned long count,
    void *data
    )
{
    debugk(KERN_INFO "/proc/%s was written to!\n", FULL_PROC);
    /* get buffer size */
    procfs_buffer_size = count;
    if (procfs_buffer_size > PROCFS_MAX_SIZE ) {
        procfs_buffer_size = PROCFS_MAX_SIZE;
    }

    /* write data to the buffer */
    if ( copy_from_user(procfs_buffer, buffer, procfs_buffer_size) ) {
        return -EFAULT;
    }
    debugk(KERN_INFO "PID:%s, registered.\n", procfs_buffer);

    return procfs_buffer_size;
}


int __init
my_module_init(void)
{
    printk(KERN_INFO "MODULE LOADED\n");
    debugk(KERN_INFO "debugging works!\n");

    //Setup /proc/mp1/status
    mp1_proc_dir_g = proc_mkdir(PROC_DIR_NAME, NULL);
    proc_file_g = create_proc_entry(PROCFS_NAME, 0666, mp1_proc_dir_g);

    if(proc_file_g == NULL)
    {
        remove_proc_entry(PROCFS_NAME, mp1_proc_dir_g);
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
    setup_timer ( &timer, timer_handler, 0);
    mod_timer ( &timer, jiffies + msecs_to_jiffies (5000) );

    return 0;
}

void __exit
my_module_exit(void)
{
    remove_proc_entry(PROCFS_NAME, mp1_proc_dir_g);
    remove_proc_entry(PROC_DIR_NAME, NULL);

    del_timer ( &timer );
    printk(KERN_INFO "MODULE UNLOADED\n");
}

/*
 * Starts the kernel thread
 */
//TODO This should be static
void
start_kthread(void)
{
    //TODO
    //task_struct = kthread_create(int(* thread_fucntion)(void* data),  NULL, THREAD_NAME);
}

/*
 * Stops the kernel thread
 */
void
stop_kthread(void)
{
    //TODO
}

/*
 * This is the function that executes when the thread is run.
 */
int
thread_function(void * data)
{
    //TODO
    return 0;
}


module_init(my_module_init);
module_exit(my_module_exit);

MODULE_LICENSE("GPL");
