#include "UEFI_boot_param_info.h"
#include "memory.h"
#include "printk.h"
#include "lib.h"
#include "errno.h"

struct Global_Memory_Descriptor memory_management_struct = {{0}, 0};

struct Slab_cache kmalloc_cache_size[16] =
	{
		{32, 0, 0, NULL, NULL, NULL, NULL},
		{64, 0, 0, NULL, NULL, NULL, NULL},
		{128, 0, 0, NULL, NULL, NULL, NULL},
		{256, 0, 0, NULL, NULL, NULL, NULL},
		{512, 0, 0, NULL, NULL, NULL, NULL},
		{1024, 0, 0, NULL, NULL, NULL, NULL}, //1KB
		{2048, 0, 0, NULL, NULL, NULL, NULL},
		{4096, 0, 0, NULL, NULL, NULL, NULL}, //4KB
		{8192, 0, 0, NULL, NULL, NULL, NULL},
		{16384, 0, 0, NULL, NULL, NULL, NULL},
		{32768, 0, 0, NULL, NULL, NULL, NULL},
		{65536, 0, 0, NULL, NULL, NULL, NULL},	//64KB
		{131072, 0, 0, NULL, NULL, NULL, NULL}, //128KB
		{262144, 0, 0, NULL, NULL, NULL, NULL},
		{524288, 0, 0, NULL, NULL, NULL, NULL},
		{1048576, 0, 0, NULL, NULL, NULL, NULL}, //1MB
};

/** 
 * @brief 负责初始化目标物理页的page结构体,并更新目标物理页所在区域空间结构zone内的统计信息
 * @param 
 * @return 
 */
unsigned long page_init(struct Page *page, unsigned long flags)
{
	page->attribute |= flags;

	if (!page->reference_count || (page->attribute & PG_Shared))
	{
		page->reference_count++;
		page->zone_struct->total_pages_link++;
	}

	return 1;
}

/** 
 * @brief 清理目标物理页的page结构体,并更新目标物理页所在区域空间结构zone内的统计信息
 * @param 
 * @return 
 */
unsigned long page_clean(struct Page *page)
{
	page->reference_count--;
	page->zone_struct->total_pages_link--;

	if (!page->reference_count)
	{
		page->attribute &= PG_PTable_Maped;
	}

	return 1;
}

/** 
 * @brief 获取页属性
 * @param 
 * @return 
 */
unsigned long get_page_attribute(struct Page *page)
{
	if (page == NULL)
	{
		color_printk(RED, BLACK, "get_page_attribute() ERROR: page == NULL\n");
		return 0;
	}
	else
		return page->attribute;
}

/** 
 * @brief 设置页属性
 * @param 
 * @return 
 */
unsigned long set_page_attribute(struct Page *page, unsigned long flags)
{
	if (page == NULL)
	{
		color_printk(RED, BLACK, "set_page_attribute() ERROR: page == NULL\n");
		return 0;
	}
	else
	{
		page->attribute = flags;
		return 1;
	}
}

/** 
 * @brief 初始化内存
 * @param 
 * @return 
 */
