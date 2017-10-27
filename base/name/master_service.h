#ifndef MASTER_SERVER_H_
#define MASTER_SERVER_H_

#define __STDC_LIMIT_MACROS

#include <stdint.h>
#include "name.h"

using namespace std;

namespace MasterModeImp
{
	namespace Server
	{
		//app call
		bool	reg(const Name::MasterMode::SvrNodeData& v, Name::MasterMode::Server::role_cb_t* cb); //tbd 注册更多信息
		bool    is_master(const section_name_t section_name);
		bool    get_master(const cluster_name_t cluster_name, struct sockaddr_in& ask);
		bool	get_slave(const cluster_name_t& cluster_name, map<section_name_t, struct sockaddr_in>& ask);

		//platform call
		bool	init(struct ev_loop* loop);
		bool	get_reg_and_cluster_list(uint64_t& seq, map<cluster_name_t, Name::MasterMode::SvrNodeData>& v);//当seq不匹配，才会返回v
		bool    set_master(const Name::MasterMode::SvrNodeData& v);
	}
}

#endif //MASTER_SERVER_H_