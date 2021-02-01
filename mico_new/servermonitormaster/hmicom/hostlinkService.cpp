
#include "hostlinkService.h"


#define MAX_HLINK_PACKET_LEN  56  //最大一包56字节
HostlinkService::HostlinkService( struct HostlinkConnectCfg& connectCfg )
{
    struct HostlinkConnect* hlinkConnect = new HostlinkConnect;
    hlinkConnect->cfg = connectCfg;
    hlinkConnect->writingCnt = 0;
    hlinkConnect->connStatus = CONNECT_NO_OPERATION;
    hlinkConnect->connectTestVar.lastUpdateTime = -1;
    m_connectList.append( hlinkConnect );
    m_currTime = getCurrTime();
    m_runTimer = NULL;
    m_rwSocket = NULL;
}

HostlinkService::~HostlinkService()
{
    if(m_runTimer){
        m_runTimer->stop();
        delete m_runTimer;
    }
    if(m_rwSocket){
        delete m_rwSocket;
        m_rwSocket = NULL;
    }
}

bool HostlinkService::meetServiceCfg( int serviceType, int COM_interface )
{
    if ( serviceType == SERVICE_TYPE_HOSTLINK )
    {
        if ( m_connectList.count() > 0 )
        {
            if ( m_connectList[0]->cfg.COM_interface == COM_interface )
                return true;
        }
        else
            Q_ASSERT( false );
    }
    return false;
}

int HostlinkService::getConnLinkStatus( int connectId )
{
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct HostlinkConnect* hlinkConnect = m_connectList[i];
        if ( connectId == hlinkConnect->cfg.id )
            return hlinkConnect->connStatus;
    }
    return -1;
}

int HostlinkService::getComMsg( QList<struct ComMsg> &msgList )
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

int HostlinkService:: splitLongVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct HostlinkConnect* hlinkConnect = m_connectList[i];
        if ( pHmiVar->connectId == hlinkConnect->cfg.id )
        {
            pHmiVar->lastUpdateTime = -1; //立即更新

            if ( pHmiVar->area == HOST_LINK_AREA_IO ||
                            pHmiVar->area == HOST_LINK_AREA_HR ||
                            pHmiVar->area == HOST_LINK_AREA_AR ||
                            pHmiVar->area == HOST_LINK_AREA_LR ||
                            pHmiVar->area == HOST_LINK_AREA_DM ||
                            pHmiVar->area == HOST_LINK_AREA_TCVAL )
            {
                if ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup )
                {
                    Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
                    if ( pHmiVar->len > MAX_HLINK_PACKET_LEN*8 && pHmiVar->splitVars.count() <= 0 )  //拆分
                    {
                        for ( int splitedLen = 0; splitedLen < pHmiVar->len; )
                        {
                            int len = pHmiVar->len - splitedLen;
                            if ( len > MAX_HLINK_PACKET_LEN*8 )
                                len = MAX_HLINK_PACKET_LEN * 8;

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
                            pSplitHhiVar->wStatus = VAR_NO_ERR;
                            pSplitHhiVar->lastUpdateTime = -1; //立即更新

                            splitedLen += len;
                            pHmiVar->splitVars.append( pSplitHhiVar );
                        }
                    }
                }
                else
                {
                    if ( pHmiVar->len > MAX_HLINK_PACKET_LEN && pHmiVar->splitVars.count() <= 0 )  //拆分
                    {
                        for ( int splitedLen = 0; splitedLen < pHmiVar->len; )
                        {
                            int len = pHmiVar->len - splitedLen;
                            if ( len > MAX_HLINK_PACKET_LEN )
                                len = MAX_HLINK_PACKET_LEN;

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
                            pSplitHhiVar->wStatus = VAR_NO_ERR;
                            pSplitHhiVar->lastUpdateTime = -1; //立即更新

                            splitedLen += len;
                            pHmiVar->splitVars.append( pSplitHhiVar );
                        }
                    }
                }
            }
            else if ( pHmiVar->area == HOST_LINK_AREA_TCBIT )
            {   /* hostlink协议一次最多可以读写112个位单位，所以不能超过MAX_HLINK_PACKET_LEN*2 */
                if ( pHmiVar->len > MAX_HLINK_PACKET_LEN*2 && pHmiVar->splitVars.count() <= 0 )  //拆分
                {
                    for ( int splitedLen = 0; splitedLen < pHmiVar->len; )
                    {
                        int len = pHmiVar->len - splitedLen;
                        if ( len > MAX_HLINK_PACKET_LEN*2 )
                            len = MAX_HLINK_PACKET_LEN * 2;

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
                        pSplitHhiVar->wStatus = VAR_NO_ERR;
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
int HostlinkService::onceRealUpdateVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    m_mutexOnceReal.lock();
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct HostlinkConnect* hlinkConnect = m_connectList[i];
        if ( pHmiVar->connectId == hlinkConnect->cfg.id )
        {
            pHmiVar->lastUpdateTime = -1;
            if ( pHmiVar->splitVars.count() > 0 )
            {
                for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
                {
                    struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                    pSplitHhiVar->lastUpdateTime = -1;
                    pSplitHhiVar->overFlow = false;
                    if ( !hlinkConnect->onceRealVarsList.contains( pSplitHhiVar ) )
                        hlinkConnect->onceRealVarsList.append( pSplitHhiVar );
                }
            }
            else
            {
                pHmiVar->lastUpdateTime = -1;
                pHmiVar->overFlow = false;
                if ( !hlinkConnect->onceRealVarsList.contains( pHmiVar ) )
                    hlinkConnect->onceRealVarsList.append( pHmiVar );
            }

            m_mutexOnceReal.unlock();
            return 0;
        }
    }
    m_mutexOnceReal.unlock();
    return -1;
}
int HostlinkService::startUpdateVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );

    m_mutexUpdateVar.lock();
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct HostlinkConnect* hlinkConnect = m_connectList[i];
        if ( pHmiVar->connectId == hlinkConnect->cfg.id )
        {
            pHmiVar->lastUpdateTime = -1;
            if ( pHmiVar->splitVars.count() > 0 )
            {
                for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
                {
                    struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                    pSplitHhiVar->lastUpdateTime = -1;
                    pSplitHhiVar->overFlow = false;
                    if ( !hlinkConnect->actVarsList.contains( pSplitHhiVar ) )
                        hlinkConnect->actVarsList.append( pSplitHhiVar );
                }
            }
            else
            {
                pHmiVar->lastUpdateTime = -1;
                pHmiVar->overFlow = false;
                if ( !hlinkConnect->actVarsList.contains( pHmiVar ) )
                    hlinkConnect->actVarsList.append( pHmiVar );
            }

            m_mutexUpdateVar.unlock();
            return 0;
        }
    }
    m_mutexUpdateVar.unlock();
    return -1;
}

