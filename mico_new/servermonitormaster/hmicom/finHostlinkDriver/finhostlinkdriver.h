#ifndef FIN_HOSTLINK_DRIVER_H
#define FIN_HOSTLINK_DRIVER_H

#include "../datatypedef.h"

#ifdef __KERNEL__
extern void ct_assert( INT8U cond, char* file, int line );
#define CT_ASSERT(cond)       ct_assert((int)cond, __FILE__, __LINE__);
#endif

#define FIN_M_HOSTLINK_RETRIES      2                                    // Number of retries requested
#define FIN_M_HOSTLINK_MAX          256                                  // Buffer bytes available for data writes
#define FIN_M_HOSTLINK_MIN_RCV      ( 5 + sizeof( struct finHostlinkRcvHeader ) + sizeof ( struct finHostlinkRcvCommand ) + 4 )// Min frame bytes number
#define FIN_M_HOSTLINK_MAX_WORD     ( FIN_M_HOSTLINK_MAX / 4 )
#define FIN_M_HOSTLINK_MAX_BIT      ( FIN_M_HOSTLINK_MAX /2 )

#define M_READ   0      //FIN HOSTLINK 读
#define M_WRITE  1      //FIN HOSTLINK 写

/*
#define FIN_HOST_LINK_AREA_CIO      1    //这一段必须俺顺序写，
#define FIN_HOST_LINK_AREA_W        2    //如果要在中间插入内存区，
#define FIN_HOST_LINK_AREA_H        3   //后面的宏要做相应增加。
#define FIN_HOST_LINK_AREA_A        4
#define FIN_HOST_LINK_AREA_DM       5
#define FIN_HOST_LINK_AREA_T        6
#define FIN_HOST_LINK_AREA_C        7
#define FIN_HOST_LINK_AREA_EM0      8
#define FIN_HOST_LINK_AREA_EM1      9
#define FIN_HOST_LINK_AREA_EM2      10
#define FIN_HOST_LINK_AREA_EM3      11
#define FIN_HOST_LINK_AREA_EM4      12
#define FIN_HOST_LINK_AREA_EM5      13
#define FIN_HOST_LINK_AREA_EM6      14
#define FIN_HOST_LINK_AREA_EM7      15
#define FIN_HOST_LINK_AREA_EM8      16
#define FIN_HOST_LINK_AREA_EM9      17
#define FIN_HOST_LINK_AREA_EMA      18
#define FIN_HOST_LINK_AREA_EMB      19
#define FIN_HOST_LINK_AREA_EMC      20
#define FIN_HOST_LINK_AREA_DR       21
#define FIN_HOST_LINK_AREA_IR       22


#define FIN_HOST_LINK_AREA_CIO_BIT      23    //这一段必须俺顺序写，
#define FIN_HOST_LINK_AREA_W_BIT        24    //如果要在中间插入内存区，
#define FIN_HOST_LINK_AREA_H_BIT        25    //后面的宏要做相应增加。
#define FIN_HOST_LINK_AREA_A_BIT        26
#define FIN_HOST_LINK_AREA_DM_BIT       27
#define FIN_HOST_LINK_AREA_T_BIT        28
#define FIN_HOST_LINK_AREA_C_BIT        29
#define FIN_HOST_LINK_AREA_EM0_BIT      30
#define FIN_HOST_LINK_AREA_EM1_BIT      31
#define FIN_HOST_LINK_AREA_EM2_BIT      32
#define FIN_HOST_LINK_AREA_EM3_BIT      33
#define FIN_HOST_LINK_AREA_EM4_BIT      34
#define FIN_HOST_LINK_AREA_EM5_BIT      35
#define FIN_HOST_LINK_AREA_EM6_BIT      36
#define FIN_HOST_LINK_AREA_EM7_BIT      37
#define FIN_HOST_LINK_AREA_EM8_BIT      38
#define FIN_HOST_LINK_AREA_EM9_BIT      39
#define FIN_HOST_LINK_AREA_EMA_BIT      40
#define FIN_HOST_LINK_AREA_EMB_BIT      41
#define FIN_HOST_LINK_AREA_EMC_BIT      42
*/

enum
{
    FIN_HOST_LINK_AREA_CIO = 1,
    FIN_HOST_LINK_AREA_W,
    FIN_HOST_LINK_AREA_H,
    FIN_HOST_LINK_AREA_A,
    FIN_HOST_LINK_AREA_DM,
    FIN_HOST_LINK_AREA_T,
    FIN_HOST_LINK_AREA_C,
    FIN_HOST_LINK_AREA_EM0,
    FIN_HOST_LINK_AREA_EM1,
    FIN_HOST_LINK_AREA_EM2,
    FIN_HOST_LINK_AREA_EM3,
    FIN_HOST_LINK_AREA_EM4,
    FIN_HOST_LINK_AREA_EM5,
    FIN_HOST_LINK_AREA_EM6,
    FIN_HOST_LINK_AREA_EM7,
    FIN_HOST_LINK_AREA_EM8,
    FIN_HOST_LINK_AREA_EM9,
    FIN_HOST_LINK_AREA_EMA,
    FIN_HOST_LINK_AREA_EMB,
    FIN_HOST_LINK_AREA_EMC,
    FIN_HOST_LINK_AREA_DR,
    FIN_HOST_LINK_AREA_IR,

