#ifndef RWSOCKET_H
#define RWSOCKET_H
#include <QObject>

#include <QMutex>
#include <QTimer>
#include <QUdpSocket>
#include "modbus/modbusdriver.h"
#include "MiProtocol4Driver/miprotocol4driver.h"
#include "miprofxDriver/miprofxdriver.h"
#include "cnetDriver/cnetdriver.h"
#include "dvpDriver/dvpdriver.h"
#include "fatekDriver/fatekdriver.h"
#include "finHostlinkDriver/finhostlinkdriver.h"
#include "mewtocolDriver/mewtocoldriver.h"
#include "hostlinkDriver/hostlinkdriver.h"
#include "ppi/ppi_l7wr.h"
#include "freePortDriver/freePort_driver.h"
#include "tcpModbus/tcpModbusDriver.h"


#define VAR_TIMEOUT 1
#define COM_OPEN_SUCCESSFUL   3
#define COM_OPEN_ERR   4
#define MSG_CACHE_CNT (32*2)

namespace HMI {
class HMIProject;
}
class ServerUdp;
const QString WIFI_IP_ADDR = "192.168.125.1";//*WIFI 模块IP地址*/

const int WIFI_PORT = 3636;/* WIFI 模块监听的端口号 */

enum ProtocolType
{
    MODBUS_RTU = 0, /* 0 modbus RTU 协议 */
    PPIMPI,         /* 1 西门子PPI MPI 200 协议 */
    HOSTLINK,       /* 2 欧姆龙 HostLink 协议 */
    MIPROTOCOL4,    /* 3 三菱 Miprotocol4 协议 */
    MIPROFX,        /* 4 三菱 mipproFX 协议 */
    FINHOSTLINK,    /* 5 欧姆龙 FinHostLink 协议 */
    MEWTOCOL,       /* 6 松下 Mewtocol 协议 */
    DVP,            /* 7 台达 DVP 协议 */
    FATEK,          /* 8 永宏 FATEK 协议 */
    CNET,           /* 9 LS产能 CNET 协议 */
    MPI300,         /* 10 西门子MPI300 协议*/
    FREE_PORT,      /* 11 自由口 炜煌打印机 */
    /* 以下为网口协议 */
    MBUS_TCP,       /* 12 Modbus TCP\IP 协议*/
    GATEWAY         /* 13 合信 网关协议*/
};

enum ConnectMsgType
{
    UART_MSG = 0,   /* 0 tcp连接WIFI模块成功 读取远程消息成功 */
    WIFI_MODE_MSG   /* 1 tcp连接WIFI模块出错 或 连接还没建立  */
//	UART_MSG,       /* 2 远程返回 协议可读串口的信息 */
//	CONFIG_MSG      /* 3 远程返回 设置参数结果等信息 */
};

enum ConnectMsgError
{
    CONN_NO_ERR = 0,    /*无错*/
    UART_IN_USE ,       /* 1 UART 正在被其他协议使用 */
    UART_PORT_NUM_ERR,  /* 2 UART 端口错误，没有这个端口 */
    //UART_READ_FAIL,   /* 1 tcp连接WIFI模块出错 或 连接还没建立  */
    //	UART_MSG,       /* 2 远程返回 协议可读串口的信息 */
    UART_NOT_OPEN,      /*3  端口错误，没有open端口*/
    PPI_NOT_INIT,       /*4 ppi协议没有初始化*/
    UART_OPEN_SUCCESS   /*5 ppi协议没有初始化uart open success*/
};

struct UartCfg  /* 串口参数配置结构体 */
{
    uchar COMNum;
    uchar dataBit;
    uchar stopBit;
    uchar parity;
    int baud;
};

struct NetCfg  /* 网口参数配置结构体 */
{
    char ipAddr[20];
    ushort port;
};

struct ClientDataMsgHead    /* tcpsocket 交换数据的头部信息 */
{
    ushort fc;
    ushort errCode;
    ushort proType;
    char remoteIp[4];
    ushort port;
    int dataLen;
    ushort serialNum;
};


union MsgPar
{
    struct MbusMsgPar mbusMsg;
    L2_MASTER_MSG_STRU L2Msg;
    L7_MSG_STRU L7Msg;
    struct MiProtocol4MsgPar mip4Msg;
    struct CnetMsgPar cnetMsg;
    struct FatekMsgPar fatekMsg;
    struct HlinkMsgPar hlinkMsg;
    struct MewtocolMsgPar mewMsg;
    struct finHlinkMsgPar finMsg;
    struct MiprofxMsgPar mipfxMsg;
    struct DvpMsgPar dvpMsg;
    struct FpMsgPar fpMsg;
    mbus_tcp_req_struct tcpMbusMsg;
};

