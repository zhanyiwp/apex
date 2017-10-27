#ifndef _ZK_CLT_H_
#define _ZK_CLT_H_

#include "stdint.h"
#include <zookeeper.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <map>
#include <sstream>

using namespace std;

const uint32_t	WATCH_BIT_CLIENT = 0x01;
const uint32_t	WATCH_BIT_SERVER = 0x02;

typedef bool node_value_changed_t(const string &path, const string &value, uint32_t flags);
typedef bool children_list_changed_t(const string &path, const vector<string> &value, uint32_t flags);

//
class ZkRet
{
	friend class zk_clt;
public:
	bool ok() const {return ZOK == code_; }
	bool nodeExist() const {return ZNODEEXISTS == code_; }
	bool nodeNotExist() const {return ZNONODE == code_; }
	int Code(){ return code_; }
	operator bool(){return ok(); }

	ZkRet(){code_ = ZOK; }
	ZkRet(int c){code_ = c; }
private:
	int code_;
};

// thread safety: single zk_cltZooKeeper object should be used in single thread.
class zk_clt
{
public:
	static zk_clt*	get_instance(bool create_if_not_exist = false);	//非线程安全创建

	//
	bool init(const string &connectString);
	void restart();
	void uninit();

	bool get_node_value(const string &path, string &value);
	bool set_node_value(const string &path, const string &value);
	bool get_children_list(const string &path, vector<string> &children);
	bool exists(const string &path);
	// ephemeral node is a special node, its has the same lifetime as the session 
	ZkRet create_ephemeral_node(const string &path, const string &value, bool recursive = true);
	// sequence node, the created node's name is not equal to the given path, it is like "path-xx", xx is an auto-increment number 
	ZkRet create_sequence_node(const string &path, const string &value, string &rpath, bool recursive = true);

	void add_watch_node_value(const string &path, node_value_changed_t* cb, uint32_t flags);
	void del_watch_node_value(const string &path, uint32_t flags);
	void add_watch_children_list(const string &path, children_list_changed_t* cb, uint32_t flags);
	void del_watch_children_list(const string &path, uint32_t flags);

	void set_debug_loglevel(bool open = true);
	void set_log_stream(FILE *stream){ zoo_set_log_stream(stream); }

	string get_path_by_id(uint64_t);
	void do_node_value_changed(uint64_t id, const string& value);
	void do_children_list_changed(uint64_t id, const vector<string>& vecs);

	void re_add_watch(bool only_fail);
	void re_add_watch_node_value(const string &path);
	void re_add_watch_children_list(const string &path);

	void set_connect_state(bool connected)
	{
		_connected = connected;
	}
	
private:
	zk_clt();
	virtual ~zk_clt();

	zhandle_t*	_zk_handler;
	string		_connectString;
	bool		_connected;
	ZooLogLevel _defaultLogLevel;

	struct watch_item
	{
		watch_item(uint64_t id, node_value_changed_t* cb, uint32_t flags)
		{
			_id = id;
			_value_cb = cb;
			_list_cb = NULL;
			_suc = false;
			_flags = flags;
		}
		watch_item(uint64_t id, children_list_changed_t* cb, uint32_t flags)
		{
			_id = id;
			_list_cb = cb;
			_value_cb = NULL;
			_suc = false;
			_flags = flags;
		}
		watch_item(const watch_item& other)
		{
			_id = other._id;
			_list_cb = other._list_cb;
			_value_cb = other._value_cb;
			_suc = other._suc;
			_flags = other._flags;
		}
		watch_item()
		{
			_id = 0;
			_list_cb = NULL;
			_value_cb = NULL;
			_suc = false;
			_flags = 0;
		}

		uint64_t					_id;
		node_value_changed_t*		_value_cb;
		children_list_changed_t*	_list_cb;
		bool						_suc;//设置成功
		uint32_t						_flags;
	};	
	uint64_t						_watch_id_counter;
	map<uint64_t, string>			_watch_id_2_path;

	map<string, watch_item>		_node_value_watch_table;
	map<string, watch_item>		_children_list_watch_table;

	uint64_t	get_id_by_node(const string &path);
	uint64_t	get_id_by_children(const string &path);


	ZkRet create_node_helper(int flag, const string &path, const string &value, char *rpath, int rpathlen, bool recursive);
	void setDebugLogLevel(ZooLogLevel loglevel);
};

#endif