int HostlinkService::stopUpdateVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    m_mutexUpdateVar.lock();
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct HostlinkConnect* hlinkConnect = m_connectList[i];
        if ( pHmiVar->connectId == hlinkConnect->cfg.id )
        {
            if ( pHmiVar->splitVars.count() > 0 )
            {
                for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
                {
                    struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                    hlinkConnect->actVarsList.removeAll( pSplitHhiVar );
                }
            }
            else
                hlinkConnect->actVarsList.removeAll( pHmiVar );
            m_mutexUpdateVar.unlock();
            return 0;
        }
    }
    m_mutexUpdateVar.unlock();
    return -1;
}

int HostlinkService::readVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    if ( pHmiVar->err == VAR_NO_ERR )  //无错误才拷贝数据
    {
        if ( pHmiVar->area == HOST_LINK_AREA_IO ||
                        pHmiVar->area == HOST_LINK_AREA_HR ||
                        pHmiVar->area == HOST_LINK_AREA_AR ||
                        pHmiVar->area == HOST_LINK_AREA_LR ||
                        pHmiVar->area == HOST_LINK_AREA_DM ||
                        pHmiVar->area == HOST_LINK_AREA_TCVAL )
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

                        if ( byteOffset & 0x01 )
                            byteOffset--;
                        else
                            byteOffset++;

                        int bitOffset = ( pHmiVar->bitOffset + j ) % 8;
                        if ( pHmiVar->rData.at( byteOffset ) & ( 1 << bitOffset ) )
                            pBuf[j/8] |= ( 1 << ( j % 8 ) );
                        else
                            pBuf[j/8] &= ~( 1 << ( j % 8 ) );
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
                        if ( pHmiVar->dataType != Com_String && pHmiVar->area != HOST_LINK_AREA_TCVAL )
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
        else if ( pHmiVar->area == HOST_LINK_AREA_TCBIT )
        {
            int dataBytes = ( pHmiVar->len + 7 ) / 8; //字节数
            if ( bufLen >= dataBytes && pHmiVar->rData.size() >= dataBytes )
                memcpy( pBuf, pHmiVar->rData.data(), dataBytes );
            else
                Q_ASSERT( false );
            //qDebug() << pHmiVar->area << pHmiVar->addOffset << "buff[0]" << (int)pBuf[0];
        }
        else if ( pHmiVar->area == HOST_LINK_AREA_CPUSTATUS ) /* PC type 和PC model直接复制一个字节*/
        {
            if ( bufLen >= pHmiVar->len &&  pHmiVar->rData.size() >= pHmiVar->len &&  pHmiVar->len > 0 )
            {
                quint8 status = pHmiVar->rData.at( 0 );
                status &= 0x03;
                pBuf[0] = ( status >> ( pHmiVar->addOffset ) ) & 0x01;
            }
            else
                Q_ASSERT( false );

        }
        else
        {
            if ( bufLen >= pHmiVar->len && pHmiVar->rData.size() >= pHmiVar->len &&  pHmiVar->len > 0 )
                memcpy( pBuf, pHmiVar->rData.data(), 1 );
        }
    }
    return pHmiVar->err;
}

int HostlinkService::readDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    Q_ASSERT( pHmiVar && pBuf );
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct HostlinkConnect* hlinkConnect = m_connectList[i];
        if ( pHmiVar->connectId == hlinkConnect->cfg.id )
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

int HostlinkService::writeVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    int ret;

    Q_ASSERT( pHmiVar && pBuf );
    m_mutexRw.lock();
    if ( pHmiVar->area == HOST_LINK_AREA_IO ||
                    pHmiVar->area == HOST_LINK_AREA_HR ||
                    pHmiVar->area == HOST_LINK_AREA_AR ||
                    pHmiVar->area == HOST_LINK_AREA_LR ||
                    pHmiVar->area == HOST_LINK_AREA_DM ||
                    pHmiVar->area == HOST_LINK_AREA_TCVAL )
    {                            //大小数等到发送写命令时再处理
        if ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup )
        {                        //位变量长度为位的个数
            Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
            int count = ( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16;  //字数
            Q_ASSERT( count > 0 && count <= MAX_HLINK_PACKET_LEN / 2 );
            if ( count > MAX_HLINK_PACKET_LEN / 2 )
                count = MAX_HLINK_PACKET_LEN / 2;
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
                int byteW = i / 8;
                int bitW = i % 8;
                int byteR = ( pHmiVar->bitOffset + i ) / 8;

                if ( byteR & 0x01 )
                    byteR--;
                else
                    byteR++;

                int bitR = ( pHmiVar->bitOffset + i ) % 8;
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
            Q_ASSERT( count > 0 && count <= MAX_HLINK_PACKET_LEN / 2 );
            if ( count > MAX_HLINK_PACKET_LEN / 2 )
                count = MAX_HLINK_PACKET_LEN / 2;
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
                    if ( pHmiVar->dataType != Com_String && pHmiVar->area != HOST_LINK_AREA_TCVAL )
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
    else if ( pHmiVar->area == HOST_LINK_AREA_TCBIT )
    {
        Q_ASSERT( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup );
        int count = pHmiVar->len;
        Q_ASSERT( count > 0 &&  count <= MAX_HLINK_PACKET_LEN*2 );
        if ( count > MAX_HLINK_PACKET_LEN*2 )
            count = MAX_HLINK_PACKET_LEN * 2;
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
    else if ( pHmiVar->area == HOST_LINK_AREA_CPUSTATUS ) //PC model 写的时候也只复制一个字节 PC type 不能写
    {
        Q_ASSERT( bufLen > 0 );

        pBuf[0] = pBuf[0] & 0x01;
        quint8 status = pHmiVar->rData.at( 0 );

        if ((( status >> ( pHmiVar->addOffset ) ) & 0x01 ) == ( pBuf[0] & 0x01 ) ) //write old status
        {
            if ( status == 2 )
                pBuf[0] = 3;
            else if ( status == 3 )
                pBuf[0] = 2;
            else
                pBuf = 0;
        }
        else
        {
            if ( pHmiVar->addOffset == 0 )
            {
                if (( pBuf[0] & 0x01 ) == 1 ) //0-1 monitor
                    pBuf[0] = 2;
                else //1-0 program
                    pBuf[0] = 0;
            }
            else
            {
                if (( pBuf[0] & 0x01 ) == 1 )  //0-1 run
                    pBuf[0] = 3;
                else //1-0 program
                    pBuf[0] = 0;
            }
        }

        int dataBytes = qMin( pHmiVar->len, bufLen );
        if ( pHmiVar->wData.size() < dataBytes )
            pHmiVar->wData.resize( dataBytes );
        memcpy( pHmiVar->wData.data(), pBuf, dataBytes );
        pHmiVar->lastUpdateTime = -1;

        //if ( pHmiVar->rData.size() < dataBytes )
        //pHmiVar->rData.resize( dataBytes );
        //memcpy( pHmiVar->rData.data(), pHmiVar->wData.data(), dataBytes );
    }
    else
    {
        Q_ASSERT( false );
    }
    pHmiVar->wStatus = VAR_NO_OPERATION;
    ret = pHmiVar->err;
    m_mutexRw.unlock();

    return ret; //是否要返回成功
}

int HostlinkService::writeDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    Q_ASSERT( pHmiVar && pBuf );

    if ( pHmiVar->area == HOST_LINK_AREA_CUPTYPE ) //pc type not surport write
        return HOSTLINK_PARM_ERR;

    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct HostlinkConnect* hlinkConnect = m_connectList[i];
        if ( pHmiVar->connectId == hlinkConnect->cfg.id )
        {
            if ( hlinkConnect->connStatus != CONNECT_LINKED_OK )
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
                    hlinkConnect->waitWrtieVarsList.append( pSplitHhiVar );
                    pBuf += len;
                    bufLen -= len;
                }
            }
            else
            {
                ret = writeVar( pHmiVar, pBuf, bufLen );
                hlinkConnect->waitWrtieVarsList.append( pHmiVar );
            }
            m_mutexWReq.unlock();

            return ret; //是否要返回成功
        }
    }
    return -1;
}