void init_memory()
{
	int i, j;
	unsigned long TotalMem = 0;
	struct EFI_E820_MEMORY_DESCRIPTOR *p = NULL;
	unsigned long *tmp = NULL;

	color_printk(BLUE, BLACK, "Display Physics Address MAP,Type(1:RAM,2:ROM or Reserved,3:ACPI Reclaim Memory,4:ACPI NVS Memory,Others:Undefine)\n");
	p = (struct EFI_E820_MEMORY_DESCRIPTOR *)boot_para_info->E820_Info.E820_Entry; //从uefi获取内存信息

	for (i = 0; i < boot_para_info->E820_Info.E820_Entry_count; i++)
	{
		//color_printk(GREEN,BLACK,"UEFI=>KERN---Address:%#018lx\tLength:%#018lx\tType:%#010x\n",p->address,p->length,p->type);
		if (p->type == 1)
			TotalMem += p->length;
		//存到结构体中
		memory_management_struct.e820[i].address = p->address;
		memory_management_struct.e820[i].length = p->length;
		memory_management_struct.e820[i].type = p->type;
		memory_management_struct.e820_length = i;

		p++;
		//追加判断条件p->length == 0 || p->type < 1截断并剔除E829数组中的脏数据,之后根据物理地址空间划分信息计算出物理地址空间
		//的结束地址(目前的结束地址位于最后一条物理内存段信息中,但不排除其他可能性)
		if (p->type > 4 || p->length == 0 || p->type < 1)
			break;
	}

	color_printk(ORANGE, BLACK, "OS Can Used Total RAM:%#018lx\n", TotalMem);

	TotalMem = 0;

	for (i = 0; i <= memory_management_struct.e820_length; i++) //将可用物理内存段进行2M物理页边界对齐，统计总量
	{
		unsigned long start, end;
		//color_printk(ORANGE,BLACK,"KERN---Address:%#018lx\tLength:%#018lx\tType:%#010x\n",memory_management_struct.e820[i].address,memory_management_struct.e820[i].length,memory_management_struct.e820[i].type);
		if (memory_management_struct.e820[i].type != 1)
			continue;
		start = PAGE_2M_ALIGN(memory_management_struct.e820[i].address);
		end = ((memory_management_struct.e820[i].address + memory_management_struct.e820[i].length) >> PAGE_2M_SHIFT) << PAGE_2M_SHIFT;
		if (end <= start)
			continue;
		TotalMem += (end - start) >> PAGE_2M_SHIFT;
	}

	color_printk(ORANGE, BLACK, "OS Can Used Total 2M PAGEs:%#018lx=%018ld\n", TotalMem, TotalMem);

	TotalMem = memory_management_struct.e820[memory_management_struct.e820_length].address + memory_management_struct.e820[memory_management_struct.e820_length].length;

	//bits map construction init
	//成员变量bits_map是映射位图的指针,指向内核程序结束地址end_brk的4KB上边界对齐位置处,此举是为了保留一小段隔离空间以防止误操作其他空间的数据
	memory_management_struct.bits_map = (unsigned long *)((memory_management_struct.start_brk + PAGE_4K_SIZE - 1) & PAGE_4K_MASK);
	//把物理地址空间的结束地址按2MB页对齐,从而统计出物理地址空间可分页数,这个物理地址空间不仅包括可用物理内存,还包括内存空洞和ROM地址空间,将物理地址空间可分页数赋值给bits_size变量
	memory_management_struct.bits_size = TotalMem >> PAGE_2M_SHIFT;
	//物理地址空间页映射位图长度
	memory_management_struct.bits_length = (((unsigned long)(TotalMem >> PAGE_2M_SHIFT) + sizeof(long) * 8 - 1) / 8) & (~(sizeof(long) - 1));
	//接着将整个空间置位memset以标注非内存页(内存空洞和ROM)已被使用,随后再通过程序将映射位图中的可用物理页复位
	memset(memory_management_struct.bits_map, 0xff, memory_management_struct.bits_length); //init bits map memory

	//pages construction init
	//page结构体数组的存储空间位于bit映射位图之后
	memory_management_struct.pages_struct = (struct Page *)(((unsigned long)memory_management_struct.bits_map + memory_management_struct.bits_length + PAGE_4K_SIZE - 1) & PAGE_4K_MASK);

	memory_management_struct.pages_size = TotalMem >> PAGE_2M_SHIFT;
	//struct page结构体数组长度
	memory_management_struct.pages_length = ((TotalMem >> PAGE_2M_SHIFT) * sizeof(struct Page) + sizeof(long) - 1) & (~(sizeof(long) - 1));

	memset(memory_management_struct.pages_struct, 0x00, memory_management_struct.pages_length); //init pages memory

	//zones construction init
	//zones结构体数组的存储空间位于page结构体数组之后
	memory_management_struct.zones_struct = (struct Zone *)(((unsigned long)memory_management_struct.pages_struct + memory_management_struct.pages_length + PAGE_4K_SIZE - 1) & PAGE_4K_MASK);
	//目前暂时无法计算出zone结构体数组的元素个数,只能将zones_size置0,而将zones_length暂且按照5个zone结构体来计算
	memory_management_struct.zones_size = 0;

	memory_management_struct.zones_length = (5 * sizeof(struct Zone) + sizeof(long) - 1) & (~(sizeof(long) - 1));

	memset(memory_management_struct.zones_struct, 0x00, memory_management_struct.zones_length); //init zones memory

	for (i = 0; i <= memory_management_struct.e820_length; i++)
	{
		unsigned long start, end;
		struct Zone *z;
		struct Page *p;

		if (memory_management_struct.e820[i].type != 1) //不是可用物理内存空间,直接跳过,保持之前的置位状态
			continue;
		start = PAGE_2M_ALIGN(memory_management_struct.e820[i].address);
		end = ((memory_management_struct.e820[i].address + memory_management_struct.e820[i].length) >> PAGE_2M_SHIFT) << PAGE_2M_SHIFT;
		if (end <= start)
			continue;

		//zone init
		//将可用内存块对齐后放入zone结构体内
		z = memory_management_struct.zones_struct + memory_management_struct.zones_size;
		memory_management_struct.zones_size++;

		z->zone_start_address = start;
		z->zone_end_address = end;
		z->zone_length = end - start;

		z->page_using_count = 0;
		z->page_free_count = (end - start) >> PAGE_2M_SHIFT;

		z->total_pages_link = 0;

		z->attribute = 0;
		z->GMD_struct = &memory_management_struct;

		z->pages_length = (end - start) >> PAGE_2M_SHIFT;
		z->pages_group = (struct Page *)(memory_management_struct.pages_struct + (start >> PAGE_2M_SHIFT));

		//page init
		p = z->pages_group; //指向zone区域的第一个page结构体
		for (j = 0; j < z->pages_length; j++, p++)
		{
			p->zone_struct = z; //指向自身所在区域的结构体
			p->PHY_address = start + PAGE_2M_SIZE * j;
			p->attribute = 0;

			p->reference_count = 0;

			p->age = 0;
			//会把当前page结构体所代表的物理地址转换成bits_map映射位图中对应的位,由于此前已经将bits_map位图全部置位,
			//此刻再将可用物理页对应的位和1执行异或操作,以将对应的页标注为0未被使用。相当于商+余数定位在bitmap所处的位置
			*(memory_management_struct.bits_map + ((p->PHY_address >> PAGE_2M_SHIFT) >> 6)) ^= 1UL << (p->PHY_address >> PAGE_2M_SHIFT) % 64;
		}
	}

	/////////////init address 0 to page struct 0; because the memory_management_struct.e820[0].type != 1
	//全局内存信息结构体的第一个page结构体所指向的zone区域=全局内存信息结构体的第一个zone区域
	memory_management_struct.pages_struct->zone_struct = memory_management_struct.zones_struct;

	memory_management_struct.pages_struct->PHY_address = 0UL; //全局内存信息结构体的第一个page结构体的起始地址0UL

	set_page_attribute(memory_management_struct.pages_struct, PG_PTable_Maped | PG_Kernel_Init | PG_Kernel);
	memory_management_struct.pages_struct->reference_count = 1;
	memory_management_struct.pages_struct->age = 0;

	/////////////

	memory_management_struct.zones_length = (memory_management_struct.zones_size * sizeof(struct Zone) + sizeof(long) - 1) & (~(sizeof(long) - 1));

	//color_printk(ORANGE,BLACK,"bits_map:%#018lx,bits_size:%#018lx,bits_length:%#018lx\n",memory_management_struct.bits_map,memory_management_struct.bits_size,memory_management_struct.bits_length);

	//color_printk(ORANGE,BLACK,"pages_struct:%#018lx,pages_size:%#018lx,pages_length:%#018lx\n",memory_management_struct.pages_struct,memory_management_struct.pages_size,memory_management_struct.pages_length);

	//color_printk(ORANGE,BLACK,"zones_struct:%#018lx,zones_size:%#018lx,zones_length:%#018lx\n",memory_management_struct.zones_struct,memory_management_struct.zones_size,memory_management_struct.zones_length);

	ZONE_DMA_INDEX = 0;
	ZONE_NORMAL_INDEX = 0;
	ZONE_UNMAPED_INDEX = 0;
	//此段程序遍历显示各个区域空间结构体zone的详细统计信息,如果当前区域的起始地址是0x1 0000 0000,就将此区域索引值记录在全局变量ZONE_UNMAPED_INDEX内
	//表示从该区域空间开始的物理地址内存页未曾经过页表映射,最后还要调整end_of_struct的值,以记录上述结构的结束地址,并且预留一段内存空间防止越界访问
	for (i = 0; i < memory_management_struct.zones_size; i++)
	{
		struct Zone *z = memory_management_struct.zones_struct + i;
		//color_printk(ORANGE,BLACK,"zone_start_address:%#018lx,zone_end_address:%#018lx,zone_length:%#018lx,pages_group:%#018lx,pages_length:%#018lx\n",z->zone_start_address,z->zone_end_address,z->zone_length,z->pages_group,z->pages_length);

		if (z->zone_start_address >= 0x100000000 && !ZONE_UNMAPED_INDEX)
			ZONE_UNMAPED_INDEX = i;
	}

	//color_printk(ORANGE,BLACK,"ZONE_DMA_INDEX:%d\tZONE_NORMAL_INDEX:%d\tZONE_UNMAPED_INDEX:%d\n",ZONE_DMA_INDEX,ZONE_NORMAL_INDEX,ZONE_UNMAPED_INDEX);

	memory_management_struct.end_of_struct = (unsigned long)((unsigned long)memory_management_struct.zones_struct + memory_management_struct.zones_length + sizeof(long) * 32) & (~(sizeof(long) - 1)); ////need a blank to separate memory_management_struct

	//color_printk(ORANGE,BLACK,"start_code:%#018lx,end_code:%#018lx,end_data:%#018lx,start_brk:%#018lx,end_of_struct:%#018lx\n",memory_management_struct.start_code,memory_management_struct.end_code,memory_management_struct.end_data,memory_management_struct.start_brk, memory_management_struct.end_of_struct);
	//将全局内存管理结构的内核层虚拟地址转换成物理地址,按2MB页右移得出是系统第几个物理页,作为循环结束标记
	i = Virt_To_Phy(memory_management_struct.end_of_struct) >> PAGE_2M_SHIFT;

	for (j = 1; j <= i; j++) //将内核代码标志为已使用
	{
		struct Page *tmp_page = memory_management_struct.pages_struct + j;
		page_init(tmp_page, PG_PTable_Maped | PG_Kernel_Init | PG_Kernel);
		*(memory_management_struct.bits_map + ((tmp_page->PHY_address >> PAGE_2M_SHIFT) >> 6)) |= 1UL << (tmp_page->PHY_address >> PAGE_2M_SHIFT) % 64;
		tmp_page->zone_struct->page_using_count++;
		tmp_page->zone_struct->page_free_count--;
	}

	//tmp = Get_gdt();

	//color_printk(INDIGO,BLACK,"tmp\t:%#018lx\n",tmp);
	//color_printk(INDIGO,BLACK,"*tmp\t:%#018lx\n",*Phy_To_Virt(tmp) & (~0xff));
	//color_printk(PURPLE,BLACK,"**tmp\t:%#018lx\n",*Phy_To_Virt(*Phy_To_Virt(tmp) & (~0xff)) & (~0xff));

	//color_printk(ORANGE,BLACK,"1.memory_management_struct.bits_map:%#018lx\tzone_struct->page_using_count:%d\tzone_struct->page_free_count:%d\n",*memory_management_struct.bits_map,memory_management_struct.zones_struct->page_using_count,memory_management_struct.zones_struct->page_free_count);

	//	for(i = 0;i < 10;i++)
	//		*(Phy_To_Virt(Global_CR3)  + i) = 0UL;

	flush_tlb();
}