struct MsgCache         /*消息缓存*/
{
    int useFlg;         /*使用标志 0：没有使用  1：已经被使用*/
    int len;            /*buf 长度*/
    int pollCnt;        /*轮询次数 定时器轮询，当轮询到一定次数后可判定该包超时，进行重发*/
    int retryCnt;       /*重发次数 超过三次，向上层提交超时消息*/
    char buf[1024];     /*消息数据*/
    ushort serialNum;   /*帧序号*/
};

class RWSocket : public QObject
{
    Q_OBJECT
public :
    RWSocket(ProtocolType proType, void *uartCfg, int connid, QObject *parent = 0, HMI::HMIProject *p = nullptr); /*串口类型协议构造函数*/
    //RWSocket(ProtocolType proType, struct NetCfg *netCfg,  QObject *parent = 0); /*网口类型协议构造函数*/
    ~RWSocket();
    int openDev();/* tcpsocket 建立连接 */
    int writeDev(char *, int );                     /* 写入消息 */
    int readDev(char *msgPar);                      /* 读取消息*/
    void setTimeoutPollCnt(int cnt);                /*set timeout poll cnt*/
    void clearAllMsgCache();
    qint64 writeDatagramAssured(const char *data, qint64 len, const QHostAddress &host, quint16 port);

public slots:
    void slot_connected();                          /* 槽函数 连接已经建立*/
    void slot_error(QAbstractSocket::SocketError err);/*槽函数 连接出错*/
    void slot_statChanged(QAbstractSocket::SocketState state);/*槽函数 连接状态改变*/
    void slot_readData();                           /* 槽函数 上层读取消息 */
    void slot_readData(const QByteArray& data);

   // void slot_udpTimout();                        /* udp 超时处理 */
    void slot_heartBeat();                          /* 保持连接的心跳 */
    void slot_checkTimeout();

signals:
    void readyRead();                               /* 信号 有可读消息 */
    void stateChanged(quint16 state);
private:
    void commomInit(ProtocolType proType);          /* 初始化类成员 */
    void packetReadMsg(char *buf, ConnectMsgType msgType);/* 打包消息给上层应用*/
    //void packetMbusRTUMsg(char *buf, ConnectMsgType msgType);/* 打包modbusRTU协议消息 */
    //void packetPpiMsg(char *buf, ConnectMsgType msgType);/*打包ppi200协议消息*/
    void packetErrMsg(MsgPar *msg,  char err);

    int packetWriteMsg(const ushort fc, char *wBuf, struct ClientDataMsgHead &dataHead, char *buf, int len);
    struct MsgCache *getAnIdleMsgCache();
    struct MsgCache *getMsgCacheBySerialNum(ushort serialNum);
    void clearMsgCache(struct MsgCache *);
    void insureUsefulUdpSocket();

    void setRemoteUdp();

    //PenetrateUdp *m_udp;
    ServerUdp *m_udp;
    QUdpSocket *m_socket;      /* 连接 WIFI 模块的 tcpSocket */
   // QTimer *m_timeoutTimer;  /* udp 超时定时器 */
    QTimer *m_udpTimer;
    ushort m_serialNum;        /*帧序号*/
    QTimer *m_heartBeat;       /*心跳定时器*/
    QTimer *m_timeout;         /*timeout timer*/
    int m_timeoutCnt;
    QMutex m_cacheMutex;       /*缓存读写锁*/
    QMutex m_msgMutex;         /*上层消息读写锁*/
    QMutex m_mutex;

//  QString m_hostIp;          /* WIFI 模块的 IP 地址 */
    ProtocolType m_proType;    /* socket 对应的串口使用的协议 */
    QList<union MsgPar> *m_readMsgList; /* 可读消息的列表 */
    struct MsgCache m_msgCache[MSG_CACHE_CNT]; /*消息缓存列表 上层最多只有32个连接，缓存大小只需要32个结构体就够了*/

    struct UartCfg *m_uartCfg;  /* 串口配置参数 */
    struct NetCfg  *m_netCfg;   /* 以太网口配置参数 */
    int m_msgLen;
    int m_msgErr;
    int netState;
    //quint8 timeOutCount;
    ushort m_deviceVersion;
    int m_timeoutPollCnt;

    QHostAddress remoteUdpAddr;
    //HMI::HMIProject *m_project;
    int m_connid = 0;
};

#endif
