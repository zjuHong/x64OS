#include "disk.h"
#include "APIC.h"
#include "memory.h"
#include "printk.h"
#include "lib.h"
#include "block.h"
#include "semaphore.h"

static int disk_flags = 0;

struct request_queue disk_request;

struct block_device_operation IDE_device_operation;
/** 
 * @brief 回收操作请求项，如果还有请求项，就继续调用cmd_out
 * @param 
 * @return 
 */
void end_request(struct block_buffer_node * node)
{
	if(node == NULL)
		color_printk(RED,BLACK,"end_request error\n");

	node->wait_queue.tsk->state = TASK_RUNNING;
	insert_task_queue(node->wait_queue.tsk);
	//node->wait_queue.tsk->flags |= NEED_SCHEDULE;
	current->flags |= NEED_SCHEDULE;

	kfree((unsigned long *)disk_request.in_using);
	disk_request.in_using = NULL;

	if(disk_request.block_request_count)
		cmd_out();
}
/** 
 * @brief 将请求项加入硬盘请求队列
 * @param 
 * @return 
 */
void add_request(struct block_buffer_node * node)
{
	list_add_to_before(&disk_request.wait_queue_list.wait_list,&node->wait_queue.wait_list);
	disk_request.block_request_count++;
}
/** 
 * @brief 硬盘空闲时，从请求队列中取出一个请求项，执行请求
 * @param 
 * @return 
 */
long cmd_out()
{
	//取出一个请求项
	wait_queue_T *wait_queue_tmp = container_of(list_next(&disk_request.wait_queue_list.wait_list),wait_queue_T,wait_list);
	struct block_buffer_node * node = disk_request.in_using = container_of(wait_queue_tmp,struct block_buffer_node,wait_queue);
	list_del(&disk_request.in_using->wait_queue.wait_list);
	disk_request.block_request_count--;

	while(io_in8(PORT_DISK0_STATUS_CMD) & DISK_STATUS_BUSY)//查询忙状态
		nop();

	switch(node->cmd)
	{
		case ATA_WRITE_CMD:	//写磁盘

			io_out8(PORT_DISK0_DEVICE,0x40);
			//48位LBA寻址，需要分两次
			io_out8(PORT_DISK0_ERR_FEATURE,0);
			io_out8(PORT_DISK0_SECTOR_CNT,(node->count >> 8) & 0xff);
			io_out8(PORT_DISK0_SECTOR_LOW ,(node->LBA >> 24) & 0xff);
			io_out8(PORT_DISK0_SECTOR_MID ,(node->LBA >> 32) & 0xff);
			io_out8(PORT_DISK0_SECTOR_HIGH,(node->LBA >> 40) & 0xff);

			io_out8(PORT_DISK0_ERR_FEATURE,0);
			io_out8(PORT_DISK0_SECTOR_CNT,node->count & 0xff);
			io_out8(PORT_DISK0_SECTOR_LOW,node->LBA & 0xff);
			io_out8(PORT_DISK0_SECTOR_MID,(node->LBA >> 8) & 0xff);
			io_out8(PORT_DISK0_SECTOR_HIGH,(node->LBA >> 16) & 0xff);

			while(!(io_in8(PORT_DISK0_STATUS_CMD) & DISK_STATUS_READY))
				nop();
			io_out8(PORT_DISK0_STATUS_CMD,node->cmd);

			while(!(io_in8(PORT_DISK0_STATUS_CMD) & DISK_STATUS_REQ))
				nop();
			port_outsw(PORT_DISK0_DATA,node->buffer,256);
			break;

		case ATA_READ_CMD://读磁盘

			io_out8(PORT_DISK0_DEVICE,0x40);

			io_out8(PORT_DISK0_ERR_FEATURE,0);
			io_out8(PORT_DISK0_SECTOR_CNT,(node->count >> 8) & 0xff);
			io_out8(PORT_DISK0_SECTOR_LOW ,(node->LBA >> 24) & 0xff);
			io_out8(PORT_DISK0_SECTOR_MID ,(node->LBA >> 32) & 0xff);
			io_out8(PORT_DISK0_SECTOR_HIGH,(node->LBA >> 40) & 0xff);

			io_out8(PORT_DISK0_ERR_FEATURE,0);
			io_out8(PORT_DISK0_SECTOR_CNT,node->count & 0xff);
			io_out8(PORT_DISK0_SECTOR_LOW,node->LBA & 0xff);
			io_out8(PORT_DISK0_SECTOR_MID,(node->LBA >> 8) & 0xff);
			io_out8(PORT_DISK0_SECTOR_HIGH,(node->LBA >> 16) & 0xff);

			while(!(io_in8(PORT_DISK0_STATUS_CMD) & DISK_STATUS_READY))
				nop();
			io_out8(PORT_DISK0_STATUS_CMD,node->cmd);
			break;
			
		case GET_IDENTIFY_DISK_CMD:	//获取信息

			io_out8(PORT_DISK0_DEVICE,0xe0);
			
			io_out8(PORT_DISK0_ERR_FEATURE,0);
			io_out8(PORT_DISK0_SECTOR_CNT,node->count & 0xff);
			io_out8(PORT_DISK0_SECTOR_LOW,node->LBA & 0xff);
			io_out8(PORT_DISK0_SECTOR_MID,(node->LBA >> 8) & 0xff);
			io_out8(PORT_DISK0_SECTOR_HIGH,(node->LBA >> 16) & 0xff);

			while(!(io_in8(PORT_DISK0_STATUS_CMD) & DISK_STATUS_READY))
				nop();			
			io_out8(PORT_DISK0_STATUS_CMD,node->cmd);

		default:
			color_printk(BLACK,WHITE,"ATA CMD Error\n");
			break;
	}
	return 1;
}

