// zjg 20160816 add interface for pragamme report

// only support char-code utf8	

#ifndef ATTR_REPORT_H
#define ATTR_REPORT_H

#include <stdint.h>
#include <string>

namespace Gaz
{
	namespace Report
	{
		const	uint16_t	MAX_TEXT_INPUTLEN		=	56;	//Server, Name, Text等字段文本的最大字节数

		const	char		REPORT_MMAP_FILEPATH[]	=	OPTION_REPORT_MMAP_FILEPATH;// "/gaz/agent/report/report.mmap";
		const	uint32_t	REPORT_MMAP_ELEMENTSIZE =	OPTION_REPORT_MMAP_ELEMENTSIZE;
		const	uint32_t	REPORT_MMAP_ELEMENTCOUNT=	OPTION_REPORT_MMAP_ELEMENTCOUNT;

		//添加进程序号（可选），如填写非默认值，server字段将会加上此数据作为后缀，如"onlinesve_2"
		void set_proc_order_index(uint16_t index = (uint16_t)-1);

		// 累加上报
		int inc(const std::string& server, const std::string& name, uint32_t value);

		// 累加上报1
		int inc_once(const std::string& server, const std::string& name);

		enum SetType
		{
			ST_NONE = 0,
			ST_MIN = 1,
			ST_MAX = 2,
			ST_FIRST = 3,
			ST_LAST = 4,
		};
		// 覆盖上报
		int set(const std::string& server, const std::string& name, uint32_t value, SetType type = ST_LAST);

		// 告警
		int alarm(const std::string& server, const std::string& name, const std::string& text);
	}
}

#endif //ATTR_REPORT_H
