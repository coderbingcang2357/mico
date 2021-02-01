#ifndef COMM_INTERFACE_H
#define COMM_INTERFACE_H

#include <QtGui>

//#include "datatypedef.h"
#include "ct_def.h"

#include "./modbus/modbusdriver.h"
//#include "./driver/mpi_40/ppi_l7wr.h"

#include "./hostlinkDriver/hostlinkdriver.h"
#include "./finHostlinkDriver/finhostlinkdriver.h"
#include "./MiProtocol4Driver/miprotocol4driver.h" //更新至HMI2018-04-26 这几行只是换了位置，其实没有改
#include "./miprofxDriver/miprofxdriver.h"
#include "./mewtocolDriver/mewtocoldriver.h"
#include "./dvpDriver/dvpdriver.h"
#include "./fatekDriver/fatekdriver.h"
#include "./cnetDriver/cnetdriver.h"
#include "./ppi/ppi_l7wr.h"
#include "gateWay/udpL1L2.h"
#include "tcpModbus/tcpmodbus.h"
#include "hmiproject/util2/datadecode.h"


//变量读写返回错误的定义
#define VAR_SYS_ERR          -1  //系统逻辑错误，不应该出现的
#define VAR_NO_ERR            0  //没有错误,完成
#define VAR_COMM_TIMEOUT      1  //超时，没有连接
#define VAR_PARAM_ERROR       2  //参数错误 >= 2 && < 255, 由从站返回的其它错误
#define VAR_DISCONNECT        3  // the connect of this var was disconnect
#define VAR_NO_OPERATION    225  //还没有获取第一次数据，未操作

typedef enum
{
	Com_NoType = 0,
	Com_Char = 1,
	Com_Byte = 2,
	Com_Int = 3,//MODBUS的"+/-Int"类型
	Com_UInt = 4,//MODBUS的"Int"和"16 Bit Group"
	Com_Word = 4,
	Com_Long = 5,//MODBUS的"+/-Double"
	Com_DInt = 5,
	Com_ULong = 6,//MODBUS的"Double"
	Com_DWord = 6,
	Com_Float = 7,
	Com_Real = 7,//MODBUS的"Float"
	Com_Double = 8,
	Com_Bool = 9,//MODBUS的"Bit"
	Com_String = 10,//MODBUS的"ASCII"
	Com_StringChar = 10,
	Com_DateTime = 11,
	Com_Timer = 12,
	Com_BitGroup = 13,//MPIPPI没有*//*MODBUS的"16 Bit Group"
	Com_BitBlock4 = 14,
	Com_BitBlock8 = 15,
	Com_BitBlock12 = 16,
	Com_BitBlock16 = 17,
	Com_BitBlock20 = 18,
	Com_BitBlock24 = 19,
	Com_BitBlock28 = 20,
	Com_BitBlock32 = 21,
	Com_PointArea = 22,

	Com_S7300_String    = 23, /* 300系列 变长字符串 第一个字节为总长度 第二个字节为字符串实际长度  // 2013-03-18 LIUDIKA*/
    Com_S7300_Timer     = 24, /* 300系列  2个字节// 2013-03-18 LIUDIKA*/
    Com_S7300_Counter   = 25, /* 300系列  2个字节// 2013-03-18 LIUDIKA*/
    Com_S7300_Date      = 26, /* 300系列  2个字节// 2013-03-18 LIUDIKA*/
    Com_S7300_Time      = 27, /* 300系列  4个字节, 当作整型数long处理// 2013-03-18 LIUDIKA*/
//   Com_ S7300_DateTime  = 28, /* 300系列  8个字节// 2013-03-18 LIUDIKA // 似乎跟内部变量的DateTime一样，未验证*/
    Com_S7300_TimeofDay = 28, /* 300系列  4个字节// 2013-03-18 LIUDIKA*/
    Com_BRIGHTEK_String = 29,
    Com_OPCItem         = 30,
    Com_KEYENCE_ASCII   = 31,

    Com_Int21           = 32,
    Com_UInt21          = 33,
    Com_Long4321        = 34,
    Com_Long2143        = 35,
    Com_Long3412        = 36,
    Com_ULong4321       = 37,
    Com_ULong2143       = 38,
    Com_ULong3412       = 39,
    Com_Real4321        = 40,
    Com_Real2143        = 41,
    Com_Real3412        = 42
} ComDataType;

