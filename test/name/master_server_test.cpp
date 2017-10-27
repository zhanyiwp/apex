#include <stdio.h>
#include <string.h>
#include "socket_func.h"
#include "util.h"
#include "net.h"
#include "name.h"

using namespace Net;

struct ev_loop *g_loop = EV_DEFAULT;

class server_handler : public tcp_server_handler_hello
{
    public:
		server_handler(const Config::section& data) : tcp_server_handler_hello(data) { }
        ~server_handler() { }

        virtual void on_accepted(tcp_server_channel* channel)
        {
			printf("on_connect\n");
			
			channel->start_timer(custom_min_id, 15000, true);

			tcp_server_handler_hello::on_accepted(channel);
        }
        virtual void on_closing(tcp_server_channel* channel)		
        {
			printf("on_closing\n");
			
			channel->stop_timer(custom_min_id);

			tcp_server_handler_hello::on_closing(channel);
        }
		virtual void on_error(ERROR_CODE err, tcp_server_channel* channel)
        {
			printf("error: %u\n", (uint32_t)err);

			tcp_server_handler_hello::on_error(err, channel);
        }

        virtual void on_timer(uint8_t timer_id, tcp_server_channel* channel)
        {
			printf("on_timer\n");
				
			tcp_server_handler_hello::on_timer(timer_id, channel);			
        }

        virtual void on_pkg(InnerHeader* netorder_header, tcp_server_channel* channel)
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
						if (Name::MasterMode::Server::is_master(_conf_section.get_name()))
						{
							host_codec is(ntohs(netorder_header->bodylen), (uint8_t*)netorder_header + sizeof(InnerHeader));

							string str;
							ReadU8Str(is, str);

							uint8_t	obuf[1024];
							InnerHeader* head = (InnerHeader*)obuf;
							memcpy(head, netorder_header, sizeof(InnerHeader));
							host_codec os(sizeof(obuf) - sizeof(InnerHeader), obuf + sizeof(InnerHeader));

							os << static_cast<uint8_t>(0);
							AppendU8Str(os, "who are you?");
							head->bodylen = htons(os.wpos());
							channel->try_send(obuf, sizeof(InnerHeader) + os.wpos());

							printf("recv: %s and send 'who are you?'\n", str.c_str());
						}
						else
						{
							printf("local is not master\n");
						}
					}
				}
				break;
			}

			tcp_server_handler_hello::on_pkg(netorder_header, channel);
        }
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
	string section_name;
	sys.get("server_name", section_name);

	Config::section conf(argv[1], section_name);	
	Name::MasterMode::Server::reg(conf);
	
    server_handler *server = new server_handler(conf);
    Net::add_tcp_listen(g_loop, server);

    run(g_loop);

    printf("hello,world\n");

    return 0;
}
