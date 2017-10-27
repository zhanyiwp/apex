
#include "async_mconn.h"
#include <stdio.h>
#include <stdlib.h>

using namespace asyncmysql;

void libev_cb(struct ev_loop *loop, ev_io *watcher, int event)
{
    mconn *conn = mconn::get_instance_via_watcher(watcher);
    conn->state_handle(loop, watcher, event);
}

mconn::mconn()
:_curtask(NULL)
,_task_list(NULL)
,_check_state(NULL)
{
    
}

mconn::~mconn()
{
    if(_task_list)
    {
        delete _task_list;
        _task_list = NULL;
    }
    if(_check_state)
    {
        delete _check_state;
        _check_state = NULL;
    }
}

int mconn::init(string &ip,int port,string &user,string &passwd,string dbname,string &character,struct ev_loop *loop)
{
    _ip = ip;
    _port = port;
    _user = user;
    _passwd = passwd;
    _dbname = dbname;
    _character = character;
    _loop = loop;
    _state = NO_CONNECTED;
    _task_list = new task_list(_loop);
    _check_state = new timer(_loop);
    check_state(_check_state);
    _check_state->set(this,&mconn::check_state,(uint32_t)1000,true,NULL);
	_check_state->start();
    return 0;
}

void mconn::check_state(timer*)
{
    printf("check_state state %d\n",_state);
    if(NO_CONNECTED == _state)
    {
        connect_start();
    }
}

int mconn::event_to_mysql_status(int event)
{
    int status= 0;
    if (event & EV_READ)
        status|= MYSQL_WAIT_READ;
    if (event & EV_WRITE)
        status|= MYSQL_WAIT_WRITE;
    return status;
}

int mconn::mysql_status_to_event(int status)
{
    int waitevent = 0;
    if (status & MYSQL_WAIT_READ)
    {
        waitevent |= EV_READ;
    }
    if (status & MYSQL_WAIT_WRITE)
    {
        waitevent |= EV_WRITE;
    }

    return waitevent;
}


int mconn::state_handle(struct ev_loop *loop, ev_io *watcher, int event)
{
    //printf("%s, state:%d\n", __func__, _state);
    if (_state == CONNECT_WAITING)
    {
        connect_wait(loop, watcher, event);
    }
    else if (_state == SET_CHARACTER_WAITING)
    {
        set_character_wait(loop, watcher, event);
    }
    else if (_state == SIMPLE_QUERY_WAITING)
    {
        simple_query_wait(loop, watcher, event);
    }
    else if (_state == QUERY_WAITING)
    {
        query_wait(loop, watcher, event);
    }
    else if(_state == PING_WATING)
    {
        ping_wait(loop, watcher, event);
    }
    else if (_state == STORE_WAITING)
    {
        store_result_wait(loop, watcher, event);
    }
    else if (_state == CLOSE_WAITING)
    {
        close_wait(loop, watcher, event);
    }
    else if (_state == CONNECT_WRITE_ABLE)
    {
        next_task();
    }
    return 0;
}

int mconn::ready_for_next_task()
{
    _state = CONNECT_WRITE_ABLE;
    int socket = mysql_get_socket(_mysql);
    if(socket == -1)
    {
        return -1;
    }
    ev_io_init (&_watcher, libev_cb, socket, EV_WRITE);
    ev_io_start(_loop, &_watcher);
    return 0;
}

int mconn::next_state(conn_state state,int status,bool stop_io /*= true*/)
{
    _state = state;
    printf("next_state status %d\n",_state);
    if(stop_io)
        ev_io_stop(_loop, &_watcher);
    int socket = mysql_get_socket(_mysql);
    int waitevent = mysql_status_to_event(status);
    ev_io_init (&_watcher, libev_cb, socket, waitevent);
    ev_io_start(_loop, &_watcher);
    return 0;
}

void mconn::next_task()
{
    if(_curtask != NULL)
    {
        _task_list->del_task(_curtask->id);
    }
    _curtask = _task_list->fetch_task();
    exec_task();
    return ;
}

