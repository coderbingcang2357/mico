
#include <QtCore/QCoreApplication>
#include <QDebug>
#include <QThread>
#include "commInterface.h"
#include "../rwsocket.h"

int ret;
/*
struct MiProtocol4ConnectCfg
{
int id;                 //连接ID
INT8U COM_interface;		//接口或端口号
INT32U baud;			//波特率
INT8U hmiAddr;		    //HMI地址,可以不要
INT8U cpuAddr;          //CPU地址
INT8U parity;		    //奇偶校验
INT8U dataBit;		    //数据位
INT8U stopBit; 		    //停止位
INT8U checkSum;
INT8U PlcNo;
};
*/
void cnetTest()
{
    struct CnetConnectCfg connectCfg;
    connectCfg.id = 4;
    connectCfg.COM_interface = 0;
    connectCfg.baud = 115200;
    connectCfg.hmiAddr = 2;
    connectCfg.cpuAddr = 4;
    connectCfg.parity = 2;
    connectCfg.dataBit = 8;
    connectCfg.stopBit = 1;

    ret = registerConnect( connectCfg );
    qDebug() << "registerConnect0 " << ret;

    struct HmiVar hmiVar;
    hmiVar.varId = 8;					 //变量的唯一标识码//用做数组的脚标,所以这个ID是连续的一个值
    hmiVar.connectId = 4;               //对应那个连接，连接数组的脚标
    hmiVar.cycleTime = 100;            //变量的刷新周期，单位毫秒 (最大周期值接近71分钟)
    hmiVar.area = CNET_AREA_D;      //变量所在的区域
    hmiVar.addOffset = 0;              //数据在PLC中的字节偏移量
    hmiVar.bitOffset = -1;               //如果需要位偏移量，那么这个位变量的偏移量
    hmiVar.len = 2;                    //变量的数据长度，字节个数, 位变量为位的个数
    hmiVar.dataType = 3;              //变量的类型
    hmiVar.lastUpdateTime = -1;          //最后更新时间
    hmiVar.err = 0;

    ret = registerVar( hmiVar );
    qDebug() << "registerVar var0 " << ret;
    ret = startUpdateVar( 8 );
    qDebug() << "startUpdateVar var0 " << ret;

    hmiVar.varId = 9;					 //变量的唯一标识码//用做数组的脚标,所以这个ID是连续的一个值
    hmiVar.connectId = 4;               //对应那个连接，连接数组的脚标
    ret = registerVar( hmiVar );
    qDebug() << "registerVar var1 " << ret;
    ret = startUpdateVar( 9 );
    qDebug() << "startUpdateVar var1 " << ret;
}
void mewtocolTest()
{
    struct MewtocolConnectCfg connectCfg;
    connectCfg.id = 4;
    connectCfg.COM_interface = 0;
    connectCfg.baud = 115200;
    connectCfg.hmiAddr = 2;
    connectCfg.cpuAddr = 4;
    connectCfg.parity = 2;
    connectCfg.dataBit = 8;
    connectCfg.stopBit = 1;

    ret = registerConnect( connectCfg );
    qDebug() << "registerConnect0 " << ret;

    struct HmiVar hmiVar;
    hmiVar.varId = 8;					 //变量的唯一标识码//用做数组的脚标,所以这个ID是连续的一个值
    hmiVar.connectId = 4;               //对应那个连接，连接数组的脚标
    hmiVar.cycleTime = 100;            //变量的刷新周期，单位毫秒 (最大周期值接近71分钟)
    hmiVar.area = MEWTOCOL_AREA_DT;      //变量所在的区域
    hmiVar.addOffset = 0;              //数据在PLC中的字节偏移量
    hmiVar.bitOffset = -1;               //如果需要位偏移量，那么这个位变量的偏移量
    hmiVar.len = 2;                    //变量的数据长度，字节个数, 位变量为位的个数
    hmiVar.dataType = 3;              //变量的类型
    hmiVar.lastUpdateTime = -1;          //最后更新时间
    hmiVar.err = 0;

    ret = registerVar( hmiVar );
    qDebug() << "registerVar var0 " << ret;
    ret = startUpdateVar( 8 );
    qDebug() << "startUpdateVar var0 " << ret;

    hmiVar.varId = 9;					 //变量的唯一标识码//用做数组的脚标,所以这个ID是连续的一个值
    hmiVar.connectId = 4;               //对应那个连接，连接数组的脚标
    ret = registerVar( hmiVar );
    qDebug() << "registerVar var1 " << ret;
    ret = startUpdateVar( 9 );
    qDebug() << "startUpdateVar var1 " << ret;
}
void mipFXTest()
{
    struct MiProfxConnectCfg connectCfg;
    connectCfg.id = 4;
    connectCfg.COM_interface = 0;
    connectCfg.baud = 115200;
    connectCfg.hmiAddr = 2;
    connectCfg.cpuAddr = 0;
    connectCfg.parity = 2;
    connectCfg.dataBit = 8;
    connectCfg.stopBit = 1;

    ret = registerConnect( connectCfg );
    qDebug() << "registerConnect0 " << ret;

    struct HmiVar hmiVar;
    hmiVar.varId = 8;					 //变量的唯一标识码//用做数组的脚标,所以这个ID是连续的一个值
    hmiVar.connectId = 4;               //对应那个连接，连接数组的脚标
    hmiVar.cycleTime = 100;            //变量的刷新周期，单位毫秒 (最大周期值接近71分钟)
    hmiVar.area = MIPROFX_AREA_D;      //变量所在的区域
    hmiVar.addOffset = 0;              //数据在PLC中的字节偏移量
    hmiVar.bitOffset = -1;               //如果需要位偏移量，那么这个位变量的偏移量
    hmiVar.len = 1;                    //变量的数据长度，字节个数, 位变量为位的个数
    hmiVar.dataType = 3;              //变量的类型
    hmiVar.lastUpdateTime = -1;          //最后更新时间
    hmiVar.err = 0;

    ret = registerVar( hmiVar );
    qDebug() << "registerVar var0 " << ret;
    ret = startUpdateVar( 8 );
    qDebug() << "startUpdateVar var0 " << ret;

    hmiVar.varId = 9;					 //变量的唯一标识码//用做数组的脚标,所以这个ID是连续的一个值
    hmiVar.connectId = 4;               //对应那个连接，连接数组的脚标
    ret = registerVar( hmiVar );
    qDebug() << "registerVar var1 " << ret;
    ret = startUpdateVar( 9 );
    qDebug() << "startUpdateVar var1 " << ret;
}

