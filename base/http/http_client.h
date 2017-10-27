#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <sys/poll.h>
#include <curl/curl.h>
#include <ev.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <string>

using namespace std;

#define SET_OPT(r,h,o,p) \
do{\
    r = curl_easy_setopt(h,o,p);\
}while(0)

#define MUTIL_SET_OPT(r,h,o,p) \
do{\
    r = curl_multi_setopt(h,o,p);\
}while(0)

typedef size_t (*result_cb)(char *ptr, size_t size,size_t nmemb, void *data);
typedef int (*progress_cb) (void *p, double dltotal, double dlnow, double ult, double uln);

namespace nb_http_client
{
	class http_respone;
	class curl_easy;
	typedef void (*respone_cb)(class http_respone *respone);

	class curl_multi 
	{
		public:
			curl_multi();
			~curl_multi();
			int init(struct ev_loop *loop);
			template<typename T> CURLMcode set_opt(CURLMoption  option,T parameter)
			{
				CURLMcode ret;
				MUTIL_SET_OPT(ret,_multi,option,parameter);
				return  ret;
			}
			template<typename T> CURLMcode set_opt(CURLMoption  option,T *parameter)
			{
				CURLMcode ret;
				MUTIL_SET_OPT(ret,_multi,option,parameter);
				return ret;
			}
			int add_easy(class curl_easy *easy);
		public:
			struct ev_loop *_loop;
			struct ev_timer _timer_event;
			CURLM *_multi;
			int _running_num;
	};

	typedef struct _sock_info
	{
		curl_socket_t sockfd;
		CURL *easy;
		int action;
		long timeout;
		struct ev_io ev;
		int evset;
		curl_multi *multi;
		~_sock_info()
		{
			printf("call ~_sock_info#########################################################\n");
		}
	}sock_info;

	class curl_easy
	{
	public:
		static curl_easy *new_curl_easy(curl_multi *m){return new curl_easy(m);};
		~curl_easy();
		int init();

		//设置属性
		template<typename T> CURLcode set_opt(CURLoption option,T parameter)
		{
			CURLcode ret;
			SET_OPT(ret,_easy,option,parameter);
			return  ret;
		}
	      
		template<typename T> CURLcode set_opt(CURLoption option,T *parameter)
		{
			CURLcode ret;
			SET_OPT(ret,_easy,option,parameter);
			return ret;
		}

		// header选项
		int set_header(char *key,char *value);
		int clear_header();

		//cookie 相关
		int set_cookie(char *cookie);
		int set_cookie_list(char *cookielist);
		int set_cookie_file(char *path);

		int perform();
	private:
		curl_easy(curl_multi *m);
	public:
		curl_multi *_m;
		CURL *_easy;
		char *_url;
		char _error[CURL_ERROR_SIZE];
		struct curl_slist *_header;
		respone_cb cb;
		void *data;
		string res_head;
		string res_body;
	};

	//基本http method 封装 
	class http_client
	{
	public:
		http_client(curl_multi *m);
		~http_client();

		static class curl_easy *pre_resquest(curl_multi *m){return curl_easy::new_curl_easy(m);}
		
		/*http method req_info为NULL则自动生成一个 采用默认选项 或者像这样自定义参数：
		class curl_easy *req_info = http_client::pre_resquest(mcurl);
		req_info->init();
		req_info->set_header("Accept","zhanyi");
		req_info->set_header("Agent","zhanyi-009");

		req_info->set_opt(CURLOPT_LOW_SPEED_TIME, 3L);
		req_info->set_opt(CURLOPT_LOW_SPEED_LIMIT, 10L);

		client.get(req_info,url, get_respone, NULL);
		*/
		
		int get(class curl_easy *req_info,char *url,respone_cb cb,void *data);
		int post(class curl_easy *req_info,char *url,char *post_data,respone_cb cb,void *data);
		
		//delete();
		//put();
		//head();
		
	private:
		curl_multi *_m;
		
	};

	
	// to do 
	class http_respone
	{
	public:
		http_respone(CURLcode r,string &h,string &b,char *e);
		~http_respone();
		//get_code();
		//get_head();
		//get_body();
	public:
		CURLcode ret;
		string head;
		string body;
		char error[CURL_ERROR_SIZE];
	};

	//下载文件封装 支持断点续传 to do 
	/*
	class http_download_file
	{
	public:
		http_download_file(curl_multi *m);
		~http_download_file();
		static class curl_easy *pre_resquest(curl_multi *m){return curl_easy::new_curl_easy(m);}
		int set_local_file_path(char *path);
		int get_file_size();
		int download(int64_t pos);

	private:
		curl_multi *_m;
	};*/
}