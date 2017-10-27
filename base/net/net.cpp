#include "string_func.h"
#include "net.h"
#include "client_mng.h"
#include "service_mng.h"
#include "auto_mutex.h"

static pthread_mutex_t s_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

//////////////////////////////////////////////////
//////////////////////////////////////////////////

struct ev_thread
{
	ev_thread()
	{
		client = NULL;
		service = NULL;
	}
	bool is_empty()
	{
		return NULL == client && NULL == service;
	}
	client_mng*		client;
	service_mng*	service;
};

static map<struct ev_loop*, ev_thread>	thread_map;

client_mng*		get_client_mng(struct ev_loop* loop, bool create_if_not_exist/* = true*/)
{
	if (!loop)
	{
		return NULL;
	}

	auto_mutex	mutex(&s_mutex);

	map<struct ev_loop*, ev_thread>::iterator it = thread_map.find(loop);
	if (it == thread_map.end())
	{
		if (!create_if_not_exist)
		{
			return NULL;
		}

		ev_thread et;
		thread_map[loop] = et;
		it = thread_map.find(loop);
	}
	if (!it->second.client)
	{
		if (!create_if_not_exist)
		{
			return NULL;
		}
		client_mng* p = new client_mng(loop);
		if (!p)
		{
			return NULL;
		}

		it->second.client = p;
	}
	
	return it->second.client;
}

service_mng*	get_service_mng(struct ev_loop* loop, bool create_if_not_exist/* = true*/)
{
	if (!loop)
	{
		return NULL;
	}

	auto_mutex mutex(&s_mutex);

	map<struct ev_loop*, ev_thread>::iterator it = thread_map.find(loop);
	if (it == thread_map.end())
	{
		if (!create_if_not_exist)
		{
			return NULL;
		}

		ev_thread et;
		thread_map[loop] = et;
		it = thread_map.find(loop);
	}
	if (!it->second.service)
	{
		if (!create_if_not_exist)
		{
			return NULL;
		}
		service_mng* p = new service_mng(loop);
		if (!p)
		{
			return NULL;
		}

		it->second.service = p;
	}

	return it->second.service;
}

bool Net::add_tcp_client(struct ev_loop* loop, Net::tcp_client_handler_raw* pHandler)
{
	if (!pHandler)
	{
		return false;
	}

	client_mng* mng = get_client_mng(loop, true);
	if (!mng)
	{
		return false;
	}

	return mng->add_tcp(pHandler);
}

bool Net::add_tcp_listen(struct ev_loop* loop, Net::tcp_server_handler_raw* pHandler)
{
	if (!pHandler)
	{
		return false;
	}

	service_mng* mng = get_service_mng(loop, true);
	if (!mng)
	{
		return false;
	}

	return mng->add_tcp_listen(pHandler);
}

void Net::run(struct ev_loop* loop)
{
	while (true)
	{
		ev_run(loop, 0);
	}
}

void	Net::destroy(struct ev_loop* loop)
{
	if (!loop)
	{
		return;
	}

	auto_mutex mutex(&s_mutex);

	map<struct ev_loop*, ev_thread>::iterator it = thread_map.find(loop);
	if (it != thread_map.end())
	{
		client_mng* client = it->second.client;
		service_mng* service = it->second.service;
		thread_map.erase(it);
		delete client;
		delete service;
	}
}

//local
Net::tcp_client_channel* Net::find_tcp_client_channel(struct ev_loop* loop, const string local_section)
{
	auto_mutex mutex(&s_mutex); //防止删除

	client_mng* mng = get_client_mng(loop, false);
	if (!mng)
	{
		return NULL;
	}

	return mng->find_tcp_channel(local_section);
}

Net::tcp_client_channel* enum_tcp_client_channel(struct ev_loop* loop, const string server_cluster, uint64_t& cursor)//cursor为0表示从第一个开始；返回值为0表示结束
{
	auto_mutex mutex(&s_mutex); //防止删除

	client_mng* mng = get_client_mng(loop, false);
	if (!mng)
	{
		return NULL;
	}

	return mng->enum_tcp_channel(server_cluster, cursor);
}

//remote
Net::tcp_client_channel* Net::find_tcp_client_channel(struct ev_loop* loop, const struct sockaddr_in& remotr_addr)
{
	auto_mutex mutex(&s_mutex); //防止删除

	client_mng* mng = get_client_mng(loop, false);
	if (!mng)
	{
		return NULL;
	}

	return mng->find_tcp_channel(remotr_addr);
}

Net::tcp_server_channel* Net::find_tcp_server_channel(struct ev_loop* loop, const string section, uint64_t channel_id)
{
	auto_mutex mutex(&s_mutex); //防止删除

	service_mng* mng = get_service_mng(loop, false);
	if (!mng)
	{
		return NULL;
	}

	return mng->find_tcp_channel(section, channel_id);
}

Net::tcp_server_channel* Net::enum_tcp_server_channel(struct ev_loop* loop, const string section, uint64_t& cursor)
{
	auto_mutex mutex(&s_mutex); //防止删除

	service_mng* mng = get_service_mng(loop, false);
	if (!mng)
	{
		return NULL;
	}

	return mng->enum_tcp_channel(section, cursor);
}
