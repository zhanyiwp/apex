#ifndef TCP_SERVER_H_
#define TCP_SERVER_H_

#include <string>
#include <map>
#include "tcp_socket.h"

using namespace std;

class service_mng;

class tcp_server : public tcp_socket, public Net::tcp_server_channel
{
public:
	tcp_server();
	virtual ~tcp_server();

	bool init(struct ev_loop* loop, uint64_t id, Net::tcp_server_handler_raw* pHandler, service_mng* mng);

	virtual bool		attach(int fd, struct sockaddr_in& client_addr);	//as server

	virtual void		on_accepted();	//连接被接受，对应client的on_connect
	virtual void		on_closing();
	virtual int32_t		on_recv(const uint8_t* data, uint32_t data_len);
	virtual void		on_inner_timer(uint16_t timer_id);

	virtual void		do_close();
	virtual void		delay_delete();
	virtual bool		try_send(const uint8_t *pkg, const uint32_t len);
	virtual uint64_t	global_id();//进程时空范围内唯一id
	virtual void		start_timer(uint16_t timer_id, uint32_t ms, bool repeat);
	virtual void		stop_timer(uint16_t timer_id);
	virtual string		get_section();
	virtual bool		local_addr(struct sockaddr_in& local);
	virtual bool		remote_addr(struct sockaddr_in& peer);

	virtual void		set_value(uint32_t key, void* value, size_t size);
	virtual bool		get_value(uint32_t key, void** value, size_t size);

private:
	uint64_t m_id;
	Net::tcp_server_handler_raw* m_pHandler;
	service_mng* m_mng;
};

#endif // TCP_SERVER_H_
