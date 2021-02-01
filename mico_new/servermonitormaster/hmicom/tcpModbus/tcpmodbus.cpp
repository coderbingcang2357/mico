#include "../rwsocket.h"
#include "tcpmodbus.h"
//#include "HomePage.h"
//#include "ProjectPage.h"

#include <QThread>
#include <QMetaType>
#include <QTimer>
using namespace std;

typedef uchar   uchar;
typedef ushort  ushort;
typedef uint    uint;

const int TCP_MBUS_READ  = 0;
const int TCP_MBUS_WRITE = 1;
const uchar MBitMaskTable[] = {0x00, 0x01, 0x03, 0x07, 0x0f, 0x1F, 0x3F, 0x7F, 0xFF};
const MbusMsgErrCode errCoreMap[10] = {TCP_MBUS_NO_ERROR, TCP_MBUS_TIMEOUT_ERROR, TCP_MBUS_REQUEST_ERROR,
                                       TCP_MBUS_TIMEOUT_ERROR , TCP_MBUS_TIMEOUT_ERROR, TCP_MBUS_TIMEOUT_ERROR,
                                       TCP_MBUS_TIMEOUT_ERROR, TCP_MBUS_TIMEOUT_ERROR, TCP_MBUS_TIMEOUT_ERROR,
                                       TCP_MBUS_TIMEOUT_ERROR};

ushort GetBE2U(ushort X)
{
    return ((X>>8) & 0x00FF) | ((X<<8) & 0xFF00);
}

tcpModbus::tcpModbus(QObject *parent)
    : QObject(parent)
{
    m_remoteFlgHash  = new QHash<QHostAddress, ConnectType>;

    m_localDevIPHash = new QHash<QHostAddress, QString>;
    m_localMbusMsgHash = new QHash<QHostAddress, MBUSMSG*>;
    m_localRwSockHash = new QHash<QString, RWSocket*>;

    m_remoteDevIDHash = new QHash<QHostAddress, quint64>;
    m_remoteRwSockHash = new QHash<quint64, RWSocket*>;
    m_remoteMbusMsgHash = new QHash<QHostAddress, MBUSMSG*>;

    qRegisterMetaType<QHostAddress>( "QHostAddress" );

    m_ActiveMbusData = new QHash<MBUSDATA*,QHostAddress>;
    m_ActiveMbusReq  = new QHash<MBUSDATA*,MBUSMSG*>;
    m_ConnListHash   = new QHash<MBUSDATA*, TcpModbusMSG*>;



    for (int i=0; i<MAX_CONN_NUM; i++)
    {
        memset(&m_Data[i], 0, sizeof(struct MBUSDATA));
        m_tcpMbusConn[i] = new TcpModbusMSG(&m_Data[i], this);
        //下面这段其实没有用，moveToThread的文档里有说明，如果对象有parent,moveToThread是无效——
        //实际测试也是这样，以前的问题是每次启动测试都会多出32个线程，且不会释放，实际上这些线程也没用到
//        QThread *thread = new QThread;
//        m_tcpMbusConn[i]->moveToThread(thread);
//        connect(thread,SIGNAL(started()),m_tcpMbusConn[i],SLOT(slot_run()));
//        thread->start();
        QTimer::singleShot(10, m_tcpMbusConn[i],SLOT(slot_run()));
        connect(m_tcpMbusConn[i], SIGNAL(dataReady(MBUSDATA *)), this, SLOT(slot_getDate(MBUSDATA *)));
        m_ConnListHash->insert(&m_Data[i], m_tcpMbusConn[i]);
    }
}

tcpModbus::~tcpModbus()
{    
    for (int i=0; i<MAX_CONN_NUM; i++)
    {
        if(m_tcpMbusConn[i])
        {
            m_tcpMbusConn[i]->disconnect();  //不加这个进入设备管理时可能死机
            delete m_tcpMbusConn[i];
        }
    }

    if(m_ActiveMbusData)
    {
        delete m_ActiveMbusData;
    }
    if(m_ActiveMbusReq)
    {
        delete m_ActiveMbusReq;
    }
    if(m_ConnListHash)
    {
        delete m_ConnListHash;
    }
}

