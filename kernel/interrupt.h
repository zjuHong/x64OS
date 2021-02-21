#ifndef __INTERRUPT_H__

#define __INTERRUPT_H__

#include "linkage.h"
#include "ptrace.h"

/*

*/

typedef struct hw_int_type
{
	void (*enable)(unsigned long irq);//使能
	void (*disable)(unsigned long irq);//禁止

	unsigned long (*install)(unsigned long irq,void * arg);//安装
	void (*uninstall)(unsigned long irq);//卸载

	void (*ack)(unsigned long irq);//应答
}hw_int_controller;

/*

*/

typedef struct {
	hw_int_controller * controller;//中断操作

	char * irq_name;//中断名
	unsigned long parameter;//参数
	void (*handler)(unsigned long nr, unsigned long parameter, struct pt_regs * regs);//中断处理函数
	unsigned long flags;//标志位
}irq_desc_T;

/*

*/

#define NR_IRQS 24

irq_desc_T interrupt_desc[NR_IRQS] = {0};
irq_desc_T SMP_IPI_desc[10] = {0};

/*

*/

int register_irq(unsigned long irq,
		void * arg,
		void (*handler)(unsigned long nr, unsigned long parameter, struct pt_regs * regs),
		unsigned long parameter,
		hw_int_controller * controller,
		char * irq_name);

/*

*/

int unregister_irq(unsigned long irq);

extern void (* interrupt[24])(void);
extern void (* SMP_interrupt[10])(void);

extern void do_IRQ(struct pt_regs * regs,unsigned long nr);

#endif