void finHlinkTest()
{
    struct FinHostlinkConnectCfg connectCfg;
    connectCfg.id = 4;
    connectCfg.COM_interface = 0;
    connectCfg.baud = 115200;
    connectCfg.hmiAddr = 2;
    connectCfg.cpuAddr = 0;
    connectCfg.parity = 2;
    connectCfg.dataBit = 8;
    connectCfg.stopBit = 1;

    ret = registerConnect( connectCfg );
    qDebug() << "registerConnect0 " << ret;

    struct HmiVar hmiVar;
    hmiVar.varId = 8;					 //变量的唯一标识码//用做数组的脚标,所以这个ID是连续的一个值
    hmiVar.connectId = 4;               //对应那个连接，连接数组的脚标
    hmiVar.cycleTime = 100;            //变量的刷新周期，单位毫秒 (最大周期值接近71分钟)
    hmiVar.area = FIN_HOST_LINK_AREA_DM;      //变量所在的区域
    hmiVar.addOffset = 0;              //数据在PLC中的字节偏移量
    hmiVar.bitOffset = -1;               //如果需要位偏移量，那么这个位变量的偏移量
    hmiVar.len = 1;                    //变量的数据长度，字节个数, 位变量为位的个数
    hmiVar.dataType = 3;              //变量的类型
    hmiVar.lastUpdateTime = -1;          //最后更新时间
    hmiVar.err = 0;

    ret = registerVar( hmiVar );
    qDebug() << "registerVar var0 " << ret;
    ret = startUpdateVar( 8 );
    qDebug() << "startUpdateVar var0 " << ret;

    hmiVar.varId = 9;					 //变量的唯一标识码//用做数组的脚标,所以这个ID是连续的一个值
    hmiVar.connectId = 4;               //对应那个连接，连接数组的脚标
    ret = registerVar( hmiVar );
    qDebug() << "registerVar var1 " << ret;
    ret = startUpdateVar( 9 );
    qDebug() << "startUpdateVar var1 " << ret;
}

