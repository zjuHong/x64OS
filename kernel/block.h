#ifndef __BLOCK_H__

#define __BLOCK_H__


/*
块设备操作接口
*/
struct block_device_operation
{
	long (* open)();
	long (* close)();
	long (* ioctl)(long cmd,long arg);
	long (* transfer)(long cmd,unsigned long blocks,long count,unsigned char * buffer);
};

#endif

