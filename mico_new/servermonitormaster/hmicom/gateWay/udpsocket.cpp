#include <QStringList>
#ifndef  HMI_ANDROID_IOS
#include <windows.h>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#endif
#include "udpsocket.h"
#include "util/logwriter.h"
//#include "HintMsgPage.h"

//////////////////////////////////////////////////////////////////////////////
UdpSocket::UdpSocket( QObject *parent ) : QThread( parent )
{
#ifndef HMI_ANDROID_IOS
    WSADATA wsa;
    WSAStartup( MAKEWORD( 2, 2 ), &wsa );
    //AF_INET means use TCP/IP, SOCK_DGRAM means UDP, IPPROTO_UDP means user datagram protocol
    m_socket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    memset(m_revcBuf, 0, 4096);
#else
    m_socket = new QUdpSocket(this);
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)),this, SLOT(printError(QAbstractSocket::SocketError)));
    connect(m_socket, SIGNAL(readyRead()),this, SLOT(readPendingDatagrams()));
    m_udpTimer = new QTimer( this ); //此定时器是因为区工说QUdpSocket的bug，有时会收不到readyRead信号
    connect( m_udpTimer, SIGNAL( timeout() ), this, SLOT( readPendingDatagrams() ) );
    m_udpTimer->start(100);
#endif
    m_locPort = BASE_LOCAL_PORT;
}

UdpSocket::~UdpSocket()
{
#ifndef HMI_ANDROID_IOS
    //shutdown(m_socket, SD_BOTH);
    closesocket( m_socket );
    m_socket = INVALID_SOCKET;
    wait();
    WSACleanup();
#else
    delete m_socket;
#endif
}

bool UdpSocket::bind( qint16 bindPort )
{
#ifndef HMI_ANDROID_IOS
    SOCKADDR_IN addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons( bindPort );
    addr.sin_addr.s_addr = INADDR_ANY;

    if ( ::bind( m_socket, ( sockaddr * )&addr, sizeof( addr ) ) != 0 )
    {        
        return false;
    }

    m_locIP = "";
    m_locPort = bindPort;

    // 设置通信模式非阻塞
    u_long noblk = 1;
    ioctlsocket( m_socket, FIONBIO, &noblk );

    start();
    return true;
#else
    if (!m_socket->bind(QHostAddress(QHostAddress::Any), bindPort))
    {        
        return false;
    }

    m_locIP = "";
    m_locPort = bindPort;

    start();
    return true;
#endif
}

void UdpSocket::printError(QAbstractSocket::SocketError error)
{
#ifdef HMI_ANDROID_IOS
    //qDebug()<<"bind error:%d\n"<<m_socket->errorString();
#endif
}

bool UdpSocket::bind( const QString &bindIP, qint16 bindPort )
{
#ifndef HMI_ANDROID_IOS
    SOCKADDR_IN addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons( bindPort );

    QByteArray iplist;
    //ipAddressCode( bindIP, iplist );
    iplist = bindIP.toLatin1();
    addr.sin_addr.s_addr = *(( u_long FAR * )( iplist.constData() ) );

    if ( ::bind( m_socket, ( sockaddr * )&addr, sizeof( addr ) ) != 0 )
    {        
        return false;
    }

    m_locIP = bindIP;
    m_locPort = bindPort;

    // 设置通信模式非阻塞
    u_long noblk = 1;
    ioctlsocket( m_socket, FIONBIO, &noblk );
#else
    if (m_socket->bind(QHostAddress(bindIP), bindPort))
    {
        printError(m_socket->error());
        return false;
    }
    m_locIP = bindIP;
    m_locPort = bindPort;
#endif
    start();
    return true;
}

/*!
    发送数据data, address远程地址,port远程端口
 */
int UdpSocket::writeDatagram( const QByteArray & data, const QHostAddress &address, qint16 port )
{
#ifndef HMI_ANDROID_IOS
    SOCKADDR_IN addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons( port );
    addr.sin_addr.s_addr = htonl( address.toIPv4Address() );

    int sed = sendto( m_socket, data.data(), data.length(), 0, ( sockaddr * ) & addr, sizeof( addr ) );
    return sed;
#else
    qint64 sentLen = m_socket->writeDatagram(data,address,port);
    if(sentLen <= 0)
    {
        insureUsefulUdpSocket();
        sentLen = m_socket->writeDatagram(data,address,port);
    }
    return sentLen;
#endif
}