////MODBUS连接属性
//struct ModbusConnectCfg
//{
//	int id;                 //连接ID
//    INT8U COM_interface;	//接口或端口号
//	INT32U baud;			//波特率
//	INT8U hmiAddr;		    //HMI地址,可以不要
//	INT8U cpuAddr;          //CPU地址
//	INT8U parity;		    //奇偶校验
//	INT8U dataBit;		    //数据位
//	INT8U stopBit; 		    //停止位
//};
//
////PPI/MPI连接属性
//struct PpiMpiConnectCfg
//{
//	int id;                 //连接ID
//    INT8U COM_interface;    //接口或端口号
//	INT32U baud;            //波特率
//	INT8U hsa;              //最高站地址
//	INT8U hmiAddr;		    //HMI地址
//	INT8U cpuAddr;          //CPU地址
//	INT8U masterNum;		//最多主站个数
//	bool isMpi;             //是否MPI协议, true表示MPI, false表示为PPI
//};
//
////HOSTLINK连接属性
//struct HostlinkConnectCfg
//{
//	int id;                 //连接ID
//    INT8U COM_interface;	//接口或端口号
//	INT32U baud;			//波特率
//	INT8U hmiAddr;		    //HMI地址,可以不要
//	INT8U cpuAddr;          //CPU地址
//	INT8U parity;		    //奇偶校验
//	INT8U dataBit;		    //数据位
//	INT8U stopBit; 		    //停止位
//};
//
////FINHOSTLINK连接属性
//struct FinHostlinkConnectCfg
//{
//	int id;                 //连接ID
//    INT8U COM_interface;	//接口或端口号
//	INT32U baud;			//波特率
//	INT8U hmiAddr;		    //HMI地址,可以不要
//	INT8U cpuAddr;          //CPU地址
//	INT8U parity;		    //奇偶校验
//	INT8U dataBit;		    //数据位
//	INT8U stopBit; 		    //停止位
//};
//
//struct MiProtocol4ConnectCfg
//{
//	int id;                 //连接ID
//    INT8U COM_interface;	//接口或端口号
//	INT32U baud;			//波特率
//	INT8U hmiAddr;		    //HMI地址,可以不要
//	INT8U cpuAddr;          //CPU地址
//	INT8U parity;		    //奇偶校验
//	INT8U dataBit;		    //数据位
//	INT8U stopBit; 		    //停止位
//	INT8U checkSum;
//	INT8U PlcNo;
//};
//
//struct MiProfxConnectCfg
//{
//	int id;                 //连接ID
//    INT8U COM_interface;	//接口或端口号
//	INT32U baud;			//波特率
//	INT8U hmiAddr;		    //HMI地址,可以不要固定为0
//	INT8U cpuAddr;          //CPU地址,可以不要固定为1
//	INT8U parity;		    //奇偶校验
//	INT8U dataBit;		    //数据位
//	INT8U stopBit; 		    //停止位
//	INT8U checkSum;         //固定为1，可以不要
//	INT8U PlcNo;            //固定为2，可以不要
//};
//struct MewtocolConnectCfg
//{
//	int id;                 //连接ID
//    INT8U COM_interface;	//接口或端口号
//	INT32U baud;			//波特率
//	INT8U hmiAddr;		    //HMI地址,可以不要
//	INT8U cpuAddr;          //CPU地址
//	INT8U parity;		    //奇偶校验
//	INT8U dataBit;		    //数据位
//	INT8U stopBit; 		    //停止位
//};
//struct DvpConnectCfg
//{
//	int id;                 //连接ID
//    INT8U COM_interface;	//接口或端口号
//	INT32U baud;			//波特率
//	INT8U hmiAddr;		    //HMI地址,可以不要
//	INT8U cpuAddr;          //CPU地址
//	INT8U parity;		    //奇偶校验
//	INT8U dataBit;		    //数据位
//	INT8U stopBit; 		    //停止位
//};
//struct FatekConnectCfg
//{
//	int id;                 //连接ID
//    INT8U COM_interface;	//接口或端口号
//	INT32U baud;			//波特率
//	INT8U hmiAddr;		    //HMI地址,可以不要
//	INT8U cpuAddr;          //CPU地址
//	INT8U parity;		    //奇偶校验
//	INT8U dataBit;		    //数据位
//	INT8U stopBit; 		    //停止位
//};
//struct CnetConnectCfg
//{
//	int id;                 //连接ID
//    INT8U COM_interface;	//接口或端口号
//	INT32U baud;			//波特率
//	INT8U hmiAddr;		    //HMI地址,可以不要
//	INT8U cpuAddr;          //CPU地址
//	INT8U parity;		    //奇偶校验
//	INT8U dataBit;		    //数据位
//	INT8U stopBit; 		    //停止位
//};
////MPI300连接属性
//struct Mpi300ConnectCfg
//{
//	int id;                 //连接ID
//    INT8U COM_interface;    //接口或端口号
//	INT32U baud;            //波特率
//	INT8U hsa;              //最高站地址
//	INT8U hmiAddr;		//HMI地址
//	INT8U cpuAddr;          //CPU地址
//	INT8U masterNum;	//最多主站个数
//	INT8U isMpi;            //暂时只有MPI一种，以后可以根据握手方式的不同会用到
//};
////自由口协议
//struct fpConnectCfg
//{
//	int id;                 //连接ID
//    INT8U COM_interface;		//鎺ュ彛鎴栫鍙ｅ彿
//	INT32U baud;			//波特率
//
//	INT8U parity;		    //奇偶校验
//	INT8U dataBit;		    //数据位
//	INT8U stopBit; 		    //停止位
//};
////TCP MODBUS连接属性
//struct TcpModbusConnectCfg
//{
//	int id;                 //连接ID
//	QString IP;		//ip address
//	INT16U port;			//port number
//	INT8U unitID;		    //slave unit ID
//};
////合信 网关连接属性 //2013.09.09 taojianjun 在线模拟增加对合信网关的支持
//struct GateWayConnectCfg
//{
//	int id;                 //连接ID
//	QString IPAddr;        //网关IP地址
//	INT32U rPort;            //远程网关端口
//	INT8U cpuAddr;          //CPU地址
//};
//HMI外部变量变量属性
struct HmiVar
{
	int varId;		        //变量的唯一标识码
	int connectId;          //对应那个连接
	int cycleTime;          //变量的刷新周期，单位毫秒 (最大周期值24小时)
	int area;               //变量所在的区域
	int subArea;            //子区域 300 协议 DB 用到
	long addOffset;          //数据在PLC中的地址偏移量
	int bitOffset;          //如果需要位偏移量，那么这个位变量的偏移量，否则写-1
	int  len;               //变量的数据长度，字节个数, 位变量为位的个数
	INT8U dataType;         //变量的类型

