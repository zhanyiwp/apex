#include "stdio.h"
#include "socket_func.h"
#include "auto_mutex.h"
#include "balance_service.h"
#include "time_func.h"
#include "period_timer.h"
#include "string_func.h"

static pthread_mutex_t _mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

#define MAX_SNAPSHOT_ELEMENT (7)
#define TIME_INTERVAL		(10)//must > GATHER_INTERVAL

struct snapshot_load
{
	time_t		t;
	uint32_t	l;

	snapshot_load()
	{
		clear();
	}
	void clear()
	{
		t = 0;
		l = 0;
	}
};
struct balance_snapshot_info
{
	cluster_name_t cluster_name;
	uint32_t ip;
	uint16_t port;
	uint16_t region_id;
	uint16_t idc_id;
	uint32_t full_load;
	snapshot_load current_load[MAX_SNAPSHOT_ELEMENT];
	map<string, string>	tags;
	uint16_t current_index;
	time_t update_time;
	bool	reg;

	balance_snapshot_info()
	{
		reg = false;
		region_id = 0;
		idc_id = 0;
		full_load = 0;
		current_index = MAX_SNAPSHOT_ELEMENT - 1;
		update_time = Time::now();
	}
};

static map<section_name_t, struct balance_snapshot_info> _kv;//section
static uint64_t	_seq = 0;
static timer* _checker = NULL;

void	on_timer(timer* t)
{
	auto_mutex	mutex(&_mutex);

	time_t tNow = Time::now();

	_seq++;	
	for (map<section_name_t, struct balance_snapshot_info>::iterator it = _kv.begin(); it != _kv.end();)
	{
		if (it->second.reg)
		{
			it->second.current_load[it->second.current_index].clear();
			it->second.current_index = (it->second.current_index + 1) % MAX_SNAPSHOT_ELEMENT;
		}
		else
		{
			if (it->second.update_time + 60 < tNow)
			{
				_kv.erase(it++);
				continue;
			}
		}

		it++;
	}
}

///////////////////////////////////////////////
//app call
bool	BalanceModeImp::Server::reg(const Name::BalanceMode::SvrNodeData& v) //tbd 注册更多信息
{
	section_name_t section_name = lowstring(v.section_name);
	cluster_name_t cluster_name = lowstring(v.cluster_name);

	if (cluster_name.empty() || cluster_name.empty())
	{
		return false;
	}
	if (!v.ip || !v.port)
	{
		return false;
	}

	auto_mutex	mutex(&_mutex);

	map<string, struct balance_snapshot_info>::iterator it = _kv.find(section_name);
	if (it == _kv.end())
	{
		struct balance_snapshot_info blank;
		_kv[section_name] = blank;
		it = _kv.find(section_name);
	}
	it->second.reg = true;

	//重复项目检查
	if (!it->second.cluster_name.empty() && it->second.cluster_name != cluster_name)
	{
		return false;
	}
	it->second.cluster_name = cluster_name;

	it->second.ip = v.ip;
	it->second.port = v.port;
	if (!it->second.region_id && it->second.region_id != v.region_id)
	{
		return false;
	}
	it->second.region_id = v.region_id;

	if (!it->second.idc_id && it->second.idc_id != v.idc_id)
	{
		return false;
	}
	it->second.idc_id = v.idc_id;
	if (!it->second.full_load && it->second.full_load != v.full_load)
	{
		return false;
	}
	it->second.full_load = v.full_load;
	_seq++;

	return true;
}

void	BalanceModeImp::Server::inc_load(const section_name_t section_name)
{
	section_name_t section_name2 = lowstring(section_name);

	auto_mutex	mutex(&_mutex);

	map<section_name_t, struct balance_snapshot_info>::iterator it = _kv.find(section_name2);
	if (it == _kv.end())
	{
		return;
	}
	it->second.update_time = Time::now();
	for (uint16_t i = 0; i < MAX_SNAPSHOT_ELEMENT; i++)
	{
		if (i != it->second.current_index)
		{
			it->second.current_load[i].l++;
		}
	}
}

void	BalanceModeImp::Server::set_tag(const section_name_t section_name, const string tag_key, const string tag_value)
{
	section_name_t section_name2 = lowstring(section_name);

	auto_mutex	mutex(&_mutex);

	map<string, struct balance_snapshot_info>::iterator it = _kv.find(section_name2);
	if (it == _kv.end())
	{
		return;
	}

	it->second.update_time = Time::now();
	it->second.tags[tag_key] = tag_value;
}

