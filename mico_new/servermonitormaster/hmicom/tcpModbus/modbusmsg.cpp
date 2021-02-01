
#include "modbusmsg.h"
//#include <QDebug>
#include <QTcpSocket>
#include <QTimer>

TcpModbusMSG::TcpModbusMSG( MBUSDATA *mbusData,QObject *parent)
    : QObject(parent)
{
    m_mbusData = mbusData;
    m_timerConn = NULL;
}

TcpModbusMSG::~TcpModbusMSG()
{
    if ( m_mbusTcpSoket != NULL )
    {        
        m_mbusTcpSoket->disconnectFromHost();
        m_mbusTcpSoket->close();        
        delete m_mbusTcpSoket;
        m_mbusTcpSoket = NULL;
    }
}

void TcpModbusMSG::slot_timerConnHook()
{
    m_timerConn->stop();
    if (m_mbusData != NULL /*&& ( m_mbusTcpSoket->state() != QAbstractSocket::ConnectedState )*/)
    {
		if (m_mbusTcpSoket->state() == QAbstractSocket::ConnectedState ||
			m_mbusTcpSoket->state() ==  QAbstractSocket::ConnectingState)
        {
            slot_closeTcpConnect();
        }
        else
        {
            m_mbusData->ConnStatus = TCP_MBUS_CONNECT_TIMEOUT;
        }
        m_mbusData->TimeOutFlag = 1;
        emit dataReady(m_mbusData);
    }
}

QTcpSocket* TcpModbusMSG::getTcpSocket()
{
    return m_mbusTcpSoket;
}

void TcpModbusMSG::connectToAddr(QHostAddress ipaddr, int port)
{
    emit s_connectToAddr(ipaddr,port);
}

void TcpModbusMSG::slot_connectToAddr(QHostAddress ipaddr, int port)
{
    /*连接不为空且还没建立*/
    //qDebug()<<"connectToAddr connect status "<<m_mbusTcpSoket->state();
    if ( m_mbusTcpSoket != NULL && 
        m_mbusTcpSoket->state() !=  QAbstractSocket::ConnectingState &&
        m_mbusTcpSoket->state() !=  QAbstractSocket::ConnectedState)
    {
        //qDebug()<<"connect to "<<ipaddr;
        m_mbusTcpSoket->connectToHost(ipaddr, port, QIODevice::ReadWrite);
        m_mbusData->ConnStatus = TCP_MBUS_CONNECT_LINKING;
       /* if(!m_mbusTcpSoket->waitForConnected(1000))
        {
            qDebug()<<"connectToAddr "<<ipaddr<<" timeout ";
            slot_timerConnHook();
        }*/
    }
    m_timerConn->start(1000); //不加此句某些系统下拔掉网线再插上恢复不了
}

void TcpModbusMSG::closeTcpConnect()
{
    emit s_closeTcpConnect();
}

void TcpModbusMSG::slot_closeTcpConnect()
{
    if ( m_mbusTcpSoket != NULL )
    {
        m_mbusTcpSoket->close();
        m_mbusData->tcpConn = NULL;
        m_mbusData->ConnStatus = TCP_MBUS_CONNECT_NULL;
    }
}

int TcpModbusMSG::writeMSG()
{
    emit s_writeMSG();
    return 1;
}

void TcpModbusMSG::slot_writeMSG()
{
    if ( m_mbusTcpSoket->state() == QAbstractSocket::ConnectedState )
    {
        /*for (int i=0; i<m_mbusData->PduLen;i++)
        {
            printf("%02X ", ((char*)m_mbusData)[i]);
        }
        printf("\n");*/
        int ret = m_mbusTcpSoket->write( (char*)m_mbusData, m_mbusData->PduLen );
        m_timerConn->start(3000);
        if (ret<0)
        {
            slot_closeTcpConnect();
        }
    }
    else
    {
        //qDebug()<<"TcpModbusMSG err connect stat"<<m_mbusTcpSoket->state();
        m_mbusData->ConnStatus = TCP_MBUS_CONNECT_NULL;
        m_mbusData->TimeOutFlag = 1;
        emit dataReady(m_mbusData);
        //return -1;
    }
}

