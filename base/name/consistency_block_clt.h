#ifndef CONSISTENCY_BLOCK_CLT_H_
#define CONSISTENCY_BLOCK_CLT_H_

#define __STDC_LIMIT_MACROS

#include <stdint.h>
#include "name.h"

using namespace std;

namespace consistency_block_clt
{
	const uint32_t ZK_BLOCK_INIT_TIMEOUT = (2 * 1000);	

	bool init(const string zk_svr_ip_list, uint32_t ms = ZK_BLOCK_INIT_TIMEOUT);
}

#endif //CONSISTENCY_BLOCK_CLT_H_