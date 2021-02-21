#include "time.h"
#include "lib.h"

struct time time;

#define CMOS_READ(addr) ({      \
	io_out8(0x70, 0x80 | addr); \
	io_in8(0x71);               \
})

/** 
 * @brief 获取墙上时钟
 * @param 
 * @return 
 */
void get_cmos_time(struct time *time)
{
	do
	{
		time->year = CMOS_READ(0x09) + CMOS_READ(0x32) * 0x100;
		time->month = CMOS_READ(0x08);
		time->day = CMOS_READ(0x07);
		time->hour = CMOS_READ(0x04);
		time->minute = CMOS_READ(0x02);
		time->second = CMOS_READ(0x00);
	} while (time->second != CMOS_READ(0x00));

	io_out8(0x70, 0x00);
}
