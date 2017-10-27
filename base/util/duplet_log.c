#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/stat.h>

#include "mmap_queue.h"
#include "log_helper.h"
#include "host_codec.h"
#include "string_func.h"
#include "duplet_log.h"

#include "auto_mutex.h"
#include "time_func.h"

#ifndef MAX_PATH
#define MAX_PATH (260)
#endif

enum LOG_TYPE
{
	LT_NONE = 0,
	LT_LOCAL = 1,
	LT_REMOTE = 2,
	LT_DUPLET = 3,
};

static pthread_mutex_t		_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

static uint16_t				_proc_index = -1;

static char					_file_name[MAX_PATH] = { 0 };
static char					_dir_name[MAX_PATH] = { 0 };
static uint16_t				_file_path_Len = 0;
static char					_file_path[MAX_PATH] = { 0 };// _file_path = _dir_name + _file_name

static enum Gaz::Log::LOG_LEVEL	_local_lowest_level = Gaz::Log::LL_WARN;
static enum Gaz::Log::LOG_LEVEL	_remote_lowest_level = Gaz::Log::LL_NONE;

static struct mmap_queue*	_queue = NULL;

static	bool				_date_category = true;
static	uint32_t			_limit_length_per_file = Gaz::Log::DEFAULT_LIMITLENGTH_PERFILE;
static  uint8_t				_file_cycle_num = Gaz::Log::DEFAULT_FILE_NUM;

static bool make_full_path()
{
	if (!strlen(_dir_name))
	{
		char process_path[MAX_PATH] = { 0 };
		int cnt = readlink("/proc/self/exe", process_path, MAX_PATH);
		if (cnt < 0 || cnt >= MAX_PATH)
		{
			return false;
		}
		process_path[cnt] = 0;

		char* pszProcessName = strrchr(process_path, '/');
		if (!pszProcessName)
		{
			return false;
		}
		*pszProcessName = 0;
				
		strlcpy(_dir_name, process_path, sizeof(_dir_name));
	}

	strlcpy(_file_path, _dir_name, sizeof(_file_path));
	size_t len = strlen(_file_path);
	if (len > 0)
	{
		if (len + 1 >= sizeof(_file_path))
		{
			return false;
		}
		if (_file_path[len - 1] != '/')
		{
			strlcat(_file_path, "/", sizeof(_file_path));		
			len++;
		}

		if (len + 1 >= sizeof(_file_path))
		{
			return false;
		}
	}

	char* pszProcessName = _file_name;
	if (!pszProcessName || !strlen(pszProcessName))
	{
		char process_path[MAX_PATH] = { 0 };
		int cnt = readlink("/proc/self/exe", process_path, MAX_PATH);
		if (cnt < 0 || cnt >= MAX_PATH)
		{
			return false;
		}
		process_path[cnt] = 0;

		pszProcessName = strrchr(process_path, '/');
		if (!pszProcessName)
		{
			return false;
		}
		++pszProcessName;
	}
	strlcat(_file_path, pszProcessName, sizeof(_file_path));
	_file_path_Len = (uint16_t)strlen(_file_path);

	return true;
}

//初始化
static bool check_queue()
{
	if (!_queue)
	{
		auto_mutex	mutex(&_mutex);

		if (!_queue)
		{
			if (!make_full_path())
			{
				return false;
			}
			_queue = open_log_queue();
			if (_queue == NULL)
			{
				return false;
			}
		}
	}
	
	return true;
}

static int do_local_log( Gaz::Log::LOG_LEVEL log_level, const char* keyword, const char* text)
{
	struct timeval tv;
	Time::now(&tv);
	return write_log_nocache(_file_path, _limit_length_per_file, _file_cycle_num, log_level, tv, keyword, text);
}

