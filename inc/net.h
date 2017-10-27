#ifndef NET_H
#define NET_H

#include "stdint.h"
#include <netinet/in.h>
#include "ev.h"
#include "proto_header.h"
#include "assert.h"
#include "err.h"
#include <cstdlib>

#include <map>
#include <string>
#include <fstream>
#include <sstream>

#include "time_func.h"
#include "conf_file.h"

using namespace std;

struct digit_addr
{
	uint32_t ip;
	uint16_t port;

	digit_addr()
	{
		ip = 0;
		port = 0;
	}
	digit_addr(uint32_t _ip, uint16_t _port)
	{
		ip = _ip;
		port = _port;
	}
	bool operator < (const digit_addr& r) const
	{
		if (ip < r.ip)
		{
			return true;
		}
		else if (ip == r.ip)
		{
			return port < r.port ? true : false;
		}
		else
		{
			return false;
		}
	}
};
//说明：通过注册handler进行连接和收消息回调处理，在回调函数里面也可以通过函数参数执行连接管理，外部可以通过调用find_tcp_channel函数给指定channel发消息
//即仅仅channel和注册函数对用户可见

//要求运行在64位系统，因为timer::set()有void*，但一个实例是用了uint64_t作为参数

namespace Net
{
	//-------client--------
	//[olinestatus_clt]
	//server_ip=192.16.8.1.5
	//server_port=3456
	//server_cluster=profile_svr
	//sock_wmem=502400
	//sock_rmem=502400
	//ibuf_len=25600000
	//obuf_len=25600000
	//hello_interval=10  //两个含义：15秒发一个包；在下次发包前如果没有收到回包，则认为hello无回应
	//
	//-------service-------
	//[profile_svr]
	//listen_ip=192.16.8.1.5
	//listen_port=3456
	//sock_wmem=502400
	//sock_rmem=502400
	//ibuf_len=25600000
	//obuf_len=25600000
	//stream_expired=15  //离上次收到客户端hello的15秒内，没有再收到hello，则认为本周期hello失败
	//
	//
	const string	conf_sock_wmem = "sock_wmem";
	const string	conf_sock_rmem = "sock_rmem";
	const string	conf_ibuf_len = "ibuf_len";
	const string	conf_obuf_len = "obuf_len";

	const string	conf_server_ip = "server_ip";
	const string	conf_server_port = "server_port";
	const string	conf_server_cluster = "server_cluster";
	const string	conf_hello_interval = "hello_interval";

	const string	conf_listen_ip = "listen_ip";
	const string	conf_listen_port = "listen_port";
	const string	conf_expired = "stream_expired";

	const uint16_t	timer_hello = 1;
	const uint32_t  hello_interval_defval = 15000;
	const uint32_t  outhello_timeout_count = 3;
	const uint32_t  innerhello_timeout_count = 1;

	const uint16_t	timer_reconnect = 2;
	const uint32_t  reconnect_interval_defval = 10000;

	const uint16_t	custom_min_id = timer_hello + 100;

	const uint32_t  keyid_hello = 1;
		
	//建议每个handle独立一个Config::section


	//channel general refer
	class tcp_client_channel
	{
	public:
		tcp_client_channel(){}
		virtual ~tcp_client_channel(){}

		virtual bool try_connect() = 0;
		virtual bool try_send(const uint8_t *pkg, const uint32_t len) = 0;
		virtual void do_close() = 0;
		virtual void delay_delete() = 0;//delete后不能操作其他接口函数，且最多只能再收到on_closing
		virtual uint64_t global_id() = 0;//进程时空范围内唯一id
		virtual bool local_addr(struct sockaddr_in& local) = 0;
		virtual bool remote_addr(struct sockaddr_in& peer) = 0;
		virtual string  get_section() = 0;
		virtual void start_timer(uint16_t timer_id, uint32_t ms, bool repeat) = 0;
		virtual void stop_timer(uint16_t timer_id) = 0;		

