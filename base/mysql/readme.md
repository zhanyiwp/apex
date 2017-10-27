基于mariadb c client api封装的异步mysql接口 
支持：
1.字符集设置
2.自动重连
3.任务超时
4.每个任务独立设置回调

// todo 结果集处理 bind type

#########################################################################
组织结构:
async_mysql.h-------------mysql操作接口类，提供对外访问,将mysql操作放入task任务队列
async_mtask.h-------------用户请求的mysql操作 任务队列管理
async_mconn.h-------------连接管理类,对mariadb c client api进行libev封装 ,处理各种事件,将任务队列中的sql发送值mysql server 
async_result.h------------结果处理类封装 提供对mysql 结果集的访问操作

#########################################################################
使用方法:
1.实例化nb_mysql类并调用init进行初始化。一个实例一个mysql连接 不要在多线程中使用同一个实例，每个线程一个独立实例才是安全的
2.调用simple_query 或者query执行sql语句,simple_query 用于不需要返回结果的sql语句，query用于需要返回结果的查询
	int simple_query(void *data,simple_query_result_cb cb,bool auto_id,int timeout,const char* pszFormat, ...);
	int query(void *data,query_result_cb cb,int mill_timeout,const char* pszFormat, ...);
	data 为用户数据，timout为超时时间单位毫秒，如果为0 则不设置超时，cb 为结果回调函数 ,如果是多线程环境 请注意cb的线程安全,cb的原型为
	typedef void (*simple_query_result_cb) (int ret,char *err,int64_t effect_rows, uint64_t insert_id,void *data);
	typedef void (*query_result_cb) (int ret, char *err,asyncmysql::result_set *resultset, void *data);
3.根据业务编写回调函数处理结果
#########################################################################
example
参考test/mysql_async

#########################################################################
关于DNS域名解析
如果使用hostname 由于dns解析是阻塞操作，因此可能会阻塞调用，建议的方式是直接使用ip连接数据库 如果使用域名 可以在本地配置其域名解析缓存

When mysql_real_connect_start() is passed a hostname (as opposed to a local unix socket or an IP address, it may need to look up the hostname in DNS, depending on local host configuration (e.g. if the name is not in /etc/hosts or cached). Such DNS lookups do not happen in a non-blocking way. This means that mysql_real_connect_start() will not return control to the application while waiting for the DNS response. Thus the application may "hang" for some time if DNS is slow or non-functional.

If this is a problem, the application can pass an IP address to mysql_real_connect_start() instead of a hostname, which avoids the problem. The IP address can be obtained by the application with whatever non-blocking DNS loopup operation is available to it from the operating system or event framework used. Alternatively, a simple solution may be to just add the hostname to the local host lookup file (/etc/hosts on Posix/Unix/Linux machines).