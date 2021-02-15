#ifndef __SMP_H__

#define __SMP_H__

extern int global_i;

#define SMP_cpu_id()	(current->cpu_id)

void Start_SMP();

#endif