void HlinkTest()
{
    struct HostlinkConnectCfg connectCfg;
    connectCfg.id = 4;
    connectCfg.COM_interface = 0;
    connectCfg.baud = 115200;
    connectCfg.hmiAddr = 2;
    connectCfg.cpuAddr = 0;
    connectCfg.parity = 2;
    connectCfg.dataBit = 8;
    connectCfg.stopBit = 1;

    ret = registerConnect( connectCfg );
    qDebug() << "registerConnect0 " << ret;

    struct HmiVar hmiVar;
    hmiVar.varId = 8;					 //变量的唯一标识码//用做数组的脚标,所以这个ID是连续的一个值
    hmiVar.connectId = 4;               //对应那个连接，连接数组的脚标
    hmiVar.cycleTime = 100;            //变量的刷新周期，单位毫秒 (最大周期值接近71分钟)
    hmiVar.area = HOST_LINK_AREA_DM;      //变量所在的区域
    hmiVar.addOffset = 0;              //数据在PLC中的字节偏移量
    hmiVar.bitOffset = -1;               //如果需要位偏移量，那么这个位变量的偏移量
    hmiVar.len = 1;                    //变量的数据长度，字节个数, 位变量为位的个数
    hmiVar.dataType = 3;              //变量的类型
    hmiVar.lastUpdateTime = -1;          //最后更新时间
    hmiVar.err = 0;

    ret = registerVar( hmiVar );
    qDebug() << "registerVar var0 " << ret;
    ret = startUpdateVar( 8 );
    qDebug() << "startUpdateVar var0 " << ret;

    hmiVar.varId = 9;					 //变量的唯一标识码//用做数组的脚标,所以这个ID是连续的一个值
    hmiVar.connectId = 4;               //对应那个连接，连接数组的脚标
    ret = registerVar( hmiVar );
    qDebug() << "registerVar var1 " << ret;
    ret = startUpdateVar( 9 );
    qDebug() << "startUpdateVar var1 " << ret;
}


void fatekTest()
{
    struct FatekConnectCfg connectCfg;
    connectCfg.id = 4;
    connectCfg.COM_interface = 0;
    connectCfg.baud = 115200;
    connectCfg.hmiAddr = 2;
    connectCfg.cpuAddr = 0;
    connectCfg.parity = 2;
    connectCfg.dataBit = 8;
    connectCfg.stopBit = 1;

    ret = registerConnect( connectCfg );
    qDebug() << "registerConnect0 " << ret;

/*	connectCfg.id = 3;
    connectCfg.COM_interface = 0;
    connectCfg.baud = 115200;
    connectCfg.hmiAddr = 0;
    connectCfg.cpuAddr = 1;
    connectCfg.parity = 2;
    connectCfg.dataBit = 8;
    connectCfg.stopBit = 1;
    ret = registerConnect( connectCfg );
    qDebug() << "registerConnect1 " << ret;
    */
    struct HmiVar hmiVar;
    hmiVar.varId = 8;					 //变量的唯一标识码//用做数组的脚标,所以这个ID是连续的一个值
    hmiVar.connectId = 4;               //对应那个连接，连接数组的脚标
    hmiVar.cycleTime = 100;            //变量的刷新周期，单位毫秒 (最大周期值接近71分钟)
    hmiVar.area =  DVP_AREA_D;      //变量所在的区域
    hmiVar.addOffset = 0;              //数据在PLC中的字节偏移量
    hmiVar.bitOffset = -1;               //如果需要位偏移量，那么这个位变量的偏移量
    hmiVar.len = 1;                    //变量的数据长度，字节个数, 位变量为位的个数
    hmiVar.dataType = 3;              //变量的类型
    hmiVar.lastUpdateTime = -1;          //最后更新时间
    hmiVar.err = 0;
/*
    hmiVar.varId = 1;					 //变量的唯一标识码//用做数组的脚标,所以这个ID是连续的一个值
    hmiVar.connectId = 1;               //对应那个连接，连接数组的脚标
    hmiVar.cycleTime = 100;            //变量的刷新周期，单位毫秒 (最大周期值接近71分钟)
    hmiVar.area =  MODBUX_AREA_4X;      //变量所在的区域
    hmiVar.addOffset = 0;              //数据在PLC中的字节偏移量
    hmiVar.bitOffset = -1;               //如果需要位偏移量，那么这个位变量的偏移量
    hmiVar.len = 32;                    //变量的数据长度，字节个数, 位变量为位的个数
    hmiVar.dataType = 3;              //变量的类型
    hmiVar.lastUpdateTime = -1;          //最后更新时间
    hmiVar.err = 0;
*/
    ret = registerVar( hmiVar );
    qDebug() << "registerVar var0 " << ret;
    ret = startUpdateVar( 8 );
    qDebug() << "startUpdateVar var0 " << ret;

    hmiVar.varId = 9;					 //变量的唯一标识码//用做数组的脚标,所以这个ID是连续的一个值
    hmiVar.connectId = 4;               //对应那个连接，连接数组的脚标
    ret = registerVar( hmiVar );
    qDebug() << "registerVar var1 " << ret;
    ret = startUpdateVar( 9 );
    qDebug() << "startUpdateVar var1 " << ret;
}

