#include "linkage.h"

.section .text

ENTRY(_start)
//令各个段寄存器加载数据段选择子0x10
	mov	%ss,	%ax
	mov	%ax,	%ds
	mov	%ax,	%es
	mov	%ax,	%fs
	mov	%ax,	%ss
	mov	$0x7E00,	%esp

//加载页目录基地址寄存器PDBR=>cr3由于引导加载程序将系统内核加载到物理地址1MB处,而页表PML4的起始地址位于内核程序1000h偏移处,那么页表PML4的物理起始地址应该是1MB + 1000h =101000h处
	movq	$0x101000,	%rax
	movq	%rax,		%cr3

//=======	load GDTR

	lgdt	GDT_POINTER(%rip)

//=======	load	IDTR

	lidt	IDT_POINTER(%rip)

//令各个段寄存器加载数据段选择子0x10
	mov	$0x10,	%ax
	mov	%ax,	%ds
	mov	%ax,	%es
	mov	%ax,	%fs
	mov	%ax,	%gs
	mov	%ax,	%ss

	movq	_stack_start(%rip),	%rsp

//=======	load	cr3   加载页目录基地址

	movq	$0x101000,	%rax
	movq	%rax,		%cr3
	movq	switch_seg(%rip),	%rax
	pushq	$0x08
	pushq	%rax
	lretq

//=======	64-bit mode code

switch_seg:
	.quad	entry64

entry64:
	movq	$0x10,	%rax
	movq	%rax,	%ds
	movq	%rax,	%es
	movq	%rax,	%gs
	movq	%rax,	%ss
	movq	_stack_start(%rip),	%rsp		/* rsp address */

	movq	$0x1b,	%rcx		//if APU
	rdmsr
	bt	$8,	%rax
	jnc	start_smp

//初始化IDT表的中断描述符
setup_IDT:							
	leaq	ignore_int(%rip),	%rdx
	movq	$(0x08 << 16),	%rax
	movw	%dx,	%ax
	movq	$(0x8E00 << 32),	%rcx		
	addq	%rcx,	%rax
	movl	%edx,	%ecx
	shrl	$16,	%ecx
	shlq	$48,	%rcx
	addq	%rcx,	%rax
	shrq	$32,	%rdx
	leaq	IDT_Table(%rip),	%rdi
	mov	$256,	%rcx
rp_sidt:		//逐一初始化
	movq	%rax,	(%rdi)
	movq	%rdx,	8(%rdi)
	addq	$0x10,	%rdi
	dec	%rcx
	jne	rp_sidt

setup_TSS64:
	leaq	init_tss(%rip),	%rdx
	xorq	%rax,	%rax
	xorq	%rcx,	%rcx
	movq	$0x89,	%rax
	shlq	$40,	%rax
	movl	%edx,	%ecx
	shrl	$24,	%ecx
	shlq	$56,	%rcx
	addq	%rcx,	%rax
	xorq	%rcx,	%rcx
	movl	%edx,	%ecx
	andl	$0xffffff,	%ecx
	shlq	$16,	%rcx
	addq	%rcx,	%rax
	addq	$103,	%rax
	leaq	GDT_Table(%rip),	%rdi
	movq	%rax,	80(%rdi)	//tss segment offset
	shrq	$32,	%rdx
	movq	%rdx,	88(%rdi)	//tss+1 segment offset

//	mov	$0x50,	%ax
//	ltr	%ax

	movq	go_to_kernel(%rip),	%rax		/* movq address */
	pushq	$0x08
	pushq	%rax
	lretq

go_to_kernel:
	.quad	Start_Kernel

start_smp:
	movq	go_to_smp_kernel(%rip),	%rax		/* movq address */
	pushq	$0x08
	pushq	%rax
	lretq

go_to_smp_kernel:
	.quad	Start_SMP


//=======	ignore_int

ignore_int:
	cld
	//保存各个寄存器值
	pushq	%rax
	pushq	%rbx
	pushq	%rcx
	pushq	%rdx
	pushq	%rbp
	pushq	%rdi
	pushq	%rsi

	pushq	%r8
	pushq	%r9
	pushq	%r10
	pushq	%r11
	pushq	%r12
	pushq	%r13
	pushq	%r14
	pushq	%r15

	//ds es设置为内核数据段
	movq	%es,	%rax
	pushq	%rax
	movq	%ds,	%rax
	pushq	%rax

	//为color_printk准备参数
	movq	$0x10,	%rax
	movq	%rax,	%ds
	movq	%rax,	%es

	leaq	int_msg(%rip),	%rax			/* leaq get address */
	pushq	%rax
	movq	%rax,	%rdx
	movq	$0x00000000,	%rsi
	movq	$0x00ff0000,	%rdi
	movq	$0,	%rax
	callq	color_printk
	addq	$0x8,	%rsp

