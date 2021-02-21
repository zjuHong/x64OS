#ifndef __SCHEDULE_H__

#define __SCHEDULE_H__

#include "task.h"

struct schedule
{
	long running_task_count;//队列内进程数
	long CPU_exec_task_jiffies;//可执行时间片数量
	struct task_struct task_queue;//进程准备就绪队列的队列头
};

extern struct schedule task_schedule[NR_CPUS];

void schedule();
void schedule_init();
struct task_struct *get_next_task();
void insert_task_queue(struct task_struct *tsk);

#endif

