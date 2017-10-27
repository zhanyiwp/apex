#ifndef MASTER_CLIENT_H_
#define MASTER_CLIENT_H_

#define __STDC_LIMIT_MACROS

#include <stdint.h>
#include <vector>
#include <string>
#include <map>
#include "name.h"

using namespace std;

namespace MasterModeImp
{
	namespace Client
	{
		//app call
		void	add_watch(const cluster_name_t cluster_name);
		bool	get_master(const cluster_name_t cluster_name, struct sockaddr_in& ask);

		//platform call
		bool	init(struct ev_loop* loop);
		bool	get_cluster_list(uint64_t& seq, map<cluster_name_t, uint64_t>& v);//当seq不匹配，才会返回v
		bool	set_master(Name::MasterMode::SvrNodeData& v);
	}
}

#endif //MASTER_CLIENT_H_