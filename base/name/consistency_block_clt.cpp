#include "stdio.h"
#include "auto_mutex.h"
#include "consistency_block_clt.h"
#include "balance_client.h"
#include "master_client.h"
#include "balance_service.h"
#include "master_service.h"
#include "string_func.h"
#include "zk_clt.h"
#include "json/json.h"
#include "name_inner_def.h"
#include "time_func.h"

using namespace Json;
//Services
//	|	
//	|-----Cluster_A
//	|		|
//	|		|--------Master	( = xxxx)			//仅仅Master下client和server	watch
//	|		|
//	|		|--------List						//仅仅Balance下client和server	watch
//	|				|	
//	|				|------svr1	( = yyy)
//	|				|
//	|				|------svr2 ( = zzz)
//	|
//	|------Cluster_B
//	|		|
//  |		...

const	uint32_t	WAIT_SLEEP_INTERVAL = 200;
const	uint32_t	LOOP_SLEEP_INTERVAL = 100;
const	uint32_t	ZK_INIT_TRY_NUM = 20;
const	uint32_t	ZK_INIT_SLEEP_INTERVAL = 100;

const	uint32_t	FLAG_GET_FAIL = 0x01;
//const	uint32_t	FLAG_WATCH_FAIL = 0x02;

enum zk_thread_state
{
	ZTS_NONE = 0,
	ZTS_TAKEPLACE = 1,
	ZTS_CONNECTTING = 2,
	ZTS_CONNECTTED_SUC = 3,
	ZTS_CONNECTTED_FAIL = 4,
};
static	pthread_mutex_t	_zk_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
static	zk_thread_state	_zk_thread_state = ZTS_NONE;

static uint64_t clt_blance_ver_old = 0;
static uint64_t clt_master_ver_old = 0;
static uint64_t srv_blance_ver_old = 0;
static uint64_t srv_master_ver_old = 0;
static map<string, uint64_t> clt_blance_watch_list_old;
static map<string, uint64_t> clt_master_watch_list_old;
static map<string, Name::BalanceMode::SvrNodeData> srv_blance_watch_list_old;
static map<string, Name::MasterMode::SvrNodeData> srv_master_watch_list_old;


static cluster_name_t get_cluster_name_helper(const string& name)
{
	std::size_t pos1 = name.find("/Services/");
	if (pos1 == std::string::npos)
	{
		return "";
	}
	pos1 += strlen("/Services/");
	if (pos1 >= name.length())
	{
		return "";
	}

	std::size_t pos2 = name.find("/", pos1 + 1);
	if (pos2 == std::string::npos)
	{
		return "";
	}

	return name.substr(pos1, pos2 - pos1);
}

static section_name_t get_section_name_helper(const string& name)
{
	std::size_t pos1 = name.find("/Services/");
	if (pos1 == std::string::npos)
	{
		return "";
	}
	std::size_t pos2 = name.find("/Master");
	if (pos2 != std::string::npos && pos2 + strlen("/Master") == name.size())
	{
		return "";
	}

	std::size_t pos3 = name.rfind('/');
	if (pos1 == std::string::npos || pos3 <= pos1)
	{
		return "";
	}

	return name.substr(pos3 + 1, name.length() - pos3 - 1);
}

static bool decode_balance_node_value_helper(const string &path, const string &value, Name::BalanceMode::SvrNodeData& data)
{
	if (path.rfind("/Master") + strlen("/Master") == path.length())
	{
		return false;
	}

	data.cluster_name = get_cluster_name_helper(path);
	data.section_name = get_section_name_helper(path);

	Value	r;
	Json::Reader	reader;
	if (!reader.parse(value, r))
	{
		return false;
	}

	if (!r.isObject())
	{
		return false;
	}
	if (!r["IP"].isInt())
	{
		return false;
	}
	data.ip = r["IP"].asInt();	
	if (!r["Port"].isInt())
	{
		return false;
	}
	data.port = r["Port"].asInt();	
	if (!r["RegionId"].isInt())
	{
		return false;
	}
	data.region_id = r["RegionId"].asInt();	
	if (!r["IdcId"].isInt())
	{
		return false;
	}
	data.idc_id = r["IdcId"].asInt();	
	if (!r["FullLoad"].isInt())
	{
		return false;
	}
	data.full_load = r["FullLoad"].asInt();	
	if (!r["CurrentLoad"].isInt())
	{
		return false;
	}
	data.current_load = r["CurrentLoad"].asInt();	
	if (!r["Tags"].isObject())
	{
		return false;
	}	
	Value	a = r["Tags"];	
		
	Json::Value::Members mem = a.getMemberNames();        
    for (Json::Value::Members::iterator it = mem.begin(); it != mem.end(); it++)        	
	{
		if(a[*it].isString())
		{			
			data.tags[*it] = a[*it].asString();	
		}
	}

	return true;
}

