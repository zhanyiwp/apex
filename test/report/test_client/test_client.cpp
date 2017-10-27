#include "stdio.h"
#include "stdlib.h"
#include <unistd.h>
#include "util_report.h"

int main(int argc, char** argv)
{
	int i = 0;
	char serv[256];
	char attr[256];
	char serv_str[256];
	char attr_str[256];
	while(1)	//1s 1000 times
	{
		printf("Input: ServiceName AttrName RepeatTimes(exit when < 0) \n");
		scanf("%s%s%d", serv, attr, &i);
		printf("Echo: ServiceName(%s) AttrName(%s) RepeatTimes(%i) \n", serv, attr, i);

		if (i < 0)
		{
			break;
		}

		for (int k = 0; k < i; k++)
		{
			snprintf(serv_str, sizeof(serv_str), "Inc_%s_%d", serv, k / 10);
			snprintf(attr_str, sizeof(attr_str), "Inc_%s_%d", attr, k % 10);

			Report_Inc(serv_str, attr_str, 1);
		}

		for (int k = 0; k < i; k++)
		{
			snprintf(serv_str, sizeof(serv_str), "Set_%s_%d", serv, k / 10);
			snprintf(attr_str, sizeof(attr_str), "Set_%s_%d", attr, k % 10);

			Report_Set(serv_str, attr_str, k);
		}

		usleep(1000);
	}
	return 0;
}
