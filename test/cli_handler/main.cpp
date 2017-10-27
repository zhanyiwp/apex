#include <iostream>
#include "client_handler_raw.h"
#include "client_tcp_conn.h"
#include "net_imp.h"

using namespace std;

client_tcp_conn g_conn;

class test_handler : public client_handler_raw
{
public:
	test_handler(){}
	virtual ~test_handler(){}

	virtual void on_connect()
	{
		cout << "on_connect" << endl;
		string data = "test";
		g_conn.send("default", (const uint8_t*)data.c_str(), data.size());
	}

	virtual void on_closing()
	{
		cout << "on_closing" << endl;
	}

	virtual void on_error(ERRCODE err)
	{
		cout << "on_error" << endl;
	}

	//return 0 包不完整 >0 完整的包 
	virtual int32_t on_recv(const uint8_t *pkg, const uint32_t len)
	{
		string data;
		data.assign((char*)pkg, len);
		cout << "data:" << data << " len:" << len << endl;
		return len;
	}

	virtual void on_pkg(uint64_t counter, const uint8_t *pkg, const uint32_t len)
	{
		string data;
		data.assign((char*)pkg, len);
		cout << "data:" << data << " len:" << len << endl;
		string ss = "test on_pkg";
		g_conn.send("default", (const uint8_t*)ss.c_str(), ss.size());
	}
};

int main(int argc, char const *argv[])
{
	if (2 != argc)
	{
		exit(-1);
	}
	// struct ev_loop* g_loop = ev_default_loop(EVFLAG_AUTO | EVBACKEND_EPOLL);

	// client_tcp_conn conn;
	// g_conn = &conn;
	// conn.add_client(g_loop, argv[1], "test", CLIENT_TCP_RAW, new test_handler());
	// string data = "test";
	// conn.send("default_raw", (const uint8_t*)data.c_str(), data.size());
	// while(true)
	// {
	// 	ev_run(g_loop, 0);
	// }

	net_imp net;
	if(!net.add_client(argv[1], "test", CLIENT_TCP_RAW, g_conn, new test_handler()))
	{
		cout << "add_client exit" << endl;
		return -1;
	}
	net.run();

	return 0;
}

