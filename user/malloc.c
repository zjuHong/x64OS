#include "limits.h"

static unsigned long brk_start_address = 0;
static unsigned long brk_used_address = 0;
static unsigned long brk_end_address = 0;

#define	SIZE_ALIGN	(8*sizeof(unsigned long))

void * malloc(unsigned long size)
{
	unsigned long address = 0;
	if(brk_start_address == 0)
		brk_end_address = brk_used_address = brk_start_address = brk(0);

	if(brk_end_address <= brk_used_address + SIZE_ALIGN + size)
		brk_end_address = brk(brk_end_address + ((size + SIZE_ALIGN + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1)));

	address = brk_used_address;
	brk_used_address += size + SIZE_ALIGN;

	return (void *)address;
}


void free(void * address)
{
}



