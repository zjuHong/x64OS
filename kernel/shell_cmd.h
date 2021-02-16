#ifndef __SHELL_CMD_H__
#define __SHELL_CMD_H__
#include "lib.h"

char *current_dir = NULL;

int cd_command(int argc,char **argv){}
int ls_command(int argc,char **argv){}
int pwd_command(int argc,char **argv)
{
	color_printk(WHITE,BLACK,"pwd execute\n");
	if(current_dir)
		color_printk(WHITE,BLACK,"%s\n",current_dir);
	return 1;
}
int cat_command(int argc,char **argv){}
int touch_command(int argc,char **argv){}
int rm_command(int argc,char **argv){}
int mkdir_command(int argc,char **argv){}
int rmdir_command(int argc,char **argv){}
int exec_command(int argc,char **argv){}
int reboot_command(int argc,char **argv)
{
	color_printk(WHITE,BLACK,"reboot execute\n");
	io_out8(0x64, 0xFE);
}

#endif
