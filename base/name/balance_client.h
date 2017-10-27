#ifndef BALANCE_CLIENT_H_
#define BALANCE_CLIENT_H_

#define __STDC_LIMIT_MACROS

#include <stdint.h>
#include <vector>
#include <string>
#include <map>
#include "name.h"

using namespace std;

namespace BalanceModeImp
{
	namespace Client
	{
		//app call
		void	add_watch(const cluster_name_t cluster_name);
		bool	pick(const cluster_name_t cluster_name, uint16_t region_id, uint16_t idc_id, uint8_t ask_count, vector<struct sockaddr_in>& ask_v);

		//platform call
		bool	init(struct ev_loop* loop);
		bool	get_watch_list(uint64_t& seq, map<string, uint64_t>& v);//当seq不匹配，才会返回v
		bool	set_cluster_data(map<string, Name::BalanceMode::SvrNodeData>& v);
		bool	set_server_data(const Name::BalanceMode::SvrNodeData& v);
	}
}

#endif //BALANCE_CLIENT_H_