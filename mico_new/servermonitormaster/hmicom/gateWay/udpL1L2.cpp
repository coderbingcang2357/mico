#include "udpL1L2.h"

#include <QSettings>
#include <QDir>
#include <QMessageBox>
#include <QTime>
//#include "../app/Src/GuiView/MainGui/ProjectPage.h"
//#include "../app/Src/GuiView/MainGui/HomePage.h"
#include <QTextCodec>
#include "util/logwriter.h"

static const uint MAX_RETRYCOUNT_LOCAL = 3; //最大重试次数
static const uint MAX_RETRYCOUNT_REMOTE = 4;//最大重试次数

L2Socket::L2Socket( QObject *parent ): QObject( parent )
{
    m_udpSocket = NULL;
    m_curPduRef = 1;
    m_retryCount = 0;
    m_socketStat = S_UNINIT;
    m_lPort = BASE_LOCAL_PORT;
    m_maxRetryCount = MAX_RETRYCOUNT_LOCAL;

    m_connStr = "";
    m_timer = NULL;
    m_udp = NULL;

    //m_project = HomePage::getCurProject();
}

L2Socket::~L2Socket()
{
    closeConnect();
    if(m_timer)
        m_timer->deleteLater(); //lvyong
    if(m_udpSocket)
        m_udpSocket->deleteLater(); //lvyong
    if (m_udp != NULL)  //不加这个场景运行关闭时会死机
    {
        m_udp->disconnect();
    }
}

void L2Socket::slot_readData( const QByteArray &data )
{
    if ( data.size() < 5/* || m_socketStat != S_BUSY */)
    {
        qDebug()<<"m_socketStat is "<<m_socketStat<<", Not the S_BUSY stat, abandon this packet.";
        return;
    }
    ushort rcvPudRef = (UCHAR)data[1]<<8 | (UCHAR)data[2];

    if ( m_curPduRef == rcvPudRef &&
        0 == (UCHAR)data[3] &&
        data.length() > 5 /*&&
        S_BUSY == m_socketStat*/ )
    {
        struct UdpL7Msg l7Msg;

        m_timer->stop();
        l7Msg.data = data.mid(4);
        l7Msg.err = UDP_PPI_NOERR;
        l7Msg.connectFlg = m_connectFlg + QString::number(data.data()[data.length()-1]);
        emit socketReadyRead(l7Msg);
        m_socketStat = S_IDLE;
        m_sendList.removeAt(0);
        if ( m_curPduRef++ == 0 )
        {
            m_curPduRef = 1;
        }
        m_retryCount = 0;  //LSH_2017-8-18 发下一包前应该将重试次数清零，不然重试次数累加会有问题
        writeSocket();
    }
}

void L2Socket::slot_timeOut()
{
    //qDebug()<<"l2soc timeout "<<QThread::currentThread();
    m_timer->stop();
    m_retryCount++;
    m_socketStat = S_IDLE;

    //::writelog(InfoType,"UDP_PPI_ERR_TIMEROUT1,m_retryCount:%d,m_curPduRef:%d", m_retryCount, m_curPduRef);
             //<< QTime::currentTime().toString();
    if ( m_retryCount < m_maxRetryCount )
    {
        writeSocket();
    }
    else
    {
        struct UdpL7Msg l7Msg;
        l7Msg.data.resize(1);
        l7Msg.data = m_sendList.at(0).right(1);
        l7Msg.err = UDP_PPI_ERR_TIMEROUT;

        l7Msg.connectFlg = m_connectFlg + QString::number(m_sendList.at(0).data()[m_sendList.at(0).length()-1]);
        //m_L7MsgList->append(l7Msg);
        emit socketReadyRead(l7Msg);
        m_retryCount = 0;

        m_sendList.removeAt(0);
        if ( m_curPduRef++ == 0 )
        {
            m_curPduRef = 1;
        }
    }
}

