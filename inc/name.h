#ifndef NAME_H
#define NAME_H

#include "stdint.h"
#include <vector>
#include <map>
#include <string>
#include "ev.h"
#include "conf_file.h"

using namespace std;

typedef string section_name_t;
typedef string cluster_name_t ;

namespace Name
{
	const string Balance_Node_Name = "/list";
	const string Master_Node_Name = "/master";

	bool	init(struct ev_loop* loop, const string conf_path);

	namespace BalanceMode
	{
		struct SvrNodeData
		{
			section_name_t section_name;//服务名
			cluster_name_t cluster_name;//集群名
			uint32_t ip;
			uint16_t port;
			uint16_t region_id;
			uint16_t idc_id;
			uint32_t full_load;//满负载请求数，单位：分钟
			uint32_t current_load;
			map<string, string>	tags;//kv属性，用于调用端和服务端通信
			uint32_t	flags;//内部使用的标记

			SvrNodeData()
			{
				ip = 0;
				port = 0;
				region_id = 0;
				idc_id = 0;
				full_load = 0;
				current_load = 0;
				flags = 0;
			}
		};

		namespace Client
		{
			void	add_watch(const cluster_name_t& cluster_name);

			//typedef bool pick_cb_t(vector<SvrNodeData>& in, vector<struct sockaddr_in>& out);//此回调处于pick线程，除非用到外部变量，内部实现无需加锁
			bool	pick(const cluster_name_t& cluster_name, uint16_t region_id, uint16_t idc_id, uint8_t ask_count, vector<struct sockaddr_in>& ask_v);
			bool	pick(const cluster_name_t& cluster_name, Config::section& conf_section, uint8_t ask_count, vector<struct sockaddr_in>& ask_v);
		}
		namespace Server
		{
			bool	reg(const Name::BalanceMode::SvrNodeData& data); //注册自己
			bool	reg(const string& conf_path, const section_name_t& sys_section_name, const section_name_t& svr_section_name);
			bool	reg(Config::section& sys_section, Config::section& svr_section);

			void	inc_load(const string& section_name_t);//请求量递增
			void	set_tag(const string& section_name_t, const string& key, const string& value);//设置自定义kv
			void	clr_tag(const string& section_name_t, const string& key);
		}
	}
	namespace MasterMode
	{
		struct SvrNodeData
		{
			section_name_t section_name;
			cluster_name_t cluster_name;
			uint32_t ip;
			uint16_t port;
			uint32_t unique_id;
			uint32_t flags;

			SvrNodeData()
			{
				ip = 0;
				port = 0;
				unique_id = 0;
				flags = 0;
			}
		};

		namespace Client
		{
			void	add_watch(const cluster_name_t& cluster_name);
			bool	get_master(const cluster_name_t& cluster_name, struct sockaddr_in& ask);
		}
		namespace Server
		{
			enum RoleType
			{
				RT_NONE = 0,
				RT_MASTER = 1,
				RT_SLAVE = 2,
			};
			typedef void role_cb_t(const section_name_t& section_name, RoleType eNewRole);
			bool	reg(const Name::MasterMode::SvrNodeData& data, role_cb_t* cb = NULL);
			bool	reg(const string& file_path, const section_name_t& section_name, role_cb_t* cb = NULL);
			bool	reg(Config::section& conf_section, Name::MasterMode::Server::role_cb_t* cb = NULL);

			//根据注册信息来做身份判断依据
			bool	is_master(const section_name_t& section_name);
			bool	get_master(const cluster_name_t& cluster_name, struct sockaddr_in& ask);//尝试转发
			bool	get_slave(const cluster_name_t& cluster_name, map<section_name_t, struct sockaddr_in>& ask);
		}
	}
}

#endif //NAME_H
