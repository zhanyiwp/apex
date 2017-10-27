#include <iostream>
#include "tcp_client.h"
#include "client_mng.h"

using namespace std;

tcp_client::tcp_client()
{
	m_id = 0;
	m_pHandler = NULL;
	m_mng = NULL;
	m_port = 0;
}

tcp_client::~tcp_client()
{

}

bool tcp_client::init(struct ev_loop* loop, uint64_t id, Net::tcp_client_handler_raw* pHandler, client_mng* mng)
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
	kv->get(Net::conf_server_ip, m_ip);
	kv->get(Net::conf_server_port, m_port);

	bool b = tcp_socket::init(loop, ibuf_len, obuf_len);
	return b;
}

void tcp_client::on_connect()
{
	if (!m_mng->is_del(m_id))
	{
		m_mng->on_connect(m_id);
		m_pHandler->on_connect(this);
	}
}

void tcp_client::on_closing()
{
	m_mng->on_closing(m_id);
	return m_pHandler->on_closing(this);
}

void tcp_client::on_error(ERROR_CODE err)
{
	if (!m_mng->is_del(m_id))
	{
		return m_pHandler->on_error(err, this);
	}
}

int32_t tcp_client::on_recv(const uint8_t* pkg, uint32_t len)
{
	if (!m_mng->is_del(m_id))
	{
		int32_t ret = m_pHandler->on_recv(pkg, len, this);
		if (ret)
		{
			m_pHandler->on_pkg(pkg, len, this);
		}
		return ret;
	}
	return 0;
}

bool tcp_client::try_connect()
{
	if (!m_mng->is_del(m_id))
	{
		return connect(m_ip.c_str(), m_port);
	}
	return false;
}

void tcp_client::do_close()
{
	if (!m_mng->is_del(m_id))
	{
		close();
	}
}

void	tcp_client::delay_delete()
{
	if (!m_mng->is_del(m_id))
	{
		m_mng->del_tcp(m_id);
	}
}

bool tcp_client::try_send(const uint8_t *pkg, const uint32_t len)
{
	if (!m_mng->is_del(m_id))
	{
		return send(pkg, len);
	}

	return false;
}

uint64_t tcp_client::global_id()//进程时空范围内唯一id
{
	if (!m_mng->is_del(m_id))
	{
		return m_id;
	}
	return 0;
}

bool tcp_client::local_addr(struct sockaddr_in& local)
{
	if (!m_mng->is_del(m_id))
	{
		return get_local_addr(local);
	}
	return false;
}

bool tcp_client::remote_addr(struct sockaddr_in& peer)
{
	if (!m_mng->is_del(m_id))
	{
		return get_remote_addr(peer);
	}
	return false;
}

string	tcp_client::get_section()
{
	return m_mng->get_section(m_id);
}

void	tcp_client::start_timer(uint16_t timer_id, uint32_t ms, bool repeat)
{
	if (!m_mng->is_del(m_id))
	{
		start_inner_timer(timer_id, ms, repeat);
	}
}

void	tcp_client::stop_timer(uint16_t timer_id)
{
	if (!m_mng->is_del(m_id))
	{
		stop_inner_timer(timer_id);
	}
}

void	tcp_client::on_inner_timer(uint16_t timer_id)
{
	if (!m_mng->is_del(m_id))
	{
		return m_pHandler->on_timer(timer_id, this);
	}
}

void	tcp_client::set_value(uint32_t key, void* value, size_t size)
{
	if (!m_mng->is_del(m_id))
	{
		string v((char*)value, size);
		set_kv(key, v);
	}
}

bool	tcp_client::get_value(uint32_t key, void** value, size_t size)
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