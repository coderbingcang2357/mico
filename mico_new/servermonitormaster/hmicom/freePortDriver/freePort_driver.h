#ifndef FP_DRIVER_H
#define FP_DRIVER_H

//#include "datatypedef.h"

struct  FpDataPacket
{  
    INT8U  data[256];//5
};

// MbusMsg 函数传递参数结构体定义
struct FpMsgPar
{
	INT8U sfd;//socket fd
	INT16U serialNum;//the frame serial number	
	INT16U count;   //Number of elements (1- 120 words or 1 to 1920 bits)
	INT8U data[256];//Pointer to data (ie &VB100)
	INT8U err;      //Error code (0 = no error)
};

#endif //MODBUS_DRIVER_H
