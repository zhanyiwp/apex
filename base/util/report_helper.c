#include <cstddef>
#include "report_helper.h"

#define UTF8_LENGTH_1(uc)		(uc < 0x80)
#define UTF8_LENGTH_ERROR(uc)	(uc < 0xC0)
#define UTF8_LENGTH_2(uc)		(uc < 0xE0)
#define UTF8_LENGTH_3(uc)		(uc < 0xF0)
#define UTF8_LENGTH_4(uc)		(uc < 0xF5)

#define UTF8_SUCCEED_CHAR(uc)  (uc >= 0x80 && uc < 0xC0)

//	1. 总存储长度不超过limit_len
//   返回码:
//			0  正常
//			-1  名字为空
//			-2  名字里有不合法字符
//			-3  名字过长
int check_report_text(const char* text, size_t limit_len)
{
	if (!text || text[0] == '\0')
	{
		return -1;	//名字为空
	}
	int left_len = limit_len + 1; 	//with '\0'
	unsigned char *pCur = (unsigned char*)text;
	unsigned char c1st = 0;
	unsigned char c2nd = 0;
	unsigned char c3rd = 0;
	unsigned char c4th = 0;

	while (left_len > 0 && (c1st = *pCur++))
	{
		--left_len;
		if (UTF8_LENGTH_1(c1st)) {
			continue;
		}
		else if (UTF8_LENGTH_ERROR(c1st)) {
			return -2;
		}
		else if (UTF8_LENGTH_2(c1st)) {
			left_len -= 1;
			c2nd = *pCur++;
			if (!UTF8_SUCCEED_CHAR(c2nd))
				return -2;
		}
		else if (UTF8_LENGTH_3(c1st)) {
			left_len -= 2;
			c2nd = *pCur++;
			c3rd = *pCur++;
			if (!UTF8_SUCCEED_CHAR(c2nd) || !UTF8_SUCCEED_CHAR(c3rd))
				return -2;
		}
		else if (UTF8_LENGTH_4(c1st)) {
			left_len -= 3;
			c2nd = *pCur++;
			c3rd = *pCur++;
			c4th = *pCur++;
			if (!UTF8_SUCCEED_CHAR(c2nd) || !UTF8_SUCCEED_CHAR(c3rd) || !UTF8_SUCCEED_CHAR(c4th))
				return -2;
		}
		else
			return -2;	//名字里有不合法字符
	}
	if (left_len > 0)
	{
		return 0;
	}
	else
	{
		return -3;//名字过长
	}
}

struct mmap_queue* open_report_queue()
{
	return mq_create(Gaz::Report::REPORT_MMAP_FILEPATH, Gaz::Report::REPORT_MMAP_ELEMENTSIZE, Gaz::Report::REPORT_MMAP_ELEMENTCOUNT);
}