#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <asm/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/page-flags.h>
#include <linux/workqueue.h>

#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/fs.h>

#include "mp3_given.h"

#define SUCCESS 0
#define PROC_DIR_NAME "mp3"
#define PROCFS_NAME "status"
#define PROCFS_MAX_SIZE 1024
#define FULL_PROC "status/mp3"

// Character Device
#define DEVICE_NAME "mp3_chardrv"

#define SHARED_BUFFER_SIZE (128 * 4096)

#define DEBUG

#ifdef DEBUG
#define debugk(...) printk(__VA_ARGS__)
#endif

struct proc_dir_entry* mp3_proc_dir_g;
struct proc_dir_entry* proc_file_g;

struct proc_dir_entry* mp3_proc_dir_g;
static char procfs_buffer[PROCFS_MAX_SIZE]; //buffer used to store character

static ulong procfs_buffer_size_g = 0; //size of buffer

void* shared_buffer_g;
ulong shared_buffer_offset_g;

int major_num_g;

static struct workqueue_struct * mp3_workqueue_g;
struct delayed_work mp3_delayed_job_g;

typedef struct mp3_task_struct{
    int PID;
    struct task_struct* linux_task;
    ulong cpu_usage;
    ulong min;
    ulong maj;

    struct list_head list_node;
} task_struct_t;

//Process List related globals
LIST_HEAD(process_list_g);
ulong process_list_size_g;
DEFINE_MUTEX(process_list_mutex_g);

//Function Prototypes
void mp3_destroy_process_list(void);
void mp3_add_task_to_list(int pid);
void mp3_remove_pid_from_list(int pid);

void mp3_update_task_data(void);
void mp3_write_task_stats_to_shared_buffer(void);

void mp3_schedule_delayed_work_queue_job(void);
void mp3_clear_work_queue(void);
void mp3_work_queue_function(struct work_struct * work);

// Device driver prototypes
static int device_open(struct inode* inode, struct file* file){return SUCCESS;}
static int device_close(struct inode* inode, struct file* file){return SUCCESS;}
static int device_mmap(struct file* file, struct vm_area_struct* vm_area);


/**
 * Delete linked list. Call after removing procfs entries
 */
void
mp3_destroy_process_list(){
    task_struct_t * pid_data = NULL;
    task_struct_t * temp_pid_data = NULL;
    mutex_lock(&process_list_mutex_g);

    //We will no longer need the work queue so clear it before the task lists.
    mp3_clear_work_queue();

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

    task_struct_t* new_task_data;

    mutex_lock(&process_list_mutex_g);

    new_task_data = (task_struct_t *)kmalloc(sizeof(task_struct_t), GFP_KERNEL);
    new_task_data->linux_task = find_task_by_pid(pid);
    new_task_data->PID = pid;

    INIT_LIST_HEAD(&new_task_data->list_node);
    list_add_tail(&new_task_data->list_node, &process_list_g);

    if(process_list_size_g == 0)
    {
        //First task in list so we also create a work queue job.
        mp3_schedule_delayed_work_queue_job();
    }
    process_list_size_g++;

    mutex_unlock(&process_list_mutex_g);
}

void
mp3_remove_pid_from_list(int pid)
{
    if(process_list_size_g > 0)
    {
        task_struct_t * pid_data = NULL;
        task_struct_t * temp_pid_data = NULL;

        mutex_lock(&process_list_mutex_g);
        list_for_each_entry_safe(pid_data, temp_pid_data, &process_list_g, list_node)
        {
            if(pid_data->PID == pid)
            {
                list_del(&pid_data->list_node);
                kfree(pid_data);
            }
        }

        process_list_size_g--;

        if(process_list_size_g == 0){
            //No more tasks, we need to clear work queue
            mp3_clear_work_queue();
        }

        mutex_unlock(&process_list_mutex_g);
    }

}

/*##### WORK QUEUE MANAGEMENT #####*/

/**
 * Adds a work item to the workqueue to be executed in 1/20th of a second from now.
 */
void
mp3_schedule_delayed_work_queue_job(void)
{
    //Schedule to run in 1/20th of a second
    INIT_DELAYED_WORK(&mp3_delayed_job_g, mp3_work_queue_function);
    queue_delayed_work(mp3_workqueue_g, &mp3_delayed_job_g, msecs_to_jiffies(50));
}

/**
 * Removes a job from the queue if there is one to remove.
 */
void
mp3_clear_work_queue(void)
{
    cancel_delayed_work_sync(&mp3_delayed_job_g);
}

/**
 * Read the data from the tasks and updates the buffer.  Then reschedules itself.
 */
void
mp3_work_queue_function(struct work_struct * work)
{
    mp3_update_task_data();
    mp3_write_task_stats_to_shared_buffer();
    mp3_schedule_delayed_work_queue_job();
}

uint
mp3_get_pids(char** pids_string)
{
    uint index = 0;
    task_struct_t * task_data;

    mutex_lock(&process_list_mutex_g);

    *pids_string = (char *)kmalloc(MAX_INPUT * sizeof(char), GFP_KERNEL);
    *pids_string[0] = '\0';

    list_for_each_entry(task_data, &process_list_g, list_node)
    {
        index += sprintf(*pids_string+index, "%d\n", task_data->PID);
    }
    mutex_unlock(&process_list_mutex_g);
    return index;
}

