#ifndef SOCKET_FUNC_H_
#define SOCKET_FUNC_H_

#include <string.h>
#include <stdlib.h>
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
#include <netdb.h>
#include <errno.h>
#include <string>
#include <stdio.h>
#include "misc_func.h"

typedef int SOCKET_RESULT;
#define SOCKET_SUCCESS(x) (x!=SOCKET_ERROR)
#define SOCKET_FAIL(x) (x==SOCKET_ERROR)

using namespace std;

/**
	* get local machine ip(exlude 127.0.0.1)
	* @param ip_list	the machine ip will be stored here
	* @param max_count	the max length of ip_list
	* @return the count of ip. return -1 on error
	*/
int32_t get_machine_ip_list(uint32_t* ip_list, uint32_t max_count);
/*
	* create a simple tcp socket.
	* return the fd on success. otherwise, INVALID_SOCKET was returned
	*/
inline SOCKET create_tcp_sock()
{
	return socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
}

/*
	* create a simple udp socket.
	* return the fd on success. otherwise, INVALID_SOCKET was returned
	*/
inline SOCKET create_udp_sock()
{
	return socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
}

/*
	* close socket
	*/
inline SOCKET_RESULT close_sock(SOCKET s)
{
	return close(s);
}

/*
	* shut down socket
	*/
inline SOCKET_RESULT shutdown_sock(SOCKET s, int how)
{
	return shutdown(s, how);
}

/*
	* set NONBLOCK for a sock fd.
*/
inline SOCKET_RESULT set_nonblock_sock(SOCKET fd, unsigned int nonblock)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0)
		return SOCKET_ERROR;

	if(nonblock>0)
		return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	else
		return fcntl(fd, F_SETFL, flags & (~O_NONBLOCK));
}

/*
	* check whether error occured when connect.
	* @param fd : a file desciptor on which connect() was called
	* it's for use in non-blocking mode.
	* errno will be set to a value describing what error happened when connect
*/
inline SOCKET_RESULT is_sock_connected(SOCKET fd)
{
	int error = 0;
	socklen_t len = sizeof(int);
	if(0 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len))
	{
		if(0==error)
			return 0;
		else
		{
			errno=error;
			return SOCKET_ERROR;
		}
	}
	return SOCKET_ERROR;
}

/**
	* get ip address of a host name
	* param @name	a host name or domain name
	* return the ip value in net-byte-order. return 0 if error occured.
	*/
uint32_t get_host_by_name(const char* name);

/*
	* set ip and port for a sockaddr_in structure
	* on success, zero is returned. otherwise, SOCKET_ERROR(-1) is returned.
	*/
