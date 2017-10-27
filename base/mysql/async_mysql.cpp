#include "async_mysql.h"
#include <stdarg.h>

using namespace asyncmysql;

#define MAX_SQL_LEN 20 * 1024 * 1024 // sql 语句长度上限阀值20M

nb_mysql::nb_mysql(int max_sql_len)
:_max_sql_len(max_sql_len)
,_sql_buf(NULL)
{

}

nb_mysql::~nb_mysql()
{
	if(_sql_buf)
	{
		delete _sql_buf;
		_sql_buf = NULL;
	}
}

int nb_mysql::init(string &ip,int port,string &user,string &passwd,string dbname,string &character,struct ev_loop *loop)
{
	if(_max_sql_len < 0 || _max_sql_len > MAX_SQL_LEN)
		_max_sql_len = MAX_SQL_LEN;
	_sql_buf = new char[_max_sql_len];
	memset(_sql_buf,0,_max_sql_len);
	return _conn.init(ip,port,user,passwd,dbname,character,loop);
}

int nb_mysql::simple_query(void *data,simple_query_result_cb cb,bool auto_id,int mill_timeout,const char* pszFormat, ...)
{
	va_list argList;
	va_start( argList, pszFormat );
	int ret = vsnprintf(_sql_buf, _max_sql_len, pszFormat, argList);
	if(ret < 0)
	{
		printf("exec simple_query failed \n");
		return -1;
	}
	va_end( argList );
	task_type type = SIMPLE_QUERY;
	if(auto_id)
	{
		type = INSERT;
	}
	return _conn._task_list->add_task(_sql_buf,data,(void*)cb,INSERT,mill_timeout);
}

int nb_mysql::query(void *data,query_result_cb cb,int mill_timeout,const char* pszFormat, ...)
{
	va_list argList;
	va_start( argList, pszFormat );
	int ret = vsnprintf(_sql_buf, _max_sql_len, pszFormat, argList);
	if(ret < 0)
	{
		printf("exec simple_query failed \n");
		return -1;
	}
	va_end( argList );
	return _conn._task_list->add_task(_sql_buf,data,(void*)cb,QUERY,mill_timeout);
}

int nb_mysql::close()
{
	return _conn._task_list->add_task("",NULL,NULL,CLOSE,0);
}