/*
 * mmap callback function to map memory into shared user space.
 */
static int device_mmap(struct file* file, struct vm_area_struct* vm_area)
{
    void* page_ptr;
    struct page* page;

    ulong curr_memory_address;
    ulong vm_end;

    debugk(KERN_INFO "mmap called!\n");

    curr_memory_address = vm_area->vm_start;
    vm_end = vm_area->vm_end;

    page_ptr = shared_buffer_g;
    page = vmalloc_to_page(page_ptr);

    debugk(KERN_INFO "Starting to map memory\n");
    while(curr_memory_address != vm_end)
    {
        vm_insert_page(vm_area, curr_memory_address, page);
        curr_memory_address += PAGE_SIZE;
        page_ptr = page_ptr + PAGE_SIZE;
        page = vmalloc_to_page(page_ptr);
    }
    printk(KERN_INFO "mmap called!\n");
    return SUCCESS;
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
        int num_copied = mp3_get_pids(&proc_buff);
        int nbytes = copy_to_user(buffer, proc_buff, num_copied);

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

/*
 * Allows a process to register by writing to the proc file
 */
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
        mp3_add_task_to_list(pid);
        debugk(KERN_INFO "PID:%s, registered.\n", procfs_buffer);
    }
    else if(action[0] == 'U')
    {
        mp3_remove_pid_from_list(pid);
    }
    else
    {
        printk(KERN_ALERT "Malformed procfs write, not registering/unregsitering\n");
        return -EINVAL;
    }

    return procfs_buffer_size_g;
}

/*
 * Initialize the kernel module
 */
int __init
my_module_init(void)
{
    // Struct with callback functions for character device driver
    static struct file_operations fops = {
        .open = device_open,
        .release = device_close,
        .mmap = device_mmap
    };

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

    //
    // Character Device Driver
    //
    major_num_g = register_chrdev(0, DEVICE_NAME, &fops);
    if(major_num_g < 0)
    {
        printk(KERN_ALERT "Registering chararacter device failed with %d!\n", major_num_g);
        return major_num_g;
    }
    printk(KERN_INFO "Character device registered!\n");

    shared_buffer_g = vzalloc(SHARED_BUFFER_SIZE);
    shared_buffer_offset_g = 0;

    process_list_size_g = 0;
    mp3_workqueue_g = create_workqueue("mp3_workqueue");

    return SUCCESS;
}

/*
 * Cleans up memory related to the module and removes it
 */
void __exit
my_module_exit(void)
{
    remove_proc_entry(PROCFS_NAME, mp3_proc_dir_g);
    remove_proc_entry(PROC_DIR_NAME, NULL);

    mp3_destroy_process_list();
    destroy_workqueue(mp3_workqueue_g);

    unregister_chrdev(major_num_g, DEVICE_NAME);

    vfree(shared_buffer_g);

    printk(KERN_INFO "MODULE UNLOADED\n");
}

/*
 * Iterate through the list of tasks and update cpu_time, min/maj page faults
 */
void
mp3_update_task_data(void)
{
    struct task_struct * task;

    task_struct_t * stored_task_data = NULL;

    mutex_lock(&process_list_mutex_g);
    rcu_read_lock();

    list_for_each_entry(stored_task_data, &process_list_g, list_node)
    {
        task = stored_task_data->linux_task;

        stored_task_data->cpu_usage = task->utime;
        stored_task_data->min = task->min_flt;
        stored_task_data->maj = task->maj_flt;
        task->utime = 0;
        task->min_flt = 0;
        task->maj_flt = 0;
    }

    rcu_read_unlock();
    mutex_unlock(&process_list_mutex_g);
}


/**
 * Writes the values recorded in the sample to the shared buffer.
 */
void
mp3_write_task_stats_to_shared_buffer(void)
{
    task_struct_t * stored_task_data = NULL;
    ulong * shared_stats_buffer;

    ulong total_cpu_usage;
    ulong total_min_faults;
    ulong total_maj_faults;

    shared_stats_buffer = (ulong *)shared_buffer_g;

    total_cpu_usage = 0;
    total_min_faults = 0;
    total_maj_faults = 0;

    mutex_lock(&process_list_mutex_g);
    list_for_each_entry(stored_task_data, &process_list_g, list_node)
    {
        total_cpu_usage += stored_task_data->cpu_usage;
        total_min_faults += stored_task_data->min;
        total_maj_faults += stored_task_data->maj;
    }

    shared_stats_buffer[shared_buffer_offset_g++] = jiffies;
    shared_stats_buffer[shared_buffer_offset_g++] = total_min_faults;
    shared_stats_buffer[shared_buffer_offset_g++] = total_maj_faults;
    shared_stats_buffer[shared_buffer_offset_g++] = total_cpu_usage;

    //Since SHARED_BUFFER_SIZE is a multiple of 4*ulong we only need to check this
    //after the 4 writes to the buffer.
    if(shared_buffer_offset_g >= SHARED_BUFFER_SIZE / (sizeof(ulong) / sizeof(char)))
    {
        shared_buffer_offset_g = 0;
    }

    mutex_unlock(&process_list_mutex_g);
}

module_init(my_module_init);
module_exit(my_module_exit);

MODULE_LICENSE("GPL");
