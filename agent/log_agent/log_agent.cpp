#include <arpa/inet.h>
#include <signal.h>
#include <vector>
#include <string>
#include <map>
#include "limits.h"

#include "util.h"
#include "mmap_queue.h"
#include "log_helper.h"
#include "net.h"

#define MAX_PKG_LEN (500)

using namespace Net;
using namespace std;

struct Log_Line
{
	LOG_TYPE		eType;
	Log::LOG_LEVEL	eLevel;
	bool			bDateCategory;
	uint32_t		u32LimitLengthPerFile;
	uint8_t			u8FileCycleNum;
	string			strFilePath;
	string			strKeyword;
	string			strText;
	struct timeval	tv;
};

typedef struct 
{
	uint32_t	SemKey;
	uint32_t	ElementSize;
	uint32_t	ElementCount;
	uint32_t	ReportInterval;
	uint32_t	CollectInterval;
	string 		ServerIP;
	uint32_t	ServerPort;
	struct mmap_queue *queue;
	vector<Log_Line>	vecValue;
} CONFIG;

CONFIG stConfig;

static long timevaldiff(struct timeval* end, struct timeval* start)
{
	long msec;
	msec=(end->tv_sec-start->tv_sec)*1000;
	msec+=(end->tv_usec-start->tv_usec)/1000;
	return msec;	 
}

//IPC 协议：	u8LogType + u8LogLevel + u8DateCategory + u32LimitLengthPerFile + u8FileCycleNum + u16FileNameLen * c8FileName + u8KeywordLen * c8Keyword + u16TextLen * c8Text
static int ConsumeQueue()
{
	static char buf[4096];
	struct timeval tv;
	struct timeval tnow;
	Time::now(&tnow);

	int len = 0;
	bool Stop = false;
	while ((len = mq_get(stConfig.queue, buf, sizeof(buf), &tv)) > 0 && !Stop)
	{
		if(timevaldiff(&tnow, &tv) >= 86400000)	//超过1天的数据不上报了
		{
			continue;
		}
		else if(timevaldiff(&tnow, &tv) < 0) //超过的不继续了
		{
			Stop = true;
		}

		//decode
		host_codec is(sizeof(buf), (uint8_t*)buf);
		Log_Line	line;

		uint8_t u8LogType = 0, u8LogLevel = 0, u8DateCategory = 0;
		is >> u8LogType >> u8LogLevel >> u8DateCategory;
		
		line.eType = (LOG_TYPE)u8LogType;
		line.eLevel = (Log::LOG_LEVEL)u8LogLevel;
		if (line.eLevel <= Log::LL_NONE || line.eLevel > Log::LL_FATAL)
		{
			continue;
		}
		line.bDateCategory = (0 != u8DateCategory);
		
		is >> line.u32LimitLengthPerFile >> line.u8FileCycleNum;
		ReadU16Str(is, line.strFilePath);
		ReadU8Str(is, line.strKeyword);
		ReadU16Str(is, line.strText);
				
		if (is)
		{
			line.tv = tv;
			stConfig.vecValue.push_back(line);
		}
	}
	return 0;
}

class client_log : public tcp_client_handler_inner
{
public:
	client_log(const Config::section& data) : tcp_client_handler_inner(data) { }
	~client_log() { }

	virtual void on_connect(tcp_client_channel* channel)
	{
		channel->start_timer(Net::timer_hello + 10, 5000, true);

		tcp_client_handler_inner::on_connect(channel);
	}
	virtual void on_closing(tcp_client_channel* channel)
	{
		channel->stop_timer(Net::timer_hello + 10);

		tcp_client_handler_inner::on_closing(channel);
	}
	virtual void on_error(ERROR_CODE err, tcp_client_channel* channel)
	{
		tcp_client_handler_inner::on_error(err, channel);
	}
	
	virtual void on_timer(uint8_t timer_id, tcp_client_channel* channel)
	{
		if (Net::timer_hello + 10 == timer_id)
		{

		}

		tcp_client_handler_inner::on_timer(timer_id, channel);
	}

	virtual void on_pkg(InnerHeader* netorder_header, tcp_client_channel* channel)
	{
		tcp_client_handler_inner::on_pkg(netorder_header, channel);
	}
};

