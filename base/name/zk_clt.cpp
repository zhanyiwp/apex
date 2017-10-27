#ifdef WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif
#include <assert.h>
#include <sstream>
#include "zk_clt.h"
#include "zookeeper_log.h"

using namespace std;

#define ZK_RECV_TIMEOUT 5000
#define ZK_BUFSIZE 10240

static zk_clt*	zk_instance = NULL;

//global aunx func--------------------------------------------------------------------------
static const char*  errorStr(int code)
{
	switch (code)
	{
	case ZOK:
		return "Everything is OK";
	case ZSYSTEMERROR:
		return "System error";
	case ZRUNTIMEINCONSISTENCY:
		return "A runtime inconsistency was found";
	case ZDATAINCONSISTENCY:
		return "A data inconsistency was found";
	case ZCONNECTIONLOSS:
		return "Connection to the server has been lost";
	case ZMARSHALLINGERROR:
		return "Error while marshalling or unmarshalling data";
	case ZUNIMPLEMENTED:
		return "Operation is unimplemented";
	case ZOPERATIONTIMEOUT:
		return "Operation timeout";
	case ZBADARGUMENTS:
		return "Invalid arguments";
	case ZINVALIDSTATE:
		return "Invalid zhandle state";
	case ZAPIERROR:
		return "Api error";
	case ZNONODE:
		return "Node does not exist";
	case ZNOAUTH:
		return "Not authenticated";
	case ZBADVERSION:
		return "Version conflict";
	case ZNOCHILDRENFOREPHEMERALS:
		return "Ephemeral nodes may not have children";
	case ZNODEEXISTS:
		return "The node already exists";
	case ZNOTEMPTY:
		return "The node has children";
	case ZSESSIONEXPIRED:
		return "The session has been expired by the server";
	case ZINVALIDCALLBACK:
		return "Invalid callback specified";
	case ZINVALIDACL:
		return "Invalid ACL specified";
	case ZAUTHFAILED:
		return "Client authentication failed";
	case ZCLOSING:
		return "ZooKeeper is closing";
	case ZNOTHING:
		return "(not error) no server responses to process";
	case ZSESSIONMOVED:
		return "Session moved to another server, so operation is ignored";
	default:
		return "unknown error";
	}
}

static const char* eventStr(int event)
{
	if (ZOO_CREATED_EVENT == event)
		return "ZOO_CREATED_EVENT";
	else if (ZOO_DELETED_EVENT == event)
		return "ZOO_DELETED_EVENT";
	else if (ZOO_CHANGED_EVENT == event)
		return "ZOO_CHANGED_EVENT";
	else if (ZOO_SESSION_EVENT == event)
		return "ZOO_SESSION_EVENT";
	else if (ZOO_NOTWATCHING_EVENT == event)
		return "ZOO_NOTWATCHING_EVENT";
	else
		return "unknown event";
}

static const char* stateStr(int state)
{
	if (ZOO_EXPIRED_SESSION_STATE == state)
		return "ZOO_EXPIRED_SESSION_STATE";
	else if (ZOO_AUTH_FAILED_STATE == state)
		return "ZOO_AUTH_FAILED_STATE";
	else if (ZOO_CONNECTING_STATE == state)
		return "ZOO_CONNECTING_STATE";
	else if (ZOO_ASSOCIATING_STATE == state)
		return "ZOO_ASSOCIATING_STATE";
	else if (ZOO_CONNECTED_STATE == state)
		return "ZOO_CONNECTED_STATE";
	else
		return "unknown state";
}

static string parentPath(const string &path)
{
	if (path.empty())
		return "";
	//
	size_t pos = path.rfind('/');
	if (path.length() - 1 == pos)
	{
		// skip the tail '/'
		pos = path.rfind('/', pos - 1);
	}
	if (string::npos == pos)
	{
		return "/"; //  parent path of "/" is also "/"
	}
	else
	{
		return path.substr(0, pos);
	}
}