/** 
 * @brief 三种回调函数
 * @param 
 * 		-nr：向量号
 * 		-parameter：等待队列结构体地址
 * @return 
 */
void read_handler(unsigned long nr, unsigned long parameter)
{
	struct block_buffer_node * node = ((struct request_queue *)parameter)->in_using;
	
	if(io_in8(PORT_DISK0_STATUS_CMD) & DISK_STATUS_ERROR)//查询是否出错
		color_printk(RED,BLACK,"read_handler:%#010x\n",io_in8(PORT_DISK0_ERR_FEATURE));
	else
		port_insw(PORT_DISK0_DATA,node->buffer,256);

	node->count--;//操作多个连续的磁盘扇区
	if(node->count)
	{
		node->buffer += 512;
		return;
	}

	end_request(node);
}
void write_handler(unsigned long nr, unsigned long parameter)
{
	struct block_buffer_node * node = ((struct request_queue *)parameter)->in_using;

	if(io_in8(PORT_DISK0_STATUS_CMD) & DISK_STATUS_ERROR)
		color_printk(RED,BLACK,"write_handler:%#010x\n",io_in8(PORT_DISK0_ERR_FEATURE));

	node->count--;
	if(node->count)
	{
		node->buffer += 512;
		while(!(io_in8(PORT_DISK0_STATUS_CMD) & DISK_STATUS_REQ))
			nop();
		port_outsw(PORT_DISK0_DATA,node->buffer,256);
		return;
	}

	end_request(node);
}
void other_handler(unsigned long nr, unsigned long parameter)
{
	struct block_buffer_node * node = ((struct request_queue *)parameter)->in_using;

	if(io_in8(PORT_DISK0_STATUS_CMD) & DISK_STATUS_ERROR)
		color_printk(RED,BLACK,"other_handler:%#010x\n",io_in8(PORT_DISK0_ERR_FEATURE));
	else
		port_insw(PORT_DISK0_DATA,node->buffer,256);

	end_request(node);
}
/** 
 * @brief 创建硬盘操作请求项目：node结构
 * @param 
 * @return 
 */
struct block_buffer_node * make_request(long cmd,unsigned long blocks,long count,unsigned char * buffer)
{
	struct block_buffer_node * node = (struct block_buffer_node *)kmalloc(sizeof(struct block_buffer_node),0);
	wait_queue_init(&node->wait_queue,current);

	switch(cmd)
	{
		case ATA_READ_CMD:
			node->end_handler = read_handler;
			node->cmd = ATA_READ_CMD;
			break;

		case ATA_WRITE_CMD:
			node->end_handler = write_handler;
			node->cmd = ATA_WRITE_CMD;
			break;

		default:///
			node->end_handler = other_handler;
			node->cmd = cmd;
			break;
	}
	
