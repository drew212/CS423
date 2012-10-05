#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <asm/uaccess.h>

#include <linux/sched.h>
#include <linux/kthread.h>

#include "mp3_given.h"

#define PROC_DIR_NAME "mp3"
#define PROCFS_NAME "status"
#define PROCFS_MAX_SIZE 1024
#define FULL_PROC "status/mp3"

#define DEBUG

#ifdef DEBUG
#define debugk(...) printk(__VA_ARGS__)
#endif

struct proc_dir_entry* mp3_proc_dir_g;
struct proc_dir_entry* proc_file_g;

static struct timer_list timer_g;

struct proc_dir_entry* mp3_proc_dir_g;
static char procfs_buffer[PROCFS_MAX_SIZE]; //buffer used to store character

static ulong procfs_buffer_size_g = 0; //size of buffer

typedef struct process_data{
    int PID;
    struct task_struct* linux_task;

    struct list_head list_node;
} process_data_t;

LIST_HEAD(process_list_g);
DEFINE_MUTEX(process_list_mutex_g);

//Function Prototypes
void mp3_destroy_process_list(void);
void mp3_add_pid_to_list(int pid);
void mp3_remove_pid_from_list(int pid);

/**
 * Delete linked list. Call after removing procfs entries
 */
void
mp3_destroy_process_list(){
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
 * Register a new task when a process registers itself
 */
void
mp3_add_task_to_list(int pid){
    task_struct_t * new_task;

    mutex_lock(&process_list_mutex_g);

    new_pid_data = (process_data_t *)kmalloc(sizeof(process_data_t), GFP_KERNEL);

    new_pid_data->linux_task = find_task_by_pid(pid);
    new_pid_data->PID = pid;

    INIT_LIST_HEAD(&new_pid_data->list_node);

    list_add_tail(&new_pid_data->list_node, &process_list_g);

    mutex_unlock(&process_list_mutex_g);
}

void
mp3_remove_pid_from_list(int pid)
{
    process_data_t * pid_data = NULL;
    process_data_t * temp_pid_data = NULL;

    mutex_lock(&process_list_mutex_g);
    list_for_each_entry_safe(pid_data, temp_pid_data, &process_list_g, list_node)
    {
        if(pid_data->PID == pid)
        {
            list_del(&pid_data->list_node);
            kfree(pid_data);
            //TODO: Can we mutex unlock and return here so we don't go through the whole list?
        }
    }
    mutex_unlock(&process_list_mutex_g);

}

void
timer_handler(ulong data)
{
    printk(KERN_INFO "TIMER RUN!!!" );

    //TODO: Timer stuff

    setup_timer(&timer_g, timer_handler, 0);
    mod_timer(&timer_g, jiffies + msecs_to_jiffies (5000));
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
        //int num_copied = mp3_get_process_times(&proc_buff);
        //int nbytes = copy_to_user(buffer, proc_buff, num_copied);
        int nbytes = sprintf(buffer, "%s", proc_buff);
        //debugk(KERN_INFO "num_coppied: %d", num_copied);
        debugk(KERN_INFO "%snbytes: %d\n", proc_buff, nbytes);

        if(nbytes != 0)
        {
            printk(KERN_ALERT "procfile_read copy_to_user failed!\n");
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
        ulong count,
        void *data
        )
{
    const char split[] = " ";
    char* procfs_buffer_ptr;

    char* action;
    char* pid_str;
    ulong pid;

    //
    // Gather input that has been written
    //
    debugk(KERN_INFO "/proc/%s was written to!\n", FULL_PROC);

    procfs_buffer_size_g = count;
    if (procfs_buffer_size_g > PROCFS_MAX_SIZE ) {
        procfs_buffer_size_g = PROCFS_MAX_SIZE;
    }

    if ( copy_from_user(procfs_buffer, buffer, procfs_buffer_size_g) ) {
        printk(KERN_ALERT "Error in copy_from_user!\n");
        return -EFAULT;
    }

    procfs_buffer_ptr = procfs_buffer;

    action = strsep(&procfs_buffer_ptr, split);
    if(action[0] == '\0')
    {
        printk(KERN_ALERT "Malformed procfs write, not registering/unregsitering\n");
        return -EINVAL;
    }

    pid_str = strsep(&procfs_buffer_ptr, split);

    //
    // Validate input from procfile write
    //
    if(pid_str[0] == '\0' )
    {
        printk(KERN_ALERT "Malformed procfs write, not registering/unregsitering\n");
        return -EINVAL;
    }

    pid = simple_strtol(pid_str, NULL, 10);

    if(!pid)
    {
        printk(KERN_ALERT "Malformed PID\n");
        return -EINVAL;
    }

    if(action[0] == 'R')
    {
        mp3_add_pid_to_list(pid);
        debugk(KERN_INFO "PID:%s, registered.\n", procfs_buffer);
    }
    else if(action[0] == 'U')
    {
        //TODO: Deregister
        //TODO: If PID not in list, return error
    }
    else
    {
        printk(KERN_ALERT "Malformed procfs write, not registering/unregsitering\n");
        return -EINVAL;
    }


    return procfs_buffer_size_g;
}

int __init
my_module_init(void)
{
    printk(KERN_INFO "MODULE LOADED\n");

    //
    //Setup /proc/mp3/status
    //
    mp3_proc_dir_g = proc_mkdir(PROC_DIR_NAME, NULL);
    proc_file_g = create_proc_entry(PROCFS_NAME, 0666, mp3_proc_dir_g);

    if(proc_file_g == NULL)
    {
        remove_proc_entry(PROCFS_NAME, mp3_proc_dir_g);
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
    setup_timer ( &timer_g, timer_handler, 0);
    mod_timer ( &timer_g, jiffies + msecs_to_jiffies (5000) );

    return 0;
}

void __exit
my_module_exit(void)
{
    remove_proc_entry(PROCFS_NAME, mp3_proc_dir_g);
    remove_proc_entry(PROC_DIR_NAME, NULL);
    mp3_destroy_process_list();

    del_timer ( &timer_g );
    printk(KERN_INFO "MODULE UNLOADED\n");
}

module_init(my_module_init);
module_exit(my_module_exit);

MODULE_LICENSE("GPL");