void dvpTest()
{
    struct DvpConnectCfg connectCfg;
    connectCfg.id = 4;
    connectCfg.COM_interface = 0;
    connectCfg.baud = 115200;
    connectCfg.hmiAddr = 2;
    connectCfg.cpuAddr = 0;
    connectCfg.parity = 2;
    connectCfg.dataBit = 8;
    connectCfg.stopBit = 1;

    ret = registerConnect( connectCfg );
    qDebug() << "registerConnect0 " << ret;

/*	connectCfg.id = 3;
    connectCfg.COM_interface = 0;
    connectCfg.baud = 115200;
    connectCfg.hmiAddr = 0;
    connectCfg.cpuAddr = 1;
    connectCfg.parity = 2;
    connectCfg.dataBit = 8;
    connectCfg.stopBit = 1;
    ret = registerConnect( connectCfg );
    qDebug() << "registerConnect1 " << ret;
    */
    struct HmiVar hmiVar;
    hmiVar.varId = 8;					 //变量的唯一标识码//用做数组的脚标,所以这个ID是连续的一个值
    hmiVar.connectId = 4;               //对应那个连接，连接数组的脚标
    hmiVar.cycleTime = 100;            //变量的刷新周期，单位毫秒 (最大周期值接近71分钟)
    hmiVar.area =  DVP_AREA_D;      //变量所在的区域
    hmiVar.addOffset = 0;              //数据在PLC中的字节偏移量
    hmiVar.bitOffset = -1;               //如果需要位偏移量，那么这个位变量的偏移量
    hmiVar.len = 1;                    //变量的数据长度，字节个数, 位变量为位的个数
    hmiVar.dataType = 3;              //变量的类型
    hmiVar.lastUpdateTime = -1;          //最后更新时间
    hmiVar.err = 0;
/*
    hmiVar.varId = 1;					 //变量的唯一标识码//用做数组的脚标,所以这个ID是连续的一个值
    hmiVar.connectId = 1;               //对应那个连接，连接数组的脚标
    hmiVar.cycleTime = 100;            //变量的刷新周期，单位毫秒 (最大周期值接近71分钟)
    hmiVar.area =  MODBUX_AREA_4X;      //变量所在的区域
    hmiVar.addOffset = 0;              //数据在PLC中的字节偏移量
    hmiVar.bitOffset = -1;               //如果需要位偏移量，那么这个位变量的偏移量
    hmiVar.len = 32;                    //变量的数据长度，字节个数, 位变量为位的个数
    hmiVar.dataType = 3;              //变量的类型
    hmiVar.lastUpdateTime = -1;          //最后更新时间
    hmiVar.err = 0;
*/
    ret = registerVar( hmiVar );
    qDebug() << "registerVar var0 " << ret;
    ret = startUpdateVar( 8 );
    qDebug() << "startUpdateVar var0 " << ret;

    hmiVar.varId = 9;					 //变量的唯一标识码//用做数组的脚标,所以这个ID是连续的一个值
    hmiVar.connectId = 4;               //对应那个连接，连接数组的脚标
    ret = registerVar( hmiVar );
    qDebug() << "registerVar var1 " << ret;
    ret = startUpdateVar( 9 );
    qDebug() << "startUpdateVar var1 " << ret;
}