		template<typename T>	//类型T因为直接赋值，所以不要虚函数，最好是一个简单无继承的结构体
		bool get_fixed_slot(uint32_t key, T* p)
		{
			return get_value(key, (void**)&p, sizeof(T));
		}
		template<typename T>	//类型T因为直接赋值，所以不要虚函数，最好是一个简单无继承的结构体
		void set_fixed_slot(uint32_t key, T t)
		{
			set_value(key, (void*)&t, sizeof(T));
		}

	protected:
		virtual void		set_value(uint32_t key, void* value, size_t size) = 0;
		virtual bool		get_value(uint32_t key, void** value, size_t size) = 0;
	};
	class tcp_server_channel
	{
	public:
		tcp_server_channel(){}
		virtual ~tcp_server_channel(){}

		virtual bool try_send(const uint8_t *pkg, const uint32_t len) = 0;
		virtual void do_close() = 0;
		virtual void delay_delete() = 0;
		virtual uint64_t global_id() = 0;//进程时空范围内唯一id
		virtual bool local_addr(struct sockaddr_in& local) = 0;
		virtual bool remote_addr(struct sockaddr_in& peer)  = 0;
		virtual string  get_section() = 0;
		virtual void start_timer(uint16_t timer_id, uint32_t ms, bool repeat) = 0;
		virtual void stop_timer(uint16_t timer_id) = 0;
		
		template<typename T>	//类型T因为直接赋值，所以不要虚函数，最好是一个简单无继承的结构体
		bool get_fixed_slot(uint32_t key, T* p)
		{
			return get_value(key, (void**)&p, sizeof(T));
		}
		template<typename T>	//类型T因为直接赋值，所以不要虚函数，最好是一个简单无继承的结构体
		void set_fixed_slot(uint32_t key, T t)
		{
			set_value(key, (void*)&t, sizeof(T));
		}

	protected:
		virtual void		set_value(uint32_t key, void* value, size_t size) = 0;
		virtual bool		get_value(uint32_t key, void** value, size_t size) = 0;
	};

	//client role
	class tcp_client_handler_raw
	{
	public:
		tcp_client_handler_raw(const Config::section& data) : _conf_section(data){}
		virtual ~tcp_client_handler_raw(){}

		Config::section* conf_section()
		{
			return &_conf_section;
		}

		virtual void on_connect(tcp_client_channel* channel){}
		virtual void on_closing(tcp_client_channel* channel){}
		virtual void on_error(ERROR_CODE err, tcp_client_channel* channel){}
		virtual void on_timer(uint8_t timer_id, tcp_client_channel* channel){}

		//return 0 包不完整 >0 完整的包 
		virtual int32_t on_recv(const uint8_t *pkg, const uint32_t len, tcp_client_channel* channel) = 0;
		virtual void on_pkg(const uint8_t *pkg, const uint32_t len, tcp_client_channel* channel){}

	public:
		Config::section	_conf_section;
	};

	class tcp_client_handler_outerhello : public tcp_client_handler_raw
	{
	public:
		tcp_client_handler_outerhello(const Config::section& data) : tcp_client_handler_raw(data){}
		virtual ~tcp_client_handler_outerhello(){}

