#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "mmap_queue.h"
#include "report_helper.h"
#include "host_codec.h"
#include "attr_report.h"
#include "auto_mutex.h"

#define MAX_TEXT_LIMITLEN		(Gaz::Report::MAX_TEXT_INPUTLEN + 10) //append "_xxxxx"

static pthread_mutex_t		_mutex		=	PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
static struct mmap_queue*	_queue		=	NULL;
static uint16_t				_proc_index =	(uint16_t)-1;

//初始化，因为有可能多线程调用，所以需要加锁
static inline bool check_context()
{
	if (!_queue)
	{
		auto_mutex	mutex(&_mutex);

		//避免同时进锁后其中一个已经成功创建，后一个重复创建
		if (!_queue)
		{
			_queue = open_report_queue();
			if (!_queue)
			{
				return false;
			}
		}
	}

	return true;
}

static bool SerializeBasic(host_codec& os, const uint8_t type, const std::string& server, const std::string& name)
{
	if (check_report_text(server.c_str(), Gaz::Report::MAX_TEXT_INPUTLEN) < 0)
	{
		return false;
	}
	if (check_report_text(name.c_str(), Gaz::Report::MAX_TEXT_INPUTLEN) < 0)
	{
		return false;
	}

	char*		pszFullServerName = (char*)server.c_str();
	char		szFullServerName[MAX_TEXT_LIMITLEN] = {0};
	if (_proc_index != (uint16_t)-1)
	{
		snprintf(szFullServerName, sizeof(szFullServerName), "%s_%u", pszFullServerName, _proc_index);
		pszFullServerName = szFullServerName;
	}
	if (strlen(pszFullServerName) > MAX_TEXT_LIMITLEN)
	{
		return false;
	}

	os << type;
	if (!AppendU8Str(os, pszFullServerName))
	{
		return false;
	}
	if (!AppendU8Str(os, name.c_str()))
	{
		return false;
	}

	return true;
}

//注册进程号，可选
void Gaz::Report::set_proc_order_index(uint16_t index/* = -1*/)
{
	auto_mutex	mutex(&_mutex);

	_proc_index = index;
}

//IPC 协议：	u8Type + u8ServerLen * sServer + u8NameLen * sName + stOther
//					u8Type	=	1,	表示	inc,	stOther = u32Value
//							=	2,	表示	set,	stOther = u8SetType + u32Value，其中u8Alarm同SetType
//							=	3,	表示	alarm，	stOther = u8AlarmLen * c8Alarm

// 累加上报
int Gaz::Report::inc(const std::string& server, const std::string& name, uint32_t value)
{
	// 挂载mmap
	if (!check_context())
	{
		return -1;
	}
	
	uint8_t	buf[sizeof(uint8_t)* 3 + MAX_TEXT_INPUTLEN + MAX_TEXT_LIMITLEN + sizeof(uint32_t)];
	host_codec os(sizeof(buf), buf);
	if (SerializeBasic(os, 1, server, name) < 0)
	{
		return -2;
	}		

	os << value;
	if (!os)
	{
		return -2;
	}

	return mq_put(_queue, (void*)os.data(), os.wpos());
}

int Gaz::Report::inc_once(const std::string& server, const std::string& name)
{
	return inc(server, name, 1);
}

// 覆盖上报
int Gaz::Report::set(const std::string& server, const std::string& name, uint32_t value, SetType type/* = ST_LAST*/)
{
	// 挂载mmap
	if (!check_context())
	{
		return -1;
	}

	uint8_t	buf[sizeof(uint8_t)* 4 + MAX_TEXT_INPUTLEN + 0 + sizeof(uint32_t)];
	host_codec os(sizeof(buf), buf);
	if (SerializeBasic(os, 2, server, name) < 0)
	{
		return -2;
	}

	os << static_cast<uint8_t>(type) << value;
	if (!os)
	{
		return -2;
	}

	return mq_put(_queue, (void*)os.data(), os.wpos());
}

// 上报字符串
int Gaz::Report::alarm(const std::string& server, const std::string& name, const std::string& text)
{
	// 挂载mmap
	if (!check_context())
	{
		return -1;
	}
	if (check_report_text(text.c_str(), MAX_TEXT_INPUTLEN) < 0)
	{
		return -2;
	}

	uint8_t	buf[sizeof(uint8_t)* 4 + MAX_TEXT_INPUTLEN + MAX_TEXT_LIMITLEN];
	host_codec os(sizeof(buf), buf);
	if (SerializeBasic(os, 3, server, name) < 0)
	{
		return -2;
	}
	if (!AppendU8Str(os, text.c_str()))
	{
		return -2;
	}

	return mq_put(_queue, (void*)os.data(), os.wpos());
}