static bool decode_master_node_value_helper(const string &path, const string &value, Name::MasterMode::SvrNodeData& data)
{
	if (path.rfind("/Master") + strlen("/Master") != path.length())
	{
		return false;
	}

	Value	r;
	Json::Reader	reader;
	if (!reader.parse(value, r))
	{
		return false;
	}

	if (!r.isObject())
	{
		return false;
	}
	if (!r["IP"].isInt())
	{
		return false;
	}
	data.ip = r["IP"].asInt();	
	if (!r["Port"].isInt())
	{
		return false;
	}
	data.port = r["Port"].asInt();	
	if (!r["UniqueId"].isInt())
	{
		return false;
	}
	data.unique_id = r["UniqueId"].asInt();		
	if (!r["SectionName"].isString())
	{
		return false;
	}
	data.section_name = r["SectionName"].asString();			
	data.cluster_name = get_cluster_name_helper(path);

	return true;
}

//协议
//ParentNode:	(Path)ClusterName
//BlanceNode:	(Path)SectionName	(Value)u32IP + u16Port + u16RegionId + u16IdcId + u32FullLoad + u32CurrentLoad + u8TagNum * (u8TagNameLen * u8TagNameText + u16TagValueLen * u8TagValueData)
//MasterNode:	(Path)"Master"		(Value)u8SectionNameLen * u8SectionNameText + u32IP + u16Port + u16UniqueId
static bool on_node_value_changed(const string &path, const string &value, uint32_t flags)
{
	if (path.rfind("/Master") + strlen("/Master") != path.length())
	{
		Name::BalanceMode::SvrNodeData data;
		if (!decode_balance_node_value_helper(path, value, data))
		{
			return false;
		}

		BalanceModeImp::Server::set_server_data(data);
		BalanceModeImp::Client::set_server_data(data);
	}
	else
	{
		Name::MasterMode::SvrNodeData data;
		
		if (!decode_master_node_value_helper(path, value, data))
		{
			return false;
		}

		data.cluster_name = get_cluster_name_helper(path);
		if (flags & WATCH_BIT_CLIENT)
		{
			MasterModeImp::Client::set_master(data);
		}

		if (flags & WATCH_BIT_SERVER)
		{
			MasterModeImp::Server::set_master(data);
		}
	}
		
	return true;
}

static bool on_children_list_changed(const string &path, const vector<string> &children, uint32_t flags)
{
	map<string, Name::BalanceMode::SvrNodeData> v;
	for (vector<string>::const_iterator it2 = children.begin(); it2 != children.end(); it2++)
	{
		string node_value;
		string child_path = path + "/" + *it2;
		bool ret = zk_clt::get_instance()->get_node_value(child_path, node_value);
		if (!ret)
		{
			return false;
		}

		Name::BalanceMode::SvrNodeData data;
		if (!decode_balance_node_value_helper(child_path, node_value, data))
		{
			return false;
		}

		v[data.section_name] = data;
	}

	BalanceModeImp::Client::set_cluster_data(v);
	BalanceModeImp::Server::set_cluster_data(v);

	return true;
}

static bool fetch_children_list_clt_helper(const string& name)
{
	vector<string> children;
	bool ret = zk_clt::get_instance()->get_children_list(name, children);
	if (!ret)
	{
		return false;
	}

	return on_children_list_changed(name, children, WATCH_BIT_CLIENT);
}

static bool fetch_node_value_clt_helper(const string& name)
{
	string value;
	bool ret = zk_clt::get_instance()->get_node_value(name, value);
	if (!ret)
	{
		return false;
	}
	
	return on_node_value_changed(name, value, WATCH_BIT_CLIENT);
}

static bool set_node_value_balance_srv_helper(string path, const Name::BalanceMode::SvrNodeData& data)
{
	Value	w(objectValue);
	w["IP"] = data.ip;
	w["Port"] = data.port;
	w["RegionId"] = data.region_id;
	w["IdcId"] = data.idc_id;
	w["FullLoad"] = data.full_load;
	w["CurrentLoad"] = data.current_load;

	Value	tags(objectValue);
	w["Tags"] = tags;

	for (map<string, string>::const_iterator it = data.tags.begin(); it != data.tags.end(); it++)
	{
		tags[it->first] = it->second;
	}

	FastWriter writer;
	std::string doc = writer.write(w);
	bool ret = zk_clt::get_instance()->set_node_value(path, doc);
	if (ret)
	{
		return true;
	}

	return false;
}

