#include "tcpModbusService.h"

#include<QHostAddress>

#define MAX_MBUS_PACKET_LEN  240  //最大一包160字节，其实能达到240

TcpModbusService::TcpModbusService( struct TcpModbusConnectCfg& connectCfg )
{
    struct TcpModbusConnect* connect = new TcpModbusConnect;
    connect->cfg = connectCfg;
    connect->writingCnt = 0;
    connect->connStatus = CONNECT_NO_OPERATION;
    connect->connectTestVar.lastUpdateTime = -1;
    m_connectList.append( connect );
    memset(&connect->msgData, 0, sizeof(connect->msgData));
    QHostAddress *ipaddr = new QHostAddress(connectCfg.IP);
    //connect->msgData.IP = *ipaddr;
    memcpy(&(connect->msgData.IP), ipaddr, sizeof(QHostAddress));
    connect->msgData.Port = connectCfg.port;
    connect->msgData.UnitID = connectCfg.unitID;
    connect->msgData.DataPtr = connect->buff;
    //printf("conn ip %s port %d unitId %d",connect->msgData.IP,connect->msgData.Port,connect->msgData.Unit);
    m_currTime = getCurrTime();
    m_devUseCnt = 0;

    m_tcpModbus = NULL;
    m_runTimer = NULL;
}
TcpModbusService::~TcpModbusService()
{
    if(m_tcpModbus)
    {
        delete m_tcpModbus;
    }
    if(m_runTimer)
    {
        delete m_runTimer;
    }
}

bool TcpModbusService::meetServiceCfg( int serviceType, int COM_interface )
{
    if ( serviceType == SERVICE_TYPE_TCPMBUS )
    {
        if ( m_connectList.count() > 0 )
        {
            return true;
        }
        else
            Q_ASSERT( false );
    }
    return false;
}

int TcpModbusService::getConnLinkStatus( int connectId )
{
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct TcpModbusConnect* connect = m_connectList[i];
        if ( connectId == connect->cfg.id )
        {
            //qDebug()<<"getConnLinkStatus "<<connect->connStatus;
            return connect->connStatus;
        }
    }
    return -1;
}

int TcpModbusService::getComMsg( QList<struct ComMsg> &msgList )
{
    int ret = 0;
    m_mutexGetMsg.lock();

    ret = m_comMsg.count();
    if ( m_comMsg.count() > 0 )
    {
        msgList.append( m_comMsg.values() );
        m_comMsg.clear();
    }

    m_mutexGetMsg.unlock();
    return ret;
}

int TcpModbusService:: splitLongVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct TcpModbusConnect* connect = m_connectList[i];
        if ( pHmiVar->connectId == connect->cfg.id )
        {
            pHmiVar->lastUpdateTime = -1; //立即更新
            if ( pHmiVar->area == MODBUX_AREA_3X || pHmiVar->area == MODBUX_AREA_4X )
            {
                if ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup )
                {
                    if ( pHmiVar->len > MAX_MBUS_PACKET_LEN*8 && pHmiVar->splitVars.count() <= 0 )  //拆分
                    {
                        for ( int splitedLen = 0; splitedLen < pHmiVar->len; )
                        {
                            int len = pHmiVar->len - splitedLen;
                            if ( len > MAX_MBUS_PACKET_LEN*8 )
                                len = MAX_MBUS_PACKET_LEN * 8;

                            struct HmiVar* pSplitHhiVar = new HmiVar;
                            pSplitHhiVar->varId = pHmiVar->varId + (( pHmiVar->splitVars.count() + 1 ) << 16 );
                            pSplitHhiVar->connectId = pHmiVar->connectId;
                            pSplitHhiVar->cycleTime = pHmiVar->cycleTime;
                            pSplitHhiVar->area = pHmiVar->area;
                            pSplitHhiVar->addOffset = pHmiVar->addOffset + ( pHmiVar->bitOffset + splitedLen ) / 16;
                            pSplitHhiVar->bitOffset = ( pHmiVar->bitOffset + splitedLen ) % 16;
                            pSplitHhiVar->len = len;
                            pSplitHhiVar->dataType = pHmiVar->dataType;
                            pSplitHhiVar->err = VAR_NO_OPERATION; //没有数据，初始状态为没有初始化
                            pSplitHhiVar->wStatus = VAR_NO_ERR;   //没有进行第一次读默认为正确
                            pSplitHhiVar->lastUpdateTime = -1; //立即更新

                            splitedLen += len;
                            pHmiVar->splitVars.append( pSplitHhiVar );
                        }
                    }
                }
                else
                {
                    if ( pHmiVar->len > MAX_MBUS_PACKET_LEN && pHmiVar->splitVars.count() <= 0 )  //拆分
                    {
                        for ( int splitedLen = 0; splitedLen < pHmiVar->len; )
                        {
                            int len = pHmiVar->len - splitedLen;
                            if ( len > MAX_MBUS_PACKET_LEN )
                                len = MAX_MBUS_PACKET_LEN;

                            struct HmiVar* pSplitHhiVar = new HmiVar;
                            pSplitHhiVar->varId = pHmiVar->varId + (( pHmiVar->splitVars.count() + 1 ) << 16 );
                            pSplitHhiVar->connectId = pHmiVar->connectId;
                            pSplitHhiVar->cycleTime = pHmiVar->cycleTime;
                            pSplitHhiVar->area = pHmiVar->area;
                            pSplitHhiVar->addOffset = pHmiVar->addOffset + ( splitedLen + 1 ) / 2;
                            pSplitHhiVar->bitOffset = -1;
                            pSplitHhiVar->len = len;
                            pSplitHhiVar->dataType = pHmiVar->dataType;
                            pSplitHhiVar->err = VAR_NO_OPERATION; //没有数据，初始状态为没有初始化
                            pSplitHhiVar->wStatus = VAR_NO_ERR;   //没有进行第一次读默认为正确
                            pSplitHhiVar->lastUpdateTime = -1; //立即更新

                            splitedLen += len;
                            pHmiVar->splitVars.append( pSplitHhiVar );
                        }
                    }
                }
            }
            else
            {
                if ( pHmiVar->len > MAX_MBUS_PACKET_LEN*8 && pHmiVar->splitVars.count() <= 0 )  //拆分
                {
                    for ( int splitedLen = 0; splitedLen < pHmiVar->len; )
                    {
                        int len = pHmiVar->len - splitedLen;
                        if ( len > MAX_MBUS_PACKET_LEN*8 )
                            len = MAX_MBUS_PACKET_LEN * 8;

                        struct HmiVar* pSplitHhiVar = new HmiVar;
                        pSplitHhiVar->varId = pHmiVar->varId + (( pHmiVar->splitVars.count() + 1 ) << 16 );
                        pSplitHhiVar->connectId = pHmiVar->connectId;
                        pSplitHhiVar->cycleTime = pHmiVar->cycleTime;
                        pSplitHhiVar->area = pHmiVar->area;
                        pSplitHhiVar->addOffset = pHmiVar->addOffset + splitedLen;
                        pSplitHhiVar->bitOffset = -1;
                        pSplitHhiVar->len = len;
                        pSplitHhiVar->dataType = pHmiVar->dataType;
                        pSplitHhiVar->err = VAR_NO_OPERATION; //没有数据，初始状态为没有初始化
                        pSplitHhiVar->wStatus = VAR_NO_ERR;   //没有进行第一次读默认为正确
                        pSplitHhiVar->lastUpdateTime = -1;    //立即更新

                        splitedLen += len;
                        pHmiVar->splitVars.append( pSplitHhiVar );
                    }
                }
            }
            return 0;
        }
    }
    return -1;
}