//往socket写数据
void L2Socket::writeSocket()
{
    //quint64 devID = m_project->devID(m_connStr);
    //quint8 modeRemote = m_project->m_modeRemote;
    quint64 devID = 0;
    quint8 modeRemote = 0;

    QByteArray sndData;
    char ch;

    //if (modeRemote == MODE_LOCAL || devID == 0)  //本地场景监控
    if (false)  //本地场景监控
    {
        if(m_udpSocket == NULL)    //本地UDP_PPI而m_udpSocket又为空，则new一个
        {
            m_udpSocket = new UdpSocket(0);
            connect(m_udpSocket, SIGNAL(readyRead( const QByteArray &)),
                    this, SLOT(slot_readData(const QByteArray &)));
            if(m_timer == NULL)
            {
                m_timer = new QTimer;
                connect( m_timer, SIGNAL( timeout() ), this, SLOT( slot_timeOut() ) );
            }

            while ( !m_udpSocket->bind( m_lPort ) )
            {
                m_lPort++;
                if ( m_lPort == 65535 )
                {
                    qDebug()<<"there is no valid port for this socket";
                    m_lPort = BASE_LOCAL_PORT;
                }
            }
        }

        if(m_udpSocket != NULL)  //本地发送
        {
            if ( m_socketStat == S_IDLE
                && m_sendList.count() > 0 )
            {
                ch = 0x06;
                sndData = m_sendList.at(0);
                sndData.prepend(m_curPduRef);
                sndData.prepend(m_curPduRef>>8);
                sndData.prepend(ch);
                m_socketStat = S_BUSY;
//                qDebug()<<"1111111111111"<<sndData.toHex();
//                if(sndData.toHex().contains("313233343536"))
//                {
//                    qDebug()<<"2222222222222"<<sndData.toHex();
//               }
                m_udpSocket->writeDatagram( sndData, m_ipAddr, m_port );
                m_timer->start(2000*(m_retryCount+1));
            }
        }
    }
    else                           //远程场景监控
    {
        if(m_udpSocket != NULL)    //m_udpSocket是只给本地UDP_PPI用的，如果不为空先删掉它
        {
            delete m_udpSocket;
            m_udpSocket = NULL;
            if(m_timer)
            {
                delete m_timer;
                m_timer = NULL;
            }
        }

        //if(m_project->getUdp(devID))   //如果通道存在，发送数据
        if (true)
        {
            //if(m_udp != m_project->getUdp(devID)) //通道有更新
            //{
            //    m_udp = m_project->getUdp(devID);
            //    connect(m_udp, SIGNAL(readyRead( const QByteArray &)),
            //            this, SLOT(slot_readData(const QByteArray &)));
            //}

            if(m_timer == NULL)
            {
                m_timer = new QTimer;
                connect( m_timer, SIGNAL( timeout() ), this, SLOT( slot_timeOut() ) );
            }
            if ( m_socketStat == S_IDLE
                && m_sendList.count() > 0 )
            {
                ch = 0x06;
                sndData = m_sendList.at(0);
                sndData.prepend(m_curPduRef);
                sndData.prepend(m_curPduRef>>8);
                sndData.prepend(ch);
                m_socketStat = S_BUSY;

                m_udp->writedata(sndData);
                m_timer->start(4000*(m_retryCount+1));
            }
        }
        else                          //如果通道不存在
        {
            struct UdpL7Msg l7Msg;
            l7Msg.data.resize(1);
            l7Msg.data = m_sendList.at(0).right(1);
            l7Msg.err = UDP_PPI_ERR_TIMEROUT;
            qDebug() << "UDP_PPI_ERR_TIMEROUT2";
            l7Msg.connectFlg = m_connectFlg + QString::number(m_sendList.at(0).data()[m_sendList.at(0).length()-1]);
            emit socketReadyRead(l7Msg);
            m_retryCount = 0;

            m_sendList.removeAt(0);
            if ( m_curPduRef++ == 0 )
            {
                m_curPduRef = 1;
            }
        }
    }
}

//返回一个消息链表给上层
/*
QList<struct UdpL7Msg> L2Socket::readL7MsgList( struct UdpL7Msg &msg )
{
    QList<struct UdpL7Msg> tmp = m_L7MsgList;
    m_L7MsgList->clear();
    return tmp;
}
*/
//添加数据到发送链表尾部
void L2Socket::writeListAppend( const QByteArray &array )
{
    m_sendList.append(array);
    writeSocket();
}

