#include "tcp_socket.h"

#define DEFAULT_LISTEN_BACKLOG 20
#define MAX_DEFER_ACCEPT 2

static void tcp_sock_on_connect(struct ev_loop *, ev_io *watcher, int events)
{
	tcp_socket* sock = tcp_socket::get_instance_via_connect_watcher(watcher);
	sock->inner_ev_connect(events);
}

static void tcp_sock_on_accept(struct ev_loop *, ev_io *watcher, int events)
{
	tcp_socket* sock = tcp_socket::get_instance_via_accept_watcher(watcher);
	sock->inner_ev_accept(events);
}

static void tcp_sock_on_write(struct ev_loop *, ev_io *watcher, int events)
{
	tcp_socket* sock = tcp_socket::get_instance_via_write_watcher(watcher);
	sock->inner_ev_write(events);
}

static void tcp_sock_on_read(struct ev_loop *, ev_io *watcher, int events)
{
	tcp_socket* sock = tcp_socket::get_instance_via_read_watcher(watcher);
	sock->inner_ev_read(events);
}

tcp_socket::tcp_socket()
{
	_s = INVALID_SOCKET;
	_oev_connnect_started = false;
	_oev_write_started = false;
	_iev_accept_started = false;
	_iev_read_started = false;
	_loop = NULL;
	_ibuf = NULL;
	_obuf = NULL;
	_ibuf_len = 0;
	_obuf_len = 0;
	_socket_type = st_none;
	_closing = false;
}

tcp_socket::~tcp_socket()
{
	stop_watch_connect();
	stop_watch_accept();
	stop_watch_write();
	stop_watch_read();

	for(map<uint16_t, timer*>::iterator it = _timer_map.begin(); it != _timer_map.end(); it++)
	{		
		it->second->stop();
		delete it->second;
	}

	if (_ibuf)
	{
		delete[]_ibuf;
	}
	if (_obuf)
	{
		delete[]_obuf;
	}
}

bool	tcp_socket::init(struct ev_loop* loop, uint32_t ibuflen, uint32_t obuflen)
{
	if (st_none != _socket_type)
	{
		assert(false);
		return false;
	}
	if (INVALID_SOCKET != _s)
	{
		assert(false);
		return false;
	}

	assert(!_ibuf);
	assert(!_obuf);

	_loop = loop;

	_ibuf = new uint8_t[ibuflen];
	_obuf = new uint8_t[obuflen];

	_ibuf_len = ibuflen;
	_obuf_len = obuflen;

	_ibuf_pos = 0;
	_obuf_pos = 0;

	return true;
}

bool	tcp_socket::connect(const char* ip, unsigned short port)	//as client
{
	if (INVALID_SOCKET != _s)
	{
		return false;
	}
	if (st_none == _socket_type)
	{
		_socket_type = st_client;
	}
	if (st_client != _socket_type)
	{
		return false;
	}

	if (SOCKET_FAIL(set_sock_addr(&_s_in_, ip, port)))
	{
		return false;
	}

	_s = create_tcp_sock();
	if (INVALID_SOCKET == _s)
	{
		return false;
	}
	
	if (SOCKET_FAIL(set_nonblock_sock(_s, 1)))
	{
		return false;
	}

	if (SOCKET_FAIL(connect_sock(_s, &_s_in_)))
	{
		if (errno == EINPROGRESS)
		{
			ev_io_init(&_oev_connnect, tcp_sock_on_connect, _s, EV_WRITE);
			start_watch_connect();
			
			return true;
		}
	}
	else
	{
		on_connect();

		return true;
	}	

	close(false);
	on_error(ERR_NET_CONNECT);//给外部一个清理或reconnect的机会，形成闭环
	return false;
}

void tcp_socket::inner_ev_connect(int events)
{
	if(!(events & EV_ERROR))
	{
		if(INVALID_SOCKET != _s)
		{
			if( SOCKET_SUCCESS(is_sock_connected(_s)) )
			{
				stop_watch_connect();

				ev_io_init(&_iev_read, tcp_sock_on_read, _s, EV_READ);
				ev_io_init(&_oev_write, tcp_sock_on_write, _s, EV_WRITE);
				start_watch_write();
				start_watch_read();

				on_connect();
				return;
			}
		}
	}
	close();
	on_error(ERR_NET_CONNECT);
}

void	tcp_socket::on_connect()
{

}

void	tcp_socket::on_closing()
{

}

void	tcp_socket::on_error(ERROR_CODE err)
{

}