/** 
 * @brief 分配物理内存页，可用物理内存页分配函数会从已初始化的内存管理单元结构中搜索符合申请条件的page,并将其设置为使用状态
 * @param 
 *		-number: 它最多可从DMA区域空间/已映射页表区域空间或者未映射页表区域空间里一次性申请64个连续的物理页，number < 64
 *		-zone_select: zone select from dma , mapped in  pagetable , unmapped in pagetable
 *		-page_flags: struct Page flages
 * @return 
 * 		-Page结构体
 */
struct Page *alloc_pages(int zone_select, int number, unsigned long page_flags)
{
	int i;
	unsigned long page = 0;
	unsigned long attribute = 0;

	int zone_start = 0;
	int zone_end = 0;

	if (number >= 64 || number <= 0)
	{
		color_printk(RED, BLACK, "alloc_pages() ERROR: number is invalid\n");
		return NULL;
	}

	switch (zone_select)
	{
	case ZONE_DMA:
		zone_start = 0;
		zone_end = ZONE_DMA_INDEX;
		attribute = PG_PTable_Maped;
		break;

	case ZONE_NORMAL:
		zone_start = ZONE_DMA_INDEX;
		zone_end = ZONE_NORMAL_INDEX;
		attribute = PG_PTable_Maped;
		break;

	case ZONE_UNMAPED:
		zone_start = ZONE_UNMAPED_INDEX;
		zone_end = memory_management_struct.zones_size - 1;
		attribute = 0;
		break;

	default:
		color_printk(RED, BLACK, "alloc_pages() ERROR: zone_select index is invalid\n");
		return NULL;
		break;
	}
	//至此确定完检测的目标内存区域空间,接下来从该区域空间中遍历出符合申请条件的page结构体数组,这部分代码从目标内存区域空间的起始内存页结构开始逐一遍历,直至内存区域空间的结尾
	for (i = zone_start; i <= zone_end; i++)
	{
		struct Zone *z;
		unsigned long j;
		unsigned long start, end;
		unsigned long tmp;

		if ((memory_management_struct.zones_struct + i)->page_free_count < number) //如果要检索的区域的空闲页数小于要申请的页数,则到下一个区域进行搜索
			continue;

		z = memory_management_struct.zones_struct + i; //获取将要检索的zone区域
		start = z->zone_start_address >> PAGE_2M_SHIFT;
		end = z->zone_end_address >> PAGE_2M_SHIFT;
		//由于起始内存页结构对应的bit映射位图往往位于非对齐(按unsigned long类型对齐)的位置处,而且每次将按unsigned long类型作为遍历步进长度,
		//同时步进过程还会按unsigned long类型对齐，因此起始页的bit映射位图只能检索tmp = 64 - start % 64;次,
		tmp = 64 - start % 64;
		//借助代码j += j % 64 ? tmp : 64将索引变量j调整到对齐位置处,为了保证alloc_pages函数可以检索出64个连续的物理页
		for (j = start; j < end; j += j % 64 ? tmp : 64)
		{
			unsigned long *p = memory_management_struct.bits_map + (j >> 6);
			unsigned long k = 0;
			unsigned long shift = j % 64;

			unsigned long num = (1UL << number) - 1;

			for (k = shift; k < 64; k++)
			{
				if (!((k ? ((*p >> k) | (*(p + 1) << (64 - k))) : *p) & (num)))
				{ //将后一个unsigned long变量的低位部分补齐到正在检索的变量中,只有这样才能保证最多可申请64个连续物理页
					unsigned long l;
					page = j + k - shift;
					for (l = 0; l < number; l++)
					{
						struct Page *pageptr = memory_management_struct.pages_struct + page + l;

						*(memory_management_struct.bits_map + ((pageptr->PHY_address >> PAGE_2M_SHIFT) >> 6)) |= 1UL << (pageptr->PHY_address >> PAGE_2M_SHIFT) % 64;
						z->page_using_count++;
						z->page_free_count--;
						pageptr->attribute = attribute;
					}
					goto find_free_pages; //初始化完各个页属性跳转到find_free_pages
				}
			}
		}
	}