		virtual void on_connect(tcp_client_channel* channel)
		{
			channel->stop_timer(timer_reconnect);

			HelloData hd;
			channel->set_fixed_slot(keyid_hello, hd);

			uint32_t hello_interval = 0;
			_conf_section.get(Net::conf_hello_interval, hello_interval);
			channel->start_timer(timer_hello, hello_interval, true);
			on_timer(timer_hello, channel);
		}
		virtual void on_closing(tcp_client_channel* channel)
		{
			channel->start_timer(timer_reconnect, reconnect_interval_defval, true);
			channel->stop_timer(timer_hello);
		}
		virtual void on_error(ERROR_CODE err, tcp_client_channel* channel)
		{			
		}
		virtual void on_timer(uint8_t timer_id, tcp_client_channel* channel)
		{
			if (timer_hello == timer_id)
			{
				HelloData* p = NULL;
				if (!channel->get_fixed_slot(keyid_hello, p))
				{
					//error log
				}
				else
				{
					if (p->m_u32LastHelloTime + hello_interval_defval < Time::now())
					{
						p->m_u16TimeoutCount++;

						if (p->m_u16TimeoutCount >= outhello_timeout_count)
						{
							channel->delay_delete();
							return;
						}
					}
					else
					{
						p->m_u16TimeoutCount = 0;
					}

					//send
				}
			}
			else if (timer_reconnect == timer_id)
			{
				channel->try_connect();
			}
		}
		//return 0 包不完整 >0 完整的包 
		virtual int32_t on_recv(const uint8_t *pkg, const uint32_t len, tcp_client_channel* channel)
		{
			if (len >= sizeof(ClientHeader))
			{
				ClientHeader* head = (ClientHeader*)pkg;
				if (len >= sizeof(ClientHeader) + ntohs(head->bodylen))
				{
					return sizeof(ClientHeader) + ntohs(head->bodylen);
				}
			}
			return 0;
		}
		virtual void on_pkg(ClientHeader* netorder_header, tcp_client_channel* channel)
		{
			//p->m_u32LastHelloTime = Time::now();
		}

	private:
		struct HelloData
		{
			HelloData()
			{
				m_u32LastHelloTime = Time::now();
				m_u16TimeoutCount = 0;
			}
			uint32_t	m_u32LastHelloTime;
			uint16_t	m_u16TimeoutCount;
		};
	};

	class tcp_client_handler_inner : public tcp_client_handler_raw
	{
	public:
		tcp_client_handler_inner(const Config::section& data) : tcp_client_handler_raw(data){}
		virtual ~tcp_client_handler_inner(){}

		virtual void on_connect(tcp_client_channel* channel)
		{
			channel->stop_timer(timer_reconnect);
		}
		virtual void on_closing(tcp_client_channel* channel)
		{
			channel->start_timer(timer_reconnect, reconnect_interval_defval, true);
		}
		virtual void on_error(ERROR_CODE err, tcp_client_channel* channel)
		{
		}
		virtual void on_timer(uint8_t timer_id, tcp_client_channel* channel)
		{
			if (timer_reconnect == timer_id)
			{
				channel->try_connect();
			}
		}

		//return 0 包不完整 >0 完整的包 
		virtual int32_t on_recv(const uint8_t *pkg, const uint32_t len, tcp_client_channel* channel)
		{
			if (len >= sizeof(InnerHeader))
			{
				InnerHeader* head = (InnerHeader*)pkg;
				if (len >= sizeof(InnerHeader) + ntohs(head->bodylen))
				{
					return sizeof(InnerHeader) + ntohs(head->bodylen);
				}
			}
			return 0;
		}
		virtual void on_pkg(InnerHeader* netorder_header, tcp_client_channel* channel){}

	private:
		virtual void on_pkg(const uint8_t *pkg, const uint32_t len, tcp_client_channel* channel)
		{
			on_pkg((InnerHeader*)pkg, channel);
		}
	};

	class tcp_client_handler_hello : public tcp_client_handler_inner
	{	
	public:
		tcp_client_handler_hello(const Config::section& data) : tcp_client_handler_inner(data)
		{
		}
		virtual ~tcp_client_handler_hello(){}

