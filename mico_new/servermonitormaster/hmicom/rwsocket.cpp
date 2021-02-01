#include <QTcpSocket>
#include <QThread>
#include <QtCore>
#include <QMutex>
#include <QSettings>
#include "rwsocket.h"
#include "serverudp/serverudp.h"
#include "serverudp/isock.h"
#include "util/logwriter.h"

//#include "ProjectPage.h"
//#include "HomePage.h"

#define MAX_RETRY 2
#define UDP_TIMEOUT 5000//2000//500
#define POLL_CNT 1

ushort g_localPort = 35350;/* WIFI 模块监听的端口号 */

const ushort FC_OPENDEV = 0x0001;    /* 功能码 打开设备 */
const ushort FC_WRITE   = 0x0002;    /* 功能码 写入设备 */
const ushort FC_READ    = 0x0003;    /* 功能码 读取设备 */
const ushort FC_HEARTBEAT   = 0x0004;    /* 功能码 心跳 */
const ushort FC_CLOSEDEV = 0x0005;    /* 功能码 关闭设备 */
/* highbyte of err code is version */
const ushort VERSION_MASK = 0xff00;

#include <QMessageBox>
RWSocket::RWSocket(ProtocolType proType, void *uartCfg, int connid, QObject *parent, HMI::HMIProject *p)
    : QObject(parent),remoteUdpAddr(QHostAddress(WIFI_IP_ADDR)),m_deviceVersion(1)
{
    //m_project = p;
    commomInit(proType);

    m_connid = connid;
	qDebug()<<"connidddddddddd:"<<connid;
    m_uartCfg = new struct UartCfg;
    m_netCfg = new struct NetCfg;
    //m_cacheMutex = new QMutex;
    memset(m_msgCache, 0, sizeof(struct MsgCache)*MSG_CACHE_CNT);

    qDebug()<<" sizeof UartCfg"<<sizeof(struct UartCfg);
    qDebug()<<" COMNum:"<<m_uartCfg->COMNum;
    qDebug()<<"   baud:"<<m_uartCfg->baud;
    qDebug()<<"dataBit:"<<m_uartCfg->dataBit;
    qDebug()<<"stopBit:"<<m_uartCfg->stopBit;
    qDebug()<<" parity:"<<m_uartCfg->parity;

    //m_project = HomePage::getCurProject();
    //connect(this,SIGNAL(stateChanged(quint16)),m_project,SLOT(slot_deviceStateChanged(quint16)));
    //QSettings conn(QDir::homePath()+"/Documents"+"/Projects/"+m_project->path()+"/connect.ini", QSettings::IniFormat, 0 );
    QString m_connStr;
    if(proType >= MBUS_TCP)
    {
        memcpy(m_netCfg, uartCfg, sizeof(struct NetCfg));
        memset(m_uartCfg, 0, sizeof(struct UartCfg));
        m_connStr = QString("%1:%2").arg(m_netCfg->ipAddr).arg(QString::number(m_netCfg->port));
    }
    else
    {
        memcpy(m_uartCfg, uartCfg, sizeof(struct UartCfg));
        m_connStr = QString("%1:%2").arg(WIFI_IP_ADDR).arg(WIFI_PORT+m_uartCfg->COMNum);
    }

    //QString macipport = conn.value(m_connStr+"new IP",WIFI_IP_ADDR).toString();
    //QStringList strs = macipport.split(":");
    //if (strs.size() > 2)
    //    strs.removeAt(0);

    //if (strs.size() > 0)
    //    remoteUdpAddr = QHostAddress(strs.at(0));
}

void RWSocket::commomInit(ProtocolType proType)
{    
    netState = 0;
   // timeOutCount = 0;
    m_timeoutPollCnt = 0;
    m_udp = NULL;

    qRegisterMetaType<QAbstractSocket::SocketError>("SocketError");
    qRegisterMetaType<QAbstractSocket::SocketState>("SocketState");

    m_proType = proType;

    m_socket = new QUdpSocket();
    //m_timeoutTimer = new QTimer;
    m_heartBeat = new QTimer;
    connect(m_heartBeat, SIGNAL(timeout()), this, SLOT(slot_heartBeat()));

    m_udpTimer = new QTimer( this );
    connect( m_udpTimer, SIGNAL( timeout() ), this, SLOT( slot_readData() ) );
    m_udpTimer->start(100);
    m_timeout = new QTimer;
    connect(m_timeout, SIGNAL(timeout()), this, SLOT(slot_checkTimeout()));
    m_timeout->start(UDP_TIMEOUT);
    m_timeoutCnt = 0;
    connect(m_socket, SIGNAL(connected()), this,
        SLOT(slot_connected()));
    connect(m_socket, SIGNAL(error( QAbstractSocket::SocketError )), this,
        SLOT(slot_error(QAbstractSocket::SocketError)));
    connect(m_socket, SIGNAL(stateChanged( QAbstractSocket::SocketState )), this,
        SLOT(slot_statChanged( QAbstractSocket::SocketState)));
    connect(m_socket, SIGNAL(readyRead()), this, SLOT(slot_readData()));
    m_readMsgList = new QList<union MsgPar>;

    switch (m_proType)
    {
    case MODBUS_RTU:
        m_msgLen = sizeof(MbusMsgPar);
        //qDebug()<<"size of MbusMsgPar"<<m_msgLen;
        break;
    case PPIMPI:
    case MPI300:
        m_msgLen = MAX_SHORT_BUF_LEN;//sizeof(MbusMsgPar);
        break;
    case HOSTLINK:
        m_msgLen = sizeof(HlinkMsgPar);
        break;
    case MIPROTOCOL4:
        m_msgLen = sizeof(MiProtocol4MsgPar);
        break;
    case MIPROFX:
        m_msgLen = sizeof(MiprofxMsgPar);
        break;
    case FINHOSTLINK:
        m_msgLen = sizeof(finHlinkMsgPar);
        break;
    case MEWTOCOL:
        m_msgLen = sizeof(MewtocolMsgPar);
        break;
    case DVP:
        m_msgLen = sizeof(DvpMsgPar);
        break;
    case FATEK:
        m_msgLen = sizeof(FatekMsgPar);
        break;
    case CNET:
        m_msgLen = sizeof(CnetMsgPar);
        break;
    case FREE_PORT:
        m_msgLen = sizeof(FpMsgPar);
        break;
    case MBUS_TCP:
        m_msgLen = sizeof(mbus_tcp_req_struct);
        break;
    case GATEWAY:
        break;
    default:
        break;
    }
}

