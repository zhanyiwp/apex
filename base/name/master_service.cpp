#include "stdio.h"
#include "socket_func.h"
#include "master_service.h"
#include "auto_mutex.h"
#include "period_timer.h"
#include "string_func.h"
#include "time_func.h"
#include <assert.h>
#include "balance_service.h"

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

enum MasterLocalState
{
	MLS_NONE = 0,
	MLS_CANDIDATE = 1,
	MLS_MASTER = 2,
};

struct master_snapshot_info
{
	uint32_t ip;
	uint16_t port;
	uint32_t unique_id;
	section_name_t	section_name;
	uint64_t update_time;
	uint64_t valid_time;
	MasterLocalState	eState;

	master_snapshot_info()
	{
		ip = 0;
		port = 0;
		unique_id = 0;
		update_time = 0;
		valid_time = 0;
		eState = MLS_NONE;
	}
};
struct register_info//候选人
{
	uint32_t ip;
	uint16_t port;
	uint32_t unique_id;
	cluster_name_t	cluster_name;
	Name::MasterMode::Server::role_cb_t* cb;

	register_info()
	{
		ip = 0;
		port = 0;
		unique_id = 0;
		cb = NULL;
	}
};

static map<cluster_name_t, master_snapshot_info>	_kv;//基于custer_name
static map<section_name_t, register_info>		_reg;//基于section_name
static uint64_t							_seq = 0;
static timer* _checker = NULL;

//////////////////////////////////////////
static void CallCB_InLock(const section_name_t& section_name, Name::MasterMode::Server::RoleType rt)
{
	map<section_name_t, register_info>::iterator it = _reg.find(section_name);
	if (it != _reg.end())
	{
		if (it->second.cb)
		{
			it->second.cb(section_name, rt);
		}
	}
}

static void CheckIsMaster_InLock(master_snapshot_info* info)
{
	//可能调试后在运行，导致时间已经无效了
	if (info->update_time + TIMEOUT_INTERVAL < Time::now_ms())
	{
		return;
	}

	if (MLS_CANDIDATE == info->eState)
	{
		assert(info->valid_time);
		if (info->valid_time + TIMEOUT_INTERVAL < info->update_time)
		{
			info->eState = MLS_MASTER;

			CallCB_InLock(info->section_name, Name::MasterMode::Server::RT_MASTER);
		}
	}
}


static void	on_timer(timer* t)
{
	uint64_t now = Time::now_ms();

	auto_mutex mutex(&_mutex);
	
	_seq++;
	for (map<string, master_snapshot_info>::iterator it = _kv.begin(); it != _kv.end();)
	{
		if (it->second.update_time + TIMEOUT_INTERVAL < now)
		{
			if (MLS_MASTER == it->second.eState)
			{
				CallCB_InLock(it->second.section_name, Name::MasterMode::Server::RT_SLAVE);
			}
			_kv.erase(it++);
		}
		else
		{
			CheckIsMaster_InLock(&it->second);

			it++;
		}
	}
}

//app call
bool	MasterModeImp::Server::reg(const Name::MasterMode::SvrNodeData& v, Name::MasterMode::Server::role_cb_t* cb) //tbd 注册更多信息
{
	string cluster_name = lowstring(v.cluster_name);
	section_name_t section_name = lowstring(v.section_name);
	if (section_name.empty() || cluster_name.empty())
	{
		return false;
	}

	Name::BalanceMode::SvrNodeData data;
	data.cluster_name = cluster_name;
	data.section_name = section_name;
	data.ip = v.ip;
	data.port = v.port;
	if (!BalanceModeImp::Server::reg(data))//复用balance_mode
	{
		return false;
	}

	auto_mutex mutex (&_mutex);

	map<section_name_t, register_info>::iterator it = _reg.find(section_name);
	if (it == _reg.end())
	{
		register_info blank;
		_reg[section_name] = blank;
		it = _reg.find(section_name);
	}
	it->second.cluster_name = cluster_name;
	it->second.ip = v.ip;
	it->second.port = v.port;
	it->second.unique_id = v.unique_id;
	it->second.cb = cb;
	_seq++;

	return true;
}