		virtual void on_connect(tcp_client_channel* channel)
		{
			tcp_client_handler_inner::on_connect(channel);

			HelloData hd;
			channel->set_fixed_slot(keyid_hello, hd);

			uint32_t hello_interval = 0;
			_conf_section.get(conf_hello_interval, hello_interval);
			channel->start_timer(timer_hello, hello_interval, true);
			
			on_timer(timer_hello, channel);
		}
		virtual void on_closing(tcp_client_channel* channel)
		{
			channel->stop_timer(timer_hello);

			tcp_client_handler_inner::on_closing(channel);
		}
		virtual void on_error(ERROR_CODE err, tcp_client_channel* channel)
		{
			tcp_client_handler_inner::on_error(err, channel);
		}
		virtual void on_timer(uint8_t timer_id, tcp_client_channel* channel)
		{
			if (timer_hello == timer_id)
			{
				HelloData* p = NULL;
				if (!channel->get_fixed_slot(keyid_hello, p))
				{
					//error log
				}
				else
				{
					assert(p);
					if (p->m_u32LastHelloTime + hello_interval_defval < Time::now())
					{
						p->m_u16TimeoutCount++;

						if (p->m_u16TimeoutCount >= outhello_timeout_count)
						{
							channel->delay_delete();
							return;
						}
					}
					else
					{
						p->m_u16TimeoutCount = 0;
					}

					//send ...
				}
			}

			tcp_client_handler_inner::on_timer(timer_id, channel);
		}

		virtual void on_pkg(InnerHeader* netorder_header, tcp_client_channel* channel)
		{
			//p->m_u32LastHelloTime = Time::now();
		}

	private:
		struct HelloData
		{
			HelloData()
			{
				m_u32LastHelloTime = Time::now();
				m_u16TimeoutCount = 0;
			}
			uint32_t	m_u32LastHelloTime;
			uint16_t	m_u16TimeoutCount;
		};
	};

	class tcp_client_handler_login : public tcp_client_handler_hello
	{
	public:
		tcp_client_handler_login(const Config::section& data) : tcp_client_handler_hello(data){}
		virtual ~tcp_client_handler_login(){}

		virtual void on_connect(tcp_client_channel* channel)
		{
			tcp_client_handler_hello::on_connect(channel);
		}
		virtual void on_closing(tcp_client_channel* channel)
		{
			tcp_client_handler_hello::on_closing(channel);
		}
		virtual void on_error(ERROR_CODE err, tcp_client_channel* channel)
		{
			tcp_client_handler_hello::on_error(err, channel);
		}
		virtual void on_timer(uint8_t timer_id, tcp_client_channel* channel)
		{
			//tbd send login
			tcp_client_handler_hello::on_timer(timer_id, channel);
		}

		virtual void on_pkg(InnerHeader* netorder_header, tcp_client_channel* channel){}
	};

	//server role
	class tcp_server_handler_raw
	{
	public:
		tcp_server_handler_raw(const Config::section& data) : _conf_section(data){}
		virtual ~tcp_server_handler_raw(){}

		virtual Config::section* conf_section()
		{
			return &_conf_section;
		}

		virtual void on_accepted(tcp_server_channel* channel){}
		virtual void on_closing(tcp_server_channel* channel){}
		virtual void on_expired(uint32_t ms, tcp_server_channel* channel) //超时
		{
			channel->delay_delete();
		}
		virtual void on_error(ERROR_CODE err, tcp_server_channel* channel){}

		virtual void on_timer(uint8_t timer_id, tcp_server_channel* channel){}

		//return 0 包不完整 >0 完整的包 
		virtual int32_t on_recv(const uint8_t *pkg, const uint32_t len, tcp_server_channel* channel) = 0;
		virtual void on_pkg(const uint8_t *pkg, const uint32_t len, tcp_server_channel* channel){}

	public:
		Config::section	_conf_section;
	};

	class tcp_server_handler_outerhello : public tcp_server_handler_raw
	{
	public:
		tcp_server_handler_outerhello(const Config::section& data) : tcp_server_handler_raw(data){}
		virtual ~tcp_server_handler_outerhello(){}

		virtual void on_accepted(tcp_server_channel* channel){}
		virtual void on_closing(tcp_server_channel* channel){}
		virtual void on_timer(uint8_t timer_id, tcp_server_channel* channel){}
		//return 0 包不完整 >0 完整的包 
		virtual int32_t on_recv(const uint8_t *pkg, const uint32_t len, tcp_server_channel* channel)
		{
			if (len >= sizeof(ClientHeader))
			{
				ClientHeader* head = (ClientHeader*)pkg;
				if (len >= sizeof(ClientHeader) + ntohs(head->bodylen))
				{
					return sizeof(ClientHeader) + ntohs(head->bodylen);
				}
			}
			return 0;
		}
		virtual void on_pkg(ClientHeader* netorder_header, tcp_server_channel* channel){}
	};

