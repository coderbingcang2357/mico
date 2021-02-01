#ifdef PI
#undef PI
#endif

#ifndef MODBUSMSG_H
#define MODBUSMSG_H

#include <QThread>
#include <QHostAddress>

class QTcpSocket;
class QHostAddress;

/*连接当前状态*/
enum TcpConnStat{
    TCP_MBUS_CONNECT_NULL,      // 0   /*0：连接没有建立*/
    TCP_MBUS_CONNECT_LINKING ,  // 1   /*1：连接正在建立*/
    TCP_MBUS_CONNECT_IDLE,      // 2   /*2: 连接已经建立，且为空闲*/
    TCP_MBUS_CONNECT_BUSY,      // 3   /*3: 连接已经建立，且正在处理请求*/
    TCP_MBUS_CONNECT_NUM_FULL,  // 4   /*4: 连接数量已达最大值*/
    TCP_MBUS_CONNECT_ABORTED,   // 5   /*5: 连接被中止*/
    TCP_MBUS_CONNECT_TIMEOUT,   // 6   /*6: 连接超时*/
    TCP_MBUS_CONNECT_CLOSING,   // 7   /*7: 连接正在关闭*/
    TCP_MBUS_CONNECT_NEED_CLOSE // 8   /*8: 服务端被停止，若这个连接为连接到本地服务端的连接，需要断开。*/
};
#pragma pack(push,1)

struct MBUSDATA
{
    ushort TI;/*Transaction identifier 客户端决定 服务端回填 不停+1*/
    ushort PI;/*Protocol identifier 客户端决定 服务端回填*/
    ushort Len;/*LEN = PDU_LEN + 1; 前6个字节不算入长度，服务端组好报文后需要计算该值*/
    uchar  UI; /*Unit identifier 客户端决定 服务端回填*/
    uchar  MbusPdu[256];
    int    PduLen;
    QTcpSocket *tcpConn;
    TcpConnStat  ConnStatus;
    uchar  TimeOutFlag;
    ushort serial;
};
#pragma pack( pop )
class QTimer;
class TcpModbusMSG : public QObject
{
    Q_OBJECT

public:
    TcpModbusMSG(MBUSDATA *mbusData,QObject *parent=0);
    ~TcpModbusMSG();

    QTcpSocket *getTcpSocket(void);
    void connectToAddr(QHostAddress ipaddr,int port);
    void closeTcpConnect(void);
    int writeMSG(void);

signals:
    void dataReady(MBUSDATA *);

protected slots:
    void slot_run(void);
    void slot_conneted(void);
    void slot_disConnected(void);
    void slot_error( QAbstractSocket::SocketError socketError );
    void slot_readyRead(void);
    void slot_timerConnHook();

    void slot_connectToAddr(QHostAddress ipaddr,int port);
    void slot_closeTcpConnect(void);
    void slot_writeMSG(void);
signals:
    void s_connectToAddr(QHostAddress ,int );
    void s_closeTcpConnect();
    void s_writeMSG();


private:
    void mosbusMSGinit();
    MBUSDATA* m_mbusData;
    QTcpSocket *m_mbusTcpSoket;
    QTimer *m_timerConn;
};

#endif // MODBUSMSG_H