void mip4Test()
{
    struct MiProtocol4ConnectCfg connectCfg;
    connectCfg.id = 4;
    connectCfg.COM_interface = 0;
    connectCfg.baud = 115200;
    connectCfg.hmiAddr = 2;
    connectCfg.cpuAddr = 0;
    connectCfg.parity = 2;
    connectCfg.dataBit = 8;
    connectCfg.stopBit = 1;
    connectCfg.checkSum = 1;
    connectCfg.PlcNo = 1;

    ret = registerConnect( connectCfg );
    qDebug() << "registerConnect0 " << ret;

/*	connectCfg.id = 3;
    connectCfg.COM_interface = 0;
    connectCfg.baud = 115200;
    connectCfg.hmiAddr = 0;
    connectCfg.cpuAddr = 1;
    connectCfg.parity = 2;
    connectCfg.dataBit = 8;
    connectCfg.stopBit = 1;
    ret = registerConnect( connectCfg );
    qDebug() << "registerConnect1 " << ret;
    */
    struct HmiVar hmiVar;
    hmiVar.varId = 8;					 //变量的唯一标识码//用做数组的脚标,所以这个ID是连续的一个值
    hmiVar.connectId = 4;               //对应那个连接，连接数组的脚标
    hmiVar.cycleTime = 100;            //变量的刷新周期，单位毫秒 (最大周期值接近71分钟)
    hmiVar.area =  MIPROTOCOL4_AREA_D;      //变量所在的区域
    hmiVar.addOffset = 0;              //数据在PLC中的字节偏移量
    hmiVar.bitOffset = -1;               //如果需要位偏移量，那么这个位变量的偏移量
    hmiVar.len = 1;                    //变量的数据长度，字节个数, 位变量为位的个数
    hmiVar.dataType = 3;              //变量的类型
    hmiVar.lastUpdateTime = -1;          //最后更新时间
    hmiVar.err = 0;
/*
    hmiVar.varId = 1;					 //变量的唯一标识码//用做数组的脚标,所以这个ID是连续的一个值
    hmiVar.connectId = 1;               //对应那个连接，连接数组的脚标
    hmiVar.cycleTime = 100;            //变量的刷新周期，单位毫秒 (最大周期值接近71分钟)
    hmiVar.area =  MODBUX_AREA_4X;      //变量所在的区域
    hmiVar.addOffset = 0;              //数据在PLC中的字节偏移量
    hmiVar.bitOffset = -1;               //如果需要位偏移量，那么这个位变量的偏移量
    hmiVar.len = 32;                    //变量的数据长度，字节个数, 位变量为位的个数
    hmiVar.dataType = 3;              //变量的类型
    hmiVar.lastUpdateTime = -1;          //最后更新时间
    hmiVar.err = 0;
*/
    ret = registerVar( hmiVar );
    qDebug() << "registerVar var0 " << ret;
    ret = startUpdateVar( 8 );
    qDebug() << "startUpdateVar var0 " << ret;

    hmiVar.varId = 9;					 //变量的唯一标识码//用做数组的脚标,所以这个ID是连续的一个值
    hmiVar.connectId = 4;               //对应那个连接，连接数组的脚标
    ret = registerVar( hmiVar );
    qDebug() << "registerVar var1 " << ret;
    ret = startUpdateVar( 9 );
    qDebug() << "startUpdateVar var1 " << ret;
}

