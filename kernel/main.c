#include "UEFI_boot_param_info.h"
#include "lib.h"
#include "printk.h"
#include "gate.h"
#include "trap.h"
#include "memory.h"
#include "task.h"
#include "cpu.h"

#if  APIC
#include "APIC.h"
#else
#include "8259A.h"
#endif

#include "keyboard.h"
#include "mouse.h"
#include "disk.h"
#include "SMP.h"
#include "spinlock.h"
#include "HPET.h"
#include "timer.h"

#include "atomic.h"
#include "semaphore.h"
#include "fat32.h"

/*
	static var 
*/

struct KERNEL_BOOT_PARAMETER_INFORMATION *boot_para_info = (struct KERNEL_BOOT_PARAMETER_INFORMATION *)0xffff800000060000;
/** 
 * @brief 内核主函数
 * @param 
 * @return 
 */
void Start_Kernel(void)
{
	unsigned int i;
	int argc = 0;
	char **argv = NULL;
	int index = -1;
	int fd = 0;
	int count = 0;
	unsigned char buf[256] = {0};

	unsigned char * ptr = NULL;

	memset((void*)&_bss,0,(unsigned long)&_end-(unsigned long)&_bss);

	Pos.XResolution = boot_para_info->Graphics_Info.HorizontalResolution;//使用引导程序获取的屏幕的信息
	Pos.YResolution = boot_para_info->Graphics_Info.VerticalResolution;

	Pos.XPosition = 0;
	Pos.YPosition = 0;

	Pos.XCharSize = 8;
	Pos.YCharSize = 16;

	Pos.FB_addr = (int *)0xffff800003000000;
	Pos.FB_length = boot_para_info->Graphics_Info.FrameBufferSize;
	global_pid = 1;
	
	spin_init(&Pos.printk_lock);
	
	load_TR(10);

	set_tss64((unsigned int *)&init_tss[0],_stack_start, _stack_start, _stack_start, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00);

	sys_vector_init();

	init_cpu();

	memory_management_struct.start_code = (unsigned long)& _text;
	memory_management_struct.end_code   = (unsigned long)& _etext;
	memory_management_struct.end_data   = (unsigned long)& _edata;
	memory_management_struct.end_rodata = (unsigned long)& _erodata;
	memory_management_struct.start_brk  = (unsigned long)& _end;

	color_printk(RED,BLACK,"memory init \n");
	init_memory();

	color_printk(RED,BLACK,"slab init \n");
	slab_init();
	
	ptr = (unsigned char *)kmalloc(STACK_SIZE,0) + STACK_SIZE;
	((struct task_struct *)(ptr - STACK_SIZE))->cpu_id = 0;
		
	init_tss[0].ist1 = (unsigned long)ptr;
	init_tss[0].ist2 = (unsigned long)ptr;
	init_tss[0].ist3 = (unsigned long)ptr;
	init_tss[0].ist4 = (unsigned long)ptr;
	init_tss[0].ist5 = (unsigned long)ptr;
	init_tss[0].ist6 = (unsigned long)ptr;
	init_tss[0].ist7 = (unsigned long)ptr;
		
	color_printk(RED,BLACK,"frame buffer init \n");
	frame_buffer_init();
	color_printk(WHITE,BLACK,"frame_buffer_init() is OK \n");

	color_printk(RED,BLACK,"pagetable init \n");	
	pagetable_init();

	color_printk(RED,BLACK,"interrupt init \n");

#if  APIC
	APIC_IOAPIC_init();
#else
	init_8259A();
#endif

	color_printk(RED,BLACK,"Schedule init \n");
	schedule_init();

	color_printk(RED,BLACK,"Soft IRQ init \n");
	softirq_init();

	color_printk(RED,BLACK,"keyboard init \n");
	keyboard_init();

	color_printk(RED,BLACK,"mouse init \n");
	mouse_init();

	color_printk(RED,BLACK,"disk init \n");
	disk_init();

	color_printk(RED,BLACK,"Timer & Clock init \n");
	timer_init();
	HPET_init();

	color_printk(RED,BLACK,"task_init \n");
	task_init();
	sti();

	color_printk(RED,BLACK,"start while(1) \n");
	
	while(1)
	{
		color_printk(WHITE,BLACK,"[SHELL]#:");
		memset(buf,0,256);
		count = read_line(fd,buf);
		if (count == 0)
			continue;

		color_printk(WHITE,BLACK,"\n");
		index = parse_command(buf,&argc,&argv);

		if(index < 0)
			color_printk(WHITE,BLACK,"Input Error,No Command Found!\n");
		else
			run_command(index,argc,argv);	//argc,argv

	}
	// while(1)
	// {
	// 	if (p_kb->count)
	// 		analysis_keycode();
	// 	if (p_mouse->count)
	// 		analysis_mousecode();
	// }
}
