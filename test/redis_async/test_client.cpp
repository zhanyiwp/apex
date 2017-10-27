/*
 * =====================================================================================
 *
 *       Filename:  test_client.cpp
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
#include <string.h>
#include "socket_func.h"
#include "util.h"
#include "net.h"

using namespace Net;

class client_raw : public tcp_client_handler_raw
{
    public:
        client_raw(const Config::section& data) : tcp_client_handler_raw(data) { }
        ~client_raw() { }

        int init(string key)
        {
            m_key = key;
            return 0;
        }

        virtual void on_connect(tcp_client_channel* channel)
        {
            printf("%s\n", __func__);
            channel->try_send((uint8_t*)m_key.c_str(),m_key.size());
        }
        virtual void on_closing(tcp_client_channel* channel)
        {
            printf("%s\n", __func__);
        }
        virtual void on_error(ERROR_CODE err, tcp_client_channel* channel)
        {
            printf("%s %d \n", __func__,err);
        }


        virtual void on_timer(uint8_t timer_id, tcp_client_channel* channel)
        {
            printf("%s\n", __func__);
        }

        //return 0 包不完整 >0 完整的包 
        virtual int32_t on_recv(const uint8_t *pkg, const uint32_t len, tcp_client_channel* channel)
        {
            if(len > 0)
            {
                
            }
            return len;
        }
        virtual void on_pkg(const uint8_t *pkg, const uint32_t len, tcp_client_channel* channel)
        {
            printf("%s %s\n", __func__,pkg);
            exit(0);
        }
        private:
            string m_key;

};


int main(int argc, char** argv)
{
    if(argc < 2)
    {
        printf("use age %s user key\n",argv[0]);
        return -1;
    }
    Config::section conf("conf.ini", "olinestatus_clt");
    client_raw *client_raw1 = new client_raw(conf);
    client_raw1->init(argv[1]);
    struct ev_loop *loop = EV_DEFAULT;
    add_tcp_client(loop, client_raw1);

    run(loop);

    printf("hello,world\n");

    return 0;
}
