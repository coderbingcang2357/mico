#ifndef MIPROTOCOL4_DRIVER_H
#define MIPROTOCOL4_DRIVER_H

#include "../datatypedef.h"

#ifdef __KERNEL__
extern void ct_assert( INT8U cond, char* file, int line );
#define CT_ASSERT(cond)       ct_assert((int)cond, __FILE__, __LINE__);
#endif

#define M_MIPROTOCOL4_RETRIES      2            // Number of retries requested

#define M_MIPROTOCOL4_TYPE_ONE     0            // FX0N FX1S
#define M_MIPROTOCOL4_TYPE_TWO     1            // FX FX2C FX1N FX2N FX2NC

#define M_MIPROTOCOL4_ONE_BR_MAX   46           // FX0N FX1S BR MAX Point (bit)  (X,Y,M only) standard protocol 54
#define M_MIPROTOCOL4_ONE_WR_MAX   11           // FX0N FX1S WR MAX Word  (word) (D,T,C only) standard protocol 13
#define M_MIPROTOCOL4_ONE_BW_MAX   46           // FX0N FX1S BW MAX Point (bit)  (X,Y,M only)
#define M_MIPROTOCOL4_ONE_WW_MAX   11           // FXON FX1S WW MAX Word  (word) (D,T,C only)

#define M_MIPROTOCOL4_TWO_BR_MAX   120          // FX FX2C FX1N FX2N FX2NC BR MAX Point (bit)  (X,Y,M only) standard protocol 256
#define M_MIPROTOCOL4_TWO_WR_MAX   58           // FX FX2C FX1N FX2N FX2NC WR MAX Word  (word) (D,T,C only)standard protocol 64
#define M_MIPROTOCOL4_TWO_BW_MAX   120          // FX FX2C FX1N FX2N FX2NC BW MAX Point (bit)  (X,Y,M only)standard protocol 160
#define M_MIPROTOCOL4_TWO_WW_MAX   58           // FX FX2C FX1N FX2N FX2NC WW MAX Word  (word) (D,T,C only)standard protocol 64

#define M_READ   0      //MODBUS 读
#define M_WRITE  1      //MODBUS 写

#define MIPROTOCOL4_AREA_M       0
#define MIPROTOCOL4_AREA_X       1
#define MIPROTOCOL4_AREA_Y       2
#define MIPROTOCOL4_AREA_D       3
#define MIPROTOCOL4_AREA_T       4
#define MIPROTOCOL4_AREA_C       5
#define MIPROTOCOL4_AREA_C200    6
#define MIPROTOCOL4_AREA_S       7

#define MIPROTOCOL4_NO_ERR    0
#define MIPROTOCOL4_TIMEOUT   1
#define MIPROTOCOL4_PARM_ERR  2
//#define MODBUS_CRC_ERR   3
//#define MODBUS_RSP_ERR   4

#define RS485_RECEIVE  0x00			// 把总线置于接收状态
#define RS485_SEND     0x01			// 把总线置于发送状态(在发送状态，依然可以接收)
//

#pragma pack(push,1)
struct  MiProtocol4DataPacket
{
        INT8U  start;
        INT8U  slave[2];
        INT8U  plcNo[2];
        INT8U  data[256];//5
};
#pragma pack(pop)

struct miProtocol4NewPackLenPar
{
    INT8U packLen;
    INT8U maxToRcv;
};

// MbusMsg 函数传递参数结构体定义
struct MiProtocol4MsgPar
{
    INT8U  sfd;
    INT16U serialNum;
    INT8U  slave;    //Slave address (0 - 247)
    INT8U  PlcNo;    //Plc type(0 FX0N FX1S, 1 FX FX2C FX1N FX2N FX2NC )；
    INT8U  rw;       //Read = 0, Write = 1
    INT8U  area;     //0x, 1x, 2x, 3x, 4x
    INT16U addr;     //addr (ie 00001)
    INT16U count;    //Number of elements (1- 120 words or 1 to 1920 bits)
    INT8U  data[256];//Pointer to data (ie &VB100)
    INT8U  checkSum; //add checkSum or Not
    INT8U  err;      //Error code (0 = no error)
};

#endif //MODBUS_DRIVER_H
