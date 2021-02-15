#ifndef __SCHEDULE_H__

#define __SCHEDULE_H__

#include "task.h"

struct schedule
{
	long running_task_count;
	long CPU_exec_task_jiffies;
	struct task_struct task_queue;
};

extern struct schedule task_schedule[NR_CPUS];

void schedule();
void schedule_init();
struct task_struct *get_next_task();
void insert_task_queue(struct task_struct *tsk);

#endif

