#include "unistd.h"
//"syscalls.h"展开了两次，第一次是定义函数名
#define SYSCALL_COMMON(nr,sym)	extern unsigned long sym(void);
SYSCALL_COMMON(0,no_system_call)
#include "syscalls.h"
#undef	SYSCALL_COMMON


#define SYSCALL_COMMON(nr,sym)	[nr] = sym,


#define MAX_SYSTEM_CALL_NR 128
typedef unsigned long (* system_call_t)(void);

//系统调用表，由"syscalls.h"展开，将函数添加到表格中
system_call_t system_call_table[MAX_SYSTEM_CALL_NR] = 
{
	[0 ... MAX_SYSTEM_CALL_NR-1] = no_system_call,
#include "syscalls.h"
};

