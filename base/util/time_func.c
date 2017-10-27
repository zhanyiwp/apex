#include <string.h>
#include <time.h>
#include "time_func.h"

//
// optimized gettimeofday()/time()
//
// Limitations:
//  1, here we assume the CPU speed is 1GB, if your CPU is 4GB, it will run well, but if your CPU is 10GB, please adjust CPU_SPEED_GB
//  2, these functions have precision of 1ms, if you wish higher precision, please adjust REGET_TIME_US, but it will degrade performance
//

#ifdef __x86_64__
#define RDTSC() ({ register uint32_t a,d; __asm__ __volatile__( "rdtsc" : "=a"(a), "=d"(d)); (((uint64_t)a)+(((uint64_t)d)<<32)); })
#else
#define RDTSC() ({ register uint64_t tim; __asm__ __volatile__( "rdtsc" : "=A"(tim)); tim; })
#endif

// atomic operations
#ifdef __x86_64__
    #define ASUFFIX "q"
#else
    #define ASUFFIX "l"
#endif

#define XCHG(ptr, val) __asm__ __volatile__("xchg"ASUFFIX" %2,%0" :"+m"(*ptr), "=r"(val) :"1"(val))
#define AADD(ptr, val) __asm__ __volatile__("lock ; add"ASUFFIX" %1,%0" :"+m" (*ptr) :"ir" (val))
#define CAS(ptr, val_old, val_new)({ char ret; __asm__ __volatile__("lock; cmpxchg"ASUFFIX" %2,%0; setz %1": "+m"(*ptr), "=q"(ret): "r"(val_new),"a"(val_old): "memory"); ret;})


#define REGET_TIME_US_GTOD   1
#define REGET_TIME_US_TIME   1
#define CPU_SPEED_GB    1  // assume a 1GB CPU

namespace Time
{
	//same as gettimeofday
	int now(struct timeval *tv)
	{
		static volatile uint64_t walltick;
		static volatile struct timeval walltime;
		static volatile long lock = 0;
		const unsigned int max_ticks = CPU_SPEED_GB * 1000 * REGET_TIME_US_GTOD;

		if (walltime.tv_sec == 0 || (RDTSC() - walltick) > max_ticks)
		{
			if (lock == 0 && CAS(&lock, 0UL, 1UL)) // try lock
			{
				gettimeofday((struct timeval*)&walltime, NULL);
				walltick = RDTSC();
				lock = 0; // unlock
			}
			else // try lock failed, use system time
			{
				return gettimeofday(tv, NULL);
			}
		}
		memcpy(tv, (void*)&walltime, sizeof(struct timeval));
		return 0;
	}

	// same algorithm with gettimeofday, except with different precision defined as REGET_TIME_US_TIME
	time_t now()
	{
		static volatile uint64_t walltick = 0;
		static volatile struct timeval walltime;
		static volatile long lock = 0;
		const unsigned int max_ticks = CPU_SPEED_GB * 1000 * REGET_TIME_US_TIME;

		if (walltime.tv_sec == 0 || (RDTSC() - walltick) > max_ticks)
		{
			if (lock == 0 && CAS(&lock, 0UL, 1UL)) // try lock
			{
				now((struct timeval*)&walltime);
				walltick = RDTSC();
				lock = 0; // unlock
			}
			else // try lock failed, use system time
			{
				struct timeval tv;
				now(&tv);
				return tv.tv_sec;
			}
		}
		return walltime.tv_sec;
	}
	uint64_t now_ms()
	{
		static volatile uint64_t walltick = 0;
		static volatile struct timeval walltime;
		static volatile long lock = 0;
		const unsigned int max_ticks = CPU_SPEED_GB * 1000 * REGET_TIME_US_TIME;

		if (walltime.tv_sec == 0 || (RDTSC() - walltick) > max_ticks)
		{
			if (lock == 0 && CAS(&lock, 0UL, 1UL)) // try lock
			{
				now((struct timeval*)&walltime);
				walltick = RDTSC();
				lock = 0; // unlock
			}
			else // try lock failed, use system time
			{
				struct timeval tv;
				now(&tv);
				return tv.tv_sec * 1000 + tv.tv_usec / 1000;
			}
		}
		return walltime.tv_sec * 1000 + walltime.tv_usec / 1000;
	}

	int extract(time_t timestamp, int32_t zone, int32_t* year, int32_t* month, int32_t* mday, int32_t* hour, int32_t* minute, int32_t* second)
	{
		timestamp += zone * 3600;
		struct tm tmresult;
		if (NULL == gmtime_r(&timestamp, &tmresult))
			return -1;
		if (NULL != year)
			*year = tmresult.tm_year + 1900;
		if (NULL != month)
			*month = tmresult.tm_mon + 1;
		if (NULL != mday)
			*mday = tmresult.tm_mday;
		if (NULL != hour)
			*hour = tmresult.tm_hour;
		if (NULL != minute)
			*minute = tmresult.tm_min;
		if (NULL != second)
			*second = tmresult.tm_sec;
		return 0;
	}
}
//////////////////////////////////////////////////////

//
// Optimized Attr_API based on opt_time()
// Add to a static counter and report once in a second
// Limitations:
//   1, attr should be a const value, you must not call Frequent_Attr_API(iAttr, 1) when iAttr differs for each call
//   2, should only be used for frequent reports rather than exception cases, since Frequent_Attr_API does not call Attr_API at once
//

// If you don't wish to call opt_time() frequently, supply your own current time
#define Frequent_Attr_API_Ext(attr, count, curtime) do { \
	static volatile time_t tPrevReport##attr = 0; \
	static volatile long iReportNum##attr = 0; \
	AADD(&iReportNum##attr, count); \
	if(tPrevReport##attr != curtime) \
	{ \
		long cnt = 0; \
		XCHG(&iReportNum##attr, cnt);\
		if(cnt) \
		{ \
			Attr_API(attr, cnt); \
		} \
		tPrevReport##attr = curtime; \
	} \
} while(0)

#define Frequent_Attr_API(attr, count) do { time_t t=opt_time(NULL); Frequent_Attr_API_Ext(attr, count, t); } while(0)


#ifdef TEST_TIME

#include <stdio.h>

int main(int argc, char *argv[])
{
	if(argc!=2)
	{
		printf("usage:\n");
		printf("       %s time           # test time performance\n", argv[0]);
		printf("       %s gettimeofday   # test gettimeofday performance\n", argv[0]);
		printf("       %s attr           # test attr api performance\n", argv[0]);
		return -1;
	}
	if(argv[1][0]=='g')
	{
		int i;
		struct timeval tv;
		for(i=0; i<100000000; i++)
		{
			Time::now(&tv);
			if(i%10000000==0)
				printf("%u-%u\n", tv.tv_sec, tv.tv_usec);
		}
	}
	else if(argv[1][0]=='t')
	{
		int i;
		time_t t;
		for(i=0; i<100000000; i++)
		{
			t = opt_time(NULL);
			if(i%10000000==0)
				printf("%u\n", t);
		}
	}
	else
	{
		int i;
		for(i=0; i<10000000; i++)
		{
			Frequent_Attr_API(51830, 1);
			Frequent_Attr_API(51831, 1);
			Frequent_Attr_API(51832, 1);
			Frequent_Attr_API(51833, 1);
			Frequent_Attr_API(51834, 1);
			Frequent_Attr_API(51835, 1);
			Frequent_Attr_API(51836, 1);
			Frequent_Attr_API(51837, 1);
			Frequent_Attr_API(51838, 1);
			Frequent_Attr_API(51839, 1);
		}
	}
}

#endif