RWSocket::~RWSocket()
{

    if(m_udpTimer){
        m_udpTimer->stop();
        delete m_udpTimer;
    }
    if(m_heartBeat){
        m_heartBeat->stop();
        delete m_heartBeat;
    }
    if(m_timeout)
    {
        m_timeout->stop();
        delete m_timeout;
    }

    struct ClientDataMsgHead dataHead;
    memset((char *)&dataHead, 0, sizeof(struct ClientDataMsgHead));

    dataHead.dataLen = 0;
    dataHead.errCode = 0;
    dataHead.fc = FC_CLOSEDEV;
    dataHead.port = 0;
    dataHead.proType = m_proType;
    dataHead.serialNum = ++m_serialNum;

    //quint8 modeRemote = MODE_LOCAL;

    //if(m_project)
    if(true)
    {
        //modeRemote = m_project->m_modeRemote;
        setRemoteUdp();

    }
    //if(modeRemote < MODE_LOCAL && m_udp)
    if(m_udp)
    {
        if(netState >= 2 )
        {
            QByteArray data;
            data.append((char *)&dataHead,sizeof(struct ClientDataMsgHead));
            m_udp->writedata(data);
        }
    }
    else
    {
        this->writeDatagramAssured((char *)&dataHead, sizeof(struct ClientDataMsgHead) ,remoteUdpAddr,WIFI_PORT);
    }

    if (m_udp != NULL)  //不加这个场景运行关闭时会死机
    {
        m_udp->disconnect();
    }

    if ( NULL != m_socket )
    {
        m_socket->close();
        delete m_socket;
        m_socket = NULL;
    }

    if(m_uartCfg)
    {
        delete m_uartCfg;
    }
    if(m_netCfg)
    {
        delete m_netCfg;
    }
	// add by wubc683 2021-01-18
	if(m_readMsgList)
	{
		delete m_readMsgList;
	}
}

int RWSocket::openDev()
{
    clearAllMsgCache();
    if ( m_socket->state() != QAbstractSocket::BoundState )
    {
        while ( !m_socket->bind(g_localPort,QAbstractSocket::DontShareAddress))
        {
            if ( ++g_localPort < 35350 )
            {
                g_localPort = 35350;
            }
        }
    }

    if ( !m_socket->isOpen() )
    {
        m_socket->open(QIODevice::ReadWrite);
    }

    slot_connected();
    return 1;
}

