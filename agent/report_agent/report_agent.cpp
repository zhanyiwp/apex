#include <arpa/inet.h>
#include <signal.h>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <iostream>
#include <stdio.h> 
#include <stdlib.h> 

#include "util.h"
#include "net.h"

using namespace std;

#define MAX_PKG_LEN (500)

typedef struct 
{
	string ConfigFile;
	uint32_t SemKey;
	uint32_t ElementSize;
	uint32_t ElementCount;
	uint32_t ReportInterval;
	uint32_t CollectInterval;
	string ServerIP;
	uint32_t ServerPort;
	string LocalIP;
	struct sockaddr_in server_addr;
	int		udp_fd;
	struct mmap_queue *queue;
	map<string, map<string, map<uint32_t, uint32_t> > >	mapValue; 	// Server - Attr - time_t(minutes) - values
} CONFIG;

CONFIG s_stConfig;

static long timevaldiff(struct timeval* end, struct timeval* start)
{
	long msec;
	msec=(end->tv_sec-start->tv_sec)*1000;
	msec+=(end->tv_usec-start->tv_usec)/1000;
	return msec;	 
}

//IPC 协议：	u8Type + u8ServerLen * sServer + u8NameLen * sName + stOther
//					u8Type	=	1,	表示	inc,	stOther = u32Value
//							=	2,	表示	set,	stOther = u8SetType + u32Value，其中u8Alarm同SetType
//							=	3,	表示	alarm，	stOther = u8AlarmLen * s8Alarm
static int CollectPkg()
{
	static char buf[4096];
	struct timeval tv;
	struct timeval tnow;
	Time::now(&tnow);

	int len = 0;
	bool Stop = false;
	while ((len = mq_get(s_stConfig.queue, buf, sizeof(buf), &tv))>0 && !Stop)
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

		uint8_t	u8Type = 0;
		uint8_t	u8ServerLen = 0;
		is >> u8Type >> u8ServerLen;
		if (!is)
		{
			break;
		}
		char szServer[256];
		is.read(u8ServerLen, szServer);
		szServer[u8ServerLen] = 0;

		uint8_t	u8NameLen = 0;
		is >> u8NameLen;
		if (!is)
		{
			break;
		}
		char szName[256];
		is.read(u8NameLen, szName);
		szName[u8NameLen] = 0;

		string strServer;
		string strName;
		strServer.assign(szServer, u8ServerLen);
		strName.assign(szName, u8NameLen);

		switch (u8Type)
		{
			case 1:
			{
				uint32_t Value;
				is >> Value;
				if (!is)
				{
					return -1;
				}

				uint32_t tmin = tv.tv_sec / 60;
				s_stConfig.mapValue[strServer][strName][tmin] += Value;
			}
			break;
		case 2:
			{
				uint8_t u8SetType = 0;
				uint32_t Value = 0;
				is >> u8SetType >> Value;
				if (!is)
				{
					return -1;
				}

				uint32_t tmin = tv.tv_sec / 60;
				switch ((Report::SetType)u8SetType)
				{
				case Report::ST_MIN:
					if (Value < s_stConfig.mapValue[strServer][strName][tmin])
					{
						s_stConfig.mapValue[strServer][strName][tmin] = Value;
					}
					break;
				case Report::ST_MAX:
					if (Value > s_stConfig.mapValue[strServer][strName][tmin])
					{
						s_stConfig.mapValue[strServer][strName][tmin] = Value;
					}
					break;
				case Report::ST_FIRST:
					{
						do 
						{
							map<string, map<string, map<uint32_t, uint32_t> > >::iterator it = s_stConfig.mapValue.find(strServer);
							if (it != s_stConfig.mapValue.end())
							{
								map<string, map<uint32_t, uint32_t> >::iterator it2 = it->second.find(strName);
								if (it2 != it->second.end())
								{
									map<uint32_t, uint32_t>::iterator it3 = it2->second.find(tmin);
									if (it3 != it2->second.end())
									{
										break;
									}
								}
							}

							s_stConfig.mapValue[strServer][strName][tmin] = Value;
						} while (0);
					}
					break;				
				case Report::ST_LAST:
					{
						s_stConfig.mapValue[strServer][strName][tmin] = Value;
					}
					break;
				default:
					break;
				}
			}
			break;
		case 3:
			{
				uint8_t	u8AlarmLen = 0;
				is >> u8AlarmLen;
				if (!is)
				{
					return -1;
				}
				char szAlarm[256];
				is.read(u8AlarmLen, szAlarm);
				szAlarm[u8AlarmLen] = 0;
				if (!is)
				{
					return -1;
				}
				string strAlarm;
				strAlarm.assign(szAlarm, u8AlarmLen);
			}
			break;
		default:
			return -1;
		}
	}
	return 0;
}

time_t last_send_time = 0;

static void SendUdpPkgHelper(const char* pBuf, int iLen)
{
	if (sendto(s_stConfig.udp_fd, pBuf, iLen, 0, (struct sockaddr*)&s_stConfig.server_addr, sizeof(s_stConfig.server_addr)) < 0)
	{
		perror("Send File Name Failed.\n");
	}
}

