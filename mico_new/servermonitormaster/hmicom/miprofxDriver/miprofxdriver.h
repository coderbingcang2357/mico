#ifndef MIPROFX_DRIVER_H
#define MIPROFX_DRIVER_H

#include "../datatypedef.h"

#ifdef __KERNEL__
extern void ct_assert( INT8U cond, char* file, int line );
#define CT_ASSERT(cond)       ct_assert((int)cond, __FILE__, __LINE__);
#endif

#define M_MIPROFX_RETRIES      2     // Number of retries requested
#define M_MIPROFX_MAX          64   // Buffer bytes available for data writes

#define M_READ        0      //Miprofx 读
#define M_WRITE       1      //Miprofx 写
#define M_FORCE_WRITE 2      //Miprofx 强制写位

#define MIPROFX_AREA_M      0
#define MIPROFX_AREA_X      1
#define MIPROFX_AREA_Y      2
#define MIPROFX_AREA_D      3
#define MIPROFX_AREA_T      4
#define MIPROFX_AREA_C      5
#define MIPROFX_AREA_C200   6
#define MIPROFX_AREA_S	    7

#define MIPROFX_NO_ERR    0
#define MIPROFX_TIMEOUT   1
#define MIPROFX_PARM_ERR  2
//#define MODBUS_CRC_ERR   3
//#define MODBUS_RSP_ERR   4

#define RS485_RECEIVE  0x00			// 把总线置于接收状态
#define RS485_SEND     0x01			// 把总线置于发送状态(在发送状态，依然可以接收)
//
#pragma pack(push,1)
struct MiprofxDataPacket
{
        INT8U  start;
        INT8U  data[256];//5
};
#pragma pack(pop)

struct miprofxNewPackLenPar
{
    INT8U packLen;
    INT8U maxToRcv;
};

// MbusMsg 函数传递参数结构体定义
struct MiprofxMsgPar
{
    INT8U  sfd;
    INT16U serialNum;
    INT8U  rw;      //Read = 0, Write = 1
    INT8U area;     //0x, 1x, 2x, 3x, 4x
    INT16U addr;    //Modbus addr (ie 00001)
    INT16U count;   //Number of elements (1- 120 words or 1 to 1920 bits)
    INT8U data[256];//Pointer to data (ie &VB100)
    INT8U err;      //Error code (0 = no error)
};

#endif //MIPROFX_DRIVER_H