	color_printk(RED, BLACK, "alloc_pages() ERROR: no page can alloc\n");
	return NULL;

find_free_pages:

	return (struct Page *)(memory_management_struct.pages_struct + page); //返回找到的第一个物理页的内存页结构地址
}

/** 
 * @brief 释放物理内存页，并更新状态
 * @param 
 *		-page: free page start from this pointer
 *		-number: number < 64
 * @return 
 */
void free_pages(struct Page *page, int number)
{
	int i = 0;

	if (page == NULL)
	{
		color_printk(RED, BLACK, "free_pages() ERROR: page is invalid\n");
		return;
	}

	if (number >= 64 || number <= 0)
	{
		color_printk(RED, BLACK, "free_pages() ERROR: number is invalid\n");
		return;
	}

	for (i = 0; i < number; i++, page++)
	{
		*(memory_management_struct.bits_map + ((page->PHY_address >> PAGE_2M_SHIFT) >> 6)) &= ~(1UL << (page->PHY_address >> PAGE_2M_SHIFT) % 64);
		page->zone_struct->page_using_count--;
		page->zone_struct->page_free_count++;
		page->attribute = 0;
	}
}

/** 
 * @brief 内核内存分配函数
 * @param 
 *		-size: 所需内存大小 
 *		-gfp_flages: the condition of get memory 
 * @return 
 */
void *kmalloc(unsigned long size, unsigned long gfp_flages)
{
	int i, j;
	struct Slab *slab = NULL;
	if (size > 1048576)
	{
		color_printk(RED, BLACK, "kmalloc() ERROR: kmalloc size too long:%08d\n", size);
		return NULL;
	}
	for (i = 0; i < 16; i++)//找到最合适的size
		if (kmalloc_cache_size[i].size >= size)
			break;
	slab = kmalloc_cache_size[i].cache_pool;

	if (kmalloc_cache_size[i].total_free != 0)//判断是否有空闲对象
	{
		do
		{
			if (slab->free_count == 0)
				slab = container_of(list_next(&slab->list), struct Slab, list);
			else
				break;
		} while (slab != kmalloc_cache_size[i].cache_pool);
	}
	else
	{
		slab = kmalloc_create(kmalloc_cache_size[i].size);

		if (slab == NULL)
		{
			color_printk(BLUE, BLACK, "kmalloc()->kmalloc_create()=>slab == NULL\n");
			return NULL;
		}

		kmalloc_cache_size[i].total_free += slab->color_count;

		color_printk(BLUE, BLACK, "kmalloc()->kmalloc_create()<=size:%#010x\n", kmalloc_cache_size[i].size); ///////

		list_add_to_before(&kmalloc_cache_size[i].cache_pool->list, &slab->list);
	}

	for (j = 0; j < slab->color_count; j++)
	{
		if (*(slab->color_map + (j >> 6)) == 0xffffffffffffffffUL)//无空闲内存
		{
			j += 63;
			continue;
		}

		if ((*(slab->color_map + (j >> 6)) & (1UL << (j % 64))) == 0)
		{
			*(slab->color_map + (j >> 6)) |= 1UL << (j % 64);
			slab->using_count++;
			slab->free_count--;

			kmalloc_cache_size[i].total_free--;
			kmalloc_cache_size[i].total_using++;

			return (void *)((char *)slab->Vaddress + kmalloc_cache_size[i].size * j);
		}
	}

	color_printk(BLUE, BLACK, "kmalloc() ERROR: no memory can alloc\n");
	return NULL;
}

/** 
 * @brief 创建slab结构体
 * @param 
 *		-size: 所需内存大小 
 * @return 
 *		-slab 
 */
struct Slab *kmalloc_create(unsigned long size)
{
	int i;
	struct Slab *slab = NULL;
	struct Page *page = NULL;
	unsigned long *vaddresss = NULL;
	long structsize = 0;

	page = alloc_pages(ZONE_NORMAL, 1, 0);//申请空闲物理页

	if (page == NULL)
	{
		color_printk(RED, BLACK, "kmalloc_create()->alloc_pages()=>page == NULL\n");
		return NULL;
	}

	page_init(page, PG_Kernel);