int HostlinkService::addConnect( struct HostlinkConnectCfg& connectCfg )
{                                      //要检查是否重复 add
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct HostlinkConnect* hlinkConnect = m_connectList[i];
        Q_ASSERT( hlinkConnect->cfg.COM_interface == connectCfg.COM_interface );//绝对要是同一个设备接口
        if ( connectCfg.id == hlinkConnect->cfg.id )
        {
            Q_ASSERT( false );
            return 0;
        }
    }
    struct HostlinkConnect* hlinkConnect = new HostlinkConnect;
    hlinkConnect->cfg = connectCfg;
    hlinkConnect->writingCnt = 0;
    hlinkConnect->connStatus = CONNECT_NO_OPERATION;
    hlinkConnect->connectTestVar.lastUpdateTime = -1;
    m_connectList.append( hlinkConnect );
    return 0;
}

int HostlinkService::sndWriteVarMsgToDriver( struct HostlinkConnect* hlinkConnect, struct HmiVar* pHmiVar )
{
    int sendedLen = 0;
    Q_ASSERT( hlinkConnect && pHmiVar );
//#ifdef Q_WS_QWS
    struct HlinkMsgPar msgPar;

    m_mutexRw.lock();
    msgPar.rw = 1;   //写
    msgPar.slave = hlinkConnect->cfg.cpuAddr;
    msgPar.area = pHmiVar->area;
    msgPar.addr = pHmiVar->addOffset;
    msgPar.err = 0;

    if ( msgPar.area == HOST_LINK_AREA_IO ||
                    msgPar.area == HOST_LINK_AREA_HR ||
                    msgPar.area == HOST_LINK_AREA_AR ||
                    msgPar.area == HOST_LINK_AREA_LR ||
                    msgPar.area == HOST_LINK_AREA_DM ||
                    msgPar.area == HOST_LINK_AREA_TCVAL )
    {
        if ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup )
        {                              // 位变量长度为位的个数
            Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
            int count = ( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16;  //字数
            Q_ASSERT( count > 0 && count <= MAX_HLINK_PACKET_LEN / 2 );
            if ( count > MAX_HLINK_PACKET_LEN / 2 )
                count = MAX_HLINK_PACKET_LEN / 2;
            msgPar.count = count;      // 占用的总字数
            int dataBytes = 2 * count;
            if ( pHmiVar->rData.size() < dataBytes )
                pHmiVar->rData.resize( dataBytes );

            for ( int i = 0; i < pHmiVar->len; i++ )
            {
                int byteW = i / 8;
                int bitW = i % 8;
                int byteR = ( pHmiVar->bitOffset + i ) / 8;

                if ( byteR & 0x01 )
                    byteR--;
                else
                    byteR++;

                int bitR = ( pHmiVar->bitOffset + i ) % 8;

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
            memcpy( msgPar.data, pHmiVar->rData.data(), dataBytes );
            sendedLen = count * 16;
        }
        else
        {
            int count = ( pHmiVar->len + 1 ) / 2; //字数
            Q_ASSERT( count > 0 && count <= MAX_HLINK_PACKET_LEN / 2 );
            if ( count > MAX_HLINK_PACKET_LEN / 2 )
                count = MAX_HLINK_PACKET_LEN / 2;
            msgPar.count = count;      // 占用的总字数
            int dataBytes = 2 * count;
            if ( pHmiVar->rData.size() < dataBytes )
                pHmiVar->rData.resize( dataBytes );

            for ( int j = 0; j < pHmiVar->len; j++ )
            {                          // 奇偶字节调换
                if ( j < pHmiVar->wData.size() && j < pHmiVar->rData.size() )
                {
                    if ( pHmiVar->dataType != Com_String && pHmiVar->area != HOST_LINK_AREA_TCVAL )
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
            memcpy( msgPar.data, pHmiVar->rData.data(), dataBytes );
            sendedLen = count * 2;
        }
    }
    else if ( msgPar.area == HOST_LINK_AREA_TCBIT )
    {                                  // 位变量长度为位的个数
        Q_ASSERT( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup );
        int count = pHmiVar->len;
        Q_ASSERT( count > 0 &&  count <= MAX_HLINK_PACKET_LEN*2 );
        if ( count > MAX_HLINK_PACKET_LEN*2 )
            count = MAX_HLINK_PACKET_LEN * 2;
        msgPar.count = count;
        int dataBytes = ( count + 7 ) / 8;
        if ( pHmiVar->rData.size() < dataBytes )
            pHmiVar->rData.resize( dataBytes );

        memcpy( pHmiVar->rData.data(), pHmiVar->wData.data(), qMin( dataBytes, pHmiVar->wData.size() ) );
        memcpy( msgPar.data, pHmiVar->rData.data(), dataBytes );
        sendedLen = count;
    }
    else if ( msgPar.area == HOST_LINK_AREA_CPUSTATUS || msgPar.area == HOST_LINK_AREA_CUPTYPE )
    {
        if ( pHmiVar->rData.size() < pHmiVar->len )
            pHmiVar->rData.resize( pHmiVar->len );

        msgPar.count = 0;
        //memcpy( pHmiVar->rData.data(), pHmiVar->wData.data(), pHmiVar->len ); //PC model PC type 只要固定一个字节表示
        memcpy( msgPar.data, pHmiVar->wData.data(), pHmiVar->len );
        sendedLen = pHmiVar->len;
    }
    else
    {
        msgPar.count = 0;
        Q_ASSERT( false );
    }

    if ( sendedLen > 0 )
    {
        m_devUseCnt++;
        m_mutexRw.unlock();
        //int len = write( m_fd, ( char* ) & msgPar, sizeof( struct HlinkMsgPar ) );
        int len = m_rwSocket->writeDev(( char* ) & msgPar, sizeof( struct HlinkMsgPar ));

        Q_ASSERT( len > 0 );
    }
    else
        m_mutexRw.unlock();
//#endif
    return sendedLen;
}

int HostlinkService::sndReadVarMsgToDriver( struct HostlinkConnect* hlinkConnect, struct HmiVar* pHmiVar )
{
    int sendedLen = 0;
    Q_ASSERT( hlinkConnect && pHmiVar );
//#ifdef Q_WS_QWS
    struct HlinkMsgPar msgPar;

    m_mutexRw.lock();
    msgPar.rw = 0;    // 读
    msgPar.slave = hlinkConnect->cfg.cpuAddr;
    msgPar.area = pHmiVar->area;
    msgPar.addr = pHmiVar->addOffset;
    msgPar.err = 0;

    if ( msgPar.area == HOST_LINK_AREA_IO ||
                    msgPar.area == HOST_LINK_AREA_HR ||
                    msgPar.area == HOST_LINK_AREA_AR ||
                    msgPar.area == HOST_LINK_AREA_LR ||
                    msgPar.area == HOST_LINK_AREA_DM ||
                    msgPar.area == HOST_LINK_AREA_TCVAL )
    {
        if ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup )
        {                              // 位变量长度为位的个数
            Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
            int count = ( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16;  //字数
            Q_ASSERT( count > 0 && count <= MAX_HLINK_PACKET_LEN / 2 );
            if ( count > MAX_HLINK_PACKET_LEN / 2 )
                count = MAX_HLINK_PACKET_LEN / 2;
            msgPar.count = count;      // 占用的总字数
            sendedLen = count * 16;
        }
        else
        {
            int count = ( pHmiVar->len + 1 ) / 2;  //字数
            Q_ASSERT( count > 0 && count <= MAX_HLINK_PACKET_LEN / 2 );
            if ( count > MAX_HLINK_PACKET_LEN / 2 )
                count = MAX_HLINK_PACKET_LEN / 2;
            msgPar.count = count;      // 占用的总字数
            sendedLen = count * 2;
        }
    }
    else if ( msgPar.area == HOST_LINK_AREA_TCBIT )
    {                                  // 位变量长度为位的个数
        Q_ASSERT( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup );
        int count = pHmiVar->len;
        Q_ASSERT( count > 0 &&  count <= MAX_HLINK_PACKET_LEN*2 );
        if ( count > MAX_HLINK_PACKET_LEN*2 )
            count = MAX_HLINK_PACKET_LEN * 2;
        msgPar.count = count;
        sendedLen = count;
    }
    else if ( msgPar.area == HOST_LINK_AREA_CPUSTATUS || msgPar.area == HOST_LINK_AREA_CUPTYPE )
    {
        msgPar.count = 0;//PC model PC type 读的东西是固定的不需要单元数
        sendedLen = 1; //实际上读出来的东西需要一个字节
    }
    else
    {
        Q_ASSERT( false );
        msgPar.count = 0;
        sendedLen = 0;
    }

    if ( sendedLen > 0 )
    {
        m_devUseCnt++;
        m_mutexRw.unlock();
        //int len = write( m_fd, ( char* ) & msgPar, sizeof( struct HlinkMsgPar ) );
        int len = m_rwSocket->writeDev( ( char* ) & msgPar, sizeof( struct HlinkMsgPar ));

        Q_ASSERT( len > 0 );
    }
    else
        m_mutexRw.unlock();
//#endif
    return sendedLen;
}

bool isWaitingReadRsp( struct HostlinkConnect* hlinkConnect, struct HmiVar* pHmiVar )
{
    Q_ASSERT( hlinkConnect && pHmiVar );
    if ( hlinkConnect->waitUpdateVarsList.contains( pHmiVar ) )
        return true;
    else if ( hlinkConnect->combinVarsList.contains( pHmiVar ) )
        return true;
    else if ( hlinkConnect->requestVarsList.contains( pHmiVar ) )
        return true;
    else
        return false;
}

bool HostlinkService::trySndWriteReq( struct HostlinkConnect* hlinkConnect ) //不处理超长变量
{
    if ( m_devUseCnt < 1 )                          //没有正在读写, snd read, 一次只能发送一包请求
    {
        if ( hlinkConnect->writingCnt == 0 && hlinkConnect->requestVarsList.count() == 0 ) //这个连接没有读写
        {
            m_mutexWReq.lock();
            if ( hlinkConnect->waitWrtieVarsList.count() > 0 )
            {
                struct HmiVar* pHmiVar = hlinkConnect->waitWrtieVarsList[0];
                if (( pHmiVar->lastUpdateTime == -1 )  //3X,4X 位变量需要先读
                                && ( pHmiVar->area == HOST_LINK_AREA_IO ||
                                     pHmiVar->area == HOST_LINK_AREA_HR ||
                                     pHmiVar->area == HOST_LINK_AREA_AR ||
                                     pHmiVar->area == HOST_LINK_AREA_LR ||
                                     pHmiVar->area == HOST_LINK_AREA_DM ||
                                     pHmiVar->area == HOST_LINK_AREA_TCVAL )
                                && ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup || pHmiVar->dataType == Com_String ) )
                {
                    pHmiVar->lastUpdateTime = m_currTime;//getCurrTime();
                    if ( !isWaitingReadRsp( hlinkConnect, pHmiVar ) )
                        hlinkConnect->waitUpdateVarsList.prepend( pHmiVar ); //加入到读队列最前,保证下次最先读
                }
                else if ( !isWaitingReadRsp( hlinkConnect, pHmiVar ) ) //没有正在等待读
                {                                   //直接写,snd write cmd to driver
                    if ( sndWriteVarMsgToDriver( hlinkConnect, pHmiVar ) > 0 )
                    {
                        pHmiVar->lastUpdateTime = m_currTime;//getCurrTime();
                        hlinkConnect->writingCnt = 1;    //进入写状态
                        m_mutexWReq.unlock();
                        return true;
                    }
                    else
                    {
                        hlinkConnect->waitWrtieVarsList.takeAt( 0 );//出列,丢掉写请求
                        pHmiVar->err = VAR_PARAM_ERROR;
                    }
                }
            }
            m_mutexWReq.unlock();
        }
    }
    return false;
}

bool HostlinkService::trySndReadReq( struct HostlinkConnect* hlinkConnect ) //不处理超长变量
{
    if ( m_devUseCnt <  MAXDEVCNT )             //没有正在读写, snd read, 一次只能发送一包请求
    {
        //qDebug()<<"trySndReadReq hlinkConnect->combinVarsList.count() = "<<hlinkConnect->combinVarsList.count();
        if ( hlinkConnect->writingCnt == 0 && hlinkConnect->requestVarsList.count() == 0 ) //这个连接没有读写
        {
            if ( hlinkConnect->combinVarsList.count() > 0 )
            {
                hlinkConnect->requestVarsList = hlinkConnect->combinVarsList;
                hlinkConnect->requestVar = hlinkConnect->combinVar;
                for ( int i = 0; i < hlinkConnect->combinVarsList.count(); i++ )
                {                      //要把已经请求的变量从等待队列中删除
                    struct HmiVar* pHmiVar = hlinkConnect->combinVarsList[i];
                    hlinkConnect->waitUpdateVarsList.removeAll( pHmiVar );
                    pHmiVar->lastUpdateTime = m_currTime;
                }
                hlinkConnect->combinVarsList.clear();

                if ( sndReadVarMsgToDriver( hlinkConnect, &( hlinkConnect->requestVar ) ) > 0 )
                {
                    return true;
                }
                else
                {
                    for ( int i = 0; i < hlinkConnect->requestVarsList.count(); i++ )
                    {
                        struct HmiVar* pHmiVar = hlinkConnect->requestVarsList[i];
                        pHmiVar->err = VAR_PARAM_ERROR;
                    }
                    hlinkConnect->requestVarsList.clear();
                }
            }
        }
    }
    return false;
}

bool HostlinkService::updateWriteReq( struct HostlinkConnect* hlinkConnect )
{
    bool newUpdateVarFlag = false;
    QList<struct HmiVar*> tmpList;
    m_mutexWReq.lock();
    //int currTime = m_currTime;//getCurrTime();
    for ( int i = 0; i < hlinkConnect->wrtieVarsList.count(); i++ )
    {
        struct HmiVar* pHmiVar = hlinkConnect->wrtieVarsList[i];


        if ( !hlinkConnect->waitWrtieVarsList.contains( pHmiVar ) )
        {
            /*if ( hlinkConnect->waitUpdateVarsList.contains( pHmiVar ) )
                hlinkConnect->waitUpdateVarsList.removeAll( pHmiVar );

            if ( hlinkConnect->requestVarsList.contains( pHmiVar )  )
                hlinkConnect->requestVarsList.removeAll( pHmiVar );*/

            hlinkConnect->waitWrtieVarsList.append( pHmiVar );
            newUpdateVarFlag = true;
        }
        else
        {
            tmpList.append(pHmiVar);
        }
    }

    hlinkConnect->wrtieVarsList.clear() ;
    hlinkConnect->wrtieVarsList += tmpList;

    m_mutexWReq.unlock();

    return newUpdateVarFlag;
}
bool HostlinkService::updateReadReq( struct HostlinkConnect* hlinkConnect )
{
    bool newUpdateVarFlag = false;
    m_mutexUpdateVar.lock();
    int currTime = m_currTime;//getCurrTime();
    if ( hlinkConnect->connStatus == CONNECT_LINKED_OK )
    {
        for ( int i = 0; i < hlinkConnect->actVarsList.count(); i++ )
        {                                  // 找出需要刷新的变量
            struct HmiVar* pHmiVar = hlinkConnect->actVarsList[i];
            if (( pHmiVar->lastUpdateTime == -1 ) || ( calIntervalTime( pHmiVar->lastUpdateTime, currTime ) >= pHmiVar->cycleTime ) )
            {                              // 需要刷新
                if (( !hlinkConnect->waitUpdateVarsList.contains( pHmiVar ) )
                                && ( !hlinkConnect->waitWrtieVarsList.contains( pHmiVar ) )
                                //&& ( !hlinkConnect->combinVarsList.contains( pHmiVar ) )
                                && ( !hlinkConnect->requestVarsList.contains( pHmiVar ) ) )
                {
                    //pHmiVar->lastUpdateTime = currTime;
                    hlinkConnect->waitUpdateVarsList.append( pHmiVar );
                    newUpdateVarFlag = true;
                }
            }
        }
    }
    else
    {
        hlinkConnect->waitUpdateVarsList.clear();
        //qDebug()<<"hlinkConnect->actVarsList.count() = "<<hlinkConnect->actVarsList.count()<<"    hlinkConnect->requestVarsList.count() =  "<<hlinkConnect->requestVarsList.count();
        if ( hlinkConnect->actVarsList.count() > 0 && hlinkConnect->requestVarsList.count() == 0 )
        {//qDebug()<<"connectTestVar  lastUpdateTime  = "<<hlinkConnect->connectTestVar.lastUpdateTime;
            hlinkConnect->actVarsList[0]->lastUpdateTime = hlinkConnect->connectTestVar.lastUpdateTime;
            hlinkConnect->connectTestVar = *( hlinkConnect->actVarsList[0] );
            hlinkConnect->connectTestVar.varId = 65535;
            hlinkConnect->connectTestVar.cycleTime = 2000;
            //qDebug()<<"hlinkConnect->connectTestVar.area = "<<hlinkConnect->connectTestVar.area;
            if (( hlinkConnect->connectTestVar.lastUpdateTime == -1 )
                            || ( calIntervalTime( hlinkConnect->connectTestVar.lastUpdateTime, currTime ) >= hlinkConnect->connectTestVar.cycleTime ) )
            {
                if (( !hlinkConnect->waitUpdateVarsList.contains( &( hlinkConnect->connectTestVar ) ) )
                                && ( !hlinkConnect->waitWrtieVarsList.contains( &( hlinkConnect->connectTestVar ) ) )
                                && ( !hlinkConnect->requestVarsList.contains( &( hlinkConnect->connectTestVar ) ) ) )
                {
                    hlinkConnect->waitUpdateVarsList.append( &( hlinkConnect->connectTestVar ) );
                    //qDebug()<<"updateReadReq hlinkConnect->waitUpdateVarsList.count() = "<<hlinkConnect->waitUpdateVarsList.count();
                    newUpdateVarFlag = true;
                }
            }
        }
    }
    m_mutexUpdateVar.unlock();

    /*m_mutexWReq.lock();
    for ( int i = 0; i < hlinkConnect->waitWrtieVarsList.count(); i++ )
    {
        struct HmiVar* pHmiVar = hlinkConnect->waitWrtieVarsList[i];

        if ( hlinkConnect->waitUpdateVarsList.contains( pHmiVar ) )
            hlinkConnect->waitUpdateVarsList.removeAll( pHmiVar );

        if ( hlinkConnect->requestVarsList.contains( pHmiVar )  )
            hlinkConnect->requestVarsList.removeAll( pHmiVar );
    }
    m_mutexWReq.unlock();*/
    return newUpdateVarFlag;
}

bool HostlinkService::updateOnceRealReadReq( struct HostlinkConnect* hlinkConnect )
{
    bool newUpdateVarFlag = false;
    m_mutexOnceReal.lock();
    //int currTime = m_currTime;//getCurrTime();
    for ( int i = 0; i < hlinkConnect->onceRealVarsList.count(); i++ )
    {
        struct HmiVar* pHmiVar = hlinkConnect->onceRealVarsList[i];
        if (( !hlinkConnect->waitUpdateVarsList.contains( pHmiVar ) )
                        && ( !hlinkConnect->requestVarsList.contains( pHmiVar ) ) )
        {
            hlinkConnect->waitUpdateVarsList.prepend( pHmiVar );
            newUpdateVarFlag = true;
        }
        else
        {
            if ( hlinkConnect->waitUpdateVarsList.contains( pHmiVar ) )
            {
                hlinkConnect->waitUpdateVarsList.removeAll( pHmiVar );
                hlinkConnect->waitUpdateVarsList.prepend( pHmiVar );
            }
        }
    }
    hlinkConnect->onceRealVarsList.clear();
    m_mutexOnceReal.unlock();

    return newUpdateVarFlag;
}
void HostlinkService::combineReadReq( struct HostlinkConnect* hlinkConnect )
{
    hlinkConnect->combinVarsList.clear();
    if ( hlinkConnect->waitUpdateVarsList.count() >= 1 )
    {
        struct HmiVar* pHmiVar = hlinkConnect->waitUpdateVarsList[0];
        hlinkConnect->combinVarsList.append( pHmiVar );
        hlinkConnect->combinVar = *pHmiVar;
        if ( hlinkConnect->combinVar.area == HOST_LINK_AREA_IO ||
                        hlinkConnect->combinVar.area == HOST_LINK_AREA_HR ||
                        hlinkConnect->combinVar.area == HOST_LINK_AREA_AR ||
                        hlinkConnect->combinVar.area == HOST_LINK_AREA_LR ||
                        hlinkConnect->combinVar.area == HOST_LINK_AREA_DM ||
                        hlinkConnect->combinVar.area == HOST_LINK_AREA_TCVAL )
        {
            if ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup )
            {
                Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
                hlinkConnect->combinVar.dataType = Com_Word;
                hlinkConnect->combinVar.len = 2 * (( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16 ); //2*count
                hlinkConnect->combinVar.bitOffset = -1;
            }
        }
    }
    for ( int i = 1; i < hlinkConnect->waitUpdateVarsList.count(); i++ )
    {
        struct HmiVar* pHmiVar = hlinkConnect->waitUpdateVarsList[i];
        if ( pHmiVar->area == hlinkConnect->combinVar.area && hlinkConnect->combinVar.overFlow == pHmiVar->overFlow && ( pHmiVar->overFlow == false ) )
        {                            //没有越界才合并，否则不合并
            int startOffset = qMin( hlinkConnect->combinVar.addOffset, pHmiVar->addOffset );
            int endOffset = 0;
            int newLen = 0;
            if ( hlinkConnect->combinVar.area == HOST_LINK_AREA_IO ||
                            hlinkConnect->combinVar.area == HOST_LINK_AREA_HR ||
                            hlinkConnect->combinVar.area == HOST_LINK_AREA_AR ||
                            hlinkConnect->combinVar.area == HOST_LINK_AREA_LR ||
                            hlinkConnect->combinVar.area == HOST_LINK_AREA_DM ||
                            hlinkConnect->combinVar.area == HOST_LINK_AREA_TCVAL )
            {
                int combineCount = ( hlinkConnect->combinVar.len + 1 ) / 2;
                int varCount = 0;

                if ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup )
                {
                    Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
                    varCount = ( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16;
                }
                else
                    varCount = ( pHmiVar->len + 1 ) / 2;

                endOffset = qMax( hlinkConnect->combinVar.addOffset + combineCount, pHmiVar->addOffset + varCount );
                newLen = 2 * ( endOffset - startOffset );
                if ( newLen <= MAX_HLINK_PACKET_LEN )  //最多组合 MAX_HLINK_PACKET_LEN 字节
                {
                    if ( newLen <= 2*( combineCount + varCount ) + 10 ) //能节省10字节 两个变量之间的距离超过10/2就不合并
                    {
                        hlinkConnect->combinVar.addOffset = startOffset;
                        hlinkConnect->combinVar.len = newLen;
                        hlinkConnect->combinVarsList.append( pHmiVar );
                    }
                }
                else
                    continue;
            }
            else if ( hlinkConnect->combinVar.area == HOST_LINK_AREA_TCBIT )
            {
                endOffset = qMax( hlinkConnect->combinVar.addOffset + hlinkConnect->combinVar.len, pHmiVar->addOffset + pHmiVar->len );
                int newLen = endOffset - startOffset;
                if ( newLen <= MAX_HLINK_PACKET_LEN*2 )
                {
                    if ( newLen <= hlinkConnect->combinVar.len + pHmiVar->len + 8 ) //能节省8字节 两个变量之间的距离超过 8 则不合并
                    {
                        hlinkConnect->combinVar.addOffset = startOffset;
                        hlinkConnect->combinVar.len = newLen;
                        hlinkConnect->combinVarsList.append( pHmiVar );
                    }
                }
                else
                    continue;
            }
        }
    }
}

bool HostlinkService::hasProcessingReq( struct HostlinkConnect* hlinkConnect )
{
    return ( hlinkConnect->writingCnt > 0 || hlinkConnect->requestVarsList.count() > 0 );
}

bool HostlinkService::initCommDev()
{
    m_devUseCnt = 0;

    if (m_runTimer == NULL)
    {
        m_runTimer = new QTimer;
        connect( m_runTimer, SIGNAL( timeout() ), this,
            SLOT( slot_runTimerTimeout() )/*,Qt::QueuedConnection*/ );
    }
    if ( m_connectList.count() > 0 )
    {
        struct HostlinkConnect* conn = m_connectList[0];

//        if ( conn->cfg.COM_interface > 1 )
//        {
//            return false;
//        }

        if ( m_rwSocket == NULL )
        {
            struct UartCfg uartCfg;
            uartCfg.COMNum = conn->cfg.COM_interface;
            uartCfg.baud = conn->cfg.baud;
            uartCfg.dataBit = conn->cfg.dataBit;
            uartCfg.stopBit = conn->cfg.stopBit;
            uartCfg.parity = conn->cfg.parity;

            m_rwSocket = new RWSocket(HOSTLINK, &uartCfg, conn->cfg.id);
            connect(m_rwSocket, SIGNAL(readyRead()),this,SLOT(slot_readData()));
            m_rwSocket->openDev();
        }
    }
    return true;
}

bool HostlinkService::processRspData( struct HlinkMsgPar msgPar ,struct ComMsg comMsg )
{
//#ifdef Q_WS_QWS
    bool result = true;

            if ( m_devUseCnt > 0 )
                m_devUseCnt--;
            for ( int i = 0; i < m_connectList.count(); i++ )
            {
                struct HostlinkConnect* hlinkConnect = m_connectList[i];
                Q_ASSERT( hlinkConnect );
                if ( hlinkConnect->cfg.cpuAddr == msgPar.slave ) //属于这个连接
                {
                    if ( hlinkConnect->writingCnt > 0 && hlinkConnect->waitWrtieVarsList.count() > 0 && msgPar.rw == 1 )
                    {                                     // 处理写请求
                        Q_ASSERT( msgPar.rw == 1 );
                        hlinkConnect->writingCnt = 0;
                        //qDebug() << "Write err:" << msgPar.err;
                        struct HmiVar* pHmiVar = hlinkConnect->waitWrtieVarsList.takeFirst(); //删除写请求记录
                        m_mutexRw.lock();
                        //pHmiVar->err = msgPar.err;
                        pHmiVar->wStatus = msgPar.err;
                        m_mutexRw.unlock();
                        comMsg.varId = ( pHmiVar->varId & 0x0000ffff );
                        comMsg.type = 1;
                        comMsg.err = msgPar.err;
                m_comMsgTmp.insertMulti( comMsg.varId, comMsg );
                    }
                    else if ( hlinkConnect->requestVarsList.count() > 0 && msgPar.rw == 0 )
                    {
                        Q_ASSERT( msgPar.rw == 0 );
                        Q_ASSERT( msgPar.area == hlinkConnect->requestVar.area );
                        //qDebug() << "Read err:" << msgPar.err;
                        if ( msgPar.err == VAR_PARAM_ERROR && hlinkConnect->requestVarsList.count() > 1 )
                        {
                            for ( int j = 0; j < hlinkConnect->requestVarsList.count(); j++ )
                            {
                                struct HmiVar* pHmiVar = hlinkConnect->requestVarsList[j];
                                pHmiVar->overFlow = true;
                                pHmiVar->lastUpdateTime = -1; //合并后越界的不算越界，立即再次更新（更新时不合并）
                            }
                        }
                        else
                        {
                            //qDebug()<<"hlinkConnect->requestVarsList.count() = "<<hlinkConnect->requestVarsList.count();
                            for ( int j = 0; j < hlinkConnect->requestVarsList.count(); j++ )
                            {
                                m_mutexRw.lock();
                                struct HmiVar* pHmiVar = hlinkConnect->requestVarsList[j];
                                //qDebug()<<"    pHmiVar->varId"<<pHmiVar->varId;
                                //qDebug()<<"msgPar.area = "<<msgPar.area<<"    pHmiVar->area"<<pHmiVar->area;
                                Q_ASSERT( msgPar.area == pHmiVar->area );
                                pHmiVar->err = msgPar.err;
                                if ( msgPar.err == VAR_PARAM_ERROR )
                                {
                                    pHmiVar->overFlow = true;
                                    comMsg.varId = ( pHmiVar->varId & 0x0000ffff );
                                    comMsg.type = 0;// 0 means read
                                    comMsg.err = VAR_PARAM_ERROR;
                                    m_comMsgTmp.insert( comMsg.varId, comMsg );
                                }
                                else if ( msgPar.err == VAR_NO_ERR )
                                {
                                    pHmiVar->overFlow = false;
                                    if ( msgPar.area == HOST_LINK_AREA_IO ||
                                                    msgPar.area == HOST_LINK_AREA_HR ||
                                                    msgPar.area == HOST_LINK_AREA_AR ||
                                                    msgPar.area == HOST_LINK_AREA_LR ||
                                                    msgPar.area == HOST_LINK_AREA_DM ||
                                                    msgPar.area == HOST_LINK_AREA_TCVAL )
                                    {
                                        Q_ASSERT( msgPar.count <= MAX_HLINK_PACKET_LEN / 2 );
                                        int offset = pHmiVar->addOffset - hlinkConnect->requestVar.addOffset;
                                        Q_ASSERT( offset >= 0 );
                                        Q_ASSERT( 2*offset <= MAX_HLINK_PACKET_LEN );
                                        if ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup )
                                        {
                                            int len = 2 * (( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16 );
                                            if ( pHmiVar->rData.size() < len )
                                                pHmiVar->rData = QByteArray( len, 0 ); //初始化为0
                                            //pHmiVar->rData.resize(len);
                                            if ( 2*offset + len <= MAX_HLINK_PACKET_LEN ) //保证不越界
                                            {
                                                memcpy( pHmiVar->rData.data(), &msgPar.data[2*offset], len );
                                                if ( pHmiVar->wStatus != VAR_NO_OPERATION )
                                                {
                                                    comMsg.varId = ( pHmiVar->varId & 0x0000ffff );
                                                    comMsg.type = 0;// 0 means read
                                                    comMsg.err = VAR_NO_ERR;
                                            m_comMsgTmp.insert( comMsg.varId, comMsg );
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
                                            if ( 2*offset + len <= MAX_HLINK_PACKET_LEN ) //保证不越界
                                            {
                                                memcpy( pHmiVar->rData.data(), &msgPar.data[2*offset], len );
                                                if ( pHmiVar->wStatus != VAR_NO_OPERATION )
                                                {
                                                    comMsg.varId = ( pHmiVar->varId & 0x0000ffff );
                                                    comMsg.type = 0;// 0 means read
                                                    comMsg.err = VAR_NO_ERR;
                                            m_comMsgTmp.insert( comMsg.varId, comMsg );
                                                }
                                            }
                                            else
                                                Q_ASSERT( false );
                                        }
                                    }
                                    else if ( msgPar.area == HOST_LINK_AREA_TCBIT )
                                    {
                                        Q_ASSERT( msgPar.count <= MAX_HLINK_PACKET_LEN*2 );
                                        if ( !hlinkConnect->waitWrtieVarsList.contains( pHmiVar ) )
                                        {
                                            int startBitOffset = pHmiVar->addOffset - hlinkConnect->requestVar.addOffset;
                                            Q_ASSERT( startBitOffset >= 0 );
                                            int len = ( pHmiVar->len + 7 ) / 8;
                                            if ( pHmiVar->rData.size() < len )
                                                pHmiVar->rData = QByteArray( len, 0 ); //初始化为0
                                            //pHmiVar->rData.resize(len);
                                            for ( int k = 0; k < pHmiVar->len; k++ )
                                            {
                                                int rOffset = startBitOffset + k;
                                                if ( msgPar.data[rOffset/8] & ( 1 << ( rOffset % 8 ) ) )
                                                    pHmiVar->rData[k/8] = pHmiVar->rData[k/8] | ( 1 << ( k % 8 ) );
                                                else
                                                    pHmiVar->rData[k/8] = pHmiVar->rData[k/8] & ( ~( 1 << ( k % 8 ) ) );
                                            }
                                            if ( pHmiVar->wStatus != VAR_NO_OPERATION )
                                            {
                                                comMsg.varId = ( pHmiVar->varId & 0x0000ffff );
                                                comMsg.type = 0;// 0 means read
                                                comMsg.err = VAR_NO_ERR;
                                        m_comMsgTmp.insert( comMsg.varId, comMsg );
                                            }

                                        }
                                    }
                                    else
                                    {
                                        if ( pHmiVar->rData.size() < pHmiVar->len )
                                            pHmiVar->rData = QByteArray( pHmiVar->len, 0 );
                                        memcpy( pHmiVar->rData.data(),  msgPar.data, pHmiVar->len );
                                        if ( pHmiVar->wStatus != VAR_NO_OPERATION )
                                        {
                                            comMsg.varId = ( pHmiVar->varId & 0x0000ffff );
                                            comMsg.type = 0;
                                            comMsg.err = VAR_COMM_TIMEOUT;
                                            m_comMsgTmp.insert( comMsg.varId, comMsg );
                                            //处理CPU Type CPU status 它们都只有固定一个字节
                                        }
                                    }
                                }
                                else if ( msgPar.err == VAR_COMM_TIMEOUT )
                                {

                                    if ( pHmiVar->varId == 65535 )
                                    {
                                        foreach( HmiVar* tempHmiVar, hlinkConnect->actVarsList )
                                        {
                                            comMsg.type = 0;
                                            comMsg.err = VAR_DISCONNECT;
                                            tempHmiVar->err = VAR_COMM_TIMEOUT;
                                            tempHmiVar->lastUpdateTime = -1;
                                            comMsg.varId = ( tempHmiVar->varId & 0x0000ffff );
                                    m_comMsgTmp.insert( comMsg.varId, comMsg );
                                        }
                                    }
                                    else
                                    {
                                        comMsg.varId = ( pHmiVar->varId & 0x0000ffff );
                                        comMsg.type = 0;
                                        comMsg.err = VAR_COMM_TIMEOUT;
                                m_comMsgTmp.insert( comMsg.varId, comMsg );
                                    }
                                }
                                m_mutexRw.unlock();
                            }
                        }
                        hlinkConnect->requestVarsList.clear();
                    }
                    if ( msgPar.err == VAR_COMM_TIMEOUT )
                        hlinkConnect->connStatus = CONNECT_NO_LINK;
                    else
                        hlinkConnect->connStatus = CONNECT_LINKED_OK;
                    break;
                }
    }
    return result;
}


void HostlinkService::slot_run()
{
    initCommDev();
    m_runTimer->start(10);
}

void HostlinkService::slot_readData()
{
    struct HlinkMsgPar msgPar;
    struct ComMsg comMsg;
    int ret;
    do
    {
        //ret =m_modbus->read( (char *)&msgPar, sizeof( struct HlinkMsgPar ) ); //接受来自Driver的消息
        ret = m_rwSocket->readDev((char *)&msgPar);
        if( ret == sizeof( struct HlinkMsgPar ) )
        {
            switch (msgPar.err)
            {
            case COM_OPEN_ERR:
                qDebug()<<"COM_OPEN_ERR";
                m_devUseCnt = 0;
                for ( int i = 0; i < m_connectList.count(); i++ )
                {
                    struct HostlinkConnect* hlinkConnect = m_connectList[i];
                    Q_ASSERT( hlinkConnect );
                    hlinkConnect->waitWrtieVarsList.clear();
                    hlinkConnect->requestVarsList.clear();
                    foreach( HmiVar* tempHmiVar, hlinkConnect->actVarsList )
                    {
                        comMsg.type = 0; //0是read;
                        comMsg.err = VAR_DISCONNECT;
                        tempHmiVar->err = VAR_COMM_TIMEOUT;
                        tempHmiVar->lastUpdateTime = -1;
                        comMsg.varId = ( tempHmiVar->varId & 0x0000ffff );
                        m_comMsgTmp.insert( comMsg.varId, comMsg );
                    }
                }
                break;
            default:
                processRspData( msgPar, comMsg );
                break;
            }
        }
    }
    while ( ret == sizeof( struct HlinkMsgPar ) );
}

void HostlinkService::slot_runTimerTimeout()
{
    m_runTimer->stop();
    int lastUpdateTime = -1;

        int connectCount = m_connectList.count();
        m_currTime = getCurrTime();
        int currTime = m_currTime;

        for ( int i = 0; i < connectCount; i++ )
        {                              //处理写请求
            //int curIndex = ( lastConnect + 1 + i ) % connectCount;//保证从前1次的后1个连接开始,使各连接机会均等
            struct HostlinkConnect* hlinkConnect = m_connectList[i];
            updateWriteReq( hlinkConnect );
            trySndWriteReq( hlinkConnect );
        }

        bool needUpdateFlag = false;

        if (( lastUpdateTime == -1 ) || ( calIntervalTime( lastUpdateTime, currTime ) >= 30 ) )
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
        {                              // 处理读请求
            //int curIndex = ( lastConnect + 1 + i ) % connectCount;//保证从前1次的后1个连接开始,使各连接机会均等
            struct HostlinkConnect* hlinkConnect = m_connectList[i];
            if ( needUpdateFlag )
            {//qDebug()<<"i = "<<i;
                updateOnceRealReadReq( hlinkConnect );
                updateReadReq( hlinkConnect );
            }
            if ( !( hasProcessingReq( hlinkConnect ) ) )
        {
            combineReadReq( hlinkConnect );
        }
        trySndReadReq( hlinkConnect );
    }
    m_runTimer->start(20);
}
