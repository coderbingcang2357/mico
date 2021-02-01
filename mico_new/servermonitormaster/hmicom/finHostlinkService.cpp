
#include "finHostlinkService.h"



FinHostlinkService::FinHostlinkService( struct FinHostlinkConnectCfg& connectCfg )
{
    struct FinHostlinkConnect* connect = new FinHostlinkConnect;
    connect->cfg = connectCfg;
    connect->writingCnt = 0;
    connect->connStatus = CONNECT_NO_OPERATION;
    connect->connectTestVar.lastUpdateTime = -1;
    m_connectList.append( connect );
    m_currTime = getCurrTime();
    m_runTimer = NULL;
    m_rwSocket = NULL;
}

FinHostlinkService::~FinHostlinkService()
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

bool FinHostlinkService::meetServiceCfg( int serviceType, int COM_interface )
{
    if ( serviceType == SERVICE_TYPE_FINHOSTLINK )
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

int FinHostlinkService::getConnLinkStatus( int connectId )
{
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct FinHostlinkConnect* finHostLinkConnect = m_connectList[i];
        if ( connectId == finHostLinkConnect->cfg.id )
            return finHostLinkConnect->connStatus;
    }
    return -1;
}

int FinHostlinkService::getComMsg( QList<struct ComMsg> &msgList )
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