	//调用接口只需要填以上的属性,下面的是内部使用属性
	int lastUpdateTime;     //最后更新时间
	QByteArray rData;       //要读的数据
	QByteArray wData;       //要写的数据
	INT8U wStatus;          //写数据的状态
	INT8U err;              //读写结果
	bool overFlow;          //表示该变量是否越界
	QList<struct HmiVar*> splitVars;//拆分的子变量,用于超长变量
    int writeCnt;
};

void DelHmiVar(struct HmiVar *hmival);

struct ComMsg
{
    int varId;
    char type;
    char err;
};
class HMICommService;
class ProtocolPars {
public:
    virtual ~ProtocolPars() {}
    virtual bool fromData(char **buf, int *len) = 0;
    virtual void registerConfig(HMICommService *hc) = 0;
};

//MODBUS连接属性
struct ModbusConnectCfg : public ProtocolPars
{
    quint16 id;                 //连接ID
    INT8U COM_interface;	//接口或端口号
    INT32U baud;			//波特率
    INT8U hmiAddr;		    //HMI地址,可以不要
    INT8U cpuAddr;          //CPU地址
    INT8U parity;		    //奇偶校验
    INT8U dataBit;		    //数据位
    INT8U stopBit; 		    //停止位
    virtual bool fromData(char **buf, int *len) {
        if ( !Util2::readUint16(buf, len, (quint16 *)&id)
                || !Util2::readUint8(buf, len, (quint8 *)&COM_interface)
                || !Util2::readUint32(buf, len, (quint32 *)&baud)
                || !Util2::readUint8(buf, len, (quint8 *)&hmiAddr)
                || !Util2::readUint8(buf, len, (quint8 *)&cpuAddr)
                || !Util2::readUint8(buf, len, (quint8 *)&parity)
                || !Util2::readUint8(buf, len, (quint8 *)&dataBit)
                || !Util2::readUint8(buf, len, (quint8 *)&stopBit)
            )
        {
            return false;
        }
        return true;
    }

