/**********************************************************************
Copyright (c) 2008-2010 by SHENZHEN CO-TRUST AUTOMATION TECH.
** 文 件 名:  fatekdriver.h
** 说    明:  永宏FBs协议驱动头文件，定义各个区域和包格式
** 版    本:  【可选】
** 创建日期:  2012.07.05
** 创 建 人:  陶健军
** 修改信息： 【可选】
**         修改人    修改日期       修改内容**
*********************************************************************/
#ifndef FATEK_DRIVER_H
#define FATEK_DRIVER_H

#include "../datatypedef.h"

#ifdef __KERNEL__
extern void ct_assert( INT8U cond, char* file, int line );
#define CT_ASSERT(cond)       ct_assert((int)cond, __FILE__, __LINE__);
#endif

#define M_FATEK_RETRIES      2     // Number of retries requested
#define M_FATEK_MAX          50   // Buffer bytes available for data writes

#define M_READ   0      //FATEK 读
#define M_WRITE  1      //FATEK 写

#define FATEK_AREA_S    0
#define FATEK_AREA_X    1
#define FATEK_AREA_Y    2
#define FATEK_AREA_M    3
#define FATEK_AREA_T    4
#define FATEK_AREA_C    5
#define FATEK_AREA_TMR  6
#define FATEK_AREA_CTR  7
#define FATEK_AREA_CTR200  8
#define FATEK_AREA_HR   9
#define FATEK_AREA_DR   10
#define FATEK_AREA_WX   11
#define FATEK_AREA_WY   12
#define FATEK_AREA_WM   13
#define FATEK_AREA_WS   14

#define FATEK_NO_ERR    0
#define FATEK_TIMEOUT   1
#define FATEK_PARM_ERR  2
//#define FATEK_CRC_ERR   3
//#define FATEK_RSP_ERR   4
#define STX 0x02
#define ETX 0x03

#define RS485_RECEIVE  0x00			// 把总线置于接收状态
#define RS485_SEND     0x01			// 把总线置于发送状态(在发送状态，依然可以接收)
//
#pragma pack(push,1)
struct  FatekDataPacket
{
        INT8U  slave;  //1
        INT8U  function;//2
        INT8U  dataNUM;//3
        INT16U  area;    //4
        INT16U addr;   //6
        INT8U  BINData[256];//8
        INT8U  ASCIIData[512];
};
#pragma pack(pop)

struct FatekNewPackLenPar
{
    INT8U packLen;
    INT8U maxToRcv;
};

// MbusMsg 函数传递参数结构体定义
struct FatekMsgPar
{
    INT8U sfd;
    INT16U serialNum;
    INT8U  slave;   //Slave address (0 - 247)
    INT8U  rw;      //Read = 0, Write = 1
    INT8U area;     //0x, 1x, 2x, 3x, 4x
    INT16U addr;    //FATEK addr (ie 00001)
    INT16U count;   //Number of elements (1- 120 words or 1 to 1920 bits)
    INT8U data[256];//Pointer to data (ie &VB100)
    INT8U err;      //Error code (0 = no error)
};

#endif //FATEK_DRIVER_H