int RWSocket::packetWriteMsg(const ushort fc,char *wBuf, struct ClientDataMsgHead &dataHead, char *buf, int len)
{
    dataHead.fc = fc;
    dataHead.errCode = 0;
    dataHead.proType = ushort(m_proType);
    dataHead.dataLen = len;

    dataHead.serialNum = ++m_serialNum;

    switch (fc)
    {
    case FC_OPENDEV:
        if ( m_proType >= MBUS_TCP )
        {
            memcpy(dataHead.remoteIp, m_netCfg->ipAddr, 4 );
            dataHead.port = m_netCfg->port;
            memcpy(wBuf + sizeof(struct ClientDataMsgHead), (char*)m_netCfg, sizeof(struct NetCfg));
            dataHead.dataLen += sizeof(struct NetCfg);
        }
        else
        {
            //qDebug()<<"sizeof(m_uartCfg) "<<sizeof(struct UartCfg);
            memset(dataHead.remoteIp, 0, 4);
            dataHead.port = m_uartCfg->COMNum&0x1;
            UartCfg tempCfg;
            memcpy((char*)&tempCfg,(char*)m_uartCfg,sizeof(struct UartCfg));
            tempCfg.COMNum &= 0x1;
            memcpy(wBuf + sizeof(struct ClientDataMsgHead), (char*)&tempCfg, sizeof(struct UartCfg));
            dataHead.dataLen += sizeof(struct UartCfg);
        }
        memcpy(wBuf,(char *)&dataHead, sizeof(struct ClientDataMsgHead));
        break;
    case FC_READ:
        break;
    case FC_WRITE:
        if ( m_proType >= MBUS_TCP )
        {
            memcpy(dataHead.remoteIp, m_netCfg->ipAddr, 4 );
            dataHead.port = m_netCfg->port;
        }
        else
        {
            memset(dataHead.remoteIp, 0, 4);
            dataHead.port = m_uartCfg->COMNum&0x1;
        }
        memcpy(wBuf,(char *)&dataHead, sizeof(struct ClientDataMsgHead));
        //memcpy(wBuf + sizeof(struct ClientDataMsgHead), buf, len);
        if ( len > 1024)
        {
           // qDebug()<<"Change buf to version"<<m_deviceVersion<<len-1024;
            if(m_deviceVersion == 0 && (m_proType != PPIMPI && m_proType != MPI300))
            { /* short&int will align to even address, so size of struct maybe different from
                 what you calculate exactly*/
                memcpy(wBuf + sizeof(struct ClientDataMsgHead)+4, buf+8, len - 1024);
                *(wBuf + sizeof(struct ClientDataMsgHead)) = *buf;
                *((wBuf + sizeof(struct ClientDataMsgHead))+1) = *(buf+4);
                *((wBuf + sizeof(struct ClientDataMsgHead))+2) = *(buf+5);
                *((wBuf + sizeof(struct ClientDataMsgHead))+3) = *(buf+6);
            }else{
                memcpy(wBuf + sizeof(struct ClientDataMsgHead), buf, len - 1024);
            }
        }
        else
        {
           // qDebug()<<"Change buf to version"<<m_deviceVersion<<len;
            if(m_deviceVersion == 0 && (m_proType != PPIMPI && m_proType != MPI300))
            {
                memcpy(wBuf + sizeof(struct ClientDataMsgHead)+4, buf+8, len);
                *(wBuf + sizeof(struct ClientDataMsgHead)) = *buf;
                *((wBuf + sizeof(struct ClientDataMsgHead))+1) = *(buf+4);
                *((wBuf + sizeof(struct ClientDataMsgHead))+2) = *(buf+5);
                *((wBuf + sizeof(struct ClientDataMsgHead))+3) = *(buf+6);
            }else{
                memcpy(wBuf + sizeof(struct ClientDataMsgHead), buf, len);
            }
        }

        break;
    default:
        break;
    }

    if ( len > 1024)
    {
        return sizeof(struct ClientDataMsgHead) + dataHead.dataLen - 1024;
    }
    else
    {
        return sizeof(struct ClientDataMsgHead) + dataHead.dataLen;
    }
}

