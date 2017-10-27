#ifndef BALANCE_SERVER_H_
#define BALANCE_SERVER_H_

#define __STDC_LIMIT_MACROS

#include <stdint.h>
#include "name.h"

using namespace std;

namespace BalanceModeImp
{
	namespace Server
	{
		//app call
		bool	reg(const Name::BalanceMode::SvrNodeData& v); //tbd 注册更多信息
		void	inc_load(const section_name_t section_name);
		void	set_tag(const section_name_t section_name, const string key, const string value);
		void	clr_tag(const section_name_t section_name, const string key);

		//platform call, only read
		bool	init(struct ev_loop* loop);
		bool	get_reg_list(uint64_t& seq, map<string, Name::BalanceMode::SvrNodeData>& v);//当seq不匹配，才会返回v

		bool	set_cluster_data(map<string, Name::BalanceMode::SvrNodeData>& v);
		bool	set_server_data(const Name::BalanceMode::SvrNodeData& v);
		bool	get_server_list(const cluster_name_t cluster_name, map<section_name_t, struct sockaddr_in>& v); //tbd 注册更多信息
	}
}

#endif //BALANCE_SERVER_H_