void	BalanceModeImp::Server::clr_tag(const section_name_t section_name, const string key)
{
	section_name_t section_name2 = lowstring(section_name);

	auto_mutex	mutex(&_mutex);

	map<section_name_t, struct balance_snapshot_info>::iterator it = _kv.find(section_name2);
	if (it == _kv.end())
	{
		return;
	}

	it->second.update_time = Time::now();
	it->second.tags.erase(key);
}

//platform call, only read
bool	BalanceModeImp::Server::init(struct ev_loop* loop)
{
	auto_mutex	mutex(&_mutex);

	if (!_checker)
	{
		_checker = new timer(loop);
		if (!_checker)
		{
			return false;
		}

		_checker->set(&on_timer, TIME_INTERVAL * 1000);
		_checker->start();
		srand(Time::now());
	}

	return true;
}

bool	BalanceModeImp::Server::get_reg_list(uint64_t& seq, map<string, Name::BalanceMode::SvrNodeData>& v)//当seq不匹配，才会返回v
{
	if (_seq == seq)
	{
		return false;
	}

	v.clear();

	auto_mutex	mutex(&_mutex);

	for (map<string, struct balance_snapshot_info>::iterator it = _kv.begin(); it != _kv.end(); it++)
	{
		if (it->second.reg)
		{
			continue;
		}

		Name::BalanceMode::SvrNodeData ni;

		ni.section_name = it->first;
		ni.cluster_name = it->second.cluster_name;
		ni.ip = it->second.ip;
		ni.port = it->second.port;
		ni.region_id = it->second.region_id;
		ni.idc_id = it->second.idc_id;
		ni.full_load = it->second.full_load;
		ni.current_load = it->second.current_load[it->second.current_index].l;

		string key = ni.section_name;
		v[ni.section_name] = ni;

		printf("server_report section_name: %s, cluster_name: %s, ip: %s, port: %u, region_id: %u, idc_id: %u, full_load: %u, current_load: %u\n", ni.section_name.c_str(), ni.cluster_name.c_str(), string_by_iph(ni.ip).c_str(), ni.port, ni.region_id, ni.idc_id, ni.full_load, ni.current_load);
	}

	seq = _seq;
	return true;
}


bool	BalanceModeImp::Server::set_cluster_data(map<string, Name::BalanceMode::SvrNodeData>& v)
{
	auto_mutex	mutex(&_mutex);

	//目前不清理掉已经不存在的服务节点，避免短暂扰动造成服务的负载冲击，我们通过前面定时器清理
	for (map<string, Name::BalanceMode::SvrNodeData>::iterator it = v.begin(); it != v.end(); it++)
	{
		set_server_data(it->second);
	}

	return true;
}

bool	BalanceModeImp::Server::set_server_data(const Name::BalanceMode::SvrNodeData& v)
{
	printf("client_update section_name: %s, cluster_name: %s, ip: %s, port: %u, region_id: %u, idc_id: %u, full_load: %u, current_load: %u\n", v.section_name.c_str(), v.cluster_name.c_str(), string_by_iph(v.ip).c_str(), v.port, v.region_id, v.idc_id, v.full_load, v.current_load);

	time_t tNow = Time::now();

	section_name_t section_name = lowstring(v.section_name);
	cluster_name_t cluster_name = lowstring(v.cluster_name);
	if (v.section_name.empty())
	{
		return false;
	}

	auto_mutex	mutex(&_mutex);

	map<section_name_t, balance_snapshot_info>::iterator it = _kv.find(section_name);
	if (it == _kv.end())
	{
		balance_snapshot_info blank;
		_kv[section_name] = blank;
		it = _kv.find(section_name);
	}
	if (it->second.reg)
	{
		return true;
	}
	
	it->second.cluster_name = cluster_name;
	it->second.ip = v.ip;
	it->second.port = v.port;
	it->second.region_id = v.region_id;
	it->second.idc_id = v.idc_id;
	it->second.tags = v.tags;
	it->second.update_time = tNow;

	return true;
}

bool	BalanceModeImp::Server::get_server_list(const cluster_name_t cluster_name, map<section_name_t, struct sockaddr_in>& v) //tbd 注册更多信息
{
	cluster_name_t cluster_name2 = lowstring(cluster_name);
	v.clear();

	auto_mutex	mutex(&_mutex);

	for (map<section_name_t, struct balance_snapshot_info>::iterator it = _kv.begin(); it != _kv.end(); it++)
	{
		if (it->second.cluster_name == cluster_name2)
		{
			struct sockaddr_in si;
			set_sock_addr(&si, it->second.ip, it->second.port);
			v[it->first] = si;
		}
	}

	return true;
}