int TcpModbusService::onceRealUpdateVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    m_mutexOnceReal.lock();
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct TcpModbusConnect* connect = m_connectList[i];
        if ( pHmiVar->connectId == connect->cfg.id )
        {
            pHmiVar->lastUpdateTime = -1; //立即更新
            if ( pHmiVar->splitVars.count() > 0 )
            {
                for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
                {
                    struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                    pSplitHhiVar->lastUpdateTime = -1;
                    pSplitHhiVar->overFlow = false;
                    if ( !connect->onceRealVarsList.contains( pSplitHhiVar ) )
                        connect->onceRealVarsList.append( pSplitHhiVar );
                }
            }
            else
            {
                pHmiVar->lastUpdateTime = -1;
                pHmiVar->overFlow = false;
                if ( !connect->onceRealVarsList.contains( pHmiVar ) )
                    connect->onceRealVarsList.append( pHmiVar );
            }
            m_mutexOnceReal.unlock();
            return 0;
        }
    }
    m_mutexOnceReal.unlock();
    return -1;
}

int TcpModbusService::startUpdateVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    m_mutexUpdateVar.lock();
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct TcpModbusConnect* connect = m_connectList[i];
        if ( pHmiVar->connectId == connect->cfg.id )
        {
            pHmiVar->lastUpdateTime = -1; //立即更新
            if ( pHmiVar->splitVars.count() > 0 )
            {
                for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
                {
                    struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                    pSplitHhiVar->lastUpdateTime = -1;
                    pSplitHhiVar->overFlow = false;
                    if ( !connect->actVarsList.contains( pSplitHhiVar ) )
                        connect->actVarsList.append( pSplitHhiVar );
                }
            }
            else
            {
                pHmiVar->lastUpdateTime = -1;
                pHmiVar->overFlow = false;
                if ( !connect->actVarsList.contains( pHmiVar ) )
                    connect->actVarsList.append( pHmiVar );
            }

            m_mutexUpdateVar.unlock();
            return 0;
        }
    }
    m_mutexUpdateVar.unlock();
    return -1;
}

int TcpModbusService::stopUpdateVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    m_mutexUpdateVar.lock();
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct TcpModbusConnect* connect = m_connectList[i];
        if ( pHmiVar->connectId == connect->cfg.id )
        {
            if ( pHmiVar->splitVars.count() > 0 )
            {
                for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
                {
                    struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                    connect->actVarsList.removeAll( pSplitHhiVar );
                }
            }
            else
                connect->actVarsList.removeAll( pHmiVar );
            m_mutexUpdateVar.unlock();
            return 0;
        }
    }
    m_mutexUpdateVar.unlock();
    return -1;
}

int TcpModbusService::readVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    if ( pHmiVar->err == VAR_NO_ERR )  //无错误才拷贝数据
    {
        if ( pHmiVar->area == MODBUX_AREA_3X || pHmiVar->area == MODBUX_AREA_4X )
        {
            if ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup )
            {                          // 位变量长度为位的个数
                Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
                int dataBytes = ( pHmiVar->len + 7 ) / 8;
                if ( bufLen >= dataBytes && pHmiVar->rData.size() >= dataBytes )
                {
                    for ( int j = 0; j < pHmiVar->len; j++ )
                    {
                        int byteOffset = ( pHmiVar->bitOffset + j ) / 8;
                        int bitOffset = /*7 - */( pHmiVar->bitOffset + j ) % 8;
                        if ( pHmiVar->rData.at( byteOffset ) & ( 1 << bitOffset ) )
                            pBuf[j/7] |= ( 1 << ( j % 8 ) );
                        else
                            pBuf[j/7] &= ~( 1 << ( j % 8 ) );
                    }
                }
                else
                    Q_ASSERT( false );
            }
            else
            {
                //Q_ASSERT(pHmiVar->len%2 == 0);
                if ( bufLen >= pHmiVar->len && pHmiVar->rData.size() >= pHmiVar->len )
                {
                    for ( int j = 0; j < pHmiVar->len; j++ )
                    {                  //奇偶字节调换
                        if ( pHmiVar->dataType == Com_String )
                        {
                            if ( j & 0x1 )
                                pBuf[j] = pHmiVar->rData.at( j - 1 );
                            else
                                pBuf[j] = pHmiVar->rData.at( j + 1 );
                        }
                        else
                            pBuf[j] = pHmiVar->rData.at( j );
                    }
                }
                else
                    Q_ASSERT( false );
            }
        }
        else
        {
            int dataBytes = ( pHmiVar->len + 7 ) / 8; //字节数
            if ( bufLen >= dataBytes && pHmiVar->rData.size() >= dataBytes )
                memcpy( pBuf, pHmiVar->rData.data(), dataBytes );
            else
                Q_ASSERT( false );
        }
    }
    //printf("return p->err %d\n",pHmiVar->err);
    return pHmiVar->err;
}

int TcpModbusService::readDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    Q_ASSERT( pHmiVar && pBuf );
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct TcpModbusConnect* connect = m_connectList[i];
        if ( pHmiVar->connectId == connect->cfg.id )
        {
            int ret;
            m_mutexRw.lock();
            if ( pHmiVar->splitVars.count() > 0 )
            {
                for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
                {
                    struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                    int len = 0;
                    if ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup )
                        len = ( pSplitHhiVar->len + 7 ) / 8;
                    else
                        len = pSplitHhiVar->len;

                    ret = readVar( pSplitHhiVar, pBuf, len );
                    if ( ret != VAR_NO_ERR )
                        break;
                    pBuf += len;
                    bufLen -= len;
                }
            }
            else
                ret = readVar( pHmiVar, pBuf, bufLen );

            m_mutexRw.unlock();
            return ret;
        }
    }
    return -1;
}