void RWSocket::setRemoteUdp()
{
    if (m_udp != nullptr)
        return;
    //QString m_connStr = QString("%1:%2").arg(WIFI_IP_ADDR).arg(WIFI_PORT+((m_uartCfg->COMNum<2)?0:m_uartCfg->COMNum));
    QString m_connStr;
    if(m_proType >= MBUS_TCP)
    {
        m_connStr = QString("%1:%2").arg(m_netCfg->ipAddr).arg(QString::number(m_netCfg->port));
    }
    else
    {
        m_connStr = QString("%1:%2").arg(WIFI_IP_ADDR).arg(WIFI_PORT+m_uartCfg->COMNum);
    }
    //QString m_connStr = QString("%1:%2").arg(WIFI_IP_ADDR).arg(WIFI_PORT+m_uartCfg->COMNum);
    //quint64 devID = m_project->devID(m_connStr);
    //if(devID == 0)
    //    return;
    //if(m_udp == m_project->getUdp(devID))
    //    return;
    //qDebug()<<"devID:"<<devID<<"udp OK";
    //if(m_udp)
    //{
    //    disconnect(m_udp, SIGNAL(readyRead( const QByteArray &)), this, SLOT(slot_readData(const QByteArray &)));
    //}
    //qDebug()<<"devID:"<<devID<<"udp OK end";
    //m_udp = m_project->getUdp(devID);
    //connect(m_udp, SIGNAL(readyRead( const QByteArray &)), this, SLOT(slot_readData(const QByteArray &)));
    m_udp = static_cast<ServerUdp*>(ISock::get(m_connid));
	qDebug()<<"2222222222222222222:m_udp,m_connid"<<m_udp<<" "<<m_connid;
	//if(!m_udp)
    connect(m_udp, SIGNAL(readyRead( const QByteArray &)), this, SLOT(slot_readData(const QByteArray &)));
}
//#include <QMessageBox>
int RWSocket::writeDev(char *buf, int len)
{
    int sndLen = 0;

    //qDebug()<<"writeDev connetStat "<<m_socket->state();
    if ( m_socket->state() != QAbstractSocket::BoundState )/* 连接没有连接 先建立连接 */
    {
        openDev();
        packetReadMsg(NULL, WIFI_MODE_MSG);
        return len;
    }

    //QString m_connStr = QString("%1:%2").arg(WIFI_IP_ADDR).arg(WIFI_PORT+((m_uartCfg->COMNum<2)?0:m_uartCfg->COMNum));
    QString m_connStr;
    if(m_proType >= MBUS_TCP)
    {
        m_connStr = QString("%1:%2").arg(m_netCfg->ipAddr).arg(QString::number(m_netCfg->port));
    }
    else
    {
        m_connStr = QString("%1:%2").arg(WIFI_IP_ADDR).arg(WIFI_PORT+m_uartCfg->COMNum);
    }
   //QString m_connStr = QString("%1:%2").arg(WIFI_IP_ADDR).arg(WIFI_PORT+m_uartCfg->COMNum);
    //quint64 devID = 0;
    //quint8 modeRemote = MODE_LOCAL;
    //qDebug()<<"writeDev connetStat "<<m_socket<<m_connStr;
    //if(m_project)
    if(true)
    {
        //devID = m_project->devID(m_connStr);
        //modeRemote = m_project->m_modeRemote;
        setRemoteUdp();
    }
    //if (devID && m_udp && modeRemote < MODE_LOCAL && netState <2)
    if (m_udp && netState <2)
    {
        netState ++;
        openDev();
        packetReadMsg(NULL, WIFI_MODE_MSG);
        return len;
    }

    char wBuf[2048];
    memset(wBuf, 0, 2048);
    struct ClientDataMsgHead dataHead;
    memset((char *)&dataHead, 0, sizeof(struct ClientDataMsgHead));

    sndLen = packetWriteMsg(FC_WRITE, wBuf, dataHead, buf, len);
    //qDebug()<<"write dev protype "<<dataHead.proType;

    int wLen=0;
    if(m_heartBeat->isActive())
    {
       m_heartBeat->stop();
    }
    //if(modeRemote < MODE_LOCAL)
    if(true)
    {
        if(m_udp)
        {
            //qDebug()<<"send remote data";
            QByteArray data(wBuf,sndLen);
            wLen = m_udp->writedata(data);
        }else{
            packetReadMsg(NULL, WIFI_MODE_MSG);
        }
    }
    else
    {
        //qDebug()<<"get connect setting"<<m_project;
        //Bug #9871 本地场景监控中途修改了连接的设备不会自动更改，便于打开多个实例使用同一场景监控不同设备的情况
//        QSettings conn(QDir::homePath()+"/Documents"+"/Projects/"+m_project->path()+"/connect.ini", QSettings::IniFormat, 0 );
//        //QString m_connStr = QString("%1:%2").arg(WIFI_IP_ADDR).arg(WIFI_PORT+((m_uartCfg->COMNum<2)?0:m_uartCfg->COMNum));
//        QString m_connStr = QString("%1:%2").arg(WIFI_IP_ADDR).arg(WIFI_PORT+m_uartCfg->COMNum);
//        QString ipport = conn.value(m_connStr+"new IP",WIFI_IP_ADDR).toString();
//        if(remoteUdpAddr.toString()!= ipport.left(ipport.lastIndexOf(":")))
//        {
//            qDebug()<<"udp address changed";
//            packetReadMsg(NULL, WIFI_MODE_MSG);
//            remoteUdpAddr = QHostAddress(ipport.left(ipport.lastIndexOf(":")));
//            openDev();
//            return len;
//        }
        if(remoteUdpAddr.toString().isEmpty())
        {
            packetReadMsg(NULL, WIFI_MODE_MSG);
            return len;
        }
        //qDebug()<<"write data to dev"<<m_socket<<remoteUdpAddr<<endl;
        wLen = this->writeDatagramAssured(wBuf,sndLen,remoteUdpAddr,WIFI_PORT);
    }
    if(!m_heartBeat->isActive())
    {
       // timeOutCount =0;
        m_heartBeat->start(UDP_TIMEOUT);
    }

    //qDebug()<<"udp write len:"<<wLen<<endl;
    //m_timeoutTimer->start(5000);
    if(wLen>0 /*&& wLen == sndLen */&& dataHead.proType != FREE_PORT )
    {
        struct MsgCache *msgCache = getAnIdleMsgCache();

        if(msgCache != NULL)
        {
            //qDebug()<<"<writeDev> msgCache"<<wLen<<endl;
            msgCache->len = wLen;
            msgCache->useFlg = 1;
            msgCache->retryCnt = MAX_RETRY;
            msgCache->pollCnt =  POLL_CNT + m_timeoutPollCnt;
            msgCache->serialNum = dataHead.serialNum;
            memcpy(msgCache->buf, wBuf, sndLen);
        }
        else if(dataHead.proType != FREE_PORT)
        {
            //qDebug()<<"<writeDev> msgCache == NULL "<<endl;
            ::writelog("<writeDev> msgCache == NULL ");
        }
    }
    else
    {
        //qDebug()<<"<writeDev> wlen sndLen "<<wLen<<" "<<sndLen;
        //::writelog(InfoType,"<writeDev> wlen:%d, sndLen:%d ",wLen,sndLen);
        packetErrMsg((MsgPar *)buf,VAR_TIMEOUT);
        packetReadMsg(buf, UART_MSG);
    }
/*
    printf("SEND LEN %d: ",sndLen - sizeof(struct ClientDataMsgHead) );

    for (int i=sizeof(struct ClientDataMsgHead); i<sndLen; i++)
    {
        printf("%02X ",wBuf[i]&0x0FF);
    }
    printf("\n");
*/
    //qDebug()<<"write DEV 2";
    return len;
}

int RWSocket::readDev(char *msgPar)
{
    if (m_readMsgList->count() > 0)
    {
        m_msgMutex.lock();
        MsgPar msgParTmp = m_readMsgList->takeAt(0);
        m_msgMutex.unlock();
        //qDebug()<<"sizeof(struct MsgPar) "<<sizeof(union MsgPar);
        memcpy(msgPar, (char*)&msgParTmp,m_msgLen);
        return m_msgLen/*sizeof(struct MbusMsgPar)*/;
    }
    else
    {
        return 0;
    }
}

