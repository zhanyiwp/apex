//g++ -o server_raw server_raw.cpp -I../../base/util -I../../inc -L ../../lib -lapexbase -lev

#include <stdio.h>
//#include "util_net.h"
#include "socket_func.h"
#include "util.h"
#include "net.h"

using namespace Net;

class server_raw : public tcp_server_handler_raw
{
    public:
        server_raw(const Config::section& data) : tcp_server_handler_raw(data) { }
        ~server_raw() { }

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
                //on_pkg(pkg,len,channel);
            }
            return len;
        }
        virtual void on_pkg(const uint8_t *pkg, const uint32_t len, tcp_server_channel* channel)
        {
            printf("%s get data %s\n", __func__,pkg);
        }
};


int main(int argc, char** argv)
{
    //conf_section conf("conf.ini", "profle_svr");
    Config::section conf("conf.ini", "profle_svr");
    server_raw *server_raw1 = new server_raw(conf);
    struct ev_loop *loop = EV_DEFAULT;
    bool bOk = add_tcp_listen(loop, server_raw1);
    if(!bOk)
    {
        printf("start fail\n");
        return -1;
    }
    run(loop);

    printf("hello,world\n");

    return 0;
}
