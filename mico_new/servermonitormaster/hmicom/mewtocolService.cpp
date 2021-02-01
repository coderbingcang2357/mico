
#include "mewtocolService.h"



MewtocolService::MewtocolService( struct MewtocolConnectCfg& connectCfg )
{
    struct MewtocolConnect* mewConnect = new MewtocolConnect;
    mewConnect->cfg = connectCfg;
    mewConnect->writingCnt = 0;
    mewConnect->connStatus = CONNECT_NO_OPERATION;
    mewConnect->connectTestVar.lastUpdateTime = -1;
    m_connectList.append( mewConnect );
    m_currTime = getCurrTime();
    m_runTimer = NULL;
    m_rwSocket = NULL;
}

MewtocolService::~MewtocolService()
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

bool MewtocolService::meetServiceCfg( int serviceType, int COM_interface )
{
    if ( serviceType == SERVICE_TYPE_MEWTOCOL/*SERVICE_TYPE_FINHOSTLINK*/ )
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

int MewtocolService::getConnLinkStatus( int connectId )
{
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct MewtocolConnect* mewConnect = m_connectList[i];
        if ( connectId == mewConnect->cfg.id )
            return mewConnect->connStatus;
    }
    return -1;
}

int MewtocolService::getComMsg( QList<struct ComMsg> &msgList )
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

int MewtocolService:: splitLongVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct MewtocolConnect* mewConnect = m_connectList[i];
        if ( pHmiVar->connectId == mewConnect->cfg.id )
        {
            pHmiVar->lastUpdateTime = -1; //立即更新
            if ( pHmiVar->area >= MEWTOCOL_AREA_DT && pHmiVar->area <= MEWTOCOL_AREA_T_BIT )
            {
                if ( pHmiVar->dataType == Com_Bool )
                {
                    Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
                    if ( pHmiVar->len > M_MEWTOCOL_MAX_WORD*16 && pHmiVar->splitVars.count() <= 0 )  //拆分 处理BIT类型
                    {
                        for ( int splitedLen = 0; splitedLen < pHmiVar->len; )
                        {
                            int len = pHmiVar->len - splitedLen;
                            if ( len > M_MEWTOCOL_MAX_WORD * 16 )
                                len = M_MEWTOCOL_MAX_WORD * 16;

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
                else if ( pHmiVar->len > ( M_MEWTOCOL_MAX_WORD * 2 ) && pHmiVar->splitVars.count() <= 0 )  //拆分 处理word类型
                {
                    for ( int splitedLen = 0; splitedLen < pHmiVar->len; )
                    {
                        int len = pHmiVar->len - splitedLen;
                        if ( len > ( M_MEWTOCOL_MAX_WORD * 2 ) )
                            len = ( M_MEWTOCOL_MAX_WORD  * 2 );

                        struct HmiVar* pSplitHhiVar = new HmiVar;
                        pSplitHhiVar->varId = pHmiVar->varId + (( pHmiVar->splitVars.count() + 1 ) << 16 );
                        pSplitHhiVar->connectId = pHmiVar->connectId;
                        pSplitHhiVar->cycleTime = pHmiVar->cycleTime;
                        pSplitHhiVar->area = pHmiVar->area;
                        pSplitHhiVar->addOffset = pHmiVar->addOffset + splitedLen / 2;
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

int MewtocolService::onceRealUpdateVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    m_mutexOnceReal.lock();
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct MewtocolConnect* mewConnect = m_connectList[i];
        if ( pHmiVar->connectId == mewConnect->cfg.id )
        {
            pHmiVar->lastUpdateTime = -1; //立即更新
            if ( pHmiVar->splitVars.count() > 0 )
            {
                for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
                {
                    struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                    pSplitHhiVar->lastUpdateTime = -1;
                    pSplitHhiVar->overFlow = false;
                    if ( !mewConnect->onceRealVarsList.contains( pSplitHhiVar ) )
                        mewConnect->onceRealVarsList.append( pSplitHhiVar );
                }
            }
            else
            {
                pHmiVar->lastUpdateTime = -1;
                pHmiVar->overFlow = false;
                if ( !mewConnect->onceRealVarsList.contains( pHmiVar ) )
                    mewConnect->onceRealVarsList.append( pHmiVar );
            }

            m_mutexOnceReal.unlock();
            return 0;
        }
    }
    m_mutexOnceReal.unlock();
    return -1;
}

int MewtocolService::startUpdateVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );

    m_mutexUpdateVar.lock();
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct MewtocolConnect* mewConnect = m_connectList[i];
        if ( pHmiVar->connectId == mewConnect->cfg.id )
        {
            pHmiVar->lastUpdateTime = -1;
            if ( pHmiVar->splitVars.count() > 0 )
            {
                for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
                {
                    struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                    pSplitHhiVar->lastUpdateTime = -1;
                    pSplitHhiVar->overFlow = false;
                    if ( !mewConnect->actVarsList.contains( pSplitHhiVar ) )
                        mewConnect->actVarsList.append( pSplitHhiVar );
                }
            }
            else
            {
                pHmiVar->lastUpdateTime = -1;
                pHmiVar->overFlow = false;
                if ( !mewConnect->actVarsList.contains( pHmiVar ) )
                    mewConnect->actVarsList.append( pHmiVar );
            }
            //qDebug() << "startUpdateVar：" << pHmiVar->varId;

            m_mutexUpdateVar.unlock();
            return 0;
        }
    }
    m_mutexUpdateVar.unlock();
    return -1;
}

int MewtocolService::stopUpdateVar( struct HmiVar* pHmiVar )
{
    //qDebug()<<"pHmiVar ID = "<<pHmiVar->varId;
    Q_ASSERT( pHmiVar );
    m_mutexUpdateVar.lock();
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct MewtocolConnect* mewConnect = m_connectList[i];
        if ( pHmiVar->connectId == mewConnect->cfg.id )
        {
            if ( pHmiVar->splitVars.count() > 0 )
            {
                for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
                {
                    struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                    mewConnect->actVarsList.removeAll( pSplitHhiVar );
                }
            }
            else
                mewConnect->actVarsList.removeAll( pHmiVar );
            m_mutexUpdateVar.unlock();
            return 0;
        }
    }
    m_mutexUpdateVar.unlock();
    return -1;
}

int MewtocolService::readVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    if ( pHmiVar->err == VAR_NO_ERR )  //无错误才拷贝数据
    {
        if ( pHmiVar->area >= MEWTOCOL_AREA_DT &&
                        pHmiVar->area <= MEWTOCOL_AREA_T_BIT )
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
                    if ( pHmiVar->dataType == Com_String )
                    {
                        if ( j & 0x1 )
                        {
                            pBuf[j] = pHmiVar->rData.at( j - 1 );
                        }
                        else
                        {
                            pBuf[j] = pHmiVar->rData.at( j + 1 );
                        }
                    }
                    else
                        pBuf[j] = pHmiVar->rData.at( j );
                }
            }
            else
                Q_ASSERT( false );
        }
    }
    return pHmiVar->err;
}

