/*
 * =====================================================================================
 *
 *       Filename:  test_server.cpp
 *
 *    Description:  该示例演示一个限制登录的功能 一个账号1分钟内只能登录一次 并将登录结果返回给客户端                                                                                                                                                                   
 *
 *        Version:  1.0
 *        Created:  06/10/2017 02:45:56 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ZHANYI (ZY), zhanyiwp@163.com
 *   Organization:  ec
 *
 * =====================================================================================
 */

#include <stdio.h>
#include "socket_func.h"
#include "util.h"
#include "net.h"
#include "nbredis.h"
#include "ev.h"

using namespace Net;
using namespace asyncredis;

struct userdata
{
    string key;
    tcp_server_channel* channel;
};

class redis_reply_handler:public reply
{
    public:
        int init(string &ip,int port,string password,struct ev_loop *loop)
        {
           return redisclient.init(ip, port, password, this, loop);
        }
        
        virtual void status_reply(reply_status result, const bool r, void *privdata)
        {
            userdata *p = (userdata*)privdata;
            if(!p)
            {
                return ;
            }
            string repsone = p->key;
            if(REPLY_TYPE_OK == result)
            {
                repsone += " login success!";
            }
            else
            {
                string errinfo;
                int err = get_error_info(errinfo);
                printf("%s get err %d(%s) key %s\n",__func__,err,errinfo.c_str(),p->key.c_str());
                repsone += " login success,but there is there is something wrong";
            }
            p->channel->try_send((uint8_t *)repsone.c_str(),repsone.size());
            if(p)
            {
                delete p;
            }
        }

        virtual void string_reply(reply_status result,const string &r, void *privdata)
        {
            userdata *p = (userdata*)privdata;
            if(!p)
            {
                return ;
            }
            if(REPLY_TYPE_OK == result)
            {
                printf("%s get success %s %s\n",__func__,r.c_str(),p->key.c_str());
                string repsone = p->key + " can not login ";
                p->channel->try_send((uint8_t *)repsone.c_str(),repsone.size());
                if(p)
                {
                    delete p;
                }
            }
            else
            {
                string errinfo;
                int err = get_error_info(errinfo);
                if(err == REPLY_TYPE_NIL)
                {
                    string key = "limit_login_test:" + p->key;
                    redisclient.set(key,"test_value",em_sec,60,(void*)p);
                }
                else
                {
                    string repsone = p->key + " can not login and there is something wrong";
                    p->channel->try_send((uint8_t *)repsone.c_str(),repsone.size());
                    printf("%s get err %d(%s) key:%s\n",__func__,err,errinfo.c_str(),p->key.c_str());
                    if(p)
                    {
                        delete p;
                    }
                }
            }
        }
    public:
        nb_redis redisclient;
};

class server_raw : public tcp_server_handler_raw
{
    public:
        server_raw(const Config::section& data) : tcp_server_handler_raw(data) { }
        ~server_raw() { }
        
        int init(string &ip,int port,string password,struct ev_loop *loop)
        {
           r = new redis_reply_handler();
           return r->init(ip, port, password, loop);
        }

        virtual void on_accepted(tcp_server_channel* channel)
        {
            printf("%s\n", __func__);
        }
        virtual void on_closing(tcp_server_channel* channel)
        {
            printf("%s\n", __func__);
        }
        virtual void on_error(ERROR_CODE err, tcp_server_channel* channel)
        {
            printf("%s\n", __func__);
        }

        virtual void on_timer(uint8_t timer_id, tcp_server_channel* channel)
        {
            printf("%s\n", __func__);
        }

        //return 0 包不完整 >0 完整的包 
        virtual int32_t on_recv(const uint8_t *pkg, const uint32_t len, tcp_server_channel* channel) 
        {
            if(len > 0)
            {
                //printf("%s get data %s\n", __func__,pkg);
            }
            return len;
        }
        virtual void on_pkg(const uint8_t *pkg, const uint32_t len, tcp_server_channel* channel)
        {
            printf("%s get data %s\n", __func__,pkg);
            userdata *data= new userdata();
            data->key.assign((char*)pkg,len);
            data->channel = channel;
            string key = "limit_login_test:" + data->key;
            r->redisclient.get(key,(void*)data);
        }
    private:
        redis_reply_handler* r;
};

struct ev_loop *g_loop = EV_DEFAULT;

int main()
{
    string ip = "192.168.1.222";
    int port = 20000;
    string passwd;
    Config::section conf("conf.ini", "profle_svr");
    server_raw *server_raw1 = new server_raw(conf);
    server_raw1->init(ip,port,passwd,g_loop);
    bool bOk = add_tcp_listen(g_loop, server_raw1);
    if(!bOk)
    {
        printf("start fail\n");
        return -1;
    }

    run(g_loop);

    printf("hello,world\n");

    return 0;
}
