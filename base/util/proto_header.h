#ifndef PROTO_HEADER_H
#define PROTO_HEADER_H

#pragma pack(push)
#pragma pack(1)

typedef enum
{
	CMD_INNER_CHANNEL_HELLO = 1,
	CMD_INNER_CHANNEL_LOGIN = 2,
}COMMAND;

typedef struct 
{
	uint32_t 	clientver;		//版本
	uint16_t 	protover;		//协议版本
	uint16_t  	cmd;			//命令字
	uint16_t 	bodylen;		//包体的长度
	uint32_t 	uid;			//用户ID
	uint16_t 	termtype;		//终端类型
	uint32_t 	lcid;			//区域
	uint32_t 	seq;			//序列号
}ClientHeader;

typedef struct 
{
	uint32_t clientver;			//版本
	uint16_t protover;			//协议版本
	uint16_t cmd;				//命令字
	uint16_t bodylen;			//包体的长度
	uint32_t uid;				//用户ID
	uint16_t termtype;			//终端类型
	uint32_t lcid;				//区域
	uint32_t seq;				//序列号
	uint32_t retip;				//接口层地址
	uint16_t retport;			//接口层端口
	uint32_t connip;			//接入层地址
	uint16_t connport;			//接入层端口
	uint32_t flags;				//染色，新格式包，加密等标记
}InnerHeader;

#define MAX_CLIENT_HEADER_LEN sizeof(ClientHeader)
#define MAX_INNER_HEADER_LEN sizeof(InnerHeader)

#pragma pack(pop)

#endif //PROTO_HEADER_H