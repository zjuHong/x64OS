#include "keyboard.h"
#include "lib.h"

#include "APIC.h"
#include "memory.h"
#include "printk.h"
#include "shell_cmd.h"

// shell命令数组
struct buildincmd shell_internal_cmd[] =
{
		{"cd", cd_command},
		{"ls", ls_command},
		{"pwd", pwd_command},
		{"cat", cat_command},
		{"touch", touch_command},
		{"rm", rm_command},
		{"mkdir", mkdir_command},
		{"rmdir", rmdir_command},
		{"exec", exec_command},
		{"reboot", reboot_command},
};

/*

*/

struct keyboard_inputbuffer *p_kb = NULL;
static int shift_l, shift_r, ctrl_l, ctrl_r, alt_l, alt_r;

/** 
 * @brief 键盘中断处理函数
 * @param 
 * @return 
 */
void keyboard_handler(unsigned long nr, unsigned long parameter, struct pt_regs *regs)
{
	unsigned char x;
	x = io_in8(0x60);//从IO端口0x60读取扫描码
	// color_printk(WHITE,BLACK,"(K:%02x)",x);

	if (p_kb->p_head == p_kb->buf + KB_BUF_SIZE)
		p_kb->p_head = p_kb->buf;

	*p_kb->p_head = x;//存入缓冲区
	p_kb->count++;
	p_kb->p_head++;
}


/** 
 * @brief 键盘缓冲区处理
 * @param 
 *
 * @return 
 *     缓冲区起始地址
 */
unsigned char get_scancode()
{
	unsigned char ret = 0;

	if (p_kb->count == 0)//键盘缓冲区为空
		while (!p_kb->count)
			nop();

	if (p_kb->p_tail == p_kb->buf + KB_BUF_SIZE)//取出1B数据返回给调用者
		p_kb->p_tail = p_kb->buf;

	ret = *p_kb->p_tail;
	p_kb->count--;
	p_kb->p_tail++;

	return ret;
}

/** 
 * @brief 解析键盘码
 * @param 
 *
 * @return 
 *     -key 键值
 */
int analysis_keycode()
{
	unsigned char x = 0;
	int i;
	int key = 0;
	int make = 0;

	x = get_scancode();

	if (x == 0xE1) //pause break;
	{
		key = PAUSEBREAK;
		for (i = 1; i < 6; i++)
			if (get_scancode() != pausebreak_scode[i])
			{
				key = 0;
				break;
			}
	}
	else if (x == 0xE0) //print screen
	{
		x = get_scancode();

		switch (x)
		{
		case 0x2A: // press printscreen

			if (get_scancode() == 0xE0)
				if (get_scancode() == 0x37)
				{
					key = PRINTSCREEN;
					make = 1;
				}
			break;

		case 0xB7: // UNpress printscreen

			if (get_scancode() == 0xE0)
				if (get_scancode() == 0xAA)
				{
					key = PRINTSCREEN;
					make = 0;
				}
			break;

		case 0x1d: // press right ctrl

			ctrl_r = 1;
			key = OTHERKEY;
			break;

		case 0x9d: // UNpress right ctrl

			ctrl_r = 0;
			key = OTHERKEY;
			break;

		case 0x38: // press right alt

			alt_r = 1;
			key = OTHERKEY;
			break;

		case 0xb8: // UNpress right alt

			alt_r = 0;
			key = OTHERKEY;
			break;

		default:
			key = OTHERKEY;
			break;
		}
	}

	if (key == 0)
	{
		unsigned int *keyrow = NULL;
		int column = 0;

		make = (x & FLAG_BREAK ? 0 : 1);

		keyrow = &keycode_map_normal[(x & 0x7F) * MAP_COLS];

		if (shift_l || shift_r)
			column = 1;

		key = keyrow[column];

		switch (x & 0x7F)
		{
		case 0x2a: //SHIFT_L:
			shift_l = make;
			key = 0;
			break;

		case 0x36: //SHIFT_R:
			shift_r = make;
			key = 0;
			break;

		case 0x1d: //CTRL_L:
			ctrl_l = make;
			key = 0;
			break;

		case 0x38: //ALT_L:
			alt_l = make;
			key = 0;
			break;

		case 0x1c: //ENTER:
			key = '\n';
			break;

		default:
			if (!make)
				key = 0;
			break;
		}

		if (key)
			return key;
		//color_printk(RED,YELLOW,"(K:%c)\t",key);
	}
	return 0;
}

