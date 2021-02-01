#ifndef UDPSOCKET_H__
#define UDPSOCKET_H__
#include <QObject>
#include <QHostAddress>
#include <QByteArray>
#ifndef  HMI_ANDROID_IOS
#include <WinSock.h>
#else
typedef unsigned char UCHAR;
/*#define TRUE true
#define FALSE false*/
#include <QUdpSocket>
#include <QTimer>
#include <QMutex>
#endif
#include <QThread>

const int BASE_LOCAL_PORT = 10000;
/*!
 处理udp通信
 */
class ProjectPage;
class UdpSocket : public QThread
{
    Q_OBJECT
public:
    UdpSocket( QObject *parent );
    ~UdpSocket();

    int writeDatagram( const QByteArray & data, const QHostAddress &address, qint16 port );
    bool bind( const QString &ip, qint16 port );
    bool bind( qint16 port );

protected:
    void run();       // 通信线程

signals:
    void readyRead( const QByteArray &data );  // data

private:
#ifndef HMI_ANDROID_IOS
    SOCKET m_socket;
    char m_revcBuf[4096];     //接收缓冲区
    qint16 m_locPort;          //本地端口
#else
    QUdpSocket *m_socket;
    QByteArray m_revcBuf;     //接收缓冲区
    quint16 m_locPort;          //本地端口
    QTimer* m_udpTimer;
    QMutex m_mutex;
#endif
    QString m_locIP;           //本地IP
public slots:
    void printError(QAbstractSocket::SocketError );
#ifdef HMI_ANDROID_IOS
    void readPendingDatagrams();
    void insureUsefulUdpSocket();
#endif
};
#endif // UDPSOCKET_H__
