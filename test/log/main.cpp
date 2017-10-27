#include "util.h"
#include "net.h"

int main(int argc, char const *argv[])
{
	if (2 > argc)
	{
		exit(-1);
	}
	
	Config::section conf(argv[1], "system");
	
	string		ip;
	uint16_t	port = 0;
	conf.get("ip", ip);
	conf.get("port", port);
	
	printf("\nip: %s, port: %u\n", ip.c_str(), port);

	//可选，默认内部会自动设置为进程名
	Log::set_file_name("custom_test.log", 1);
	//可选，默认为程序运行路径所在目录	
	Log::set_local_cfg(Log::LL_TRACE, NULL, false, 200 * 1024 * 1024, 4);
	//可选。不小于其参数值执行远端写log
	Log::set_remote_cfg(Log::LL_NONE);	
	
	Log::Local::trace("network", "line1");
	Log::Local::info("network", "line2 %d", 1);
	Log::Local::warn("network", "line3 %d %s", 2, "param invalid");
	Log::Local::error("network", "line4 %d %s %u", 3, "loop error", 20);
	Log::Local::fatal("network", "line5 %d %s %u %f", 4, "exit", 100, 4.9f);		

	Net::run(EV_DEFAULT);

	return 0;
}

