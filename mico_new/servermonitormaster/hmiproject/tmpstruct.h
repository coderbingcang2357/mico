#ifndef TMP_STRUCT_H
#define TMP_STRUCT_H
//将X的第Y位置1
#define SETBIT(X,Y) ((X) |= 1<<(Y))

//将X的第Y位清0
#define CLEARBIT(X,Y) ((X) &= ~(1<<(Y)))

//判断X的第Y位是否为1
#define TESTBIT(X,Y) ((X) >> (Y)&1)

//将X的第Y位取反
#define TOGGLEBIT(X,Y) ((X) ^= 1<<(Y))

enum ALARMSTATE{
	PRODUCE = 0, // 报警产生
	RELEASE , // 报警解除	
	INVALID ,
};
enum ALARMSTYLE{
	ANAGOLY = 0, // 模拟量
	DISCRETE     // 离散量
};
const double EPS = 1e-6;

inline bool SmallerAndEqualOfDouble(double d1,double d2){
	if(d2-d1 > EPS || fabs(d2-d1) < EPS){
		return true;
	}	
	else{
		return false;
	}
}

inline bool SmallerOfDouble(double d1,double d2){
	if(d2-d1 > EPS){
		return true;
	}	
	else{
		return false;
	}
}

//变量线性转换信息结构体
struct LinearScaling
{
    double upperPLCValue;
    double lowerPLCValue;
    double upperHMIValue;
    double lowerHMIValue;
};

//变量读写返回错误的定义
#define VAR_SYS_ERR          -1  //系统逻辑错误，不应该出现的
#define VAR_NO_ERR            0  //没有错误,完成
#define VAR_COMM_TIMEOUT      1  //超时，没有连接
#define VAR_PARAM_ERROR       2  //参数错误 >= 2 && < 255, 由从站返回的其它错误
#define VAR_DISCONNECT        3  // the connect of this var was disconnect
#define VAR_NO_OPERATION    225  //还没有获取第一次数据，未操作
#include "hmicom/commInterface.h"
//struct ComMsg
//{
//    int varId;
//    char type;
//    char err;
//};

#define CONNECT_NO_OPERATION    0 //连接未操作
#define CONNECT_NO_LINK         1 //没有连接
#define CONNECT_LINKED_OK       2 //连接成功
#define CONNECT_LINKING			3
#define CONNECT_LINKED_READY    4

enum TagDataType
{
    NoType = 0,
    Char = 1,
    Byte = 2,/* Omron中的byte类型;*/
    Int = 3,/*MODBUS的"+/-Int"类型;----Omron中的"+/-DEC"; ----Mitsubishi中的"Int"类型*/
    UInt = 4,/*MODBUS的"Int"和"16 Bit Group";----Omron的"DEC"; ----Mitsubishi的"Word"类型*/
    Word = 4,
    Long = 5,/*MODBUS的"+/-Double";----Omron的"+/-LDEC"; ----Mitsubishi的"DInt"类型*/
    DInt = 5,
    ULong = 6,/*MODBUS的"Double";----Omron的"LDEC"; ----Mitsubishi的"DWord"类型*/
    DWord = 6,
    Float = 7,
    Real = 7,/*MODBUS的"Float";----Omron的"IEEE"; ----Mitsubishi的"Float"类型*/
    Double = 8,
    Bool = 9,/*MODBUS的"Bit";----Omron的BIN; ----Mitsubishi的"Bit"类型*/
    String = 10,/*MODBUS的"ASCII";----Omron的"ASCII"; ----Mitsubishi的"String"类型*/
    StringChar = 10,
    DateTime = 11,	/*内部变量的DataTime和S7300的DateTime类型*/
    Timer = 12,
    BitGroup = 13, /*MODBUS的"16 Bit Group"*/
    BitBlock4 = 14, /*Mitsubishi的"4 bit" 类型, 占用一个字节,实际为Byte类型*/
    BitBlock8 = 15, /*Mitsubishi的"8 bit" 类型, 占用一个字节,实际为Byte类型*/
    BitBlock12 = 16, /*Mitsubishi的"12 bit" 类型, 占用两个字节,实际为UInt类型*/
    BitBlock16 = 17, /*Mitsubishi的"16 bit" 类型, 占用两个字节,实际为UInt类型*/
    BitBlock20 = 18, /*Mitsubishi的"20 bit" 类型, 占用三个字节,实际为ULong类型*/
    BitBlock24 = 19, /*Mitsubishi的"24 bit" 类型, 占用三个字节,实际为ULong类型*/
    BitBlock28 = 20, /*Mitsubishi的"28 bit" 类型, 占用三个字节,实际为ULong类型*/
    BitBlock32 = 21, /*Mitsubishi的"32 bit" 类型, 占用三个字节,实际为ULong类型*/
    PointArea = 22,  /*Mitsubishi added. modbus ppi/mpi hostlink use StringChar. New protocol must use this type not "StringChar"*/

    S7300_String    = 23, /* 300系列 变长字符串 第一个字节为总长度 第二个字节为字符串实际长度  // 2013-03-18 LIUDIKA*/
    S7300_Timer     = 24, /* 300系列  2个字节 特殊,作为ULong 4字节处理// 2013-03-18 LIUDIKA*/
    S7300_Counter   = 25, /* 300系列  2个字节 UInt处理// 2013-03-18 LIUDIKA*/
    S7300_Date      = 26, /* 300系列  2个字节// 2013-03-18 LIUDIKA*/
    S7300_Time      = 27, /* 300系列  4个字节, 当作整型数long处理// 2013-03-18 LIUDIKA*/
//    S7300_DateTime  = 28, /* 300系列  8个字节// 2013-03-18 LIUDIKA // 似乎跟内部变量的DateTime一样，未验证*/
    S7300_TimeofDay = 28, /* 300系列  4个字节// 2013-03-18 LIUDIKA*/
    BRIGHTEK_String = 29,
    OPCItem = 30,
    KEYENCE_ASCII = 31,

    Int21 = 32,
    UInt21 = 33,
    Long4321 = 34,
    Long2143 = 35,
    Long3412 = 36,
    ULong4321 = 37,
    ULong2143 = 38,
    ULong3412 = 39,
    Real4321 = 40,
    Real2143 = 41,
    Real3412 = 42
 };
#endif