inline SOCKET_RESULT set_sock_addr(struct sockaddr_in* addr, const char* ip, unsigned short port)
{
	memset(addr, 0, sizeof(struct sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);
	//addr->sin_addr.s_addr = inet_addr(ip);
	//return 0;
	return (0==inet_aton(ip, &(addr->sin_addr)))?SOCKET_ERROR : 0;
}

inline void set_sock_addr(struct sockaddr_in* addr, uint32_t iph, unsigned short porth)
{
	memset(addr, 0, sizeof(struct sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_port = htons(porth);
	addr->sin_addr.s_addr = htonl(iph);	
}
	
inline SOCKET_RESULT get_local_sock_addr(SOCKET fd, struct sockaddr_in* addr)
{
	socklen_t addr_len = sizeof(struct sockaddr_in);
	return (0==getsockname(fd, (struct sockaddr*)(addr), &addr_len)) ? 0 : SOCKET_ERROR;
}

inline SOCKET_RESULT get_peer_sock_addr(SOCKET fd, struct sockaddr_in* addr)
{
	socklen_t addr_len = sizeof(struct sockaddr_in);
	return (0==getpeername(fd, (struct sockaddr*)(addr), &addr_len)) ? 0 : SOCKET_ERROR;
}

/*
	* get binary value of ip in net byte order from a dotted string
	*/
inline SOCKET_RESULT ipn_by_string(const char* cp, unsigned int *ip)
{
	struct sockaddr_in addr;
	if(0==inet_aton(cp, &(addr.sin_addr)))
		return SOCKET_ERROR;
	*ip = (unsigned int)(addr.sin_addr.s_addr);
	return 0;
}

inline string string_by_addr(const struct sockaddr_in* addr)
{
	char ip[20];
	if (!inet_ntop(AF_INET, (void *)&addr->sin_addr, ip, 16))
	{
		return "";
	}
	char text[32];
	snprintf(text, sizeof(text), "%s:%u", ip, ntohs(addr->sin_port));
	return string(text);
}

/**
	* get ip string from a net byte order ip value
	*/
inline string string_by_ipn(uint32_t nbo_ip)
{
	char ip[20];
	if (!inet_ntop(AF_INET, (void *)&nbo_ip, ip, 16))
	{
		return "";
	}
	return string(ip);
}

inline string string_by_iph(uint32_t hbo_ip)
{
	return string_by_ipn(htonl(hbo_ip));
}
/*
	* get binary value of ip in host byte order from a sockaddr_in structure
	*/
inline unsigned int iph_by_addr(const struct sockaddr_in* addr)
{
	return (unsigned int)(ntohl(addr->sin_addr.s_addr));
}
/*
	* get binary value of ip in net byte order from a sockaddr_in structure
	*/
inline unsigned int ipn_by_addr(const struct sockaddr_in* addr)
{
	return (unsigned int)(addr->sin_addr.s_addr);
}

/*
	* get binary value of port in host byte order from a sockaddr_in structure
	*/
inline unsigned short porth_by_addr(const struct sockaddr_in* addr)
{
	return ntohs( addr->sin_port );
}
/*
	* get binary value of port in net byte order from a sockaddr_in structure
	*/
inline unsigned short portn_by_addr(const struct sockaddr_in* addr)
{
	return addr->sin_port;
}

/*
	* bind a socket fd to specified ip and port.
	*/
inline SOCKET_RESULT bind_sock(SOCKET fd, struct sockaddr_in* addr)
{
	return bind(fd, (struct sockaddr *)addr, sizeof(struct sockaddr_in));
}

/*
	* start listen on a server socket
	*/
inline SOCKET_RESULT listen_sock(SOCKET fd, int backlog)
{
	return listen(fd, backlog);
}

/*
	* socket accept.
	* this function just called accept() and return the return value of accept().
	*/
inline SOCKET accept_sock(SOCKET fd, struct sockaddr_in* addr)
{
	socklen_t l = sizeof(struct sockaddr_in);
	return accept(fd, (struct sockaddr*)addr, &l);
}
/*
	* conect.
	*/
inline SOCKET_RESULT connect_sock(SOCKET fd, struct sockaddr_in* addr)
{
	return connect(fd, (struct sockaddr*)addr, sizeof(struct sockaddr_in));
}

/*
	* set TCP_DEFER_ACCEPT option for a fd.
	*/
SOCKET_RESULT set_defer_accept_sock(SOCKET fd, int defer);

/*
	* set TCP_NO_DELAY option for a fd.
	*/
SOCKET_RESULT set_no_delay_sock(SOCKET fd, int no_delay);

/*
	* set SO_LINGER option for a fd.
	* you can specify the linger timeout width the second param.
	*/
SOCKET_RESULT set_linger_sock(SOCKET fd, int on, int timeout);

/*
	* get SO_SNDBUF option for a fd.
	*/
inline SOCKET_RESULT get_send_buflen_sock(SOCKET fd, int *buf)
{
	socklen_t len = sizeof(buf);
	return getsockopt(fd,SOL_SOCKET,SO_SNDBUF,&buf,&len);
}

/*
	* set SO_SNDBUF option for a fd.
	*/
inline SOCKET_RESULT set_send_buflen_sock(SOCKET fd, int buf)
{
	return setsockopt(fd,SOL_SOCKET,SO_SNDBUF,(char*)&buf,sizeof(buf));
}

/*
	* get SO_RCVBUF option for a fd.
	*/
inline SOCKET_RESULT get_recv_buflen_sock(SOCKET fd, int *buf)
{
	socklen_t len = sizeof(buf);
	return getsockopt(fd,SOL_SOCKET,SO_RCVBUF,&buf,&len);
}

/*
	* set SO_RCVBUF option for a fd.
	*/
inline SOCKET_RESULT set_recv_buflen_sock(SOCKET fd, int buf)
{
	return setsockopt(fd,SOL_SOCKET,SO_RCVBUF,(char*)&buf,sizeof(buf));
}

/**
	* set keep alive option for a fd
	*/
inline SOCKET_RESULT set_keep_alive_sock(SOCKET fd, int keep_alive_interval)
{
	return setsockopt(fd,SOL_SOCKET,SO_KEEPALIVE,(char*)&keep_alive_interval,sizeof(keep_alive_interval));
}

/*
	* set REUSEADDR for fd.
	* param reuse: 1 set REUSEADDR to true, 0 set to false.
	* on success, zero is returned. otherwise, SOCKET_ERROR(-1) is returned.
	*/
SOCKET_RESULT set_reuse_addr_sock(SOCKET fd, int reuse);

/*
	* create a common tcp server socket for accept
	* note: the svr socket will set DEFER_ACCEPT automaticlly on linux.
	* on success, the socket fd is returned. otherwise, INVALID_SOCKET is returned.
	*/
SOCKET create_artery_listen_sock(int non_block, int no_delay, int reuse_addr, int defer_accept);


#endif //SOCKET_FUNC_H_
