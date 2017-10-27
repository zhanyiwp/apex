#include <arpa/inet.h>
#include <signal.h>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <iostream>

#include "mmap_queue.h"
#include "opt_time.h"
#include "wbl_config_file.h"
#include "wbl_timer.h"
#include "wbl_comm.h"
#include "tc_ex.h"

#include "byteios.h"
#include "public_define.h"

using namespace std;
using namespace taf;
using namespace wbl;

typedef struct 
{
	string ConfigFile;
	uint32_t ServerPort;
} CONFIG;

CONFIG stConfig;
int s_udp_fd = -1;

static int Init(int argc, char *argv[])
{
	CONFIG *pstConfig = &stConfig;
	if (argc < 2)
	{
		printf("Usage: %s listen_port\n", argv[0]); 
		exit(0);
	}
		
	pstConfig->ServerPort = strtoul(argv[1], NULL, 10);

	if(pstConfig->ServerPort == 0)
	{
		printf("[error] config parameter error.\n");
		return -1;
	}

	s_udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (s_udp_fd == -1)
	{
		printf("Create Socket Failed.\n");
		return -1;
	}

	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons((uint16_t)pstConfig->ServerPort);
	if (-1 == (bind(s_udp_fd, (struct sockaddr*)&server_addr, sizeof(server_addr))))
	{
		printf("Server Bind Failed.\n");
		return -1;
	}
		
	return 0;
}

#define MAX_PKG_RECVLEN (1000)
int main(int argc, char *argv[])
{
	if (Init(argc, argv)) 
    { 
        printf("[error] Initialize failed.\n");
        return -1; 
    }
	
//	Daemon();
	int8_t iCount = 0;
	char buffer[MAX_PKG_RECVLEN];
	while (1)
	{
		struct sockaddr_in client_addr;
		socklen_t client_addr_length = sizeof(client_addr);
		
		int8_t iLen = recvfrom(s_udp_fd, buffer, MAX_PKG_RECVLEN, 0, (struct sockaddr*)&client_addr, &client_addr_length);
		if (iLen < 0)
		{
			printf("Receive Data Failed:\n");
			continue;
		}

		buffer[iLen] = 0;
		printf("[%u] %s\n", ++iCount, buffer);
	}

	return 0;
}

