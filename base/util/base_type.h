#ifndef BASE_TYPE_H_
#define BASE_TYPE_H_

#undef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE 1
#undef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
	
#include <stdint.h>
#include <stddef.h>

#ifndef UINT32_MAX
#define UINT32_MAX (0xFFFFFFFF)
#endif

#ifndef INT32_MAX
#define INT32_MAX (0x7FFFFFFFF)
#endif

typedef int SOCKET;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)

//only 64 bit sys support
#define member_offsetof(Type, Member) ( (size_t)( &(((Type*)8)->Member) ) - 8 )

#endif //BASE_TYPE_H_
