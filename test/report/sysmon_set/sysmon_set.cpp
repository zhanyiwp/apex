#include <stdio.h>
#include <stdlib.h>
#include "attr_report.h"

static void usage()
{
	printf("Usage: 1 ServiceName AttrName1 Inc_Value\n");
	printf("Usage: 2 ServiceName AttrName1 Set_Value\n");		
	printf("Usage: 3 ServiceName AttrName1 Alarm_Text\n");			
}

int main(int argc, char** argv)
{
	if (argc < 5)
	{
		usage();
		return 0;
	}
	
	int ret = 0;
	
	int iType = atoi(argv[1]);
	switch(iType)
	{
	case 1:
		ret = Report::inc(argv[2], argv[3], strtoul(argv[4], NULL, 10));
		break;
	case 2:
		ret = Report::set(argv[2], argv[3], Report::ST_FIRST, strtoul(argv[4], NULL, 10));	
		break;
	case 3:
		ret = Report::alarm(argv[2], argv[3], argv[4]);
		break;
	default:
		usage();
		return -1;
	}

	if (ret != 0)
	{
		printf("[RET_ERR] %s|%s|%s|%d\n", argv[2], argv[3], argv[4], ret);
	}

	return 0;
}
