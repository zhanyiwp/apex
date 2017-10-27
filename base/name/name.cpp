#include <string>
#include "socket_func.h"
#include "conf_file.h"
#include "auto_mutex.h"
#include "balance_client.h"
#include "master_client.h"
#include "balance_service.h"
#include "master_service.h"
#include "consistency_block_clt.h"
#include "name.h"
#include "net.h"

bool	Name::init(struct ev_loop* loop, const string conf_path)
{
	if (!BalanceModeImp::Client::init(loop))
	{
		return false;
	}
	if (!MasterModeImp::Client::init(loop))
	{
		return false;
	}
	if (!BalanceModeImp::Server::init(loop))
	{
		return false;
	}
	if (!MasterModeImp::Server::init(loop))
	{
		return false;
	}

	Config::section global_section(conf_path, "zk_svr");
	string zk_svr_addr;
	global_section.get("addr", zk_svr_addr);
	
	return consistency_block_clt::init(zk_svr_addr);
}

////////////////////////////////////////////////////Balance-Client//////////
void	Name::BalanceMode::Client::add_watch(const cluster_name_t& cluster_name)
{
	BalanceModeImp::Client::add_watch(cluster_name);
}

//app call
bool	Name::BalanceMode::Client::pick(const cluster_name_t& cluster_name, uint16_t region_id, uint16_t idc_id, uint8_t ask_count, vector<struct sockaddr_in>& ask_v)
{
	return BalanceModeImp::Client::pick(cluster_name, region_id, idc_id, ask_count, ask_v);
}

bool	Name::BalanceMode::Client::pick(const cluster_name_t& cluster_name, Config::section& conf_section, uint8_t ask_count, vector<struct sockaddr_in>& ask_v)
{
	uint16_t	region_id = 0;
	uint16_t	idc_id = 0;

	conf_section.get("region_id", region_id);
	conf_section.get("idc_id", idc_id);

	return pick(cluster_name, region_id, idc_id, ask_count, ask_v);
}

////////////////////////////////////////////////////Balance-Service//////////
//无需Net::tcp_server_channel*，因为原路回，这样架构更简单
bool	Name::BalanceMode::Server::reg(const Name::BalanceMode::SvrNodeData& data) //tbd 注册更多信息
{
	return BalanceModeImp::Server::reg(data);
}

bool	Name::BalanceMode::Server::reg(const string& conf_path, const string& sys_section_name, const string& svr_section_name)
{
	Config::section sys_conf(conf_path, sys_section_name);
	Config::section svr_conf(conf_path, svr_section_name);

	return reg(sys_conf, svr_conf);
}

bool	Name::BalanceMode::Server::reg(Config::section& sys_section, Config::section& svr_section)
{
	Name::BalanceMode::SvrNodeData data;

	sys_section.get("idc_id", data.idc_id);
	sys_section.get("region_id", data.region_id);

	data.section_name = svr_section.get_name();
	svr_section.get("cluster_name", data.cluster_name);
	string str_ip;
	svr_section.get(Net::conf_listen_ip, str_ip);
	svr_section.get(Net::conf_listen_port, data.port);
	svr_section.get("full_load", data.full_load);
	data.ip = ntohl(inet_addr(str_ip.c_str()));

	return reg(data);
}

//上报一个包
void	Name::BalanceMode::Server::inc_load(const string& section_name)
{
	BalanceModeImp::Server::inc_load(section_name);
}

void	Name::BalanceMode::Server::set_tag(const string& section_name, const string& key, const string& value)//设置自定义kv
{
	BalanceModeImp::Server::set_tag(section_name, key, value);
}

void	Name::BalanceMode::Server::clr_tag(const string& section_name, const string& key)
{
	BalanceModeImp::Server::clr_tag(section_name, key);
}

////////////////////////////////////////////////////Master-Client//////////
void	Name::MasterMode::Client::add_watch(const cluster_name_t& cluster_name)
{
	MasterModeImp::Client::add_watch(cluster_name);
}

bool	Name::MasterMode::Client::get_master(const cluster_name_t& cluster_name, struct sockaddr_in& ask)
{
	return MasterModeImp::Client::get_master(cluster_name, ask);
}

////////////////////////////////////////////////////Master-Service//////////
bool	Name::MasterMode::Server::reg(const Name::MasterMode::SvrNodeData& data, Name::MasterMode::Server::role_cb_t* cb/* = NULL*/) //tbd 注册更多信息
{
	return MasterModeImp::Server::reg(data, cb);
}

bool	Name::MasterMode::Server::reg(const string& file_path, const string& section_name, Name::MasterMode::Server::role_cb_t* cb/* = NULL*/)
{
	Config::section conf(file_path, section_name);

	return reg(conf, cb);
}

bool	Name::MasterMode::Server::reg(Config::section& conf_section, Name::MasterMode::Server::role_cb_t* cb/* = NULL*/)
{
	Name::MasterMode::SvrNodeData data;

	conf_section.get("cluster_name", data.cluster_name);
	data.section_name = conf_section.get_name();
	string str_ip;
	conf_section.get(Net::conf_listen_ip, str_ip);
	conf_section.get(Net::conf_listen_port, data.port);
	conf_section.get("unique_id", data.unique_id);

	data.ip = ntohl(inet_addr(str_ip.c_str()));

	return reg(data, cb);
}

bool    Name::MasterMode::Server::is_master(const string& section_name)
{
	return MasterModeImp::Server::is_master(section_name);
}

bool    Name::MasterMode::Server::get_master(const cluster_name_t& cluster_name, struct sockaddr_in& ask)
{
	return MasterModeImp::Server::get_master(cluster_name, ask);
}

bool	Name::MasterMode::Server::get_slave(const cluster_name_t& cluster_name, map<string, struct sockaddr_in>& ask)
{
	return MasterModeImp::Server::get_slave(cluster_name, ask);
}