static bool set_node_value_master_srv_helper(const string path, const Name::MasterMode::SvrNodeData& data)
{
	Value	w(objectValue);
	w["SectionName"] = data.section_name;
	w["IP"] = data.ip;
	w["Port"] = data.port;	
	w["UniqueId"] = data.unique_id;;

	FastWriter writer;
	std::string doc = writer.write(w);

	static int s_i = 0;
	printf("[%d] set_node_value_master_srv_helper\n", ++s_i);

	if (zk_clt::get_instance()->exists(path))
	{
		bool ret = zk_clt::get_instance()->get_node_value(path, doc);
		if (ret)
		{
			Name::MasterMode::SvrNodeData data2;

			if (!decode_master_node_value_helper(path, doc, data2))
			{
				printf("decode_master_node_value_helper failed\n");
				return false;
			}

			MasterModeImp::Server::set_master(data2);

			return true;
		}
		else
		{
			printf("master_server get_node_value failed\n");
		}
	}
	else
	{
		ZkRet ret = zk_clt::get_instance()->create_ephemeral_node(path, doc);
		if (ret.ok())
		{
			MasterModeImp::Server::set_master(data);

			return true;
		}
		else
		{
			printf("master_server create_ephemeral_node failed: %d\n", ret.Code());
		}
	}

	return false;
}

