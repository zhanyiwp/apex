#include "string_func.h"
#include "client_mng.h"

#define MAX_TRYTIME_INFAIL	(64)	//最大失败尝试次数，仅仅回绕有此问题

client_mng::client_mng(struct ev_loop* loop) : m_timer_delay_del(loop)
{
	assert(loop);
	m_loop = loop;
	m_id_counter = 0;

	m_timer_delay_del.set(this, &client_mng::on_timer_delay_del, 1000);
}

client_mng::~client_mng()
{	
	m_tcp_section.clear();
	m_tcp_addr.clear();

	map<uint64_t, tcp_channel_desc> tcp_channel = m_tcp_channel;
	for (map<uint64_t, tcp_channel_desc>::iterator it = tcp_channel.begin(); it != tcp_channel.end(); it++)
	{
		if (m_tcp_channel.find(it->first) != m_tcp_channel.end())
		{
			//避免某channel的close触发其他channel的erase导致的崩溃
			m_tcp_channel.erase(it->first);

			it->second.raw_channel_ptr->close();
			delete it->second.raw_channel_ptr;
		}
	}
}

uint64_t	client_mng::alloc_new_id()
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

bool client_mng::add_tcp(Net::tcp_client_handler_raw* pHandler)
{
	Config::section* kv = pHandler->conf_section();
	if (!kv)
	{
		return false;
	}

	string sectionLow = lowstring(kv->get_name());
	map<string, uint64_t>::iterator it = m_tcp_section.find(sectionLow);
	if (it != m_tcp_section.end())
	{
		return false;
	}

	uint64_t	id = alloc_new_id();
	if (!id)
	{
		return false;
	}

	tcp_client* p = new tcp_client;
	if (!p)
	{
		return false;
	}
	if (!p->init(m_loop, id, pHandler, this))
	{
		return false;
	}

	tcp_channel_desc desc;
	desc.id = id;
	desc.section = sectionLow;
	desc.raw_channel_ptr = p;
	desc.raw_handler_ptr = pHandler;
	desc.bMakrDel = false;

	m_tcp_channel[id] = desc;
	m_tcp_section[sectionLow] = id;

	//到这里是占坑成功，所以无需理会try_connect返回值
	p->try_connect();

	return true;
}

void client_mng::del_tcp(uint64_t con_id)
{
	map<uint64_t, tcp_channel_desc>::iterator it = m_tcp_channel.find(con_id);
	if (it == m_tcp_channel.end())
	{
		return;
	}
	if (it->second.bMakrDel)
	{
		return;
	}
	m_tcp_section.erase(it->second.section);//不用等到on_timer_delay_del
	m_tcp_addr.erase(it->second.addr);

	it->second.bMakrDel = true;

	if (!m_timer_delay_del.is_active())
	{
		m_timer_delay_del.start();
	}
}

bool client_mng::is_del(uint64_t con_id)
{
	map<uint64_t, tcp_channel_desc>::iterator it = m_tcp_channel.find(con_id);
	if (it == m_tcp_channel.end())
	{
		return true;
	}

	return it->second.bMakrDel;
}

string client_mng::get_section(uint64_t con_id)
{
	map<uint64_t, tcp_channel_desc>::iterator it = m_tcp_channel.find(con_id);
	if (it == m_tcp_channel.end())
	{
		return "";
	}

	return it->second.section;
}

void	client_mng::on_timer_delay_del(timer* timer)
{
	m_timer_delay_del.stop();

	map<uint64_t, tcp_channel_desc> tcp_channel = m_tcp_channel;
	for (map<uint64_t, tcp_channel_desc>::iterator it = tcp_channel.begin(); it != tcp_channel.end(); it++)
	{
		if (m_tcp_channel.find(it->first) != m_tcp_channel.end())
		{
			m_tcp_channel.erase(it->first);
			
			it->second.raw_channel_ptr->close();
			delete it->second.raw_channel_ptr;
		}
	}
}

Net::tcp_client_channel* client_mng::find_tcp_channel(const string section)
{
	string sectionLow = lowstring(section);

	map<string, uint64_t>::iterator it = m_tcp_section.find(section);
	if (it == m_tcp_section.end())
	{
		return NULL;
	}

	map<uint64_t, tcp_channel_desc>::iterator it2 = m_tcp_channel.find(it->second);
	assert(it2 != m_tcp_channel.end());

	return it2->second.raw_channel_ptr;
}

Net::tcp_client_channel* client_mng::enum_tcp_channel(const string cluster, uint64_t& cursor)
{
	string clusterLow = lowstring(cluster);

	if (m_tcp_channel.find(cursor) == m_tcp_channel.end())
	{
		cursor = 0;
	}
	if (!cursor)
	{
		if (m_tcp_channel.size())
		{
			cursor = m_tcp_channel.begin()->first;
		}
	}
	if (!cursor)
	{
		return NULL;
	}

	Net::tcp_client_channel* refer = NULL;

	map<uint64_t, tcp_channel_desc>::iterator it2 = m_tcp_channel.find(cursor);
	for (; it2 != m_tcp_channel.end(); it2++)
	{
		Config::section* kv = it2->second.raw_handler_ptr->conf_section();
		assert(kv);
		string server_cluster;
		kv->get(Net::conf_server_cluster, server_cluster);
		server_cluster = lowstring(server_cluster);

		if (clusterLow == server_cluster)
		{
			refer = it2->second.raw_channel_ptr;
			it2++;
			break;
		}		
	}	
	
	if (it2 != m_tcp_channel.end())
	{
		cursor = it2->first;
	}
	else
	{
		cursor = 0;
	}

	return refer;
}

void client_mng::on_connect(uint64_t id)
{
	map<uint64_t, tcp_channel_desc>::iterator it = m_tcp_channel.find(id);
	if (it == m_tcp_channel.end())
	{
		return;
	}

	struct sockaddr_in in;
	if (!it->second.raw_channel_ptr->remote_addr(in))
	{
		//error
		return;	
	}

	it->second.addr.ip = ipn_by_addr(&in);
	it->second.addr.port = portn_by_addr(&in);

	m_tcp_addr[it->second.addr] = id;
}

void client_mng::on_closing(uint64_t id)
{
	map<uint64_t, tcp_channel_desc>::iterator it = m_tcp_channel.find(id);
	if (it == m_tcp_channel.end())
	{
		return;
	}

	m_tcp_addr.erase(it->second.addr);
	it->second.addr.ip = 0;
	it->second.addr.port = 0;
}

Net::tcp_client_channel* client_mng::find_tcp_channel(const struct sockaddr_in& remotr_addr)
{
	digit_addr addr(ipn_by_addr(&remotr_addr), portn_by_addr(&remotr_addr));

	map<digit_addr, uint64_t>::iterator it = m_tcp_addr.find(addr);
	if (it != m_tcp_addr.end())
	{
		map<uint64_t, tcp_channel_desc>::iterator it2 = m_tcp_channel.find(it->second);
		if (it2 != m_tcp_channel.end())
		{
			return it2->second.raw_channel_ptr;
		}
	}

	return NULL;
}