void ppiTest()
{
    struct PpiMpiConnectCfg connectCfg;
    connectCfg.id = 3;
    connectCfg.COM_interface = 0;
    connectCfg.baud = 19200;
    connectCfg.hmiAddr = 0;
    connectCfg.cpuAddr = 2;
    connectCfg.hsa = 128;
    connectCfg.isMpi = 1;

    ret = registerConnect( connectCfg );
    qDebug() << "registerConnect0 " << ret;

/*	connectCfg.id = 3;
    connectCfg.COM_interface = 0;
    connectCfg.baud = 115200;
    connectCfg.hmiAddr = 0;
    connectCfg.cpuAddr = 1;
    connectCfg.parity = 2;
    connectCfg.dataBit = 8;
    connectCfg.stopBit = 1;
    ret = registerConnect( connectCfg );
    qDebug() << "registerConnect1 " << ret;
    */
    struct HmiVar hmiVar;
    hmiVar.varId = 5;					 //变量的唯一标识码//用做数组的脚标,所以这个ID是连续的一个值
    hmiVar.connectId = 3;               //对应那个连接，连接数组的脚标
    hmiVar.cycleTime = 100;            //变量的刷新周期，单位毫秒 (最大周期值接近71分钟)
    hmiVar.area =  PPI_AP_AREA_V;      //变量所在的区域
    hmiVar.addOffset = 0;              //数据在PLC中的字节偏移量
    hmiVar.bitOffset = -1;               //如果需要位偏移量，那么这个位变量的偏移量
    hmiVar.len = 32;                    //变量的数据长度，字节个数, 位变量为位的个数
    hmiVar.dataType = 3;              //变量的类型
    hmiVar.lastUpdateTime = -1;          //最后更新时间
    hmiVar.err = 0;
/*
    hmiVar.varId = 1;					 //变量的唯一标识码//用做数组的脚标,所以这个ID是连续的一个值
    hmiVar.connectId = 1;               //对应那个连接，连接数组的脚标
    hmiVar.cycleTime = 100;            //变量的刷新周期，单位毫秒 (最大周期值接近71分钟)
    hmiVar.area =  MODBUX_AREA_4X;      //变量所在的区域
    hmiVar.addOffset = 0;              //数据在PLC中的字节偏移量
    hmiVar.bitOffset = -1;               //如果需要位偏移量，那么这个位变量的偏移量
    hmiVar.len = 32;                    //变量的数据长度，字节个数, 位变量为位的个数
    hmiVar.dataType = 3;              //变量的类型
    hmiVar.lastUpdateTime = -1;          //最后更新时间
    hmiVar.err = 0;
*/
    ret = registerVar( hmiVar );
    qDebug() << "registerVar var0 " << ret;
    ret = startUpdateVar( 5 );
    qDebug() << "startUpdateVar var0 " << ret;

    hmiVar.varId = 6;					 //变量的唯一标识码//用做数组的脚标,所以这个ID是连续的一个值
    hmiVar.connectId = 3;               //对应那个连接，连接数组的脚标
    hmiVar.cycleTime = 1000;
    ret = registerVar( hmiVar );
    qDebug() << "registerVar var1 " << ret;
    ret = startUpdateVar( 6 );
    qDebug() << "startUpdateVar var1 " << ret;
}

