#pragma once
#include <string>
#include <stdint.h>
#include <map>
#include "async_mresult.h"
#include "period_timer.h"

using namespace std;

// ret = 0 成功 < 0 超时 > 0 失败
typedef void (*simple_query_result_cb) (int ret,char *err,int64_t effect_rows, uint64_t insert_id,void *data);
typedef void (*query_result_cb) (int ret, char *err,asyncmysql::result_set *resultset, void *data);

namespace asyncmysql
{
    enum task_type
    {
        SIMPLE_QUERY,
        INSERT,
        QUERY,
        CLOSE
    };

    typedef struct _task
    {
        string sql;
        int timeout;
        uint64_t id;
        void *data;
        void *cb;
        int type;
        bool _btimeout;
    }task;

	class task_list
	{
        public:
            task_list(struct ev_loop* loop);
            ~task_list();
            int add_task(const string &sql,void *data,void *cb,task_type type,int timeout);
            task* fetch_task();
            int del_task(uint64_t id);
            void task_timeout(timer* t);
        private:
            uint64_t alloc_id(){return ++id;}
            map<uint64_t,task*> map_task;
            map<uint64_t,timer*> map_timer; 
            uint64_t id;
            struct ev_loop* _loop;
	};
}
