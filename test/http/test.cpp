#include "http_client.h"
using namespace nb_http_client;
void get_respone(class http_respone *respone)
{
	printf("call get_respone ret %d err %s\n",respone->ret,respone->error);
	printf("header %s\n",respone->head.c_str());
	printf("body %s\n",respone->body.c_str());
	return ;
}

void post_respone(class http_respone *respone)
{
	printf("call post_respone 2222  ret %d err %s\n",respone->ret,respone->error);
	printf("header %s\n",respone->head.c_str());
	printf("body %s\n",respone->body.c_str());
}

static void timer_cb(EV_P_ struct ev_timer *w, int revents)
{
	curl_multi *mcurl = (curl_multi *)w->data;
	http_client client(mcurl);
	//client.get(NULL,"http://10.0.109.18:8080/keymanage/dbkey/check/v1?key=2222", get_respone, NULL);
	
	class curl_easy *req_info = http_client::pre_resquest(mcurl);
	req_info->init();
	req_info->set_header("Accept","zhanyi");
	req_info->set_header("Agent","zhanyi-009");

	req_info->set_opt(CURLOPT_LOW_SPEED_TIME, 3L);
	req_info->set_opt(CURLOPT_LOW_SPEED_LIMIT, 10L);
	client.get(req_info,"http://10.0.109.18:8080/keymanage/dbkey/check1/v1?key=2222", get_respone, NULL);
}

int main()
{
	struct ev_loop *g_loop = EV_DEFAULT;
	curl_multi *mcurl = new curl_multi();
	mcurl->init(g_loop);
	struct ev_timer timer_event;
	ev_timer_init(&timer_event, timer_cb, 0.005, 10);
	timer_event.data = mcurl;
	ev_timer_start(g_loop, &timer_event);
	ev_run(g_loop, 0);
	return 0;
}