int FinHostlinkService:: splitLongVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct FinHostlinkConnect* finHostLinkConnect = m_connectList[i];
        if ( pHmiVar->connectId == finHostLinkConnect->cfg.id )
        {
            pHmiVar->lastUpdateTime = -1; //立即更新

            if ( pHmiVar->area >= FIN_HOST_LINK_AREA_CIO_BIT &&
                            pHmiVar->area <= FIN_HOST_LINK_AREA_EMC_BIT )
            {
                Q_ASSERT( pHmiVar->dataType == Com_Bool );
                if ( pHmiVar->area != FIN_HOST_LINK_AREA_T_BIT &&
                                pHmiVar->area != FIN_HOST_LINK_AREA_C_BIT )
                {
                    Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
                    if ( pHmiVar->len > FIN_M_HOSTLINK_MAX_BIT && pHmiVar->splitVars.count() <= 0 )  //拆分
                    {
                        for ( int splitedLen = 0; splitedLen < pHmiVar->len; )
                        {
                            int len = pHmiVar->len - splitedLen;
                            if ( len > FIN_M_HOSTLINK_MAX_BIT )
                                len = FIN_M_HOSTLINK_MAX_BIT;

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
                    if ( pHmiVar->len > FIN_M_HOSTLINK_MAX_BIT && pHmiVar->splitVars.count() <= 0 )  //拆分
                    {
                        for ( int splitedLen = 0; splitedLen < pHmiVar->len; )
                        {
                            int len = pHmiVar->len - splitedLen;
                            if ( len > FIN_M_HOSTLINK_MAX_BIT )
                                len = FIN_M_HOSTLINK_MAX_BIT;

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
            }
            else if ( pHmiVar->area >= FIN_HOST_LINK_AREA_CIO && pHmiVar->area <= FIN_HOST_LINK_AREA_IR )
            {
                if ( pHmiVar->dataType == Com_Bool )
                {
                    Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
                    if ( pHmiVar->len > FIN_M_HOSTLINK_MAX_WORD*16 && pHmiVar->splitVars.count() <= 0 )  //拆分
                    {
                        for ( int splitedLen = 0; splitedLen < pHmiVar->len; )
                        {
                            int len = pHmiVar->len - splitedLen;
                            if ( len > FIN_M_HOSTLINK_MAX_WORD * 16 )
                                len = FIN_M_HOSTLINK_MAX_WORD * 16;

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
                else if ( pHmiVar->len > ( FIN_M_HOSTLINK_MAX_WORD*2 ) && pHmiVar->splitVars.count() <= 0 )  //拆分
                {
                    for ( int splitedLen = 0; splitedLen < pHmiVar->len; )
                    {
                        int len = pHmiVar->len - splitedLen;
                        if ( len > ( FIN_M_HOSTLINK_MAX_WORD*2 ) )
                            len = ( FIN_M_HOSTLINK_MAX_WORD * 2 );

                        struct HmiVar* pSplitHhiVar = new HmiVar;
                        pSplitHhiVar->varId = pHmiVar->varId + (( pHmiVar->splitVars.count() + 1 ) << 16 );
                        pSplitHhiVar->connectId = pHmiVar->connectId;
                        pSplitHhiVar->cycleTime = pHmiVar->cycleTime;
                        pSplitHhiVar->area = pHmiVar->area;
                        if ( pHmiVar->area != FIN_HOST_LINK_AREA_IR )
                            pSplitHhiVar->addOffset = pHmiVar->addOffset + ( splitedLen + 1 ) / 2;
                        else
                            pSplitHhiVar->addOffset = pHmiVar->addOffset + ( splitedLen + 3 ) / 4;
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
            return 0;
        }
    }
    return -1;
}

int FinHostlinkService::onceRealUpdateVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    m_mutexOnceReal.lock();
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct FinHostlinkConnect* finHostLinkConnect = m_connectList[i];
        if ( pHmiVar->connectId == finHostLinkConnect->cfg.id )
        {
            pHmiVar->lastUpdateTime = -1; //立即更新
            if ( pHmiVar->splitVars.count() > 0 )
            {
                for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
                {
                    struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                    pSplitHhiVar->lastUpdateTime = -1;
                    pSplitHhiVar->overFlow = false;
                    if ( !finHostLinkConnect->onceRealVarsList.contains( pSplitHhiVar ) )
                        finHostLinkConnect->onceRealVarsList.append( pSplitHhiVar );
                }
            }
            else
            {
                pHmiVar->lastUpdateTime = -1;
                pHmiVar->overFlow = false;
                if ( !finHostLinkConnect->onceRealVarsList.contains( pHmiVar ) )
                    finHostLinkConnect->onceRealVarsList.append( pHmiVar );
            }

            m_mutexOnceReal.unlock();
            return 0;
        }
    }
    m_mutexOnceReal.unlock();
    return -1;
}

int FinHostlinkService::startUpdateVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );

    m_mutexUpdateVar.lock();
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct FinHostlinkConnect* finHostLinkConnect = m_connectList[i];
        if ( pHmiVar->connectId == finHostLinkConnect->cfg.id )
        {
            pHmiVar->lastUpdateTime = -1;
            if ( pHmiVar->splitVars.count() > 0 )
            {
                for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
                {
                    struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                    pSplitHhiVar->lastUpdateTime = -1;
                    pSplitHhiVar->overFlow = false;
                    if ( !finHostLinkConnect->actVarsList.contains( pSplitHhiVar ) )
                        finHostLinkConnect->actVarsList.append( pSplitHhiVar );
                }
            }
            else
            {
                pHmiVar->lastUpdateTime = -1;
                pHmiVar->overFlow = false;
                if ( !finHostLinkConnect->actVarsList.contains( pHmiVar ) )
                    finHostLinkConnect->actVarsList.append( pHmiVar );
            }

            m_mutexUpdateVar.unlock();
            return 0;
        }
    }
    m_mutexUpdateVar.unlock();
    return -1;
}

int FinHostlinkService::stopUpdateVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    m_mutexUpdateVar.lock();
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct FinHostlinkConnect* finHostLinkConnect = m_connectList[i];
        if ( pHmiVar->connectId == finHostLinkConnect->cfg.id )
        {
            if ( pHmiVar->splitVars.count() > 0 )
            {
                for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
                {
                    struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                    finHostLinkConnect->actVarsList.removeAll( pSplitHhiVar );
                }
            }
            else
                finHostLinkConnect->actVarsList.removeAll( pHmiVar );
            m_mutexUpdateVar.unlock();
            return 0;
        }
    }
    m_mutexUpdateVar.unlock();
    return -1;
}

int FinHostlinkService::readVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    if ( pHmiVar->err == VAR_NO_ERR )  //无错误才拷贝数据
    {
        if ( pHmiVar->area >= FIN_HOST_LINK_AREA_CIO &&
                        pHmiVar->area <= FIN_HOST_LINK_AREA_IR )
        {
            if ( pHmiVar->dataType == Com_Bool )
            {
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
            else if ( bufLen >= pHmiVar->len && pHmiVar->rData.size() >= pHmiVar->len )
            {
                for ( int j = 0; j < pHmiVar->len; j++ )
                {
                    if ( pHmiVar->dataType != Com_String )
                    {
                        if ( j & 0x1 )
                            pBuf[j] = pHmiVar->rData.at( j - 1 );
                        else
                            pBuf[j] = pHmiVar->rData.at( j + 1 );
                        if ( pHmiVar->area == FIN_HOST_LINK_AREA_IR && j % 4 == 3 )
                        {
                            pBuf[j]   = pHmiVar->rData.at( j - 3 );
                            pBuf[j-1] = pHmiVar->rData.at( j - 2 );
                            pBuf[j-2] = pHmiVar->rData.at( j - 1 );
                            pBuf[j-3] = pHmiVar->rData.at( j );
                        }
                    }
                    else
                        pBuf[j] = pHmiVar->rData.at( j );
                }
            }
            else
                Q_ASSERT( false );
        }
        else if ( pHmiVar->area >= FIN_HOST_LINK_AREA_CIO_BIT &&
                        pHmiVar->area <= FIN_HOST_LINK_AREA_EMC_BIT )
        {
            int dataBytes = ( pHmiVar->len + 7 ) / 8; //字节数
            if ( bufLen >= dataBytes && pHmiVar->rData.size() >= dataBytes )
                memcpy( pBuf, pHmiVar->rData.data(), dataBytes );
            else
                Q_ASSERT( false );
            //qDebug() << pHmiVar->area << pHmiVar->addOffset << "buff[0]" << (int)pBuf[0];
        }
        else
        {
            if ( bufLen >= pHmiVar->len && pHmiVar->rData.size() >= pHmiVar->len &&  pHmiVar->len > 0 )
                memcpy( pBuf, pHmiVar->rData.data(), 1 );
        }
    }
    return pHmiVar->err;
}

int FinHostlinkService::readDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    Q_ASSERT( pHmiVar && pBuf );
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct FinHostlinkConnect* finHostLinkConnect = m_connectList[i];
        if ( pHmiVar->connectId == finHostLinkConnect->cfg.id )
        {
            int ret;
            m_mutexRw.lock();
            if ( pHmiVar->splitVars.count() > 0 )
            {
                for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
                {
                    struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                    int len = 0;
                    if ( pHmiVar->dataType == Com_Bool )
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

int FinHostlinkService::writeVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    int ret;

    Q_ASSERT( pHmiVar && pBuf );
    m_mutexRw.lock();
    if ( pHmiVar->area >= FIN_HOST_LINK_AREA_CIO &&
                    pHmiVar->area <= FIN_HOST_LINK_AREA_IR )     //IR count 1 = byte 4
    {                            //大小数等到发送写命令时再处理
        if ( pHmiVar->dataType == Com_Bool )
        {//qDebug()<<"writeVar pHmiVar->area :      "<<pHmiVar->area;
            //qDebug()<<"writeVar pHmiVar->bitOffset : "<<pHmiVar->bitOffset;
            Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
            int count = ( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16;  //字数
            Q_ASSERT( count > 0 && count <= FIN_M_HOSTLINK_MAX_WORD );
            if ( count > FIN_M_HOSTLINK_MAX_WORD )
                count = FIN_M_HOSTLINK_MAX_WORD;
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
            Q_ASSERT( count > 0 && count <= FIN_M_HOSTLINK_MAX_WORD );
            if ( count > FIN_M_HOSTLINK_MAX_WORD )
                count = FIN_M_HOSTLINK_MAX_WORD;
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
                    if ( pHmiVar->dataType != Com_String )
                    {
                        if ( j & 0x1 )
                            pHmiVar->rData[j] = pHmiVar->wData.at( j - 1 );
                        else if ( j + 1 < pHmiVar->wData.size() )
                            pHmiVar->rData[j] = pHmiVar->wData.at( j + 1 );
                        if ( pHmiVar->area == FIN_HOST_LINK_AREA_IR && j % 4 == 3 )
                        {
                            pHmiVar->rData[j] = pHmiVar->wData.at( j - 3 );
                            pHmiVar->rData[j-1] = pHmiVar->wData.at( j - 2 );
                            pHmiVar->rData[j-2] = pHmiVar->wData.at( j - 1 );
                            pHmiVar->rData[j-3] = pHmiVar->wData.at( j );
                        }
                    }
                    else
                        pHmiVar->rData[j] = pHmiVar->wData.at( j );
                }
                else
                    break;
            }
        }
    }
    else if ( pHmiVar->area >= FIN_HOST_LINK_AREA_CIO_BIT &&
                    pHmiVar->area <= FIN_HOST_LINK_AREA_EMC_BIT )
    {
        Q_ASSERT( pHmiVar->dataType == Com_Bool );
        int count = pHmiVar->len;
        Q_ASSERT( count > 0 &&  count <= FIN_M_HOSTLINK_MAX_BIT );
        if ( count > FIN_M_HOSTLINK_MAX_BIT )
            count = FIN_M_HOSTLINK_MAX_BIT;
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
    else
    {
        Q_ASSERT( false );
    }
    pHmiVar->wStatus = VAR_NO_OPERATION;
    ret = pHmiVar->err;
    m_mutexRw.unlock();

    return ret; //是否要返回成功
}

int FinHostlinkService::writeDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    Q_ASSERT( pHmiVar && pBuf );

    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct FinHostlinkConnect* finHostLinkConnect = m_connectList[i];
        if ( pHmiVar->connectId == finHostLinkConnect->cfg.id )
        {
            if ( finHostLinkConnect->connStatus != CONNECT_LINKED_OK )
                return  VAR_COMM_TIMEOUT;
            int ret = 0;
            m_mutexWReq.lock();          //注意2个互斥锁顺序,防止死锁
            if ( pHmiVar->splitVars.count() > 0 )
            {
                for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
                {
                    struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                    int len = 0;
                    if ( pHmiVar->dataType == Com_Bool )
                        len = ( pSplitHhiVar->len + 7 ) / 8;
                    else
                        len = pSplitHhiVar->len;

                    ret = writeVar( pSplitHhiVar, pBuf, len );
                    finHostLinkConnect->wrtieVarsList.append( pSplitHhiVar );
                    pBuf += len;
                    bufLen -= len;
                }
            }
            else
            {
                ret = writeVar( pHmiVar, pBuf, bufLen );
                finHostLinkConnect->wrtieVarsList.append( pHmiVar );
            }
            m_mutexWReq.unlock();

            return ret; //是否要返回成功
        }
    }
    return -1;
}

int FinHostlinkService::addConnect( struct FinHostlinkConnectCfg& connectCfg )
{                                      //要检查是否重复 add
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct FinHostlinkConnect* finHostLinkConnect = m_connectList[i];
        Q_ASSERT( finHostLinkConnect->cfg.COM_interface == connectCfg.COM_interface );//绝对要是同一个设备接口
        if ( connectCfg.id == finHostLinkConnect->cfg.id )
        {
            Q_ASSERT( false );
            return 0;
        }
    }
    struct FinHostlinkConnect* finHostLinkConnect = new FinHostlinkConnect;
    finHostLinkConnect->cfg = connectCfg;
    finHostLinkConnect->writingCnt = 0;
    finHostLinkConnect->connStatus = CONNECT_NO_OPERATION;
    finHostLinkConnect->connectTestVar.lastUpdateTime = -1;
    m_connectList.append( finHostLinkConnect );
    return 0;
}

int FinHostlinkService::sndWriteVarMsgToDriver( struct FinHostlinkConnect* finHostLinkConnect, struct HmiVar* pHmiVar )
{
    int sendedLen = 0;
    Q_ASSERT( finHostLinkConnect && pHmiVar );
//#ifdef Q_WS_QWS
    struct finHlinkMsgPar msgPar;

    m_mutexRw.lock();
    msgPar.rw = 1;   //写
    msgPar.slave = finHostLinkConnect->cfg.cpuAddr;
    msgPar.area = pHmiVar->area;
    msgPar.wordAddr = pHmiVar->addOffset;
    msgPar.err = 0;

    if ( pHmiVar->area >= FIN_HOST_LINK_AREA_CIO &&
                    pHmiVar->area <= FIN_HOST_LINK_AREA_IR )
    {
        if ( pHmiVar->dataType == Com_Bool )
        {
            Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
            int count = ( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16;  //字数
            Q_ASSERT( count > 0 && count <= FIN_M_HOSTLINK_MAX_WORD );
            if ( count > FIN_M_HOSTLINK_MAX_WORD )
                count = FIN_M_HOSTLINK_MAX_WORD;
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
            Q_ASSERT( count > 0 && count <= FIN_M_HOSTLINK_MAX_WORD );
            if ( count > FIN_M_HOSTLINK_MAX_WORD )
                count = FIN_M_HOSTLINK_MAX_WORD;

            msgPar.count = count;      // 占用的总字数
            //qDebug()<<"write Count = "<<count;
            msgPar.bitAddr = 0;

            int dataBytes = 2 * count;
            if ( pHmiVar->rData.size() < dataBytes )
                pHmiVar->rData.resize( dataBytes );

            for ( int j = 0; j < pHmiVar->len; j++ )
            {                          // 奇偶字节调换
                if ( j < pHmiVar->wData.size() && j < pHmiVar->rData.size() )
                {
                    if ( pHmiVar->dataType != Com_String )
                    {
                        if ( j & 0x1 )
                            pHmiVar->rData[j] = pHmiVar->wData.at( j - 1 );
                        else
                            pHmiVar->rData[j] = pHmiVar->wData.at( j + 1 );
                        if ( pHmiVar->area == FIN_HOST_LINK_AREA_IR && j % 4 == 3 )
                        {
                            pHmiVar->rData[j]   = pHmiVar->wData.at( j - 3 );
                            pHmiVar->rData[j-1] = pHmiVar->wData.at( j - 2 );
                            pHmiVar->rData[j-2] = pHmiVar->wData.at( j - 1 );
                            pHmiVar->rData[j-3] = pHmiVar->wData.at( j );
                        }
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
    else if ( pHmiVar->area >= FIN_HOST_LINK_AREA_CIO_BIT &&
                    pHmiVar->area <= FIN_HOST_LINK_AREA_EMC_BIT )
    {                                  // 位变量长度为位的个数
        Q_ASSERT( pHmiVar->dataType == Com_Bool );
        int count = pHmiVar->len;
        Q_ASSERT( count > 0 &&  count <= FIN_M_HOSTLINK_MAX_BIT );

        if ( count > FIN_M_HOSTLINK_MAX_BIT )
            count = FIN_M_HOSTLINK_MAX_BIT;

        msgPar.count = count;
        msgPar.bitAddr = pHmiVar->bitOffset;

        if ( pHmiVar->area == FIN_HOST_LINK_AREA_T_BIT || pHmiVar->area == FIN_HOST_LINK_AREA_C_BIT )
            msgPar.bitAddr = 0;

        int dataBytes = ( count + 7 ) / 8;
        if ( pHmiVar->rData.size() < dataBytes )
            pHmiVar->rData.resize( dataBytes );

        memcpy( pHmiVar->rData.data(), pHmiVar->wData.data(), qMin( dataBytes, pHmiVar->wData.size() ) );
        memcpy( msgPar.data, pHmiVar->rData.data(), dataBytes );
        sendedLen = count;
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
        //int len = write( m_fd, ( char* ) & msgPar, sizeof( struct finHlinkMsgPar ) );
        int len = m_rwSocket->writeDev(( char* ) & msgPar, sizeof( struct finHlinkMsgPar ));
        Q_ASSERT( len > 0 );
    }
    else
        m_mutexRw.unlock();
//#endif
    return sendedLen;
}

int FinHostlinkService::sndReadVarMsgToDriver( struct FinHostlinkConnect* finHostLinkConnect, struct HmiVar* pHmiVar )
{
    int sendedLen = 0;
    Q_ASSERT( finHostLinkConnect && pHmiVar );
//#ifdef Q_WS_QWS
    struct finHlinkMsgPar msgPar;

    m_mutexRw.lock();
    msgPar.rw = 0;    // 读
    msgPar.slave = finHostLinkConnect->cfg.cpuAddr;
    msgPar.area = pHmiVar->area;
    msgPar.wordAddr = pHmiVar->addOffset;
    msgPar.err = 0;

    if ( pHmiVar->area >= FIN_HOST_LINK_AREA_CIO &&
                    pHmiVar->area <= FIN_HOST_LINK_AREA_IR )
    {
        if ( pHmiVar->dataType == Com_Bool )
        {
            Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
            int count = ( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16;  //字数
            Q_ASSERT( count > 0 && count <= FIN_M_HOSTLINK_MAX_WORD );
            if ( count > FIN_M_HOSTLINK_MAX_WORD )
                count = FIN_M_HOSTLINK_MAX_WORD;
            msgPar.count = count;      // 占用的总字数
            sendedLen = count * 16;
        }
        else
        {
            int count = ( pHmiVar->len + 1 ) / 2;  //字数
            Q_ASSERT( count > 0 && count <= FIN_M_HOSTLINK_MAX_WORD );
            if ( count > FIN_M_HOSTLINK_MAX_WORD )
                count = FIN_M_HOSTLINK_MAX_WORD;

            msgPar.count = count;      // 占用的总字数
            //qDebug()<<"                read  Count = "<<count;
            msgPar.bitAddr = 0;
            sendedLen = count * 2;
        }
    }
    else if ( pHmiVar->area >= FIN_HOST_LINK_AREA_CIO_BIT &&
                    pHmiVar->area <= FIN_HOST_LINK_AREA_EMC_BIT )
    {                                  // 位变量长度为位的个数
        Q_ASSERT( pHmiVar->dataType == Com_Bool );
        int count = pHmiVar->len;
        Q_ASSERT( count > 0 &&  count <= FIN_M_HOSTLINK_MAX_BIT );
        if ( count > FIN_M_HOSTLINK_MAX_BIT )
            count = FIN_M_HOSTLINK_MAX_BIT;

        msgPar.count = count;
        msgPar.bitAddr = pHmiVar->bitOffset;

        if ( pHmiVar->area == FIN_HOST_LINK_AREA_T_BIT || pHmiVar->area == FIN_HOST_LINK_AREA_C_BIT )
            msgPar.bitAddr = 0;

        sendedLen = count;
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
        //int len = write( m_fd, ( char* ) & msgPar, sizeof( struct finHlinkMsgPar ) );
        int len = m_rwSocket->writeDev( ( char* ) & msgPar, sizeof( struct finHlinkMsgPar ));
        Q_ASSERT( len > 0 );
    }
    else
        m_mutexRw.unlock();
//#endif
    return sendedLen;
}

bool isWaitingReadRsp( struct FinHostlinkConnect* finHostLinkConnect, struct HmiVar* pHmiVar )
{
    Q_ASSERT( finHostLinkConnect && pHmiVar );
    if ( finHostLinkConnect->waitUpdateVarsList.contains( pHmiVar ) )
        return true;
    else if ( finHostLinkConnect->combinVarsList.contains( pHmiVar ) )
        return true;
    else if ( finHostLinkConnect->requestVarsList.contains( pHmiVar ) )
        return true;
    else
        return false;
}

bool FinHostlinkService::trySndWriteReq( struct FinHostlinkConnect* finHostLinkConnect ) //不处理超长变量
{
    if ( m_devUseCnt < MAXDEVCNT )                          //没有正在读写, snd read, 一次只能发送一包请求
    {
        if ( finHostLinkConnect->writingCnt == 0 && finHostLinkConnect->requestVarsList.count() == 0 ) //这个连接没有读写
        {
            m_mutexWReq.lock();
            if ( finHostLinkConnect->waitWrtieVarsList.count() > 0 )
            {
                struct HmiVar* pHmiVar = finHostLinkConnect->waitWrtieVarsList[0];
                if (( pHmiVar->lastUpdateTime == -1 )  // WORD区的字符串变量要先读
                                && ( pHmiVar->area >= FIN_HOST_LINK_AREA_CIO &&
                                     pHmiVar->area <= FIN_HOST_LINK_AREA_IR )
                                && ( pHmiVar->dataType == Com_String ||  pHmiVar->dataType == Com_Bool ) )
                {
                    //pHmiVar->lastUpdateTime = m_currTime;//getCurrTime();
                    if ( !isWaitingReadRsp( finHostLinkConnect, pHmiVar ) )
                        finHostLinkConnect->waitUpdateVarsList.prepend( pHmiVar ); //加入到读队列最前,保证下次最先读
                }
                else if ( !isWaitingReadRsp( finHostLinkConnect, pHmiVar ) ) //没有正在等待读
                {                                   //直接写,snd write cmd to driver
                    if ( sndWriteVarMsgToDriver( finHostLinkConnect, pHmiVar ) > 0 )
                    {
                        //pHmiVar->lastUpdateTime = m_currTime;//getCurrTime();
                        finHostLinkConnect->writingCnt = 1;    //进入写状态
                        m_mutexWReq.unlock();
                        return true;
                    }
                    else
                    {
                        finHostLinkConnect->waitWrtieVarsList.takeAt( 0 );//出列,丢掉写请求
                        pHmiVar->err = VAR_PARAM_ERROR;
                    }
                }
            }
            m_mutexWReq.unlock();
        }
    }
    return false;
}

bool FinHostlinkService::trySndReadReq( struct FinHostlinkConnect* finHostLinkConnect ) //不处理超长变量
{
    if ( m_devUseCnt < MAXDEVCNT )             //没有正在读写, snd read, 一次只能发送一包请求
    {
        if ( finHostLinkConnect->writingCnt == 0 && finHostLinkConnect->requestVarsList.count() == 0 ) //这个连接没有读写
        {
            if ( finHostLinkConnect->combinVarsList.count() > 0 )
            {
                finHostLinkConnect->requestVarsList = finHostLinkConnect->combinVarsList;
                finHostLinkConnect->requestVar = finHostLinkConnect->combinVar;
                for ( int i = 0; i < finHostLinkConnect->combinVarsList.count(); i++ )
                {                      //要把已经请求的变量从等待队列中删除
                    struct HmiVar* pHmiVar = finHostLinkConnect->combinVarsList[i];
                    finHostLinkConnect->waitUpdateVarsList.removeAll( pHmiVar );
                    pHmiVar->lastUpdateTime = m_currTime;
                }
                finHostLinkConnect->combinVarsList.clear();

                if ( sndReadVarMsgToDriver( finHostLinkConnect, &( finHostLinkConnect->requestVar ) ) > 0 )
                {
                    return true;
                }
                else
                {
                    for ( int i = 0; i < finHostLinkConnect->requestVarsList.count(); i++ )
                    {
                        struct HmiVar* pHmiVar = finHostLinkConnect->requestVarsList[i];
                        pHmiVar->err = VAR_PARAM_ERROR;
                    }
                    finHostLinkConnect->requestVarsList.clear();
                }
            }
        }
    }
    return false;
}

bool FinHostlinkService::updateWriteReq( struct FinHostlinkConnect* finHostLinkConnect )
{
    bool newUpdateVarFlag = false;
    QList<struct HmiVar*> tmpList;
    m_mutexWReq.lock();
    //int currTime = m_currTime;//getCurrTime();
    for ( int i = 0; i < finHostLinkConnect->wrtieVarsList.count(); i++ )
    {
        struct HmiVar* pHmiVar = finHostLinkConnect->wrtieVarsList[i];


        if ( !finHostLinkConnect->waitWrtieVarsList.contains( pHmiVar ) )
        {
            /*if ( finHostLinkConnect->waitUpdateVarsList.contains( pHmiVar ) )
                finHostLinkConnect->waitUpdateVarsList.removeAll( pHmiVar );

            if ( finHostLinkConnect->requestVarsList.contains( pHmiVar )  )
                finHostLinkConnect->requestVarsList.removeAll( pHmiVar );*/

            finHostLinkConnect->waitWrtieVarsList.append( pHmiVar );
            newUpdateVarFlag = true;
        }
        else
        {
            tmpList.append(pHmiVar);
        }
    }

    finHostLinkConnect->wrtieVarsList.clear() ;
    finHostLinkConnect->wrtieVarsList += tmpList;
    m_mutexWReq.unlock();

    return newUpdateVarFlag;
}
bool FinHostlinkService::updateReadReq( struct FinHostlinkConnect* finHostLinkConnect )
{
    bool newUpdateVarFlag = false;
    m_mutexUpdateVar.lock();
    int currTime = m_currTime;//getCurrTime();
    if ( finHostLinkConnect->connStatus == CONNECT_LINKED_OK )
    {
        for ( int i = 0; i < finHostLinkConnect->actVarsList.count(); i++ )
        {                                  // 找出需要刷新的变量
            struct HmiVar* pHmiVar = finHostLinkConnect->actVarsList[i];
            if (( pHmiVar->lastUpdateTime == -1 ) || ( calIntervalTime( pHmiVar->lastUpdateTime, currTime ) >= pHmiVar->cycleTime ) )
            {                              // 需要刷新
                if (( !finHostLinkConnect->waitUpdateVarsList.contains( pHmiVar ) )
                                && ( !finHostLinkConnect->waitWrtieVarsList.contains( pHmiVar ) )
                                //&& ( !finHostLinkConnect->combinVarsList.contains( pHmiVar ) )
                                && ( !finHostLinkConnect->requestVarsList.contains( pHmiVar ) ) )
                {
                    //pHmiVar->lastUpdateTime = currTime;
                    finHostLinkConnect->waitUpdateVarsList.append( pHmiVar );
                    newUpdateVarFlag = true;
                }
            }
        }
    }
    else
    {
        finHostLinkConnect->waitUpdateVarsList.clear();
        if ( finHostLinkConnect->actVarsList.count() > 0 && finHostLinkConnect->requestVarsList.count() == 0 )
        {//qDebug()<<"connectTestVar  lastUpdateTime  = "<<finHostLinkConnect->connectTestVar.lastUpdateTime;
            finHostLinkConnect->actVarsList[0]->lastUpdateTime = finHostLinkConnect->connectTestVar.lastUpdateTime;
            finHostLinkConnect->connectTestVar = *( finHostLinkConnect->actVarsList[0] );
            finHostLinkConnect->connectTestVar.varId = 65535;
            finHostLinkConnect->connectTestVar.cycleTime = 2000;
            if (( finHostLinkConnect->connectTestVar.lastUpdateTime == -1 )
                            || ( calIntervalTime( finHostLinkConnect->connectTestVar.lastUpdateTime, currTime ) >= finHostLinkConnect->connectTestVar.cycleTime ) )
            {
                if (( !finHostLinkConnect->waitUpdateVarsList.contains( &( finHostLinkConnect->connectTestVar ) ) )
                                && ( !finHostLinkConnect->waitWrtieVarsList.contains( &( finHostLinkConnect->connectTestVar ) ) )
                                && ( !finHostLinkConnect->requestVarsList.contains( &( finHostLinkConnect->connectTestVar ) ) ) )
                {
                    finHostLinkConnect->waitUpdateVarsList.append( &( finHostLinkConnect->connectTestVar ) );
                    newUpdateVarFlag = true;
                }
            }


        }
    }
    m_mutexUpdateVar.unlock();

    /*m_mutexWReq.lock();
    for ( int i = 0; i < finHostLinkConnect->waitWrtieVarsList.count(); i++ )
    {
        struct HmiVar* pHmiVar = finHostLinkConnect->waitWrtieVarsList[i];

        if ( finHostLinkConnect->waitUpdateVarsList.contains( pHmiVar ) )
            finHostLinkConnect->waitUpdateVarsList.removeAll( pHmiVar );

        if ( finHostLinkConnect->requestVarsList.contains( pHmiVar )  )
            finHostLinkConnect->requestVarsList.removeAll( pHmiVar );
    }
    m_mutexWReq.unlock();*/
    return newUpdateVarFlag;
}

bool FinHostlinkService::updateOnceRealReadReq( struct FinHostlinkConnect* finHostLinkConnect )
{
    bool newUpdateVarFlag = false;
    m_mutexOnceReal.lock();
    //int currTime = m_currTime;//getCurrTime();
    for ( int i = 0; i < finHostLinkConnect->onceRealVarsList.count(); i++ )
    {
        struct HmiVar* pHmiVar = finHostLinkConnect->onceRealVarsList[i];
        if (( !finHostLinkConnect->waitUpdateVarsList.contains( pHmiVar ) )
                        && ( !finHostLinkConnect->requestVarsList.contains( pHmiVar ) ) )
        {
            finHostLinkConnect->waitUpdateVarsList.prepend( pHmiVar );
            newUpdateVarFlag = true;
        }
        else
        {
            if ( finHostLinkConnect->waitUpdateVarsList.contains( pHmiVar ) )
            {
                finHostLinkConnect->waitUpdateVarsList.removeAll( pHmiVar );
                finHostLinkConnect->waitUpdateVarsList.prepend( pHmiVar );
            }
        }
    }
    finHostLinkConnect->onceRealVarsList.clear();
    m_mutexOnceReal.unlock();

    return newUpdateVarFlag;
}
void FinHostlinkService::combineReadReq( struct FinHostlinkConnect* finHostLinkConnect )
{
    finHostLinkConnect->combinVarsList.clear();
    if ( finHostLinkConnect->waitUpdateVarsList.count() >= 1 )
    {
        struct HmiVar* pHmiVar = finHostLinkConnect->waitUpdateVarsList[0];
        finHostLinkConnect->combinVarsList.append( pHmiVar );
        finHostLinkConnect->combinVar = *pHmiVar;

        if (( finHostLinkConnect->combinVar.area == FIN_HOST_LINK_AREA_T ||
                        finHostLinkConnect->combinVar.area == FIN_HOST_LINK_AREA_C ) &&
                        finHostLinkConnect->combinVar.dataType == Com_Bool )
        {
            Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
            finHostLinkConnect->combinVar.dataType = Com_Word;
            finHostLinkConnect->combinVar.len = 2 * (( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16 ); //2*count
            finHostLinkConnect->combinVar.bitOffset = -1;
        }
    }
    for ( int i = 1; i < finHostLinkConnect->waitUpdateVarsList.count(); i++ )
    {
        struct HmiVar* pHmiVar = finHostLinkConnect->waitUpdateVarsList[i];
        if ( pHmiVar->area == finHostLinkConnect->combinVar.area && finHostLinkConnect->combinVar.overFlow == pHmiVar->overFlow && ( pHmiVar->overFlow == false ) )
        {                            //没有越界才合并，否则不合并
            int startOffset = qMin( finHostLinkConnect->combinVar.addOffset, pHmiVar->addOffset );
            int endOffset = 0;
            int newLen = 0;
            if ( pHmiVar->area >= FIN_HOST_LINK_AREA_CIO &&
                            pHmiVar->area <= FIN_HOST_LINK_AREA_IR )
            {
                int combineCount = 0;
                int varCount = 0;

                combineCount = ( finHostLinkConnect->combinVar.len + 1 ) / 2;

                if ( pHmiVar->dataType != Com_Bool )
                    varCount = ( pHmiVar->len + 1 ) / 2;
                else
                {
                    Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
                    varCount = ( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16;
                }

                if ( pHmiVar->area != FIN_HOST_LINK_AREA_IR )
                {
                    endOffset = qMax( finHostLinkConnect->combinVar.addOffset + combineCount, pHmiVar->addOffset + varCount );
                    newLen = 2 * ( endOffset - startOffset );
                }
                else
                {
                    endOffset = qMax( finHostLinkConnect->combinVar.addOffset + ( combineCount + 1 ) / 2, pHmiVar->addOffset + ( varCount + 1 ) / 2 );
                    newLen = 4 * ( endOffset - startOffset );
                }

                if ( newLen <= FIN_M_HOSTLINK_MAX_WORD * 2 )                   //最多组合 MAX_HLINK_PACKET_LEN 字节
                {
                    if ( newLen <= 2*( combineCount + varCount ) + 16 ) //能节省10字节 两个变量之间的距离超过10/2就不合并
                    {
                        finHostLinkConnect->combinVar.addOffset = startOffset;
                        finHostLinkConnect->combinVar.bitOffset = -1;
                        finHostLinkConnect->combinVar.len = newLen;
                        finHostLinkConnect->combinVarsList.append( pHmiVar );
                    }
                }
                else
                    continue;
            }
            else if ( pHmiVar->area == FIN_HOST_LINK_AREA_T_BIT ||
                            pHmiVar->area == FIN_HOST_LINK_AREA_C_BIT )
            {
                endOffset = qMax( finHostLinkConnect->combinVar.addOffset + finHostLinkConnect->combinVar.len, pHmiVar->addOffset + pHmiVar->len );
                newLen = endOffset - startOffset;
                if ( newLen <= FIN_M_HOSTLINK_MAX_BIT )
                {
                    if ( newLen <= finHostLinkConnect->combinVar.len + pHmiVar->len + 16 ) //能节省2字节 两个变量之间的距离超过 8 则不合并
                    {
                        finHostLinkConnect->combinVar.addOffset = startOffset;
                        finHostLinkConnect->combinVar.bitOffset = -1;
                        finHostLinkConnect->combinVar.len = newLen;
                        finHostLinkConnect->combinVarsList.append( pHmiVar );
                    }
                }
                else
                    continue;
            }
            else if ( pHmiVar->area >= FIN_HOST_LINK_AREA_CIO_BIT &&
                            pHmiVar->area <= FIN_HOST_LINK_AREA_EMC_BIT )
            {
                int startBitOffset = 0;
                int endBitOffset = 0;

                if ( finHostLinkConnect->combinVar.addOffset > pHmiVar->addOffset )
                {
                    startOffset = pHmiVar->addOffset;
                    startBitOffset = pHmiVar->bitOffset;
                }
                else if ( finHostLinkConnect->combinVar.addOffset < pHmiVar->addOffset )
                {
                    startOffset = finHostLinkConnect->combinVar.addOffset;
                    startBitOffset = finHostLinkConnect->combinVar.bitOffset;
                }
                else
                {
                    startOffset = finHostLinkConnect->combinVar.addOffset;
                    startBitOffset = qMin( finHostLinkConnect->combinVar.bitOffset, pHmiVar->bitOffset );
                }

                if (( finHostLinkConnect->combinVar.addOffset + ( finHostLinkConnect->combinVar.bitOffset + finHostLinkConnect->combinVar.len ) / 16 ) >
                                ( pHmiVar->addOffset + ( pHmiVar->bitOffset + pHmiVar->len ) / 16 ) )
                {
                    endOffset = finHostLinkConnect->combinVar.addOffset + ( finHostLinkConnect->combinVar.bitOffset + finHostLinkConnect->combinVar.len ) / 16;
                    endBitOffset = ( finHostLinkConnect->combinVar.bitOffset + finHostLinkConnect->combinVar.len ) % 16;
                }
                else if (( finHostLinkConnect->combinVar.addOffset + ( finHostLinkConnect->combinVar.bitOffset + finHostLinkConnect->combinVar.len ) / 16 ) >
                                ( pHmiVar->addOffset + ( pHmiVar->bitOffset + pHmiVar->len ) / 16 ) )
                {
                    endOffset = pHmiVar->addOffset + ( pHmiVar->bitOffset + pHmiVar->len ) / 16;
                    endBitOffset = ( pHmiVar->bitOffset + pHmiVar->len ) % 16;
                }
                else
                {
                    endOffset = pHmiVar->addOffset + ( pHmiVar->bitOffset + pHmiVar->len ) / 16;
                    endBitOffset = qMax(( finHostLinkConnect->combinVar.bitOffset + finHostLinkConnect->combinVar.len ) % 16,
                                        ( pHmiVar->bitOffset + pHmiVar->len ) % 16 );
                }

                newLen = endBitOffset - startBitOffset + ( endOffset - startOffset ) * 16;

                if ( newLen <= FIN_M_HOSTLINK_MAX_BIT )
                {
                    if ( newLen <= finHostLinkConnect->combinVar.len + pHmiVar->len + 16 ) //能节省2字节 两个变量之间的距离超过 8 则不合并
                    {
                        finHostLinkConnect->combinVar.addOffset = startOffset;
                        finHostLinkConnect->combinVar.bitOffset = startBitOffset;
                        finHostLinkConnect->combinVar.len = newLen;
                        finHostLinkConnect->combinVarsList.append( pHmiVar );
                    }
                }
            }
            else
                continue;

        }
    }
}

bool FinHostlinkService::hasProcessingReq( struct FinHostlinkConnect* finHostLinkConnect )
{
    return ( finHostLinkConnect->writingCnt > 0 || finHostLinkConnect->requestVarsList.count() > 0 );
}

bool FinHostlinkService::initCommDev()
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
        struct FinHostlinkConnect* conn = m_connectList[0];

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

            m_rwSocket = new RWSocket(FINHOSTLINK, &uartCfg, conn->cfg.id);
            connect(m_rwSocket, SIGNAL(readyRead()),this,SLOT(slot_readData()));
            m_rwSocket->openDev();
        }
    }
    return true;
}

bool FinHostlinkService::processRspData( struct finHlinkMsgPar msgPar ,struct ComMsg comMsg )
{
    bool result = true;

    if ( m_devUseCnt > 0 )
    {
        m_devUseCnt--;
    }
            for ( int i = 0; i < m_connectList.count(); i++ )
            {
                struct FinHostlinkConnect* finHostLinkConnect = m_connectList[i];
                Q_ASSERT( finHostLinkConnect );
                if ( finHostLinkConnect->cfg.cpuAddr == msgPar.slave ) //属于这个连接
                {
                    //qDebug()<<"ID =  "<<msgPar.varId<<"    rw = "<<msgPar.rw;
                    if ( finHostLinkConnect->writingCnt > 0 && finHostLinkConnect->waitWrtieVarsList.count() > 0 && msgPar.rw == 1 )
                    {                                     // 处理写请求
                        //qDebug()<<"writingCnt  "<<finHostLinkConnect->writingCnt<<"    waitWrtieVarsList    "<<finHostLinkConnect->waitWrtieVarsList.count();
                        Q_ASSERT( msgPar.rw == 1 );
                        finHostLinkConnect->writingCnt = 0;
                        //qDebug() << "Write err:" << msgPar.err;
                        struct HmiVar* pHmiVar = finHostLinkConnect->waitWrtieVarsList.takeFirst(); //删除写请求记录
                        m_mutexRw.lock();
                        //pHmiVar->err = msgPar.err;
                        pHmiVar->wStatus = msgPar.err;
                        m_mutexRw.unlock();
                        comMsg.varId = ( pHmiVar->varId & 0x0000ffff );
                        comMsg.type = 1;
                        comMsg.err = msgPar.err;
                        m_comMsgTmp.insertMulti( comMsg.varId, comMsg );
                    }
                    else if ( finHostLinkConnect->requestVarsList.count() > 0 && msgPar.rw == 0 )
                    {
                        Q_ASSERT( msgPar.rw == 0 );
                        Q_ASSERT( msgPar.area == finHostLinkConnect->requestVar.area );
                        //qDebug() << "Read err:" << msgPar.err;
                        if ( msgPar.err == VAR_PARAM_ERROR && finHostLinkConnect->requestVarsList.count() > 1 )
                        {
                            for ( int j = 0; j < finHostLinkConnect->requestVarsList.count(); j++ )
                            {
                                struct HmiVar* pHmiVar = finHostLinkConnect->requestVarsList[j];
                                pHmiVar->overFlow = true;
                                pHmiVar->lastUpdateTime = -1; //合并后越界的不算越界，立即再次更新（更新时不合并）
                            }
                        }
                        else
                        {
                            for ( int j = 0; j < finHostLinkConnect->requestVarsList.count(); j++ )
                            {
                                m_mutexRw.lock();
                                struct HmiVar* pHmiVar = finHostLinkConnect->requestVarsList[j];

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
                                    if ( msgPar.area >= FIN_HOST_LINK_AREA_CIO &&
                                                    msgPar.area <= FIN_HOST_LINK_AREA_IR )
                                    {
                                        Q_ASSERT( msgPar.count <= FIN_M_HOSTLINK_MAX_WORD );
                                        int offset = pHmiVar->addOffset - finHostLinkConnect->requestVar.addOffset;
                                        Q_ASSERT( offset >= 0 );
                                        if ( msgPar.area == FIN_HOST_LINK_AREA_IR )
                                        {
                                            Q_ASSERT( 4*offset <= FIN_M_HOSTLINK_MAX_WORD*2 );

                                            int len = 4 * (( pHmiVar->len + 3 ) / 4 );
                                            if ( pHmiVar->rData.size() < len )
                                                pHmiVar->rData = QByteArray( len, 0 ); //初始化为0

                                            if ( 4*offset + len <= FIN_M_HOSTLINK_MAX_WORD*2 ) //保证不越界
                                            {
                                                memcpy( pHmiVar->rData.data(), &msgPar.data[4*offset], len );
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
                                            Q_ASSERT( 2*offset <= FIN_M_HOSTLINK_MAX_WORD*2 );

                                            if ( pHmiVar->dataType == Com_Bool )
                                            {
                                                int len = 2 * (( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16 );
                                                if ( pHmiVar->rData.size() < len )
                                                    pHmiVar->rData = QByteArray( len, 0 ); //初始化为0
                                                //pHmiVar->rData.resize(len);
                                                if ( 2*offset + len <= FIN_M_HOSTLINK_MAX_WORD*2 ) //保证不越界
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

                                                if ( 2*offset + len <= FIN_M_HOSTLINK_MAX_WORD*2 ) //保证不越界
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

                                    }
                                    else if ( msgPar.area >= FIN_HOST_LINK_AREA_CIO_BIT &&
                                                    msgPar.area <= FIN_HOST_LINK_AREA_EMC_BIT )
                                    {
                                        Q_ASSERT( msgPar.count <= FIN_M_HOSTLINK_MAX_BIT );
                                        if ( !finHostLinkConnect->waitWrtieVarsList.contains( pHmiVar ) )
                                        {
                                            int startBitOffset = ( pHmiVar->addOffset - finHostLinkConnect->requestVar.addOffset ) * 16;
                                            startBitOffset += ( pHmiVar->bitOffset - finHostLinkConnect->requestVar.bitOffset );

                                            if ( msgPar.area == FIN_HOST_LINK_AREA_T_BIT ||
                                                            msgPar.area == FIN_HOST_LINK_AREA_C_BIT )
                                                startBitOffset = pHmiVar->addOffset - finHostLinkConnect->requestVar.addOffset;

                                            Q_ASSERT( startBitOffset >= 0 && ( startBitOffset + pHmiVar->len ) <= FIN_M_HOSTLINK_MAX_BIT );
                                            int len = ( pHmiVar->len + 7 ) / 8;
                                            if ( pHmiVar->rData.size() < len )
                                                pHmiVar->rData = QByteArray( len, 0 ); //初始化为0

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
                                }
                                else if ( msgPar.err == VAR_COMM_TIMEOUT )
                                {

                                    if ( pHmiVar->varId == 65535 )
                                    {
                                        foreach( HmiVar* tempHmiVar, finHostLinkConnect->actVarsList )
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
                        finHostLinkConnect->requestVarsList.clear();
                    }
                    if ( msgPar.err == VAR_COMM_TIMEOUT )
                        finHostLinkConnect->connStatus = CONNECT_NO_LINK;
                    else
                        finHostLinkConnect->connStatus = CONNECT_LINKED_OK;
                    break;
                }
    }
    return result;
}



void FinHostlinkService::slot_run()
{
    initCommDev();
    m_runTimer->start(10);
}
void FinHostlinkService::slot_readData()
{
    struct finHlinkMsgPar msgPar;
    struct ComMsg comMsg;
    int ret;
    do
    {
        ret = m_rwSocket->readDev((char *)&msgPar);
        switch (msgPar.err)
        {
        case COM_OPEN_ERR:
            qDebug()<<"COM_OPEN_ERR";
            m_devUseCnt = 0;
            for ( int i = 0; i < m_connectList.count(); i++ )
            {
                struct FinHostlinkConnect* finHostLinkConnect = m_connectList[i];
                Q_ASSERT( finHostLinkConnect );
                finHostLinkConnect->waitWrtieVarsList.clear();
                finHostLinkConnect->requestVarsList.clear();
                foreach( HmiVar* tempHmiVar, finHostLinkConnect->actVarsList )
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
    while ( ret == sizeof( struct finHlinkMsgPar ) );

}
void FinHostlinkService::slot_runTimerTimeout()
{
    m_runTimer->stop();
    int lastUpdateTime = -1;


        int connectCount = m_connectList.count();
        m_currTime = getCurrTime();
        int currTime = m_currTime;

        for ( int i = 0; i < connectCount; i++ )
        {                              //处理写请求
            //int curIndex = ( lastConnect + 1 + i ) % connectCount;//保证从前1次的后1个连接开始,使各连接机会均等
            struct FinHostlinkConnect* finHostLinkConnect = m_connectList[i];
            updateWriteReq( finHostLinkConnect );
            trySndWriteReq( finHostLinkConnect );
        }

        bool needUpdateFlag = false;

        if (( lastUpdateTime == -1 ) || ( calIntervalTime( lastUpdateTime, currTime ) >= 48 ) )
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
        //bool hasProcessingReqFlag = false;
        for ( int i = 0; i < connectCount; i++ )
        {                              // 处理读请求
            //int curIndex = ( lastConnect + 1 + i ) % connectCount;//保证从前1次的后1个连接开始,使各连接机会均等
            struct FinHostlinkConnect* finHostLinkConnect = m_connectList[i];
            if ( needUpdateFlag )
            {
                updateOnceRealReadReq( finHostLinkConnect );
                updateReadReq( finHostLinkConnect ); //无论如何都要重组请求变量
            }
            if ( !( hasProcessingReq( finHostLinkConnect ) ) )
        {
            combineReadReq( finHostLinkConnect );
        }
        trySndReadReq( finHostLinkConnect );
    }
    m_runTimer->start(20);
}

