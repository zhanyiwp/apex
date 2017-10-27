#include "nbmysql.h"
#include "net.h"
#include <stdio.h>
#include <string>

using namespace std;
using namespace asyncmysql;

nb_mysql mysqltest(4096);
ev_timer timeout_watcher;
struct ev_loop *loop = EV_DEFAULT;
int gi=0;

void on_simple_query(int ret,char *err,int64_t effect_rows, uint64_t insert_id,void *data)
{
    printf("%s\n", __func__);
    if(ret != 0)
    {
        printf("exec sql failed ret %d err %s\n",ret , err);
    }
    else
    {
        printf("exec sql success. effect rows: %lu insert_id:%lu\n",effect_rows,insert_id);
    }
}

void on_query (int ret, char *err,result_set *resultset, void *data)
{
    printf("%s ret %d err %s\n", __func__,ret,err);
    if(ret != 0)
    {
        return;
    }
    u64 count = resultset->row_count();
    printf("total count %llu\n",count);
    printf("%8s%8s%8s%10s\n", "id", "s1","s2","s3");
    u32 id = 0;
    u32 s1 = 0;
    while (resultset->next())
    {
        id = resultset->get_unsigned32("id");
        s1 = resultset->get_unsigned32("s1");
        const char *s2 = resultset->get_string("s2");
        const char *s3 = resultset->get_string("s3");
        printf("%8u%8u%8s%10s\n", id,s1,s2,s3);
    }
    if(count == 100)
    {
        mysqltest.query((void *)"query",on_query,3000,"select * from test.test where id > %u limit 100",id);
    }
    else
    {
        printf("query finish\n");
        mysqltest.close();
    }
}

void timeout_cb (EV_P_ ev_timer *w, int revents)
{
     //printf("come here timeout \n");
     //mysqltest.simple_query((void *)"simple_query",on_simple_query,false,3000,"insert into test.test(s1) values(%d)",gi++);
     //mysqltest.query((void *)"query",on_query,3000,"select * from test.test where id < 100");
     //ev_timer_start (loop, &timeout_watcher);
}

int main()
{
    string ip="10.0.109.18";
    int port = 3306;
    string user = "zhanyi";
    string passwd = "123456";
    string dbname = "test";
    string character = "utf8mb4";

    //struct ev_loop *loop = EV_DEFAULT;
   
    //ev_timer_init (&timeout_watcher, timeout_cb, 5, 5);
    //ev_timer_start (loop, &timeout_watcher);
   //ev_timer_again (&timeout_watcher);
    //nb_mysql mysqltest(4096);
    mysqltest.init(ip,port,user,passwd,dbname,character,loop);
    mysqltest.simple_query((void *)"simple_query",on_simple_query,false,3000,"insert into test.test(s1,s2,s3) values(%d,'%s','%s')",5,"test","12:31:21");
    mysqltest.query((void *)"query",on_query,3000,"select * from test.test where id > 0 limit 100");
    //mysqltest.close();
   Net::run(loop);

    return 0;
}