	switch (size)
	{
		////////////////////slab + map in 2M page

	case 32:
	case 64:
	case 128:
	case 256:
	case 512:

		vaddresss = Phy_To_Virt(page->PHY_address);
		structsize = sizeof(struct Slab) + PAGE_2M_SIZE / size / 8;
		//slab保存在物理页末尾处
		slab = (struct Slab *)((unsigned char *)vaddresss + PAGE_2M_SIZE - structsize);
		slab->color_map = (unsigned long *)((unsigned char *)slab + sizeof(struct Slab));

		slab->free_count = (PAGE_2M_SIZE - (PAGE_2M_SIZE / size / 8) - sizeof(struct Slab)) / size;
		slab->using_count = 0;
		slab->color_count = slab->free_count;
		slab->Vaddress = vaddresss;
		slab->page = page;
		list_init(&slab->list);

		slab->color_length = ((slab->color_count + sizeof(unsigned long) * 8 - 1) >> 6) << 3;
		memset(slab->color_map, 0xff, slab->color_length);

		for (i = 0; i < slab->color_count; i++)//染色
			*(slab->color_map + (i >> 6)) ^= 1UL << i % 64;

		break;

		///////////////////kmalloc slab and map,not in 2M page anymore

	case 1024: //1KB
	case 2048:
	case 4096: //4KB
	case 8192:
	case 16384:

		//////////////////color_map is a very short buffer.

	case 32768:
	case 65536:
	case 131072: //128KB
	case 262144:
	case 524288:
	case 1048576: //1MB

		slab = (struct Slab *)kmalloc(sizeof(struct Slab), 0);

		slab->free_count = PAGE_2M_SIZE / size;
		slab->using_count = 0;
		slab->color_count = slab->free_count;

		slab->color_length = ((slab->color_count + sizeof(unsigned long) * 8 - 1) >> 6) << 3;
		//slab和内存不在同一个物理页，使用kmalloc再申请
		slab->color_map = (unsigned long *)kmalloc(slab->color_length, 0);
		memset(slab->color_map, 0xff, slab->color_length);

		slab->Vaddress = Phy_To_Virt(page->PHY_address);
		slab->page = page;
		list_init(&slab->list);

		for (i = 0; i < slab->color_count; i++)//染色
			*(slab->color_map + (i >> 6)) ^= 1UL << i % 64;

		break;

	default:

		color_printk(RED, BLACK, "kmalloc_create() ERROR: wrong size:%08d\n", size);
		free_pages(page, 1);

		return NULL;
	}

	return slab;
}

/** 
 * @brief 内存释放函数
 * @param 
 *		-address: 释放内存地址 
 * @return 
 *		-unsigned long：释放结果 
 */
unsigned long kfree(void *address)
{
	int i;
	int index;
	struct Slab *slab = NULL;
	void *page_base_address = (void *)((unsigned long)address & PAGE_2M_MASK);

	for (i = 0; i < 16; i++)
	{
		slab = kmalloc_cache_size[i].cache_pool;
		do
		{
			if (slab->Vaddress == page_base_address)//确定释放对象所在的slab结构
			{
				index = (address - slab->Vaddress) / kmalloc_cache_size[i].size;

				*(slab->color_map + (index >> 6)) ^= 1UL << index % 64;

				slab->free_count++;
				slab->using_count--;

				kmalloc_cache_size[i].total_free++;
				kmalloc_cache_size[i].total_using--;

				if ((slab->using_count == 0) && (kmalloc_cache_size[i].total_free >= slab->color_count * 3 / 2) && (kmalloc_cache_size[i].cache_pool != slab))
				{//满足条件则销毁该slab结构
					switch (kmalloc_cache_size[i].size)
					{
						////////////////////slab + map in 2M page

					case 32:
					case 64:
					case 128:
					case 256:
					case 512:
						list_del(&slab->list);
						kmalloc_cache_size[i].total_free -= slab->color_count;

						page_clean(slab->page);
						free_pages(slab->page, 1);
						break;

					default:
						list_del(&slab->list);
						kmalloc_cache_size[i].total_free -= slab->color_count;

						kfree(slab->color_map);

						page_clean(slab->page);
						free_pages(slab->page, 1);
						kfree(slab);
						break;
					}
				}

				return 1;
			}
			else
				slab = container_of(list_next(&slab->list), struct Slab, list);

		} while (slab != kmalloc_cache_size[i].cache_pool);
	}

	color_printk(RED, BLACK, "kfree() ERROR: can`t free memory\n");

	return 0;
}

/** 
 * @brief 内存池创建函数
 * @param 
 * 		-size 内存池大小
 * 		-constructor 构造函数
 * 		-destructor 析构函数
 * 		-arg 参数
 * @return 
 * 		-Slab_cache 结构
 */
struct Slab_cache *slab_create(unsigned long size, void *(*constructor)(void *Vaddress, unsigned long arg), void *(*destructor)(void *Vaddress, unsigned long arg), unsigned long arg)
{
	struct Slab_cache *slab_cache = NULL;

	slab_cache = (struct Slab_cache *)kmalloc(sizeof(struct Slab_cache), 0);

	if (slab_cache == NULL)
	{
		color_printk(RED, BLACK, "slab_create()->kmalloc()=>slab_cache == NULL\n");
		return NULL;
	}

	memset(slab_cache, 0, sizeof(struct Slab_cache));

	slab_cache->size = SIZEOF_LONG_ALIGN(size);
	slab_cache->total_using = 0;
	slab_cache->total_free = 0;
	slab_cache->cache_pool = (struct Slab *)kmalloc(sizeof(struct Slab), 0);

	if (slab_cache->cache_pool == NULL)
	{
		color_printk(RED, BLACK, "slab_create()->kmalloc()=>slab_cache->cache_pool == NULL\n");
		kfree(slab_cache);
		return NULL;
	}

	memset(slab_cache->cache_pool, 0, sizeof(struct Slab));

	slab_cache->cache_dma_pool = NULL;
	slab_cache->constructor = constructor;
	slab_cache->destructor = destructor;

	list_init(&slab_cache->cache_pool->list);

	slab_cache->cache_pool->page = alloc_pages(ZONE_NORMAL, 1, 0);//申请一页

	if (slab_cache->cache_pool->page == NULL)
	{
		color_printk(RED, BLACK, "slab_create()->alloc_pages()=>slab_cache->cache_pool->page == NULL\n");
		kfree(slab_cache->cache_pool);
		kfree(slab_cache);
		return NULL;
	}

	page_init(slab_cache->cache_pool->page, PG_Kernel);