//键盘中断控制器
hw_int_controller keyboard_int_controller =
{
		.enable = IOAPIC_enable,
		.disable = IOAPIC_disable,
		.install = IOAPIC_install,
		.uninstall = IOAPIC_uninstall,
		.ack = IOAPIC_edge_ack,
};

/** 
 * @brief 键盘初始化程序
 * @param 
 * @return 
 */
void keyboard_init()
{
	struct IO_APIC_RET_entry entry;
	unsigned long i, j;

	p_kb = (struct keyboard_inputbuffer *)kmalloc(sizeof(struct keyboard_inputbuffer), 0);

	p_kb->p_head = p_kb->buf;
	p_kb->p_tail = p_kb->buf;
	p_kb->count = 0;
	memset(p_kb->buf, 0, KB_BUF_SIZE);

	entry.vector = 0x21;
	entry.deliver_mode = APIC_ICR_IOAPIC_Fixed;
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

	wait_KB_write();
	io_out8(PORT_KB_CMD, KBCMD_WRITE_CMD);//发送配置命令
	wait_KB_write();
	io_out8(PORT_KB_DATA, KB_INIT_MODE);//具体模式设置

	for (i = 0; i < 1000; i++)
		for (j = 0; j < 1000; j++)
			nop();

	shift_l = 0;
	shift_r = 0;
	ctrl_l = 0;
	ctrl_r = 0;
	alt_l = 0;
	alt_r = 0;
	//中断向量号0x21
	register_irq(0x21, &entry, &keyboard_handler, (unsigned long)p_kb, &keyboard_int_controller, "ps/2 keyboard");
}

/** 
 * @brief 键盘卸载程序
 * @param 
 * @return 
 */
void keyboard_exit()
{
	unregister_irq(0x21);
	kfree((unsigned long *)p_kb);
}

/** 
 * @brief 获取键盘的一行内容放入缓冲区
 * @param fd    文件描述符
 * @param buf   缓冲区
 *
 * @return 
 *     -count 内容长度
 */
int read_line(int fd, char *buf)
{
	int key = 0;
	int count = 0;

	while (1)
	{
		key = analysis_keycode();
		if (key == '\n')
		{
			return count;
		}
		else if (key)
		{
			buf[count++] = key;
			color_printk(WHITE, BLACK, "%c", key);
		}
		else
			continue;
	}
}

/** 
 * @brief 根据命令字符查找命令函数
 * @param cmd   命令字符
 *
 * @return 
 *     -1 查找失败
 * 		1 查找成功
 */
int find_cmd(char *cmd)
{
	int i = 0;
	for (i = 0; i < sizeof(shell_internal_cmd) / sizeof(struct buildincmd); i++)
		if (!strcmp(cmd, shell_internal_cmd[i].name))
			return i;
	return -1;
}

/** 
 * @brief 运行命令
 * @param index   命令索引
 * @param argc   
 * @param argv   
 * @return 
 */
void run_command(int index, int argc, char **argv)
{
	color_printk(WHITE, BLACK, "run_command %s\n", shell_internal_cmd[index].name);
	shell_internal_cmd[index].function(argc, argv);
}

/** 
 * @brief 将键盘缓冲区的数据进行解析
 * @param buf   键盘缓冲区
 * @param argc   
 * @param argv   
 * @return 
 */
int parse_command(char *buf, int *argc, char ***argv)
{
	int i = 0;
	int j = 0;

	while (buf[j] == ' ')
		j++;

	for (i = j; i < 256; i++)//计算参数个数
	{
		if (!buf[i])
			break;
		if (buf[i] != ' ' && (buf[i + 1] == ' ' || buf[i + 1] == '\0'))
			(*argc)++;
	}

	color_printk(WHITE, BLACK, "parse_command argc:%d\n", *argc);

	if (!*argc)
		return -1;

	*argv = (char **)kmalloc(sizeof(char **) * (*argc), 0);

	color_printk(WHITE, BLACK, "parse_command argv:%#018lx,*argv:%#018lx\n", argv, *argv);

	for (i = 0; i < *argc && j < 256; i++)//拆分命令各个参数
	{
		*((*argv) + i) = &buf[j];

		while (buf[j] && buf[j] != ' ')
			j++;
		buf[j++] = '\0';
		while (buf[j] == ' ')
			j++;
		color_printk(WHITE, BLACK, "%s\n", (*argv)[i]);
	}

	return find_cmd(**argv);
}