void mconn::exec_task()
{
    if (_curtask != NULL )
    {
        if(_curtask->_btimeout) // 已经超时
        {
            printf("sql timeout %s\n",_curtask->sql.c_str());
            _task_list->del_task(_curtask->id);
            _curtask = NULL;
            ev_io_stop(_loop, &_watcher);
            ready_for_next_task();
        }
        else
        {
            printf("exec sql %s\n",_curtask->sql.c_str());
            if (_curtask->type < QUERY)
            {
                simple_query_start((char*)_curtask->sql.c_str(), _curtask->sql.size());
            }
            else if (_curtask->type == QUERY)
            {   
                query_start((char*)_curtask->sql.c_str(), _curtask->sql.size());
            }
            else if (_curtask->type == CLOSE)
            {
                close_start();
            }
        }
    }
}

int mconn::connect_start()
{
    _mysql = mysql_init(_mysql);
    mysql_options(_mysql, MYSQL_OPT_NONBLOCK, 0);
    char value = 1;
    mysql_options(_mysql, MYSQL_OPT_RECONNECT, (char *)&value);
    MYSQL *ret;
    printf("connect_start state %d\n",_state);
    int status = mysql_real_connect_start(&ret, _mysql, _ip.c_str(), _user.c_str(), _passwd.c_str(), _dbname.c_str(), _port, NULL, 0);
    if (status)
    {
        next_state(CONNECT_WAITING,status,false);
    }
    else
    {
        if(!ret)
        {
            // 连接失败
            _state = NO_CONNECTED;
            printf("Failed to mysql_real_connect()1111 %d %s\n",mysql_errno(_mysql),mysql_error(_mysql));
            _check_state->set(this,&mconn::check_state,(uint32_t)1000,true,NULL);
	        _check_state->start();
        }
        else
        {
            printf("connect_start connect success\n");
            if(!_character.empty())
            {
                set_character_start();
            }
            else
            {
                ready_for_next_task();
            }
        }
    }

    return 0;
}

int mconn::connect_wait(struct ev_loop *loop, ev_io *watcher, int event)
{
    MYSQL *ret;
    int status = event_to_mysql_status(event);
    status= mysql_real_connect_cont(&ret, _mysql, status);
    if (status != 0)
    {
        next_state(CONNECT_WAITING,status);
    }
    else
    {
        if(!ret)
        {
            // 连接失败
            _state = NO_CONNECTED;
            printf("Failed to mysql_real_connect() %d %s\n",mysql_errno(_mysql),mysql_error(_mysql));
            ev_io_stop(_loop, &_watcher);
            _check_state->set(this,&mconn::check_state,(uint32_t)1000,true,NULL);
	        _check_state->start();
        }
        else
        {
            printf("connect_wait connect success\n");
            _check_state->stop();
            ev_io_stop(_loop, &_watcher);
            if(!_character.empty())
            {
                set_character_start();
            }
            else
            {
                ready_for_next_task();
            }
        }
    }
    return 0;
}

int mconn::set_character_start()
{
    int ret, status;
    status = mysql_set_character_set_start(&ret, _mysql, _character.c_str());
    if (status)
    {
        next_state(SET_CHARACTER_WAITING,status);
    }
    else
    {
        if(ret == 0)
        {
            printf("set character success: new character %s \n", mysql_character_set_name(_mysql));
        }
        else
        {
            printf("set character(%s) failed: new character %s \n", _character.c_str(),mysql_character_set_name(_mysql));
        }
        ev_io_stop(_loop, &_watcher);
        ready_for_next_task();
    }

    return 0;
}