    FIN_HOST_LINK_AREA_CIO_BIT,
    FIN_HOST_LINK_AREA_W_BIT,
    FIN_HOST_LINK_AREA_H_BIT,
    FIN_HOST_LINK_AREA_A_BIT,
    FIN_HOST_LINK_AREA_DM_BIT,
    FIN_HOST_LINK_AREA_T_BIT,
    FIN_HOST_LINK_AREA_C_BIT,
    FIN_HOST_LINK_AREA_EM0_BIT,
    FIN_HOST_LINK_AREA_EM1_BIT,
    FIN_HOST_LINK_AREA_EM2_BIT,
    FIN_HOST_LINK_AREA_EM3_BIT,
    FIN_HOST_LINK_AREA_EM4_BIT,
    FIN_HOST_LINK_AREA_EM5_BIT,
    FIN_HOST_LINK_AREA_EM6_BIT,
    FIN_HOST_LINK_AREA_EM7_BIT,
    FIN_HOST_LINK_AREA_EM8_BIT,
    FIN_HOST_LINK_AREA_EM9_BIT,
    FIN_HOST_LINK_AREA_EMA_BIT,
    FIN_HOST_LINK_AREA_EMB_BIT,
    FIN_HOST_LINK_AREA_EMC_BIT
};

#define FIN_HOSTLINK_NO_ERR    0
#define FIN_HOSTLINK_TIMEOUT   1
#define FIN_HOSTLINK_PARM_ERR  2
//#define MODBUS_CRC_ERR   3
//#define MODBUS_RSP_ERR   4

#define RS485_RECEIVE  0x00			// 把总线置于接收状态
#define RS485_SEND     0x01			// 把总线置于发送状态(在发送状态，依然可以接收)
//
#pragma pack(push,1)
struct  finHostlinkSendHeader
{
        INT8U  waitTime;
        INT8U  ICF[2];
        INT8U  RSV[2];
        INT8U  GCT[2];
        INT8U  DNA[2];
        INT8U  DA1[2];
        INT8U  DA2[2];
        INT8U  SNA[2];
        INT8U  SA1[2];
        INT8U  SA2[2];
        INT8U  SID[2];
};

struct finHostlinkRcvHeader
{
        INT8U  ZERO[2];
        INT8U  ICF[2];
        INT8U  RSV[2];
        INT8U  GCT[2];
        INT8U  DNA[2];
        INT8U  DA1[2];
        INT8U  DA2[2];
        INT8U  SNA[2];
        INT8U  SA1[2];
        INT8U  SA2[2];
        INT8U  SID[2];
};

struct finHostlinkSendCommand
{
        INT8U  command[4];
        INT8U  area[2];  //1
        INT8U  wordAddr[4];
        INT8U  bitAddr[2];
        INT8U  count[4];
};

struct finHostlinkRcvCommand
{
        INT8U  command[4];
        INT8U  endCode[4];
};

struct finHostlinkDataPacket
{
        INT8U  start;
        INT8U  slave[2];  //1
        INT8U  frameType[2];
        INT8U  data[sizeof( struct finHostlinkSendHeader ) + sizeof( struct finHostlinkSendCommand ) + FIN_M_HOSTLINK_MAX + 4 ];//5  256(data)+22(head) +4(fcs * CR)
};
#pragma pack(pop)

struct finHostLinkNewPackLenPar
{
    INT16U packLen;
    INT16U maxToRcv;
};

// MbusMsg 函数传递参数结构体定义
struct finHlinkMsgPar
{
    INT8U  sfd;
    INT16U serialNum;
    INT8U  slave;       //Slave address (0 - 247)
    INT8U  rw;          //Read = 0, Write = 1
    INT8U  area;        //
    INT16U wordAddr;    //wrod addr (ie 0001) 0x0000-0xffff
    INT8U  bitAddr;     //bit addr (ie 01) 0x01 - 0x0f
    INT16U count;       //Number of elements (1- 120 words or 1 to 1920 bits)
    INT8U  data[FIN_M_HOSTLINK_MAX];   //Pointer to data (ie &VB100)
    INT8U  err;         //Error code (0 = no error)
};

#endif //MODBUS_DRIVER_H