	class tcp_server_handler_inner : public tcp_server_handler_raw
	{
	public:
		tcp_server_handler_inner(const Config::section& data) : tcp_server_handler_raw(data){}
		virtual ~tcp_server_handler_inner(){}

		//return 0 包不完整 >0 完整的包 
		virtual int32_t on_recv(const uint8_t *pkg, const uint32_t len, tcp_server_channel* channel)
		{
			if (len >= sizeof(InnerHeader))
			{
				InnerHeader* head = (InnerHeader*)pkg;
				if (len >= sizeof(InnerHeader) + ntohs(head->bodylen))
				{
					return sizeof(InnerHeader) + ntohs(head->bodylen);
				}
			}
			return 0;
		}
		virtual void on_pkg(InnerHeader* netorder_header, tcp_server_channel* channel){}

	private:
		virtual void on_pkg(const uint8_t *pkg, const uint32_t len, tcp_server_channel* channel)
		{
			on_pkg((InnerHeader*)pkg, channel);
		}

	};

	class tcp_server_handler_hello : public tcp_server_handler_inner
	{
	public:
		tcp_server_handler_hello(const Config::section& data) : tcp_server_handler_inner(data)
		{
		}
		virtual ~tcp_server_handler_hello(){}

		virtual void on_accepted(tcp_server_channel* channel){}
		virtual void on_closing(tcp_server_channel* channel){}
		virtual void on_timer(uint8_t timer_id, tcp_server_channel* channel){}

		virtual void on_pkg(InnerHeader* netorder_header, tcp_server_channel* channel){}

	private:
		struct ClientHelloData
		{
			ClientHelloData()
			{
				m_u32LastHelloTime = Time::now();
			}
			uint32_t	m_u32LastHelloTime;
		};
	};

	class tcp_server_handler_login : public tcp_server_handler_hello
	{
	public:
		tcp_server_handler_login(const Config::section& data) : tcp_server_handler_hello(data){}
		virtual ~tcp_server_handler_login(){}

		virtual void on_accepted(tcp_server_channel* channel)
		{
// 			uint32_t hello_interval = 0;
// 			_conf_section.get(conf_hello_interval, hello_interval);
// 			channel->start_timer(timer_hello, hello_interval, true);
			//			channel->start_timer(timer_id_hello, _conf_section.get_u32(hello_interval, hello_interval_defval), true);
		}
		virtual void on_timer(uint8_t timer_id, tcp_server_channel* channel)
		{
			//tbd send login
			tcp_server_handler_hello::on_timer(timer_id, channel);
		}

		virtual void on_pkg(InnerHeader* netorder_header, tcp_server_channel* channel){}
	};

	bool	add_tcp_client(struct ev_loop* loop, tcp_client_handler_raw* pHandler);
	bool	add_tcp_listen(struct ev_loop* loop, tcp_server_handler_raw* pHandler);

	void	run(struct ev_loop* loop);
	void	destroy(struct ev_loop* loop);

	//local
	tcp_client_channel* find_tcp_client_channel(struct ev_loop* loop, const string local_section);
	tcp_client_channel* enum_tcp_client_channel(struct ev_loop* loop, const string server_cluster, uint64_t& cursor);//cursor为0表示从第一个开始；返回值为0表示结束
	//remote
	tcp_client_channel* find_tcp_client_channel(struct ev_loop* loop, const struct sockaddr_in& remotr_addr);

	tcp_server_channel* find_tcp_server_channel(struct ev_loop* loop, const string section, uint64_t channel_id);
	tcp_server_channel* enum_tcp_server_channel(struct ev_loop* loop, const string section, uint64_t& cursor);//cursor为0表示从第一个开始；返回值为0表示结束
}

#endif //NET_H
