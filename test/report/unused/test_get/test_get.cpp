#include <arpa/inet.h>
#include <stdio.h>
#include <signal.h>
#include <vector>
#include <map>
#include <string>
#include <iostream>

#include "tc_clientsocket.h"
#include "monitor.pb.h"

using namespace std;

int usage()
{

	printf("cmd type [params]\n");
	printf("\ttype = 1: get service\n");
	printf("\t\tcmd 1 servicename (days)\n");
	printf("\ttype = 2: get serviceattr\n");
	printf("\t\tcmd 2 servicename attrname day\n");
	printf("\ttype = 3: get attrip\n");
	printf("\t\tcmd 3 servicename attrname ip day\n");
	printf("\ttype = 4: get ip info\n");
	printf("\t\tcmd 4 ip (days)\n");
	printf("\ttype = 5: get ipattr\n");
	printf("\t\tcmd 5 ip servicename attrname day\n");
	printf("\ttype = 20: set alarm attr\n");
	printf("\t\tcmd 20 servicename attrname max min diff diffp\n");
	printf("\ttype = 21: del alarm attr\n");
	printf("\t\tcmd 21 servicename attrname\n");
	printf("\ttype = 22: get newest alarms\n");
	printf("\t\tcmd 22 servicename day\n");	
	printf("\ttype = 23: del alarm\n");
	printf("\t\tcmd 23 servicename attrname alarmtype date time\n");
	return 0;
}

int main(int argc, char *argv[])
{
	taf::TC_TCPClient client;
	client.init("10.104.95.106", 48003, 1000);
//	sendrecv
	string req_str;
	ngse::monitor::ReqMonitor req;
	if(argc < 2)
		return usage();	
	int cmd = atoi(argv[1]);
	if(cmd == 1)
	{
		if(argc < 3)
			return usage();
		ngse::monitor::ReqService* service = req.mutable_service();
		service->set_servicename(argv[2]);
		for(int i = 3; i < argc; i++)
			service->add_days(atoi(argv[i]));
	}
	else if(cmd == 2)
	{
		if(argc != 5)
			return usage();
		ngse::monitor::ReqServiceAttr* serviceattr = req.mutable_serviceattr();
		serviceattr->set_servicename(argv[2]);
		serviceattr->add_attrnames(argv[3]);
		serviceattr->add_days(atoi(argv[4]));
	}
	else if(cmd == 3)
	{
		if(argc != 6)
			return usage();
		ngse::monitor::ReqAttrIP* attrip = req.mutable_attrip();
		attrip->set_servicename(argv[2]);
		attrip->set_attrname(argv[3]);
		attrip->add_ips(argv[4]);
		attrip->add_days(atoi(argv[5]));
	}
	else if(cmd == 4)
	{
		if(argc < 3)
			return usage();
		ngse::monitor::ReqIP* ip = req.mutable_ip();
		ip->set_ip(argv[2]);
		for(int i = 3; i < argc; i++)
			ip->add_days(atoi(argv[i]));
	}
	else if(cmd == 5)
	{
		if(argc != 6)
			return usage();
		ngse::monitor::ReqIPAttr* ipattr = req.mutable_ipattr();
		ipattr->set_ip(argv[2]);
		ngse::monitor::IPData* ipdata = ipattr->add_attrs();
		ipdata->set_servicename(argv[3]);
		ipdata->add_attrnames(argv[4]);
		ipattr->add_days(atoi(argv[5]));
	}
	else if(cmd == 20)
	{
		if(argc != 8)
			return usage();
		ngse::monitor::ReqSetAlarmAttr* setalarmattr = req.mutable_setalarmattr();
		setalarmattr->set_servicename(argv[2]);
		setalarmattr->set_attrname(argv[3]);
		if(atoi(argv[4]) != -1)
			setalarmattr->set_max(atoi(argv[4]));
		if(atoi(argv[5]) != -1)
			setalarmattr->set_min(atoi(argv[5]));
		if(atoi(argv[6]) != -1)
			setalarmattr->set_diff(atoi(argv[6]));
		if(atoi(argv[7]) != -1)
			setalarmattr->set_diff_percent(atoi(argv[7]));
	}
	else if(cmd == 21)
	{
		if(argc != 4)
			return usage();
		ngse::monitor::ReqDelAlarmAttr* delalarmattr = req.mutable_delalarmattr();
		delalarmattr->set_servicename(argv[2]);
		delalarmattr->set_attrname(argv[3]);
	}
	else if(cmd == 22)
	{
		if(argc != 3 && argc != 4)
			return usage();
		ngse::monitor::ReqNewestAlarm* newalarm = req.mutable_newalarm();
		if(argc == 3)
		{
			newalarm->set_day(atoi(argv[2]));
		}
		else
		{
			newalarm->set_servicename(argv[2]);
			newalarm->set_day(atoi(argv[3]));
		}
	}
	else if(cmd == 23)
	{
		if(argc != 7)
			return usage();
		ngse::monitor::ReqDelAlarm* delalarm = req.mutable_delalarm();
		delalarm->set_servicename(argv[2]);
		delalarm->set_attrname(argv[3]);
		delalarm->set_type(ngse::monitor::AlarmType(atoi(argv[4])));
		delalarm->set_day(atoi(argv[5]));
		delalarm->set_time(atoi(argv[6]));
	}
	else
		return usage();
	
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
	int ret = client.sendRecv(req_str.data(), req_str.size(), &recvBuf[0], len);
	if(ret == 0)
	{
		printf("recv len:%lu|%u\n", len, ntohl(*(uint32_t*)&recvBuf[0]));
		ngse::monitor::RespMonitor resp;
		if(resp.ParseFromArray( &recvBuf[4], len-4 ))
		{
			printf("%s\n",resp.ShortDebugString().c_str());
		}
	}
	else
	{
		printf("sendrecv failed|%d\n", ret);
	}
	return 0;
}

