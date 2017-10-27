#include <mysql.h>
#include <stdint.h>
#include <assert.h>
#include <ev.h>
#include <sys/queue.h>
#include <map>
#include <string>
#include <stdio.h>
#include "period_timer.h"
#include "async_mtask.h" 
#include "async_mresult.h"

#pragma once
using namespace std;

#define util_offsetof(Type, Member) ( (size_t)( &(((Type*)8)->Member) ) - 8 )

namespace asyncmysql
{
    enum conn_state
    {
        NO_CONNECTED = 1,
        CONNETING,
        CONNECT_WAITING,
        SET_CHARACTER_WAITING,
        CONNECT_WRITE_ABLE,
        SIMPLE_QUERY_WAITING,
        QUERY_WAITING,
        PING_WATING,
        STORE_WAITING,
        CLOSE_WAITING,
        CLOSE_FINISH
    };

	class mconn
	{
        public:
        mconn();
        ~mconn();

        static inline mconn* get_instance_via_watcher(ev_io* watcher)
        {   
            return reinterpret_cast<mconn*>( reinterpret_cast<uint8_t*>(watcher) - util_offsetof(mconn, _watcher) );  
        }

        int init(string &ip,int port,string &user,string &passwd,string dbname,string &character, struct ev_loop *loop);
        
        int event_to_mysql_status(int event);
        int mysql_status_to_event(int status);

        int state_handle(struct ev_loop *loop, ev_io *watcher, int event);
        int next_state(conn_state state,int status,bool stop_io = true);

        int connect_start();
        int connect_wait(struct ev_loop *loop, ev_io *watcher, int event);
        
        int set_character_start();
        int set_character_wait(struct ev_loop *loop, ev_io *watcher, int event);

        int simple_query_start(char *sql, int sqllen);
        int simple_query_wait(struct ev_loop *loop, ev_io *watcher, int event);

        int query_start(char *sql, int sqllen);
        int query_wait(struct ev_loop *loop, ev_io *watcher, int event);

        int ping_start();
        int ping_wait(struct ev_loop *loop, ev_io *watcher, int event);

        int store_result_start();
        int store_result_wait(struct ev_loop *loop, ev_io *watcher, int event);

        int close_start();
        int close_wait(struct ev_loop *loop, ev_io *watcher, int event);
    private:
        void check_state(timer*);
        int ready_for_next_task();
        void next_task();
        void exec_task();

    private:
        string _ip;
		int _port;
		string _user;
		string _passwd;
		string _dbname;
		string _character;
        
        MYSQL *_mysql;
        MYSQL_RES *_queryres;
        MYSQL_ROW _row;

        ev_io _watcher;
        struct ev_loop *_loop;

        conn_state _state;
        task      *_curtask;
        timer *_check_state;
    public:
        task_list  *_task_list;
	};
}