void tcpModbus::endiandChg(uchar *dataPtr, int len)
{
    uchar tmp=0;
    for (int i=0; i<len;i+=2)
    {
        tmp = dataPtr[i];
        dataPtr[i]   = dataPtr[i+1] ;
        dataPtr[i+1] = tmp ;
    }
}

void tcpModbus::slot_getDate(MBUSDATA *pMbusData)
{
    MBUSMSG *pMbuMsg;
    if (m_ActiveMbusReq == NULL) //点击设备管理有时会死机
        return;

    pMbuMsg = m_ActiveMbusReq->value(pMbusData);
    //Q_ASSERT(pMbuMsg != NULL);
    if ( pMbuMsg == NULL )
    {
        //qDebug()<<"slot_getDate err <pMbuMsg == NULL> ";
    }

    if (pMbusData->TimeOutFlag)
    {
        pMbuMsg->ErrCode = TCP_MBUS_TIMEOUT_ERROR;
        //qDebug()<<"slot_getDate here timeout";
        pMbusData->TimeOutFlag = 0;
    }
    else
    {
        if ( pMbusData->serial != pMbusData->TI )
        {
            pMbuMsg->ErrCode = TCP_MBUS_TIMEOUT_ERROR;
            //qDebug()<<"slot_getDate here timeout2";
        }
        else if ( pMbusData->MbusPdu[0] > 0x80 )
        {
            pMbuMsg->ErrCode = TCP_MBUS_RESPONSE_ERROR;
        }
        else if ( pMbusData->MbusPdu[0] <= 4 )
        {
            memcpy(pMbuMsg->DataPtr, &pMbusData->MbusPdu[2], pMbusData->MbusPdu[1]);
            //printf("%02X%02X Len %d\n",pMbuMsg->DataPtr[0],pMbuMsg->DataPtr[1],pMbusData->MbusPdu[1]);
            if ( pMbusData->MbusPdu[0] >= 3 )
            {
                endiandChg(pMbuMsg->DataPtr, pMbusData->MbusPdu[1]);
            }
            //printf("%02X%02X %02X%02X \n",pMbuMsg->DataPtr[0],pMbuMsg->DataPtr[1],pMbuMsg->DataPtr[3],pMbuMsg->DataPtr[4]);
            pMbuMsg->ErrCode = TCP_MBUS_NO_ERROR;
        }//LSH_2018-8-28 之前那个else不能要，否则会有本地TCP Modbus快速读写位变量存在延时的问题
        pMbusData->ConnStatus = TCP_MBUS_CONNECT_IDLE;
    }
    pMbuMsg->Done = 1;

/*	for ( int i=0; i<pMbusData->PduLen; i++ )
    {
        cout<<setw(2)<<setfill('0')<<hex<<(((int)*(((char*)pMbusData)+i)) & 0xFF)<<ends;
    }
    cout<<endl;*/
}

MbusMsgErrCode tcpModbus::getIdleMbusData(MBUSDATA **pMbusData)
{
    for (int i=0; i<MAX_CONN_NUM; i++)
    {
        if ( m_Data[i].ConnStatus == TCP_MBUS_CONNECT_NULL )
        {
            *pMbusData = &m_Data[i];
            m_Data[i].ConnStatus =  TCP_MBUS_CONNECT_TIMEOUT;
            return TCP_MBUS_NO_ERROR;
        }
    }
    *pMbusData = NULL;
    return TCP_MBUS_CONN_FULL_ERROR;
}


