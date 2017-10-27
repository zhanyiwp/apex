#include "http_client.h"

using namespace nb_http_client;

static void timer_cb(EV_P_ struct ev_timer *w, int revents);

static int multi_timer_cb(CURLM *c, long timeout_ms, curl_multi *multi)
{
	ev_timer_stop(multi->_loop, &multi->_timer_event);
	if (timeout_ms > 0)
	{
		double t = timeout_ms / 1000;
		ev_timer_init(&multi->_timer_event, timer_cb, t, 0.);
		ev_timer_start(multi->_loop, &multi->_timer_event);
	}
	else if (timeout_ms == 0)
		timer_cb(multi->_loop, &multi->_timer_event, 0);
	return 0;
}

/* Die if we get a bad CURLMcode somewhere */
static void mcode_or_die(const char *where, CURLMcode code)
{
	if (CURLM_OK != code)
	{
		const char *s;
		switch (code)
		{
		case CURLM_BAD_HANDLE:
			s = "CURLM_BAD_HANDLE";
			break;
		case CURLM_BAD_EASY_HANDLE:
			s = "CURLM_BAD_EASY_HANDLE";
			break;
		case CURLM_OUT_OF_MEMORY:
			s = "CURLM_OUT_OF_MEMORY";
			break;
		case CURLM_INTERNAL_ERROR:
			s = "CURLM_INTERNAL_ERROR";
			break;
		case CURLM_UNKNOWN_OPTION:
			s = "CURLM_UNKNOWN_OPTION";
			break;
		case CURLM_LAST:
			s = "CURLM_LAST";
			break;
		default:
			s = "CURLM_unknown";
			break;
		case CURLM_BAD_SOCKET:
			s = "CURLM_BAD_SOCKET";
			printf("ERROR: %s returns %s\n", where, s);
			/* ignore this error */
			return;
		}
		printf("ERROR: %s returns %s\n", where, s);
		exit(code);
	}
}

static void check_multi_info(curl_multi *multi)
{
	char *eff_url;
	CURLMsg *msg;
	int msgs_left;
	curl_easy *conn;
	CURL *easy;
	CURLcode ret;
	printf("REMAINING: %d\n", multi->_running_num);
	while ((msg = curl_multi_info_read(multi->_multi, &msgs_left)))
	{
		if (msg->msg == CURLMSG_DONE)
		{
			easy = msg->easy_handle;
			ret = msg->data.result;
			curl_easy_getinfo(easy, CURLINFO_PRIVATE, &conn);
			curl_easy_getinfo(easy, CURLINFO_EFFECTIVE_URL, &eff_url);
			printf("DONE: %s\n", eff_url);
			// to do 何时释放？ 
			// to do curl_easy_getinfo 获取各种附加信息: 状态码 cookie 就够了 更多的暂时不提供支持
			if (conn->cb)
			{
				http_respone *respone = new http_respone(ret,conn->res_head,conn->res_body,conn->_error);
				conn->cb(respone);
				delete respone;
				respone = NULL;
			}
			curl_multi_remove_handle(multi->_multi, easy);
			delete conn;
			conn = NULL;
			printf("msgs_left %d\n",msgs_left);
		}
	}
}

/* Called by libevent when we get action on a multi socket */
static void event_cb(EV_P_ struct ev_io *w, int revents)
{
	curl_multi *multi = (curl_multi*)w->data;
	CURLMcode rc;
	//int running_num = multi->get_running_num();
	int action = (revents & EV_READ ? CURL_POLL_IN : 0) | (revents & EV_WRITE ? CURL_POLL_OUT : 0);
	rc = curl_multi_socket_action(multi->_multi, w->fd, action, &multi->_running_num);
	if (CURLM_OK != rc)
	{
		printf("event_cb:curl_multi_socket_action failed %d %s\n",rc,curl_multi_strerror(rc));
		return ;
	}
	check_multi_info(multi);
	if (multi->_running_num <= 0)
	{
		printf("last transfer done, kill timeout\n");
		ev_timer_stop(multi->_loop, &multi->_timer_event);
	}
}

/* Called by libev when our timeout expires */
static void timer_cb(EV_P_ struct ev_timer *w, int revents)
{
	curl_multi *multi = (curl_multi*)w->data;
	CURLMcode rc = curl_multi_socket_action(multi->_multi, CURL_SOCKET_TIMEOUT, 0, &multi->_running_num);
	if (CURLM_OK != rc)
	{
		printf("timer_cb:curl_multi_socket_action failed %d %s\n",rc,curl_multi_strerror(rc));
		return ;
	}
	check_multi_info(multi);
}

/* Clean up the sock_info structure */
static void remsock(sock_info *f, curl_multi *multi)
{
	if (f)
	{
		if (f->evset)
			ev_io_stop(multi->_loop, &f->ev);
		delete f;
		f = NULL;
	}
}

