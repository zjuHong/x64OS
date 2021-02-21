#include "VFS.h"
#include "lib.h"
#include "printk.h"
/** 
 * @brief 搜索指定目录下的文件
 * @param 
 * @return 
 */
struct dir_entry * path_walk(char * name,unsigned long flags)
{
	char * tmpname = NULL;
	int tmpnamelen = 0;
	struct dir_entry * parent = root_sb->root;
	struct dir_entry * path = NULL;

	while(*name == '/')//滤除'/'
		name++;

	if(!*name)
	{
		return parent;
	}

	for(;;)
	{
		tmpname = name;
		while(*name && (*name != '/'))//分割当前目录
			name++;
		tmpnamelen = name - tmpname;

		path = (struct dir_entry *)kmalloc(sizeof(struct dir_entry),0);
		memset(path,0,sizeof(struct dir_entry));

		path->name = kmalloc(tmpnamelen+1,0);
		memset(path->name,0,tmpnamelen+1);
		memcpy(tmpname,path->name,tmpnamelen);
		path->name_length = tmpnamelen;

		if(parent->dir_inode->inode_ops->lookup(parent->dir_inode,path) == NULL)//lookup搜索匹配项
		{
			color_printk(RED,WHITE,"can not find file or dir:%s\n",path->name);
			kfree(path->name);
			kfree(path);
			return NULL;
		}

		list_init(&path->child_node);
		list_init(&path->subdirs_list);
		path->parent = parent;
		list_add_to_behind(&parent->subdirs_list,&path->child_node);

		if(!*name)//搜索至结尾
			goto last_component;
		while(*name == '/')
			name++;
		if(!*name)
			goto last_slash;

		parent = path;
	}

last_slash:
last_component:

	if(flags & 1)//根据flag决定返回目标目录还是其父目录
	{
		return parent;
	}

	return path;
}

//function mount_root
struct super_block * root_sb = NULL;
struct file_system_type filesystem = {"filesystem",0};
/** 
 * @brief 文件系统挂载
 * @param 
 * @return 
 */
struct super_block* mount_fs(char * name,struct Disk_Partition_Table_Entry * DPTE,void * buf)
{
	struct file_system_type * p = NULL;

	for(p = &filesystem;p;p = p->next)
		if(!strcmp(p->name,name))
		{
			return p->read_superblock(DPTE,buf);//读取引导扇区
		}
	return 0;
}
/** 
 * @brief 文件系统注册
 * @param 
 * @return 
 */
unsigned long register_filesystem(struct file_system_type * fs)
{
	struct file_system_type * p = NULL;

	for(p = &filesystem;p;p = p->next)
		if(!strcmp(fs->name,p->name))
			return 0;
	//注册到链表上
	fs->next = filesystem.next;
	filesystem.next = fs;

	return 1;
}
/** 
 * @brief 文件系统注销
 * @param 
 * @return 
 */
unsigned long unregister_filesystem(struct file_system_type * fs)
{
	struct file_system_type * p = &filesystem;

	while(p->next)
		if(p->next == fs)
		{
			p->next = p->next->next;
			fs->next = NULL;
			return 1;
		}
		else
			p = p->next;
	return 0;
}