bool	tcp_socket::listen(const char* ip, unsigned short port, uint32_t backlog, uint32_t defer_accept)	//as server
{
	if (INVALID_SOCKET != _s)
	{
		return true;
	}
	if (st_none == _socket_type)
	{
		_socket_type = st_listen;
	}
	if (st_listen != _socket_type)
	{
		return false;
	}

	if (SOCKET_FAIL(set_sock_addr(&_s_in_, ip, port)))
	{
		return false;
	}

	if (defer_accept > MAX_DEFER_ACCEPT)
	{
		defer_accept = MAX_DEFER_ACCEPT;
	}
	_s = create_artery_listen_sock(1, 0, 1, defer_accept);
	if (INVALID_SOCKET == _s)
	{
		return false;
	}
	if (SOCKET_FAIL(bind_sock(_s, &_s_in_)))
	{
		close(false);
		return false;
	}

	if (0 == backlog)
	{
		backlog = DEFAULT_LISTEN_BACKLOG;
	}
	if (SOCKET_FAIL(listen_sock(_s, DEFAULT_LISTEN_BACKLOG)))
	{
		close();
		return false;
	}

	ev_io_init(&_iev_accept, tcp_sock_on_accept, _s, EV_READ);
	start_watch_accept();

	return true;
}

void tcp_socket::inner_ev_accept(int events)
{
	if (!(events & EV_ERROR))
	{
		do
		{
			struct sockaddr_in client_addr;
			int fd = accept_sock(_s, &client_addr);
			if (fd >= 0)
			{
				if (SOCKET_FAIL(set_nonblock_sock(fd, 1)))
				{
					::close(fd);
				}
				else
				{
					on_accept(fd, client_addr);
				}
			}
			else
			{
				break;
			}
		} while (true);

		return;
	}

	on_error(ERR_NET_ACCEPT);
}

bool	tcp_socket::attach(int fd, struct sockaddr_in& client_addr)
{
	//must init first
	assert(_loop);
	assert(INVALID_SOCKET != fd);

	if (INVALID_SOCKET != _s)
	{
		return false;
	}
	if (st_none != _socket_type)
	{
		return false;
	}

	_socket_type = st_server;

	_s = fd;
	_s_in_ = client_addr;

	ev_io_init(&_iev_read, tcp_sock_on_read, _s, EV_READ);
	ev_io_init(&_oev_write, tcp_sock_on_write, _s, EV_WRITE);
	start_watch_write();
	start_watch_read();

	return true;
}

bool	tcp_socket::send(const uint8_t* data, uint32_t data_len)
{
	if (0 == data_len)
	{
		return true;
	}

	if (_obuf_len < data_len + _obuf_pos)
	{
		if (!do_send())
		{
			return false;
		}

		if (_obuf_len < data_len + _obuf_pos)
		{
			on_error(ERR_NET_SENDSPACE);

			return false;
		}
	}

	ssize_t real_write = 0;
	if (0 == _obuf_pos)//no data in out buf
	{
		ssize_t iret = ::send(_s, reinterpret_cast<const void*>(data), data_len, MSG_NOSIGNAL);
		if (iret < 0)
		{
#if EAGAIN == EWOULDBLOCK
			if (errno != EAGAIN)
#else
			if (errno != EAGAIN && errno != EWOULDBLOCK)
#endif
			{
				assert(errno != EFAULT);
				return false;
			}
		}
// 		else if (iret == 0)
// 		{
// 			close();
// 			return;
// 		}
		else
		{
			real_write = iret;
		}
	}
		
	memcpy(_obuf + _obuf_pos, data + real_write, data_len - real_write);
	_obuf_pos += data_len - real_write;

	return do_send();
}

void tcp_socket::inner_ev_write(int events)
{
	if (!(events & EV_ERROR))
	{
		if (do_send())
		{
			return;
		}
	}

	close();
	//on_error(ERR_NET_SEND);
}

bool tcp_socket::do_send()
{
	ssize_t real_write = 0;
	ssize_t iret = ::send(_s, _obuf, _obuf_pos, MSG_NOSIGNAL);
	if (iret<0)
	{
#if EAGAIN == EWOULDBLOCK
		if (errno != EAGAIN)
#else
		if (errno != EAGAIN && errno != EWOULDBLOCK)
#endif
		{
			assert(errno != EFAULT);
			on_error(ERR_NET_SEND);
			return false;
		}
	}
// 	else if (iret == 0)
// 	{
// 		close();
// 		return false;
// 	}
	else
	{
		real_write = iret;
	}

	memmove(_obuf, _obuf + real_write, _obuf_pos - real_write);
	_obuf_pos -= real_write;

	return true;
}