    virtual void registerConfig(HMICommService *hc);
};

//PPI/MPI连接属性
struct PpiMpiConnectCfg : public ProtocolPars
{
    quint16 id;                 //连接ID
    INT8U COM_interface;    //接口或端口号
    INT32U baud;            //波特率
    INT8U hsa;              //最高站地址
    INT8U hmiAddr;		    //HMI地址
    INT8U cpuAddr;          //CPU地址
    INT8U masterNum;		//最多主站个数
    bool isMpi;             //是否MPI协议, true表示MPI, false表示为PPI
    virtual bool fromData(char **buf, int *len) {
        quint8 isMpi8 = 0;
        if ( !Util2::readUint16(buf, len, (quint16 *)&id)
                || !Util2::readUint8(buf, len, (quint8 *)&COM_interface)
                || !Util2::readUint32(buf, len, (quint32 *)&baud)
                || !Util2::readUint8(buf, len, (quint8 *)&hsa)
                || !Util2::readUint8(buf, len, (quint8 *)&hmiAddr)
                || !Util2::readUint8(buf, len, (quint8 *)&cpuAddr)
                || !Util2::readUint8(buf, len, (quint8 *)&masterNum)
                || !Util2::readUint8(buf, len, (quint8 *)&isMpi8)
            )
        {
            return false;
        }
        isMpi = isMpi8 != 0;
		printf("ppimpi iddddddd:%u\n",id);
		//printf("baud:%u", baud);
        return true;
    }
    virtual void registerConfig(HMICommService *hc);
};

//HOSTLINK连接属性
struct HostlinkConnectCfg : public ProtocolPars
{
    quint16 id;                 //连接ID
    INT8U COM_interface;	//接口或端口号
    INT32U baud;			//波特率
    INT8U hmiAddr;		    //HMI地址,可以不要
    INT8U cpuAddr;          //CPU地址
    INT8U parity;		    //奇偶校验
    INT8U dataBit;		    //数据位
    INT8U stopBit; 		    //停止位
    virtual bool fromData(char **buf, int *len){
        if ( !Util2::readUint16(buf, len, (quint16 *)&id)
                || !Util2::readUint8(buf, len, (quint8 *)&COM_interface)
                || !Util2::readUint32(buf, len, (quint32 *)&baud)
                || !Util2::readUint8(buf, len, (quint8 *)&hmiAddr)
                || !Util2::readUint8(buf, len, (quint8 *)&cpuAddr)
                || !Util2::readUint8(buf, len, (quint8 *)&parity)
                || !Util2::readUint8(buf, len, (quint8 *)&dataBit)
                || !Util2::readUint8(buf, len, (quint8 *)&stopBit)
            )
        {
            return false;
        }
        return true;
    }
    virtual void registerConfig(HMICommService *hc);
};

