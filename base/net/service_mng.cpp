#include "string_func.h"
#include "service_mng.h"

#define MAX_TRYTIME_INFAIL	(64)	//最大失败尝试次数，仅仅回绕有此问题

service_mng::service_mng(struct ev_loop* loop) : m_timer_delay_del(loop)
{
	assert(loop);
	m_loop = loop;
	m_id_counter = 0;

	m_timer_delay_del.set(this, &service_mng::on_timer_delay_del, 1000);
}

service_mng::~service_mng()
{
	m_tcp_section.clear();
	m_tcp_server.clear();

	for (map<uint64_t, tcp_channel_desc>::iterator it = m_tcp_channel.begin(); it != m_tcp_channel.end(); it++)
	{
		map<uint64_t, tcp_server*> del_channel_map = it->second.del_channel_map;
		for (map<uint64_t, tcp_server*>::iterator it2 = del_channel_map.begin(); it2 != del_channel_map.end(); it2++)
		{
			if (it->second.del_channel_map.find(it2->first) != it->second.del_channel_map.end())
			{
				it->second.del_channel_map.erase(it2->first);
				it2->second->close();
				delete it2->second;
			}
		}
		map<uint64_t, tcp_server*> alive_channel_map = it->second.alive_channel_map;
		for (map<uint64_t, tcp_server*>::iterator it2 = alive_channel_map.begin(); it2 != alive_channel_map.end(); it2++)
		{
			if (it->second.alive_channel_map.find(it2->first) != it->second.alive_channel_map.end())
			{
				it->second.alive_channel_map.erase(it2->first);
				it2->second->close();
				delete it2->second;
			}
		}

		it->second.tcp_listen_ptr->close();
		delete it->second.tcp_listen_ptr;

		delete it->second.timer_check_expired;
	}
	m_tcp_channel.clear();
}

uint64_t	service_mng::alloc_new_id()
{
	for (uint32_t i = 0; i < MAX_TRYTIME_INFAIL; i++)
	{
		m_id_counter++;
		/*
		64位可以用到地球爆炸
		if (m_tcp_raw.find(m_id_counter) != m_tcp_raw.end())
		{
		continue;
		}

		if (m_tcp_outerhello.find(m_id_counter) != m_tcp_outerhello.end())
		{
		continue;
		}

		if (m_tcp_inner.find(m_id_counter) != m_tcp_inner.end())
		{
		continue;
		}

		if (m_tcp_hello.find(m_id_counter) != m_tcp_hello.end())
		{
		continue;
		}

		if (m_tcp_login.find(m_id_counter) != m_tcp_login.end())
		{
		continue;
		}
		*/
		return m_id_counter;
	}

	return 0;
}

bool service_mng::add_tcp_listen(Net::tcp_server_handler_raw* pHandler)
{
	Config::section* kv = pHandler->conf_section();
	if (!kv)
	{
		return false;
	}
	
	string sectionLow = lowstring(kv->get_name());
	if (m_tcp_section.find(sectionLow) != m_tcp_section.end())
	{
		return false;
	}
	uint32_t expired = 0;
	kv->get(Net::conf_expired, expired);
	if (!expired)
	{
		return false;
	}

	uint64_t	id = alloc_new_id();
	if (!id)
	{
		return false;
	}

	tcp_listen* p = new tcp_listen;
	if (!p)
	{
		return false;
	}

	if (!p->init(m_loop, id, pHandler, this))
	{
		delete p;
		return false;
	}

	tcp_channel_desc desc;
	desc.tcp_listen_ptr = p;
	desc.raw_handler_ptr = pHandler;
	desc.expired = expired;
	desc.timer_check_expired = NULL;
	desc.section_name = sectionLow;
	m_tcp_channel[id] = desc;

	m_tcp_section[sectionLow] = id;

	string ip;
	uint16_t port = 0;
	kv->get(Net::conf_listen_ip, ip);
	kv->get(Net::conf_listen_port, port);

	if (!p->listen(ip.c_str(), port, 0, 0))
	{
		map<uint64_t, tcp_channel_desc>::iterator it = m_tcp_channel.find(id);
		if (it != m_tcp_channel.end())
		{
			delete it->second.timer_check_expired;
			it->second.tcp_listen_ptr->close(false);
			delete it->second.tcp_listen_ptr;
			
			m_tcp_section.erase(it->second.section_name);
			m_tcp_channel.erase(id);	
		}

		return false;
	}

	return true;
}

