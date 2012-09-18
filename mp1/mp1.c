#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/timer.h>


#define PROCFS_NAME "mp1"

struct proc_dir_entry* proc_file_g;

static struct timer_list timer;

void
timer_handler(unsigned long data)
{
	printk (KERN_ALERT "TIMER RUN!!!" );

	setup_timer ( &timer, timer_handler, 0);
	mod_timer ( &timer, jiffies + msecs_to_jiffies (5000) );
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
    //TODO
}

int __init
my_module_init(void)
{
    printk(KERN_ALERT "MODULE LOADED\n");
    proc_file_g = create_proc_entry(procfs_name, 0666, NULL);

    if(proc_file_g == NULL)
    {
        remove_proc_entry(PROCFS_NAME, &proc_root);
        printk(KERN_ALERT "Error: Could not initialize /proc/%s\n", PROCFS_NAME);
        return -ENOMEM;
    }

    //TODO - figure out why this needs to happen?
    proc_file_g->read_proc = procfile_read;
    proc_file_g->owner = THIS_MODULE;
    proc_file_g->mode = S_IFREG | S_IRUGO;
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
    remove_proc_entry(PROCFS_NAME, &proc_root);
    del_timer ( &timer );
    printk(KERN_ALERT "MODULE UNLOADED\n");
}

module_init(my_module_init);
module_exit(my_module_exit);

MODULE_LICENSE("GPL");
