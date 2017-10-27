#ifndef TCP_SOCKET_H_
#define TCP_SOCKET_H_

#include "ev.h"
#include <map>
#include <assert.h>
#include "socket_func.h"
#include "period_timer.h"
#include "net.h"

using namespace std;

class tcp_socket
{
public:
	//inner
	static inline tcp_socket* get_instance_via_connect_watcher(ev_io* ev) { return reinterpret_cast<tcp_socket*>(reinterpret_cast<uint8_t*>(ev)-member_offsetof(tcp_socket, _oev_connnect)); }
	static inline tcp_socket* get_instance_via_write_watcher(ev_io* ev) { return reinterpret_cast<tcp_socket*>(reinterpret_cast<uint8_t*>(ev)-member_offsetof(tcp_socket, _oev_write)); }
	static inline tcp_socket* get_instance_via_accept_watcher(ev_io* ev) { return reinterpret_cast<tcp_socket*>(reinterpret_cast<uint8_t*>(ev)-member_offsetof(tcp_socket, _iev_accept)); }
	static inline tcp_socket* get_instance_via_read_watcher(ev_io* ev) { return reinterpret_cast<tcp_socket*>(reinterpret_cast<uint8_t*>(ev)-member_offsetof(tcp_socket, _iev_read)); }
	void inner_ev_connect(int events);
	void inner_ev_accept(int events);
	void inner_ev_write(int events);
	void inner_ev_read(int events);

	//interface
	tcp_socket();
	virtual ~tcp_socket();

	bool	init(struct ev_loop* loop, uint32_t ibuflen, uint32_t obuflen);
	
	/**
	* close this client forcefully, stop the io event watcher
	* NOTE: do not call this function if you are using svr_tcp_sock, you should call svr_tcp_session::close() instead.
	*/
	void	close(bool triggle_closing = true);

	/**
	* close this client gracefully using shutdown(RDWR). fd will be set to invalid too.
	* do not call this function if you are using svr_tcp_sock, you should call svr_tcp_session::close_gracefully() instead.
	*/
	SOCKET	close_gracefully(bool triggle_closing = true);

protected:
	virtual bool	connect(const char* ip, unsigned short port);	//as client
	virtual bool	listen(const char* ip, unsigned short port, uint32_t backlog, uint32_t defer_accept);	//as listen
	virtual bool	attach(int fd, struct sockaddr_in& client_addr);	//as server
	virtual bool	send(const uint8_t* data, uint32_t data_len);

	virtual void	on_connect();
	virtual void	on_closing();
	virtual void	on_error(ERROR_CODE err);
	virtual void	on_accept(int fd, struct sockaddr_in client_addr);
	virtual int32_t on_recv(const uint8_t* data, uint32_t data_len);

	void			start_inner_timer(uint16_t timer_id, uint32_t ms, bool repeat);
	void			stop_inner_timer(uint16_t timer_id);
	virtual void	on_inner_timer(uint16_t timer_id);

	bool			get_local_addr(struct sockaddr_in& local);
	bool			get_remote_addr(struct sockaddr_in& peer);

	bool			get_kv(uint32_t key, string** value);
	void			set_kv(uint32_t key, const string& value);
private:
	SOCKET _s;
	struct sockaddr_in _s_in_;
	uint8_t * _ibuf;
	uint32_t _ibuf_len;
	uint32_t _ibuf_pos;
	uint8_t * _obuf;
	uint32_t _obuf_len;
	uint32_t _obuf_pos;
	struct ev_loop* _loop;

	enum socket_type
	{
		st_none = 0,
		st_listen = 1,//listen server
		st_server = 2,//server side channel
		st_client = 3,//client side channel
	}_socket_type;

	//start/stop watching read/write event
	inline void start_watch_connect() { if (!_oev_connnect_started) { ev_io_start(_loop, &_oev_connnect); _oev_connnect_started = true; } }
	inline void stop_watch_connect() { if (_oev_connnect_started) { ev_io_stop(_loop, &_oev_connnect); _oev_connnect_started = false; } }
	inline void start_watch_write() { if (!_oev_write_started) { ev_io_start(_loop, &_oev_write); _oev_write_started = true; } }
	inline void stop_watch_write() { if (_oev_write_started) { ev_io_stop(_loop, &_oev_write); _oev_write_started = false; } }

	inline void start_watch_accept() { if (!_iev_accept_started) { ev_io_start(_loop, &_iev_accept); _iev_accept_started = true; } }
	inline void stop_watch_accept() { if (_iev_accept_started) { ev_io_stop(_loop, &_iev_accept); _iev_accept_started = false; } }
	inline void start_watch_read() { if (!_iev_read_started) { ev_io_start(_loop, &_iev_read); _iev_read_started = true; } }
	inline void stop_watch_read() { if (_iev_read_started) { ev_io_stop(_loop, &_iev_read); _iev_read_started = false; } }

	ev_io _oev_connnect;
	ev_io _oev_write;
	ev_io _iev_accept;
	ev_io _iev_read;
	bool _oev_connnect_started;
	bool _oev_write_started;
	bool _iev_accept_started;
	bool _iev_read_started;

	//aux
	inline void shutdown() { shutdown_sock(_s, SHUT_RDWR); }
	inline void shutdown_read() { shutdown_sock(_s, SHUT_RD); }
	inline void shutdown_write() { shutdown_sock(_s, SHUT_WR); }
	
	bool do_send();
	void on_timer_cb(timer* t);

	bool _closing;

	map<uint16_t, timer*>	_timer_map;
	map<uint32_t, string>	_slot_map;
};

#endif // TCP_SOCKET_H_
