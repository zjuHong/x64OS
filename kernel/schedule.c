#include "schedule.h"
#include "task.h"
#include "lib.h"
#include "printk.h"
#include "timer.h"
#include "SMP.h"

struct schedule task_schedule[NR_CPUS];
/** 
 * @brief 就绪进程出队函数
 * @param 
 * @return 
 */
struct task_struct *get_next_task()
{
	struct task_struct * tsk = NULL;

	if(list_is_empty(&task_schedule[SMP_cpu_id()].task_queue.list))
	{
		return init_task[SMP_cpu_id()];
	}

	tsk = container_of(list_next(&task_schedule[SMP_cpu_id()].task_queue.list),struct task_struct,list);
	list_del(&tsk->list);

	task_schedule[SMP_cpu_id()].running_task_count -= 1;

	return tsk;
}
/** 
 * @brief 就绪进程入队函数
 * @param 
 * @return 
 */
void insert_task_queue(struct task_struct *tsk)
{
	struct task_struct *tmp = NULL;

	if(tsk == init_task[SMP_cpu_id()])
		return ;

	tmp = container_of(list_next(&task_schedule[SMP_cpu_id()].task_queue.list),struct task_struct,list);

	if(list_is_empty(&task_schedule[SMP_cpu_id()].task_queue.list))
	{
	}
	else
	{
		while(tmp->vrun_time < tsk->vrun_time)
			tmp = container_of(list_next(&tmp->list),struct task_struct,list);
	}

	list_add_to_before(&tmp->list,&tsk->list);
	task_schedule[SMP_cpu_id()].running_task_count += 1;
}
/** 
 * @brief 进程调度器，此处为了直观验证shell的效果，将其注释
 * @param 
 * @return 
 */
void schedule()
{
}
/*	struct task_struct *tsk = NULL;
	long cpu_id = SMP_cpu_id();

	cli();//关中断

	current->flags &= ~NEED_SCHEDULE;
	tsk = get_next_task();
//	color_printk(YELLOW,BLACK,"RFLAGS:%#018lx\n",get_rflags());
	color_printk(YELLOW,BLACK,"#schedule:%ld,pid:%ld(%ld)=>>pid:%ld(%ld)#\n",jiffies,current->pid,current->vrun_time,tsk->pid,tsk->vrun_time);

	if(current->vrun_time >= tsk->vrun_time || current->state != TASK_RUNNING)//时间片用完或者状态改变
	{
		if(current->state == TASK_RUNNING)
			insert_task_queue(current);
			
		if(!task_schedule[cpu_id].CPU_exec_task_jiffies)//重新分配时间片
			switch(tsk->priority)//根据进程优先级调整时间片消耗速度
			{
				case 0:
				case 1:
					task_schedule[cpu_id].CPU_exec_task_jiffies = 4/task_schedule[cpu_id].running_task_count;
					break;
				case 2:
				default:
					task_schedule[cpu_id].CPU_exec_task_jiffies = 4/task_schedule[cpu_id].running_task_count*3;
					break;
			}
		
		switch_mm(current,tsk);
		switch_to(current,tsk);	
	}
	else //当前进程的运行时间最少
	{
		insert_task_queue(tsk);
		
		if(!task_schedule[cpu_id].CPU_exec_task_jiffies)
			switch(tsk->priority)
			{
				case 0:
				case 1:
					task_schedule[cpu_id].CPU_exec_task_jiffies = 4/task_schedule[cpu_id].running_task_count;
					break;
				case 2:
				default:
					task_schedule[cpu_id].CPU_exec_task_jiffies = 4/task_schedule[cpu_id].running_task_count*3;
					break;
			}
	}

	sti();//恢复中断
}
*/

/** 
 * @brief 就绪队列初始化
 * @param 
 * @return 
 */
void schedule_init()
{
	int i = 0;
	memset(&task_schedule,0,sizeof(struct schedule) * NR_CPUS);

	for(i = 0;i<NR_CPUS;i++)
	{
		list_init(&task_schedule[i].task_queue.list);
		task_schedule[i].task_queue.vrun_time = 0x7fffffffffffffff;//idle进程暂时设置为最大值

		task_schedule[i].running_task_count = 1;
		task_schedule[i].CPU_exec_task_jiffies = 4;
	}
}
