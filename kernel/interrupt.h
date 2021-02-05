#ifndef __INTERRUPT_H__

#define __INTERRUPT_H__

#include "linkage.h"
#include "ptrace.h"

/*

*/

typedef struct hw_int_type
{
	void (*enable)(unsigned long irq);
	void (*disable)(unsigned long irq);

	unsigned long (*install)(unsigned long irq,void * arg);
	void (*uninstall)(unsigned long irq);

	void (*ack)(unsigned long irq);
}hw_int_controller;

/*

*/

typedef struct {
	hw_int_controller * controller;

	char * irq_name;
	unsigned long parameter;
	void (*handler)(unsigned long nr, unsigned long parameter, struct pt_regs * regs);
	unsigned long flags;
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

/*

*/

int register_IPI(unsigned long irq,
		void * arg,
		void (*handler)(unsigned long nr, unsigned long parameter, struct pt_regs * regs),
		unsigned long parameter,
		hw_int_controller * controller,
		char * irq_name);

/*

*/

int unregister_IPI(unsigned long irq);

extern void (* interrupt[24])(void);
extern void (* SMP_interrupt[10])(void);

extern void do_IRQ(struct pt_regs * regs,unsigned long nr);

#endif
