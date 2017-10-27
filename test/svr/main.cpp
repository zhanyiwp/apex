#include <iostream>
#include <stdio.h>
#include <ev.h>
#include "util_platform.h"
#include "service_tcp_conn.h"
#include "service_container.h"
#include "server_handler_raw.h"
#include "net_imp.h"

using namespace std;

service_container *g_container;

class test_raw : public server_handler_raw
{
public:
	test_raw(){}
	virtual ~test_raw(){}

	virtual void on_connect()
	{
		cout << "on_connect" << endl;
	}

	virtual void on_closing()
	{
		cout << "on_closing" << endl;
	}

	virtual void on_error(ERRCODE err)
	{
		cout << "on_error " << err << endl;
	}

	//return 0 包不完整 >0 完整的包 
	virtual int32_t on_recv(const uint8_t *pkg, const uint32_t len)
	{
		string data;
		data.assign((char*)pkg, len);
		cout << "on_recv data:" << data << " len:" << len << endl;
		return len;
	}

	virtual void on_pkg(uint64_t channal_id, const uint8_t *pkg, const uint32_t len)
	{
		string data;
		data.assign((char*)pkg, len);
		cout << "on_pkg data:" << data << " channal_id:" << channal_id << endl;

		// relation.insert(pair<uint64_t, uint64_t>(channal_id, channal_id));
		g_container->send(channal_id, pkg, len);
	}

	virtual bool on_find_relation(void* param, uint64_t& counter)
	{
		cout << "on_find_relation" << endl;
		map<uint64_t, uint64_t>::iterator it = relation.find(*(uint64_t*)param);
		if (it != relation.end())
		{
			counter = it->second;
			return true;
		}
		return false;
	}

	map<uint64_t, uint64_t> relation;
};

int main(int argc, char const *argv[])
{
	if (argc != 2)
	{
		exit(-1);
	}
	srand(time(NULL));

	// struct ev_loop* g_loop = ev_default_loop(EVFLAG_AUTO | EVBACKEND_EPOLL);

	// service_container container;
	// g_container = &container;
	// container.init(g_loop, TCP_SERVER_RAW, 0, 0, 65535, 65535, new test_raw());
	// service_tcp_conn service;
	// service.init(g_loop, argv[1], "test", g_container);

	// while(true)
	// {
	// 	ev_run(g_loop, 0);
	// }

	net_imp net;
	service_tcp_conn service;
	if(!net.add_service(argv[1], "test", TCP_SERVER_RAW, service, new test_raw()))
	{
		cout << "add_service" << endl;
		return -1;
	}

 	g_container = service.get_container();

	net.run();


	return 0;
}