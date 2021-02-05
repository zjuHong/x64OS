#ifndef __CPU_H__

#define __CPU_H__

#define NR_CPUS 8

/*

*/

inline void get_cpuid(unsigned int Mop,unsigned int Sop,unsigned int * a,unsigned int * b,unsigned int * c,unsigned int * d)
{
	__asm__ __volatile__	(	"cpuid	\n\t"
					:"=a"(*a),"=b"(*b),"=c"(*c),"=d"(*d)
					:"0"(Mop),"2"(Sop)
				);
}

/*

*/

void init_cpu(void);

#endif
