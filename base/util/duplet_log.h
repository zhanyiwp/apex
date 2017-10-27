// zjg 20160816 add interface for pragamme log

// only support char-code utf8
// lineformat: YYYY-MM-DD HH:MM:SS LEVEL KEYWORD CONTENT
//	for example: 2016-10-21 10:12:57 INFO Conn	"Disconnect for error Pkg format"

// notice=:
//  you need init set_file_name/set_local_cfg/set_remote_cfg in main thread at first if options non-default, because them are not thread safe fully
//  so use "char*" not "std::string"

// local:  当前进程直接写本地磁盘 
// Remote: 通过ipc由专门统一的进程处理 	
// duplet: local + Remote

#ifndef DUPLET_LOG_H
#define DUPLET_LOG_H

#include <stdint.h>

namespace Gaz
{
	namespace Log
	{
		const	char		LOG_MMAP_FILEPATH[] = OPTION_LOG_MMAP_FILEPATH;// "/gaz/agent/log/log.mmap";
		const	uint32_t	LOG_MMAP_ELEMENTSIZE = OPTION_LOG_MMAP_ELEMENTSIZE;
		const	uint32_t	LOG_MMAP_ELEMENTCOUNT = OPTION_LOG_MMAP_ELEMENTCOUNT;

		const	uint8_t		MAX_KEYWORD_LEN = 128;
		const	uint16_t	MAX_TEXT_LEN = 1024;
		const	uint32_t	DEFAULT_LIMITLENGTH_PERFILE = 50 * 1024 * 1024;
		const	uint8_t		DEFAULT_FILE_NUM = 5;
		
		enum LOG_LEVEL
		{
			LL_NONE = 0,
			LL_TRACE = 1,
			LL_INFO = 2,
			LL_WARN = 3,
			LL_ERROR = 4,
			LL_FATAL = 5,
		};

		//可选，默认内部会自动设置为进程名
		bool set_file_name(const char* file_name = NULL, uint16_t proc_index = (uint16_t)-1);
		//可选，默认为程序运行路径所在目录	
		bool set_local_cfg(LOG_LEVEL lowest_level = LL_TRACE, const char* dir_path = NULL, bool date_category = true, uint32_t limit_length_per_file = DEFAULT_LIMITLENGTH_PERFILE, uint8_t file_cycle_num = DEFAULT_FILE_NUM);
		//可选。不小于其参数值执行远端写log
		bool set_remote_cfg(LOG_LEVEL lowest_level = LL_NONE);

		namespace Local
		{
			int trace(const char* pszKeyword, const char* pszFormat, ...);
			int info(const char* pszKeyword, const char* pszFormat, ...);
			int warn(const char* pszKeyword, const char* pszFormat, ...);
			int error(const char* pszKeyword, const char* pszFormat, ...);
			int fatal(const char* pszKeyword, const char* pszFormat, ...);
		}
		namespace Remote
		{
			int trace(const char* pszKeyword, const char* pszFormat, ...);
			int info(const char* pszKeyword, const char* pszFormat, ...);
			int warn(const char* pszKeyword, const char* pszFormat, ...);
			int error(const char* pszKeyword, const char* pszFormat, ...);
			int fatal(const char* pszKeyword, const char* pszFormat, ...);
		}
		namespace Duplet
		{
			int trace(const char* pszKeyword, const char* pszFormat, ...);
			int info(const char* pszKeyword, const char* pszFormat, ...);
			int warn(const char* pszKeyword, const char* pszFormat, ...);
			int error(const char* pszKeyword, const char* pszFormat, ...);
			int fatal(const char* pszKeyword, const char* pszFormat, ...);
		}
	}
}

