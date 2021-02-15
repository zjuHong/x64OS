#ifndef __SCHED_H__

#define __SCHED_H__

/*
 * cloning flags:
 */


#define CLONE_VM	(1 << 0)	/* shared Virtual Memory between processes */
#define CLONE_FS	(1 << 1)	/* shared fs info between processes */
#define CLONE_VFORK	(1 << 2)	/* free the parent Virtual Memory Space and wake up parent */
#define CLONE_SIGNAL	(1 << 3)

#endif