void TcpModbusMSG::slot_conneted()
{
    m_timerConn->stop();
    m_mbusData->tcpConn = m_mbusTcpSoket;
    m_mbusData->ConnStatus = TCP_MBUS_CONNECT_IDLE;
    //writeMSG();
    m_mbusData->TimeOutFlag = 1;
    emit dataReady(m_mbusData);
    //qDebug()<<"slot_conneted ";
}

void TcpModbusMSG::slot_disConnected()
{
    m_mbusTcpSoket->close();
    m_mbusData->tcpConn = NULL;
    m_mbusData->ConnStatus = TCP_MBUS_CONNECT_NULL;
    //qDebug()<<"slot_disConnected";
    //m_mbusData->TimeOutFlag = 1;
    //emit dataReady(m_mbusData);
}

void TcpModbusMSG::slot_error( QAbstractSocket::SocketError socketError )
{
    //qDebug()<<"slot_error SocketError    "<<socketError;
    //qDebug()<<"slot_error connect status "<<m_mbusTcpSoket->state();

    if (m_timerConn->isActive())
    {
        m_timerConn->stop();
    }

    switch(m_mbusTcpSoket->error())
    {
    case QAbstractSocket::NetworkError:/*目标不可达*/
        m_mbusTcpSoket->close();
        m_mbusData->tcpConn = NULL;
        m_mbusData->ConnStatus = TCP_MBUS_CONNECT_NULL;
        break;
    case QAbstractSocket::ConnectionRefusedError:/*超时*/
        m_mbusTcpSoket->close();
        m_mbusData->tcpConn = NULL;
        m_mbusData->ConnStatus = TCP_MBUS_CONNECT_TIMEOUT;
        break;
    default:
        m_mbusTcpSoket->close();
        m_mbusData->tcpConn = NULL;
        m_mbusData->ConnStatus =TCP_MBUS_CONNECT_NULL;
        break;
    }

    m_mbusData->TimeOutFlag = 1;
    emit dataReady(m_mbusData);
}

void TcpModbusMSG::slot_readyRead()
{
    m_timerConn->stop();
    QByteArray buf = m_mbusTcpSoket->readAll();
    if ( buf.size() <= 256 )
    {
        //qDebug()<<"len "<<buf.size()<<" "<<buf.toHex();
        memcpy( (void *)m_mbusData, buf.data(), buf.size() );
        m_mbusData->PduLen = buf.size();
        int len = (m_mbusData->Len>>8 & 0x0ff);
        if ( len == m_mbusData->PduLen - 6 )
        {
            emit dataReady(m_mbusData);
        }
        else
        {
            //get a heartBeat packet
        }
    }
    else
    {
        //qDebug()<<"rcv buf too long"<<buf.size();
    }

}

void TcpModbusMSG::mosbusMSGinit()
{
    m_mbusTcpSoket = new QTcpSocket(this);
    connect(m_mbusTcpSoket, SIGNAL(connected()),this, SLOT(slot_conneted()));
    connect(m_mbusTcpSoket, SIGNAL(error(QAbstractSocket::SocketError)),
        this, SLOT(slot_error( QAbstractSocket::SocketError)));
    connect(m_mbusTcpSoket, SIGNAL(disconnected()),this, SLOT(slot_disConnected()));
    connect(m_mbusTcpSoket, SIGNAL(readyRead()),this, SLOT(slot_readyRead()));
    m_timerConn = new QTimer;
    connect(m_timerConn, SIGNAL(timeout()), this, SLOT(slot_timerConnHook()));

    connect(this, SIGNAL(s_connectToAddr(QHostAddress,int)), this, SLOT(slot_connectToAddr(QHostAddress,int)));
    connect(this, SIGNAL(s_writeMSG()), this, SLOT(slot_writeMSG()));
    connect(this, SIGNAL(s_closeTcpConnect()), this, SLOT(slot_closeTcpConnect()));
}

void TcpModbusMSG::slot_run()
{
    //exec();
    mosbusMSGinit();
}