int MewtocolService::readDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    Q_ASSERT( pHmiVar && pBuf );
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct MewtocolConnect* mewConnect = m_connectList[i];
        if ( pHmiVar->connectId == mewConnect->cfg.id )
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

int MewtocolService::writeVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    int ret;
    Q_ASSERT( pHmiVar && pBuf );
    m_mutexRw.lock();
    if ( pHmiVar->area >= MEWTOCOL_AREA_DT && pHmiVar->area <= MEWTOCOL_AREA_WL )
    {                            //大小数等到发送写命令时再处理
        if ( pHmiVar->dataType == Com_Bool )
        {
            Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
            int count = ( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16;  //字数
            Q_ASSERT( count > 0 && count <= M_MEWTOCOL_MAX_WORD );
            if ( count > M_MEWTOCOL_MAX_WORD )
                count = M_MEWTOCOL_MAX_WORD;
            int dataBytes = 2 * count;
            dataBytes = qMin( bufLen, dataBytes );
            if ( pHmiVar->wData.size() < dataBytes )
                pHmiVar->wData.resize( dataBytes );

            memcpy( pHmiVar->wData.data(), pBuf, bufLen );
            pHmiVar->lastUpdateTime = -1;

            if ( pHmiVar->rData.size() < dataBytes )
                pHmiVar->rData.resize( dataBytes );

            for ( int i = 0; i < pHmiVar->len; i++ )
            {
                int byteW = i / 8;
                int bitW = i % 8;
                int byteR = ( pHmiVar->bitOffset + i ) / 8;
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
            Q_ASSERT( count > 0 && count <= M_MEWTOCOL_MAX_WORD );
            if ( count > M_MEWTOCOL_MAX_WORD )
                count = M_MEWTOCOL_MAX_WORD;
            int dataBytes = 2 * count;
            //dataBytes = qMin( bufLen, pHmiVar->len );

            if ( pHmiVar->wData.size() < dataBytes )
                pHmiVar->wData.resize( dataBytes );

            memcpy( pHmiVar->wData.data(), pBuf, bufLen );
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
                            pHmiVar->rData[j - 1] = pHmiVar->wData.at( j );
                        else
                            pHmiVar->rData[j + 1] = pHmiVar->wData.at( j );
                    }
                    else
                        pHmiVar->rData[j] = pHmiVar->wData.at( j );
                }
                else
                    break;
            }

        }
    }
    else if ( pHmiVar->area >= MEWTOCOL_AREA_C_BIT && pHmiVar->area <= MEWTOCOL_AREA_T_BIT )
        return -1;
    else
    {
        Q_ASSERT( false );
    }
    pHmiVar->wStatus = VAR_NO_OPERATION;
    ret = pHmiVar->err;
    m_mutexRw.unlock();

    return ret; //是否要返回成功
}

