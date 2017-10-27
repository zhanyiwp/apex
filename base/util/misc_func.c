#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>

#include <stdio.h>
#include <stdlib.h>

#include <iconv.h>
#include <zlib.h>
#include <assert.h>
#include "misc_func.h"

int strerr_r(int errnum, char* buf, unsigned int len)
{
	char* msg = strerror_r(errnum, buf, len);
	memcpy(buf,msg,len);
	return strlen(msg);
}

int set_file_limit(int max_fd)
{
	struct rlimit rlim;
	memset(&rlim, 0, sizeof(rlim));
	if(getrlimit((__rlimit_resource_t)RLIMIT_OFILE, &rlim) == 0)
	{
		if(rlim.rlim_cur < (unsigned)max_fd)
		{
			rlim.rlim_cur = max_fd;
			rlim.rlim_max = max_fd;
			if(setrlimit((__rlimit_resource_t)RLIMIT_OFILE, &rlim) == 0)
			{
				return 0;
			}
		}
		else
		{
			return 0;
		}
	}
	return -1;
}

void daemon_proc()
{
	pid_t pid;

	if ((pid = fork()) != 0)
		exit(0);

	setsid();

	signal(SIGINT,  SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	signal(SIGIO,   SIG_IGN);

	//ignore_pipe();
	struct sigaction sig;
	sig.sa_handler = SIG_IGN;
	sig.sa_flags = 0;
	sigemptyset(&sig.sa_mask);
	sigaction(SIGPIPE,&sig,NULL);

	if ((pid = fork()) != 0)
		exit(0);

	umask(0);
}

int set_core_limit(uint64_t max_size)
{
	struct rlimit rlim = {0,0};
	if(getrlimit((__rlimit_resource_t)RLIMIT_CORE, &rlim) == 0)
	{
		if(rlim.rlim_cur < (unsigned)max_size)
		{
			if(sizeof(rlim.rlim_cur)==4 && max_size>UINT32_MAX)
				max_size = UINT32_MAX;
			rlim.rlim_cur = max_size;
			rlim.rlim_max = max_size;
			if(setrlimit((__rlimit_resource_t)RLIMIT_CORE, &rlim) == 0)
			{
				return 0;
			}
		}
		else
		{
			return 0;
		}
	}
	return -1;
}

int reopen_std_file()
{
	if(NULL==freopen("/dev/null", "a+", stdin))
		return -1;
	if(NULL==freopen("/dev/null", "a+", stdout))
		return -1;
	if(NULL==freopen("/dev/null", "a+", stderr))
		return -1;
	return 0;
}

void ignore_sigpipe()
{
	struct sigaction sa;
	bzero(&sa, sizeof(sa));
	sa.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sa, NULL);
}

void print_dump(const void* mem, unsigned int len)
{
	printf("dump memory, base %p, length %u\n", mem, len);
	unsigned char* base = (unsigned char*)(mem);
	unsigned char ch;
	unsigned int i=0, m=0;
	char szLine[66] = {0};//3*16 bytes for hex text, 1 byte for \t, 16 bytes for ascii, 1 byte for \0 temernitor.
	for(i=0; i<len; i+=16)
	{
		memset((void*)szLine, 0, sizeof(szLine));
		for(m=0; m<16; ++m)
		{
			ch = *(base+i+m);
			if(i+m>=len)
			{
				snprintf(szLine + 3 * m, 4, "-- ");
				szLine[3*16+1+m] = ' ';
			}
			else
			{
				snprintf(szLine + 3 * m, 4, "%02x ", ch);
				if(ch>=32 && ch<=126)
					szLine[3*16+1+m] = ch;
				else
					szLine[3*16+1+m] = ' ';
			}
		}
		szLine[3*16] = '\t';
		puts(szLine);
	}
}

int iconv(char* to_str, size_t to_str_len, const char* to_code, const char* from_str, size_t from_str_len, const char* from_code)
{
	iconv_t cd = iconv_open(to_code, from_code);
	if ((iconv_t)(-1) == cd)
		return -1;

	size_t old_to_str_len = to_str_len;
	if (((size_t)-1) != iconv(cd, (char**)(&from_str), &from_str_len, &to_str, &to_str_len))
	{
		iconv_close(cd);
		return old_to_str_len - to_str_len;
	}
	iconv_close(cd);
	return -1;
}

void* iconv_open(const char* to_code, const char* from_code)
{
	iconv_t cd = iconv_open(to_code, from_code);
	if ((iconv_t)(-1) == cd)
		return NULL;
	return cd;
}

int iconv_conv(void* cd, char* to_str, size_t to_str_len, const char* from_str, size_t from_str_len)
{
	size_t old_to_str_len = to_str_len;
	if (((size_t)-1) != iconv(cd, (char**)(&from_str), &from_str_len, &to_str, &to_str_len))
		return old_to_str_len - to_str_len;
	return -1;
}

unsigned int buff_hash_djb2(const unsigned char *str, int len)
{
	uint32_t hash = 5381;
	int c, i;

	for(i=0;i<len;++i)
	{
		c=str[i];
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	}

	return hash;
}

unsigned int buf_hash_sdbm(const unsigned char *str, int len)
{
	uint32_t hash = 0;
	int c, i;

	for(i=0;i<len;++i)
	{
		c=str[i];
		hash = c + (hash << 6) + (hash << 16) - hash;
	}

	return hash;
}

uint16_t crc16(uint16_t* buf, int nwords)
{
	uint32_t sum = 0;
	while(nwords>0)
	{
		sum += *buf++;
		--nwords;
	}
	sum = (sum >> 16) + (sum &0xffff);
	sum += (sum >> 16);
	return (uint16_t)(~sum);
}

int zlib_zip(uint8_t* output, uint32_t output_max_len, uint8_t* input, uint32_t input_len)
{
	assert(0);
// 	int ret, output_len;
// 	z_stream strm;
// 	strm.zalloc = Z_NULL;
// 	strm.zfree = Z_NULL;
// 	strm.opaque = Z_NULL;
// 	ret = deflateInit(&strm, 9);//leve is -1 to 9
// 	if (ret != Z_OK)
// 		return -1;
// 	strm.avail_in = input_len;
// 	strm.next_in = input;
// 	strm.avail_out = output_max_len;
// 	strm.next_out = output;
// 	ret = deflate(&strm, Z_FINISH);
// 	if (ret == Z_STREAM_ERROR)
// 		return -1;
// 	output_len = output_max_len - strm.avail_out;
// 	deflateEnd(&strm);
// 	if (strm.avail_in == 0 && ret == Z_STREAM_END)
// 		return output_len;
	return -1;
}

int zlib_unzip(uint8_t* output, uint32_t output_max_len, uint8_t* input, uint32_t input_len)
{
	assert(0);
// 	int ret, output_len;
// 	z_stream strm;
// 	strm.zalloc = Z_NULL;
// 	strm.zfree = Z_NULL;
// 	strm.opaque = Z_NULL;
// 	ret = inflateInit(&strm);
// 	if (ret != Z_OK)
// 		return -1;
// 	strm.avail_in = input_len;
// 	strm.next_in = input;
// 	strm.avail_out = output_max_len;
// 	strm.next_out = output;
// 	ret = inflate(&strm, Z_FINISH);
// 	if (ret == Z_STREAM_ERROR)
// 		return -1;
// 	output_len = output_max_len - strm.avail_out;
// 	inflateEnd(&strm);
// 	if (strm.avail_in == 0 && ret == Z_STREAM_END)
// 		return output_len;
	return -1;
}