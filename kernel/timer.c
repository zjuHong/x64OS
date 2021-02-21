#include "timer.h"
#include "softirq.h"
#include "printk.h"
#include "lib.h"
#include "memory.h"

unsigned long volatile jiffies = 0;
struct timer_list timer_list_head;
/** 
 * @brief 初始化timer
 * @param 
 * 		-timer 定时器结构
 * 		-func 定时器处理函数
 * 		-data 形参
 * 		-expire_jiffies 定时时间
 * @return 
 */
void init_timer(struct timer_list * timer,void (* func)(void * data),void *data,unsigned long expire_jiffies)
{
	list_init(&timer->list);
	timer->func = func;
	timer->data = data;
	timer->expire_jiffies = jiffies + expire_jiffies;
}

/** 
 * @brief 增加定时器
 * @param -timer
 * @return 
 */
void add_timer(struct timer_list * timer)
{
	struct timer_list * tmp = container_of(list_next(&timer_list_head.list),struct timer_list,list);

	if(list_is_empty(&timer_list_head.list))
	{
		
	}
	else
	{
		while(tmp->expire_jiffies < timer->expire_jiffies) //按延时大小排序
			tmp = container_of(list_next(&tmp->list),struct timer_list,list);
	}

	list_add_to_behind(&tmp->list,&timer->list);
}

/** 
 * @brief 删除定时器
 * @param -timer
 * @return 
 */
void del_timer(struct timer_list * timer)
{
	list_del(&timer->list);
}

/** 
 * @brief 定时器处理函数
 * @param -data
 * @return 
 */
void test_timer(void * data)
{
	//color_printk(BLUE,WHITE,"test_timer");
}

/** 
 * @brief 定时器初始化函数
 * @param 
 * @return 
 */
void timer_init()
{
	struct timer_list *tmp = NULL;
	jiffies = 0;
	init_timer(&timer_list_head,NULL,NULL,-1UL);
	register_softirq(0,&do_timer,NULL);//注册定时器软中断

	tmp = (struct timer_list *)kmalloc(sizeof(struct timer_list),0);
	init_timer(tmp,&test_timer,NULL,5);
	add_timer(tmp);
}

/** 
 * @brief 定时器执行函数
 * @param -data
 * @return 
 */
void do_timer(void * data)
{
	struct timer_list * tmp = container_of(list_next(&timer_list_head.list),struct timer_list,list);
	while((!list_is_empty(&timer_list_head.list)) && (tmp->expire_jiffies <= jiffies))//当前系统时间计数值超过期望计数值
	{
		del_timer(tmp);
		tmp->func(tmp->data);//执行函数
		tmp = container_of(list_next(&timer_list_head.list),struct timer_list,list);
		jiffies = 0;
	}

	//color_printk(RED,WHITE,"(HPET:%ld)",jiffies);
}