int MewtocolService::writeDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    Q_ASSERT( pHmiVar && pBuf );
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct MewtocolConnect* mewConnect = m_connectList[i];
        if ( pHmiVar->connectId == mewConnect->cfg.id )
        {
            if ( pHmiVar->area == MEWTOCOL_AREA_T_BIT || mewConnect->connStatus != CONNECT_LINKED_OK )
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
                    mewConnect->writeVarsList.append( pSplitHhiVar );
                    pBuf += len;
                    bufLen -= len;
                }
            }
            else
            {
                ret = writeVar( pHmiVar, pBuf, bufLen );
                mewConnect->writeVarsList.append( pHmiVar );
            }
            m_mutexWReq.unlock();

            return ret; //是否要返回成功
        }
    }
    return -1;
}

int MewtocolService::addConnect( struct MewtocolConnectCfg& connectCfg )
{                           //要检查是否重复 add
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct MewtocolConnect* mewConnect = m_connectList[i];
        Q_ASSERT( mewConnect->cfg.COM_interface == connectCfg.COM_interface );//绝对要是同一个设备接口
        if ( connectCfg.id == mewConnect->cfg.id )
        {
            Q_ASSERT( false );
            return 0;
        }
    }
    struct MewtocolConnect* mewConnect = new MewtocolConnect;
    mewConnect->cfg = connectCfg;
    mewConnect->writingCnt = 0;
    mewConnect->connStatus = CONNECT_NO_OPERATION;
    mewConnect->connectTestVar.lastUpdateTime = -1;
    m_connectList.append( mewConnect );
    return 0;
}

