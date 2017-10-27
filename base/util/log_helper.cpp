#include "log_helper.h"
#include "file_func.h"
#include "time_func.h"
#include "limits.h"
#include "stdio.h"
#include <map>

using namespace std;

static char aLevelStr[7][10] =
{
	"NONE",
	"NONE",
	"TRACE",
	"INFO",
	"WARN",
	"ERROR",
	"FATAL"
};

//暂时不缓存，后续优化可能需要缓存+锁
int	write_log_nocache(std::string file_path, const uint32_t length_per_file, const uint8_t file_cycle_num, const Gaz::Log::LOG_LEVEL level, const struct timeval tv, const std::string keyword, const std::string& text)
{
	if (level > Gaz::Log::LL_FATAL || level < Gaz::Log::LL_NONE)
	{
		return -1;
	}

	//本地log
	string file_name = file_path;
	int32_t pos = file_path.find_last_of(".log");
	if (pos >= 0 && pos + 1 == (int32_t)file_path.size())
	{
		file_name = file_path.substr(0, pos);
	}
	else
	{
		file_path += ".log";
	}

	map<string, FILE*> mapFile;
	//检查文件内容长度
	if (get_file_size(file_path.c_str()) >= length_per_file)
	{
		for (int16_t j = (int16_t)file_cycle_num - 1; j > 0; j--)
		{
			char src[PATH_MAX];
			char dst[PATH_MAX];
			if (j == 0)
			{
				snprintf(src, sizeof(src), "%s.log", file_name.c_str());
			}
			else
			{
				snprintf(src, sizeof(src), "%s%d.log", file_name.c_str(), j);
			}
			snprintf(dst, sizeof(dst), "%s%d.log", file_name.c_str(), j + 1);

			rename(src, dst);
		}

		FILE* fp = fopen(file_path.c_str(), "ab+");

		//	for example: 2016-10-21 10:12:57 .567432 INFO Conn	"Disconnect for error Pkg format"
		int32_t year, mon, mday, hour, minu, sec;
		Time::extract(tv.tv_sec, 8, &year, &mon, &mday, &hour, &minu, &sec);
		char szText[4096];
		int iRet = snprintf(szText, sizeof(szText), "%04d-%02d-%02d %02d:%02d:%02d.%06ld .%6u %s %s %s\n", year, mon, mday, hour, minu, sec, tv.tv_usec / 1000000, aLevelStr[level - Gaz::Log::LL_NONE], keyword.c_str(), text.c_str());

		if (iRet > 0)
		{
			fwrite(szText, iRet, 1, fp);
		}

		fclose(fp);
	}

	return 0;
}

struct mmap_queue* open_log_queue()
{
	return mq_create(Gaz::Log::LOG_MMAP_FILEPATH, Gaz::Log::LOG_MMAP_ELEMENTSIZE, Gaz::Log::LOG_MMAP_ELEMENTCOUNT);
}