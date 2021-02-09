#include "VFS.h"
#include "lib.h"


struct file_system_type filesystem = {"filesystem",0};

//function mount_root

struct super_block* mount_fs(char * name,struct Disk_Partition_Table_Entry * DPTE,void * buf)
{
	struct file_system_type * p = NULL;

	for(p = &filesystem;p;p = p->next)
		if(!strcmp(p->name,name))
		{
			return p->read_superblock(DPTE,buf);
		}
	return 0;
}

unsigned long register_filesystem(struct file_system_type * fs)
{
	struct file_system_type * p = NULL;

	for(p = &filesystem;p;p = p->next)
		if(!strcmp(fs->name,p->name))
			return 0;

	fs->next = filesystem.next;
	filesystem.next = fs;

	return 1;
}

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


