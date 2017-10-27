#ifndef ERR_H
#define ERR_H

#include "stdint.h"

typedef enum
{
	SUCCESS = 0,
	ERR_NET_RECVSPACE = 1,
	ERR_NET_SENDSPACE = 2,
	ERR_NET_CONNECT = 3,
	ERR_NET_ACCEPT = 4,
	ERR_NET_SEND = 5,
	ERR_NET_RECV = 6,
}ERROR_CODE;
 

#endif //ERR_H
