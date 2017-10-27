#include <iostream>
#include <svr_tcp_client.h>

using namespace std;

#define ip "127.0.0.1"
#define port 19988

class test_client : public svr_tcp_client
{
public:
	test_client(){data = "test_client";}
	virtual ~test_client(){}
	
	virtual bool on_data()
	{
		cout << "on_data" << endl;
		string buf;
		buf.assign((char*)_sock._ibuf, _sock._ibuf_pos);
		_sock._ibuf_pos = 0;
		cout << "buf:" << buf << endl;
		sleep(1);
		send((const uint8_t*)data.c_str(), data.size());
		return true;
	}

	virtual void on_accepted()
	{
		cout << "on_accepted" << endl;
	}

	virtual void on_closing()
	{
		svr_tcp_client::on_closing();
		cout << "on_closing" << endl;
	}

	string data;
};

int main(int argc, char const *argv[])
{
	struct ev_loop* g_loop = ev_default_loop(EVFLAG_AUTO | EVBACKEND_EPOLL);

	test_client cli;
	cli.init(g_loop, 65535, 65535);
	cli.set_server(ip, port);
	cli.connect();

	cli.set_connect_interval(3);
	cli.start_check_timer(0.5);

	string data = "start client";
	cli.send((const uint8_t*)data.c_str(), data.size());
	
	while(true)
	{
		ev_run(g_loop, 0);
	}
	
	return 0;
}