#ifndef TCP_CLIENT_H_
#define TCP_CLIENT_H_

#include <stdint.h>
#include <map>
#include "tcp_socket.h"

using namespace std;

class client_mng;

class tcp_client : public tcp_socket, public Net::tcp_client_channel
{
public:
	tcp_client();
	virtual ~tcp_client();

	bool init(struct ev_loop* loop, uint64_t id, Net::tcp_client_handler_raw* pHandler, client_mng* mng);

	virtual void		on_connect();
	virtual void		on_closing();
	virtual void		on_error(ERROR_CODE err);
	virtual int32_t		on_recv(const uint8_t* pkg, uint32_t len); 
	virtual void		on_inner_timer(uint16_t timer_id);

	virtual bool		try_connect();
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

protected:
	inline tcp_client& operator = (const tcp_client& u){ return *this; }

private:
	uint64_t m_id;
	Net::tcp_client_handler_raw* m_pHandler;
	client_mng* m_mng;
	
	string m_ip;
	uint16_t	m_port;
};

#endif // TCP_CLIENT_H_
