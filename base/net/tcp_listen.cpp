#include "tcp_listen.h"
#include "service_mng.h"

tcp_listen::tcp_listen()
{
	m_pHandler = NULL;
	m_mng = NULL;
	m_id = 0;
}

tcp_listen::~tcp_listen()
{

}

bool tcp_listen::init(struct ev_loop* loop, uint64_t id, Net::tcp_server_handler_raw* pHandler, service_mng* mng)
{
	assert(id);
	assert(pHandler);

	Config::section* kv = pHandler->conf_section();
	if (!kv)
	{
		return false;
	}

	m_id = id;
	m_pHandler = pHandler;
	m_mng = mng;

	uint32_t ibuf_len = 0;
	uint32_t obuf_len = 0;
	kv->get(Net::conf_ibuf_len, ibuf_len);
	kv->get(Net::conf_obuf_len, obuf_len);
	bool b = tcp_socket::init(loop, ibuf_len, obuf_len);
	return b;
}

bool	tcp_listen::listen(const char* ip, unsigned short port, uint32_t backlog, uint32_t defer_accept)	//as listen
{
	return tcp_socket::listen(ip, port, backlog, defer_accept);
}

void	tcp_listen::on_accept(int fd, struct sockaddr_in client_addr)
{
	m_mng->on_accept(m_id, fd, client_addr);
}