#ifdef HMI_ANDROID_IOS
void UdpSocket::insureUsefulUdpSocket()
{
    m_mutex.lock();
    //LSH_2016-7-28 此处存在内存泄漏，区工说是因为IOS黑屏时Socket会发送失败，
    //如果此处加上下面这句会导致软件立即crash，主要是针对IOS平台的处理
    //SAFE_DELETE(m_socket);
    QUdpSocket *lastSocket = m_socket;
    m_socket->disconnect();
    quint16 retryCount = 0;
    m_socket = NULL;
    while(m_socket == NULL && retryCount++ <3)
    {
        m_socket = new QUdpSocket;
        qWarning() << "new QUdpSocket retry count:" << retryCount;

        connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)),this, SLOT(printError(QAbstractSocket::SocketError)));
        connect(m_socket, SIGNAL(readyRead()),this, SLOT(readPendingDatagrams()));
        int localPort = m_locPort++;
        for (; localPort <= 65535; localPort++ )
        {
            if ( !m_socket->bind( localPort, QAbstractSocket::DontShareAddress) )
            {
                continue;
            }
            else
            {
                if(lastSocket)
                {
                    lastSocket->close();
                    delete lastSocket;
                    lastSocket = NULL;
                }
                m_locPort = localPort;
                qDebug() << "retry create socket:" << m_locPort;
                break;
            }
        }
    }
    m_mutex.unlock();
}
#endif

/*!
    等待接收数据,收到数据后发送readyRead(data)信号,data是收到的数据
 */
void UdpSocket::run()
{    
#ifndef HMI_ANDROID_IOS
    struct fd_set   fread;
    int             recvLen = 4096;
    int             addrLen;
    QByteArray      readData;
    SOCKADDR_IN     addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons( m_locPort );

    if ( m_locIP == "" )
    {
        addr.sin_addr.s_addr = INADDR_ANY;
    }
    else
    {
        QByteArray iplist;
        //ipAddressCode( m_locIP, iplist );
        iplist = m_locIP.toLatin1();
        addr.sin_addr.s_addr = *(( u_long FAR * )( iplist.constData() ) );
    }

    for ( ;; )
    {
        addrLen = sizeof( addr ); // 这个要初使化,否则收不到数据
        FD_ZERO( &fread );
        FD_SET( m_socket, &fread ); // 添加到读集合中

        if (select( 0, &fread, NULL, NULL, 0 ) == SOCKET_ERROR )
        {
            printf( "select error.%d\n", WSAGetLastError() );
            return;
        }

        if ( m_socket == INVALID_SOCKET )
            return;

        if ( FD_ISSET( m_socket, &fread ) )
        {
            if (recvLen >= 0) //LIUSHENGHONG_2013-1-26 不加此判断，recvLen=-1时下面清缓冲区会出错（可连本机上一个端口测试）
            {
            ::ZeroMemory( m_revcBuf, recvLen );
            }
            recvLen = recvfrom( m_socket, ( char * )m_revcBuf, 4096, 0, ( sockaddr * ) & addr, &addrLen );
            //printf("recv len:%d\n", recvLen);
            if ( recvLen != SOCKET_ERROR )
            {
                emit readyRead( QByteArray(( char * )m_revcBuf, recvLen ) );
            }
        }
    }
#else
//    for(;;)   //LIUSHENGHONG_2016-6-2  解决运行场景会卡的问题（进入了死循环）
//    {
//    }
#endif
}

#ifdef HMI_ANDROID_IOS
void UdpSocket::readPendingDatagrams()
{
    qint64          recvLen = 4096;
    QHostAddress host;
    quint16 port;

    while(m_socket->hasPendingDatagrams())
    {
        m_revcBuf.resize(m_socket->pendingDatagramSize());
        //readDatagram will return the sender host address and port used
        recvLen = m_socket->readDatagram(m_revcBuf.data(), m_revcBuf.size(), &host, &port);
        if(recvLen > 0)// && port == 20000)
        {
            emit readyRead(m_revcBuf);
        }
    }
}
#endif