int TcpModbusService::writeVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    int ret;
    Q_ASSERT( pHmiVar && pBuf );
    m_mutexRw.lock();
    if ( pHmiVar->area == MODBUX_AREA_3X || pHmiVar->area == MODBUX_AREA_4X )
    {                            //大小数等到发送写命令时再处理
        if ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup )
        {                        //位变量长度为位的个数
            Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
            int count = ( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16;  //字数
            Q_ASSERT( count > 0 && count <= 120 );
            if ( count > 120 )
                count = 120;
            int dataBytes = 2 * count;
            dataBytes = qMin( bufLen, dataBytes );
            if ( pHmiVar->wData.size() < dataBytes )
                pHmiVar->wData.resize( dataBytes );

            memcpy( pHmiVar->wData.data(), pBuf, dataBytes );
            pHmiVar->lastUpdateTime = -1;

            if ( pHmiVar->rData.size() < dataBytes )
                pHmiVar->rData.resize( dataBytes );
            for ( int i = 0; i < pHmiVar->len; i++ )
            {
                int byteW = i / 8 ;
                int bitW = i % 8;
                int byteR = ( pHmiVar->bitOffset + i ) / 8 ;
                int bitR = /*7 -*/ ( pHmiVar->bitOffset + i ) % 8;
                if ( byteW < pHmiVar->wData.size() && byteR < pHmiVar->rData.size() )
                {
                    if ( pHmiVar->wData[byteW] & ( 1 << bitW ) )
                        pHmiVar->rData[byteR] = (( uchar )pHmiVar->rData[byteR] ) | ( 1 << bitR );
                    else
                        pHmiVar->rData[byteR] = (( uchar )pHmiVar->rData[byteR] ) & ( ~( 1 << bitR ) );
                }
                else
                    break;
            }
        }
        else
        {
            int count = ( pHmiVar->len + 1 ) / 2; //字数
            Q_ASSERT( count > 0 && count <= 120 );
            if ( count > 120 )
                count = 120;
            int dataBytes = 2 * count;
            dataBytes = qMin( bufLen, pHmiVar->len );
            if ( pHmiVar->wData.size() < dataBytes )
                pHmiVar->wData.resize( dataBytes );

            memcpy( pHmiVar->wData.data(), pBuf, dataBytes );
            pHmiVar->lastUpdateTime = -1;

            if ( pHmiVar->rData.size() < dataBytes )
                pHmiVar->rData.resize( dataBytes );
            for ( int j = 0; j < pHmiVar->len; j++ )
            {                          // 奇偶字节调换
                if ( j < pHmiVar->wData.size() && j < pHmiVar->rData.size() )
                {
                    if ( pHmiVar->dataType == Com_String )
                    {
                        if ( j & 0x1 )
                            pHmiVar->rData[j] = pHmiVar->wData.at( j - 1 );
                        else if ( j + 1 < pHmiVar->wData.size() )
                            pHmiVar->rData[j] = pHmiVar->wData.at( j + 1 );
                    }
                    else
                        pHmiVar->rData[j] = pHmiVar->wData.at( j );
                }
                else
                    break;
            }
        }
    }
    else
    {
        Q_ASSERT( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup );
        int count = pHmiVar->len;
        Q_ASSERT( count > 0 &&  count <= 120*8 );
        if ( count > 8*120 )
            count = 8 * 120;
        int dataBytes = ( count + 7 ) / 8;  //字节数
        dataBytes = qMin( bufLen, dataBytes );
        if ( pHmiVar->wData.size() < dataBytes )
            pHmiVar->wData.resize( dataBytes );
        memcpy( pHmiVar->wData.data(), pBuf, dataBytes );
        pHmiVar->lastUpdateTime = -1;

        if ( pHmiVar->rData.size() < dataBytes )
            pHmiVar->rData.resize( dataBytes );
        memcpy( pHmiVar->rData.data(), pHmiVar->wData.data(), dataBytes );
    }
    pHmiVar->wStatus = VAR_NO_OPERATION; // 正在读写
    ret = pHmiVar->err;
    m_mutexRw.unlock();

    return ret; //是否要返回成功
}

int TcpModbusService::writeDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    Q_ASSERT( pHmiVar && pBuf );
    //qDebug("%02x%02X",pBuf[0],pBuf[1]);
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct TcpModbusConnect* connect = m_connectList[i];
        if ( pHmiVar->connectId == connect->cfg.id )
        {
            if ( connect->connStatus != CONNECT_LINKED_OK )
                return  VAR_COMM_TIMEOUT;
            int ret = 0;
            m_mutexWReq.lock();          //注意2个互斥锁顺序,防止死锁
            if ( pHmiVar->splitVars.count() > 0 )
            {
                for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
                {
                    struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                    int len = 0;
                    if ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup )
                        len = ( pSplitHhiVar->len + 7 ) / 8;
                    else
                        len = pSplitHhiVar->len;

                    ret = writeVar( pSplitHhiVar, pBuf, len );
                    connect->wrtieVarsList.append( pSplitHhiVar );
                    pBuf += len;
                    bufLen -= len;
                }
            }
            else
            {
                ret = writeVar( pHmiVar, pBuf, bufLen );
                connect->wrtieVarsList.append( pHmiVar );
            }
            m_mutexWReq.unlock();

            return ret; //是否要返回成功
        }
    }
    return -1;
}

int TcpModbusService::addConnect( struct TcpModbusConnectCfg& connectCfg )
{                                      //要检查是否重复 add
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct TcpModbusConnect* connect = m_connectList[i];
        //Q_ASSERT( connect->cfg.COM_interface == connectCfg.COM_interface );//绝对要是同一个设备接口
        if ( connectCfg.id == connect->cfg.id )
        {
            Q_ASSERT( false );
            return 0;
        }
    }
    struct TcpModbusConnect* connect = new TcpModbusConnect;
    connect->cfg = connectCfg;
    connect->writingCnt = 0;
    connect->connStatus = CONNECT_NO_OPERATION;
    connect->connectTestVar.lastUpdateTime = -1;

    memset(&connect->msgData, 0, sizeof(connect->msgData));
    //memcpy(connect->msgData.IP, connectCfg.IP.toLatin1().data(), connectCfg.IP.toLatin1().size());
    QHostAddress *ipaddr = new QHostAddress(connectCfg.IP);
    //connect->msgData.IP = *ipaddr;
    memcpy(&(connect->msgData.IP), ipaddr, sizeof(QHostAddress));
    //connect->msgData.IP.setAddress(connectCfg.IP);
    connect->msgData.Port = connectCfg.port;
    connect->msgData.UnitID = connectCfg.unitID;
    connect->msgData.DataPtr = connect->buff;

    //printf("conn ip %s port %d unitId %d connect->cfg.id %d\n",connect->msgData.IP,connect->msgData.Port,connect->msgData.Unit, connect->cfg.id);

    m_connectList.append( connect );
    return 0;
}