//全局回调--------------------------------------------------------
static void defaultWatcher(zhandle_t *zh, int type, int state, const char *path, void *watcherCtx)
{
	if(type == ZOO_SESSION_EVENT)
	{	
		if(state == ZOO_CONNECTED_STATE)
		{
			zk_instance->set_connect_state(true);
			zk_instance->re_add_watch(false);

			//
			LOG_DEBUG(("connected, session state: %s", stateStr(state)));
		}
		else if(state == ZOO_EXPIRED_SESSION_STATE)
		{
			zk_instance->set_connect_state(false);
			// 
			LOG_ERROR(("session expired"));
			LOG_DEBUG(("restart"));
			zk_instance->restart();
		}
		else
		{
			zk_instance->set_connect_state(false);
			LOG_WARN(("not connected, session state: %s", stateStr(state)));
		}
		// TODO: ZOO_CONNECTED_LOSS
	}
	else if(type == ZOO_CREATED_EVENT)
	{
		LOG_DEBUG(("node created: %s", path));
	}
	else if(type == ZOO_DELETED_EVENT)
	{
		LOG_DEBUG(("node deleted: %s", path));
	}
	else if(type == ZOO_CHANGED_EVENT)
	{
// 		DataWatch *watch = dynamic_cast<DataWatch*>(static_cast<Watch*>(watcherCtx));
// 		LOG_DEBUG(("ZOO_CHANGED_EVENT"));
// 		watch->getAndSet();
		zk_instance->re_add_watch_node_value(path);
	}
	else if(type == ZOO_CHILD_EVENT)
	{
//		ChildrenWatch *watch = dynamic_cast<ChildrenWatch*>(static_cast<Watch*>(watcherCtx));
//		LOG_DEBUG(("ZOO_CHILDREN_EVENT"));
//		watch->getAndSet();
		zk_instance->re_add_watch_children_list(path);
	}
	else
	{
		LOG_WARN(("unhandled zookeeper event: %s", eventStr(type)));
	}
}

void dataCompletion(int rc, const char *value, int valueLen, const struct Stat *stat, const void *data)
{
	if(ZOK == rc)
	{
		zk_instance->do_node_value_changed((uint64_t)(uint64_t*)data, string(value, valueLen));
	}
	else
	{
		// ZCONNECTIONLOSS
		// ZOPERATIONTIMEOUT
		// ZNONODE
		// ZNOAUTH
		LOG_ERROR(("data completion error, ret=%s, path=%s", errorStr(rc), zk_instance->get_path_by_id((uint64_t)(uint64_t*)data).c_str()));
	}
	//
}

void stringsCompletion(int rc, const struct String_vector *strings, const void *data)
{
	if(ZOK == rc)
	{
		vector<string> vecs;
		for(int i = 0; i < strings->count; ++i)
		{
			vecs.push_back(strings->data[i]);
		}
		zk_instance->do_children_list_changed((uint64_t)(uint64_t*)data, vecs);
	}
	else
	{
		// ZCONNECTIONLOSS
		// ZOPERATIONTIMEOUT
		// ZNONODE
		// ZNOAUTH
		LOG_ERROR(("strings completion error, ret=%s, path=%s", errorStr(rc), zk_instance->get_path_by_id((uint64_t)(uint64_t*)data).c_str()));
	}
}

////////////////////////////////////
zk_clt*	zk_clt::get_instance(bool create_if_not_exist/* = false*/)
{
	if (zk_instance)
	{
		return zk_instance;
	}
	if (create_if_not_exist)
	{
		zk_instance = new zk_clt;
		if (zk_instance)
		{
			return zk_instance;
		}		
	}
	
	return NULL;
}

zk_clt::zk_clt()
{
	_watch_id_counter = 0;
	_zk_handler = NULL;
	_connected = false;
	setDebugLogLevel(ZOO_LOG_LEVEL_DEBUG);
}

zk_clt::~zk_clt()
{
	uninit();
}

bool zk_clt::init(const string &connectString)
{
	_connectString = connectString;
	restart();
	return _connected;
}

void zk_clt::uninit()
{
	if (_zk_handler)
	{
		zookeeper_close(_zk_handler);
		_zk_handler = NULL;
	}
}

void zk_clt::restart()
{
	// zookeeper_close(zhandle_); // close an expired session will cause segment fault
	uninit();
	while (!_zk_handler)
	{
		usleep(100 * 1000);
		_zk_handler = zookeeper_init(_connectString.c_str(), defaultWatcher, ZK_RECV_TIMEOUT, NULL, this, 0);
		while (_zk_handler && !_connected)
		{
			usleep(100 * 1000);
		}
	}

	if (!_connected)
	{
		LOG_ERROR(("restart failed."));
	}
}

bool zk_clt::get_node_value(const string &path, string &value)
{
	if(!_zk_handler)
	{
		return false;
	}
	
	char buf[ZK_BUFSIZE] = {0};
	int bufsize = sizeof(buf);
	int ret = zoo_get(_zk_handler, path.c_str(), false, buf, &bufsize, NULL);
	if(ZOK != ret)
	{
		LOG_ERROR(("get %s failed, ret=%s", path.c_str(), errorStr(ret)));
		return false;
	}
	else
	{
		value = buf;
		return true;
	}
}