/* Assign information to a sock_info structure */
static void setsock(sock_info *f, curl_socket_t s, CURL *e, int act, curl_multi *multi)
{

	int kind = (act & CURL_POLL_IN ? EV_READ : 0) | (act & CURL_POLL_OUT ? EV_WRITE : 0);
	f->sockfd = s;
	f->action = act;
	f->easy = e;
	if (f->evset)
		ev_io_stop(multi->_loop, &f->ev);
	ev_io_init(&f->ev, event_cb, f->sockfd, kind);
	f->ev.data = multi;
	f->evset = 1;
	ev_io_start(multi->_loop, &f->ev);
}

/* Initialize a new sock_info structure */
static void addsock(curl_socket_t s, CURL *easy, int action, curl_multi *multi)
{
	sock_info *fdp = new sock_info();
	fdp->multi = multi;
	setsock(fdp, s, easy, action, multi);
	curl_multi_assign(multi->_multi, s, fdp);
}

/* CURLMOPT_SOCKETFUNCTION */
static int sock_cb(CURL *e, curl_socket_t s, int what, void *cbp, void *sockp)
{
	printf("%s e %p s %i what %i cbp %p sockp %p\n", __PRETTY_FUNCTION__, e, s, what, cbp, sockp);
	curl_multi *multi = (curl_multi *)cbp;
	sock_info *fdp = (sock_info *)sockp;
	const char *whatstr[] = {"none", "IN", "OUT", "INOUT", "REMOVE"};
	printf("socket callback: s=%d e=%p what=%s\n", s, e, whatstr[what]);
	if (what == CURL_POLL_REMOVE)
	{
		remsock(fdp, multi);
	}
	else
	{
		if (!fdp)
		{
			printf("Adding sock: %s\n", whatstr[what]);
			addsock(s, e, what, multi);
		}
		else
		{
			printf("Changing action from %s to %s\n", whatstr[fdp->action], whatstr[what]);
			setsock(fdp, s, e, what, multi);
		}
	}
	return 0;
}

static size_t header_cb(char *ptr, size_t size,size_t nmemb, void *data)
{
	size_t realsize = size * nmemb;
	curl_easy *conn = (curl_easy *)data;
	//(void)ptr;
	//(void)conn;
	conn->res_head.append((char *)ptr, realsize);
	//printf("header_cb : size %d %s\n",realsize,ptr);
	return realsize;
}

static size_t write_cb(void *ptr, size_t size, size_t nmemb, void *data)
{
	size_t realsize = size * nmemb;
	curl_easy *conn = (curl_easy *)data;
	conn->res_body.append((char *)ptr, realsize);
	return realsize;
}

/* CURLOPT_PROGRESSFUNCTION */
static int prog_cb(void *p, double dltotal, double dlnow, double ult, double uln)
{
	curl_easy *conn = (curl_easy *)p;
	//printf("Progress: %s (%g/%g) (%g/%g)\n", conn->url, dlnow, dltotal, ult, uln);
	return 0;
}

curl_multi::curl_multi()
{
}

curl_multi::~curl_multi()
{
	if(_multi)
	{
		curl_multi_cleanup(_multi);
		_multi = NULL;
	}
}

int curl_multi::init(struct ev_loop *loop)
{
	_loop = loop;
	_multi = curl_multi_init();
	if(_multi == NULL)
	{
		return -1;
	}
	ev_timer_init(&_timer_event, timer_cb, 0., 0.);
	_timer_event.data = this;

	set_opt(CURLMOPT_SOCKETFUNCTION, sock_cb);
	set_opt(CURLMOPT_SOCKETDATA, this);
	set_opt(CURLMOPT_TIMERFUNCTION, multi_timer_cb);
	set_opt(CURLMOPT_TIMERDATA, this);

	return 0;
}

int curl_multi::add_easy(class curl_easy *easy)
{
	printf("Adding easy %p to multi %p (%s)\n", easy->_easy, _multi, easy->_url);
	CURLMcode rc = curl_multi_add_handle(_multi, easy->_easy);
	if (CURLM_OK != rc)
	{
		printf("event_cb:curl_multi_add_handle failed %d %s\n",rc,curl_multi_strerror(rc));
		return -1;
	}
	return 0;
}

curl_easy::curl_easy(curl_multi *m)
{
	_m = m;
	_easy = NULL;
	_url = NULL;
	memset(_error,0,sizeof(_error));
	_header = NULL;
}