	slab_cache->cache_pool->using_count = PAGE_2M_SIZE / slab_cache->size;

	slab_cache->cache_pool->free_count = slab_cache->cache_pool->using_count;

	slab_cache->total_free = slab_cache->cache_pool->free_count;

	slab_cache->cache_pool->Vaddress = Phy_To_Virt(slab_cache->cache_pool->page->PHY_address);//线性地址

	slab_cache->cache_pool->color_count = slab_cache->cache_pool->free_count;

	slab_cache->cache_pool->color_length = ((slab_cache->cache_pool->color_count + sizeof(unsigned long) * 8 - 1) >> 6) << 3;

	slab_cache->cache_pool->color_map = (unsigned long *)kmalloc(slab_cache->cache_pool->color_length, 0);

	if (slab_cache->cache_pool->color_map == NULL)
	{
		color_printk(RED, BLACK, "slab_create()->kmalloc()=>slab_cache->cache_pool->color_map == NULL\n");

		free_pages(slab_cache->cache_pool->page, 1);
		kfree(slab_cache->cache_pool);
		kfree(slab_cache);
		return NULL;
	}

	memset(slab_cache->cache_pool->color_map, 0, slab_cache->cache_pool->color_length);

	return slab_cache;
}

/** 
 * @brief 内存池销毁函数
 * @param 
 * 		-Slab_cache 结构
 * @return 
 */
unsigned long slab_destroy(struct Slab_cache *slab_cache)
{
	struct Slab *slab_p = slab_cache->cache_pool;
	struct Slab *tmp_slab = NULL;

	if (slab_cache->total_using != 0)
	{
		color_printk(RED, BLACK, "slab_cache->total_using != 0\n");
		return 0;
	}

	while (!list_is_empty(&slab_p->list))
	{
		tmp_slab = slab_p;
		slab_p = container_of(list_next(&slab_p->list), struct Slab, list);

		list_del(&tmp_slab->list);
		kfree(tmp_slab->color_map);

		page_clean(tmp_slab->page);
		free_pages(tmp_slab->page, 1);
		kfree(tmp_slab);
	}

	kfree(slab_p->color_map);

	page_clean(slab_p->page);
	free_pages(slab_p->page, 1);
	kfree(slab_p);

	kfree(slab_cache);

	return 1;
}

/** 
 * @brief 申请一个内存池对象，分配内存
 * @param 
 * 		-slab_cache
 * 		-arg
 * @return 
 */
void *slab_malloc(struct Slab_cache *slab_cache, unsigned long arg)
{
	struct Slab *slab_p = slab_cache->cache_pool;
	struct Slab *tmp_slab = NULL;
	int j = 0;

	if (slab_cache->total_free == 0)//池中可用内存对象为0
	{
		tmp_slab = (struct Slab *)kmalloc(sizeof(struct Slab), 0);

		if (tmp_slab == NULL)
		{
			color_printk(RED, BLACK, "slab_malloc()->kmalloc()=>tmp_slab == NULL\n");
			return NULL;
		}

		memset(tmp_slab, 0, sizeof(struct Slab));

		list_init(&tmp_slab->list);

		tmp_slab->page = alloc_pages(ZONE_NORMAL, 1, 0);

		if (tmp_slab->page == NULL)
		{
			color_printk(RED, BLACK, "slab_malloc()->alloc_pages()=>tmp_slab->page == NULL\n");
			kfree(tmp_slab);
			return NULL;
		}

		page_init(tmp_slab->page, PG_Kernel);

		tmp_slab->using_count = PAGE_2M_SIZE / slab_cache->size;
		tmp_slab->free_count = tmp_slab->using_count;
		tmp_slab->Vaddress = Phy_To_Virt(tmp_slab->page->PHY_address);

		tmp_slab->color_count = tmp_slab->free_count;
		tmp_slab->color_length = ((tmp_slab->color_count + sizeof(unsigned long) * 8 - 1) >> 6) << 3;
		tmp_slab->color_map = (unsigned long *)kmalloc(tmp_slab->color_length, 0);

		if (tmp_slab->color_map == NULL)
		{
			color_printk(RED, BLACK, "slab_malloc()->kmalloc()=>tmp_slab->color_map == NULL\n");
			free_pages(tmp_slab->page, 1);
			kfree(tmp_slab);
			return NULL;
		}

		memset(tmp_slab->color_map, 0, tmp_slab->color_length);

		list_add_to_behind(&slab_cache->cache_pool->list, &tmp_slab->list);

		slab_cache->total_free += tmp_slab->color_count;

		for (j = 0; j < tmp_slab->color_count; j++)
		{
			if ((*(tmp_slab->color_map + (j >> 6)) & (1UL << (j % 64))) == 0)//该块内存还未使用
			{
				*(tmp_slab->color_map + (j >> 6)) |= 1UL << (j % 64);

				tmp_slab->using_count++;
				tmp_slab->free_count--;

				slab_cache->total_using++;
				slab_cache->total_free--;

				if (slab_cache->constructor != NULL)
				{
					return slab_cache->constructor((char *)tmp_slab->Vaddress + slab_cache->size * j, arg);
				}
				else
				{
					return (void *)((char *)tmp_slab->Vaddress + slab_cache->size * j);
				}
			}
		}
	}
	else
	{
		do
		{
			if (slab_p->free_count == 0)//该slab没可用内存
			{
				slab_p = container_of(list_next(&slab_p->list), struct Slab, list);
				continue;
			}

			for (j = 0; j < slab_p->color_count; j++)
			{

				if (*(slab_p->color_map + (j >> 6)) == 0xffffffffffffffffUL)//此区间无空闲内存对象
				{
					j += 63;
					continue;
				}

				if ((*(slab_p->color_map + (j >> 6)) & (1UL << (j % 64))) == 0)
				{
					*(slab_p->color_map + (j >> 6)) |= 1UL << (j % 64);

					slab_p->using_count++;
					slab_p->free_count--;

					slab_cache->total_using++;
					slab_cache->total_free--;

					if (slab_cache->constructor != NULL)
					{
						return slab_cache->constructor((char *)slab_p->Vaddress + slab_cache->size * j, arg);
					}
					else
					{
						return (void *)((char *)slab_p->Vaddress + slab_cache->size * j);
					}
				}
			}
		} while (slab_p != slab_cache->cache_pool);
	}

	color_printk(RED, BLACK, "slab_malloc() ERROR: can`t alloc\n");
	if (tmp_slab != NULL)
	{
		list_del(&tmp_slab->list);
		kfree(tmp_slab->color_map);
		page_clean(tmp_slab->page);
		free_pages(tmp_slab->page, 1);
		kfree(tmp_slab);
	}

	return NULL;
}

