#include "UEFI_boot_param_info.h"
#include "lib.h"
#include "printk.h"
//#include "spinlock.h"
#include "gate.h"
#include "trap.h"
#include "memory.h"
#include "task.h"
#include "cpu.h"
#include "interrupt.h"

#if  APIC
#include "APIC.h"
#else
#include "8259A.h"
#endif

#include "time.h"

#include "keyboard.h"
#include "mouse.h"
#include "disk.h"

/*
	static var 
*/

struct Global_Memory_Descriptor memory_management_struct = {{0},0};
struct KERNEL_BOOT_PARAMETER_INFORMATION *boot_para_info = (struct KERNEL_BOOT_PARAMETER_INFORMATION *)0xffff800000060000;

void Start_Kernel(void)
{
	unsigned int i;
	char buf[512];
	struct time time;

	memset((void*)&_bss,0,(unsigned long)&_end-(unsigned long)&_bss);

	Pos.XResolution = boot_para_info->Graphics_Info.HorizontalResolution;
	Pos.YResolution = boot_para_info->Graphics_Info.VerticalResolution;

	Pos.XPosition = 0;
	Pos.YPosition = 0;

	Pos.XCharSize = 8;
	Pos.YCharSize = 16;

	Pos.FB_addr = (int *)0xffff800003000000;
	Pos.FB_length = boot_para_info->Graphics_Info.FrameBufferSize;
	
	//spin_init(&Pos.printk_lock);
	
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

	get_cmos_time(&time);

	color_printk(RED,BLACK,"year:%#010x,month:%#010x,day:%#010x,hour:%#010x,mintue:%#010x,second:%#010x\n",time.year,time.month,time.day,time.hour,time.minute,time.second);

	color_printk(RED,BLACK,"keyboard init \n");
	keyboard_init();

	color_printk(RED,BLACK,"mouse init \n");
	mouse_init();

	color_printk(RED,BLACK,"disk init \n");
	disk_init();
	
	//	color_printk(RED,BLACK,"task_init \n");
	//	task_init();

	sti();

	color_printk(RED,BLACK,"start while(1) \n");
	while(1)
	{
		if (p_kb->count)
			analysis_keycode();
		if (p_mouse->count)
			analysis_mousecode();
	}
}