void service_mng::del_tcp_channel(uint64_t con_id)
{
	map<uint64_t, uint64_t>::iterator it = m_tcp_server.find(con_id);
	if (it == m_tcp_server.end())
	{
		return;
	}

	map<uint64_t, tcp_channel_desc>::iterator it2 = m_tcp_channel.find(it->first);
	if (it2 == m_tcp_channel.end())
	{
		return;
	}

	map<uint64_t, tcp_server*>::iterator it3 = it2->second.alive_channel_map.find(con_id);
	if (it3 == it2->second.alive_channel_map.end())
	{
		return;
	}

	map<uint64_t, tcp_server*>::iterator it4 = it2->second.del_channel_map.find(con_id);
	if (it4 != it2->second.del_channel_map.end())
	{
		return;
	}
	it2->second.del_channel_map[con_id] = it3->second;
	it2->second.alive_channel_map.erase(con_id);
	it2->second.alive_expired_map.erase(con_id);

	if (!m_timer_delay_del.is_active())
	{
		m_timer_delay_del.start();
	}
}

void	service_mng::on_accept(uint64_t listen_con_id, int new_fd, struct sockaddr_in client_addr)
{
	map<uint64_t, tcp_channel_desc>::iterator it = m_tcp_channel.find(listen_con_id);
	if (it == m_tcp_channel.end())
	{
		return;
	}

	uint64_t	accept_id = alloc_new_id();
	if (!accept_id)
	{
		return;
	}

	if (!it->second.timer_check_expired)
	{
		it->second.timer_check_expired = new timer(m_loop);
		if (!it->second.timer_check_expired)
		{
			return;
		}
		it->second.timer_check_expired->set(this, &service_mng::on_timer_check_expired, it->second.expired, true,(void*)listen_con_id);
	}
	if (!it->second.timer_check_expired->is_active())
	{
		it->second.timer_check_expired->start();
	}

	tcp_server* p = new tcp_server;
	if (!p)
	{
		return;
	}

	if (!p->init(m_loop, accept_id, it->second.raw_handler_ptr, this))
	{
		delete p;
		return;
	}

	if (!p->attach(new_fd, client_addr))
	{
		delete p;
		return;
	}
	
	m_tcp_server[accept_id] = it->first;
	it->second.alive_channel_map[accept_id] = p;
	it->second.alive_expired_map[accept_id] = Time::now();

	it->second.raw_handler_ptr->on_accepted(p);
}

bool service_mng::is_del(uint64_t id)
{
	map<uint64_t, uint64_t>::iterator it = m_tcp_server.find(id);
	if (it == m_tcp_server.end())
	{
		return true;
	}

	map<uint64_t, tcp_channel_desc>::iterator it2 = m_tcp_channel.find(it->second);
	if (it2 == m_tcp_channel.end())
	{
		return true;
	}

	map<uint64_t, tcp_server*>::iterator it3 = it2->second.alive_channel_map.find(id);
	if (it3 == it2->second.alive_channel_map.end())
	{
		return true;
	}

	return false;
}

string service_mng::get_section(uint64_t con_id)
{
	map<uint64_t, uint64_t>::iterator it = m_tcp_server.find(con_id);
	if (it == m_tcp_server.end())
	{
		return "";
	}

	map<uint64_t, tcp_channel_desc>::iterator it2 = m_tcp_channel.find(it->second);
	if (it2 == m_tcp_channel.end())
	{
		return "";
	}

	return it2->second.section_name;
}

void	service_mng::update_expired(uint64_t id)
{
	map<uint64_t, uint64_t>::iterator it = m_tcp_server.find(id);
	if (it == m_tcp_server.end())
	{
		return;
	}

	map<uint64_t, tcp_channel_desc>::iterator it2 = m_tcp_channel.find(it->second);
	if (it2 == m_tcp_channel.end())
	{
		return;
	}

	map<uint64_t, uint32_t>::iterator it3 = it2->second.alive_expired_map.find(id);
	if (it3 == it2->second.alive_expired_map.end())
	{
		return;
	}

	it3->second = Time::now();
}