MbusMsgErrCode tcpModbus::tcpMbusPackNewRequest( MBUSDATA *pTcpMbusData, uchar RW, int Addr, ushort Count, uchar UnitID, uchar *DataPtr )
{/*
    uchar RW =          pTcpMbusData->MbusMsg.RW;
    uint Addr =       pTcpMbusData->MbusMsg.Addr;
    ushort Count =      pTcpMbusData->MbusMsg.Count;
    uchar* DataPtr =    pTcpMbusData->MbusMsg.Tbl + 16;
*/
    ushort byteCnt;
    int *pudLen           = & ( pTcpMbusData->PduLen );
    uchar *modbusFunction   = ( uchar* ) & ( pTcpMbusData->MbusPdu[0] );
    ushort *modbusAddr      = ( ushort* ) & ( pTcpMbusData->MbusPdu[1] );
    ushort *modbusBufrW4    = ( ushort* ) & ( pTcpMbusData->MbusPdu[3] );
    uchar *modbusBufrB6     = ( uchar* ) & ( pTcpMbusData->MbusPdu[5] );
    uchar *modbusBufrB7     = ( uchar* ) & ( pTcpMbusData->MbusPdu[6] );

    if (( Count == 0 ) || ( Count > 8 * TCP_MBUS_MODBUS_MAX ) || ( NULL == DataPtr ) )
    {
        /*检查参数正确性*/
        return TCP_MBUS_REQUEST_ERROR;
    }

    if (( Addr > 0 ) && ( Addr < 10000 ) )
    {
        Addr -= 1;
        if ( RW == TCP_MBUS_READ )
        {
            *modbusFunction = 1;/*读地址为0 - 10000 线圈 功能号为 1*/
        }
        else if ( RW == TCP_MBUS_WRITE )
        {
            if ( Count == 1 )
            {
                *modbusFunction = 5;/*写单个线圈 功能号为 5*/
            }
            if ( Count > 1 /*|| mModbusForceMulti == 1*/ )
            {
                *modbusFunction = 15;/*写多个线圈 功能号为 15*/
            }
        }
        else
        {
            return TCP_MBUS_REQUEST_ERROR;
        }
    }
    else if (( Addr > 10000 ) && ( Addr < 20000 ) )
    {
        Addr -= 10001;
        if ( RW == TCP_MBUS_READ )
        {
            *modbusFunction = 2;/*读离散输入量 功能号为 2*/
        }
        else
        {
            return TCP_MBUS_REQUEST_ERROR;
        }
    }
    else if ((( Addr >= 30001 ) && ( Addr <= 39999 ))  || (( Addr >= 310000 ) && ( Addr <= 365536 )))
    {
    	if( Addr <= 39999 )
        {
            Addr -= 30001;
        }
        else if( Addr >= 310000 )
        {
            Addr = Addr - 300001;
        }
        if ( RW == TCP_MBUS_READ )
        {
            *modbusFunction = 4;    /*读输入寄存器 功能号为 4*/
        }
        else
        {
            return TCP_MBUS_REQUEST_ERROR;
        }
    }
    else if ( ((Addr >= 40001) && (Addr <= 49999)) || ((Addr >= 410000) && (Addr <= 465536)) )
    {
        if( Addr <= 49999 )
        {
            Addr -= 40001;
        }
        else if( Addr >= 410000 )
        {
            Addr = Addr - 400001;
        }
        else
        {
            return TCP_MBUS_REQUEST_ERROR;
        }

        if ( RW == TCP_MBUS_READ )
        {
            *modbusFunction = 3;/*读多个寄存器 功能号为3*/
        }
        else if ( RW == TCP_MBUS_WRITE )
        {
            if ( Count == 1 )
            {
                *modbusFunction = 6;/*写单个寄存器 功能号为6*/
            }
            if ( Count > 1 /*|| mModbusForceMulti == 1*/ )
            {
                *modbusFunction = 16;   /*写多个寄存器功能号为 16*/
            }
        }
        else
        {
            return TCP_MBUS_REQUEST_ERROR;
        }
    }
    else
    {
        //qDebug()<< "new packed addr err" ;
        return TCP_MBUS_REQUEST_ERROR;
    }

    *modbusAddr = GetBE2U(( ushort )Addr );        /* 地址 2 byte, Addr < 50000 */
    switch ( *modbusFunction )
    {
    case 1:
    case 2:/*功能号1 2 做相同处理*/
        *modbusBufrW4 = GetBE2U( Count );        /* Count >= 0*/
        *pudLen = 5;
        /*maxChar = ((Count + 7)/8) + 5;*/
        break;

    case 3:
    case 4:/*功能号3 4 做相同处理*/
        *modbusBufrW4 = GetBE2U( Count );        /* Count >= 0*/
        /*Printf("count %d\n",Count);*/
        *pudLen = 5;
        /*maxChar = (Count*2) + 5;     */
        break;

    case 5:
        /*if ((*(__packed ushort*)DataPtr) & 0x1)*/
        if (( *( uchar* )DataPtr ) & 0x1 )
        {
            *modbusBufrW4 = GetBE2U( 0xFF00 );
        }
        else
        {
            *modbusBufrW4 = GetBE2U( 0x0000 );
        }
        *pudLen = 5;
        /*maxChar = 3 + 5;*/
        break;

    case 6:
        endiandChg(DataPtr, 2);
        *modbusBufrW4 = ( *( ushort* )DataPtr ); /*no need GetBE2U*/
        *pudLen = 5;
        /*maxChar = 3 + 5;  */
        break;

    case 15:
        byteCnt = ( Count + 7 ) / 8;      /* byte len, Count >= 0 */
        if ( byteCnt <= TCP_MBUS_MODBUS_MAX )
        {
            *modbusBufrW4 = GetBE2U( Count );
            *modbusBufrB6 = byteCnt;    /* byteCnt <= 240*/

             memcpy( modbusBufrB7, DataPtr, byteCnt );

            if ( Count & 0x7 )          /* bit left */
            {
                *( modbusBufrB6 + byteCnt ) &= MBitMaskTable[Count & 0x7];
            }

            *pudLen = *modbusBufrB6 + 6;
            /*maxChar = 3 + 5;      */
        }
        else
        {
            return TCP_MBUS_REQUEST_ERROR;
        }
        break;

    case 16:
        byteCnt = Count * 2;              /* byte len, Count >= 0*/
        if ( byteCnt <= TCP_MBUS_MODBUS_MAX )
        {
            *modbusBufrW4 = GetBE2U( Count );
            *modbusBufrB6 = ( uchar )byteCnt;
            endiandChg(DataPtr, byteCnt);
             memcpy( modbusBufrB7, DataPtr, byteCnt );

            *pudLen = *modbusBufrB6 + 6;
            /*maxChar = 3 + 5;      */
        }
        else
        {
            return TCP_MBUS_REQUEST_ERROR;
        }
        break;

    default:
        return TCP_MBUS_REQUEST_ERROR;
    }

    pTcpMbusData->Len = GetBE2U( 0x0000 | ( pTcpMbusData->PduLen + 1 ) );
    pTcpMbusData->PI = 0;
    pTcpMbusData->TI = pTcpMbusData->serial; //更新至HMI2018-04-26
    pTcpMbusData->UI = UnitID;
    pTcpMbusData->PduLen += 7;
    /*for(int i=0;i<pTcpMbusData->PduLen;i++)
    {
        cout<<hex<<(int)(*(((uchar*)pTcpMbusData)+i))<<ends;
    }
    cout<<endl;*/

   // Printf("packed len %d \n",pTcpMbusData->PduLen);*/
    return TCP_MBUS_NO_ERROR;
}