int TcpModbusService::sndWriteVarMsgToDriver( struct TcpModbusConnect* connect, struct HmiVar* pHmiVar )
{
    int sendedLen = 0;
    Q_ASSERT( connect && pHmiVar );

    MBUSMSG *msgPar = &connect->msgData;
    msgPar->ErrCode = TCP_MBUS_NO_ERROR;
    msgPar->Done = 0;

    m_mutexRw.lock();

    msgPar->RW = 1;   //写
    msgPar->Addr = pHmiVar->addOffset + 1;
    if ( msgPar->Addr < 10000 )
    {
        msgPar->Addr += pHmiVar->area * 10000;
    }
    else
    {
        msgPar->Addr += pHmiVar->area * 100000;
    }

    if ( pHmiVar->area == MODBUX_AREA_3X || pHmiVar->area == MODBUX_AREA_4X )
    {
        if ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup )
        {                              // 位变量长度为位的个数
            Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
            int count = ( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16;  //字数
            Q_ASSERT( count > 0 && count <= 120 );
            if ( count > 120 )
                count = 120;
            msgPar->Count = count;      // 占用的总字数
            int dataBytes = 2 * count;
            if ( pHmiVar->rData.size() < dataBytes )
                pHmiVar->rData.resize( dataBytes );
            //qDebug()<< pHmiVar->wData.toHex()<<" "<<pHmiVar->bitOffset<<" "<<pHmiVar->len;
            for ( int i = 0; i < pHmiVar->len; i++ )
            {
                int byteW = i / 8 ;
                int bitW = i % 8;
                int byteR = ( pHmiVar->bitOffset + i ) / 8 ;
                int bitR = /*7 - */( pHmiVar->bitOffset + i ) % 8;

                //qDebug()<<"pHmiVar->bitOffset  "<<pHmiVar->bitOffset ;

                if ( byteW < pHmiVar->wData.size() && byteR < pHmiVar->rData.size() )
                {
                    if ( pHmiVar->wData[byteW] & ( 1 << bitW ) )
                        pHmiVar->rData[byteR] = (( uchar )pHmiVar->rData[byteR] ) | ( 1 << bitR );
                    else
                        pHmiVar->rData[byteR] = (( uchar )pHmiVar->rData[byteR] ) & ( ~( 1 << bitR ) );
                }
                else
                    break;
            }
            memcpy( msgPar->DataPtr, pHmiVar->rData.data(), dataBytes );
            sendedLen = count * 16;
        }
        else
        {
            int count = ( pHmiVar->len + 1 ) / 2; //字数
            Q_ASSERT( count > 0 && count <= 120 );
            if ( count > 120 )
                count = 120;
            msgPar->Count = count;      // 占用的总字数
            int dataBytes = 2 * count;
            if ( pHmiVar->rData.size() < dataBytes )
                pHmiVar->rData.resize( dataBytes );

            for ( int j = 0; j < pHmiVar->len; j++ )
            {                          // 奇偶字节调换
                if ( j < pHmiVar->wData.size() && j < pHmiVar->rData.size() )
                {
                    if ( pHmiVar->dataType == Com_String )
                    {
                        if ( j & 0x1 )
                            pHmiVar->rData[j] = pHmiVar->wData.at( j - 1 );
                        else
                            pHmiVar->rData[j] = pHmiVar->wData.at( j + 1 );
                    }
                    else
                        pHmiVar->rData[j] = pHmiVar->wData.at( j );
                }
                else
                    break;
            }
            memcpy( msgPar->DataPtr, pHmiVar->rData.data(), dataBytes );
            sendedLen = count * 2;
        }
    }
    else if ( pHmiVar->area == MODBUX_AREA_0X || pHmiVar->area == MODBUX_AREA_1X )
    {                                  // 位变量长度为位的个数
        Q_ASSERT( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup );
        int count = pHmiVar->len;
        Q_ASSERT( count > 0 &&  count <= 120*8 );
        if ( count > 8*120 )
            count = 8 * 120;
        msgPar->Count = count;
        int dataBytes = ( count + 7 ) / 8;
        if ( pHmiVar->rData.size() < dataBytes )
            pHmiVar->rData.resize( dataBytes );

        memcpy( pHmiVar->rData.data(), pHmiVar->wData.data(), qMin( dataBytes, pHmiVar->wData.size() ) );
        memcpy( msgPar->DataPtr, pHmiVar->rData.data(), dataBytes );
        sendedLen = count;
    }
    else
    {
        Q_ASSERT( false );
        msgPar->Count = 0;
    }
    if ( sendedLen > 0 )
    {
        m_devUseCnt++;
        m_mutexRw.unlock();
        //qDebug("%02X%02X",msgPar->DataPtr[0],msgPar->DataPtr[1]);
        m_tcpModbus->tcpMbusMsgReq(msgPar, connect->cfg.id);
        //printf("REQ W F=1\n");
        //mbus_tcp_req__main(msgPar);
        //connect->processTimeFlg = 0 ;
        //connect->processTime =0;
        //printf("REQ W F=1\n");
        //int len = write( m_fd, ( char* ) & msgPar, sizeof( struct MbusMsgPar ) );
        //Q_ASSERT( len > 0 );
    }
    else
        m_mutexRw.unlock();

    return sendedLen;
}

int TcpModbusService::sndReadVarMsgToDriver( struct TcpModbusConnect* connect, struct HmiVar* pHmiVar )
{
    int sendedLen = 0;
    Q_ASSERT( connect && pHmiVar );

    MBUSMSG *msgPar = &connect->msgData;
    msgPar->ErrCode = TCP_MBUS_NO_ERROR;
    msgPar->Done = 0;
//printf("try send read msg\n");
    m_mutexRw.lock();

    msgPar->RW = 0;   //写
    msgPar->Addr = pHmiVar->addOffset + 1;
    if ( msgPar->Addr < 10000 )
    {
        msgPar->Addr += pHmiVar->area * 10000;
    }
    else
    {
        msgPar->Addr += pHmiVar->area * 100000;
    }

    if ( pHmiVar->area == MODBUX_AREA_3X || pHmiVar->area == MODBUX_AREA_4X )
    {
        if ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup )
        {                              // 位变量长度为位的个数
            Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
            int count = ( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16;  //字数
            Q_ASSERT( count > 0 && count <= 120 );
            if ( count > 120 )
                count = 120;
            msgPar->Count = count;      // 占用的总字数
            sendedLen = count * 16;
        }
        else
        {
            int count = ( pHmiVar->len + 1 ) / 2;  //字数
            Q_ASSERT( count > 0 && count <= 120 );
            if ( count > 120 )
                count = 120;
            msgPar->Count = count;      // 占用的总字数
            sendedLen = count * 2;
        }
    }
    else if ( pHmiVar->area == MODBUX_AREA_0X || pHmiVar->area == MODBUX_AREA_1X )
    {                                  // 位变量长度为位的个数
        Q_ASSERT( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup );
        int count = pHmiVar->len;
        Q_ASSERT( count > 0 &&  count <= 120*8 );
        if ( count > 8*120 )
            count = 8 * 120;
        msgPar->Count = count;
        sendedLen = count;
    }
    else
    {
        Q_ASSERT( false );
        msgPar->Count = 0;
        sendedLen = 0;
    }

    if ( sendedLen > 0 )
    {
        m_devUseCnt++;
        m_mutexRw.unlock();
        //printf("REQ R F=1\n");
        //printf("addr %d count%d\n",msgPar->Addr,msgPar->Count);
        //mbus_tcp_req__main(msgPar);
        m_tcpModbus->tcpMbusMsgReq(msgPar, connect->cfg.id);
        //connect->processTimeFlg = 0 ;
        //connect->processTime =0;
        //printf("REQ R F=1\n");
        //int len = write( m_fd, ( char* ) & msgPar, sizeof( struct MbusMsgPar ) );
        //Q_ASSERT( len > 0 );
    }
    else
        m_mutexRw.unlock();

    return sendedLen;
}

