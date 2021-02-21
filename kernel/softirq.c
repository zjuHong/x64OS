#include "softirq.h"
#include "lib.h"

unsigned long softirq_status = 0;//64位bitmap，标志软中断是否触发
struct softirq softirq_vector[64] = {0};

/** 
 * @brief 设置软中断状态
 * @param 
 * @return 
 */
void set_softirq_status(unsigned long status)
{
	softirq_status |= status;
}

/** 
 * @brief 获取软中断状态
 * @param 
 * @return 
 */
unsigned long get_softirq_status()
{
	return softirq_status;
}

/** 
 * @brief 注册软中断
 * @param -nr 中断号
 * @param -action 执行函数
 * @param -data 函数形参
 * @return 
 */
void register_softirq(int nr,void (*action)(void * data),void * data)
{
	softirq_vector[nr].action = action;
	softirq_vector[nr].data = data;
}

/** 
 * @brief 注销软中断
 * @param 
 * @return 
 */
void unregister_softirq(int nr)
{
	softirq_vector[nr].action = NULL;
	softirq_vector[nr].data = NULL;
}

/** 
 * @brief 执行软中断处理函数
 * @param 
 * @return 
 */
void do_softirq()
{
	int i;
	sti();
	for(i = 0;i < 64 && softirq_status;i++)
	{
		if(softirq_status & (1 << i))
		{
			softirq_vector[i].action(softirq_vector[i].data);
			softirq_status &= ~(1 << i);
		}
	}
	cli();
}

/** 
 * @brief 软中断初始化
 * @param 
 * @return 
 */
void softirq_init()
{
	softirq_status = 0;
	memset(softirq_vector,0,sizeof(struct softirq) * 64);
}