bool zk_clt::set_node_value(const string &path, const string &value)
{
	if(!_zk_handler)
	{
		return false;
	}
	
	int ret = zoo_set(_zk_handler, path.c_str(), value.data(), value.size(), -1);
	if(ZOK != ret)
	{
		if(ZNONODE == ret)
		{
			// create it, but just a normal node, if you want a different type, create it explicitly before setData
			return create_ephemeral_node(path, value);
		}
		else
		{
			LOG_ERROR(("get %s failed, ret=%s", path.c_str(), errorStr(ret)));
			return false;
		}
	}
	else
	{
		return true;
	}
}

bool zk_clt::get_children_list(const string &path, vector<string> &children)
{	
	if(!_zk_handler)
	{
		return false;
	}
	
	String_vector sv;
	int ret = zoo_get_children(_zk_handler, path.c_str(), false, &sv);
	if(ZOK != ret)
	{
		LOG_ERROR(("get children %s failed, ret=%s", path.c_str(), errorStr(ret)));
		return false;
	}
	else
	{
		children.clear();
		for(int i = 0; i < sv.count; ++i)
		{
			children.push_back(sv.data[i]);
		}
		return true;
	}
}

bool zk_clt::exists(const string &path)
{
	if(!_zk_handler)
	{
		return false;
	}
	
	int ret = zoo_exists(_zk_handler, path.c_str(), false, NULL);
	return (ZOK == ret);
}

ZkRet zk_clt::create_ephemeral_node(const string &path, const string &value, bool recursive/*=true*/)
{
	return create_node_helper(ZOO_EPHEMERAL, path, value, NULL, 0, recursive);
}

ZkRet zk_clt::create_sequence_node(const string &path, const string &value, string &rpath, bool recursive/*=true*/)
{
	char buf[ZK_BUFSIZE] = {0};
	ZkRet zr = create_node_helper(ZOO_SEQUENCE, path, value, buf, sizeof(buf), recursive);
	rpath = buf;
	return zr;
}

ZkRet zk_clt::create_node_helper(int flag, const string &path, const string &value, char *rpath, int rpathlen, bool recursive)
{
	if(!_zk_handler)
	{
		return ZkRet(ZINVALIDSTATE);
	}
		
	assert((NULL == rpath) || (flag == ZOO_SEQUENCE));
	int ret = zoo_create(_zk_handler, path.c_str(), value.data(), value.size(), &ZOO_OPEN_ACL_UNSAFE, flag, rpath, rpathlen);
	if(ZNONODE == ret)
	{
		// create parent node
		string ppath = parentPath(path);
		if(ppath.empty())
		{
			return ZkRet(ret);
		}
		// parent node must not be ephemeral node or sequence node
		ZkRet zr = create_node_helper(0, ppath, "", NULL, 0, true);
		if(zr.ok() || zr.nodeExist())
		{
			// if create parent node ok, then create this node
			ret = zoo_create(_zk_handler, path.c_str(), value.data(), value.size(), &ZOO_OPEN_ACL_UNSAFE, flag, rpath, rpathlen);
			if(ZOK != ret && ZNODEEXISTS != ret)
			{
				LOG_ERROR(("create node failed, path=%s, ret=%s", path.c_str(), errorStr(ret)));
			}
			return ZkRet(ret);	
		}
		else
		{
			return zr;
		}
	}
	else if(ZOK != ret && ZNODEEXISTS != ret)
	{
		LOG_ERROR(("create node failed, path=%s, ret=%s", path.c_str(), errorStr(ret)));
	}
	return ZkRet(ret);
}

void zk_clt::add_watch_node_value(const string &path, node_value_changed_t* cb, uint32_t flags)
{		
	uint64_t	id = get_id_by_node(path);
	if (!id)
	{
		watch_item	item(++_watch_id_counter, cb, flags);
		_node_value_watch_table[path] = item;
	}
	if (_children_list_watch_table[path]._list_cb)
	{
		return;
	}
	_node_value_watch_table[path]._flags |= flags;
	if(!_zk_handler)
	{
		return;
	}
	
	int ret = zoo_awget(_zk_handler, path.c_str(), &defaultWatcher, this, &dataCompletion, (uint64_t*)id);
	if (ZOK != ret)
	{
		_node_value_watch_table[path]._suc = false;
		// ZBADARGUMENTS
		// ZINVALIDSTATE
		// ZMARSHALLINGERROR
		LOG_ERROR(("awget failed, path=%s, ret=%s", path.c_str(), errorStr(ret)));
	}
	else
	{
		_node_value_watch_table[path]._suc = true;
	}
}

