#ifndef __TASK_H__

#define __TASK_H__

#include "memory.h"
#include "cpu.h"
#include "lib.h"
#include "ptrace.h"
#include "VFS.h"

#define KERNEL_CS 	(0x08)
#define	KERNEL_DS 	(0x10)

#define	USER_CS		(0x28)
#define USER_DS		(0x30)

//表示进程的内核栈空间和task_struct结构体占用的存储空间总量为32KB
#define STACK_SIZE 32768
//用来表示地址的外部标号,理解成汇编级的标号
extern char _text;
extern char _etext;
extern char _data;
extern char _edata;
extern char _rodata;
extern char _erodata;
extern char _bss;
extern char _ebss;
extern char _end;

extern unsigned long _stack_start;
extern long global_pid;
extern int fat32_init_flag;

/*

*/

#define TASK_RUNNING		(1 << 0)
#define TASK_INTERRUPTIBLE	(1 << 1)
#define	TASK_UNINTERRUPTIBLE	(1 << 2)
#define	TASK_ZOMBIE		(1 << 3)	
#define	TASK_STOPPED		(1 << 4)

/*
内存空间分布结构体
*/
struct mm_struct
{
	pml4t_t *pgd;	//内存页表指针
	
	unsigned long start_code,end_code;//代码段空间
	unsigned long start_data,end_data;//数据段空间
	unsigned long start_rodata,end_rodata;//只读数据段空间
	unsigned long start_bss,end_bss;//bss段空间
	unsigned long start_brk,end_brk;//堆区域
	unsigned long start_stack;//应用层栈基地址
};

/*
进程切换时保留的状态信息
*/
struct thread_struct
{
	unsigned long rsp0;	//内核层栈基地址

	unsigned long rip;//内核层代码指针
	unsigned long rsp;//内核层当前栈指针	

	unsigned long fs;//fs段寄存器
	unsigned long gs;//gs段寄存器

	unsigned long cr2;//cr2寄存器
	unsigned long trap_nr;//产生异常的异常号
	unsigned long error_code;//异常错误码
};

#define TASK_FILE_MAX	10
/*
进程结构体
*/
struct task_struct
{
	volatile long state;//进程状态
	unsigned long flags;//进程标志
	long preempt_count;//自旋锁数量
	long signal;
	long cpu_id;		//CPU ID

	struct mm_struct *mm;//内存空间分布结构体
	struct thread_struct *thread;//进程切换时保留的状态信息

	struct List list;//连接各个进程控制结构体

	unsigned long addr_limit;	/*0x0000,0000,0000,0000 - 0x0000,7fff,ffff,ffff user*/
					/*0xffff,8000,0000,0000 - 0xffff,ffff,ffff,ffff kernel*/
	long pid;//进程id号
	long priority;//进程优先级
	long vrun_time;//可用时间片

	struct file * file_struct[TASK_FILE_MAX];//文件描述符

	struct task_struct *next;
	struct task_struct *parent;//父进程
};

///////struct task_struct->flags:

#define PF_KTHREAD	(1UL << 0)
#define NEED_SCHEDULE	(1UL << 1)
#define PF_VFORK	(1UL << 2)	//当前进程的资源是否在共享


union task_union
{//task_struct和内核层栈空间相连
	struct task_struct task;
	unsigned long stack[STACK_SIZE / sizeof(unsigned long)];
}__attribute__((aligned (8)));	//8Bytes align



#define INIT_TASK(tsk)	\
{			\
	.state = TASK_UNINTERRUPTIBLE,		\
	.flags = PF_KTHREAD,		\
	.preempt_count = 0,		\
	.signal = 0,		\
	.cpu_id = 0,		\
	.mm = &init_mm,			\
	.thread = &init_thread,		\
	.addr_limit = 0xffffffffffffffff,	\
	.pid = 0,			\
	.priority = 2,		\
	.vrun_time = 0,		\
	.file_struct = {0},	\
	.next = &tsk,		\
	.parent = &tsk,		\
}