static void do_check_watch()
{
	zk_clt* zk = zk_clt::get_instance();

	uint64_t clt_blance_ver = clt_blance_ver_old;
	map<string, uint64_t> clt_blance_watch_list;
	if (BalanceModeImp::Client::get_watch_list(clt_blance_ver, clt_blance_watch_list))
	{
		//获取初始值
		for (map<string, uint64_t>::iterator it = clt_blance_watch_list.begin(); it != clt_blance_watch_list.end(); it++)
		{
			string name = "/Services/";
			name += it->first + "/List";

			if (!fetch_children_list_clt_helper(name))
			{
				it->second |= FLAG_GET_FAIL;
			}
			else
			{
				it->second &= ~FLAG_GET_FAIL;				
			}
		}

		//删除被废弃的
		for (map<string, uint64_t>::iterator it = clt_blance_watch_list_old.begin(); it != clt_blance_watch_list_old.end(); it++)
		{
			if (clt_blance_watch_list.find(it->first) == clt_blance_watch_list.end())
			{			
				string name = "/Services/";
				name += it->first + "/List";
				zk->del_watch_children_list(name, WATCH_BIT_CLIENT);
			}
		}

		//添加新的
		for (map<string, uint64_t>::iterator it = clt_blance_watch_list.begin(); it != clt_blance_watch_list.end(); it++)
		{
			if (clt_blance_watch_list_old.find(it->first) == clt_blance_watch_list_old.end())
			{		
				string name = "/Services/";
				name += it->first + "/List";
				zk->add_watch_children_list(name, on_children_list_changed, WATCH_BIT_CLIENT);
			}
		}
		
		clt_blance_ver_old = clt_blance_ver;
		clt_blance_watch_list_old = clt_blance_watch_list;
	}
	else
	{
		//删除被废弃的
		for (map<string, uint64_t>::iterator it = clt_blance_watch_list_old.begin(); it != clt_blance_watch_list_old.end(); it++)
		{
			string name = "/Services/";
			name += it->first + "/List";
			if (it->second & FLAG_GET_FAIL)
			{
				if (fetch_children_list_clt_helper(name));//同步
				{
					it->second &= ~FLAG_GET_FAIL;
				}
			}
		}
	}

	uint64_t clt_master_ver = clt_master_ver_old;
	map<string, uint64_t> clt_master_watch_list;
	if (MasterModeImp::Client::get_cluster_list(clt_master_ver, clt_master_watch_list))
	{
		for (map<string, uint64_t>::iterator it = clt_master_watch_list.begin(); it != clt_master_watch_list.end(); it++)
		{
			string name = "/Services/";
			name += it->first + "/Master";
				
			if (!fetch_node_value_clt_helper(name))
			{
				it->second |= FLAG_GET_FAIL;
			}
			else
			{
				it->second &= ~FLAG_GET_FAIL;
			}
		}

		//删除被废弃的
		for (map<string, uint64_t>::iterator it = clt_master_watch_list_old.begin(); it != clt_master_watch_list_old.end(); it++)
		{
			if (clt_master_watch_list.find(it->first) == clt_master_watch_list.end())
			{
				string name = "/Services/";
				name += it->first + "/Master";
				zk->del_watch_node_value(name, WATCH_BIT_CLIENT);
			}
		}
		//添加新的
		for (map<string, uint64_t>::iterator it = clt_master_watch_list.begin(); it != clt_master_watch_list.end(); it++)
		{
			if (clt_master_watch_list_old.find(it->first) == clt_master_watch_list_old.end())
			{
				string name = "/Services/";
				name += it->first + "/Master";
				zk->add_watch_node_value(name, on_node_value_changed, WATCH_BIT_CLIENT);
			}
		}

		clt_master_ver_old = clt_master_ver;
		clt_master_watch_list_old = clt_master_watch_list;
	}
	else
	{
		//删除被废弃的
		for (map<string, uint64_t>::iterator it = clt_master_watch_list_old.begin(); it != clt_master_watch_list_old.end(); it++)
		{
			string name = "/Services/";
			name += it->first + "/Master";
			if (it->second & FLAG_GET_FAIL)
			{
				if (fetch_node_value_clt_helper(name));//同步
				{
					it->second &= ~FLAG_GET_FAIL;
				}
			}
		}
	}

	//////////////////
	uint64_t srv_blance_ver = srv_blance_ver_old;
	map<string, Name::BalanceMode::SvrNodeData>	srv_blance_data;
	if (BalanceModeImp::Server::get_reg_list(srv_blance_ver, srv_blance_data))
	{
		//set-value
		for(map<string, Name::BalanceMode::SvrNodeData>::iterator it = srv_blance_data.begin(); it != srv_blance_data.end(); it++)
		{
			string name = "/Services/";
			name += it->second.cluster_name + "/List/" + it->second.section_name;

			if (!set_node_value_balance_srv_helper(name, it->second))//同步
			{
				it->second.flags |= FLAG_GET_FAIL;
			}
			else
			{
				it->second.flags &= ~FLAG_GET_FAIL;
			}
		}

		//删除被废弃的
		for (map<string, Name::BalanceMode::SvrNodeData>::iterator it = srv_blance_watch_list_old.begin(); it != srv_blance_watch_list_old.end(); it++)
		{
			if (srv_blance_data.find(it->first) == srv_blance_data.end())
			{
				string name = "/Services/";
				name += it->first + "/List";
				zk->del_watch_children_list(name, WATCH_BIT_SERVER);
			}
		}

		//添加新的
		for (map<string, Name::BalanceMode::SvrNodeData>::iterator it = srv_blance_data.begin(); it != srv_blance_data.end(); it++)
		{
			if (srv_blance_watch_list_old.find(it->first) == srv_blance_watch_list_old.end())
			{
				string name = "/Services/";
				name += it->first + "/List";
				zk->add_watch_children_list(name, on_children_list_changed, WATCH_BIT_SERVER);
			}
		}

		srv_blance_ver_old = srv_blance_ver;
		srv_blance_watch_list_old = srv_blance_data;
	}
	else
	{
		//删除被废弃的
		for (map<string, Name::BalanceMode::SvrNodeData>::iterator it = srv_blance_watch_list_old.begin(); it != srv_blance_watch_list_old.end(); it++)
		{
			string name = "/Services/";
			name += it->second.cluster_name + "/List/" + it->second.section_name;
			
			if (it->second.flags & FLAG_GET_FAIL)
			{
				if (set_node_value_balance_srv_helper(name, it->second));//同步
				{
					it->second.flags &= ~FLAG_GET_FAIL;
				}
			}
		}
	}

	uint64_t srv_master_ver = srv_master_ver_old;
	map<string, Name::MasterMode::SvrNodeData>	srv_master_data;
	if (MasterModeImp::Server::get_reg_and_cluster_list(srv_master_ver, srv_master_data))
	{
		//set-value
		map<string, uint64_t> srv_master_watch_list;
		for (map<string, Name::MasterMode::SvrNodeData>::iterator it = srv_master_data.begin(); it != srv_master_data.end(); it++)
		{
			string name = "/Services/";
			name += it->second.cluster_name + "/Master";

			if (!set_node_value_master_srv_helper(name, it->second))
			{
				it->second.flags |= FLAG_GET_FAIL;
			}
			else
			{
				it->second.flags &= ~FLAG_GET_FAIL;
			}
		}

		//watch

		//删除被废弃的
		for (map<string, Name::MasterMode::SvrNodeData>::iterator it = srv_master_watch_list_old.begin(); it != srv_master_watch_list_old.end(); it++)
		{
			if (srv_master_watch_list.find(it->first) == srv_master_watch_list.end())
			{
				string name = "/Services/";
				name += it->second.cluster_name + "/Master";

				zk->del_watch_children_list(it->first, WATCH_BIT_SERVER);
			}
		}
		//添加新的
		for (map<string, Name::MasterMode::SvrNodeData>::iterator it = srv_master_data.begin(); it != srv_master_data.end(); it++)
		{
			if (srv_master_watch_list_old.find(it->first) == srv_master_watch_list_old.end())
			{
				string name = "/Services/";
				name += it->second.cluster_name + "/Master";

				zk->add_watch_node_value(name, on_node_value_changed, WATCH_BIT_SERVER);
			}
		}

		srv_master_ver_old = srv_master_ver;
		srv_master_watch_list_old = srv_master_data;
	}
	else
	{
		for (map<string, Name::MasterMode::SvrNodeData>::iterator it = srv_master_watch_list_old.begin(); it != srv_master_watch_list_old.end(); it++)
		{
			if (it->second.flags & FLAG_GET_FAIL)
			{
				string name = "/Services/";
				name += it->second.cluster_name + "/Master";
				if (set_node_value_master_srv_helper(name, it->second))
				{
					it->second.flags &= ~FLAG_GET_FAIL;
				}
			}
		}
	}

	zk->re_add_watch(true);
}

