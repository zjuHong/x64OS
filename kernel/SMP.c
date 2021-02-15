#include "SMP.h"
#include "printk.h"
#include "lib.h"
#include "gate.h"
#include "interrupt.h"
#include "task.h"

int global_i = 0;

void Start_SMP()
{
	memset(current,0,sizeof(struct task_struct));
}
