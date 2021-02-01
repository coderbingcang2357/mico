#ifndef TCPMODBUS_H
#define TCPMODBUS_H

#include "modbusmsg.h"
#include "tcpModbusDriver.h"

#include <QHostAddress>
#include <QObject>


const int MAX_CONN_NUM = 32;/*最大从站数量 即最多能连几个不同的IP*/

const int TCP_MBUS_MODBUS_MAX = 240;/*一包最大字节数*/

#define MODBUX_AREA_0X   0
#define MODBUX_AREA_1X   1
#define MODBUX_AREA_2X   2
#define MODBUX_AREA_3X   3
#define MODBUX_AREA_4X   4

enum MbusMsgErrCode
{
 TCP_MBUS_NO_ERROR,         // 0   /*No error*/
 TCP_MBUS_CONN_FULL_ERROR,   //1  /*连接达到最大值*/
 TCP_MBUS_CONN_LIKING_ERROR,       //2  /*连接正在建立*/
 TCP_MBUS_TIMEOUT_ERROR,     //3   /*超时错误*/
 TCP_MBUS_REQUEST_ERROR,     //4   /*请求错误 */
 TCP_MBUS_ACTIVE_ERROR,      //5   /*Modbus没有使能*/
 TCP_MBUS_BUSY_ERROR,        //6   /*连接正忙于处理其他请求忙*/
 TCP_MBUS_RESPONSE_ERROR,   // 7   /*应答错误*/
 TCP_MBUS_CREATE_CONN_ERROR, //8 /*创建连接失败*/
 TCP_MBUS_CONN_EXIST,        //9   /*连接已存在*/
 TCP_MBUS_CONN_NOT_EXIST    //10  /*连接不存在*/
};


struct MBUSMSG
{
    QHostAddress IP;/*目标IP地址，[192][168][100][125]*/
    ushort Port;
    uchar RW;   /*操作命令:0--读,1--写*/
    int   Addr;/*选择读写的数据类型,0000至0xxxx--开关量输出*/
            /*10000至1xxxx--开关量输入*/
            /*30000至3xxxx--模拟量输入*/
            /*40000至4xxxx--保持寄存器*/
    ushort Count;/*通讯的数据个数（位或字的个
                 数Modbus主站每一 个MbusMsg指令可读/写的最
                 大数据量为120个字*/
    uchar UnitID;
    uchar *DataPtr;
    uchar Done;
    MbusMsgErrCode ErrCode;
};

enum ConnectType{
    CONNECT_NULL = 0,
    CONNECT_ORG,
    CONNECT_LOCAL,
    CONNECT_REMOTE
};

//class QHash;
class RWSocket;
class tcpModbus : public QObject
{
    Q_OBJECT

public:
    tcpModbus(QObject *parent = 0);
    ~tcpModbus();
    void tcpMbusMsgReq(MBUSMSG *, int connid);
    void endiandChg(uchar *dataPtr, int len);
public slots:
    void slot_getDate(MBUSDATA *);
    void slot_getLocalData();
    void slot_getRemoteData();

private:
    ConnectType getConnectType(QHostAddress ipAddr, unsigned short port, int connid);
    MbusMsgErrCode tcpMbusPackNewRequest( MBUSDATA *pTcpMbusData, uchar RW, int Addr, ushort count, uchar UnitID, uchar *DataPtr );
    MbusMsgErrCode getIdleMbusData(MBUSDATA **);

private:
    TcpModbusMSG *m_tcpMbusConn[MAX_CONN_NUM];
    MBUSDATA      m_Data[MAX_CONN_NUM];

    QHash<MBUSDATA*, TcpModbusMSG*> *m_ConnListHash;
    QHash<MBUSDATA*,QHostAddress> *m_ActiveMbusData;
    QHash<MBUSDATA*,MBUSMSG*>     *m_ActiveMbusReq;


    QHash<QHostAddress, ConnectType> *m_remoteFlgHash;  //记录组态的IP 地址对应连接的类型

    QHash<QHostAddress, QString> *m_localDevIPHash;    //记录组态的IP 与 设备IP 的对应关系   连接类型为CONNECT_LOCAL时使用
    QHash<QHostAddress, MBUSMSG*> *m_localMbusMsgHash;    //记录RWSocket 地址与 MBUSMSG 的对应关系 连接类型为CONNECT_LOCAL时使用
    QHash<QString, RWSocket*> *m_localRwSockHash;  //记录组态的IP 地址与RWsocket的关系 连接类型为CONNECT_LOCAL时使用

    QHash<QHostAddress, quint64> *m_remoteDevIDHash;    //记录组态的IP 与 设备ID 的对应关系   连接类型为CONNECT_REMOTE时使用
    QHash<QHostAddress, MBUSMSG*> *m_remoteMbusMsgHash; //记录组态的IP 地址与MBUSMSG的关系 连接类型为CONNECT_REMOTE时使用
    QHash<quint64, RWSocket*> *m_remoteRwSockHash;      //记录设备ID与 RWsocket的对应关系 连接类型为CONNECT_REMOTE时使用
};
/*
 * 连接没有选择设备的时候，按照TCP MUDBUS标准流程走
 * 连接选择了本地的设备的时候 由于不允许组态相同的IP地址，所以对应关系为
 * 数据请求 IP --> DestIP DestIP -- > RWSocket  RWSocket --> 6453网关
 * 数据应答 6453 --> RWSocket RWSocket返回数据中获取IP IP --> MBUSMSG   RWSocket --> MBUSMSG
 *
 * 连接选择了远程设备的时候
 * 数据请求 IP --> DeviceID,  DeviceID --> RWSocket， RWSocket --> 6453网关
 * 数据应答 6453 --> RWSocket  RWSocket返回数据中获取IP IP --> MBUSMSG
*/

#endif // TCPMODBUS_H