void RWSocket::slot_connected()
{
#if 0
    qDebug()<<"connect to "<<WIFI_IP_ADDR<<" success!"<<QThread::currentThread();
#endif

    char wBuf[2048];
    struct ClientDataMsgHead dataHead;
    memset((char *)&dataHead, 0, sizeof(struct ClientDataMsgHead));

    if(m_heartBeat->isActive())
    {
       m_heartBeat->stop();
    }
    int sndLen = packetWriteMsg(FC_OPENDEV, wBuf, dataHead, NULL, 0);

#if 0
    qDebug()<<"send FC_OPENDEV remoteUdpAddr"<<remoteUdpAddr;
#endif
    QString m_connStr;
    if(m_proType >= MBUS_TCP)
    {
        m_connStr = QString("%1:%2").arg(m_netCfg->ipAddr).arg(QString::number(m_netCfg->port));
    }
    else
    {
        m_connStr = QString("%1:%2").arg(WIFI_IP_ADDR).arg(WIFI_PORT+m_uartCfg->COMNum);
    }
    //QString m_connStr = QString("%1:%2").arg(WIFI_IP_ADDR).arg(WIFI_PORT+((m_uartCfg->COMNum<2)?0:m_uartCfg->COMNum));
    //QString m_connStr = QString("%1:%2").arg(WIFI_IP_ADDR).arg(WIFI_PORT+m_uartCfg->COMNum);
    //quint64 devID = 0;
    //quint8 modeRemote = MODE_LOCAL;
    //if(m_project)
    if(true)
    {
        //devID = m_project->devID(m_connStr);
        //modeRemote = m_project->m_modeRemote;
        setRemoteUdp();
    }
    // local
    //if(!remoteUdpAddr.toString().isEmpty() && modeRemote == MODE_LOCAL){
    //    this->writeDatagramAssured(wBuf,sndLen,remoteUdpAddr,WIFI_PORT);
    //}
    //if(devID && modeRemote < MODE_LOCAL)
    //if(devID) //remote
    if(true) //remote
    {
        if(m_udp)
        {
            QByteArray data(wBuf,sndLen);
            m_udp->writedata(data);
        }
    }

    if (!m_heartBeat->isActive())
    {
       // timeOutCount =0;
        m_heartBeat->start(UDP_TIMEOUT);
        //qDebug("h s");
    }
    //qDebug()<<"OPEN DEV";
}

void RWSocket::slot_error(QAbstractSocket::SocketError err)
{
    qDebug()<<"connect err "<<err;
    packetReadMsg(NULL, WIFI_MODE_MSG);
}

void RWSocket::slot_statChanged( QAbstractSocket::SocketState state)
{
    qDebug()<<"connect changeed "<<state;
}

void RWSocket::slot_readData(const QByteArray& dataIn)
{
    QByteArray data = dataIn;
    ClientDataMsgHead *msgHead = (ClientDataMsgHead *)data.data();
    //quint8 modeRemote = MODE_LOCAL;
    ushort errCode = 0;
    //if(m_project)
    if(true)
    {
        //modeRemote = m_project->m_modeRemote;
    }
    int readLen = 0;
    int rcvLen = 0;
    //if(m_udp && modeRemote < MODE_LOCAL)
    if(m_udp)
    {
        readLen = data.length();
        //qDebug()<<"readLen:"<<readLen;

        if (readLen < sizeof(struct ClientDataMsgHead))
        {
            return;
        }
        else
        {
            if ( msgHead->dataLen > 1024 )
            {
                msgHead->dataLen -= 1024;
            }

            rcvLen = readLen - sizeof(struct ClientDataMsgHead);
            // 当组态多个 连接的时候 会同时发多个udp包出去 这时候回过来的 serialnum就可能不是最新的了 固不验证 serialNum
            /*if ( m_serialNum != msgHead->serialNum && msgHead->proType != PPIMPI && msgHead->proType != MPI300 )
            {
                qDebug()<<"serialNum err m_serial:"<<m_serialNum<<" msgHead->serialNum "<<msgHead->serialNum;
                return;
            }*/

            /*if(msgHead->errCode&VERSION_MASK)
                m_deviceVersion = (msgHead->errCode&VERSION_MASK)>>8;*/

            netState = 3;
            errCode = msgHead->errCode & (~VERSION_MASK);
            if ( rcvLen < msgHead->dataLen || errCode != CONN_NO_ERR )
            {
#if 0
                qDebug()<<"udp conn rcv errCode from remote path"<<msgHead->errCode;
#endif
                if(errCode != UART_OPEN_SUCCESS )
                {
                    if( errCode == UART_NOT_OPEN )
                    {
                        //m_project->setTitle(tr("error"));
                        openDev();
                        emit stateChanged((m_uartCfg->COMNum<<8)+1);
                    }
                    packetReadMsg(NULL, WIFI_MODE_MSG);
                }else {
                    qDebug()<<"Open dev successfully remote: "<<remoteUdpAddr.toString();
                    if(msgHead->errCode&VERSION_MASK)
                        m_deviceVersion = (msgHead->errCode&VERSION_MASK)>>8;
                   // if(m_proType == PPIMPI || m_proType == MPI300)
                   // {
                        packetReadMsg(NULL, WIFI_MODE_MSG);
                   // }
                    emit stateChanged((m_uartCfg->COMNum<<8)+2);
                }
            }
            else
            {
                //mbus_tcp_req_struct *mbus = ( mbus_tcp_req_struct *)(data.data() + sizeof(struct ClientDataMsgHead));
                //qDebug("get data %s sn %d",mbus->IP,  msgHead->serialNum);
                //qDebug()<<"udp conn state 2";
                emit stateChanged((m_uartCfg->COMNum<<8)+3);
                MsgCache *msgCache = getMsgCacheBySerialNum(msgHead->serialNum);

                if (msgCache != NULL && msgCache->useFlg == 1)
                {
                    clearMsgCache(msgCache);
                }                
                else if(m_proType != FREE_PORT)
                {
                    qDebug()<<"can find sn "<<msgHead->serialNum<<msgCache;
                    return;
                }
                //qDebug()<<"received one package from PLC"<<data.length()<<sizeof(struct ClientDataMsgHead);
                packetReadMsg(data.data() + sizeof(struct ClientDataMsgHead), UART_MSG);
               // timeOutCount = 0;
                m_timeoutCnt = 0;
            }

           // m_timeoutTimer->stop();
            return;
        }
    }
}