static int do_remote_log(Gaz::Log::LOG_LEVEL log_level, const char* keyword, const char* text)
{
	// 挂载mmap
	if (!check_queue())
	{
		return -1;
	}

	int iKeywordLen = strlen(keyword);
	if (iKeywordLen > Gaz::Log::MAX_KEYWORD_LEN)
	{
		return -2;
	}

	//input
	uint8_t	buf[sizeof(uint8_t)* 5 + sizeof(uint32_t)+sizeof(uint16_t) + Gaz::Log::MAX_TEXT_LEN + Gaz::Log::MAX_KEYWORD_LEN];
	host_codec os(sizeof(buf), buf);

	os << static_cast<uint8_t>(log_level);
	os << static_cast<uint8_t>(_date_category);
	os << _limit_length_per_file;
	os << _file_cycle_num;

	if (!AppendU16Str(os, _file_path, _file_path_Len))
	{
		return -2;
	}
	if (!AppendU8Str(os, keyword, (uint8_t)iKeywordLen))
	{
		return -2;
	}
	if (!AppendU16Str(os, text))
	{
		return -2;
	}
	if (!os)
	{
		return -2;
	}

	return mq_put(_queue, (void*)buf, os.wpos());
}

//u8LogLevel + u8DateCategory + u32LimitLengthPerFile + u8FileCycleNum + u16FilePathLen * c8FilePath + u8KeywordLen * c8Keyword + u16TextLen * c8Text
static int do_log(LOG_TYPE log_type, Gaz::Log::LOG_LEVEL log_level, const char* keyword, const char* format, va_list va)
{
	//filter 1
	switch (log_type)
	{
	case LT_LOCAL:
		if (log_level < _local_lowest_level)
		{
			return 1;
		}
		break;
	case LT_REMOTE:
		if (log_level < _remote_lowest_level)
		{
			return 1;
		}
		break;
	case LT_DUPLET:
		if (log_level < _local_lowest_level)
		{
			return do_log(LT_REMOTE, log_level, keyword, format, va);
		}
		if (log_level < _remote_lowest_level)
		{
			return do_log(LT_LOCAL, log_level, keyword, format, va);
		}
		break;
	default:
		return -1;
	}

	char text[Gaz::Log::MAX_TEXT_LEN];
	vsnprintf(text, sizeof(text), format, va);

	//filter 2
	switch (log_type)
	{
	case LT_LOCAL:
		return do_local_log(log_level, keyword, text);
	case LT_REMOTE:
		return do_remote_log(log_level, keyword, text);
	case LT_DUPLET://fall through
	default:
		return -1;
	}
}

////////////////////////////////////////////////////////////////////////////
//可选，默认内部会自动设置为进程名
bool Gaz::Log::set_file_name(const char* file_name/* = NULL*/, uint16_t proc_index/* = (uint16_t)-1*/)
{
	if (file_name)
	{
		strlcpy(_file_name, file_name, sizeof(_file_name));
	}
	
	_proc_index = proc_index;

	return true;
}

//可选，默认为程序运行路径所在目录	
bool Gaz::Log::set_local_cfg(Gaz::Log::LOG_LEVEL lowest_level/* = LL_TRACE*/, const char* dir_path/* = NULL*/, bool date_category/* = true*/, uint32_t limit_length_per_file/* = DEFAULT_LIMITLENGTH_PERFILE*/, uint8_t file_cycle_num/* = DEFAULT_FILE_NUM*/)
{
	_local_lowest_level = lowest_level;

	if (dir_path)
	{
		strlcpy(_dir_name, dir_path, sizeof(_dir_name));
	}

	_date_category = date_category;

	_limit_length_per_file = limit_length_per_file;

	_file_cycle_num = file_cycle_num;

	return true;
}

//可选。不小于其参数值执行远端写log
bool Gaz::Log::set_remote_cfg(Gaz::Log::LOG_LEVEL lowest_level/* = LL_NONE*/)
{
	_remote_lowest_level = lowest_level;

	return true;
}

