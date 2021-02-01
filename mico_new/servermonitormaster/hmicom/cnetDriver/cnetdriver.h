/**********************************************************************
Copyright (c) 2008-2010 by SHENZHEN CO-TRUST AUTOMATION TECH.
** 文 件 名:  cnetdriver.h
** 说    明:  LS CNET协议驱动头文件，定义各个区域和包格式
** 版    本:  【可选】
** 创建日期:  2012.07.11
** 创 建 人:  陶健军
** 修改信息： 【可选】
**         修改人    修改日期       修改内容**
*********************************************************************/
#ifndef CNET_DRIVER_H
#define CNET_DRIVER_H

#include "../datatypedef.h"

#ifdef __KERNEL__
extern void ct_assert( INT8U cond, char* file, int line );
#define CT_ASSERT(cond)       ct_assert((int)cond, __FILE__, __LINE__);
#endif

#define M_CNET_RETRIES      2     // Number of retries requested
#define M_CNET_MAX          32   // Buffer bytes available for data writes

#define M_READ   0      //CNET 读
#define M_WRITE  1      //CNET 写

#define CNET_AREA_P    0
#define CNET_AREA_K    1
#define CNET_AREA_M    2
#define CNET_AREA_L    3
#define CNET_AREA_F    4
#define CNET_AREA_T    5
#define CNET_AREA_C    6

#define CNET_AREA_S    7
#define CNET_AREA_D    8
#define CNET_AREA_TV   9
#define CNET_AREA_CV   10
#define CNET_AREA_WP   11
#define CNET_AREA_WK   12
#define CNET_AREA_WM   13
#define CNET_AREA_WL   14
#define CNET_AREA_WF   15
/*
寄存器类型： P		范围： 0.0- 255.f（仅bit）
寄存器类型： K		范围： 0.0- 255.f（仅bit）
寄存器类型： M		范围： 0.0- 255.f（仅bit）
寄存器类型： L		范围： 0.0- 255.f（仅bit）
寄存器类型： F		范围： 0.0- 255.f（仅bit）
寄存器类型： S		范围： 0 - 9999  (仅字 )
寄存器类型： T		范围： 0- 255（仅bit）
寄存器类型： C		范围： 0.0- 255.f （仅bit）
寄存器类型： D		范围： 0.0 - 9999.f
寄存器类型： TV		范围： 0 - 255 (仅字 )
寄存器类型： CV		范围： 0 - 255 (仅字 )
寄存器类型： WP		范围： 0 - 255 (仅字 )
寄存器类型： WK		范围： 0 - 255 (仅字 )
寄存器类型： WM		范围： 0 - 255 (仅字 )
寄存器类型： WL		范围： 0 - 255 (仅字 )
寄存器类型： WF		范围： 0 - 255 (仅字 )
以上寄存器中，WP，WK，WM，WL, WF分别为P, K, M, L, F对应的WORD类型。
*/
#define CNET_NO_ERR    0
#define CNET_TIMEOUT   1
#define CNET_PARM_ERR  2
//#define CNET_CRC_ERR   3
//#define CNET_RSP_ERR   4
#define C_ENQ 0x05	//请求       请求帧初始代码
#define C_ACK 0x06	//正常应答   ACK响应帧初始代码
#define C_NAK 0x15	//错误应答   NAK响应帧初始代码
#define C_EOT 0x04	//正文结束   请求帧结束ASCII码
#define C_ETX 0x03	//正文结束   响应帧结束ASCII码

#define RS485_RECEIVE  0x00			// 把总线置于接收状态
#define RS485_SEND     0x01			// 把总线置于发送状态(在发送状态，依然可以接收)
//
#pragma pack(push,1)
struct  CnetDataPacket
{
        INT8U  slave[2];  //1
        INT8U  function;//2
        INT8U  code[2];//3
        INT8U  areaLen[2];
        INT8U  separate;//分隔符 '%'
        INT8U  area[2];    //4
        INT8U  addr[4];   //6
        INT8U  rwLen[2];
        // INT8U  BINData[128];//8
        INT8U  ASCIIData[512];
};
#pragma pack(pop)

struct CnetNewPackLenPar
{
    INT8U packLen;
    INT8U maxToRcv;
};

// MbusMsg 函数传递参数结构体定义
struct CnetMsgPar
{
    INT8U sfd;//socket fd
    INT16U serialNum;
    INT8U  slave;   //Slave address (0 - 247)
    INT8U  rw;      //Read = 0, Write = 1
    INT8U area;     //0x, 1x, 2x, 3x, 4x
    INT16U addr;    //CNET addr (ie 00001)
    INT16U count;   //Number of elements (1- 120 words or 1 to 1920 bits)
    INT8U data[256];//Pointer to data (ie &VB100)
    INT8U err;      //Error code (0 = no error)
};

#endif //CNET_DRIVER_H
