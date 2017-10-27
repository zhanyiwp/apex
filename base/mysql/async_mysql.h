#include "async_mconn.h"
#pragma once

namespace asyncmysql
{
	class nb_mysql
	{
		public:
			/**
			* @brief nb_mysql 
			* @param [in]	max_sql_len 单条mysql语句的最大长度 上限20M
			*/
			nb_mysql(int max_sql_len);
			~nb_mysql();
			/**
			* @brief init 
			* @param [in]	ip		mysql ip
			* @param [in]	port		mysql port
			* @param [in]	passwd		mysql password
			* @param [in]	dbname		
			* @param [in]	character	字符集 为空时不设置采用默认字符集	
			* return 成功时返回0 失败返回小于0 
			*/
			int init(string &ip,int port,string &user,string &passwd,string dbname,string &character,struct ev_loop *loop);
			
			/**
			* @brief simple_query 		简单查询接口，用于不需要返回复杂结果集sql语句比如insert update delete 已经其他命令式语句等
			* @param [in]	data		用户数据
			* @param [in]	cb		结果回调函数
			* @param [in]	auto_id		是否需要返回insert_id 即mysql_insert_id
			* @param [in]	mill_timeout	超时时间	
			* @param [in]	pszFormat	sql语句	
			* return 成功加入任务队列时返回0 失败返回小于0 
			* note
			* 回调函数原型：
			* typedef void (*simple_query_result_cb) (int ret,char *err,int64_t effect_rows, uint64_t insert_id,void *data);
			* ret 执行结果，执行成功ret=0 执行失败 ret > 0  执行超时，ret < 0
			* err 执行失败时返回错误原因
			* effect_rows 执行成功是返回影响行数
			* insert_id simple_query调用 auto_id = true 时有效 返回mysql当前的自增id
			* data 用户数据
			*/
			int simple_query(void *data,simple_query_result_cb cb,bool auto_id,int mill_timeout,const char* pszFormat, ...);
			
			/**
			* @brief query 			查询接口，用于select等需要返回复杂结果集的查询
			* @param [in]	data		用户数据
			* @param [in]	cb		结果回调函数
			* @param [in]	mill_timeout	超时时间	
			* @param [in]	pszFormat	sql语句	
			* return 成功加入任务队列时返回0 失败返回小于0 
			* note
			* 回调函数原型：
			* typedef void (*query_result_cb) (int ret, char *err,asyncmysql::result_set *resultset, void *data);
			* ret 执行结果，执行成功ret=0 执行失败 ret > 0  执行超时，ret < 0
			* err 执行失败时返回错误原因
			* resultset 结果集指针，获取结果
			* data 用户数据
			*/
			int query(void *data,query_result_cb cb,int mill_timeout,const char* pszFormat, ...);
			
			//关闭mysql连接
			int close();
		private:
			int _max_sql_len;
			char *_sql_buf;
			mconn _conn;
	};
}