curl_easy::~curl_easy()
{
	_m = NULL;
	clear_header();
	if(_easy)
	{
		curl_easy_cleanup(_easy);
		printf("call ~_conn_info+++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
		_easy = NULL;
	}
}

int curl_easy::init()
{
	_easy = curl_easy_init();
	return _easy == NULL ? -1 : 0;
}

int curl_easy::set_header(char *key,char *value)
{
	if(key == NULL || value == NULL || strlen(key) < 0 || strlen(value) < 0)
	{
		return -1;
	}
	string header(key);  
	header.append(": ");  
	header.append(value);  
	_header = curl_slist_append(_header, header.c_str());  
	return curl_easy_setopt(_easy, CURLOPT_HTTPHEADER, _header);  
}

int curl_easy::clear_header()
{
	if (_header )  
	{  
	    curl_slist_free_all(_header);  
	    _header = NULL;  
	}  
	return 0;
}

int curl_easy::set_cookie(char *cookie)
{
	if(cookie == NULL)
	{
		return -1;
	}
	return curl_easy_setopt(_easy, CURLOPT_COOKIE, cookie); 
}

int curl_easy::set_cookie_list(char *cookielist)
{
	return 0;
}

int curl_easy::set_cookie_file(char *path)
{
	if(path == NULL)
	{
		return -1;
	}
	return curl_easy_setopt(_easy, CURLOPT_COOKIEFILE, path); 
}

int curl_easy::perform()
{
	if(_m == NULL)
	{
		return -1;
	}
	return _m->add_easy(this);
}


http_client::http_client(curl_multi *m)
:_m(m)
{
	
}

http_client::~http_client()
{
	_m = NULL;	
}

int http_client::get(class curl_easy *req_info,char *url,respone_cb cb,void *data)
{
	if (req_info == NULL)
	{
		req_info = pre_resquest(_m);
		if (req_info->init() < 0)
		{
			delete req_info;
			req_info = NULL;
			return -1;
		}
	}
	req_info->cb = cb;
	req_info->data = data;
	req_info->_url = strdup(url);
	req_info->set_opt(CURLOPT_URL, url); 
	//req_info->set_opt(CURLOPT_HEADER, 1L);
	req_info->set_opt(CURLOPT_HEADERFUNCTION, header_cb);
	req_info->set_opt(CURLOPT_HEADERDATA,req_info);
	req_info->set_opt(CURLOPT_WRITEFUNCTION, write_cb);
	req_info->set_opt(CURLOPT_WRITEDATA, req_info);

	req_info->set_opt(CURLOPT_NOPROGRESS, 0L);
	req_info->set_opt(CURLOPT_PROGRESSFUNCTION, prog_cb);
	req_info->set_opt(CURLOPT_PROGRESSDATA, req_info);

	req_info->set_opt(CURLOPT_PRIVATE, req_info);
	req_info->set_opt(CURLOPT_ERRORBUFFER, req_info->_error);
	req_info->set_opt(CURLOPT_VERBOSE, 1L);

	//req_info->set_opt(CURLOPT_LOW_SPEED_TIME, 3L);
	//req_info->set_opt(CURLOPT_LOW_SPEED_LIMIT, 10L);

	return req_info->perform();
}

int http_client::post(class curl_easy *req_info,char *url,char *post_data,respone_cb cb,void *data)
{
	if (req_info == NULL)
	{
		req_info = pre_resquest(_m);
		if (req_info->init() < 0)
		{
			delete req_info;
			req_info = NULL;
			return -1;
		}
	}
	req_info->cb = cb;
	req_info->data = data;
	req_info->_url = strdup(url);

	req_info->set_opt(CURLOPT_URL,url);
	req_info->set_opt(CURLOPT_POSTFIELDS,post_data);
	
	//req_info->set_opt( CURLOPT_HEADER, 1L);
	req_info->set_opt(CURLOPT_HEADERFUNCTION, header_cb);
	req_info->set_opt(CURLOPT_HEADERDATA,req_info);

	req_info->set_opt(CURLOPT_WRITEFUNCTION, write_cb);
	req_info->set_opt(CURLOPT_WRITEDATA, req_info);

	// help debug
	req_info->set_opt(CURLOPT_VERBOSE, 1L);
	req_info->set_opt(CURLOPT_ERRORBUFFER, req_info->_error);
	req_info->set_opt(CURLOPT_PRIVATE, req_info);

	//CURLOPT_VERBOSE
	req_info->set_opt(CURLOPT_NOPROGRESS, 0L);
	req_info->set_opt(CURLOPT_PROGRESSFUNCTION, prog_cb);
	req_info->set_opt(CURLOPT_PROGRESSDATA, req_info);

	//req_info->set_opt(CURLOPT_LOW_SPEED_TIME, 3L);
	//req_info->set_opt(CURLOPT_LOW_SPEED_LIMIT, 10L);
	return req_info->perform();
}

http_respone::http_respone(CURLcode r, string &h, string &b, char *e)
{
	ret = r;
	head = h;
	body = b;
	strncpy(error,e,sizeof(error));
}


http_respone::~http_respone()
{
	printf("call ~http_respone----------------------------------------\n");
}