void tcpModbus::tcpMbusMsgReq(MBUSMSG *pMbusMsg, int connid)
{
    ConnectType connType = getConnectType(pMbusMsg->IP,
                                          pMbusMsg->Port,
                                          connid);

    if( connType == CONNECT_LOCAL )
    {
        mbus_tcp_req_struct mbus;        
        pMbusMsg->ErrCode = TCP_MBUS_NO_ERROR;
        pMbusMsg->Done = 0;

        strcpy((char *)mbus.IP, pMbusMsg->IP.toString().toLatin1().data());
        mbus.Addr = pMbusMsg->Addr;
        mbus.Count = pMbusMsg->Count;
        mbus.RW = pMbusMsg->RW;
        mbus.Port = pMbusMsg->Port;
        mbus.Unit = pMbusMsg->UnitID;
        mbus.First = 1;

        if(mbus.RW == 1)
        {
            memcpy( mbus.Data, pMbusMsg->DataPtr, 256 );
        }

        QString destIp = m_localDevIPHash->value(pMbusMsg->IP,"");

        if(destIp.length() == 0)
        {
            qDebug()<<"tcpmodbus.cpp err destIp";
            pMbusMsg->Done    = 1;
            pMbusMsg->ErrCode = TCP_MBUS_CONN_FULL_ERROR;
            return;
        }
        RWSocket *rwSocket = m_localRwSockHash->value(destIp, NULL);

        if(rwSocket == NULL)
        {
            struct NetCfg cfg;
            memset((char*)&cfg, 0, sizeof(struct NetCfg));
            ProtocolType proTye = MBUS_TCP;

            strcpy(cfg.ipAddr, pMbusMsg->IP.toString().toLatin1());
            cfg.port = pMbusMsg->Port;
            rwSocket = new RWSocket(proTye, (void *)&cfg, connid);
            connect(rwSocket, SIGNAL(readyRead()), this, SLOT(slot_getLocalData()), Qt::QueuedConnection);
            rwSocket->setTimeoutPollCnt(1);
            m_localRwSockHash->insert(destIp, rwSocket);
            rwSocket->openDev();
        }

        m_localMbusMsgHash->insert( QHostAddress((char *)(mbus.IP)), pMbusMsg );
        int len = rwSocket->writeDev((char *)&mbus, sizeof(mbus));
        if(len <= 0)
        {
            pMbusMsg->Done    = 1;
            pMbusMsg->ErrCode = TCP_MBUS_CONN_FULL_ERROR;
        }
    }
    else if( connType == CONNECT_REMOTE )
    {
        mbus_tcp_req_struct mbus;
        pMbusMsg->ErrCode = TCP_MBUS_NO_ERROR;
        pMbusMsg->Done = 0;

        strcpy((char *)mbus.IP, pMbusMsg->IP.toString().toLatin1().data());
        mbus.Addr = pMbusMsg->Addr;
        mbus.Count = pMbusMsg->Count;
        mbus.RW = pMbusMsg->RW;
        mbus.Port = pMbusMsg->Port;
        mbus.Unit = pMbusMsg->UnitID;
        mbus.First = 1;

        if(mbus.RW == 1)
        {
            memcpy( mbus.Data, pMbusMsg->DataPtr, 256 );
        }

        //quint64 devID = m_remoteDevIDHash->value(pMbusMsg->IP, 0);

        //if(devID == 0)
        //{
        //    pMbusMsg->Done    = 1;
        //    pMbusMsg->ErrCode = TCP_MBUS_CONN_FULL_ERROR;
        //    return;
        //}

        //RWSocket *rwSocket = m_remoteRwSockHash->value(devID, NULL);
        RWSocket *rwSocket = m_remoteRwSockHash->value(connid, NULL);

        if(rwSocket == NULL)
        {
            struct NetCfg cfg;
            memset((char*)&cfg, 0, sizeof(struct NetCfg));
            ProtocolType proTye = MBUS_TCP;

            strcpy(cfg.ipAddr, pMbusMsg->IP.toString().toLatin1());
            cfg.port = pMbusMsg->Port;
            rwSocket = new RWSocket(proTye, (void *)&cfg, connid);
            connect(rwSocket, SIGNAL(readyRead()), this, SLOT(slot_getRemoteData()), Qt::QueuedConnection);
            rwSocket->setTimeoutPollCnt(2);
            //m_remoteRwSockHash->insert(devID, rwSocket);
            m_remoteRwSockHash->insert(connid, rwSocket);
            rwSocket->openDev();
        }

        //qDebug(">>>>> REQ IP %s %p", (char *)mbus.IP, pMbusMsg);
        m_remoteMbusMsgHash->insert( QHostAddress((char *)mbus.IP), pMbusMsg );

        int len = rwSocket->writeDev((char *)&mbus, sizeof(mbus));
        if(len <= 0)
        {
            pMbusMsg->Done    = 1;
            pMbusMsg->ErrCode = TCP_MBUS_CONN_FULL_ERROR;
        }
    }
    else
    {
        MBUSDATA *pMbusData = NULL;
        TcpModbusMSG *pConn;
        MbusMsgErrCode err;

        //qDebug()<<"pMbusMsg ip "<<pMbusMsg->IP;

        pMbusData = m_ActiveMbusData->key(pMbusMsg->IP);

        if (pMbusData == NULL)
        {
            err = getIdleMbusData(&pMbusData);
            if (err != TCP_MBUS_NO_ERROR)
            {
                pMbusMsg->Done    = 1;
                pMbusMsg->ErrCode = err;
                return;
            }
        }

        if ( pMbusData != NULL)
        {
           // qDebug()<<"pMbusMsg ip "<<pMbusMsg->IP<<pMbusData->ConnStatus<<" pMbusData"<<pMbusData;
            m_ActiveMbusReq->insert(pMbusData,pMbusMsg);

            switch(pMbusData->ConnStatus)
            {
            case TCP_MBUS_CONNECT_LINKING:
                //pMbusMsg->Done	  = 1;
                //pMbusMsg->ErrCode = TCP_MBUS_TIMEOUT_ERROR;
                //qDebug()<<"TCP_MBUS_CONNECT_LINKING";
                break;
            case TCP_MBUS_CONNECT_NULL:
            case TCP_MBUS_CONNECT_TIMEOUT:
                pConn = m_ConnListHash->value(pMbusData);
                pConn->connectToAddr(pMbusMsg->IP, pMbusMsg->Port);
                m_ActiveMbusData->insert(pMbusData, pMbusMsg->IP);
                //pMbusMsg->Done	  = 1;
                //pMbusMsg->ErrCode = TCP_MBUS_TIMEOUT_ERROR;
                //qDebug()<<"TCP_MBUS_CONNECT_NULL";
                break;
            case TCP_MBUS_CONNECT_IDLE:
                //qDebug()<<"TCP_MBUS_CONNECT_IDLE";
                pConn = m_ConnListHash->value(pMbusData);
                err = tcpMbusPackNewRequest(pMbusData,pMbusMsg->RW, pMbusMsg->Addr, pMbusMsg->Count, pMbusMsg->UnitID, pMbusMsg->DataPtr);
                if ( err == TCP_MBUS_NO_ERROR )
                {
                    //m_ActiveMbusReq->insert(pMbusData,pMbusMsg);
                    pMbusMsg->Done = 0;
                    pMbusData->ConnStatus = TCP_MBUS_CONNECT_BUSY;
                    int ret = pConn->writeMSG();
                    if (ret < 0)
                    {
                        //qDebug()<<"tcpModbus::tcpMbusMsgReq write msg err ip "<<pMbusMsg->IP;
                        pConn->closeTcpConnect();
                        pMbusData->ConnStatus = TCP_MBUS_CONNECT_NULL;
                    }
                }
                else
                {
                    pMbusMsg->Done    = 1;
                    pMbusMsg->ErrCode = err;
                }
                break;
            case TCP_MBUS_CONNECT_BUSY:
                //pMbusMsg->Done = 1;
                //pMbusMsg->ErrCode = TCP_MBUS_BUSY_ERROR;
                break;
            default:
                //qDebug()<<"unexpect ConnStatus "<<pMbusData->ConnStatus;
                break;
            }
        }
        else
        {
            pMbusMsg->Done    = 1;
            pMbusMsg->ErrCode = TCP_MBUS_CONN_FULL_ERROR;
        }
    }
}
void tcpModbus::slot_getLocalData()
{
    mbus_tcp_req_struct mbus;
    MBUSMSG *pMbusMsg = NULL;
    RWSocket *rwSocket = (RWSocket *)sender();

    while(rwSocket->readDev((char *)&mbus) > 0)
    {
        //qDebug(">>>>>>>>>>>>>> %s",mbus.IP);
        if( strlen((char *)mbus.IP) == 0 )
        {
            QString destIp = m_localRwSockHash->key(rwSocket);
            QList<QHostAddress> ipAddrList = m_localDevIPHash->keys(destIp);

            foreach(QHostAddress ipAddr, ipAddrList)
            {
                pMbusMsg = m_localMbusMsgHash->value(ipAddr);

                if(pMbusMsg != NULL)
                {
                    pMbusMsg->ErrCode = errCoreMap[mbus.Error%10];
                    pMbusMsg->Done = 1;
                }
            }
            return;
        }

        pMbusMsg = m_localMbusMsgHash->value(QHostAddress((char *)(mbus.IP)));

        if(pMbusMsg)
        {
            pMbusMsg->Count = mbus.Count;
            memcpy(pMbusMsg->DataPtr, mbus.Data, 256);
            pMbusMsg->ErrCode = errCoreMap[mbus.Error%10];
            pMbusMsg->Done = 1;
        }
    }
}