int mconn::set_character_wait(struct ev_loop *loop, ev_io *watcher, int event)
{
    int ret = 0;
    int status = event_to_mysql_status(event);
    status= mysql_set_character_set_cont(&ret, _mysql, status);
    if (status != 0)
    {
        next_state(SET_CHARACTER_WAITING,status);
    }
    else
    {
        if(ret == 0)
        {
            printf("set character success: new character %s \n", mysql_character_set_name(_mysql));
        }
        else
        {
            printf("set character(%s) failed: new character %s \n", _character.c_str(),mysql_character_set_name(_mysql));
        }
        ev_io_stop(_loop, &_watcher);
        ready_for_next_task();
    }
    return 0;
}

int mconn::simple_query_start(char *sql, int sqllen)
{
    int ret, status;
    status = mysql_real_query_start(&ret, _mysql, sql, sqllen);
    if (status)
    {
        next_state(SIMPLE_QUERY_WAITING,status);
    }
    else
    {
        simple_query_result_cb cb = (simple_query_result_cb) _curtask->cb;
        if(ret != 0)
        {
            printf("exec simple_query Error: %d (%s) sql %s\n", mysql_errno(_mysql),mysql_error(_mysql),_curtask->sql.c_str());
            if(2013 == mysql_errno(_mysql)) // 连接丢失
            {
                printf("call ping_start\n");
                ping_start();
            }
            else if(cb != NULL && !_curtask->_btimeout)
            {
                cb(ret,(char*)mysql_error(_mysql),0,0,_curtask->data);
            }
        }
        else
        {
            if(cb != NULL && !_curtask->_btimeout)
            {
                uint64_t insert_id = 0;
                if(INSERT == _curtask->type)
                {
                    insert_id = mysql_insert_id(_mysql);
                }
                int64_t effect_rows = mysql_affected_rows(_mysql);
                cb(0,"",effect_rows,insert_id,_curtask->data);
            }
        }
        ev_io_stop(_loop, &_watcher);
        ready_for_next_task();
    }
    return 0;
}


int mconn::simple_query_wait(struct ev_loop *loop, ev_io *watcher, int event)
{
    int ret = 0;
    int status = event_to_mysql_status(event);

    status= mysql_real_query_cont(&ret, _mysql, status);
    if (status != 0)
    {
        next_state(SIMPLE_QUERY_WAITING,status);
    }
    else
    {
        printf("simple_query_wait finish\n");
        simple_query_result_cb cb = (simple_query_result_cb) _curtask->cb;
        if(ret != 0)
        {
            printf("exec simple_query Error: %d (%s) sql %s\n", mysql_errno(_mysql),mysql_error(_mysql),_curtask->sql.c_str());
            if(2013 == mysql_errno(_mysql)) // 连接丢失
            {
                printf("call ping_start\n");
                ping_start();
            }
            else if(cb != NULL && !_curtask->_btimeout)
            {
                cb(ret,(char*)mysql_error(_mysql),0,0,_curtask->data);
            }
        }
        else
        {
            if(cb != NULL && !_curtask->_btimeout)
            {
                uint64_t insert_id = 0;
                if(INSERT == _curtask->type)
                {
                     insert_id = mysql_insert_id(_mysql);
                }
                int64_t effect_rows = mysql_affected_rows(_mysql);
                cb(0,"",effect_rows,insert_id,_curtask->data);
            }
        }
        ev_io_stop(_loop, &_watcher);
        ready_for_next_task();
    }

    return 0;
}

int mconn::query_start(char *sql, int sqllen)
{
    int ret, status;
    status = mysql_real_query_start(&ret, _mysql, sql, sqllen);
    if (status)
    {
        next_state(QUERY_WAITING,status);
    }
    else
    {
        query_result_cb cb = (query_result_cb) _curtask->cb;
        if(ret != 0)
        {
            printf("exec simple_query Error: %d (%s) sql %s\n", mysql_errno(_mysql),mysql_error(_mysql),_curtask->sql.c_str());
            if(2013 == mysql_errno(_mysql)) // 连接丢失
            {
                printf("call ping_start\n");
                ping_start();
            }
            else if(cb != NULL && !_curtask->_btimeout)
            {
                cb(ret,(char*)mysql_error(_mysql),NULL,_curtask->data);
            }
        }
        else
        {
            store_result_start();
        }
    }

    return 0;
}

