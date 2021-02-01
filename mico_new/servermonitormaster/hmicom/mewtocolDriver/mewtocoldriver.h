#ifndef MEWTOCOL_DRIVER_H
#define MEWTOCOL_DRIVER_H

#include "../datatypedef.h"

#ifdef __KERNEL__
extern void ct_assert( INT8U cond, char* file, int line );
#define CT_ASSERT(cond)       ct_assert((int)cond, __FILE__, __LINE__);
#endif

#define M_MEWTOCOL_RETRIES      2     // Number of retries requested
#define M_MEWTOCOL_MAX          96   // Buffer bytes available for data writes
#define M_MEWTOCOL_MAX_WORD		24
#define M_MEWTOCOL_MAX_BIT		21


#define M_READ   0      //MEWTOCOL 读
#define M_WRITE  1      //MEWTOCOL 写

// #define MEWTOCOL_AREA_X    0
// #define MEWTOCOL_AREA_Y    1
// #define MEWTOCOL_AREA_R    2
// #define MEWTOCOL_AREA_T    3
// #define MEWTOCOL_AREA_C    4
// #define MEWTOCOL_AREA_L    5
#define MEWTOCOL_AREA_DT   1  //word addr
#define MEWTOCOL_AREA_LD   2
#define MEWTOCOL_AREA_FL   3
#define MEWTOCOL_AREA_SV   4
#define MEWTOCOL_AREA_EV   5
#define MEWTOCOL_AREA_WX   6
#define MEWTOCOL_AREA_WY   7
#define MEWTOCOL_AREA_WR   8
#define MEWTOCOL_AREA_WL   9

/*#define MEWTOCOL_AREA_X_BIT    10 //bit addr
#define MEWTOCOL_AREA_Y_BIT    11
#define MEWTOCOL_AREA_R_BIT    12
#define MEWTOCOL_AREA_L_BIT    13*/
#define MEWTOCOL_AREA_C_BIT    10
#define MEWTOCOL_AREA_T_BIT    11

#define MEWTOCOL_NO_ERR    0
#define MEWTOCOL_TIMEOUT   1
#define MEWTOCOL_PARM_ERR  2
//#define MEWTOCOL_CRC_ERR   3
//#define MEWTOCOL_RSP_ERR   4

#define RS485_RECEIVE  0x00			// 把总线置于接收状态
#define RS485_SEND     0x01			// 把总线置于发送状态(在发送状态，依然可以接收)
//
#pragma pack(push,1)
struct MewtocolDataPacket
{
        INT8U  start;
        INT8U  slave[2];
        INT8U  symbol;
        INT8U  founction[3];
        INT8U  area[2];
        INT8U  wordAddr[4];
        //INT8U  bitAddr;
        INT8U  startAddr[5];
        INT8U  endAddr[5];
        INT8U  data[M_MEWTOCOL_MAX];
        INT8U  bcc[2];
        INT8U  CR;
        INT8U  sendData[118];
};
#pragma pack(pop)

struct Mew_NewPackLenPar
{
    INT8U packLen;
    INT8U maxToRcv;
};

// MbusMsg 函数传递参数结构体定义
struct MewtocolMsgPar
{
    INT8U  sfd;
    INT16U serialNum;
    INT8U  slave;   //Slave address (0 - 247)
    INT8U  rw;      //Read = 0, Write = 1
    INT8U area;     //0x, 1x, 2x, 3x, 4x
    INT32U wordAddr; ///wrod addr (ie 0001) 0x0000-0xffff
    INT8U  bitAddr; //bit addr (ie 01) 0x00 - 0x0f
    INT16U count;   //Number of elements (1- 24 words or 1 to 1920 bits)
    INT8U data[M_MEWTOCOL_MAX];//Pointer to data (ie &VB100)
    INT8U err;      //Error code (0 = no error)
};

#endif //MEWTOCOL_DRIVER_H