//读取外部控制变量是否退出线程	thread_exit
//写入连接是否成功变量			zk_run
static void* zk_thread_routine(void* thread_param)
{
	do
	{
		auto_mutex	mutex(&_zk_mutex);
		_zk_thread_state = ZTS_CONNECTTING;
	} while (0);

	char* connect_string = (char*)thread_param;
	zk_clt* zk = zk_clt::get_instance(true);

	bool init_suc = false;
	
	for (uint32_t i = 0; i < ZK_INIT_TRY_NUM; i++)
	{
		if (zk->init(connect_string))
		{
			init_suc = true;
			break;
		}

		usleep(ZK_INIT_SLEEP_INTERVAL * 1000);
	}
	
	free(connect_string);

	if (init_suc)
	{	
		do
		{
			auto_mutex	mutex(&_zk_mutex);
			_zk_thread_state = ZTS_CONNECTTED_SUC;
		} while (0);

		for (uint64_t counter = 0; ;)
		{
			uint64_t now = Time::now_ms();
			if (counter + GATHER_INTERVAL <= now)
			{
				counter = now;
				//检查watch
				do_check_watch();
			}

			usleep(LOOP_SLEEP_INTERVAL);
		}
	}
	else
	{
		do
		{
			auto_mutex	mutex(&_zk_mutex);
			_zk_thread_state = ZTS_CONNECTTED_FAIL;
		} while (0);
	}
	
	return NULL;
}

//init是否正在进行
//写入控制变量是否退出线程	thread_exit
//读取连接是否成功变量		zk_run
bool consistency_block_clt::init(const string zk_svr_ip_list, uint32_t ms/* = ZK_BLOCK_INIT_TIMEOUT*/)
{
	do
	{
		auto_mutex	mutex(&_zk_mutex);
		if (_zk_thread_state != ZTS_NONE)
		{
			break;
		}
		_zk_thread_state = ZTS_TAKEPLACE;

		//另起发送线程
		pthread_attr_t attr;
		if (0 != pthread_attr_init(&attr))
		{
			break;
		}
		if (0 != pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED))
		{
			break;
		}

		char* connect_string = strdup(zk_svr_ip_list.c_str());
		if (!connect_string)
		{
			break;
		}
		
		pthread_t thread_id = 0;
		int32_t ret = pthread_create(&thread_id, &attr, &zk_thread_routine, connect_string);
		pthread_attr_destroy(&attr);
		if (0 > ret)
		{
			free(connect_string);
			break;
		}
	} while (0);

	while (1)
	{
		do 
		{
			auto_mutex	mutex(&_zk_mutex);
			if (_zk_thread_state == ZTS_CONNECTTED_SUC || _zk_thread_state == ZTS_CONNECTTED_FAIL)
			{
				bool ret = true;
				if (_zk_thread_state == ZTS_CONNECTTED_FAIL)
				{
					ret = false;
					_zk_thread_state = ZTS_NONE;
				}
				return ret;
			}
		} while (0);

		usleep(WAIT_SLEEP_INTERVAL * 1000);
	}

	return false;
}
