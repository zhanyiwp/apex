#include "stdio.h"
#include <algorithm>
#include "socket_func.h"
#include "master_client.h"
#include "auto_mutex.h"
#include "period_timer.h"
#include "string_func.h"
#include "time_func.h"

static pthread_mutex_t _mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

//本文件所有时间单位为：毫秒
#define TIMEOUT_INTERVAL	(10 * 1000 )	//must 大于 GATHER_INTERVAL
#define TIMER_INTERVAL		(2 * 1000)	//must 小于 GATHER_INTERVAL

//
//1，valid_time为0表示初始态,不为0表示第一次设置的时间
//2，当update_time - valid_time >= TIMEOUT_INTERVAL，表示可以跃迁到正常态，即确认为Master，至于master是否认为自己是否“Master”，则由“Master”保证
//3，当now - update_time >= TIMEOUT_INTERVAL，表示可以删除
//
//

struct master_server_info
{
	uint32_t ip;
	uint16_t port;
	uint32_t unique_id;
	section_name_t	section_name;
	uint64_t update_time;
	uint64_t valid_time;//

	master_server_info()
	{
		ip = 0;
		port = 0;
		update_time = 0;
		valid_time = 0;
	}
};
static map<cluster_name_t, master_server_info>	_kv;//基于cluster_name
static uint64_t							_seq = 0;
static map<cluster_name_t, uint64_t>			_watch_cluster;
static timer*							_checker = NULL;

///////////////////////////////////////////////////////////////////////////
static void	on_timer(timer* t)
{
	uint64_t now = Time::now_ms();

	auto_mutex mutex(&_mutex);

	_seq++;
	for (map<string, master_server_info>::iterator it = _kv.begin(); it != _kv.end();)
	{
		if (it->second.update_time + TIMEOUT_INTERVAL < now)
		{
			_kv.erase(it++);
		}
		else
		{
			it++;
		}
	}
}

//app call
void	MasterModeImp::Client::add_watch(const cluster_name_t cluster_name)
{
	auto_mutex mutex(&_mutex);

	if (cluster_name.empty())
	{
		return;
	}
	_watch_cluster[lowstring(cluster_name)] = 0;
	_seq++;
}

bool	MasterModeImp::Client::get_master(const cluster_name_t cluster_name, struct sockaddr_in& ask)
{
	cluster_name_t cls = lowstring(cluster_name);

	auto_mutex mutex(&_mutex);

	map<string, master_server_info>::iterator it = _kv.find(cls);
	if (it != _kv.end())
	{
		//可能调试后在运行，导致时间已经无效了
		if (it->second.update_time + TIMEOUT_INTERVAL < Time::now_ms() && it->second.valid_time + TIMEOUT_INTERVAL <= it->second.update_time)
		{
			set_sock_addr(&ask, it->second.ip, it->second.port);
			return true;
		}
	}

	return false;
}

//platform call
bool	MasterModeImp::Client::init(struct ev_loop* loop)
{
	auto_mutex mutex(&_mutex);

	if (!_checker)
	{
		_checker = new timer(loop);
		if (!_checker)
		{
			return false;
		}

		_checker->set(&on_timer, TIMER_INTERVAL);
		_checker->start();
		srand(Time::now());
	}

	return true;
}

bool	MasterModeImp::Client::get_cluster_list(uint64_t& seq, map<string, uint64_t>& v)//当seq不匹配，才会返回v
{
	auto_mutex mutex(&_mutex);

	if (seq == _seq)
	{
		return false;
	}

	v.clear();

	for (map<string, uint64_t>::iterator it = _watch_cluster.begin(); it != _watch_cluster.end(); it++)
	{
		v[it->first] = 0;
	}

	seq = _seq;
	return true;
}

bool	MasterModeImp::Client::set_master(Name::MasterMode::SvrNodeData& v)
{
	auto_mutex mutex(&_mutex);

	map<cluster_name_t, master_server_info>::iterator it = _kv.find(v.cluster_name);
	if (it == _kv.end())
	{
		master_server_info si;
		si.valid_time = Time::now_ms();
		_kv[v.cluster_name] = si;
		it = _kv.find(v.cluster_name);
	}
	if (it->second.section_name != v.section_name)
	{
		it->second.valid_time = Time::now_ms();
		it->second.section_name = v.section_name;
	}

	it->second.ip = v.ip;
	it->second.port = v.port;
	it->second.unique_id = v.unique_id;
	it->second.update_time = Time::now_ms();

	printf("set_master section_name: %s, cluster_name: %s, ip: %s, port: %u, unique_id: %u, vt: %llu, ut: %llu, Diff: %llu\n", it->second.section_name.c_str(), it->first.c_str(), string_by_iph(it->second.ip).c_str(), it->second.port, it->second.unique_id, it->second.valid_time, it->second.update_time, it->second.update_time - it->second.valid_time);

	return true;
}

