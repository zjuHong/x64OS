#ifndef __SOFTIRQ_H__

#define __SOFTIRQ_H__

#define TIMER_SIRQ	(1 << 0)

unsigned long softirq_status = 0;

struct softirq
{
	void (*action)(void * data);
	void * data;
};

struct softirq softirq_vector[64] = {0};


void register_softirq(int nr,void (*action)(void * data),void * data);
void unregister_softirq(int nr);

void set_softirq_status(unsigned long status);
unsigned long get_softirq_status();
void do_softirq();

void softirq_init();

#endif