void L2Socket::udpClient(QString ipAddr, ushort port, uint connectID)
{
    //if (m_project == NULL)
    //    return;
    //QString path = QDir::homePath()+"/Documents"+"/Projects/"+m_project->path();
    //QDir dir(path);

    //if(!dir.exists("connect.ini"))
    //    return;
    //QSettings conn(path + "/connect.ini", QSettings::IniFormat, 0 );
    //quint8 modeRemote = m_project->m_modeRemote; //lvyong

    //if (modeRemote == MODE_LOCAL)
    if (false)
        m_maxRetryCount = MAX_RETRYCOUNT_LOCAL;
    else
        m_maxRetryCount = MAX_RETRYCOUNT_REMOTE;

    m_connStr = QString("%1:%2").arg(ipAddr).arg(port);
    //if(conn.value(m_connStr).isValid() && modeRemote < MODE_LOCAL)
    if(true) // remote
    {
        // quint64 devID = conn.value(m_connStr).toLongLong();
        //quint64 devID = 0;//conn.value(m_connStr).toLongLong();
        //if(m_project->getUdp(devID))
        if(true)
        {
            if (m_udp == nullptr)
            {
                m_udp = static_cast<ServerUdp*>(ISock::get(connectID));
				qDebug()<<"connectID"<<connectID;
				qDebug()<<"m_udp:"<<m_udp;
                connect(m_udp, SIGNAL(readyRead( const QByteArray &)), this, SLOT(slot_readData(const QByteArray &)));
            }
            //if(m_udp != m_project->getUdp(devID))
            //{
            //    if(m_udp)
            //        disconnect(m_udp, SIGNAL(readyRead( const QByteArray &)), this, SLOT(slot_readData(const QByteArray &)));
            //    m_udp = m_project->getUdp(devID);
            //    connect(m_udp, SIGNAL(readyRead( const QByteArray &)), this, SLOT(slot_readData(const QByteArray &)));
            //}
            m_timer = new QTimer;
            connect( m_timer, SIGNAL( timeout() ), this, SLOT( slot_timeOut() ) );
        }
    }
    else
    {
        m_udpSocket = new UdpSocket(0);
        connect(m_udpSocket, SIGNAL(readyRead( const QByteArray &)), this, SLOT(slot_readData(const QByteArray &)));
        m_timer = new QTimer;
        connect( m_timer, SIGNAL( timeout() ), this, SLOT( slot_timeOut() ) );
        //qDebug()<<"new timer "<<QThread::currentThread();
        while ( !m_udpSocket->bind( /*ipAddr,*/ m_lPort ) )
        {
            m_lPort++;
            if ( m_lPort == 65535 )
            {
                qDebug()<<"there is no valid port for this socket";
                m_lPort = BASE_LOCAL_PORT;
            }
        }
    }
}

//初始化socket
bool L2Socket::creatConnect( QString ipAddr, ushort port, uint connectID )
{
    udpClient(ipAddr,port,connectID);// udpclient();
    m_ipAddr = QHostAddress(ipAddr);
    m_port = port;
    m_connectFlg = ipAddr + QString::number(port);
    m_socketStat = S_IDLE;
    return true;
}

//关闭socket
bool L2Socket::closeConnect()
{
    if(m_udpSocket)
        delete m_udpSocket;
    m_udpSocket = NULL;
    return true;
}

SocketStat L2Socket::getSocketStat()
{
    return m_socketStat;
}

////////////////////////////////////////////////////////////////////
UDPL1L2::UDPL1L2( QObject *parent ): QObject( parent )
{
    m_L7MsgList = new QList<struct UdpL7Msg>;
    m_udpSocketList = new QHash<QString, L2Socket *>;
//	m_timerList = new QHash<QTimer *, L2Socket>;
}

UDPL1L2::~UDPL1L2()
{
	//m_L7MsgList->clear();
	delete m_L7MsgList;
#if 0
    QHash<QString, L2Socket *>::const_iterator it =  m_udpSocketList->constBegin();
	while(it != m_udpSocketList->constEnd()){
		qDebug() << it.key();
		L2Socket *soc = it.value();
		if(NULL != soc){
			delete soc;
			//soc = NULL;
		}
		++it;
	}
	m_udpSocketList->clear();
#endif
	delete m_udpSocketList;
}

void UDPL1L2::slot_readData( struct UdpL7Msg &msg )
{
    m_L7MsgList->append(msg);
    emit readyRead();
}

bool UDPL1L2::readMsg( struct UdpL7Msg &msg )
{
    if ( m_L7MsgList->count()>0 )
    {
        msg = m_L7MsgList->at(0);
        m_L7MsgList->removeAt(0);
        return true;
    }
    return false;
}

void UDPL1L2::slot_writeMsg( QString ipAddr, int Port, int connectID, const QByteArray &array )
{
    QString str = ipAddr+QString::number(Port);
    L2Socket *l2socket = m_udpSocketList->value(str, NULL);

    if ( l2socket == NULL )
    {
        //还没有对应的socket 分配一个给这个ip和端口
        for ( int i=0; i<MAX_GATEWAYNUM; i++ )
        {
            if ( m_L2Socket[i].getSocketStat() == S_UNINIT )
            {
                //qDebug()<<"creatConnect ID: "<<connectID;
                m_L2Socket[i].creatConnect( ipAddr, Port, connectID );
                l2socket = &m_L2Socket[i];
                connect(l2socket, SIGNAL(socketReadyRead( struct UdpL7Msg &)), this, SLOT(slot_readData( struct UdpL7Msg & )));
                m_udpSocketList->insert(str, l2socket);
                break;
            }
        }
    }

    if ( l2socket != NULL )
    {
        l2socket->writeListAppend(array);
    }
}