void tcpModbus::slot_getRemoteData()
{
    mbus_tcp_req_struct mbus;
    MBUSMSG *pMbusMsg = NULL;
    RWSocket *rwSocket = (RWSocket *)sender();

    while(rwSocket->readDev((char *)&mbus) > 0)
    {
        //qDebug(">>>>>>>>>>>>>> %s",mbus.IP);
        if( strlen((char *)mbus.IP) == 0 )
        {
            quint64 devID = m_remoteRwSockHash->key(rwSocket);
            QList<QHostAddress> ipAddrList = m_remoteDevIDHash->keys(devID);

            foreach(QHostAddress ipAddr, ipAddrList)
            {
                pMbusMsg = m_remoteMbusMsgHash->value(ipAddr, NULL);
                //qDebug()<<"remote rsp "<<ipAddr<<pMbusMsg;

                if(pMbusMsg != NULL)
                {
                    pMbusMsg->ErrCode = errCoreMap[mbus.Error%10];
                    pMbusMsg->Done = 1;
                }
            }
            return;
        }

        pMbusMsg = m_remoteMbusMsgHash->value(QHostAddress((char *)mbus.IP));

        if(pMbusMsg)
        {
            pMbusMsg->Count = mbus.Count;
            memcpy(pMbusMsg->DataPtr, mbus.Data, 256);
            pMbusMsg->ErrCode = errCoreMap[mbus.Error%10];
            pMbusMsg->Done = 1;
        }
    }
}

