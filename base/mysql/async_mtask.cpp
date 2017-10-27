#include "async_mtask.h"

using namespace asyncmysql;

task_list::task_list(struct ev_loop* loop)
:id(0)
,_loop(loop)
{

}

task_list::~task_list()
{

}

int task_list::add_task(const string &sql,void *data,void *cb,task_type type,int timeout)
{
    task *cur = new task();
    cur->sql = sql;
    cur->id = alloc_id();
    cur->data = data;
    cur->cb = cb;
    cur->type = type;
    cur->timeout = timeout;
    cur->_btimeout = false;
    map_task[cur->id] = cur;
    if(cur->timeout > 0)
    {
        timer* t = new timer(_loop);
		map_timer[cur->id] = t;
		t->set(this, &task_list::task_timeout,(uint32_t)cur->timeout,false, (void**)cur->id);
        t->start();
    }
    return 0;
}

task *task_list::fetch_task()
{
    task *cur = NULL;
    map<uint64_t,task*>::iterator it = map_task.begin();
    if(it != map_task.end())
    {
        cur = it->second;
    }
    return cur;
}

int task_list::del_task(uint64_t id)
{
    map<uint64_t,timer*>::iterator it = map_timer.find(id);
    if(it != map_timer.end())
    {
        if(it->second)
        {
            it->second->stop();
            delete it->second;
        }
    }
    map<uint64_t,task*>::iterator it_task = map_task.find(id);
    if(it_task != map_task.end())
    {
        if(it_task->second)
        {
            delete it_task->second;
        }
    }
    map_task.erase(id);
    return 0;
}

void task_list::task_timeout(timer* t)
{
    if(t)
    {
        uint64_t id = (uint64_t)t->get_arg();
        map<uint64_t, task*>::iterator it = map_task.find(id);
        if (it != map_task.end())
        {
            it->second->_btimeout = true;
            if (it->second->type < QUERY && it->second->cb)
            {
                simple_query_result_cb cb = (simple_query_result_cb) it->second->cb;
                cb(-2,"task time out",0,0,it->second->data);
            }
            else if (it->second->type == QUERY && it->second->cb)
            {   
                query_result_cb cb = (query_result_cb) it->second->cb;
                cb(-2,"task time out",NULL,it->second->data);
            }
        }
    }
}