//FINHOSTLINK连接属性
struct FinHostlinkConnectCfg : public ProtocolPars
{
    quint16 id;                 //连接ID
    INT8U COM_interface;	//接口或端口号
    INT32U baud;			//波特率
    INT8U hmiAddr;		    //HMI地址,可以不要
    INT8U cpuAddr;          //CPU地址
    INT8U parity;		    //奇偶校验
    INT8U dataBit;		    //数据位
    INT8U stopBit; 		    //停止位
    virtual bool fromData(char **buf, int *len){
        if ( !Util2::readUint16(buf, len, (quint16 *)&id)
                || !Util2::readUint8(buf, len, (quint8 *)&COM_interface)
                || !Util2::readUint32(buf, len, (quint32 *)&baud)
                || !Util2::readUint8(buf, len, (quint8 *)&hmiAddr)
                || !Util2::readUint8(buf, len, (quint8 *)&cpuAddr)
                || !Util2::readUint8(buf, len, (quint8 *)&parity)
                || !Util2::readUint8(buf, len, (quint8 *)&dataBit)
                || !Util2::readUint8(buf, len, (quint8 *)&stopBit)
            )
        {
            return false;
        }
        return true;
    }
    virtual void registerConfig(HMICommService *hc);
};

struct MiProtocol4ConnectCfg : public ProtocolPars
{
    quint16 id;                 //连接ID
    INT8U COM_interface;	//接口或端口号
    INT32U baud;			//波特率
    INT8U hmiAddr;		    //HMI地址,可以不要
    INT8U cpuAddr;          //CPU地址
    INT8U parity;		    //奇偶校验
    INT8U dataBit;		    //数据位
    INT8U stopBit; 		    //停止位
    INT8U checkSum;
    INT8U PlcNo;
    virtual bool fromData(char **buf, int *len){
        if ( !Util2::readUint16(buf, len, (quint16 *)&id)
                || !Util2::readUint8(buf, len, (quint8 *)&COM_interface)
                || !Util2::readUint32(buf, len, (quint32 *)&baud)
                || !Util2::readUint8(buf, len, (quint8 *)&hmiAddr)
                || !Util2::readUint8(buf, len, (quint8 *)&cpuAddr)
                || !Util2::readUint8(buf, len, (quint8 *)&parity)
                || !Util2::readUint8(buf, len, (quint8 *)&dataBit)
                || !Util2::readUint8(buf, len, (quint8 *)&stopBit)
                || !Util2::readUint8(buf, len, (quint8 *)&checkSum)
                || !Util2::readUint8(buf, len, (quint8 *)&PlcNo)
            )
        {
            return false;
        }
        return true;
    }
    virtual void registerConfig(HMICommService *hc);
};