/** 
 * @brief 释放一个slab对象
 * @param 
 * 		-slab_cache
 * 		-address 希望释放的地址
 * 		-arg
 * @return 
 */
unsigned long slab_free(struct Slab_cache *slab_cache, void *address, unsigned long arg)
{

	struct Slab *slab_p = slab_cache->cache_pool;
	int index = 0;

	do
	{
		if (slab_p->Vaddress <= address && address < slab_p->Vaddress + PAGE_2M_SIZE)//输入地址为slab_p所管理的范围
		{
			index = (address - slab_p->Vaddress) / slab_cache->size;
			*(slab_p->color_map + (index >> 6)) ^= 1UL << index % 64;//复位color_map索引
			slab_p->free_count++;
			slab_p->using_count--;

			slab_cache->total_using--;
			slab_cache->total_free++;

			if (slab_cache->destructor != NULL)
			{
				slab_cache->destructor((char *)slab_p->Vaddress + slab_cache->size * index, arg);
			}

			if ((slab_p->using_count == 0) && (slab_cache->total_free >= slab_p->color_count * 3 / 2))
			{//内存对象全部空闲且内存池空闲内存数量超过1.5倍的slab_p->color_count值，则将slab结构体释放掉
				list_del(&slab_p->list);
				slab_cache->total_free -= slab_p->color_count;

				kfree(slab_p->color_map);

				page_clean(slab_p->page);
				free_pages(slab_p->page, 1);
				kfree(slab_p);
			}

			return 1;
		}
		else
		{
			slab_p = container_of(list_next(&slab_p->list), struct Slab, list);
			continue;
		}

	} while (slab_p != slab_cache->cache_pool);

	color_printk(RED, BLACK, "slab_free() ERROR: address not in slab\n");

	return 0;
}

/** 
 * @brief slab初始化
 * @param 
 * @return 
 */
unsigned long slab_init()
{
	struct Page *page = NULL;
	unsigned long *virtual = NULL; // get a free page and set to empty page table and return the virtual address
	unsigned long i, j;

	unsigned long tmp_address = memory_management_struct.end_of_struct;

	for (i = 0; i < 16; i++)
	{
		kmalloc_cache_size[i].cache_pool = (struct Slab *)memory_management_struct.end_of_struct;
		memory_management_struct.end_of_struct = memory_management_struct.end_of_struct + sizeof(struct Slab) + sizeof(long) * 10;

		list_init(&kmalloc_cache_size[i].cache_pool->list);

		//////////// init sizeof struct Slab of cache size

		kmalloc_cache_size[i].cache_pool->using_count = 0;
		kmalloc_cache_size[i].cache_pool->free_count = PAGE_2M_SIZE / kmalloc_cache_size[i].size;

		kmalloc_cache_size[i].cache_pool->color_length = ((PAGE_2M_SIZE / kmalloc_cache_size[i].size + sizeof(unsigned long) * 8 - 1) >> 6) << 3;

		kmalloc_cache_size[i].cache_pool->color_count = kmalloc_cache_size[i].cache_pool->free_count;

		kmalloc_cache_size[i].cache_pool->color_map = (unsigned long *)memory_management_struct.end_of_struct;

		memory_management_struct.end_of_struct = (unsigned long)(memory_management_struct.end_of_struct + kmalloc_cache_size[i].cache_pool->color_length + sizeof(long) * 10) & (~(sizeof(long) - 1));

		memset(kmalloc_cache_size[i].cache_pool->color_map, 0xff, kmalloc_cache_size[i].cache_pool->color_length);

		for (j = 0; j < kmalloc_cache_size[i].cache_pool->color_count; j++)
			*(kmalloc_cache_size[i].cache_pool->color_map + (j >> 6)) ^= 1UL << j % 64;

		kmalloc_cache_size[i].total_free = kmalloc_cache_size[i].cache_pool->color_count;
		kmalloc_cache_size[i].total_using = 0;
	}

	////////////	init page for kernel code and memory management struct

	i = Virt_To_Phy(memory_management_struct.end_of_struct) >> PAGE_2M_SHIFT;

	for (j = PAGE_2M_ALIGN(Virt_To_Phy(tmp_address)) >> PAGE_2M_SHIFT; j <= i; j++)
	{//配置page结构体，标识该部分空间已经被使用
		page = memory_management_struct.pages_struct + j;
		*(memory_management_struct.bits_map + ((page->PHY_address >> PAGE_2M_SHIFT) >> 6)) |= 1UL << (page->PHY_address >> PAGE_2M_SHIFT) % 64;
		page->zone_struct->page_using_count++;
		page->zone_struct->page_free_count--;
		page_init(page, PG_PTable_Maped | PG_Kernel_Init | PG_Kernel);
	}

	//color_printk(ORANGE,BLACK,"2.memory_management_struct.bits_map:%#018lx\tzone_struct->page_using_count:%d\tzone_struct->page_free_count:%d\n",*memory_management_struct.bits_map,memory_management_struct.zones_struct->page_using_count,memory_management_struct.zones_struct->page_free_count);

	for (i = 0; i < 16; i++)
	{//初始化page
		virtual = (unsigned long *)((memory_management_struct.end_of_struct + PAGE_2M_SIZE * i + PAGE_2M_SIZE - 1) & PAGE_2M_MASK);
		page = Virt_To_2M_Page(virtual);

		*(memory_management_struct.bits_map + ((page->PHY_address >> PAGE_2M_SHIFT) >> 6)) |= 1UL << (page->PHY_address >> PAGE_2M_SHIFT) % 64;
		page->zone_struct->page_using_count++;
		page->zone_struct->page_free_count--;

		page_init(page, PG_PTable_Maped | PG_Kernel_Init | PG_Kernel);

		kmalloc_cache_size[i].cache_pool->page = page;
		kmalloc_cache_size[i].cache_pool->Vaddress = virtual;
	}

	//color_printk(ORANGE,BLACK,"3.memory_management_struct.bits_map:%#018lx\tzone_struct->page_using_count:%d\tzone_struct->page_free_count:%d\n",*memory_management_struct.bits_map,memory_management_struct.zones_struct->page_using_count,memory_management_struct.zones_struct->page_free_count);
	//color_printk(ORANGE,BLACK,"start_code:%#018lx,end_code:%#018lx,end_data:%#018lx,start_brk:%#018lx,end_of_struct:%#018lx\n",memory_management_struct.start_code,memory_management_struct.end_code,memory_management_struct.end_data,memory_management_struct.start_brk, memory_management_struct.end_of_struct);

	return 1;
}

