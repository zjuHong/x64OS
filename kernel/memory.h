#ifndef __MEMORY_H__

#define __MEMORY_H__

#include "UEFI_boot_param_info.h"
#include "lib.h"

//	8Bytes per cell
#define PTRS_PER_PAGE	512		//页表项个数	4KB / 8B

/*

*/

#define PAGE_OFFSET	((unsigned long)0xffff800000000000)	//内核层起始线性地址
#define	TASK_SIZE	((unsigned long)0x00007fffffffffff)

#define PAGE_GDT_SHIFT	39//几个指数
#define PAGE_1G_SHIFT	30
#define PAGE_2M_SHIFT	21
#define PAGE_4K_SHIFT	12

#define PAGE_2M_SIZE	(1UL << PAGE_2M_SHIFT)
#define PAGE_4K_SIZE	(1UL << PAGE_4K_SHIFT)

#define PAGE_2M_MASK	(~ (PAGE_2M_SIZE - 1))//屏蔽低于2m的数值
#define PAGE_4K_MASK	(~ (PAGE_4K_SIZE - 1))

#define PAGE_2M_ALIGN(addr)	(((unsigned long)(addr) + PAGE_2M_SIZE - 1) & PAGE_2M_MASK)//按2m页的上边界对齐
#define PAGE_4K_ALIGN(addr)	(((unsigned long)(addr) + PAGE_4K_SIZE - 1) & PAGE_4K_MASK)

#define Virt_To_Phy(addr)	((unsigned long)(addr) - PAGE_OFFSET)//虚拟地址转换为物理地址
#define Phy_To_Virt(addr)	((unsigned long *)((unsigned long)(addr) + PAGE_OFFSET))//物理转换为虚拟地址

#define Virt_To_2M_Page(kaddr)	(memory_management_struct.pages_struct + (Virt_To_Phy(kaddr) >> PAGE_2M_SHIFT))
#define Phy_to_2M_Page(kaddr)	(memory_management_struct.pages_struct + ((unsigned long)(kaddr) >> PAGE_2M_SHIFT))

////page table attribute

//	bit 63	Execution Disable:
#define PAGE_XD		(1UL << 63)

//	bit 12	Page Attribute Table
#define	PAGE_PAT	(1UL << 12)

//	bit 8	Global Page:1,global;0,part
#define	PAGE_Global	(1UL << 8)

//	bit 7	Page Size:1,big page;0,small page;
#define	PAGE_PS		(1UL << 7)

//	bit 6	Dirty:1,dirty;0,clean;
#define	PAGE_Dirty	(1UL << 6)

//	bit 5	Accessed:1,visited;0,unvisited;
#define	PAGE_Accessed	(1UL << 5)

//	bit 4	Page Level Cache Disable
#define PAGE_PCD	(1UL << 4)

//	bit 3	Page Level Write Through
#define PAGE_PWT	(1UL << 3)

//	bit 2	User Supervisor:1,user and supervisor;0,supervisor;
#define	PAGE_U_S	(1UL << 2)

//	bit 1	Read Write:1,read and write;0,read;
#define	PAGE_R_W	(1UL << 1)

//	bit 0	Present:1,present;0,no present;
#define	PAGE_Present	(1UL << 0)

//1,0
#define PAGE_KERNEL_GDT		(PAGE_R_W | PAGE_Present)

//1,0	
#define PAGE_KERNEL_Dir		(PAGE_R_W | PAGE_Present)

//7,1,0
#define	PAGE_KERNEL_Page	(PAGE_PS  | PAGE_R_W | PAGE_Present)

//1,0
#define PAGE_USER_GDT		(PAGE_U_S | PAGE_R_W | PAGE_Present)

//2,1,0
#define PAGE_USER_Dir		(PAGE_U_S | PAGE_R_W | PAGE_Present)

//7,2,1,0
#define	PAGE_USER_Page		(PAGE_PS  | PAGE_U_S | PAGE_R_W | PAGE_Present)

/*

*/

typedef struct {unsigned long pml4t;} pml4t_t;
#define	mk_mpl4t(addr,attr)	((unsigned long)(addr) | (unsigned long)(attr))
#define set_mpl4t(mpl4tptr,mpl4tval)	(*(mpl4tptr) = (mpl4tval))

typedef struct {unsigned long pdpt;} pdpt_t;
#define mk_pdpt(addr,attr)	((unsigned long)(addr) | (unsigned long)(attr))
#define set_pdpt(pdptptr,pdptval)	(*(pdptptr) = (pdptval))

typedef struct {unsigned long pdt;} pdt_t;
#define mk_pdt(addr,attr)	((unsigned long)(addr) | (unsigned long)(attr))
#define set_pdt(pdtptr,pdtval)		(*(pdtptr) = (pdtval))

typedef struct {unsigned long pt;} pt_t;
#define mk_pt(addr,attr)	((unsigned long)(addr) | (unsigned long)(attr))
#define set_pt(ptptr,ptval)		(*(ptptr) = (ptval))

/*
内存管理全局结构体
*/
struct Global_Memory_Descriptor
{
	struct EFI_E820_MEMORY_DESCRIPTOR 	e820[32];
	unsigned long 	e820_length;

	unsigned long * bits_map;
	unsigned long 	bits_size;
	unsigned long   bits_length;

	struct Page *	pages_struct;
	unsigned long	pages_size;
	unsigned long 	pages_length;

	struct Zone * 	zones_struct;
	unsigned long	zones_size;
	unsigned long 	zones_length;

	unsigned long 	start_code , end_code , end_data , end_rodata , start_brk;

