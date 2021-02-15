#ifndef __TIMER_H__

#define __TIMER_H__

#include "lib.h"

extern unsigned long volatile jiffies;

struct timer_list
{
	struct List list;
	unsigned long expire_jiffies;
	void (* func)(void * data);
	void *data;
};

extern struct timer_list timer_list_head;

void init_timer(struct timer_list * timer,void (* func)(void * data),void *data,unsigned long expire_jiffies);

void add_timer(struct timer_list * timer);

void del_timer(struct timer_list * timer);

void timer_init();

void do_timer();

#endif