/*是否在connec.ini文件中配置了设备*/
ConnectType tcpModbus::getConnectType(QHostAddress ipAddr, unsigned short port, int connid)
{
    ConnectType ret = m_remoteFlgHash->value(ipAddr, CONNECT_NULL);
    if( ret == CONNECT_NULL )
    {
        //ProjectPage *project = HomePage::getCurProject();
        //QSettings conn(QDir::homePath()+"/Documents"+"/Projects/"+project->path()+"/connect.ini", QSettings::IniFormat, 0 );
        //QString localStr = QString("%1:%2").arg(ipAddr.toString()).arg(QString::number(port)) + "new IP";
        //QString remoteStr = QString("%1:%2").arg(ipAddr.toString()).arg(QString::number(port));

        // if(conn.value(remoteStr).isValid())
        if(true)
        {
            //quint64 devID= conn.value(remoteStr).toLongLong();
            //quint64 devID=0;
            //m_remoteDevIDHash->insert(ipAddr, devID);
            m_remoteDevIDHash->insert(ipAddr, connid);
            m_remoteFlgHash->insert(ipAddr, CONNECT_REMOTE);
            ret = CONNECT_REMOTE;
        }
        //else if( conn.value(localStr).isValid())
        //{
        //    QString destIp = conn.value(localStr).toString();
        //    m_localDevIPHash->insert(ipAddr, destIp);
        //    m_remoteFlgHash->insert(ipAddr, CONNECT_LOCAL);
        //    ret = CONNECT_LOCAL;
        //}
        else
        {
            m_remoteFlgHash->insert(ipAddr, CONNECT_ORG);
            ret = CONNECT_ORG;
        }
    }
    return ret;
}