void modbusTest()
{
    struct ModbusConnectCfg connectCfg;
    connectCfg.id = 2;
    connectCfg.COM_interface = 0;
    connectCfg.baud = 115200;
    connectCfg.hmiAddr = 0;
    connectCfg.cpuAddr = 2;
    connectCfg.parity = 2;
    connectCfg.dataBit = 8;
    connectCfg.stopBit = 1;
    ret = registerConnect( connectCfg );
    qDebug() << "registerConnect0 " << ret;

/*	connectCfg.id = 3;
    connectCfg.COM_interface = 0;
    connectCfg.baud = 115200;
    connectCfg.hmiAddr = 0;
    connectCfg.cpuAddr = 1;
    connectCfg.parity = 2;
    connectCfg.dataBit = 8;
    connectCfg.stopBit = 1;
    ret = registerConnect( connectCfg );
    qDebug() << "registerConnect1 " << ret;
    */
    struct HmiVar hmiVar;
    hmiVar.varId = 2;					 //变量的唯一标识码//用做数组的脚标,所以这个ID是连续的一个值
    hmiVar.connectId = 2;               //对应那个连接，连接数组的脚标
    hmiVar.cycleTime = 100;            //变量的刷新周期，单位毫秒 (最大周期值接近71分钟)
    hmiVar.area =  MODBUX_AREA_4X;      //变量所在的区域
    hmiVar.addOffset = 0;              //数据在PLC中的字节偏移量
    hmiVar.bitOffset = -1;               //如果需要位偏移量，那么这个位变量的偏移量
    hmiVar.len = 32;                    //变量的数据长度，字节个数, 位变量为位的个数
    hmiVar.dataType = 3;              //变量的类型
    hmiVar.lastUpdateTime = -1;          //最后更新时间
    hmiVar.err = 0;
/*
    hmiVar.varId = 1;					 //变量的唯一标识码//用做数组的脚标,所以这个ID是连续的一个值
    hmiVar.connectId = 1;               //对应那个连接，连接数组的脚标
    hmiVar.cycleTime = 100;            //变量的刷新周期，单位毫秒 (最大周期值接近71分钟)
    hmiVar.area =  MODBUX_AREA_4X;      //变量所在的区域
    hmiVar.addOffset = 0;              //数据在PLC中的字节偏移量
    hmiVar.bitOffset = -1;               //如果需要位偏移量，那么这个位变量的偏移量
    hmiVar.len = 32;                    //变量的数据长度，字节个数, 位变量为位的个数
    hmiVar.dataType = 3;              //变量的类型
    hmiVar.lastUpdateTime = -1;          //最后更新时间
    hmiVar.err = 0;
*/
    ret = registerVar( hmiVar );
    qDebug() << "registerVar var0 " << ret;
    ret = startUpdateVar( 2 );
    qDebug() << "startUpdateVar var0 " << ret;

    hmiVar.varId = 3;					 //变量的唯一标识码//用做数组的脚标,所以这个ID是连续的一个值
    hmiVar.connectId = 2;               //对应那个连接，连接数组的脚标
    ret = registerVar( hmiVar );
    qDebug() << "registerVar var1 " << ret;
    ret = startUpdateVar( 3 );
    qDebug() << "startUpdateVar var1 " << ret;
}

void gatwayTest()
{
    struct GateWayConnectCfg ppiconnectCfg;
    ppiconnectCfg.id = 0;
    ppiconnectCfg.IPAddr = "10.10.10.22";
    ppiconnectCfg.rPort = 20000;
    ppiconnectCfg.cpuAddr = 2;
    ret = registerConnect( ppiconnectCfg );
    qDebug() << "registerConnect 0 " << ret;

    ppiconnectCfg.id = 1;
    ppiconnectCfg.IPAddr = "192.168.1.200";
    ppiconnectCfg.rPort = 20000;
    ppiconnectCfg.cpuAddr = 0;
    ret = registerConnect( ppiconnectCfg );
    qDebug() << "registerConnect 1 " << ret;

    struct HmiVar hmiVar;

    hmiVar.varId = 0;					 //变量的唯一标识码//用做数组的脚标,所以这个ID是连续的一个值
    hmiVar.connectId = 0;               //对应那个连接，连接数组的脚标
    hmiVar.cycleTime = 100;            //变量的刷新周期，单位毫秒 (最大周期值接近71分钟)
    hmiVar.area =  PPI_AP_AREA_V;      //变量所在的区域
    hmiVar.addOffset = 0;              //数据在PLC中的字节偏移量
    hmiVar.bitOffset = -1;               //如果需要位偏移量，那么这个位变量的偏移量
    hmiVar.len = 32;                    //变量的数据长度，字节个数, 位变量为位的个数
    hmiVar.dataType = 3;              //变量的类型
    hmiVar.lastUpdateTime = -1;          //最后更新时间
    hmiVar.err = 0;
    ret = registerVar( hmiVar );
    qDebug() << "registerVar var0 " << ret;
    ret = startUpdateVar( 0 );
    qDebug() << "startUpdateVar var0 " << ret;

    hmiVar.varId = 1;					 //变量的唯一标识码//用做数组的脚标,所以这个ID是连续的一个值
    hmiVar.connectId = 1;               //对应那个连接，连接数组的脚标
    hmiVar.cycleTime = 100;            //变量的刷新周期，单位毫秒 (最大周期值接近71分钟)
    hmiVar.area =  PPI_AP_AREA_V;      //变量所在的区域
    hmiVar.addOffset = 0;              //数据在PLC中的字节偏移量
    hmiVar.bitOffset = -1;               //如果需要位偏移量，那么这个位变量的偏移量
    hmiVar.len = 100;                    //变量的数据长度，字节个数, 位变量为位的个数
    hmiVar.dataType = 3;              //变量的类型
    hmiVar.lastUpdateTime = -1;          //最后更新时间
    hmiVar.err = 0;
    ret = registerVar( hmiVar );
    qDebug() << "registerVar var1 " << ret;
    ret = startUpdateVar( 1 );
    qDebug() << "startUpdateVar var1 " << ret;
}

