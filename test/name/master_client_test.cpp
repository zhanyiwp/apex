#include <stdio.h>
#include <string.h>
#include <vector>
#include "socket_func.h"
#include "util.h"
#include "net.h"
#include "name.h"

using namespace Net;

struct ev_loop *g_loop = EV_DEFAULT;

class client_handler : public tcp_client_handler_hello
{
public:
	client_handler(const Config::section& sys, const Config::section& data) : tcp_client_handler_hello(data), _sys_section(sys)
	{ 		
	}
    ~client_handler() { }

    virtual void on_connect(tcp_client_channel* channel)
    {
		printf("on_connect\n");
			
		channel->start_timer(custom_min_id, 2000, true);

		tcp_client_handler_hello::on_connect(channel);
    }
    virtual void on_closing(tcp_client_channel* channel)		
    {
		printf("on_closing\n");
			
		channel->stop_timer(custom_min_id);

		tcp_client_handler_hello::on_closing(channel);
    }
	virtual void on_error(ERROR_CODE err, tcp_client_channel* channel)
    {
		printf("error: %u\n", (uint32_t)err);

		tcp_client_handler_hello::on_error(err, channel);
    }

    virtual void on_timer(uint8_t timer_id, tcp_client_channel* channel)
    {
		printf("on_timer\n");
			
		if (timer_id == custom_min_id)
		{
			string cluster_name;
			_conf_section.get("server_cluster", cluster_name);

			struct sockaddr_in v;
			if (!Name::MasterMode::Client::get_master(cluster_name, v))
            {
				printf("MasterMode::Client::get_master of %s failed\n", cluster_name.c_str());
			}  
			else
			{
				tcp_client_channel* low_channel = Net::find_tcp_client_channel(g_loop, v);
				if(!low_channel)
				{
					printf("find_tcp_client_channel fail: %s\n", string_by_addr(&v).c_str());							
				}
				else
				{
					uint8_t	obuf[4096];
					InnerHeader* head = (InnerHeader*)obuf;
					host_codec os(sizeof(obuf) - sizeof(InnerHeader), (uint8_t*)obuf + sizeof(InnerHeader));

					memset(head, 0, sizeof(InnerHeader));
					head->protover = htons(1);
					head->cmd = htons(0x1001);

					AppendU8Str(os, "hello world");
					head->bodylen = htons(os.wpos());
					low_channel->try_send(obuf, sizeof(InnerHeader) + os.wpos());

					printf("send 'hello word'\n");
				}
			}				
		}
		tcp_client_handler_hello::on_timer(timer_id, channel);			
    }

    virtual void on_pkg(InnerHeader* netorder_header, tcp_client_channel* channel)
    {
		printf("on_pkg\n");
			
		switch (ntohs(netorder_header->cmd))
		{
		case 0x1001:
			{
				if (ntohs(netorder_header->bodylen) > 1024)
				{
				}
				else
				{
					host_codec is(ntohs(netorder_header->bodylen), (uint8_t*)netorder_header + sizeof(InnerHeader));

					uint8_t result = 0;
					is >> result;
						
					if(!result)
					{
						string str;
						ReadU8Str(is, str);
							
						printf("recv %s\n", str.c_str());
					}
					else
					{
						printf("recv 0x%x\n", result);
					}
				}
			}
			break;
		}

		tcp_client_handler_hello::on_pkg(netorder_header, channel);
    }

private:
	Config::section	_sys_section;
};

int main(int argc, char** argv)
{
	if(argc < 2)
	{
		printf("argv so little\n");
		return -1;		
	}
	
	if(!Name::init(g_loop, argv[1]))
	{
		printf("Name::Client::init error\n");
		return -1;
	}
	Config::section sys(argv[1], "system");		

	uint16_t num = 0;
	sys.get("test_clt_num", num);	
	for (uint16_t i = 0; i < num; i++)
	{
		char section[32];
		snprintf(section, sizeof(section), "test_clt_%u", i);
		
		Config::section conf(argv[1], section);
		
		string cluster_name;
		conf.get("server_cluster", cluster_name);		
		Name::MasterMode::Client::add_watch(cluster_name);		
		
		client_handler *client = new client_handler(sys, conf);
		add_tcp_client(g_loop, client);		
	}

    run(g_loop);

    printf("hello,world\n");

    return 0;
}