	node->LBA = blocks;
	node->count = count;
	node->buffer = buffer;

	return node;
}
/** 
 * @brief 将硬盘操作请求项目的node结构加入请求队列中
 * @param 
 * 		-node
 * @return 
 */
void submit(struct block_buffer_node * node)
{	
	add_request(node);
	
	if(disk_request.in_using == NULL)
		cmd_out();
}
/** 
 * @brief 等待硬盘中断
 * @param 
 * @return 
 */
void wait_for_finish()
{
	current->state = TASK_UNINTERRUPTIBLE;
	schedule();//阻塞后调度
}

/** 
 * @brief 硬盘的接口函数实现
 * @param 
 * @return 
 */
long IDE_open()
{
	color_printk(BLACK,WHITE,"DISK0 Opened\n");
	return 1;
}

long IDE_close()
{
	color_printk(BLACK,WHITE,"DISK0 Closed\n");
	return 1;
}

long IDE_ioctl(long cmd,long arg)
{
	struct block_buffer_node * node = NULL;
	
	if(cmd == GET_IDENTIFY_DISK_CMD)
	{
		node = make_request(cmd,0,0,(unsigned char *)arg);
		submit(node);
		wait_for_finish();
		return 1;
	}
	
	return 0;
}
/** 
 * @brief 硬盘访问操作
 * @param 
 * @return 
 */
long IDE_transfer(long cmd,unsigned long blocks,long count,unsigned char * buffer)
{
	struct block_buffer_node * node = NULL;
	if(cmd == ATA_READ_CMD || cmd == ATA_WRITE_CMD)//只处理读写命令
	{
		node = make_request(cmd,blocks,count,buffer);
		submit(node);
		wait_for_finish();
	}
	else
	{
		return 0;
	}
	
	return 1;
}
//硬盘操作接口
struct block_device_operation IDE_device_operation = 
{
	.open = IDE_open,
	.close = IDE_close,
	.ioctl = IDE_ioctl,
	.transfer = IDE_transfer,
};

//硬盘中断控制器
hw_int_controller disk_int_controller = 
{
	.enable = IOAPIC_enable,
	.disable = IOAPIC_disable,
	.install = IOAPIC_install,
	.uninstall = IOAPIC_uninstall,
	.ack = IOAPIC_edge_ack,
};

/** 
 * @brief 硬盘中断函数
 * @param 
 * 		-nr
 * 		-parameter
 * 		-regs
 * @return 
 */
void disk_handler(unsigned long nr, unsigned long parameter, struct pt_regs * regs)
{
	struct block_buffer_node * node = ((struct request_queue *)parameter)->in_using;
	color_printk(BLACK,WHITE,"disk_handler\t");
	node->end_handler(nr,parameter);
}
/** 
 * @brief 硬盘初始化
 * @param 
 * @return 
 */
void disk_init()
{
	struct IO_APIC_RET_entry entry;

	entry.vector = 0x2e;
	entry.deliver_mode = APIC_ICR_IOAPIC_Fixed ;
	entry.dest_mode = ICR_IOAPIC_DELV_PHYSICAL;
	entry.deliver_status = APIC_ICR_IOAPIC_Idle;
	entry.polarity = APIC_IOAPIC_POLARITY_HIGH;
	entry.irr = APIC_IOAPIC_IRR_RESET;
	entry.trigger = APIC_ICR_IOAPIC_Edge;
	entry.mask = APIC_ICR_IOAPIC_Masked;
	entry.reserved = 0;

	entry.destination.physical.reserved1 = 0;
	entry.destination.physical.phy_dest = 0;
	entry.destination.physical.reserved2 = 0;
	//向量号2e，第二块硬盘为2f
	register_irq(0x2e, &entry , &disk_handler, (unsigned long)&disk_request, &disk_int_controller, "DISK0");

	io_out8(PORT_DISK0_ALT_STA_CTL,0);
	
	wait_queue_init(&disk_request.wait_queue_list,NULL);//硬盘就绪队列初始化
	disk_request.in_using = NULL;
	disk_request.block_request_count = 0;
}

void disk_exit()
{
	unregister_irq(0x2e);
}