bool isWaitingReadRsp( struct TcpModbusConnect* connect, struct HmiVar* pHmiVar )
{
    Q_ASSERT( connect && pHmiVar );
    if ( connect->waitUpdateVarsList.contains( pHmiVar ) )
        return true;
    else if ( connect->combinVarsList.contains( pHmiVar ) )
        return true;
    else if ( connect->requestVarsList.contains( pHmiVar ) )
        return true;
    else
        return false;
}

bool TcpModbusService::trySndWriteReq( struct TcpModbusConnect* connect ) //不处理超长变量
{
    if ( m_devUseCnt < MAXDEVCNT )                          //没有正在读写, snd read, 一次只能发送一包请求
    {
        if ( connect->writingCnt == 0 && connect->requestVarsList.count() == 0 ) //这个连接没有读写
        {
            m_mutexWReq.lock();
            if ( connect->waitWrtieVarsList.count() > 0 )
            {
                struct HmiVar* pHmiVar = connect->waitWrtieVarsList[0];
                if (( pHmiVar->lastUpdateTime == -1 )  //3X,4X 位变量需要先读
                                && ( pHmiVar->area == MODBUX_AREA_3X || pHmiVar->area == MODBUX_AREA_4X )
                                && ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup || pHmiVar->dataType == Com_String ) )
                {
                    if ( !isWaitingReadRsp( connect, pHmiVar ) )
                        connect->waitUpdateVarsList.prepend( pHmiVar ); //加入到读队列最前,保证下次最先读
                }
                else if ( !isWaitingReadRsp( connect, pHmiVar ) ) //没有正在等待读
                {                                   //直接写,snd write cmd to driver
                    if ( sndWriteVarMsgToDriver( connect, pHmiVar ) > 0 )
                    {
                        connect->writingCnt = 1;    //进入写状态
                        m_mutexWReq.unlock();

                        return true;
                    }
                    else
                    {
                        connect->waitWrtieVarsList.takeAt( 0 );//出列,丢掉写请求
                        pHmiVar->err = VAR_PARAM_ERROR;
                    }
                }
            }
            m_mutexWReq.unlock();
        }
    }
    return false;
}

bool TcpModbusService::trySndReadReq( struct TcpModbusConnect* connect ) //不处理超长变量
{
//printf("trySndReadReq %d %d\n",m_devUseCnt,MAXDEVCNT);
    if ( m_devUseCnt < MAXDEVCNT )             //没有正在读写, snd read, 一次只能发送一包请求
    {
        //printf("wc %d rc %d", connect->writingCnt, connect->requestVarsList.count());
        if ( connect->writingCnt == 0 && connect->requestVarsList.count() == 0 ) //这个连接没有读写
        {
            //printf("connect->combinVarsList.count() %d\n",connect->combinVarsList.count());
            if ( connect->combinVarsList.count() > 0 )
            {
                connect->requestVarsList = connect->combinVarsList;
                connect->requestVar = connect->combinVar;
                for ( int i = 0; i < connect->combinVarsList.count(); i++ )
                {                      //要把已经请求的变量从等待队列中删除
                    struct HmiVar* pHmiVar = connect->combinVarsList[i];
                    connect->waitUpdateVarsList.removeAll( pHmiVar );
                    pHmiVar->lastUpdateTime = m_currTime; //这个时候才是变量上次刷新时间
                }
                connect->combinVarsList.clear();

                if ( sndReadVarMsgToDriver( connect, &( connect->requestVar ) ) > 0 )
                {
                    return true;
                }
                else
                {
                    for ( int i = 0; i < connect->requestVarsList.count(); i++ )
                    {
                        struct HmiVar* pHmiVar = connect->requestVarsList[i];
                        pHmiVar->err = VAR_PARAM_ERROR;
                        //printf("sndReadVarMsgToDriver  VAR_PARAM_ERROR\n");
                    }
                    connect->requestVarsList.clear();
                }
            }
        }
    }
    return false;
}

bool TcpModbusService::updateWriteReq( struct TcpModbusConnect* connect )
{
    bool newUpdateVarFlag = false;
    QList<struct HmiVar*> tmpList;
    m_mutexWReq.lock();
    //int currTime = m_currTime;//getCurrTime();
    for ( int i = 0; i < connect->wrtieVarsList.count(); i++ )
    {                                  // 找出需要刷新的变量
        struct HmiVar* pHmiVar = connect->wrtieVarsList[i];


        if ( !connect->waitWrtieVarsList.contains( pHmiVar ) )
        {
            /*if ( connect->waitUpdateVarsList.contains( pHmiVar ) )
                connect->waitUpdateVarsList.removeAll( pHmiVar );

            if ( connect->requestVarsList.contains( pHmiVar )  )
                connect->requestVarsList.removeAll( pHmiVar );*/

            connect->waitWrtieVarsList.append( pHmiVar );
            newUpdateVarFlag = true;
        }
        else
        {
            tmpList.append(pHmiVar);
        }
    }

    connect->wrtieVarsList.clear() ;
    connect->wrtieVarsList += tmpList;
    m_mutexWReq.unlock();

    return newUpdateVarFlag;
}

