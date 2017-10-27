#ifndef NAME_PROTO_H_
#define NAME_PROTO_H_

#define __STDC_LIMIT_MACROS

#include <stdint.h>

const uint16_t cmd_inner_name_query = 0x10;
const uint16_t cmd_inner_name_report = 0x11;

//client_role	->	name_svr 
//	CMD 0x10	Query by server-cluster-name
//		C->S	u16ClusterNum * (u8ClusterNameLen * u8ClusterNameText), 
//				for simply, 1 request will echo n response
//		S->C	u8Result + stOther
//					u8Result = 0,		suc, stOther = u8ClusterNameLen * u8ClusterNameText + u16ServerNodeNum * (u32Time + u8ServerNodeNameLen * u8ServerNodeNameText + u32IP
//													   + u16Port + u16RegionID + u16IdcID + u32FullLoad + u32CurrentLoad + u8TagNum * (u8KeyNameLen * u8KeyNameText + u8ValueLen * u8ValueText))
//					u8Result = other, fail,	 stOther = null

//server_role	->	name_svr 
//	CMD 0x11	Report
//		C->S	u16Num * (u32Time + u8ClusterNameLen * u8ClusterNameText + u8ServerNodeNameLen * u8ServerNodeNameText + u32IP + u16Port + u16RegionID + u16IdcID + u32FullLoad + u32CurrentLoad
//							+ u8TagNum * (u8KeyNameLen * u8KeyNameText + u8ValueLen * u8ValueText))
//		S->C	u8Result + stOther
//					u8Result = 0,		suc, stOther = null
//					u8Result = other, fail,	 stOther = null
//
//	Notice:     every line must keep timeout watch

#endif //NAME_PROTO_H_