struct MiProfxConnectCfg : public ProtocolPars
{
    quint16 id;                 //连接ID
    INT8U COM_interface;	//接口或端口号
    INT32U baud;			//波特率
    INT8U hmiAddr;		    //HMI地址,可以不要固定为0
    INT8U cpuAddr;          //CPU地址,可以不要固定为1
    INT8U parity;		    //奇偶校验
    INT8U dataBit;		    //数据位
    INT8U stopBit; 		    //停止位
    INT8U checkSum;         //固定为1，可以不要
    INT8U PlcNo;            //固定为2，可以不要
    virtual bool fromData(char **buf, int *len){
        if ( !Util2::readUint16(buf, len, (quint16 *)&id)
                || !Util2::readUint8(buf, len, (quint8 *)&COM_interface)
                || !Util2::readUint32(buf, len, (quint32 *)&baud)
                || !Util2::readUint8(buf, len, (quint8 *)&hmiAddr)
                || !Util2::readUint8(buf, len, (quint8 *)&cpuAddr)
                || !Util2::readUint8(buf, len, (quint8 *)&parity)
                || !Util2::readUint8(buf, len, (quint8 *)&dataBit)
                || !Util2::readUint8(buf, len, (quint8 *)&stopBit)
                || !Util2::readUint8(buf, len, (quint8 *)&checkSum)
                || !Util2::readUint8(buf, len, (quint8 *)&PlcNo)
            )
        {
            return false;
        }
        return true;
    }
    virtual void registerConfig(HMICommService *hc);
};
struct MewtocolConnectCfg : public ProtocolPars
{
    quint16 id;                 //连接ID
    INT8U COM_interface;	//接口或端口号
    INT32U baud;			//波特率
    INT8U hmiAddr;		    //HMI地址,可以不要
    INT8U cpuAddr;          //CPU地址
    INT8U parity;		    //奇偶校验
    INT8U dataBit;		    //数据位
    INT8U stopBit; 		    //停止位
    virtual bool fromData(char **buf, int *len){
        if ( !Util2::readUint16(buf, len, (quint16 *)&id)
                || !Util2::readUint8(buf, len, (quint8 *)&COM_interface)
                || !Util2::readUint32(buf, len, (quint32 *)&baud)
                || !Util2::readUint8(buf, len, (quint8 *)&hmiAddr)
                || !Util2::readUint8(buf, len, (quint8 *)&cpuAddr)
                || !Util2::readUint8(buf, len, (quint8 *)&parity)
                || !Util2::readUint8(buf, len, (quint8 *)&dataBit)
                || !Util2::readUint8(buf, len, (quint8 *)&stopBit)
            )
        {
            return false;
        }
        return true;
    }
    virtual void registerConfig(HMICommService *hc);
};
struct DvpConnectCfg : public ProtocolPars
{
    quint16 id;                 //连接ID
    INT8U COM_interface;	//接口或端口号
    INT32U baud;			//波特率
    INT8U hmiAddr;		    //HMI地址,可以不要
    INT8U cpuAddr;          //CPU地址
    INT8U parity;		    //奇偶校验
    INT8U dataBit;		    //数据位
    INT8U stopBit; 		    //停止位
    virtual bool fromData(char **buf, int *len){
        if ( !Util2::readUint16(buf, len, (quint16 *)&id)
                || !Util2::readUint8(buf, len, (quint8 *)&COM_interface)
                || !Util2::readUint32(buf, len, (quint32 *)&baud)
                || !Util2::readUint8(buf, len, (quint8 *)&hmiAddr)
                || !Util2::readUint8(buf, len, (quint8 *)&cpuAddr)
                || !Util2::readUint8(buf, len, (quint8 *)&parity)
                || !Util2::readUint8(buf, len, (quint8 *)&dataBit)
                || !Util2::readUint8(buf, len, (quint8 *)&stopBit)
            )
        {
            return false;
        }
        return true;
    }
    virtual void registerConfig(HMICommService *hc);
};
struct FatekConnectCfg : public ProtocolPars
{
    quint16 id;                 //连接ID
    INT8U COM_interface;	//接口或端口号
    INT32U baud;			//波特率
    INT8U hmiAddr;		    //HMI地址,可以不要
    INT8U cpuAddr;          //CPU地址
    INT8U parity;		    //奇偶校验
    INT8U dataBit;		    //数据位
    INT8U stopBit; 		    //停止位
    virtual bool fromData(char **buf, int *len){
        if ( !Util2::readUint16(buf, len, (quint16 *)&id)
                || !Util2::readUint8(buf, len, (quint8 *)&COM_interface)
                || !Util2::readUint32(buf, len, (quint32 *)&baud)
                || !Util2::readUint8(buf, len, (quint8 *)&hmiAddr)
                || !Util2::readUint8(buf, len, (quint8 *)&cpuAddr)
                || !Util2::readUint8(buf, len, (quint8 *)&parity)
                || !Util2::readUint8(buf, len, (quint8 *)&dataBit)
                || !Util2::readUint8(buf, len, (quint8 *)&stopBit)
            )
        {
            return false;
        }
        return true;
    }
    virtual void registerConfig(HMICommService *hc);
};
struct CnetConnectCfg : public ProtocolPars
{
    quint16 id;                 //连接ID
    INT8U COM_interface;	//接口或端口号
    INT32U baud;			//波特率
    INT8U hmiAddr;		    //HMI地址,可以不要
    INT8U cpuAddr;          //CPU地址
    INT8U parity;		    //奇偶校验
    INT8U dataBit;		    //数据位
    INT8U stopBit; 		    //停止位
    virtual bool fromData(char **buf, int *len){
        if ( !Util2::readUint16(buf, len, (quint16 *)&id)
                || !Util2::readUint8(buf, len, (quint8 *)&COM_interface)
                || !Util2::readUint32(buf, len, (quint32 *)&baud)
                || !Util2::readUint8(buf, len, (quint8 *)&hmiAddr)
                || !Util2::readUint8(buf, len, (quint8 *)&cpuAddr)
                || !Util2::readUint8(buf, len, (quint8 *)&parity)
                || !Util2::readUint8(buf, len, (quint8 *)&dataBit)
                || !Util2::readUint8(buf, len, (quint8 *)&stopBit)
            )
        {
            return false;
        }
        return true;
    }
    virtual void registerConfig(HMICommService *hc);
};
//MPI300连接属性
struct Mpi300ConnectCfg : public ProtocolPars
{
    quint16 id;                 //连接ID
    INT8U COM_interface;    //接口或端口号
    INT32U baud;            //波特率
    INT8U hsa;              //最高站地址
    INT8U hmiAddr;		//HMI地址
    INT8U cpuAddr;          //CPU地址
    INT8U masterNum;	//最多主站个数
    INT8U isMpi;            //暂时只有MPI一种，以后可以根据握手方式的不同会用到
    virtual bool fromData(char **buf, int *len) {
        if (!Util2::readUint16(buf, len, (quint16 *)&id)
                || !Util2::readUint8(buf, len, (quint8 *)&COM_interface)
                || !Util2::readUint32(buf, len, (quint32 *)&baud)
                || !Util2::readUint8(buf, len, (quint8 *)&hsa)
                || !Util2::readUint8(buf, len, (quint8 *)&hmiAddr)
                || !Util2::readUint8(buf, len, (quint8 *)&cpuAddr)
                || !Util2::readUint8(buf, len, (quint8 *)&masterNum)
                || !Util2::readUint8(buf, len, (quint8 *)&isMpi)
            )
        {
            return false;
        }
        return true;
    }
    virtual void registerConfig(HMICommService *hc);
};
//自由口协议
struct fpConnectCfg : public ProtocolPars
{
    quint16 id;                 //连接ID
    INT8U COM_interface;		//鎺ュ彛鎴栫鍙ｅ彿
    INT32U baud;			//波特率

