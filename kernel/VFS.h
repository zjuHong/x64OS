#ifndef __VFS_H__

#define __VFS_H__
#include "lib.h"
/*
硬盘分区表项
*/
struct Disk_Partition_Table_Entry
{
	unsigned char flags;
	unsigned char start_head;
	unsigned short  start_sector	:6,	//0~5
			start_cylinder	:10;	//6~15
	unsigned char type;
	unsigned char end_head;
	unsigned short  end_sector	:6,	//0~5
			end_cylinder	:10;	//6~15
	unsigned int start_LBA;
	unsigned int sectors_limit;
}__attribute__((packed));
/*
硬盘分区表
*/
struct Disk_Partition_Table
{
	unsigned char BS_Reserved[446];
	struct Disk_Partition_Table_Entry DPTE[4];//4个分区表项
	unsigned short BS_TrailSig;
}__attribute__((packed));
/*
VFS支持的文件类型
*/
struct file_system_type
{
	char * name;
	int fs_flags;
	struct super_block * (*read_superblock)(struct Disk_Partition_Table_Entry * DPTE,void * buf);//解析引导扇区的方法
	struct file_system_type * next;//文件系统链表
};

struct super_block * mount_fs(char * name,struct Disk_Partition_Table_Entry * DPTE,void * buf);
unsigned long register_filesystem(struct file_system_type * fs);
unsigned long unregister_filesystem(struct file_system_type * fs);

struct super_block_operations;
struct index_node_operations;
struct dir_entry_operations;
struct file_operations;
/*
超级块
*/
struct super_block
{
	struct dir_entry * root;//根目录项

	struct super_block_operations * sb_ops;

	void * private_sb_info;//超级块文件特有的信息
};
/*
inode节点
*/
struct index_node
{
	unsigned long file_size;
	unsigned long blocks;
	unsigned long attribute;

	struct super_block * sb;

	struct file_operations * f_ops;
	struct index_node_operations * inode_ops;

	void * private_index_info;//index文件特有的信息
};

#define FS_ATTR_FILE	(1UL << 0)	//文件项
#define FS_ATTR_DIR		(1UL << 1)	//目录项
/*
描述文件/目录的层级关系
*/
struct dir_entry
{
	char * name;
	int name_length;
	struct List child_node;
	struct List subdirs_list;

	struct index_node * dir_inode;
	struct dir_entry * parent;

	struct dir_entry_operations * dir_ops;
};
/*
file结构
*/
struct file
{
	long position;//文件访问位置
	unsigned long mode;//文件访问模式

	struct dir_entry * dentry;

	struct file_operations * f_ops;

	void * private_data;//file文件特有的信息
};

/*上述结构的相应接口*/

struct super_block_operations
{
	void(*write_superblock)(struct super_block * sb);
	void(*put_superblock)(struct super_block * sb);

	void(*write_inode)(struct index_node * inode);
};

struct index_node_operations
{
	long (*create)(struct index_node * inode,struct dir_entry * dentry,int mode);
	struct dir_entry* (*lookup)(struct index_node * parent_inode,struct dir_entry * dest_dentry);
	long (*mkdir)(struct index_node * inode,struct dir_entry * dentry,int mode);
	long (*rmdir)(struct index_node * inode,struct dir_entry * dentry);
	long (*rename)(struct index_node * old_inode,struct dir_entry * old_dentry,struct index_node * new_inode,struct dir_entry * new_dentry);
	long (*getattr)(struct dir_entry * dentry,unsigned long * attr);
	long (*setattr)(struct dir_entry * dentry,unsigned long * attr);
};

struct dir_entry_operations
{
	long (*compare)(struct dir_entry * parent_dentry,char * source_filename,char * destination_filename);
	long (*hash)(struct dir_entry * dentry,char * filename);
	long (*release)(struct dir_entry * dentry);
	long (*iput)(struct dir_entry * dentry,struct index_node * inode);
};

struct file_operations
{
	long (*open)(struct index_node * inode,struct file * filp);
	long (*close)(struct index_node * inode,struct file * filp);
	long (*read)(struct file * filp,char * buf,unsigned long count,long * position);
	long (*write)(struct file * filp,char * buf,unsigned long count,long * position);
	long (*lseek)(struct file * filp,long offset,long origin);
	long (*ioctl)(struct index_node * inode,struct file * filp,unsigned long cmd,unsigned long arg);
};

struct dir_entry * path_walk(char * name,unsigned long flags);
extern struct super_block * root_sb;

#endif