bool    MasterModeImp::Server::is_master(const section_name_t section_name)
{
	section_name_t section_name2 = lowstring(section_name);

	auto_mutex mutex(&_mutex);

	for (map<string, master_snapshot_info>::iterator it = _kv.begin(); it != _kv.end(); it++)
	{
		if (it->second.section_name == section_name2)
		{
			CheckIsMaster_InLock(&it->second);
			return MLS_MASTER == it->second.eState;
		}
	}

	return false;
}

bool    MasterModeImp::Server::get_master(const cluster_name_t cluster_name, struct sockaddr_in& ask)
{
	cluster_name_t cluster_name2 = lowstring(cluster_name);

	auto_mutex mutex(&_mutex);

	map<cluster_name_t, master_snapshot_info>::iterator it = _kv.find(cluster_name2);
	if (it != _kv.end())
	{
		CheckIsMaster_InLock(&it->second);
		if (MLS_MASTER == it->second.eState)
		{
			set_sock_addr(&ask, it->second.ip, it->second.port);
			return true;
		}
	}

	return false;
}


bool    MasterModeImp::Server::get_slave(const cluster_name_t& cluster_name, map<section_name_t, struct sockaddr_in>& ask)
{
	cluster_name_t cluster_name2 = lowstring(cluster_name);
	ask.clear();
	if (!BalanceModeImp::Server::get_server_list(cluster_name2, ask))
	{
		return false;
	}

	auto_mutex mutex(&_mutex);

	map<cluster_name_t, master_snapshot_info>::iterator it = _kv.find(cluster_name2);
	if (it != _kv.end())
	{
		ask.erase(it->second.section_name);
	}

	return true;
}


//platform call
bool	MasterModeImp::Server::init(struct ev_loop* loop)
{
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

bool	MasterModeImp::Server::get_reg_and_cluster_list(uint64_t& seq, map<string, Name::MasterMode::SvrNodeData>& v)//当seq不匹配，才会返回v
{
	if (seq == _seq)
	{
		return false;
	}

	v.clear();

	auto_mutex mutex(&_mutex);
	for (map<section_name_t, register_info>::iterator it = _reg.begin(); it != _reg.end(); it++)
	{
		Name::MasterMode::SvrNodeData	data;
		data.section_name = it->first;
		data.cluster_name = it->second.cluster_name;
		data.ip = it->second.ip;
		data.port = it->second.port;
		data.unique_id = it->second.unique_id;

		v[it->second.cluster_name] = data;
	}

	seq = _seq;
	return true;
}

bool	MasterModeImp::Server::set_master(const Name::MasterMode::SvrNodeData& v)
{
	auto_mutex mutex(&_mutex);

	map<cluster_name_t, master_snapshot_info>::iterator it = _kv.find(v.cluster_name);
	if (it == _kv.end())
	{
		master_snapshot_info si;
		si.valid_time = Time::now_ms();
		si.eState = MLS_CANDIDATE;
		_kv[v.cluster_name] = si;
		it = _kv.find(v.cluster_name);
	}
	if (it->second.section_name != v.section_name)
	{
		if (MLS_MASTER == it->second.eState)
		{
			CallCB_InLock(it->second.section_name, Name::MasterMode::Server::RT_SLAVE);
		}
		it->second.valid_time = Time::now_ms();
		it->second.eState = MLS_CANDIDATE;
		it->second.section_name = v.section_name;
	}
	it->second.ip = v.ip;
	it->second.port = v.port;
	it->second.unique_id = v.unique_id;
	it->second.update_time = Time::now_ms();

	CheckIsMaster_InLock(&it->second);

	printf("set_master section_name: %s, cluster_name: %s, ip: %s, port: %u, unique_id: %u, vt: %llu, ut: %llu, Diff: %llu, eState: %u\n", it->second.section_name.c_str(), it->first.c_str(), string_by_iph(it->second.ip).c_str(), it->second.port, it->second.unique_id, it->second.valid_time, it->second.update_time, it->second.update_time - it->second.valid_time, (uint32_t)it->second.eState);

	return true;
}