int mconn::query_wait(struct ev_loop *loop, ev_io *watcher, int event)
{
    int ret = 0;
    int status = event_to_mysql_status(event);

    status= mysql_real_query_cont(&ret, _mysql, status);
    if (status != 0)
    {
        next_state(QUERY_WAITING,status);
    }
    else
    {
        query_result_cb cb = (query_result_cb) _curtask->cb;
        if(ret != 0)
        {
            printf("exec simple_query Error: %d (%s) sql %s\n", mysql_errno(_mysql),mysql_error(_mysql),_curtask->sql.c_str());
            if(2013 == mysql_errno(_mysql)) // 连接丢失
            {
                printf("call ping_start\n");
                ping_start();
            }
            else if(cb != NULL && !_curtask->_btimeout)
            {
                cb(ret,(char*)mysql_error(_mysql),NULL,_curtask->data);
            }
        }
        else
        {
            store_result_start();
        }
    }

    return 0;
}

int mconn::ping_start()
{
    int ret, status;
    status = mysql_ping_start(&ret, _mysql);
    if (status)
    {
        next_state(PING_WATING,status);
    }
    else
    {
        if(ret == 0)// 连接正常
        {
            printf("connect normal\n");
            exec_task();
        }
        else
        {
            printf("connect unnormal\n");
            ping_start();
        }
        
    }

    return 0;
}

int mconn::ping_wait(struct ev_loop *loop, ev_io *watcher, int event)
{
    int status,ret;
    status = event_to_mysql_status(event);
    printf("ping_wait state %d\n",_state);
    status= mysql_ping_cont(&ret, _mysql, status);
    if (status != 0)
    {
        next_state(PING_WATING,status);
    }
    else
    {
        if(ret == 0)
        {
            printf("connect normal\n");
            exec_task();
        }
        else
        {
            printf("connect unnormal\n");
            ping_start();
        }
    }
    return 0;
}

int mconn::store_result_start()
{
    int status;
    status = mysql_store_result_start(&_queryres, _mysql);
    if (status)
    {
        next_state(STORE_WAITING,status);
    }
    else
    {
        query_result_cb cb = (query_result_cb) _curtask->cb;
        if(cb != NULL && !_curtask->_btimeout)
        {
            result_set *_result_set = new result_set(_queryres);
            cb(0,"",_result_set,_curtask->data);
            delete _result_set;
            _result_set = NULL;
        }
        ev_io_stop(_loop, &_watcher);
        ready_for_next_task();
    }

    return 0;
}

int mconn::store_result_wait(struct ev_loop *loop, ev_io *watcher, int event)
{
    int status = event_to_mysql_status(event);

    status= mysql_store_result_cont(&_queryres, _mysql, status);
    if (status != 0)
    {
        next_state(STORE_WAITING,status);
    }
    else
    {   
        query_result_cb cb = (query_result_cb) _curtask->cb;
        if(cb != NULL && !_curtask->_btimeout)
        {
            result_set *_result_set = new result_set(_queryres);
            cb(0,"",_result_set,_curtask->data);
            delete _result_set;
            _result_set = NULL;
        }
        ev_io_stop(_loop, &_watcher);
        ready_for_next_task();
    }

    return 0;
}

int mconn::close_start()
{
    int status;
    status = mysql_close_start(_mysql);
    if (status)
    {
        next_state(CLOSE_WAITING,status);
    }
    else
    {
        printf("close done, no need wait\n");
    }

    return 0;
}

int mconn::close_wait(struct ev_loop *loop, ev_io *watcher, int event)
{
    int status = event_to_mysql_status(event);
    status= mysql_close_cont(_mysql, status);
    if (status != 0)
    {
        next_state(CLOSE_WAITING,status);
    }
    else
    {
        printf("close done\n");
        ev_io_stop(loop, watcher);
        _state = CLOSE_FINISH;
    }
    return 0;
}


