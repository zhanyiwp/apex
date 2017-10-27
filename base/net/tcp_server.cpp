#include "tcp_server.h"
#include "service_mng.h"

tcp_server::tcp_server()
{
	m_id = 0;
	m_pHandler = NULL;
	m_mng = NULL;
}

tcp_server::~tcp_server()
{

}

bool tcp_server::init(struct ev_loop* loop, uint64_t id, Net::tcp_server_handler_raw* pHandler, service_mng* mng)
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

bool	tcp_server::attach(int fd, struct sockaddr_in& client_addr)	//as server
{
	//if (!m_mng->is_del(m_id))
	//{
		return tcp_socket::attach(fd, client_addr);
	//}
	//return false;
}

void	tcp_server::on_accepted()	//连接被接受，对应client的on_connect
{
	if (!m_mng->is_del(m_id))
	{
		m_pHandler->on_accepted(this);
	}
}

void	tcp_server::on_closing()
{
	m_pHandler->on_closing(this);
}

int32_t tcp_server::on_recv(const uint8_t* data, uint32_t data_len)
{
	if (!m_mng->is_del(m_id))
	{
		int32_t ret = m_pHandler->on_recv(data, data_len, this);
		if (ret > 0)
		{
			m_mng->update_expired(m_id);

			m_pHandler->on_pkg(data, data_len, this);
		}
		return ret;
	}
	return 0;
}

void tcp_server::do_close()
{
	if (!m_mng->is_del(m_id))
	{
		close();
	}
}

void	tcp_server::delay_delete()
{
	if (!m_mng->is_del(m_id))
	{
		m_mng->del_tcp_channel(m_id);
	}
}

bool tcp_server::try_send(const uint8_t *pkg, const uint32_t len)
{
	if (!m_mng->is_del(m_id))
	{
		return send(pkg, len);
	}
	return false;
}

uint64_t tcp_server::global_id()//进程时空范围内唯一id
{
	if (!m_mng->is_del(m_id))
	{
		return m_id;
	}
	return 0;
}

bool tcp_server::local_addr(struct sockaddr_in& local)
{
	if (!m_mng->is_del(m_id))
	{
		return get_local_addr(local);
	}
	return false;
}

bool tcp_server::remote_addr(struct sockaddr_in& peer)
{
	if (!m_mng->is_del(m_id))
	{
		return get_remote_addr(peer);
	}
	return false;
}

string	tcp_server::get_section()
{
	return m_mng->get_section(m_id);
}

void	tcp_server::start_timer(uint16_t timer_id, uint32_t ms, bool repeat)
{
	if (!m_mng->is_del(m_id))
	{
		start_inner_timer(timer_id, ms, repeat);
	}
}

void	tcp_server::stop_timer(uint16_t timer_id)
{
	if (!m_mng->is_del(m_id))
	{
		stop_inner_timer(timer_id);
	}
}

void	tcp_server::on_inner_timer(uint16_t timer_id)
{
	if (!m_mng->is_del(m_id))
	{
		return m_pHandler->on_timer(timer_id, this);
	}
}

void	tcp_server::set_value(uint32_t key, void* value, size_t size)
{
	if (!m_mng->is_del(m_id))
	{

		string v((char*)value, size);

		set_kv(key, v);
	}
}

bool	tcp_server::get_value(uint32_t key, void** value, size_t size)
{
	if (!m_mng->is_del(m_id))
	{
		string* v = NULL;
		if (get_kv(key, &v) && v->size() != size)
		{
			*value = (void*)v->data();
			return true;
		}
	}

	return false;
}