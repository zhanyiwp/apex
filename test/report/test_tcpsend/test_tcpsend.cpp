#include <arpa/inet.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include "stdio.h"
#include "stdlib.h"
#include <arpa/inet.h>
#include <time.h>

#define MAX_PKG_LEN (500)

int main(int argc, char *argv[])
{
	if (argc != 7)
	{
		printf("Usage: ServerIP ServerPort LocalHostIP ServiceName AttrName ValueSet\n");
		return -1;
	}

	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(argv[1]);
	server_addr.sin_port = htons((uint16_t)strtoul(argv[2], NULL, 10));
	int s_udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (s_udp_fd < 0)
	{
		printf("Create Socket Failed.\n");
		return -1;
	}

	uint32_t tMin = (uint32_t)((time(NULL) / 60) * 60);// +8 * 3600;//EST-8

	char szPkg[MAX_PKG_LEN * 10];
	uint64_t u64Time = tMin;
	u64Time *= 1000000000;
	int iLen = snprintf(szPkg, sizeof(szPkg), "%s,host=%s %s=%u,attr1=1,attr2=2 %lu", argv[4], argv[3], argv[5], strtoul(argv[6], NULL, 10), u64Time);
//	int iLen = snprintf(szPkg, sizeof(szPkg), "%s,host=%s %s=%u,attr1=1,attr2=2", argv[4], argv[3], argv[5], strtoul(argv[6], NULL, 10));

	if (sendto(s_udp_fd, szPkg, iLen, 0, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("Send File Name Failed.\n");
	}

	return 0;
}