void RWSocket::slot_readData()
{
    char buf[2048];
    memset(buf, 0, 2048);
    int readLen = 0;
    int rcvLen = 0;
    ushort errCode = 0;

    ClientDataMsgHead *msgHead = (ClientDataMsgHead *)buf;;

    if (m_socket->hasPendingDatagrams())
    {
        readLen = m_socket->readDatagram(buf, 2048);
        //qDebug()<<"readLen:"<<readLen;

        /*for (int i=0; i<readLen-sizeof(struct ClientDataMsgHead); i++)
        {
            printf("%02X ",(buf + sizeof(struct ClientDataMsgHead))[i]);
        }
        printf("\n");*/

        if (readLen <= 0)
        {
            return;
        }
        else
        {
            if ( msgHead->dataLen > 1024 )
            {
                msgHead->dataLen -= 1024;
            }

            rcvLen = readLen - sizeof(struct ClientDataMsgHead);
            // 当组态多个 连接的时候 会同时发多个udp包出去 这时候回过来的 serialnum就可能不是最新的了 固不验证 serialNum
            /*if ( m_serialNum != msgHead->serialNum && msgHead->proType != PPIMPI && msgHead->proType != MPI300 )
            {
                qDebug()<<"serialNum err m_serial:"<<m_serialNum<<" msgHead->serialNum "<<msgHead->serialNum;
                return;
            }*/

            /*if(msgHead->errCode&VERSION_MASK)
                m_deviceVersion = (msgHead->errCode&VERSION_MASK)>>8;*/

            netState = 3;
            errCode = msgHead->errCode & (~VERSION_MASK);
            if ( rcvLen < msgHead->dataLen || errCode != CONN_NO_ERR )
            {
                qDebug()<<"udp conn rcv errCode "<<msgHead->errCode;

                if( errCode != UART_OPEN_SUCCESS )
                {
                    if( errCode == UART_NOT_OPEN )
                    {
                        //m_project->setTitle(tr("error"));
                        openDev();
                    }

                    emit stateChanged((m_uartCfg->COMNum<<8)+1);
                    packetReadMsg(NULL, WIFI_MODE_MSG);
                }else{
                    qDebug()<<"Open dev successfully: "<<remoteUdpAddr.toString();
                    if(msgHead->errCode&VERSION_MASK)
                        m_deviceVersion = (msgHead->errCode&VERSION_MASK)>>8;
                    if(m_proType == PPIMPI  || m_proType == MPI300)
                    {
                        packetReadMsg(NULL, WIFI_MODE_MSG);
                    }
                    emit stateChanged((m_uartCfg->COMNum<<8)+2);
                }
            }
            else
            {
                //qDebug()<<"udp conn state 2";
                emit stateChanged((m_uartCfg->COMNum<<8)+3);
                MsgCache *msgCache = getMsgCacheBySerialNum(msgHead->serialNum);

                if (msgCache != NULL && msgCache->useFlg == 1)
                {
                    packetReadMsg(buf + sizeof(struct ClientDataMsgHead), UART_MSG);
                    m_timeoutCnt = 0;
                    clearMsgCache(msgCache);
                }
                else if(m_proType != FREE_PORT)
                {
                    return;
                }
            }

            //m_timeoutTimer->stop();
            return;
        }
    }
}

void RWSocket:: packetErrMsg(MsgPar *msg, char err)
{
    switch (m_proType)
    {
    case MODBUS_RTU:
        msg->mbusMsg.err = err;
        break;
    case PPIMPI:
        if(msg->L2Msg.Da != 0)
        {
            char da = msg->L2Msg.Da;
            memset(msg,0,m_msgLen);
            msg->L7Msg.MsgType = 7;
            msg->L7Msg.Saddr = da;
            msg->L7Msg.Err = err;
            qDebug("packetErrMsg da %d",da);
        }
        else
        {
           // clearAllMsgCache();
            msg->L7Msg.Err = err;
            //qDebug("packetErrMsg err da %d",msg->L2Msg.Da);
        }
        /*if (msg->L7Msg.MsgType != 1 && msg->L7Msg.MsgType != 4 && msg->L7Msg.MsgType != 7 )
        {
            clearAllMsgCache();
            msg->L7Msg.Err = COM_OPEN_ERR;
        }*/
        break;
    case HOSTLINK:
        msg->hlinkMsg.err = err;
        break;
    case MIPROTOCOL4:
        msg->mip4Msg.err = err;
        break;
    case MIPROFX:
        msg->mipfxMsg.err = err;
        break;
    case FINHOSTLINK:
        msg->finMsg.err = err;
        break;
    case MEWTOCOL:
        msg->mewMsg.err = err;
        break;
    case DVP:
        msg->dvpMsg.err = err;
        break;
    case FATEK:
        msg->fatekMsg.err = err;
        break;
    case CNET:
        msg->cnetMsg.err = err;
        break;
    case MPI300:
        msg->L7Msg.Err = err;
        msg->L7Msg.Saddr = msg->L2Msg.Da;

        /*if (msg->L7Msg.MsgType != 1 && msg->L7Msg.MsgType != 4 && msg->L7Msg.MsgType != 7 )
        {
            clearAllMsgCache();
            msg->L7Msg.Err = COM_OPEN_ERR;
        }*/
        break;
    case FREE_PORT:
        msg->fpMsg.err = err;
        break;
    case MBUS_TCP:
        memset(msg, 0, sizeof(mbus_tcp_req_struct));
        msg->tcpMbusMsg.Error = VAR_TIMEOUT;
        msg->tcpMbusMsg.Done = 1;
        break;
    case GATEWAY:
        break;
    default:
        break;
    }
}

