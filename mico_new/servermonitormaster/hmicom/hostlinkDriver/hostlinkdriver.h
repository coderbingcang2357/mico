#ifndef HOSTLINK_DRIVER_H
#define HOSTLINK_DRIVER_H

#include "../datatypedef.h"

#ifdef __KERNEL__
extern void ct_assert( INT8U cond, char* file, int line );
#define CT_ASSERT(cond)       ct_assert((int)cond, __FILE__, __LINE__);
#endif

#define M_HOSTLINK_RETRIES      2     // Number of retries requested
#define M_HOSTLINK_MAX          112   // Buffer bytes available for data writes
#define M_HOSTLINK_MAX_RCV      125   // Max frame bytes number
#define M_HOSTLINK_MIN_RCV      9    // Min frame bytes number

#define M_READ   0      //MODBUS 读
#define M_WRITE  1      //MODBUS 写

#define HOST_LINK_AREA_CPUSTATUS 0
#define HOST_LINK_AREA_CUPTYPE   1
#define HOST_LINK_AREA_IO        2
#define HOST_LINK_AREA_HR        3
#define HOST_LINK_AREA_AR        4
#define HOST_LINK_AREA_LR        5
#define HOST_LINK_AREA_DM        6
#define HOST_LINK_AREA_TCVAL     7
#define HOST_LINK_AREA_TCBIT     8

#define HOSTLINK_NO_ERR    0
#define HOSTLINK_TIMEOUT   1
#define HOSTLINK_PARM_ERR  2
//#define MODBUS_CRC_ERR   3
//#define MODBUS_RSP_ERR   4

#define RS485_RECEIVE  0x00			// 把总线置于接收状态
#define RS485_SEND     0x01			// 把总线置于发送状态(在发送状态，依然可以接收)
//
#pragma pack(push,1)
struct  HostlinkDataPacket
{
        INT8U  start;
        INT8U  slave[2];  //1
        INT8U  function[2];//2  //3
        INT8U  data[120];//5
};
#pragma pack(pop)

struct hostLinkNewPackLenPar
{
    INT8U packLen;
    INT8U maxToRcv;
};

// MbusMsg 函数传递参数结构体定义
struct HlinkMsgPar
{
    INT8U  sfd;
    INT16U serialNum;
    INT8U  slave;   //Slave address (0 - 247)
    INT8U  rw;      //Read = 0, Write = 1
    INT8U  area;    //0x, 1x, 2x, 3x, 4x
    INT16U addr;    //Modbus addr (ie 00001)
    INT16U count;   //Number of elements (1- 120 words or 1 to 1920 bits)
    INT8U  data[112];//Pointer to data (ie &VB100)
    INT8U  err;      //Error code (0 = no error)
};

#endif //MODBUS_DRIVER_H