static int Init(int argc, char *argv[])
{
	if (argc < 2)
	{
		printf("Usage: %s config_file\n", argv[0]); 
		return -1;
	}

	string str_agr1 = argv[1];
	if (str_agr1 == "-h" || str_agr1 == "-H" || str_agr1 == "?")
	{
		printf("Usage: %s config_file\n", argv[0]);
		return -1;
	}
	Config::section conf(str_agr1, "system");
	
	conf.get("ElementSize", stConfig.ElementSize);
	conf.get("ElementCount", stConfig.ElementCount);
	conf.get("ReportInterval", stConfig.ReportInterval);
	conf.get("CollectInterval", stConfig.CollectInterval);
	
	if (stConfig.ElementSize == 0 || stConfig.ElementCount == 0
		|| stConfig.ReportInterval == 0 || stConfig.CollectInterval == 0)
	{
		printf("[error] config parameter error.\n");
		return -1;
	}
	
	stConfig.queue = mq_create(LOG_MMAP_FILE, stConfig.ElementSize, stConfig.ElementCount);
	if(stConfig.queue == NULL)
	{
		printf("[error] mmap queue create/open failed.\n");
		return -1;
	}

	return 0;
}

static bool SendPkg(Log_Line* p)
{
	return false;
}

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

static void	DoLog()
{
	//本地log
	map<string, FILE*>	mapFile;
	for (size_t i = 0; i < stConfig.vecValue.size(); i++)
	{
		Log_Line* p = &stConfig.vecValue[i];

		if (p->eType == LT_REMOTE || p->eType == LT_DUPLET)
		{
			SendPkg(p);
		}
		if (p->eType == LT_LOCAL || p->eType == LT_DUPLET)
		{
			string file_path = lowstring(p->strFilePath);
			string file_name = file_path;
			int32_t pos = file_path.find_last_of(".log");
			if (pos >= 0 && pos + 1 == file_path.size())
			{
				file_name = file_path.substr(0, pos);
			}
			else
			{
				file_path += ".log";
			}

			//检查文件内容长度
			if (get_file_size(file_path.c_str()) >= p->u32LimitLengthPerFile)
			{
				map<string, FILE*>::iterator it = mapFile.find(file_path);
				if (it != mapFile.end())
				{
					fclose(it->second);
					mapFile.erase(file_path);
				}
				remove(file_path.c_str());

				for (int16_t j = (int16_t)p->u8FileCycleNum - 1; j > 0; j--)
				{
					char szSrc[PATH_MAX];
					char szDst[PATH_MAX];
					if (j == 0)
					{
						snprintf(szSrc, sizeof(szSrc), "%s.log", file_name.c_str());
					}
					else
					{
						snprintf(szSrc, sizeof(szSrc), "%s%d.log", file_name.c_str(), j);
					}
					snprintf(szDst, sizeof(szDst), "%s%d.log", file_name.c_str(), j + 1);

					it = mapFile.find(szSrc);
					if (it != mapFile.end())
					{
						fclose(it->second);
						mapFile.erase(szSrc);
					}
					it = mapFile.find(szDst);
					if (it != mapFile.end())
					{
						fclose(it->second);
						mapFile.erase(szDst);
					}
					rename(szSrc, szDst);
				}
			}

			map<string, FILE*>::iterator it = mapFile.find(file_path);
			if (it == mapFile.end())
			{
				FILE* fp = fopen(file_path.c_str(), "ab+");
				if (!fp)
				{
					continue;
				}

				mapFile[file_path] = fp;
				it = mapFile.find(file_path);
			}

			//	for example: 2016-10-21 10:12:57 INFO Conn	"Disconnect for error Pkg format"
			int32_t year, mon, mday, hour, minu, sec;
			Time::extract(p->tv.tv_sec, 8, &year, &mon, &mday, &hour, &minu, &sec);
			char szText[4096];
			int iRet = snprintf(szText, sizeof(szText), "%04d-%02d-%02d %02d:%02d:%02d.%06ld %s %s %s\n", year, mon, mday, hour, minu, sec, p->tv.tv_usec, aLevelStr[p->eLevel - Log::LL_NONE], p->strKeyword.c_str(), p->strText.c_str());

			if (iRet > 0)
			{
				fwrite(szText, iRet, 1, it->second);
			}
		}
	}
	for (map<string, FILE*>::iterator it = mapFile.begin(); it != mapFile.end(); it++)
	{
		fclose(it->second);
	}

	stConfig.vecValue.clear();
}

static void	on_timer_cb(timer* t)
{
	ConsumeQueue();
	DoLog();
}

int main(int argc, char *argv[])
{
	struct ev_loop *loop = EV_DEFAULT;

	if (Init(argc, argv)) 
    { 
        printf("[error] Initialize failed.\n");
        return -1; 
    }
	

// 	Config::section conf("conf.ini", "olinestatus_clt");
// 	client_log *client_raw = new client_log(conf);
// 	add_tcp_client(loop, client_raw);

	timer t(loop);
	t.set(on_timer_cb, 60 * 1000);
	t.start();

	daemon_proc();
	run(loop);

	return 0;
}