/*
tss数据结构
*/
struct tss_struct
{
	unsigned int  reserved0;
	unsigned long rsp0;
	unsigned long rsp1;
	unsigned long rsp2;
	unsigned long reserved1;
	unsigned long ist1;
	unsigned long ist2;
	unsigned long ist3;
	unsigned long ist4;
	unsigned long ist5;
	unsigned long ist6;
	unsigned long ist7;
	unsigned long reserved2;
	unsigned short reserved3;
	unsigned short iomapbaseaddr;
}__attribute__((packed));

#define INIT_TSS \
{	.reserved0 = 0,	 \
	.rsp0 = (unsigned long)(init_task_union.stack + STACK_SIZE / sizeof(unsigned long)),	\
	.rsp1 = (unsigned long)(init_task_union.stack + STACK_SIZE / sizeof(unsigned long)),	\
	.rsp2 = (unsigned long)(init_task_union.stack + STACK_SIZE / sizeof(unsigned long)),	\
	.reserved1 = 0,	 \
	.ist1 = 0xffff800000007c00,	\
	.ist2 = 0xffff800000007c00,	\
	.ist3 = 0xffff800000007c00,	\
	.ist4 = 0xffff800000007c00,	\
	.ist5 = 0xffff800000007c00,	\
	.ist6 = 0xffff800000007c00,	\
	.ist7 = 0xffff800000007c00,	\
	.reserved2 = 0,	\
	.reserved3 = 0,	\
	.iomapbaseaddr = 0	\
}


/** 
 * @brief 获取task_struct结构的地址：利用联合体的特性，将rsp按32k下边界对齐即可
 * @param 
 * @return 
 */
inline	struct task_struct * get_current()
{
	struct task_struct * current = NULL;
	__asm__ __volatile__ ("andq %%rsp,%0	\n\t":"=r"(current):"0"(~32767UL));
	return current;
}

#define current get_current()

#define GET_CURRENT			\
	"movq	%rsp,	%rbx	\n\t"	\
	"andq	$-32768,%rbx	\n\t"

/** 
 * @brief 进程切换
 * @param 
 * 		-RDI保存prev结构
 * 		-RSI保存next结构
 * @return 
 */
#define switch_to(prev,next)			\
do{							\
	__asm__ __volatile__ (	"pushq	%%rbp	\n\t"	\
				"pushq	%%rax	\n\t"	\
				"movq	%%rsp,	%0	\n\t"	\
				"movq	%2,	%%rsp	\n\t"	\
				"leaq	1f(%%rip),	%%rax	\n\t"	\
				"movq	%%rax,	%1	\n\t"	\
				"pushq	%3		\n\t"	\
				"jmp	__switch_to	\n\t"	\
				"1:	\n\t"	\
				"popq	%%rax	\n\t"	\
				"popq	%%rbp	\n\t"	\
				:"=m"(prev->thread->rsp),"=m"(prev->thread->rip)		\
				:"m"(next->thread->rsp),"m"(next->thread->rip),"D"(prev),"S"(next)	\
				:"memory"		\
				);			\
}while(0)
//prev进程(当前进程)通过调用switch_to模块保存RSP,并指定切换回prev时的RIP值,此处默认保存到标识符1:处,随后将next进程的栈指针恢复到RSP
//再把next的执行现场RIP压入next进程的内核层栈空间(RSP寄存器的恢复在前,此后的数据将压入next进程的内核层栈空间),最后借助JMP指令执行__switch_to
//__switch_to会在返回过程中执行RET指令,进而跳转到next进程继续执行(恢复执行现场的RIP寄存器),至此进程间切换工作执行完毕
/*

*/
long get_pid();
struct task_struct *get_task(long pid);

inline void wakeup_process(struct task_struct *tsk);
void exit_files(struct task_struct *tsk);

unsigned long do_fork(struct pt_regs * regs, unsigned long clone_flags, unsigned long stack_start, unsigned long stack_size);
unsigned long do_execve(struct pt_regs *regs,char *name);
unsigned long do_exit(unsigned long exit_code);

void task_init();

inline void switch_mm(struct task_struct *prev,struct task_struct *next);

extern void ret_system_call(void);
extern void system_call(void);

extern struct task_struct *init_task[NR_CPUS];
extern union task_union init_task_union;
extern struct mm_struct init_mm;
extern struct thread_struct init_thread;

extern struct tss_struct init_tss[NR_CPUS];

#endif