int MewtocolService::sndWriteVarMsgToDriver( struct MewtocolConnect* mewConnect, struct HmiVar* pHmiVar )
{
    int sendedLen = 0;
    Q_ASSERT( mewConnect && pHmiVar );
//#ifdef Q_WS_QWS
    struct MewtocolMsgPar msgPar;
    //qDebug()<<"sndWriteVarMsgToDriver";
    m_mutexRw.lock();
    msgPar.rw = 1;   //写
    msgPar.slave = mewConnect->cfg.cpuAddr;
    msgPar.area = pHmiVar->area;
    msgPar.wordAddr = pHmiVar->addOffset;
    msgPar.err = 0;

    if ( pHmiVar->area >= MEWTOCOL_AREA_DT && pHmiVar->area <= MEWTOCOL_AREA_WL )
    {
        msgPar.bitAddr = 0;
        if ( pHmiVar->dataType == Com_Bool )
        {
            Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
            int count = ( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16;  //字数
            Q_ASSERT( count > 0 && count <= M_MEWTOCOL_MAX_WORD );
            if ( count > M_MEWTOCOL_MAX_WORD )
                count = M_MEWTOCOL_MAX_WORD;
            msgPar.count = count;      // 占用的总字数
            int dataBytes = 2 * count;
            if ( pHmiVar->rData.size() < dataBytes )
                pHmiVar->rData.resize( dataBytes );

            for ( int i = 0; i < pHmiVar->len; i++ )
            {
                int byteW = i / 8;
                int bitW = i % 8;
                int byteR = ( pHmiVar->bitOffset + i ) / 8;

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
            Q_ASSERT( count > 0 && count <= M_MEWTOCOL_MAX_WORD );
            if ( count > M_MEWTOCOL_MAX_WORD )
                count = M_MEWTOCOL_MAX_WORD;

            msgPar.count = count;      // 占用的总字数
            msgPar.bitAddr = 0;

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
                            pHmiVar->rData[j - 1] = pHmiVar->wData.at( j );
                        else
                            pHmiVar->rData[j + 1] = pHmiVar->wData.at( j );
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
    else if ( pHmiVar->area == MEWTOCOL_AREA_T_BIT || pHmiVar->area == MEWTOCOL_AREA_C_BIT )
        return -1;
    else
    {
        msgPar.count = 0;
        Q_ASSERT( false );
    }

    if ( sendedLen > 0 )
    {
        m_devUseCnt++;
        m_mutexRw.unlock();
        //int len = write( m_fd, ( char* ) & msgPar, sizeof( struct MewtocolMsgPar ) );
        int len = m_rwSocket->writeDev(( char* ) & msgPar, sizeof( struct MewtocolMsgPar ));
        Q_ASSERT( len > 0 );
    }
    else
        m_mutexRw.unlock();
//#endif
    return sendedLen;
}

int MewtocolService::sndReadVarMsgToDriver( struct MewtocolConnect* mewConnect, struct HmiVar* pHmiVar )
{
    int sendedLen = 0;
    Q_ASSERT( mewConnect && pHmiVar );
//#ifdef Q_WS_QWS
    struct MewtocolMsgPar msgPar;

    m_mutexRw.lock();
    msgPar.rw = 0;    // 读
    msgPar.slave = mewConnect->cfg.cpuAddr;
    msgPar.area = pHmiVar->area;
    msgPar.wordAddr = pHmiVar->addOffset;
    msgPar.err = 0;

    if ( pHmiVar->area >= MEWTOCOL_AREA_DT &&  pHmiVar->area <= MEWTOCOL_AREA_T_BIT )
    {
        if ( pHmiVar->dataType == Com_Bool )
        {
            Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
            int count = ( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16;  //字数
            Q_ASSERT( count > 0 && count <= M_MEWTOCOL_MAX_WORD );
            if ( count > M_MEWTOCOL_MAX_WORD )
                count = M_MEWTOCOL_MAX_WORD;
            msgPar.count = count;      // 占用的总字数
            sendedLen = count * 16;
        }
        else
        {
            int count = ( pHmiVar->len + 1 ) / 2;  //字数
            Q_ASSERT( count > 0 && count <= M_MEWTOCOL_MAX_WORD );
            if ( count > M_MEWTOCOL_MAX_WORD )
                count = M_MEWTOCOL_MAX_WORD;

            msgPar.count = count;      // 占用的总字数
            //qDebug()<<"                read  Count = "<<count;
            msgPar.bitAddr = 0;
            sendedLen = count * 2;
        }
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
        //int len = write( m_fd, ( char* ) & msgPar, sizeof( struct MewtocolMsgPar ) );
        int len = m_rwSocket->writeDev( ( char* ) & msgPar, sizeof( struct MewtocolMsgPar ));
        Q_ASSERT( len > 0 );
    }
    else
        m_mutexRw.unlock();
//#endif
    return sendedLen;
}

bool isWaitingReadRsp( struct MewtocolConnect* mewConnect, struct HmiVar* pHmiVar )
{
    Q_ASSERT( mewConnect && pHmiVar );
    if ( mewConnect->waitUpdateVarsList.contains( pHmiVar ) )
        return true;
    else if ( mewConnect->combinVarsList.contains( pHmiVar ) )
        return true;
    else if ( mewConnect->requestVarsList.contains( pHmiVar ) )
        return true;
    else
        return false;
}

bool MewtocolService::trySndWriteReq( struct MewtocolConnect* mewConnect ) //不处理超长变量
{
    if ( m_devUseCnt < MAXDEVCNT )                          //没有正在读写, snd read, 一次只能发送一包请求
    {
        if ( mewConnect->writingCnt == 0 && mewConnect->requestVarsList.count() == 0 ) //这个连接没有读写
        {
            m_mutexWReq.lock();
            if ( mewConnect->waitWriteVarsList.count() > 0 )
            {
                struct HmiVar* pHmiVar = mewConnect->waitWriteVarsList[0];
                if (( pHmiVar->lastUpdateTime == -1 )  // WORD区的字符串变量要先读
                                && ( pHmiVar->area >= MEWTOCOL_AREA_DT &&
                                     pHmiVar->area <= MEWTOCOL_AREA_T_BIT )
                                && ( pHmiVar->dataType == Com_String ||  pHmiVar->dataType == Com_Bool ) )
                {
                    //pHmiVar->lastUpdateTime = m_currTime;//getCurrTime();
                    if ( !isWaitingReadRsp( mewConnect, pHmiVar ) )
                        mewConnect->waitUpdateVarsList.prepend( pHmiVar ); //加入到读队列最前,保证下次最先读
                }
                else if ( !isWaitingReadRsp( mewConnect, pHmiVar ) ) //没有正在等待读
                {                                   //直接写,snd write cmd to driver
                    if ( sndWriteVarMsgToDriver( mewConnect, pHmiVar ) > 0 )
                    {
                        //pHmiVar->lastUpdateTime = m_currTime;//getCurrTime();
                        mewConnect->writingCnt = 1;    //进入写状态
                        m_mutexWReq.unlock();
                        return true;
                    }
                    else
                    {
                        mewConnect->waitWriteVarsList.takeAt( 0 );//出列,丢掉写请求
                        pHmiVar->err = VAR_PARAM_ERROR;
                    }
                }
            }
            m_mutexWReq.unlock();
        }
    }
    return false;
}

bool MewtocolService::trySndReadReq( struct MewtocolConnect* mewConnect ) //不处理超长变量
{
    if ( m_devUseCnt < MAXDEVCNT )             //没有正在读写, snd read, 一次只能发送一包请求
    {
        if ( mewConnect->writingCnt == 0 && mewConnect->requestVarsList.count() == 0 ) //这个连接没有读写
        {
            if ( mewConnect->combinVarsList.count() > 0 )
            {
                mewConnect->requestVarsList = mewConnect->combinVarsList;
                mewConnect->requestVar = mewConnect->combinVar;
                for ( int i = 0; i < mewConnect->combinVarsList.count(); i++ )
                {                      //要把已经请求的变量从等待队列中删除
                    struct HmiVar* pHmiVar = mewConnect->combinVarsList[i];
                    mewConnect->waitUpdateVarsList.removeAll( pHmiVar );
                    pHmiVar->lastUpdateTime = m_currTime;
                }
                mewConnect->combinVarsList.clear();

                if ( sndReadVarMsgToDriver( mewConnect, &( mewConnect->requestVar ) ) > 0 )
                {
                    return true;
                }
                else
                {
                    for ( int i = 0; i < mewConnect->requestVarsList.count(); i++ )
                    {
                        struct HmiVar* pHmiVar = mewConnect->requestVarsList[i];
                        pHmiVar->err = VAR_PARAM_ERROR;
                    }
                    mewConnect->requestVarsList.clear();
                }
            }
        }
    }
    return false;
}

bool MewtocolService::updateWriteReq( struct MewtocolConnect* mewConnect )
{
    bool newUpdateVarFlag = false;
    QList<struct HmiVar*> tmpList;
    m_mutexWReq.lock();
    //int currTime = m_currTime;//getCurrTime();
    for ( int i = 0; i < mewConnect->writeVarsList.count(); i++ )
    {
        struct HmiVar* pHmiVar = mewConnect->writeVarsList[i];


        if ( !mewConnect->waitWriteVarsList.contains( pHmiVar ) )
        {
            mewConnect->waitWriteVarsList.append( pHmiVar );
            newUpdateVarFlag = true;
        }
        else
        {
            tmpList.append(pHmiVar);
        }
    }

    mewConnect->writeVarsList.clear();
    mewConnect->writeVarsList += tmpList;
    m_mutexWReq.unlock();

    return newUpdateVarFlag;
}
bool MewtocolService::updateReadReq( struct MewtocolConnect* mewConnect )
{
    bool newUpdateVarFlag = false;
    m_mutexUpdateVar.lock();
    int currTime = m_currTime;//getCurrTime();
    if ( mewConnect->connStatus == CONNECT_LINKED_OK )
    {
        for ( int i = 0; i < mewConnect->actVarsList.count(); i++ )
        {                                  // 找出需要刷新的变量
            struct HmiVar* pHmiVar = mewConnect->actVarsList[i];
            if (( pHmiVar->lastUpdateTime == -1 ) || ( calIntervalTime( pHmiVar->lastUpdateTime, currTime ) >= pHmiVar->cycleTime ) )
            {                              // 需要刷新
                if (( !mewConnect->waitUpdateVarsList.contains( pHmiVar ) )
                                && ( !mewConnect->waitWriteVarsList.contains( pHmiVar ) )
                                //&& ( !mewConnect->combinVarsList.contains( pHmiVar ) )
                                && ( !mewConnect->requestVarsList.contains( pHmiVar ) ) )
                {
                    //pHmiVar->lastUpdateTime = currTime;
                    mewConnect->waitUpdateVarsList.append( pHmiVar );
                    newUpdateVarFlag = true;
                }
            }
        }
    }
    else
    {
        mewConnect->waitUpdateVarsList.clear();
        if ( mewConnect->actVarsList.count() > 0 && mewConnect->requestVarsList.count() == 0 )
        {//qDebug()<<"connectTestVar  lastUpdateTime  = "<<mewConnect->connectTestVar.lastUpdateTime;
            mewConnect->actVarsList[0]->lastUpdateTime = mewConnect->connectTestVar.lastUpdateTime;
            mewConnect->connectTestVar = *( mewConnect->actVarsList[0] );
            mewConnect->connectTestVar.varId = 65535;
            mewConnect->connectTestVar.cycleTime = 2000;
            if (( mewConnect->connectTestVar.lastUpdateTime == -1 )
                            || ( calIntervalTime( mewConnect->connectTestVar.lastUpdateTime, currTime ) >= mewConnect->connectTestVar.cycleTime ) )
            {
                if (( !mewConnect->waitUpdateVarsList.contains( &( mewConnect->connectTestVar ) ) )
                                && ( !mewConnect->waitWriteVarsList.contains( &( mewConnect->connectTestVar ) ) )
                                && ( !mewConnect->requestVarsList.contains( &( mewConnect->connectTestVar ) ) ) )
                {
                    mewConnect->waitUpdateVarsList.append( &( mewConnect->connectTestVar ) );
                    newUpdateVarFlag = true;
                }
            }
        }
    }
    m_mutexUpdateVar.unlock();
    return newUpdateVarFlag;
}

bool MewtocolService::updateOnceRealReadReq( struct MewtocolConnect* mewConnect )
{
    bool newUpdateVarFlag = false;
    m_mutexOnceReal.lock();
    //int currTime = m_currTime;//getCurrTime();
    for ( int i = 0; i < mewConnect->onceRealVarsList.count(); i++ )
    {
        struct HmiVar* pHmiVar = mewConnect->onceRealVarsList[i];
        if (( !mewConnect->waitUpdateVarsList.contains( pHmiVar ) )
                        && ( !mewConnect->requestVarsList.contains( pHmiVar ) ) )
        {
            mewConnect->waitUpdateVarsList.prepend( pHmiVar );
            newUpdateVarFlag = true;
        }
        else
        {
            if ( mewConnect->waitUpdateVarsList.contains( pHmiVar ) )
            {
                mewConnect->waitUpdateVarsList.removeAll( pHmiVar );
                mewConnect->waitUpdateVarsList.prepend( pHmiVar );
            }
        }
    }
    mewConnect->onceRealVarsList.clear();
    m_mutexOnceReal.unlock();

    return newUpdateVarFlag;
}
void MewtocolService::combineReadReq( struct MewtocolConnect* mewConnect )
{
    mewConnect->combinVarsList.clear();
    if ( mewConnect->waitUpdateVarsList.count() >= 1 )
    {
        struct HmiVar* pHmiVar = mewConnect->waitUpdateVarsList[0];
        mewConnect->combinVarsList.append( pHmiVar );
        mewConnect->combinVar = *pHmiVar;
        if (( mewConnect->combinVar.area >= MEWTOCOL_AREA_DT && mewConnect->combinVar.area <= MEWTOCOL_AREA_T_BIT ) && mewConnect->combinVar.dataType == Com_Bool )
        {
            Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
            mewConnect->combinVar.dataType = Com_Word;
            mewConnect->combinVar.len = 2 * (( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16 ); //2*count
            mewConnect->combinVar.bitOffset = -1;
        }
    }
    for ( int i = 1; i < mewConnect->waitUpdateVarsList.count(); i++ )
    {
        struct HmiVar* pHmiVar = mewConnect->waitUpdateVarsList[i];
        if ( pHmiVar->area == mewConnect->combinVar.area && mewConnect->combinVar.overFlow == pHmiVar->overFlow && ( pHmiVar->overFlow == false ) )
        {                            //没有越界才合并，否则不合并
            int startOffset = qMin( mewConnect->combinVar.addOffset, pHmiVar->addOffset );
            int endOffset = 0;
            int newLen = 0;
            if ( pHmiVar->area >= MEWTOCOL_AREA_DT &&
                            pHmiVar->area <= MEWTOCOL_AREA_WL )
            {
                int combineCount = 0;
                int varCount = 0;

                combineCount = ( mewConnect->combinVar.len + 1 ) / 2;

                if ( pHmiVar->dataType != Com_Bool )
                    varCount = ( pHmiVar->len + 1 ) / 2;
                else
                {
                    Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
                    varCount = ( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16;
                }

                endOffset = qMax( mewConnect->combinVar.addOffset + combineCount, pHmiVar->addOffset + varCount );
                newLen = 2 * ( endOffset - startOffset );

                if ( newLen <= M_MEWTOCOL_MAX_WORD * 2 )                   //最多组合 MAX_HLINK_PACKET_LEN 字节
                {
                    if ( newLen <= 2*( combineCount + varCount ) + 16 ) //能节省10字节 两个变量之间的距离超过10/2就不合并
                    {
                        mewConnect->combinVar.addOffset = startOffset;
                        mewConnect->combinVar.bitOffset = -1;
                        mewConnect->combinVar.len = newLen;
                        mewConnect->combinVarsList.append( pHmiVar );
                    }
                }
                else
                    continue;
            }
            else
                continue;
        }
    }
}

bool MewtocolService::hasProcessingReq( struct MewtocolConnect* mewConnect )
{
    return ( mewConnect->writingCnt > 0 || mewConnect->requestVarsList.count() > 0 );
}

bool MewtocolService::initCommDev()
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
        struct MewtocolConnect* conn = m_connectList[0];

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

            m_rwSocket = new RWSocket(MEWTOCOL, &uartCfg, conn->cfg.id);
            connect(m_rwSocket, SIGNAL(readyRead()),this,SLOT(slot_readData()));
            m_rwSocket->openDev();
        }
    }
    return true;
}

bool MewtocolService::processRspData( struct MewtocolMsgPar msgPar ,struct ComMsg comMsg )
{
//#ifdef Q_WS_QWS
    bool result = true;

            if ( m_devUseCnt > 0 )
                m_devUseCnt--;

            for ( int i = 0; i < m_connectList.count(); i++ )
            {
                struct MewtocolConnect* mewConnect = m_connectList[i];
                Q_ASSERT( mewConnect );
                if ( mewConnect->cfg.cpuAddr == msgPar.slave ) //属于这个连接
                {
                    //qDebug()<<"ID =  "<<msgPar.varId<<"    rw = "<<msgPar.rw;
                    if ( mewConnect->writingCnt > 0 && mewConnect->waitWriteVarsList.count() > 0 && msgPar.rw == 1 )
                    {                                     // 处理写请求
                        //qDebug()<<"writingCnt  "<<mewConnect->writingCnt<<"    waitWriteVarsList    "<<mewConnect->waitWriteVarsList.count();
                        Q_ASSERT( msgPar.rw == 1 );
                        mewConnect->writingCnt = 0;
                        //qDebug() << "Write err:" << msgPar.err;
                        struct HmiVar* pHmiVar = mewConnect->waitWriteVarsList.takeFirst(); //删除写请求记录
                        m_mutexRw.lock();
                        //pHmiVar->err = msgPar.err;
                        pHmiVar->wStatus = msgPar.err;
                        m_mutexRw.unlock();
                        comMsg.varId = ( pHmiVar->varId & 0x0000ffff );
                        comMsg.type = 1;
                        comMsg.err = msgPar.err;
                        m_comMsgTmp.insertMulti( comMsg.varId, comMsg );
                    }
                    else if ( mewConnect->requestVarsList.count() > 0 && msgPar.rw == 0 )
                    {
                        Q_ASSERT( msgPar.rw == 0 );
                        Q_ASSERT( msgPar.area == mewConnect->requestVar.area );
                        //qDebug() << "Read err:" << msgPar.err;
                        if ( msgPar.err == VAR_PARAM_ERROR && mewConnect->requestVarsList.count() > 1 )
                        {
                            for ( int j = 0; j < mewConnect->requestVarsList.count(); j++ )
                            {
                                struct HmiVar* pHmiVar = mewConnect->requestVarsList[j];
                                pHmiVar->overFlow = true;
                                pHmiVar->lastUpdateTime = -1; //合并后越界的不算越界，立即再次更新（更新时不合并）
                            }
                        }
                        else
                        {
                            for ( int j = 0; j < mewConnect->requestVarsList.count(); j++ )
                            {
                                m_mutexRw.lock();
                                struct HmiVar* pHmiVar = mewConnect->requestVarsList[j];

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
                                    if ( msgPar.area >= MEWTOCOL_AREA_DT && msgPar.area <= MEWTOCOL_AREA_T_BIT )
                                    {
                                        Q_ASSERT( msgPar.count <= M_MEWTOCOL_MAX_WORD );
                                        int offset = pHmiVar->addOffset - mewConnect->requestVar.addOffset;
                                        Q_ASSERT( offset >= 0 );
                                        Q_ASSERT( offset <= M_MEWTOCOL_MAX_WORD );

                                        if ( pHmiVar->dataType == Com_Bool )
                                        {
                                            int len = 2 * (( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16 );
                                            if ( pHmiVar->rData.size() < len )
                                                pHmiVar->rData = QByteArray( len, 0 ); //初始化为0
                                            //pHmiVar->rData.resize(len);
                                            if ( 2*offset + len <= M_MEWTOCOL_MAX_WORD*2 ) //保证不越界
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

                                            if ( 2*offset + len <= M_MEWTOCOL_MAX_WORD*2 ) //保证不越界
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
                                else if ( msgPar.err == VAR_COMM_TIMEOUT )
                                {

                                    if ( pHmiVar->varId == 65535 )
                                    {
                                        foreach( HmiVar* tempHmiVar, mewConnect->actVarsList )
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
                        mewConnect->requestVarsList.clear();
                    }
                    if ( msgPar.err == VAR_COMM_TIMEOUT )
                        mewConnect->connStatus = CONNECT_NO_LINK;
                    else
                        mewConnect->connStatus = CONNECT_LINKED_OK;
                    break;
                }
    }
    return result;
}



void MewtocolService::slot_run()
{
    initCommDev();
    m_runTimer->start(10);
}

void MewtocolService::slot_readData()
{
    struct MewtocolMsgPar msgPar;
    struct ComMsg comMsg;
    int ret;
    do
    {
        //ret =m_modbus->read( (char *)&msgPar, sizeof( struct MbusMsgPar ) ); //接受来自Driver的消息
        ret = m_rwSocket->readDev((char *)&msgPar);
        switch (msgPar.err)
        {
        case COM_OPEN_ERR:
            qDebug()<<"COM_OPEN_ERR";
            m_devUseCnt = 0;
            for ( int i = 0; i < m_connectList.count(); i++ )
            {
                struct MewtocolConnect* mewConnect = m_connectList[i];
                Q_ASSERT( mewConnect );
                mewConnect->waitWriteVarsList.clear();
                mewConnect->requestVarsList.clear();
                foreach( HmiVar* tempHmiVar, mewConnect->actVarsList )
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
    while ( ret == sizeof( struct MewtocolMsgPar ) );

}

void MewtocolService::slot_runTimerTimeout()
{
    m_runTimer->stop();
    int lastUpdateTime = -1;
        int connectCount = m_connectList.count();
        m_currTime = getCurrTime();
        int currTime = m_currTime;

        for ( int i = 0; i < connectCount; i++ )
        {                              //处理写请求
            struct MewtocolConnect* mewConnect = m_connectList[i];
            updateWriteReq( mewConnect );
            trySndWriteReq( mewConnect );
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
    {
        struct MewtocolConnect* mewConnect = m_connectList[i];
        if ( needUpdateFlag )
        {
            updateOnceRealReadReq( mewConnect );  //先重组一次更新变量
            updateReadReq( mewConnect ); //无论如何都要重组请求变量
        }
        if ( !( hasProcessingReq( mewConnect ) ) )
        {
            combineReadReq( mewConnect );
        }
        trySndReadReq( mewConnect );
    }
    m_runTimer->start(20);
}

