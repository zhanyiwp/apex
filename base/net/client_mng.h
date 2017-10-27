#ifndef CLIENT_MNG_H
#define CLIENT_MNG_H

#include <map>

#include "net.h"
#include "period_timer.h"
#include "tcp_client.h"

using namespace std;

class client_mng
{
public:
	client_mng(struct ev_loop* loop);
	~client_mng();

	bool add_tcp(Net::tcp_client_handler_raw* pHandler);
	void del_tcp(uint64_t con_id);//必须延迟delete
	bool is_del(uint64_t con_id);
	string get_section(uint64_t con_id);

	void on_connect(uint64_t id);
	void on_closing(uint64_t id);

	//local
	Net::tcp_client_channel* find_tcp_channel(const string local_section);
	Net::tcp_client_channel* enum_tcp_channel(const string server_cluster, uint64_t& cursor);//cursor为0表示从第一个开始；返回值为0表示结束

	//remote
	Net::tcp_client_channel* find_tcp_channel(const struct sockaddr_in& remotr_addr);

private:
	struct ev_loop*	m_loop;
	uint64_t	alloc_new_id();
	uint64_t	m_id_counter;

	struct tcp_channel_desc 
	{
		uint64_t						id;
		string							section;
		digit_addr				addr;
		tcp_client*						raw_channel_ptr;
		Net::tcp_client_handler_raw*	raw_handler_ptr;
		bool							bMakrDel;
	};
	
	map<uint64_t, tcp_channel_desc>		m_tcp_channel;
	map<string, uint64_t>				m_tcp_section;
	
	map<digit_addr, uint64_t>			m_tcp_addr;

	timer								m_timer_delay_del;
	void	on_timer_delay_del(timer* timer);
};


#endif //CLIENT_MNG_H