    INT8U parity;		    //奇偶校验
    INT8U dataBit;		    //数据位
    INT8U stopBit; 		    //停止位
    virtual bool fromData(char **buf, int *len){
        if (!Util2::readUint16(buf, len, (quint16 *)&id)
                || !Util2::readUint8(buf, len, (quint8 *)&COM_interface)
                || !Util2::readUint32(buf, len, (quint32 *)&baud)
                || !Util2::readUint8(buf, len, (quint8 *)&parity)
                || !Util2::readUint8(buf, len, (quint8 *)&dataBit)
                || !Util2::readUint8(buf, len, (quint8 *)&stopBit)
            )
        {
            return false;
        }
        return true;
    }
    virtual void registerConfig(HMICommService *hc);
};
//TCP MODBUS连接属性
struct TcpModbusConnectCfg : public ProtocolPars
{
    quint16 id;                 //连接ID
    QString IP;		//ip address
    INT16U port;			//port number
    INT8U unitID;		    //slave unit ID
    virtual bool fromData(char **buf, int *len){
        if (!Util2::readUint16(buf, len, (quint16 *)&id)
                || !Util2::readString(buf, len, &IP)
                || !Util2::readUint16(buf, len, (quint16 *)&port)
                || !Util2::readUint8(buf, len, (quint8 *)&unitID)
            )
        {
            return false;
        }
        return true;
    }
    virtual void registerConfig(HMICommService *hc);
};
//合信 网关连接属性 //2013.09.09 taojianjun 在线模拟增加对合信网关的支持
struct GateWayConnectCfg : public ProtocolPars
{
    quint16 id;                 //连接ID
    QString IPAddr;        //网关IP地址
    INT32U rPort;            //远程网关端口
    INT8U cpuAddr;          //CPU地址