void RWSocket::packetReadMsg(char *buf, ConnectMsgType msgType)
{
    MsgPar msg;

    if (buf == NULL && msgType == UART_MSG )
    {
        //qDebug()<<"packetMbusRTUMsg buf == NULL";
        return;
    }

    switch (msgType)
    {
    case UART_MSG:
        if(m_deviceVersion==0 && (m_proType != PPIMPI && m_proType != MPI300))
        {
            qDebug()<<"pack data from version"<<m_deviceVersion<<m_msgLen;
            memcpy((char*)&msg+8, buf+4, m_msgLen-8);
            *(char *)&msg = *buf;
            *((char *)&msg+4) = *(buf+1);
            *((char *)&msg+5) = *(buf+2);
            *((char *)&msg+6) = *(buf+3);
        }else{
            memcpy(&msg, buf, m_msgLen);

           /* QString printstr;
            for(int j = 0; j<m_msgLen ; j++)
            {
                printstr += QString("%1 ").arg((uchar)buf[j],0,16);
            }
            qDebug()<<m_msgLen<<"received from remote PLC: " << printstr;*/
        }
        break;
    case WIFI_MODE_MSG:
        packetErrMsg(&msg, COM_OPEN_ERR);
        break;
    default:
        break;
    }
    m_msgMutex.lock();
    m_readMsgList->append(msg);
    m_msgMutex.unlock();
    emit readyRead();
    //qDebug()<<"readyRead";
}

void RWSocket::slot_heartBeat()
{
    struct ClientDataMsgHead dataHead;
    memset((char *)&dataHead, 0, sizeof(struct ClientDataMsgHead));
    dataHead.fc = FC_HEARTBEAT;
    dataHead.errCode = 0;
    dataHead.proType = m_proType;
    //qDebug()<<"beat heart data"<<FC_HEARTBEAT;
    //quint8 modeRemote = MODE_LOCAL;
    //if(m_project)
    if(true)
    {
        //modeRemote = m_project->m_modeRemote;

        setRemoteUdp();
    }
    if(m_udp)
    {
        if(netState < 2 )
        {
            netState ++;
            openDev();
            packetReadMsg(NULL, WIFI_MODE_MSG);
            return;
        }
        QByteArray data;
        data.append((char *)&dataHead,sizeof(struct ClientDataMsgHead));
        //qDebug()<<"111111111111"<<data.toHex()<<sizeof(struct ClientDataMsgHead)<<data.length();

        m_udp->writedata(data);
    }
    else
    {
        this->writeDatagramAssured((char *)&dataHead,sizeof(struct ClientDataMsgHead),remoteUdpAddr,WIFI_PORT);
        //m_project->setTitle(tr("%1").arg(netState++));
    }
}

struct MsgCache * RWSocket::getAnIdleMsgCache()
{
    int i=0;
    m_cacheMutex.lock();
    for ( i=0; i<MSG_CACHE_CNT; i++ )
    {
        if ( m_msgCache[i].useFlg == 0 )
        {
            m_cacheMutex.unlock();
            return &m_msgCache[i];
        }
    }
    m_cacheMutex.unlock();
    return NULL;
}
struct MsgCache * RWSocket::getMsgCacheBySerialNum(ushort serialNum)
{
    int i=0;
    m_cacheMutex.lock();
    for ( i=0; i<MSG_CACHE_CNT; i++ )
    {
        //qDebug()<<"get sn: "<<m_msgCache[i].serialNum << serialNum;
        if ( m_msgCache[i].serialNum == serialNum )
        {
            m_cacheMutex.unlock();
            return &m_msgCache[i];
        }
    }
    m_cacheMutex.unlock();
    return NULL;
}
void RWSocket::clearMsgCache(struct MsgCache *msgCache)
{
    m_cacheMutex.lock();
    memset(msgCache, 0, sizeof(struct MsgCache));
    m_cacheMutex.unlock();
}