bool TcpModbusService::updateReadReq( struct TcpModbusConnect* connect )
{
    bool newUpdateVarFlag = false;
    m_mutexUpdateVar.lock();
    int currTime = m_currTime;//getCurrTime();
    if ( connect->connStatus == CONNECT_LINKED_OK )
    {
        for ( int i = 0; i < connect->actVarsList.count(); i++ )
        {                                  // 找出需要刷新的变量
            struct HmiVar* pHmiVar = connect->actVarsList[i];
            if (( pHmiVar->lastUpdateTime == -1 ) || ( calIntervalTime( pHmiVar->lastUpdateTime, currTime ) >= pHmiVar->cycleTime ) )
            {                              // 需要刷新
                if (( !connect->waitUpdateVarsList.contains( pHmiVar ) )
                                && ( !connect->waitWrtieVarsList.contains( pHmiVar ) )
                                //&& ( !connect->combinVarsList.contains( pHmiVar ) ) 好像多余，每次发送前都会清空一次
                                && ( !connect->requestVarsList.contains( pHmiVar ) ) )
                {
                    //pHmiVar->lastUpdateTime = currTime; 应该确定已经发送到驱动了才更新
                    connect->waitUpdateVarsList.append( pHmiVar );
                    newUpdateVarFlag = true;
                }
            }
        }
    }
    else
    {
        connect->waitUpdateVarsList.clear();
        if ( connect->actVarsList.count() > 0 && connect->requestVarsList.count() == 0 )
        {
            connect->actVarsList[0]->lastUpdateTime = connect->connectTestVar.lastUpdateTime;
            connect->connectTestVar = *( connect->actVarsList[0] );
            connect->connectTestVar.varId = 65535;
            connect->connectTestVar.cycleTime = 2000;
            if (( connect->connectTestVar.lastUpdateTime == -1 )
                            || ( calIntervalTime( connect->connectTestVar.lastUpdateTime, currTime ) >= connect->connectTestVar.cycleTime ) )
            {
                if (( !connect->waitUpdateVarsList.contains( &( connect->connectTestVar ) ) )
                                && ( !connect->waitWrtieVarsList.contains( &( connect->connectTestVar ) ) )
                                && ( !connect->requestVarsList.contains( &( connect->connectTestVar ) ) ) )
                {
                    connect->waitUpdateVarsList.append( &( connect->connectTestVar ) );
                    newUpdateVarFlag = true;
                }
            }


        }
    }
    m_mutexUpdateVar.unlock();

    //清除正在写的变量
    /*m_mutexWReq.lock();
    for ( int i = 0; i < connect->waitWrtieVarsList.count(); i++ )
    {                                  // 找出需要刷新的变量
        struct HmiVar* pHmiVar = connect->waitWrtieVarsList[i];

        if ( connect->waitUpdateVarsList.contains( pHmiVar ) )
            connect->waitUpdateVarsList.removeAll( pHmiVar );

        if ( connect->requestVarsList.contains( pHmiVar )  )
            connect->requestVarsList.removeAll( pHmiVar );
    }
    m_mutexWReq.unlock();*/

    return newUpdateVarFlag;
}

bool TcpModbusService::updateOnceRealReadReq( struct TcpModbusConnect* connect )
{
    bool newUpdateVarFlag = false;
    m_mutexOnceReal.lock();
    //int currTime = m_currTime;//getCurrTime();
    for ( int i = 0; i < connect->onceRealVarsList.count(); i++ )
    {                                  // 找出需要刷新的变量
        struct HmiVar* pHmiVar = connect->onceRealVarsList[i];
        if (( !connect->waitUpdateVarsList.contains( pHmiVar ) )
                        && ( !connect->waitWrtieVarsList.contains( pHmiVar ) ) //写的时候还是要去读该变量
                        //&& ( !connect->combinVarsList.contains( pHmiVar ) )    有可能被清除所以还是要加入等待读队列
                        && ( !connect->requestVarsList.contains( pHmiVar ) ) )
        {
            connect->waitUpdateVarsList.prepend( pHmiVar );  //插入队列最前面堆栈操作后进先读
            newUpdateVarFlag = true;
        }
        else
        {
            if ( connect->waitUpdateVarsList.contains( pHmiVar ) )
            {
                //提前他
                connect->waitUpdateVarsList.removeAll( pHmiVar );
                connect->waitUpdateVarsList.prepend( pHmiVar );
            }
        }
    }
    connect->onceRealVarsList.clear();// 清楚一次读列表
    m_mutexOnceReal.unlock();

    return newUpdateVarFlag;
}

void TcpModbusService::combineReadReq( struct TcpModbusConnect* connect )
{
    connect->combinVarsList.clear();
    if ( connect->waitUpdateVarsList.count() >= 1 )
    {
        struct HmiVar* pHmiVar = connect->waitUpdateVarsList[0];
        connect->combinVarsList.append( pHmiVar );
        connect->combinVar = *pHmiVar;
        if ( connect->combinVar.area == MODBUX_AREA_3X || connect->combinVar.area == MODBUX_AREA_4X )
        {
            if ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup )
            {
                Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
                connect->combinVar.dataType = Com_Word;
                connect->combinVar.len = 2 * (( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16 ); //2*count
                connect->combinVar.bitOffset = -1;
            }
        }
    }
    for ( int i = 1; i < connect->waitUpdateVarsList.count(); i++ )
    {
        struct HmiVar* pHmiVar = connect->waitUpdateVarsList[i];
        if ( pHmiVar->area == connect->combinVar.area && connect->combinVar.overFlow == pHmiVar->overFlow && ( pHmiVar->overFlow == false ) )
        {                            //没有越界才合并，否则不合并
            int startOffset = qMin( connect->combinVar.addOffset, pHmiVar->addOffset );
            int endOffset = 0;
            int newLen = 0;
            if ( connect->combinVar.area == MODBUX_AREA_3X || connect->combinVar.area == MODBUX_AREA_4X )
            {
                int combineCount = ( connect->combinVar.len + 1 ) / 2;
                int varCount = 0;

                if ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup )
                {
                    Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
                    varCount = ( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16;
                }
                else
                    varCount = ( pHmiVar->len + 1 ) / 2;

                endOffset = qMax( connect->combinVar.addOffset + combineCount, pHmiVar->addOffset + varCount );
                newLen = 2 * ( endOffset - startOffset );
                if ( newLen <= MAX_MBUS_PACKET_LEN )  //最多组合 MAX_MBUS_PACKET_LEN 字节
                {
                    if ( newLen <= 2*( combineCount + varCount ) + 10 ) //能节省10字节
                    {
                        connect->combinVar.addOffset = startOffset;
                        connect->combinVar.len = newLen;
                        connect->combinVarsList.append( pHmiVar );
                    }
                }
                else
                    continue;
            }
            else
            {
                endOffset = qMax( connect->combinVar.addOffset + connect->combinVar.len, pHmiVar->addOffset + pHmiVar->len );
                int newLen = endOffset - startOffset;
                if ( newLen <= MAX_MBUS_PACKET_LEN*8 )
                {
                    if ( newLen <= connect->combinVar.len + pHmiVar->len + 8*8 ) //能节省8字节
                    {
                        connect->combinVar.addOffset = startOffset;
                        connect->combinVar.len = newLen;
                        connect->combinVarsList.append( pHmiVar );
                    }
                }
                else
                    continue;
            }
        }
    }
}

