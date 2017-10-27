#include "socket_func.h"
#include <net/if.h>
#include <sys/ioctl.h>

int32_t get_machine_ip_list(uint32_t* ip_list, uint32_t max_count)
{
	int sfd, intr;
	struct ifreq buf[16];
	struct ifconf ifc;
	sfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sfd < 0)
		return -1;
	uint32_t count = 0;
	do
	{
		ifc.ifc_len = sizeof(buf);
		ifc.ifc_buf = (caddr_t)buf;
		if(ioctl(sfd, SIOCGIFCONF, (char *)&ifc))
			break;
		intr = ifc.ifc_len / sizeof(struct ifreq);
		while (count<max_count && intr-- > 0 && 0==ioctl(sfd, SIOCGIFADDR, (char *)&buf[intr]))
		{
			uint32_t ip = ((struct sockaddr_in*)(&buf[intr].ifr_addr))->sin_addr.s_addr;
			if(ip==0x100007F)//skip 127.0.0.1
				continue;
			ip_list[count] = ip;
			++count;
		}
		close(sfd);
		return count;
	}while(0);
	close(sfd);
	return -1;
}

uint32_t get_host_by_name(const char* name)
{
	struct addrinfo *answer, hint;
	uint32_t ret_ip = 0;
	bzero(&hint, sizeof(hint));
	hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_STREAM;

	int ret = getaddrinfo(name, NULL, &hint, &answer);
	if (ret != 0)
		return 0;

	if(NULL!=answer)
		ret_ip = ((struct sockaddr_in *)(answer->ai_addr))->sin_addr.s_addr;
	freeaddrinfo(answer);
	return ret_ip;
}

SOCKET_RESULT set_defer_accept_sock(SOCKET fd, int defer)
{
	int val = defer!=0?1:0;
	int len = sizeof(val);

	return setsockopt(fd, /*SOL_TCP*/IPPROTO_TCP, TCP_DEFER_ACCEPT, (const char*)&val, len);

	/*check whether the option is set*/
	/*
	socklen_t tlen;
	if (getsockopt(fd, SOL_TCP, TCP_DEFER_ACCEPT, (char*)&val, &tlen) == -1)
		return -1;
	else if (val == 0)
		return -1;
	return 0;
	*/
}

SOCKET_RESULT set_no_delay_sock(SOCKET fd, int no_delay)
{
	int val = no_delay!=0?1:0;
	return setsockopt(fd, IPPROTO_TCP/*SOL_TCP*/, TCP_NODELAY, (const char *)(&val), sizeof(val));
}

SOCKET_RESULT set_linger_sock(SOCKET fd, int on, int timeout)
{
	struct linger slinger;

	slinger.l_onoff = on!=0?1:0;
	slinger.l_linger = timeout;

	return setsockopt (fd, SOL_SOCKET, SO_LINGER, (const char *)(&slinger), sizeof(slinger));
}

SOCKET_RESULT set_reuse_addr_sock(SOCKET fd, int reuse)
{
	int val = reuse>0?1:0;
	return setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, (const char *)(&val), sizeof(val));
}

SOCKET create_artery_listen_sock(int non_block, int no_delay, int reuse_addr, int defer_accept)
{
	 /* Request socket. */
	SOCKET s = create_tcp_sock();
	if (s==INVALID_SOCKET)
		return INVALID_SOCKET;

	/* set nonblock */
	if(non_block && SOCKET_FAIL(set_nonblock_sock(s,1)))
	{
		close_sock(s);
		return INVALID_SOCKET;
	}
	if(SOCKET_FAIL(set_linger_sock(s, 1, 0)))
	{
		close_sock(s);
		return INVALID_SOCKET;
	}

	/* set defer accept */
	if(defer_accept && SOCKET_FAIL(set_defer_accept_sock(s,defer_accept)))
	{
		close_sock(s);
		return INVALID_SOCKET;
	}

	/* set no delay */
	if(no_delay && SOCKET_FAIL(set_no_delay_sock(s,1)))
	{
		close_sock(s);
		return INVALID_SOCKET;
	}

	/* set reuse addr */
	if(reuse_addr && SOCKET_FAIL(set_reuse_addr_sock(s, 1)))
	{
		close_sock(s);
		return INVALID_SOCKET;
	}

	return s;
}
