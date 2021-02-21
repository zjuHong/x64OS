#ifndef __SCHED_H__

#define __SCHED_H__

/*
 * cloning flags:
 */


#define CLONE_VM		(1 << 0)	/* 共享进程的地址空间 */
#define CLONE_FS		(1 << 1)	/* 共享进程的打开文件 */
#define CLONE_SIGNAL	(1 << 2)	/* 共享进程拥有的信号 */


#endif
