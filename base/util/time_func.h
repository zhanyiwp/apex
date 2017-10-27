// time_func.h

#ifndef TIME_FUNC_H
#define TIME_FUNC_H

#include <stdint.h>
#include <sys/time.h>

namespace Time
{
	//same as gettimeofday
	int now(struct timeval *tv);

	// same algorithm with gettimeofday, except with different precision defined as REGET_TIME_US_TIME
	time_t now();
	uint64_t now_ms();

	int extract(time_t timestamp, int32_t zone, int32_t* year, int32_t* month, int32_t* mday, int32_t* hour, int32_t* minute, int32_t* second);
}

#endif //TIME_FUNC_H

