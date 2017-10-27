#ifndef SERVICE_MNG_H
#define SERVICE_MNG_H

#include <map>

#include "net.h"
#include "period_timer.h"
#include "tcp_listen.h"
#include "tcp_server.h"

using namespace std;

class service_mng
{
public:
	service_mng(struct ev_loop* loop);
	~service_mng();

	bool add_tcp_listen(Net::tcp_server_handler_raw* pHandler);
	void del_tcp_channel(uint64_t con_id);//必须延迟delete

	Net::tcp_server_channel* find_tcp_channel(const string section, uint64_t id);
	Net::tcp_server_channel* enum_tcp_channel(const string section, uint64_t& cursor);//cursor为0表示从第一个开始；返回值为0表示结束
	
	//inner
	void	on_accept(uint64_t listen_con_id, int new_fd, struct sockaddr_in client_addr);
	bool	is_del(uint64_t con_id);
	string  get_section(uint64_t con_id);
	void	update_expired(uint64_t con_id);

private:
	struct ev_loop*	m_loop;
	uint64_t	alloc_new_id();
	uint64_t	m_id_counter;

	struct tcp_channel_desc
	{
		uint64_t						id;
		string							section_name;
		tcp_listen*						tcp_listen_ptr;
		map<uint64_t, tcp_server*>		alive_channel_map;
		map<uint64_t, tcp_server*>		del_channel_map;
		map<uint64_t, uint32_t>			alive_expired_map;
		Net::tcp_server_handler_raw*	raw_handler_ptr;
		uint32_t						expired;
		timer*							timer_check_expired;
	};

	map<uint64_t, tcp_channel_desc>		m_tcp_channel;

	map<string, uint64_t>				m_tcp_section;//通过section找服务描述id
	map<uint64_t, uint64_t>				m_tcp_server;//通过连接id找服务描述id

	timer							m_timer_delay_del;
	void	on_timer_delay_del(timer* timer);

	void	on_timer_check_expired(timer* timer);
};


#endif //SERVICE_MNG_H