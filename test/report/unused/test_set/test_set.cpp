#include <arpa/inet.h>
#include <stdio.h>
#include <signal.h>
#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <sys/time.h>

#include "tc_clientsocket.h"
#include "monitor.pb.h"

using namespace std;

const uint32_t SEC_PER_MIN = 60;
const uint32_t MIN_PER_HOUR = 60;
const uint32_t HOUR_PER_DAY = 24;

const uint32_t SEC_PER_HOUR = MIN_PER_HOUR*SEC_PER_MIN;
const uint32_t SEC_PER_DAY = HOUR_PER_DAY*SEC_PER_HOUR;
const uint32_t MIN_PER_DAY = HOUR_PER_DAY*MIN_PER_HOUR;

class CLocalTime
{
public:
	CLocalTime(time_t tTime = ::time(NULL)): m_tTime(tTime)
	{
		localtime_r(&m_tTime, &m_stTM);
	}

	~CLocalTime()	{}

	time_t time(void) const
	{
		return m_tTime;
	}

	size_t hour(void) const
	{
		return m_stTM.tm_hour;
	}

	size_t minute(void) const
	{
		return m_stTM.tm_min;
	}

	size_t second(void) const
	{
		return m_stTM.tm_sec;
	}

private:
	time_t m_tTime;
	struct tm m_stTM;
};	


uint64_t GetTimeInUs()
{
	uint64_t ret;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	ret = tv.tv_sec * (uint64_t)1000000 + tv.tv_usec;
	return ret;
}

int main(int argc, char *argv[])
{
	taf::TC_TCPClient client;
	client.init("10.104.95.106", 38002, 1000);
//	sendrecv
	string req_str;
	ngse::monitor::ReqReport req;

    for (int i=0; i<atoi(argv[1]); ++i)
    {
        char attrname[32];
        snprintf(attrname, sizeof(attrname), "func%d", i+5000);
        ngse::monitor::Attr* attr = req.add_attrs();
        attr->set_servicename("QQService.TestSvc");
        attr->set_attrname(attrname);

        CLocalTime lt;
//        time_t day_begin_time = lt.time() - SEC_PER_HOUR*lt.hour()-SEC_PER_MIN*lt.minute()-lt.second();
//        attr->set_begin_time(day_begin_time - 60);
//        attr->set_end_time(day_begin_time + 60);
	time_t begin_time = lt.time() - lt.second() - 60;
	attr->set_begin_time(begin_time);
	attr->set_end_time(begin_time);
        attr->add_values(3);
    }
	
	
	string body;
	if(!req.SerializeToString(&body))
	{
		printf("[error] fails to serialize.\n");
		return 0;
	}
	uint32_t size = htonl(body.size() + sizeof(uint32_t));
	req_str.assign((char*)&size, sizeof(uint32_t));
	req_str.append(body);
	static char recvBuf[128*1024];
	size_t len = sizeof(recvBuf);

    uint64_t tbeg = GetTimeInUs();    
    int count = 300;
    for (int i=0; i<count; ++i)
    {
        int ret = client.sendRecv(req_str.data(), req_str.size(), &recvBuf[0], len);
        if(ret == 0)
        {
            printf("recv len:%lu|%u\n", len, ntohl(*(uint32_t*)&recvBuf[0]));
            /*ngse::monitor::RespReport resp;
            if(resp.ParseFromArray( &recvBuf[4], len-4 ))
            {
                printf("%s\n",resp.ShortDebugString().c_str());
            }*/
        }
        else
        {
            printf("sendrecv failed|%d\n", ret);
        }
    }
    uint64_t tend = GetTimeInUs();    
    printf("avg timecost: %llu ms\n", (tend - tbeg) / count / 1000);


	return 0;
}