static void SendUdpPkg(map<string, map<uint32_t, map<string, uint32_t> > >& mapValueByTime)
{
	char	szPkg[2 * MAX_PKG_LEN];
	int		iLen = 0;
	uint64_t	u64Minute = 0;
	for (map<string, map<uint32_t, map<string, uint32_t> > >::iterator it = mapValueByTime.begin(); it != mapValueByTime.end(); it++)
	{
		for (map<uint32_t, map<string, uint32_t> >::iterator it2 = it->second.begin(); it2 != it->second.end(); it2++)
		{
			u64Minute = it2->first;
			for (map<string, uint32_t>::iterator it3 = it2->second.begin(); it3 != it2->second.end(); it3++)
			{
				if (!iLen)
				{
					iLen = snprintf(szPkg, sizeof(szPkg), "%s,host=%s %s=%u", it->first.c_str(), s_stConfig.LocalIP.c_str(), it3->first.c_str(), it3->second);
				}
				else
				{
					iLen += snprintf(&szPkg[iLen], sizeof(szPkg) - iLen, ",%s=%u", it3->first.c_str(), it3->second);
				}
				if (iLen >= MAX_PKG_LEN)
				{
					uint64_t ns = u64Minute * 1000000000 * 60;
					iLen += snprintf(&szPkg[iLen], sizeof(szPkg) - iLen, " %lu", ns);

					SendUdpPkgHelper(szPkg, iLen);
					iLen = 0;
				}
			}
		}
	}
	if (iLen)
	{
		uint64_t ns = u64Minute * 1000000000 * 60;
		iLen += snprintf(&szPkg[iLen], sizeof(szPkg) - iLen, " %lu", ns);

		SendUdpPkgHelper(szPkg, iLen);
		iLen = 0;
	}
}

static void HandleRecord(time_t tnow)
{
	map<string, map<uint32_t, map<string, uint32_t> > >	mapValueByTime;

	for (map<string, map<string, map<uint32_t, uint32_t> > >::iterator it = s_stConfig.mapValue.begin(); it != s_stConfig.mapValue.end();)
	{
		for (map<string, map<uint32_t, uint32_t> >::iterator it2 = it->second.begin(); it2 != it->second.end();)
		{
			if (!it2->second.size())
			{
				continue;
			}

			for (map<uint32_t, uint32_t>::iterator it3 = it2->second.begin(); it3 != it2->second.end();)
			{
				if (tnow / 60 > it3->first)
				{
					mapValueByTime[it->first][it3->first][it2->first] = it3->second;
					it2->second.erase(it3++);
				}
				else
				{
					it3++;
				}
			}

			if (!it2->second.size())
			{
				it->second.erase(it2++);
			}
			else
			{
				it2++;
			}
		}

		if (!it->second.size())
		{
			s_stConfig.mapValue.erase(it++);
		}
		else
		{
			it++;
		}
	}

	SendUdpPkg(mapValueByTime);
}

static void	on_timer_collect(timer* t)
{
	CollectPkg();
}

static void	on_timer_report(timer* t)
{
	HandleRecord(Time::now());
}

static int Init(int argc, char *argv[])
{
	if (argc < 2)
	{
		printf("Usage: %s config_file\n", argv[0]); 
		exit(0);
	}

	s_stConfig.ConfigFile = argv[1];

	if ( s_stConfig.ConfigFile == "-h" || s_stConfig.ConfigFile == "-H" || s_stConfig.ConfigFile == "?" )
	{
		printf("Usage: %s config_file\n", argv[0]);
		exit(0);
	}
	Config::section sec(s_stConfig.ConfigFile.c_str(), "system");
	sec.get("ElementSize", s_stConfig.ElementSize);
	sec.get("ElementCount", s_stConfig.ElementCount);
	sec.get("ReportInterval", s_stConfig.ReportInterval);
	sec.get("CollectInterval", s_stConfig.CollectInterval);
	sec.get("ServerIP", s_stConfig.ServerIP);
	sec.get("ServerPort", s_stConfig.ServerPort);
	sec.get("LocalIP", s_stConfig.LocalIP);
	
	if(s_stConfig.ElementSize == 0 || s_stConfig.ElementCount == 0
		|| s_stConfig.ReportInterval == 0 || s_stConfig.CollectInterval == 0
		|| s_stConfig.ServerIP == "" || s_stConfig.ServerPort == 0)
	{
		printf("[error] config parameter error.\n");
		return -1;
	}
	
	s_stConfig.queue = mq_create(REPORT_MMAP_FILE, s_stConfig.ElementSize, s_stConfig.ElementCount);
	if (s_stConfig.queue == NULL)
	{
		printf("[error] mmap queue create/open failed.\n");
		return -1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	if (Init(argc, argv)) 
    { 
        printf("[error] Initialize failed.\n");
        return -1; 
    }


	bzero(&s_stConfig.server_addr, sizeof(s_stConfig.server_addr));
	s_stConfig.server_addr.sin_family = AF_INET;
	s_stConfig.server_addr.sin_addr.s_addr = inet_addr(s_stConfig.ServerIP.c_str());
	s_stConfig.server_addr.sin_port = htons((uint16_t)s_stConfig.ServerPort);
	s_stConfig.udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (s_stConfig.udp_fd < 0)
	{
		printf("Create Socket Failed.\n");
		return -1;
	}

	struct ev_loop *loop = EV_DEFAULT;
	// 	Config::section conf("conf.ini", "olinestatus_clt");
	// 	client_log *client_raw = new client_log(conf);
	// 	add_tcp_client(loop, client_raw);

	timer t(loop);
	t.set(on_timer_collect, s_stConfig.CollectInterval);
	t.start();

	timer t1(loop);
	t1.set(on_timer_report, s_stConfig.ReportInterval);
	t1.start();

	daemon_proc();
	Net::run(loop);

	return 0;
}