bool TcpModbusService::hasProcessingReq( struct TcpModbusConnect* connect )
{
    return ( connect->writingCnt > 0 || connect->requestVarsList.count() > 0 );
}
/*
bool TcpModbusService::initCommDev()
{
    m_devUseCnt = 0;
#ifdef Q_WS_QWS
    m_fd = open( "/dev/modbus_dev", O_RDWR | O_NOCTTY | O_NONBLOCK );
    if ( m_fd < 0 )
    {
        qDebug() << "modbus_dev busy!\n";
        return false;
    }
    if ( m_connectList.count() > 0 )
    {
        struct TcpModbusConnect* conn = m_connectList[0];

#ifdef MINTERFACE_2
        if ( conn->cfg.COM_interface > 1 )
        {
            qDebug() << "no such COM_interface";
            return false;
        }
#endif
        ioctl( m_fd, conn->cfg.baud, (( conn->cfg.COM_interface + 1 ) << 24 ) | ( conn->cfg.dataBit << 16 ) | ( conn->cfg.parity << 8 ) | conn->cfg.stopBit );
        qDebug() << "open dev Ok!";
        return true;
    }
    else
        Q_ASSERT( false );
#endif
    return false;
}
*/
bool TcpModbusService::processRspData( struct TcpModbusConnect* connect, QHash<int, struct ComMsg> &comMsgTmp )
{
    bool result = false;

    int err = VAR_NO_ERR;

    MBUSMSG *msgPar = &connect->msgData;
    struct ComMsg comMsg;

    //printf("PM IN\n");
    //mbus_tcp_req__main(msgPar);
    if( msgPar->Done == 0  )
    {
        return result;
    }
    //printf("PM OUT\n");
    //printf("REQ F=0\n");
    if ( msgPar->Done == 1 && msgPar->ErrCode == TCP_MBUS_NO_ERROR )// read successful
    {
        connect->connStatus = CONNECT_LINKED_OK;
        err = VAR_NO_ERR;
        result = true;
        msgPar->Done = 0;
    }
    else if ( msgPar->Done == 1 && msgPar->ErrCode != TCP_MBUS_NO_ERROR )
    {
        result = true;
        //connect->processTime = 0;
        if ( msgPar->ErrCode == TCP_MBUS_ACTIVE_ERROR || msgPar->ErrCode == TCP_MBUS_TIMEOUT_ERROR  )
        {
            connect->connStatus = CONNECT_NO_LINK;
            //qDebug()<<"avtive err"<<CONNECT_NO_LINK<<" err code "<<msgPar->ErrCode;
            err = VAR_DISCONNECT;
            //printf("avtive err %d\n", msgPar->ErrCode);
        }
        else if ( msgPar->ErrCode == TCP_MBUS_RESPONSE_ERROR )
        {
            connect->connStatus = CONNECT_LINKED_OK;
            err = VAR_COMM_TIMEOUT;
        }
        else
        {
            //printf("read err %d\n", msgPar->ErrCode);
        }
        msgPar->Done = 0;
    }

    if ( result == true )
    {
        if ( m_devUseCnt > 0 )
            m_devUseCnt--;

        if ( connect->writingCnt > 0 && connect->waitWrtieVarsList.count() > 0 && msgPar->RW == 1 )
        {
            Q_ASSERT( msgPar->RW == 1 );
            connect->writingCnt = 0;
            struct HmiVar* pHmiVar = connect->waitWrtieVarsList.takeFirst(); //删除写请求记录
            m_mutexRw.lock();
            pHmiVar->wStatus = err;
            m_mutexRw.unlock();

            //加入消息队列
            comMsg.varId = ( pHmiVar->varId & 0x0000ffff ); //防止长变量
            comMsg.type = 1; // 1是write;
            comMsg.err = err;
            comMsgTmp.insertMulti( comMsg.varId, comMsg );
        }
        else if ( connect->requestVarsList.count() > 0 && msgPar->RW == 0 )
        {
            Q_ASSERT( msgPar->RW == 0 );
            //Q_ASSERT( pHmiVar->area == connect->requestVar.area );
            if ( msgPar->ErrCode == TCP_MBUS_RESPONSE_ERROR && connect->requestVarsList.count() > 1 )
            {
                for ( int j = 0; j < connect->requestVarsList.count(); j++ )
                {
                    struct HmiVar* pHmiVar = connect->requestVarsList[j];
                    pHmiVar->overFlow = true;
                    pHmiVar->lastUpdateTime = -1; //合并后越界的不算越界，立即再次更新（更新时不合并）
                }
            }
            else
            {
                for ( int j = 0; j < connect->requestVarsList.count(); j++ )
                {
                    m_mutexRw.lock();
                    struct HmiVar* pHmiVar = connect->requestVarsList[j];
                    Q_ASSERT( pHmiVar->area == pHmiVar->area );
                    pHmiVar->err = err;
                    /*if ( msgPar->ErrCode == VAR_PARAM_ERROR )
                    {
                        pHmiVar->overFlow = true;

                        //加入消息队列
                        comMsg.varId = ( pHmiVar->varId & 0x0000ffff ); //防止长变量
                        comMsg.type = 0; //0是read;
                        comMsg.err = VAR_PARAM_ERROR;
                        comMsgTmp.insert( comMsg.varId, comMsg );
                    }
                    else*/ if ( err == VAR_NO_ERR ) //没有写的时候才能更新
                    {
                        pHmiVar->overFlow = false;
                        if ( pHmiVar->area == MODBUX_AREA_3X || pHmiVar->area == MODBUX_AREA_4X )
                        {
                            Q_ASSERT( msgPar->Count < 120 );
                            int offset = pHmiVar->addOffset - connect->requestVar.addOffset;
                            Q_ASSERT( offset >= 0 );
                            Q_ASSERT( 2*offset <= 255 );
                            if ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup )
                            {
                                int len = 2 * (( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16 );
                                if ( pHmiVar->rData.size() < len )
                                    pHmiVar->rData = QByteArray( len, 0 ); //初始化为0
                                //pHmiVar->rData.resize(len);
                                if ( 2*offset + len < 256 ) //保证不越界
                                {
                                    memcpy( pHmiVar->rData.data(), &msgPar->DataPtr[2*offset], len );

                                    //加入消息队列
                                    if ( pHmiVar->wStatus != VAR_NO_OPERATION )
                                    {
                                        comMsg.varId = ( pHmiVar->varId & 0x0000ffff ); //防止长变量
                                        comMsg.type = 0; //0是read;
                                        comMsg.err = VAR_NO_ERR;
                                        comMsgTmp.insert( comMsg.varId, comMsg );
                                    }
                                }
                                else
                                    Q_ASSERT( false );
                            }
                            else
                            {
                                int len = 2 * (( pHmiVar->len + 1 ) / 2 );
                                if ( pHmiVar->rData.size() < len )
                                    pHmiVar->rData = QByteArray( len, 0 ); //初始化为0
                                //pHmiVar->rData.resize(len);
                                if ( 2*offset + len < 256 ) //保证不越界
                                {
                                    memcpy( pHmiVar->rData.data(), &msgPar->DataPtr[2*offset], len );

                                    //加入消息队列
                                    if ( pHmiVar->wStatus != VAR_NO_OPERATION )
                                    {
                                        comMsg.varId = ( pHmiVar->varId & 0x0000ffff ); //防止长变量
                                        comMsg.type = 0; //0是read;
                                        comMsg.err = VAR_NO_ERR;
                                        comMsgTmp.insert( comMsg.varId, comMsg );
                                    }
                                }
                                else
                                    Q_ASSERT( false );
                            }
                        }
                        else
                        {
                            Q_ASSERT( msgPar->Count < 240*8 );
                            if ( !connect->waitWrtieVarsList.contains( pHmiVar ) )
                            {
                                int startBitOffset = pHmiVar->addOffset - connect->requestVar.addOffset;
                                Q_ASSERT( startBitOffset >= 0 );
                                int len = ( pHmiVar->len + 7 ) / 8;
                                if ( pHmiVar->rData.size() < len )
                                    pHmiVar->rData = QByteArray( len, 0 ); //初始化为0
                                //pHmiVar->rData.resize(len);
                                for ( int k = 0; k < pHmiVar->len; k++ )
                                {
                                    int rOffset = startBitOffset + k;
                                    if ( msgPar->DataPtr[rOffset/8] & ( 1 << ( rOffset % 8 ) ) )
                                        pHmiVar->rData[k/8] = pHmiVar->rData[k/8] | ( 1 << ( k % 8 ) );
                                    else
                                        pHmiVar->rData[k/8] = pHmiVar->rData[k/8] & ( ~( 1 << ( k % 8 ) ) );
                                }

                                //加入消息队列
                                if ( pHmiVar->wStatus != VAR_NO_OPERATION )
                                {
                                    comMsg.varId = ( pHmiVar->varId & 0x0000ffff ); //防止长变量
                                    comMsg.type = 0; //0是read;
                                    comMsg.err = VAR_NO_ERR;
                                    comMsgTmp.insert( comMsg.varId, comMsg );
                                }
                            }
                        }
                    }
                    else if ( msgPar->ErrCode == TCP_MBUS_TIMEOUT_ERROR || msgPar->ErrCode == TCP_MBUS_ACTIVE_ERROR )
                    {
                        if ( pHmiVar->varId == 65535 )
                        {
                            //printf("Err %d\n", msgPar->ErrCode);
                            foreach( HmiVar* tempHmiVar, connect->actVarsList )
                            {
                                comMsg.type = 0; //0是read;
                                comMsg.err = VAR_DISCONNECT;
                                tempHmiVar->err = VAR_COMM_TIMEOUT;
                                tempHmiVar->lastUpdateTime = -1;
                                comMsg.varId = ( tempHmiVar->varId & 0x0000ffff );
                                comMsgTmp.insert( comMsg.varId, comMsg );
                            }
                        }
                        else
                        {
                            //printf("Err2 %d\n", msgPar->ErrCode);
                            comMsg.varId = ( pHmiVar->varId & 0x0000ffff ); //防止长变量
                            comMsg.type = 0; //0是read;
                            comMsg.err = VAR_COMM_TIMEOUT;
                            comMsgTmp.insert( comMsg.varId, comMsg );
                        }
                    }
                    m_mutexRw.unlock();
                }
            }
        }
        connect->requestVarsList.clear();
    }

    if(err == VAR_DISCONNECT)
    {
        for (int i=0; i<connect->actVarsList.count(); i++)
        {
            comMsg.varId = ( connect->actVarsList.at(i)->varId & 0x0000ffff ); //防止长变量
            comMsg.type = 0; // 1是write;
            comMsg.err = err;
            comMsgTmp.insert( comMsg.varId, comMsg );
        }
    }


    return result;
}