void	service_mng::on_timer_delay_del(timer* timer)
{
	m_timer_delay_del.stop();

	for (map<uint64_t, tcp_channel_desc>::iterator it = m_tcp_channel.begin(); it != m_tcp_channel.end(); it++)
	{
		map<uint64_t, tcp_server*> del_channel_map = it->second.del_channel_map;
		for (map<uint64_t, tcp_server*>::iterator it2 = del_channel_map.begin(); it2 != del_channel_map.end(); it2++)
		{
			if (it->second.del_channel_map.find(it2->first) != it->second.del_channel_map.end())
			{
				tcp_server* p = it2->second;

				m_tcp_server.erase(it2->first);
				it->second.alive_expired_map.erase(it2->first);
				it->second.del_channel_map.erase(it2->first);

				p->close_gracefully();
				delete p;
			}
		}
	}
}

void	service_mng::on_timer_check_expired(timer* timer)
{
	void* data = timer->get_arg();
	assert(data);
	uint64_t id = (uint64_t)data;

	map<uint64_t, tcp_channel_desc>::iterator it = m_tcp_channel.find(id);
	if (it == m_tcp_channel.end())
	{
		return;
	}

	uint32_t now = Time::now();
	map<uint64_t, tcp_server*> alive_channel_map = it->second.alive_channel_map;		
	for (map<uint64_t, tcp_server*>::iterator it2 = alive_channel_map.begin(); it2 != alive_channel_map.end(); it2++)
	{
		if (it->second.alive_channel_map.find(it2->first) != it->second.alive_channel_map.end())
		{
			map<uint64_t, uint32_t>::iterator it3 = it->second.alive_expired_map.find(it2->first);
			assert(it3 != it->second.alive_expired_map.end());
			if (it3->second + it->second.expired < now)
			{
				it->second.raw_handler_ptr->on_expired(now - it3->second, it2->second);
			}
		}
	}
}

Net::tcp_server_channel* service_mng::find_tcp_channel(const string section, uint64_t id)
{
	string sectionLow = lowstring(section);

	map<string, uint64_t>::iterator it = m_tcp_section.find(sectionLow);
	if (it == m_tcp_section.end())
	{
		return NULL;
	}

	map<uint64_t, tcp_channel_desc>::iterator it2 = m_tcp_channel.find(it->second);
	assert(it2 != m_tcp_channel.end());

	map<uint64_t, tcp_server*>::iterator it3 = it2->second.alive_channel_map.find(id);
	if (it3 == it2->second.alive_channel_map.end())
	{
		return NULL;
	}

	return it3->second;
}

Net::tcp_server_channel* service_mng::enum_tcp_channel(const string section, uint64_t& cursor)
{
	string sectionLow = lowstring(section);
	map<string, uint64_t>::iterator it = m_tcp_section.find(sectionLow);
	if (it == m_tcp_section.end())
	{
		return NULL;
	}

	map<uint64_t, tcp_channel_desc>::iterator it2 =	m_tcp_channel.find(it->second);
	if (it2 == m_tcp_channel.end())
	{
		return NULL;
	}

	if (it2->second.alive_channel_map.find(cursor) == it2->second.alive_channel_map.end())
	{
		cursor = 0;
	}
	if (!cursor)
	{
		if (it2->second.alive_channel_map.size())
		{
			cursor = it2->second.alive_channel_map.begin()->first;
		}
	}
	if (!cursor)
	{
		return NULL;
	}

	Net::tcp_server_channel* refer = NULL;

	map<uint64_t, tcp_server*>::iterator it3 = it2->second.alive_channel_map.find(cursor);
	if (it3 != it2->second.alive_channel_map.end())
	{
		refer = it3->second;	
		it3++;
	}

	if (it3 != it2->second.alive_channel_map.end())
	{
		cursor = it3->first;
	}
	else
	{
		cursor = 0;
	}

	return refer;
}