/** 
 * @brief 页表初始化
 * @param 
 * @return 
 */
void pagetable_init()
{
	unsigned long i, j;
	unsigned long *tmp = NULL;
	//顶层页表的物理基地址，(~0xfffUL)将标注位屏蔽
	tmp = (unsigned long *)(((unsigned long)Phy_To_Virt((unsigned long)Get_gdt() & (~0xfffUL))) + 8 * 256);

	//	color_printk(YELLOW,BLACK,"1:%#018lx,%#018lx\t\t\n",(unsigned long)tmp,*tmp);
	tmp = Phy_To_Virt(*tmp & (~0xfffUL));

	//	color_printk(YELLOW,BLACK,"2:%#018lx,%#018lx\t\t\n",(unsigned long)tmp,*tmp);
	tmp = Phy_To_Virt(*tmp & (~0xfffUL));
	//	color_printk(YELLOW,BLACK,"3:%#018lx,%#018lx\t\t\n",(unsigned long)tmp,*tmp);

	for (i = 0; i < memory_management_struct.zones_size; i++)
	{
		struct Zone *z = memory_management_struct.zones_struct + i;
		struct Page *p = z->pages_group;

		if (ZONE_UNMAPED_INDEX && i == ZONE_UNMAPED_INDEX)
			break;
		//将ZONE_NORMAL_INDEX中的物理页全部映射到线性地址空间
		for (j = 0; j < z->pages_length; j++, p++)
		{
			//右移39位，索引出所在的顶层页表项
			tmp = (unsigned long *)(((unsigned long)Phy_To_Virt((unsigned long)Get_gdt() & (~0xfffUL))) + (((unsigned long)Phy_To_Virt(p->PHY_address) >> PAGE_GDT_SHIFT) & 0x1ff) * 8);

			if (*tmp == 0)//为0说明未进行次级表映射
			{
				unsigned long *virtual = kmalloc(PAGE_4K_SIZE, 0);//4k的次级页表
				memset(virtual, 0, PAGE_4K_SIZE);
				set_mpl4t(tmp, mk_mpl4t(Virt_To_Phy(virtual), PAGE_KERNEL_GDT));//将次级页表的物理地址和页表属性结合
			}

			tmp = (unsigned long *)((unsigned long)Phy_To_Virt(*tmp & (~0xfffUL)) + (((unsigned long)Phy_To_Virt(p->PHY_address) >> PAGE_1G_SHIFT) & 0x1ff) * 8);

			if (*tmp == 0)
			{
				unsigned long *virtual = kmalloc(PAGE_4K_SIZE, 0);
				memset(virtual, 0, PAGE_4K_SIZE);
				set_pdpt(tmp, mk_pdpt(Virt_To_Phy(virtual), PAGE_KERNEL_Dir));
			}

			tmp = (unsigned long *)((unsigned long)Phy_To_Virt(*tmp & (~0xfffUL)) + (((unsigned long)Phy_To_Virt(p->PHY_address) >> PAGE_2M_SHIFT) & 0x1ff) * 8);
			//将目标物理页基地址和页面属性合成页表项
			set_pdt(tmp, mk_pdt(p->PHY_address, PAGE_KERNEL_Page));

			// if (j % 50 == 0)
			// 	color_printk(GREEN, BLACK, "@:%#018lx,%#018lx\t\n", (unsigned long)tmp, *tmp);
		}
	}

	flush_tlb();
}
/** 
 * @brief 为指定地址空间分配页表项，并建立页表映射关系
 * @param 
 * @return 
 */
unsigned long do_brk(unsigned long addr, unsigned long len)
{
	unsigned long *tmp = NULL;
	unsigned long *virtual = NULL;
	struct Page *p = NULL;
	unsigned long i = 0;

	for (i = addr; i < addr + len; i += PAGE_2M_SIZE)
	{
		tmp = Phy_To_Virt((unsigned long *)((unsigned long)current->mm->pgd & (~0xfffUL)) + ((i >> PAGE_GDT_SHIFT) & 0x1ff));
		if (*tmp == NULL)
		{
			virtual = kmalloc(PAGE_4K_SIZE, 0);
			memset(virtual, 0, PAGE_4K_SIZE);
			set_mpl4t(tmp, mk_mpl4t(Virt_To_Phy(virtual), PAGE_USER_GDT));
		}

		tmp = Phy_To_Virt((unsigned long *)(*tmp & (~0xfffUL)) + ((i >> PAGE_1G_SHIFT) & 0x1ff));
		if (*tmp == NULL)
		{
			virtual = kmalloc(PAGE_4K_SIZE, 0);
			memset(virtual, 0, PAGE_4K_SIZE);
			set_pdpt(tmp, mk_pdpt(Virt_To_Phy(virtual), PAGE_USER_Dir));
		}

		tmp = Phy_To_Virt((unsigned long *)(*tmp & (~0xfffUL)) + ((i >> PAGE_2M_SHIFT) & 0x1ff));
		if (*tmp == NULL)
		{
			p = alloc_pages(ZONE_NORMAL, 1, PG_PTable_Maped);
			if (p == NULL)
				return -ENOMEM;
			set_pdt(tmp, mk_pdt(p->PHY_address, PAGE_USER_Page));
		}
	}

	current->mm->end_brk = i;

	flush_tlb();

	return i;
}