#define LLOG_TRACE(keyworld, fmt, args...)								\
do{																		\
	Gaz::Log::Local::trace(keyworld, (char*)"[%-10s][%-4d][%-10s]"fmt,	\
		__FILE__, __LINE__, __FUNCTION__, ##args);						\
}while (0)

#define LLOG_INFO(keyworld, fmt, args...)								\
do{																		\
	Gaz::Log::Local::info(keyworld, (char*)"[%-10s][%-4d][%-10s]"fmt, \
		__FILE__, __LINE__, __FUNCTION__, ##args);						\
}while (0)

#define LLOG_WARN(keyworld, fmt, args...)								\
do{																		\
	Gaz::Log::Local::warn(keyworld, (char*)"[%-10s][%-4d][%-10s]"fmt, \
		__FILE__, __LINE__, __FUNCTION__, ##args);						\
}while (0)

#define LLOG_ERROR(keyworld, fmt, args...)								\
do{																		\
	Gaz::Log::Local::error(keyworld, (char*)"[%-10s][%-4d][%-10s]"fmt, \
		__FILE__, __LINE__, __FUNCTION__, ##args);						\
}while (0)

#define LLOG_FATAL(keyworld, fmt, args...)								\
do{																		\
	Gaz::Log::Local::fatal(keyworld, (char*)"[%-10s][%-4d][%-10s]"fmt, \
		__FILE__, __LINE__, __FUNCTION__, ##args);						\
}while (0)


#define RLOG_TRACE(keyworld, fmt, args...)								\
do{																		\
	Gaz::Log::Remote::trace(keyworld, (char*)"[%-10s][%-4d][%-10s]"fmt, \
		__FILE__, __LINE__, __FUNCTION__, ##args);						\
}while (0)

#define RLOG_INFO(keyworld, fmt, args...)								\
do{																		\
	Gaz::Log::Remote::info(keyworld, (char*)"[%-10s][%-4d][%-10s]"fmt, \
		__FILE__, __LINE__, __FUNCTION__, ##args);						\
}while (0)

#define RLOG_WARN(keyworld, fmt, args...)								\
do{																		\
	Gaz::Log::Remote::warn(keyworld, (char*)"[%-10s][%-4d][%-10s]"fmt, \
		__FILE__, __LINE__, __FUNCTION__, ##args);						\
}while (0)

#define RLOG_ERROR(keyworld, fmt, args...)								\
do{																		\
	Gaz::Log::Remote::error(keyworld, (char*)"[%-10s][%-4d][%-10s]"fmt, \
		__FILE__, __LINE__, __FUNCTION__, ##args);						\
}while (0)

#define RLOG_FATAL(keyworld, fmt, args...)								\
do{																		\
	Gaz::Log::Remote::fatal(keyworld, (char*)"[%-10s][%-4d][%-10s]"fmt, \
		__FILE__, __LINE__, __FUNCTION__, ##args);						\
}while (0)


#define DLOG_TRACE(keyworld, fmt, args...)								\
do{																		\
	Gaz::Log::Duplet::trace(keyworld, (char*)"[%-10s][%-4d][%-10s]"fmt, \
		__FILE__, __LINE__, __FUNCTION__, ##args);						\
}while (0)

#define DLOG_INFO(keyworld, fmt, args...)								\
do{																		\
	Gaz::Log::Duplet::info(keyworld, (char*)"[%-10s][%-4d][%-10s]"fmt, \
		__FILE__, __LINE__, __FUNCTION__, ##args);						\
}while (0)

#define DLOG_WARN(keyworld, fmt, args...)								\
do{																		\
	Gaz::Log::Duplet::warn(keyworld, (char*)"[%-10s][%-4d][%-10s]"fmt, \
		__FILE__, __LINE__, __FUNCTION__, ##args);						\
}while (0)

#define DLOG_ERROR(keyworld, fmt, args...)								\
do{																		\
	Gaz::Log::Duplet::error(keyworld, (char*)"[%-10s][%-4d][%-10s]"fmt, \
		__FILE__, __LINE__, __FUNCTION__, ##args);						\
}while (0)

#define DLOG_FATAL(keyworld, fmt, args...)								\
do{																		\
	Gaz::Log::Duplet::fatal(keyworld, (char*)"[%-10s][%-4d][%-10s]"fmt, \
		__FILE__, __LINE__, __FUNCTION__, ##args);						\
}while (0)

#endif	//DUPLET_LOG_H
