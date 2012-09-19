#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/timer.h>

#define PROC_DIR_NAME "mp1"
#define PROCFS_NAME "status"

struct proc_dir_entry* proc_file_g;

static struct timer_list timer;

struct proc_dir_entry* mp1_proc_dir_g;

struct process_data{
    int pid_id;
    unsigned long cpu_time;
    struct list_head list_node;
};

LIST_HEAD(process_list_g);

//Function Prototypes
void init_process_list();
void destroy_process_list();
void add_pid_to_list(int pid);
void update_process_times();
unsigned int get_process_times(char ** process_times);

void
init_process_list(){
    //TODO: Initialize list.  May not be needed due to LIST_HEAD macro
}

/**
 * Delete linked list.  Call after removing procfs entries
 */
void
destroy_process_list(){
    //TODO: Delete nodes and free memory.
}

/**
 * Register a new pid when a process registers itself
 */
void
add_pid_to_list(int pid){
    //TODO: Make this thread safe.

}

/**
 * Called by kernel thread to update process information in linked list
 */
void
update_process_times(){
    //TODO: Make this thread safe.
}

/**
 * Retrieves a formatted string of process info.  Returns length of string.
 */
unsigned int
get_process_times(char ** process_times){
    //TODO: Make this thread safe.
}




void
timer_handler(unsigned long data)
{
    printk (KERN_ALERT "TIMER RUN!!!" );

    setup_timer(&timer, timer_handler, 0);
    mod_timer(&timer, jiffies + msecs_to_jiffies (5000));
}

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
    if (offset > 0)
    {
        ret = 0;
    }
    else
    {
        ret = sprintf(buffer, "HelloWorld!\n");
    }
    return ret;
}

int __init
my_module_init(void)
{
    printk(KERN_ALERT "MODULE LOADED\n");

    mp1_proc_dir_g = proc_mkdir(PROC_DIR_NAME, NULL);
    proc_file_g = create_proc_entry(PROCFS_NAME, 0666, mp1_proc_dir_g);

    if(proc_file_g == NULL)
    {
        remove_proc_entry(PROCFS_NAME, mp1_proc_dir_g);
        printk(KERN_ALERT "Error: Could not initialize /proc/%s\n", PROCFS_NAME);
        return -ENOMEM;
    }

    //TODO - figure out why this needs to happen?
    proc_file_g->read_proc = procfile_read;
    proc_file_g->mode = S_IFREG | S_IRUGO;//What are these?
    proc_file_g->uid = 0;
    proc_file_g->gid = 0;
    proc_file_g->size = 37;

    printk(KERN_INFO "/proc/%s created\n", PROCFS_NAME);

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
    printk(KERN_ALERT "MODULE UNLOADED\n");
}

module_init(my_module_init);
module_exit(my_module_exit);

MODULE_LICENSE("GPL");