    virtual bool fromData(char **buf, int *len) {
        if (!Util2::readUint16(buf, len, (quint16 *)&id)
                //|| !Util2::readString(buf, len, &IPAddr)
                || !Util2::readString_end0(buf, len, &IPAddr)
                || !Util2::readUint32(buf, len, (quint32 *)&rPort)
                || !Util2::readUint8(buf, len, (quint8 *)&cpuAddr)
            )
        {
            return false;
        }
		printf("gateway idddddd:%u\n",id);
        return true;
    }
    virtual void registerConfig(HMICommService *hc);
};

class CommService;
class HMICommService
{
public:
    int registerConnect( struct ModbusConnectCfg& connectCfg );//注册MODBUS连接
    int registerConnect( struct PpiMpiConnectCfg& connectCfg );//注册PPI/MPI连接
    int registerConnect( struct GateWayConnectCfg& connectCfg );//注册合信网关连接
    int registerConnect( struct TcpModbusConnectCfg& connectCfg );//TCP MODBUS连接 //更新至HMI2018-04-26 这几行只是换了位置，其实没有改
    int registerConnect( struct HostlinkConnectCfg& connectCfg );//注册HOSTLINK连接
    int registerConnect( struct FinHostlinkConnectCfg& connectCfg );//注册FINHOSTLINK连接
    int registerConnect( struct MiProtocol4ConnectCfg& connectCfg ); //注册MITSUBISHI COMPUTER CONTROL LINK 连接
    int registerConnect( struct MiProfxConnectCfg& connectCfg ); //注册MITSUBISHI 编程口连接
    int registerConnect( struct MewtocolConnectCfg& connectCfg );//注册松下 MEWTOCOL 编程口连接
    int registerConnect( struct DvpConnectCfg& connectCfg );//注册台达 DVP 编程口连接
    int registerConnect( struct FatekConnectCfg& connectCfg );//注册永宏 FATEK 编程口连接
    int registerConnect( struct CnetConnectCfg& connectCfg );//注册LS MasterK Cnet连接
    int registerConnect( struct Mpi300ConnectCfg& connectCfg );//MPI300连接
    int registerConnect( struct fpConnectCfg& connectCfg );//freeport连接

    int registerVar( struct HmiVar& hmiVar );//注册外部变量
    int onceRealUpdateVar( int varId );//启动变量一次刷新
    int startUpdateVar( int varId ); //启动更新变量, 注意要在注册完它使用的连接后再启动
    int stopUpdateVar( int varId );  //停止更新变量
    int readDevVar( int varId, uchar* pBuf, int bufLen ); //读取外部变量
    int writeDevVar( int varId, uchar* pBuf, int bufLen ); //写外部变量
    int getWriteStatus( int varId ); //获取写变量的状态，VAR_NO_OPERATION为未操作，其它为返回结果
    int getConnLinkStatus( int connectId );
    int getComMsg( QList<struct ComMsg> &msgList );
    void stopAllConn();

    //
    QList<CommService*> m_commServiceList;
    //QList<QThread*> m_commSrvThreadList; //保存线程的指针，退出时用
    QHash<int, struct HmiVar*> m_hmiVarList;
};

#define CONNECT_NO_OPERATION    0 //连接未操作
#define CONNECT_NO_LINK         1 //没有连接
#define CONNECT_LINKED_OK       2 //连接成功
#define CONNECT_LINKING			3
#define CONNECT_LINKED_READY    4
//int getConnLinkStatus( int connectId ); //获取连接状态

#endif    //COMM_INTERFACE_H

