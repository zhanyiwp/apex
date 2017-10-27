#include "stdio.h"
#include <algorithm>
#include "socket_func.h"
#include "balance_client.h"
#include "auto_mutex.h"
#include "net.h"
#include "period_timer.h"
#include "string_func.h"

static pthread_mutex_t _mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

#define MAX_SLOT_NUM	(6)	//为了避免大幅波动，采用两次平均值进行平滑化
#define TIME_INTERVAL	(10)//must > GATHER_INTERVAL

struct slot_item
{
	uint32_t full_load;
	uint32_t current_load;

	slot_item()
	{
		full_load = 0;
		current_load = 0;
	}
};

struct balance_server_info
{
	uint32_t ip;
	uint16_t port;
	uint16_t region_id;
	uint16_t idc_id;
	slot_item	slot[MAX_SLOT_NUM];
	uint16_t	current_slot;
	cluster_name_t cluster_name;
	section_name_t section_name;
	time_t update_time;
	map<string, string>	tags;

	balance_server_info()
	{
		ip = 0;
		port = 0;
		region_id = 0;
		idc_id = 0;
		update_time = Time::now();
		current_slot = 0;
	}
};

static map<section_name_t, balance_server_info>		_kv;//section_name
static uint64_t						_seq = 0;
static map<string, uint64_t>		_watch_cluster;
static timer*						_checker = NULL;

///////////////////////////////////////////////////////////////////////////
static void	on_timer(timer* t)
{
	time_t now = Time::now();

	auto_mutex	mutex(&_mutex);

	_seq++;
	for (map<string, balance_server_info>::iterator it = _kv.begin(); it != _kv.end();)
	{
		if (it->second.update_time + 60 < now)
		{
			printf("Remove: %s\n", it->first.c_str());
			_kv.erase(it++);
		}
		else
		{
			it++;
		}
	}
}


//app call
void	BalanceModeImp::Client::add_watch(const cluster_name_t cluster_name)
{
	if (cluster_name.empty())
	{
		return;
	}

	auto_mutex	mutex(&_mutex);

	_watch_cluster[lowstring(cluster_name)] = 0;
	_seq++;
}

struct  kv_item
{
	uint32_t	ip;
	uint16_t	port;
	uint64_t	weight;

	kv_item(uint32_t ip_, uint16_t port_, uint64_t weight_)
	{
		ip = ip_;
		port = port_;
		weight = weight_;
	}
	bool operator<(const kv_item &other) const
	{
		return weight > other.weight;
	}
};
bool	BalanceModeImp::Client::pick(const cluster_name_t cluster_name, uint16_t region_id, uint16_t idc_id, uint8_t ask_count, vector<struct sockaddr_in>& ask_v)
{
	//基于权重
	vector<kv_item> weight_load;
	uint64_t	total = 0;

	do 
	{
		auto_mutex	mutex(&_mutex);

		for (map<string, balance_server_info>::iterator it = _kv.begin(); it != _kv.end(); it++)
		{
			if (it->second.cluster_name == cluster_name)
			{
				uint32_t	m = 0;
				uint32_t	c = 0;
				uint16_t	valid_num = 0;
				for (uint16_t i = 0; i < MAX_SLOT_NUM; i++)
				{
					if (0 != it->second.slot[i].full_load)
					{
						m += it->second.slot[i].full_load;
						c += it->second.slot[i].current_load;
						valid_num++;
					}
				}
				if (valid_num)
				{
					m /= valid_num;
					c /= valid_num;

					uint64_t rate = 10000 - ((uint64_t)c) * 10000 / m;
					if (it->second.idc_id == idc_id)
					{
						rate *= 95;
					}
					else if (it->second.region_id == region_id)
					{
						rate *= 4;
					}
					//rate *= (uint32_t)log(rate);

					printf("m: %u, c: %u, rate: %llu\n", m, c, rate);

					total += rate;

					kv_item kv(it->second.ip, it->second.port, rate);
					weight_load.push_back(kv);
				}
			}
		}
	} while (0);

	ask_v.clear();
	int16_t count = ask_count;
	if (weight_load.size() && total)
	{
		std::sort(weight_load.begin(), weight_load.end());
	}

	uint64_t total_native = total;
	for (int16_t i = count; i > 0 && weight_load.size() && total; i--)
	{
		uint32_t r = uint32_t(rand()) % total;
		uint32_t r_native = r;
		for (vector<kv_item>::reverse_iterator it = weight_load.rbegin(); it != weight_load.rend(); it++)
		{
			if (r <= it->weight)
			{
				printf("total_native: %llu, native: %u, r: %u, client_pick addr: %s:%u, cluster_name: %s\n", total_native, r_native, r, string_by_iph(it->ip).c_str(), it->port, cluster_name.c_str());

				struct sockaddr_in si;
				set_sock_addr(&si, it->ip, it->port);
				ask_v.push_back(si);
				total -= it->weight;
				weight_load.erase((++it).base());

				break;
			}

			r -= it->weight;
		}
	}

	return ask_v.size() > 0;
}