Loop:
	jmp	Loop	

	popq	%rax
	movq	%rax,	%ds
	popq	%rax
	movq	%rax,	%es

	popq	%r15
	popq	%r14
	popq	%r13
	popq	%r12
	popq	%r11
	popq	%r10
	popq	%r9
	popq	%r8

	popq	%rsi
	popq	%rdi
	popq	%rbp
	popq	%rdx
	popq	%rcx
	popq	%rbx
	popq	%rax
	iretq

int_msg:
	.asciz "Unknown interrupt or fault at RIP\n"

ENTRY(_stack_start)		//与rsp0成员变量的值相同，都指向系统第一个进程的栈基地址
	.quad	init_task_union + 32768

//=======	init page
.align 8

.org	0x1000
//页目录的起始线性地址位于0xFFFF 8000 0010 0000 + 0x1000 = 0xFFFF 8000 0010 1000处
__PML4E:

	.quad	0x102003
	.fill	255,8,0
	.quad	0x102003
	.fill	255,8,0

.org	0x2000

__PDPTE:
	
	.quad	0x103003	/* 0x103003 */
	.fill	511,8,0

.org	0x3000

__PDE:

	.quad	0x000083	
	.quad	0x200083
	.quad	0x400083
	.quad	0x600083
	.quad	0x800083		/* 0x800083 */
	.quad	0xa00083
	.quad	0xc00083
	.quad	0xe00083
	.quad	0x1000083
	.quad	0x1200083
	.quad	0x1400083
	.quad	0x1600083
	.quad	0x1800083
	.quad	0x1a00083
	.quad	0x1c00083
	.quad	0x1e00083
	.quad	0x2000083
	.quad	0x2200083
	.quad	0x2400083
	.quad	0x2600083
	.quad	0x2800083
	.quad	0x2a00083
	.quad	0x2c00083
	.quad	0x2e00083



	.quad	0xe0000083		/*0x 3000000*/
	.quad	0xe0200083
	.quad	0xe0400083
	.quad	0xe0600083
	.quad	0xe0800083
	.quad	0xe0a00083
	.quad	0xe0c00083
	.quad	0xe0e00083
	.fill	480,8,0
//init page程序段将前10MB物理内存分别映射到线性地址0处和0xFFFF 8000 0000 0000处,
//接着把物理地址0xE000 0000开始的16MB内存映射到线性地址0xA0 0000处和0xFFFF 8000 00A0 0000处,
//最后使用伪指令.fill将数值0填充到页表的剩余499个页表项里

//=======	GDT_Table

.section .data

.globl GDT_Table

GDT_Table:
	.quad	0x0000000000000000			/*0	NULL descriptor		       	00*/
	.quad	0x0020980000000000			/*1	KERNEL	Code	64-bit	Segment	08*/
	.quad	0x0000920000000000			/*2	KERNEL	Data	64-bit	Segment	10*/
	.quad	0x0000000000000000			/*3	USER	Code	32-bit	Segment 18*/
	.quad	0x0000000000000000			/*4	USER	Data	32-bit	Segment 20*/
	.quad	0x0020f80000000000			/*5	USER	Code	64-bit	Segment	28*/
	.quad	0x0000f20000000000			/*6	USER	Data	64-bit	Segment	30*/
	.quad	0x00cf9a000000ffff			/*7	KERNEL	Code	32-bit	Segment	38*/
	.quad	0x00cf92000000ffff			/*8	KERNEL	Data	32-bit	Segment	40*/
	.fill	100,8,0					/*10 ~ 11 TSS (jmp one segment <9>) in long-mode 128-bit 50*/
GDT_END:

GDT_POINTER:
GDT_LIMIT:	.word	GDT_END - GDT_Table - 1
GDT_BASE:	.quad	GDT_Table

//=======	IDT_Table

.globl IDT_Table

IDT_Table:
	.fill  512,8,0
IDT_END:

IDT_POINTER:
IDT_LIMIT:	.word	IDT_END - IDT_Table - 1
IDT_BASE:	.quad	IDT_Table