void tcp_socket::inner_ev_read(int events)
{
	if (!(events & EV_ERROR))
	{
		if (_ibuf_len > _ibuf_pos)
		{
			ssize_t iret = ::recv(_s, _ibuf + _ibuf_pos, _ibuf_len - _ibuf_pos, 0);
			if (iret > 0)
			{
				_ibuf_pos += (uint32_t)iret;

				uint32_t size = 0;
				while (true)
				{
					int32_t ir = on_recv(_ibuf + size, _ibuf_pos - size);
					if (ir == 0)
					{
						break;
					}
					else if (ir < 0)
					{
						on_error(ERR_NET_RECV);
						return;
					}

					size += ir;
				}
				if (size)
				{
					memmove(_ibuf, _ibuf + _ibuf_pos, _ibuf_pos - size);
					_ibuf_pos -= size;
				}
			}
			else if (iret < 0)
			{
#if EAGAIN == EWOULDBLOCK
				if (errno != EAGAIN && errno != EINTR)
#else
				if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
#endif
				{
					on_error(ERR_NET_RECV);
				}
			}
			else
			{
				close();
			}
		}
		else
		{
			on_error(ERR_NET_RECVSPACE);
		}
	}
	else
	{
		close();
	}
}

void tcp_socket::close(bool triggle_closing/* = true*/)
{
	if (_closing)
	{
		return;
	}
	if (_s != INVALID_SOCKET)
	{
		if (triggle_closing)
		{
			on_closing();
		}

		stop_watch_connect();
		stop_watch_accept();
		stop_watch_write();
		stop_watch_read();

		//set_linger_sock(_s, 1, 0);
		SOCKET_RESULT ret = close_sock(_s);
		if (SOCKET_FAIL(ret))
			//TODO svr_log("********WARNING: close failed, sock_fd=" << _s << ", errno=" << errno << ", strerror=" << strerror(errno));
			;
		_s = INVALID_SOCKET;
		_socket_type = st_none;
	}
	_closing = false;
}

/**
* close this client gracefully using shutdown(RDWR). fd will be set to invalid too.
* do not call this function if you are using svr_tcp_sock, you should call svr_tcp_session::close_gracefully() instead.
*/
SOCKET tcp_socket::close_gracefully(bool triggle_closing/* = true*/)
{
	SOCKET ret = INVALID_SOCKET;
	if (_closing)
	{
		return ret;
	}

	if (_s != INVALID_SOCKET)
	{
		if (triggle_closing)
		{
			on_closing();
		}

		stop_watch_connect();
		stop_watch_accept();
		stop_watch_write();
		stop_watch_read();

		shutdown();
		ret = _s;
		_s = INVALID_SOCKET;
		_socket_type = st_none;
	}
	_closing = false;
	return ret;
}



void	tcp_socket::on_accept(int fd, struct sockaddr_in client_addr)
{
	assert(false);
}

int32_t tcp_socket::on_recv(const uint8_t* data, uint32_t data_len)
{
	assert(false);
	return 0;
}

void	tcp_socket::start_inner_timer(uint16_t timer_id, uint32_t ms, bool repeat)
{
	map<uint16_t, timer*>::iterator it = _timer_map.find(timer_id);
	if (it == _timer_map.end())
	{
		timer* t = new timer(_loop);
		if (!t)
		{
			return;
		}

		_timer_map[timer_id] = t;
		it = _timer_map.find(timer_id);;
		t->set(this, &tcp_socket::on_timer_cb, ms, repeat, (void**)timer_id);
	}
	it->second->start();
}

void	tcp_socket::stop_inner_timer(uint16_t timer_id)
{
	map<uint16_t, timer*>::iterator it = _timer_map.find(timer_id);
	if (it != _timer_map.end())
	{
		timer* t = it->second;
		t->stop();
	}
}

void	tcp_socket::on_timer_cb(timer* t)
{
	uint16_t id = (uint16_t)(uint64_t)t->get_arg();
	map<uint16_t, timer*>::iterator it = _timer_map.find(id);
	if (it != _timer_map.end())
	{
		on_inner_timer(id);
	}
}

void	tcp_socket::on_inner_timer(uint16_t timer_id)
{
}

bool tcp_socket::get_local_addr(struct sockaddr_in& local)
{
	if (_s != INVALID_SOCKET)
	{
		return SOCKET_SUCCESS(get_local_sock_addr(_s, &local));
	}
	return false;
}

bool tcp_socket::get_remote_addr(struct sockaddr_in& peer)
{
	if (_s != INVALID_SOCKET)
	{
		return SOCKET_SUCCESS(get_peer_sock_addr(_s, &peer));
	}
	return false;
}

bool	tcp_socket::get_kv(uint32_t key, string** value)
{
	map<uint32_t, string>::iterator it = _slot_map.find(key);
	if (it == _slot_map.end())
	{
		return false;
	}

	*value = &it->second;
	return true;
}

void	tcp_socket::set_kv(uint32_t key, const string& value)
{
	map<uint32_t, string>::iterator it = _slot_map.find(key);
	if (it == _slot_map.end())
	{		
		_slot_map[key] = value;	
	}
}