void RWSocket::clearAllMsgCache()
{
    int i;
    m_cacheMutex.lock();
    for ( i=0; i<MSG_CACHE_CNT; i++ )
    {
         if ( m_msgCache[i].useFlg > 0 )
         {
             packetErrMsg((MsgPar *)(m_msgCache[i].buf + sizeof(struct ClientDataMsgHead)),VAR_TIMEOUT);
             packetReadMsg(m_msgCache[i].buf+ sizeof(struct ClientDataMsgHead), UART_MSG);
         }
         memset(&m_msgCache[i], 0, sizeof(struct MsgCache));
    }
    m_cacheMutex.unlock();
}
//#include <QTime>
void RWSocket::slot_checkTimeout()
{
    int i=0;
    //quint8 modeRemote = MODE_LOCAL;
    //if(m_project)
    if(true)
    {
        //modeRemote = m_project->m_modeRemote;
        setRemoteUdp();
    }

    //qDebug()<<"<checkTimeout> "<<QTime::currentTime();
    m_cacheMutex.lock();
    for ( i=0; i<MSG_CACHE_CNT; i++ )
    {
        if ( m_msgCache[i].useFlg > 0 &&  --m_msgCache[i].pollCnt <= 0)
        {
            //m_msgCache[i].pollCnt--;
            if ( m_msgCache[i].retryCnt-- > 0  )
            {
                /* send again*/
                //L2_MASTER_MSG_STRU *l2msg =  (L2_MASTER_MSG_STRU*)(m_msgCache[i].buf+sizeof(struct ClientDataMsgHead));
                //qDebug()<<"send again "<<i<<" retrycnt "<<m_msgCache[i].retryCnt
                       //<<"  modeRemote "<<modeRemote<<" m_udp "<<m_udp<<" da "<<l2msg->Da<<" poll cnt "<<POLL_CNT + m_timeoutPollCnt;

                m_msgCache[i].pollCnt = POLL_CNT + m_timeoutPollCnt;
                //if(modeRemote < MODE_LOCAL && m_udp)
                if(true)
                {
					//qDebug()<<"m_proType:"<<m_proType;
                    if(m_proType == MBUS_TCP)
                    {
                        struct ClientDataMsgHead *dataHead = (struct ClientDataMsgHead *)(m_msgCache[i].buf);

                        ushort oldSn = dataHead->serialNum;
                        dataHead->serialNum = ++m_serialNum;
                        m_msgCache[i].serialNum = dataHead->serialNum;
                        mbus_tcp_req_struct *mbus = ( mbus_tcp_req_struct *)(m_msgCache[i].buf + sizeof(struct ClientDataMsgHead));
                        //qDebug("send retry %s %d sn %d %d",mbus->IP, m_msgCache[i].retryCnt, oldSn, dataHead->serialNum);
                    }
                    QByteArray data(m_msgCache[i].buf,m_msgCache[i].len);
                    if(m_udp->writedata(data)<=0)
                    {
                        qDebug()<<"<checkTimeout> m_socket->writeDatagram <= 0";
                    }

                }else
                {
                    qDebug()<<"send retry"<<m_msgCache[i].len<<" "<<remoteUdpAddr<<" "<<WIFI_PORT<<m_msgCache[i].retryCnt;
                    if (this->writeDatagramAssured( m_msgCache[i].buf, m_msgCache[i].len,remoteUdpAddr,WIFI_PORT) <= 0)
                    {
                        qDebug()<<"<checkTimeout> m_socket->writeDatagram <= 0";
                    }
                }
            }
            else
            {
                /*if(netState<3)
                {
                    openDev();
                }*/
                /*return timeout msg to up layer*/
                if(m_proType != MBUS_TCP)
                {
                    m_timeoutCnt++;
                }
                qDebug()<<"return timeout msg to up layer "<<i<<"netstate:"<<netState;
                packetErrMsg((MsgPar *)(m_msgCache[i].buf + sizeof(struct ClientDataMsgHead)),VAR_TIMEOUT);
                packetReadMsg(m_msgCache[i].buf+ sizeof(struct ClientDataMsgHead), UART_MSG);
                //clearMsgCache(&m_msgCache[i]);
                memset(&m_msgCache[i], 0, sizeof(struct MsgCache));
            }
            //break;
        }
    }
    m_cacheMutex.unlock();

    /****************for remote****************/
    if(m_timeoutCnt > (POLL_CNT + m_timeoutPollCnt))
    {
        m_timeoutCnt = 0;
        if(netState<=2)
        {
            emit stateChanged((m_uartCfg->COMNum<<8));
        }else {
            netState = 2;
            // qDebug("##########");
            openDev();
            // qDebug(">>>>>>>>>>");
            emit stateChanged((m_uartCfg->COMNum<<8)+1);
        }
        packetReadMsg(NULL, WIFI_MODE_MSG);
    }
    /********************************/
}

void RWSocket::setTimeoutPollCnt(int cnt)                /*set timeout poll cnt*/
{
    if(cnt > 0)
    {
        m_timeoutPollCnt = 1;//cnt;
        m_timeout->stop();
        m_timeout->start(UDP_TIMEOUT + cnt*1000);
        //qDebug()<<"m_timeout set: "<<(UDP_TIMEOUT + cnt*1000);
    }else
    {
        m_timeoutPollCnt = 0;
    }
}

qint64 RWSocket::writeDatagramAssured(const char *data, qint64 len, const QHostAddress &host, quint16 port)
{
    qint64 sentLen = m_socket->writeDatagram(data, len, host, port);
    if(sentLen <= 0)
    {
        insureUsefulUdpSocket();
        sentLen = m_socket->writeDatagram(data, len, host, port);
    }
    return sentLen;
}

void RWSocket::insureUsefulUdpSocket()
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
        connect(m_socket, SIGNAL(connected()), this, SLOT(slot_connected()));
        connect(m_socket, SIGNAL(error( QAbstractSocket::SocketError )), this,
                SLOT(slot_error(QAbstractSocket::SocketError)));
        connect(m_socket, SIGNAL(stateChanged( QAbstractSocket::SocketState )), this,
                SLOT(slot_statChanged( QAbstractSocket::SocketState)));
        connect(m_socket, SIGNAL(readyRead()), this, SLOT(slot_readData()));

        if(lastSocket)
        {
            lastSocket->close();
            delete lastSocket;
            lastSocket = NULL;
        }
    }
    m_mutex.unlock();
}
