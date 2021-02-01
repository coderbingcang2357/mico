#ifndef UDPL1L2_H__
#define UDPL1L2_H__
#include "udpsocket.h"
#include <QTimer>
#include <QList>
#include "serverudp/serverudp.h"
#include "serverudp/isock.h"

const int MAX_GATEWAYNUM = 255; //LSH_2018-1-16 HMI3.8.2 UDP_PPI连接数放开到255，产品经理的邮件

class ProjectPage;
class PenetrateUdp;

struct UdpL7Msg
{
    QString connectFlg;
    QByteArray data;
    unsigned char err;
};

enum SocketStat
{
    S_IDLE = 1,//空闲
    S_BUSY,// 忙碌
    S_UNINIT//木有初始化
};

enum
{
    UDP_PPI_NOERR,
    UDP_PPI_ERR_TIMEROUT/*,
    UDP_COM_OPENERR,
    UDP_COM_OPENSUCCESS,
    UDP_PPI_BUILD_SD2_ERR,
    UDP_PPI_CONNECT_OK,
    UDP_PPI_CONNECT_FAIL*/
};
/////////////////////////////////////////////////////////
class L2Socket : public QObject
{
    Q_OBJECT

public:
    L2Socket( QObject *parent = 0 );
    ~L2Socket();

    //QList<struct UdpL7Msg> readL7MsgList();
    void writeListAppend( const QByteArray &array );
    bool creatConnect( QString ipAddr, ushort port, uint connectID );
    bool closeConnect( );
    SocketStat getSocketStat();

public slots:
    void slot_timeOut();
    void slot_readData( const QByteArray &data );

signals:
    void socketReadyRead( struct UdpL7Msg &msg );

private:
    void writeSocket();
    void udpClient(QString ipAddr, ushort port, uint connectID);

    QString m_connStr;
    UdpSocket *m_udpSocket; //对应的Socket
    SocketStat m_socketStat;//连接状态
    QList<QByteArray> m_sendList;//发送链表
    //QList<struct UdpL7Msg> *m_L7MsgList;
    QTimer *m_timer;//超时定时器
    uint m_retryCount; //重发次数
    QString m_connectFlg;
    QHostAddress m_ipAddr;
    ushort m_port;
    ushort m_curPduRef;
    //PenetrateUdp *m_udp;
    ServerUdp *m_udp;
    //ProjectPage  *m_project;
    int m_lPort;
    uint m_maxRetryCount; //最大重试次数
};
/////////////////////////////////////////////////////////
class UDPL1L2 : public QObject
{
    Q_OBJECT

public:
    UDPL1L2( QObject *parent = 0 );
    ~UDPL1L2();
    bool readMsg( struct UdpL7Msg &msg );


public slots:
    void slot_readData( struct UdpL7Msg &msg );
    void slot_writeMsg( QString ipAddr, int Port, int connectID, const QByteArray &array );


signals:
    void readyRead();

private:

    L2Socket m_L2Socket[MAX_GATEWAYNUM];
    QList<struct UdpL7Msg> *m_L7MsgList;
    QHash<QString, L2Socket *> *m_udpSocketList;

};
#endif