//platform call
bool BalanceModeImp::Client::init(struct ev_loop* loop)
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

bool	BalanceModeImp::Client::get_watch_list(uint64_t& seq, map<string, uint64_t>& v)//当seq不匹配，才会返回v
{
	if (seq == _seq)
	{
		return false;
	}
	
	v.clear();
	auto_mutex	mutex(&_mutex);
	for (map<string, uint64_t>::iterator it = _watch_cluster.begin(); it != _watch_cluster.end(); it++)
	{
		v[it->first] = 0;
	}

	seq = _seq;
	return true;
}

bool	BalanceModeImp::Client::set_cluster_data(map<string, Name::BalanceMode::SvrNodeData>& v)
{
	auto_mutex	mutex(&_mutex);

	//目前不清理掉已经不存在的服务节点，避免短暂扰动造成服务的负载冲击，我们通过前面定时器清理
	for (map<string, Name::BalanceMode::SvrNodeData>::iterator it = v.begin(); it != v.end(); it++)
	{
		set_server_data(it->second);
	}

	return true;
}

bool	BalanceModeImp::Client::set_server_data(const Name::BalanceMode::SvrNodeData& v)
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

	map<section_name_t, balance_server_info>::iterator it = _kv.find(section_name);
	if (it == _kv.end())
	{
		balance_server_info blank;
		_kv[section_name] = blank;
		it = _kv.find(section_name);
	}
	if (it->second.update_time > tNow)
	{
		return false;
	}
	uint16_t slot_offset = (tNow % TIME_INTERVAL + MAX_SLOT_NUM - it->second.update_time % TIME_INTERVAL) % MAX_SLOT_NUM;
	for (uint16_t i = it->second.current_slot + 1; i < it->second.current_slot + slot_offset; i++)
	{
		it->second.slot[i % MAX_SLOT_NUM].current_load = 0;
		it->second.slot[i % MAX_SLOT_NUM].full_load = 0;
	}
	it->second.cluster_name = cluster_name;
	it->second.ip = v.ip;
	it->second.port = v.port;
	it->second.region_id = v.region_id;
	it->second.idc_id = v.idc_id;
	it->second.current_slot = (it->second.current_slot + slot_offset) % MAX_SLOT_NUM;
	it->second.slot[it->second.current_slot].current_load = v.current_load;
	it->second.slot[it->second.current_slot].full_load = v.full_load;
	if (it->second.slot[it->second.current_slot].current_load > it->second.slot[it->second.current_slot].full_load)
	{
		it->second.slot[it->second.current_slot].current_load = it->second.slot[it->second.current_slot].full_load;
	}	
	it->second.tags = v.tags;
	it->second.update_time = tNow;
	
	return true;
}