	unsigned long	end_of_struct;	
};

////alloc_pages zone_select

//
#define ZONE_DMA	(1 << 0)

//
#define ZONE_NORMAL	(1 << 1)

//
#define ZONE_UNMAPED	(1 << 2)

////struct page attribute

//	是否在页表映射：mapped=1 or un-mapped=0 
#define PG_PTable_Maped	(1 << 0)

//	是否为内核初始化程序：init-code=1 or normal-code/data=0
#define PG_Kernel_Init	(1 << 1)

//	设备寄存器还是物理内存空间：device=1 or memory=0
#define PG_Device	(1 << 2)

//	内核/应用层地址空间：kernel=1 or user=0
#define PG_Kernel	(1 << 3)

//	已被/未被共享：shared=1 or single-use=0 
#define PG_Shared	(1 << 4)

/*
物理页管理结构
*/
struct Page
{
	struct Zone *	zone_struct;//本页所属的区域结构体
	unsigned long	PHY_address;//物理地址
	unsigned long	attribute;//属性

	unsigned long	reference_count;//引用次数
	
	unsigned long	age;//创建时间
};

//// each zone index

int ZONE_DMA_INDEX	= 0;
int ZONE_NORMAL_INDEX	= 0;	//low 1GB RAM ,was mapped in pagetable
int ZONE_UNMAPED_INDEX	= 0;	//above 1GB RAM,unmapped in pagetable

#define MAX_NR_ZONES	10	//max zone

/*
区域管理结构
*/
struct Zone
{
	struct Page * 	pages_group;//page结构体数组指针
	unsigned long	pages_length;//本区域page数组长度
	
	unsigned long	zone_start_address;//本区域起始页对齐地址
	unsigned long	zone_end_address;//本区域结束页对齐地址
	unsigned long	zone_length;
	unsigned long	attribute;

	struct Global_Memory_Descriptor * GMD_struct;//指向全局结构体

	unsigned long	page_using_count;//已使用物理页内存数量
	unsigned long	page_free_count;//未使用物理页内存数量

	unsigned long	total_pages_link;//引用次数
};

extern struct Global_Memory_Descriptor memory_management_struct;

/*
管理每个物理页为单位的内存空间
*/
struct Slab
{
	struct List list;
	struct Page * page;

	unsigned long using_count;
	unsigned long free_count;

	void * Vaddress;//当前页面所在线性地址

	unsigned long color_length;
	unsigned long color_count;

	unsigned long * color_map;//着色位图
};
/*
内存池抽象结构
*/
struct Slab_cache
{
	unsigned long	size;
	unsigned long	total_using;
	unsigned long	total_free;
	struct Slab *	cache_pool;
	struct Slab *	cache_dma_pool;
	void *(* constructor)(void * Vaddress,unsigned long arg);
	void *(* destructor)(void * Vaddress,unsigned long arg);
};

/*
	kmalloc`s struct
*/

extern struct Slab_cache kmalloc_cache_size[16];

#define SIZEOF_LONG_ALIGN(size) ((size + sizeof(long) - 1) & ~(sizeof(long) - 1) )//按long对齐
#define SIZEOF_INT_ALIGN(size) ((size + sizeof(int) - 1) & ~(sizeof(int) - 1) )

/*

*/

#define	flush_tlb_one(addr)	\
	__asm__ __volatile__	("invlpg	(%0)	\n\t"::"r"(addr):"memory")

/** 
 * @brief 重新赋值CR3寄存器，刷新页表
 * @param 
 * @return 
 */
#define flush_tlb()						\
do								\
{								\
	unsigned long	tmpreg;					\
	__asm__ __volatile__ 	(				\
				"movq	%%cr3,	%0	\n\t"	\
				"movq	%0,	%%cr3	\n\t"	\
				:"=r"(tmpreg)			\
				:				\
				:"memory"			\
				);				\
}while(0)

/** 
 * @brief 获取CR3寄存器的页目录物理基地址
 * @param 
 * @return 
 * 		-tmp 页目录物理基地址
 */
inline unsigned long * Get_gdt()
{
	unsigned long * tmp;
	__asm__ __volatile__	(
					"movq	%%cr3,	%0	\n\t"
					:"=r"(tmp)
					:
					:"memory"
				);
	return tmp;
}

/*

*/

unsigned long page_init(struct Page * page,unsigned long flags);

unsigned long page_clean(struct Page * page);

/*

*/

unsigned long get_page_attribute(struct Page * page);

unsigned long set_page_attribute(struct Page * page,unsigned long flags);

/*

*/

void init_memory();

/*

*/

struct Page * alloc_pages(int zone_select,int number,unsigned long page_flags);

void free_pages(struct Page * page,int number);

/*
	return virtual kernel address
*/

void * kmalloc(unsigned long size,unsigned long flags);

/*

*/

struct Slab * kmalloc_create(unsigned long size);

/*

*/

unsigned long kfree(void * address);

/*

*/

struct Slab_cache * slab_create(unsigned long size,void *(* constructor)(void * Vaddress,unsigned long arg),void *(* destructor)(void * Vaddress,unsigned long arg),unsigned long arg);

/*

*/

unsigned long slab_destroy(struct Slab_cache * slab_cache);

/*

*/

void * slab_malloc(struct Slab_cache * slab_cache,unsigned long arg);

/*

*/

unsigned long slab_free(struct Slab_cache * slab_cache,void * address,unsigned long arg);

/*

*/

unsigned long slab_init();

/*

*/

void pagetable_init();

unsigned long do_brk(unsigned long addr,unsigned long len);

#endif