int Gaz::Log::Local::trace(const char* keyword, const char* format, ...)
{
	va_list va;
	va_start(va, format);
	int iRet = do_log(LT_LOCAL, LL_TRACE, keyword, format, va);
	va_end(va);

	return iRet;
}

int Gaz::Log::Local::info(const char* keyword, const char* format, ...)
{
	va_list va;
	va_start(va, format);
	int iRet = do_log(LT_LOCAL, LL_INFO, keyword, format, va);
	va_end(va);

	return iRet;
}

int Gaz::Log::Local::warn(const char* keyword, const char* format, ...)
{
	va_list va;
	va_start(va, format);
	int iRet = do_log(LT_LOCAL, LL_WARN, keyword, format, va);
	va_end(va);

	return iRet;
}

int Gaz::Log::Local::error(const char* keyword, const char* format, ...)
{
	va_list va;
	va_start(va, format);
	int iRet = do_log(LT_LOCAL, LL_ERROR, keyword, format, va);
	va_end(va);

	return iRet;
}

int Gaz::Log::Local::fatal(const char* keyword, const char* format, ...)
{
	va_list va;
	va_start(va, format);
	int iRet = do_log(LT_LOCAL, LL_FATAL, keyword, format, va);
	va_end(va);

	return iRet;
}

int Gaz::Log::Remote::trace(const char* keyword, const char* format, ...)
{
	va_list va;
	va_start(va, format);
	int iRet = do_log(LT_REMOTE, LL_TRACE, keyword, format, va);
	va_end(va);

	return iRet;
}

int Gaz::Log::Remote::info(const char* keyword, const char* format, ...)
{
	va_list va;
	va_start(va, format);
	int iRet = do_log(LT_REMOTE, LL_INFO, keyword, format, va);
	va_end(va);

	return iRet;
}

int Gaz::Log::Remote::warn(const char* keyword, const char* format, ...)
{
	va_list va;
	va_start(va, format);
	int iRet = do_log(LT_REMOTE, LL_WARN, keyword, format, va);
	va_end(va);

	return iRet;
}

int Gaz::Log::Remote::error(const char* keyword, const char* format, ...)
{
	va_list va;
	va_start(va, format);
	int iRet = do_log(LT_REMOTE, LL_ERROR, keyword, format, va);
	va_end(va);

	return iRet;
}

int Gaz::Log::Remote::fatal(const char* keyword, const char* format, ...)
{
	va_list va;
	va_start(va, format);
	int iRet = do_log(LT_REMOTE, LL_FATAL, keyword, format, va);
	va_end(va);

	return iRet;
}

int Gaz::Log::Duplet::trace(const char* keyword, const char* format, ...)
{
	va_list va;
	va_start(va, format);
	int iRet = do_log(LT_DUPLET, LL_TRACE, keyword, format, va);
	va_end(va);

	return iRet;
}

int Gaz::Log::Duplet::info(const char* keyword, const char* format, ...)
{
	va_list va;
	va_start(va, format);
	int iRet = do_log(LT_DUPLET, LL_INFO, keyword, format, va);
	va_end(va);

	return iRet;
}

int Gaz::Log::Duplet::warn(const char* keyword, const char* format, ...)
{
	va_list va;
	va_start(va, format);
	int iRet = do_log(LT_DUPLET, LL_WARN, keyword, format, va);
	va_end(va);

	return iRet;
}

int Gaz::Log::Duplet::error(const char* keyword, const char* format, ...)
{
	va_list va;
	va_start(va, format);
	int iRet = do_log(LT_DUPLET, LL_ERROR, keyword, format, va);
	va_end(va);

	return iRet;
}

int Gaz::Log::Duplet::fatal(const char* keyword, const char* format, ...)
{
	va_list va;
	va_start(va, format);
	int iRet = do_log(LT_DUPLET, LL_FATAL, keyword, format, va);
	va_end(va);

	return iRet;
}