void zk_clt::del_watch_node_value(const string &path, uint32_t flags)
{
	map<string, watch_item>::iterator it = _node_value_watch_table.find(path);
	if (it != _node_value_watch_table.end() && it->second._value_cb)
	{
		it->second._flags &= ~flags;
		if (!it->second._flags)
		{
			_node_value_watch_table.erase(it);
		}		
	}
}

void zk_clt::add_watch_children_list(const string &path, children_list_changed_t* cb, uint32_t flags)
{
	uint64_t	id = get_id_by_children(path);
	if (!id)
	{
		watch_item	item(++_watch_id_counter, cb, flags);
		_children_list_watch_table[path] = item;
	}
	if (_children_list_watch_table[path]._value_cb)
	{
		return;
	}
	
	_children_list_watch_table[path]._flags |= flags;

	if(!_zk_handler)
	{
		return;
	}
	
	int ret = zoo_awget_children(_zk_handler, path.c_str(), &defaultWatcher, this, &stringsCompletion, (uint64_t*)id);
	if (ZOK != ret)
	{
		_children_list_watch_table[path]._flags = false;
		LOG_ERROR(("awget_children failed, path=%s, ret=%s", path.c_str(), errorStr(ret)));
	}
	else
	{
		_children_list_watch_table[path]._suc = true;
	}
}

void zk_clt::del_watch_children_list(const string &path, uint32_t flags)
{
	map<string, watch_item>::iterator it = _children_list_watch_table.find(path);
	if (it != _children_list_watch_table.end() && it->second._list_cb)
	{
		it->second._flags &= ~flags;
		if (!it->second._flags)
		{
			_children_list_watch_table.erase(it);
		}
	}
}

void zk_clt::setDebugLogLevel(ZooLogLevel loglevel)
{
	zoo_set_debug_level(loglevel);
}

string zk_clt::get_path_by_id(uint64_t id)
{
	map<uint64_t, string>::iterator it =_watch_id_2_path.find(id);
	if (it != _watch_id_2_path.end())
	{
		return it->second;
	}
	
	return "";
}

void zk_clt::do_node_value_changed(uint64_t id, const string& value)
{
	string path = get_path_by_id(id);
	if (!path.empty())
	{
		map<string, watch_item>::iterator it = _node_value_watch_table.find(path);
		if (it != _node_value_watch_table.end() && it->second._value_cb)
		{
			(it->second._value_cb)(path, value, it->second._flags);
		}		
	}
}

void zk_clt::do_children_list_changed(uint64_t id, const vector<string>& vecs)
{
	string path = get_path_by_id(id);
	if (!path.empty())
	{
		map<string, watch_item>::iterator it = _children_list_watch_table.find(path);
		if (it != _children_list_watch_table.end() && it->second._list_cb)
		{
			(it->second._list_cb)(path, vecs, it->second._flags);
		}
	}
}

void zk_clt::re_add_watch_node_value(const string &path)
{
	map<string, watch_item>::iterator it = _node_value_watch_table.find(path);
	if (it != _node_value_watch_table.end() && it->second._value_cb)
	{
		add_watch_node_value(path, it->second._value_cb, it->second._flags);
	}
}

void zk_clt::re_add_watch_children_list(const string &path)
{
	map<string, watch_item>::iterator it = _children_list_watch_table.find(path);
	if (it != _children_list_watch_table.end() && it->second._list_cb)
	{
		add_watch_children_list(path, it->second._list_cb, it->second._flags);
	}
}

uint64_t	zk_clt::get_id_by_node(const string &path)
{
	map<string, watch_item>::iterator it = _node_value_watch_table.find(path);
	if (it != _node_value_watch_table.end() && it->second._value_cb)
	{
		return it->second._id;
	}

	return 0;
}

uint64_t	zk_clt::get_id_by_children(const string &path)
{
	map<string, watch_item>::iterator it = _children_list_watch_table.find(path);
	if (it != _children_list_watch_table.end() && it->second._list_cb)
	{
		return it->second._id;
	}

	return 0;
}

void zk_clt::re_add_watch(bool only_fail)
{
	for (map<string, watch_item>::iterator it = _node_value_watch_table.begin(); it != _node_value_watch_table.end(); it++)
	{
		if (false == only_fail || false == it->second._suc)
		{
			re_add_watch_node_value(it->first);
		}		
	}
	for (map<string, watch_item>::iterator it = _children_list_watch_table.begin(); it != _children_list_watch_table.end(); it++)
	{
		if (false == only_fail || false == it->second._suc)
		{
			re_add_watch_children_list(it->first);
		}
	}
}