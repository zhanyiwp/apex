#ifndef TCP_LISTEN_H_
#define TCP_LISTEN_H_

#include "tcp_socket.h"

class service_mng;

class tcp_listen : public tcp_socket
{
public:
	tcp_listen();
	virtual ~tcp_listen();

	bool init(struct ev_loop* loop, uint64_t id, Net::tcp_server_handler_raw* pHandler, service_mng* mng);

	virtual bool	listen(const char* ip, unsigned short port, uint32_t backlog, uint32_t defer_accept);	//as listen

	virtual void	on_accept(int fd, struct sockaddr_in client_addr);
	
private:
	uint64_t	m_id;
	Net::tcp_server_handler_raw* m_pHandler;
	service_mng* m_mng;
};

#endif // TCP_LISTEN_H_
