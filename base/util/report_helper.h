#ifndef REPORT_HELPER_H
#define REPORT_HELPER_H

#include <stdint.h>
#include <unistd.h>
#include "mmap_queue.h"
#include "attr_report.h"

//---------------Monitor---------------
//IPC 协议：	u8Type + u8ServerLen * sServer + u8NameLen * sName + stOther
//					u8Type	=	1,	表示	inc,	stOther = u32Value
//							=	2,	表示	set,	stOther = u8SetType + u32Value，其中u8Alarm同SetType
//							=	3,	表示	alarm，	stOther = u8AlarmLen * c8Alarm

//	1. 总存储长度不超过MAX_STR_LEN
//   返回码:
//			0  正常
//			-1  名字为空
//			-2  名字里有不合法字符
//			-3  名字过长
int check_report_text(const char* text, size_t limit_len);

struct mmap_queue* open_report_queue();

#endif
