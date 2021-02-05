#ifndef	__MOUSE_H__

#define	__MOUSE_H__

extern struct keyboard_inputbuffer * p_mouse;

#define KBCMD_SENDTO_MOUSE	0xd4
#define MOUSE_ENABLE		0xf4

#define KBCMD_EN_MOUSE_INTFACE	0xa8

struct mouse_packet
{	
	unsigned char Byte0;	//7:Y overflow,6:X overflow,5:Y sign bit,4:X sign bit,3:Always,2:Middle Btn,1:Right Btn,0:Left Btn
	char Byte1;	//X movement
	char Byte2;	//Y movement
};

struct mouse_packet mouse;

/*

*/

void mouse_init();
void mouse_exit();

/*

*/

void analysis_mousecode();

#endif