void mpi300Test()
{
    struct Mpi300ConnectCfg connectCfg;
    connectCfg.id = 3;
    connectCfg.COM_interface = 0;
    connectCfg.baud = 187500;
    connectCfg.hmiAddr = 0;
    connectCfg.cpuAddr = 2;
    connectCfg.hsa = 126;
    connectCfg.isMpi = 1;

    ret = registerConnect( connectCfg );
    qDebug() << "registerConnect0 " << ret;

/*	connectCfg.id = 3;
    connectCfg.COM_interface = 0;
    connectCfg.baud = 115200;
    connectCfg.hmiAddr = 0;
    connectCfg.cpuAddr = 1;
    connectCfg.parity = 2;
    connectCfg.dataBit = 8;
    connectCfg.stopBit = 1;
    ret = registerConnect( connectCfg );
    qDebug() << "registerConnect1 " << ret;
    */
    struct HmiVar hmiVar;
    hmiVar.varId = 5;					 //变量的唯一标识码//用做数组的脚标,所以这个ID是连续的一个值
    hmiVar.connectId = 3;               //对应那个连接，连接数组的脚标
    hmiVar.cycleTime = 100;            //变量的刷新周期，单位毫秒 (最大周期值接近71分钟)
    hmiVar.area =  PPI_AP_AREA_V;      //变量所在的区域
    hmiVar.addOffset = 0;              //数据在PLC中的字节偏移量
    hmiVar.bitOffset = -1;               //如果需要位偏移量，那么这个位变量的偏移量
    hmiVar.len = 32;                    //变量的数据长度，字节个数, 位变量为位的个数
    hmiVar.dataType = 3;              //变量的类型
    hmiVar.lastUpdateTime = -1;          //最后更新时间
    hmiVar.err = 0;
/*
    hmiVar.varId = 1;					 //变量的唯一标识码//用做数组的脚标,所以这个ID是连续的一个值
    hmiVar.connectId = 1;               //对应那个连接，连接数组的脚标
    hmiVar.cycleTime = 100;            //变量的刷新周期，单位毫秒 (最大周期值接近71分钟)
    hmiVar.area =  MODBUX_AREA_4X;      //变量所在的区域
    hmiVar.addOffset = 0;              //数据在PLC中的字节偏移量
    hmiVar.bitOffset = -1;               //如果需要位偏移量，那么这个位变量的偏移量
    hmiVar.len = 32;                    //变量的数据长度，字节个数, 位变量为位的个数
    hmiVar.dataType = 3;              //变量的类型
    hmiVar.lastUpdateTime = -1;          //最后更新时间
    hmiVar.err = 0;
*/
    ret = registerVar( hmiVar );
    qDebug() << "registerVar var0 " << ret;
    ret = startUpdateVar( 5 );
    qDebug() << "startUpdateVar var0 " << ret;

    hmiVar.varId = 6;					 //变量的唯一标识码//用做数组的脚标,所以这个ID是连续的一个值
    hmiVar.connectId = 3;               //对应那个连接，连接数组的脚标
    ret = registerVar( hmiVar );
    qDebug() << "registerVar var1 " << ret;
    ret = startUpdateVar( 6 );
    qDebug() << "startUpdateVar var1 " << ret;
}
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    //modbusTest();
    //gatwayTest();
    ppiTest();
    //mip4Test();
    //dvpTest();
    //fatekTest();
    //HlinkTest();
    //finHlinkTest();
    //mipFXTest();
    //mewtocolTest();
    //cnetTest();
    //mpi300Test();
    return a.exec();
}