void TcpModbusService::slot_run()
{
    /*while ( 1 )
    {
        if ( initCommDev() )
        {
            msleep( 100 );            //初始化成功后，休眠100ms
            break;
        }
        msleep( 1000 );                //1m之后再尝试初始化设备
    }*/
    m_tcpModbus = new tcpModbus();
    int lastUpdateTime = -1;
    m_runTimer = new QTimer;
    connect(m_runTimer, SIGNAL(timeout()), this, SLOT(slot_timerHook()));
    m_runTimer->start(100);
    //printf("runing.....\n\n");
}

void TcpModbusService::slot_timerHook()
{
    m_runTimer->stop();

    int lastUpdateTime = -1;
    int connectCount = m_connectList.count();
    m_currTime = getCurrTime();
    int currTime = m_currTime;


    for ( int i = 0; i < connectCount; i++ )
    {                              //处理写请求
        struct TcpModbusConnect* connect = m_connectList[i];
        updateWriteReq( connect );
        trySndWriteReq( connect );
    }

    bool needUpdateFlag = false;

    if (( lastUpdateTime == -1 ) || ( calIntervalTime( lastUpdateTime, currTime ) >= 10 ) )
    {
        needUpdateFlag = true;     //该更新变量了
        lastUpdateTime = currTime;
        if ( m_comMsgTmp.count() > 0 )
        {
            m_mutexGetMsg.lock();
            foreach( int key, m_comMsgTmp.keys() )
            {
                m_comMsg.insert( key, m_comMsgTmp.value( key ) );
            }
            m_mutexGetMsg.unlock();
            m_comMsgTmp.clear();
        }
    }
    bool hasProcessingReqFlag = false;
    for ( int i = 0; i < connectCount; i++ )
    {
        struct TcpModbusConnect* connect = m_connectList[i];
        if ( needUpdateFlag )
        {
            //printf("updata  i = %d\n", i);
            updateOnceRealReadReq( connect );  //先重组一次更新变量
            updateReadReq( connect ); //无论如何都要重组请求变量
        }
        if ( !( hasProcessingReq( connect ) ) )
        {
            //printf("combine i = %d\n", i);
            combineReadReq( connect );
            trySndReadReq( connect );
        }
        //trySndReadReq( connect );

        if ( hasProcessingReq( connect ) )
        {
            //printf("process i = %d\n", i);
            hasProcessingReqFlag = true; //有正在处理的请求
            processRspData( connect, m_comMsgTmp ); //等待应答100ms
        }
    }
    if ( hasProcessingReqFlag )  //有已经发下去的请求
    {
        m_runTimer->start( 10 ); //留10ms处理
    }
    else
    {
        m_runTimer->start( 30 ); //休眠100ms
    }
    //回去继续尝试更新发送，直到1个也发送不下去
}
