#ifndef __MP3_GIVEN_H__
#define __MP3_GIVEN_H__

#include <linux/pid.h>
#include <linux/sched.h>

struct task_struct* find_task_by_pid(unsigned int nr)
{
    struct task_struct* task;
    rcu_read_lock();
    task=pid_task(find_vpid(nr), PIDTYPE_PID);
    rcu_read_unlock();

    return task;
}


#endif
