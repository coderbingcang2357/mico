
#include "miprofxService.h"



MiProfxService::MiProfxService( struct MiProfxConnectCfg& connectCfg )
{
    m_connect.cfg = connectCfg;
    m_connect.writingCnt = 0;
    m_connect.connStatus = CONNECT_NO_OPERATION;
    m_connect.connectTestVar.lastUpdateTime = -1;
    m_currTime = getCurrTime();
    m_runTimer = NULL;
    m_rwSocket = NULL;
}

MiProfxService::~MiProfxService()
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

bool MiProfxService::meetServiceCfg( int serviceType, int COM_interface )
{
    if ( serviceType == SERVICE_TYPE_MIPROFX )
    {
        if ( m_connect.cfg.COM_interface == COM_interface )
            return true;
    }

    return false;
}

int MiProfxService::getConnLinkStatus( int connectId )
{
    if ( connectId == m_connect.cfg.id )
        return m_connect.connStatus;

    return -1;
}

int MiProfxService::getComMsg( QList<struct ComMsg> &msgList )
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
int MiProfxService::splitLongVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );

    if ( pHmiVar->connectId == m_connect.cfg.id )
    {
        pHmiVar->lastUpdateTime = -1; //立即更新

        if ( pHmiVar->dataType == Com_Bool || pHmiVar->area == MIPROFX_AREA_M
                        || pHmiVar->area == MIPROFX_AREA_X || pHmiVar->area == MIPROFX_AREA_Y )
        {
            if ( pHmiVar->len > M_MIPROFX_MAX*8 && pHmiVar->splitVars.count() <= 0 )  //拆分
            {
                for ( int splitedLen = 0; splitedLen < pHmiVar->len; )
                {
                    int len = pHmiVar->len - splitedLen;
                    if ( len > M_MIPROFX_MAX*8 )
                        len = M_MIPROFX_MAX * 8;

                    struct HmiVar* pSplitHhiVar = new HmiVar;
                    pSplitHhiVar->varId = pHmiVar->varId + (( pHmiVar->splitVars.count() + 1 ) << 16 );
                    pSplitHhiVar->connectId = pHmiVar->connectId;
                    pSplitHhiVar->cycleTime = pHmiVar->cycleTime;
                    pSplitHhiVar->area = pHmiVar->area;
                    pSplitHhiVar->addOffset = pHmiVar->addOffset + ( pHmiVar->bitOffset + splitedLen ) / 8;
                    pSplitHhiVar->bitOffset = ( pHmiVar->bitOffset + splitedLen ) % 8;
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
            if ( pHmiVar->len > M_MIPROFX_MAX && pHmiVar->splitVars.count() <= 0 )  //拆分
            {
                for ( int splitedLen = 0; splitedLen < pHmiVar->len; )
                {
                    int len = pHmiVar->len - splitedLen;
                    if ( len > M_MIPROFX_MAX )
                        len = M_MIPROFX_MAX;

                    struct HmiVar* pSplitHhiVar = new HmiVar;
                    pSplitHhiVar->varId = pHmiVar->varId + (( pHmiVar->splitVars.count() + 1 ) << 16 );
                    pSplitHhiVar->connectId = pHmiVar->connectId;
                    pSplitHhiVar->cycleTime = pHmiVar->cycleTime;
                    pSplitHhiVar->area = pHmiVar->area;
                    pSplitHhiVar->addOffset = pHmiVar->addOffset + splitedLen ;
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
    return -1;
}

int MiProfxService::onceRealUpdateVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    m_mutexOnceReal.lock();
    if ( pHmiVar->connectId == m_connect.cfg.id )
    {
        pHmiVar->lastUpdateTime = -1; //立即更新
        if ( pHmiVar->splitVars.count() > 0 )
        {
            for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
            {
                struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                pSplitHhiVar->lastUpdateTime = -1;
                pSplitHhiVar->overFlow = false;
                if ( !m_connect.onceRealVarsList.contains( pSplitHhiVar ) )
                    m_connect.onceRealVarsList.append( pSplitHhiVar );
            }
        }
        else
        {
            pHmiVar->lastUpdateTime = -1;
            pHmiVar->overFlow = false;
            if ( !m_connect.onceRealVarsList.contains( pHmiVar ) )
                m_connect.onceRealVarsList.append( pHmiVar );
        }
        m_mutexOnceReal.unlock();
        return 0;
    }
    m_mutexOnceReal.unlock();
    return -1;
}
int MiProfxService::startUpdateVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    m_mutexUpdateVar.lock();
    if ( pHmiVar->connectId == m_connect.cfg.id )
    {
        pHmiVar->lastUpdateTime = -1; //立即更新
        if ( pHmiVar->splitVars.count() > 0 )
        {
            for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
            {
                struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                pSplitHhiVar->lastUpdateTime = -1;
                pSplitHhiVar->overFlow = false;
                if ( !m_connect.actVarsList.contains( pSplitHhiVar ) )
                    m_connect.actVarsList.append( pSplitHhiVar );
            }
        }
        else
        {
            pHmiVar->lastUpdateTime = -1;
            pHmiVar->overFlow = false;
            if ( !m_connect.actVarsList.contains( pHmiVar ) )
                m_connect.actVarsList.append( pHmiVar );
        }
        //qDebug() << "startUpdateVar：" << pHmiVar->varId;
        m_mutexUpdateVar.unlock();
        return 0;
    }

    m_mutexUpdateVar.unlock();
    return -1;
}

int MiProfxService::stopUpdateVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    m_mutexUpdateVar.lock();

    if ( pHmiVar->connectId == m_connect.cfg.id )
    {
        if ( pHmiVar->splitVars.count() > 0 )
        {
            for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
            {
                struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                m_connect.actVarsList.removeAll( pSplitHhiVar );
            }
        }
        else
            m_connect.actVarsList.removeAll( pHmiVar );
        m_mutexUpdateVar.unlock();
        return 0;
    }

    m_mutexUpdateVar.unlock();
    return -1;
}

int MiProfxService::readVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    if ( pHmiVar->err == VAR_NO_ERR )  //无错误才拷贝数据
    {
        if ( pHmiVar->dataType == Com_Bool || pHmiVar->area == MIPROFX_AREA_M
                        || pHmiVar->area == MIPROFX_AREA_X || pHmiVar->area == MIPROFX_AREA_Y )
        {                          // 位变量长度为位的个数
            if ( pHmiVar->area == MIPROFX_AREA_D || pHmiVar->area == MIPROFX_AREA_T || pHmiVar->area == MIPROFX_AREA_C )
                Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );//为了兼容汇川D区不能单字节读添加
            else
                Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 7 );

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
        else
        {
            if ( bufLen >= pHmiVar->len && pHmiVar->rData.size() >= pHmiVar->len )
            {
                for ( int j = 0; j < pHmiVar->len; j++ )
                {                  //奇偶字节调换
                    if ( pHmiVar->dataType == Com_String )
                    {
                        if ( j & 0x1 )
                            pBuf[j] = pHmiVar->rData.at( j - 1 );
                        else if ( j + 1 < pHmiVar->rData.size() )
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

    return pHmiVar->err;
}

int MiProfxService::readDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    Q_ASSERT( pHmiVar && pBuf );

    if ( pHmiVar->connectId == m_connect.cfg.id )
    {
        int ret;
        if ( pHmiVar->wStatus == VAR_NO_OPERATION )
            ret = VAR_NO_OPERATION;
        else
        {
            m_mutexRw.lock();
            if ( pHmiVar->splitVars.count() > 0 )
            {
                for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
                {
                    struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                    int len = 0;
                    if ( pHmiVar->dataType == Com_Bool || pHmiVar->area == MIPROFX_AREA_M
                                    || pHmiVar->area == MIPROFX_AREA_X || pHmiVar->area == MIPROFX_AREA_Y )
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
        }
        return ret;
    }

    return -1;
}

int MiProfxService::writeVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    int ret;

    Q_ASSERT( pHmiVar && pBuf );
    m_mutexRw.lock();

    if ( pHmiVar->dataType == Com_Bool || pHmiVar->area == MIPROFX_AREA_M
                    || pHmiVar->area == MIPROFX_AREA_X || pHmiVar->area == MIPROFX_AREA_Y )
    {                        //位变量长度为位的个数
        int count;
        if ( pHmiVar->area == MIPROFX_AREA_D || pHmiVar->area == MIPROFX_AREA_T || pHmiVar->area == MIPROFX_AREA_C )
        {
            Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
            count = (( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16 ) * 2;   //为了兼容汇川D区不能单字节读添加
        }
        else
        {
            Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 7 );
            count = ( pHmiVar->bitOffset + pHmiVar->len + 7 ) / 8;  //字节数
        }
        Q_ASSERT( count > 0 && count <= M_MIPROFX_MAX );
        if ( count > M_MIPROFX_MAX )
            count = M_MIPROFX_MAX;
        int dataBytes = count;
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
        int count = (( pHmiVar->len + 1 ) / 2 ) * 2;  //字节数
        Q_ASSERT( count > 0 && count <= M_MIPROFX_MAX );
        if ( count > M_MIPROFX_MAX )
            count = M_MIPROFX_MAX;
        int dataBytes = count;
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
                        pHmiVar->rData[ j - 1 ] = pHmiVar->wData.at( j );
                    else if ( j + 1 < pHmiVar->rData.size() )
                        pHmiVar->rData[ j + 1 ] = pHmiVar->wData.at( j );
                }
                else
                    pHmiVar->rData[j] = pHmiVar->wData.at( j );
            }
            else
                break;
        }
    }

    pHmiVar->wStatus = VAR_NO_OPERATION;
    ret = pHmiVar->err;
    m_mutexRw.unlock();

    return ret; //是否要返回成功
}

int MiProfxService::writeDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    Q_ASSERT( pHmiVar && pBuf );

    if ( pHmiVar->connectId == m_connect.cfg.id )
    {
        int ret = 0;
        if ( m_connect.connStatus == CONNECT_NO_LINK )//if this var has read and write err, do not execute write, until it is read or wriet correct.
            ret = VAR_PARAM_ERROR;
        else
        {
            m_mutexWReq.lock();          //注意2个互斥锁顺序,防止死锁
            if ( pHmiVar->splitVars.count() > 0 )
            {
                for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
                {
                    struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                    int len = 0;
                    if ( pHmiVar->dataType == Com_Bool || pHmiVar->area == MIPROFX_AREA_M
                                    || pHmiVar->area == MIPROFX_AREA_X || pHmiVar->area == MIPROFX_AREA_Y )
                        len = ( pSplitHhiVar->len + 7 ) / 8;
                    else
                        len = pSplitHhiVar->len;

                    ret = writeVar( pSplitHhiVar, pBuf, len );
                    m_connect.waitWrtieVarsList.append( pSplitHhiVar );
                    pBuf += len;
                    bufLen -= len;
                }
            }
            else
            {
                ret = writeVar( pHmiVar, pBuf, bufLen );
                m_connect.waitWrtieVarsList.append( pHmiVar );
            }
            m_mutexWReq.unlock();
        }

        return ret; //是否要返回成功
    }
    return -1;
}

int MiProfxService::sndWriteVarMsgToDriver( struct HmiVar* pHmiVar )
{
    int sendedLen = 0;
    Q_ASSERT( pHmiVar );
//#ifdef Q_WS_QWS
    struct MiprofxMsgPar msgPar;

    m_mutexRw.lock();
    msgPar.rw = 1;   //写
    msgPar.area = pHmiVar->area;
    msgPar.addr = pHmiVar->addOffset;
    msgPar.err = 0;

    if ( pHmiVar->dataType == Com_Bool || pHmiVar->area == MIPROFX_AREA_M
                    || pHmiVar->area == MIPROFX_AREA_X || pHmiVar->area == MIPROFX_AREA_Y )
    {                              // 位变量长度为位的个数
        int count;
        if ( pHmiVar->area == MIPROFX_AREA_D || pHmiVar->area == MIPROFX_AREA_T || pHmiVar->area == MIPROFX_AREA_C )
        {
            Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
            count = (( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16 ) * 2;   //为兼容汇川添加
        }
        else
        {
            Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 7 );
            count = ( pHmiVar->bitOffset + pHmiVar->len + 7 ) / 8;  //字数
        }

        Q_ASSERT( count > 0 && count <= M_MIPROFX_MAX );
        if ( count > M_MIPROFX_MAX )
            count = M_MIPROFX_MAX;
        msgPar.count = count;      // 占用的总字节数
        int dataBytes = count;
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
        if ( pHmiVar->len == 1 && ( pHmiVar->area == MIPROFX_AREA_M ||
                                    pHmiVar->area == MIPROFX_AREA_X || pHmiVar->area == MIPROFX_AREA_Y ) )
        {
            msgPar.rw = 2; //强制写位
            msgPar.addr = pHmiVar->addOffset * 8 + pHmiVar->bitOffset;
            memcpy( msgPar.data, pHmiVar->wData.data(), dataBytes );
        }
        else
            memcpy( msgPar.data, pHmiVar->rData.data(), dataBytes );
        sendedLen = count;
    }
    else if ( msgPar.area == MIPROFX_AREA_D || msgPar.area == MIPROFX_AREA_T
                    || msgPar.area == MIPROFX_AREA_C || msgPar.area == MIPROFX_AREA_C200 )
    {
        int count = (( pHmiVar->len + 1 ) / 2 ) * 2;  //字节数
        Q_ASSERT( count > 0 && count <= M_MIPROFX_MAX );
        if ( count > M_MIPROFX_MAX )
            count = M_MIPROFX_MAX;
        msgPar.count = count;      // 占用的总字节数
        int dataBytes = count;
        if ( pHmiVar->rData.size() < dataBytes )
            pHmiVar->rData.resize( dataBytes );

        for ( int j = 0; j < pHmiVar->len; j++ )
        {                          // 奇偶字节调换
            if ( j < pHmiVar->wData.size() && j < pHmiVar->rData.size() )
            {
                if ( pHmiVar->dataType == Com_String )
                {
                    if ( j & 0x1 )
                        pHmiVar->rData[ j - 1 ] = pHmiVar->wData.at( j );
                    else if ( j + 1 < pHmiVar->rData.size() )
                        pHmiVar->rData[ j + 1 ] = pHmiVar->wData.at( j );
                }
                else
                    pHmiVar->rData[j] = pHmiVar->wData.at( j );
            }
            else
                break;
        }
        memcpy( msgPar.data, pHmiVar->rData.data(), dataBytes );
        sendedLen = count;
    }
    else
    {
        Q_ASSERT( false );
        msgPar.count = 0;
    }

    if ( sendedLen > 0 )
    {
        m_devUseCnt++;
        m_mutexRw.unlock();
        //int len = write( m_fd, ( char* ) & msgPar, sizeof( struct MiprofxMsgPar ) );
        int len = m_rwSocket->writeDev(( char* ) & msgPar, sizeof( struct MiprofxMsgPar ));
        Q_ASSERT( len > 0 );
    }
    else
        m_mutexRw.unlock();
//#endif
    return sendedLen;
}

int MiProfxService::sndReadVarMsgToDriver( struct HmiVar* pHmiVar )
{
    int sendedLen = 0;
    Q_ASSERT( pHmiVar );
//#ifdef Q_WS_QWS
    struct MiprofxMsgPar msgPar;

    m_mutexRw.lock();
    msgPar.rw = 0;    // 读
    msgPar.area = pHmiVar->area;
    msgPar.addr = pHmiVar->addOffset;
    msgPar.err = 0;


    if ( pHmiVar->dataType == Com_Bool || (( pHmiVar->area == MIPROFX_AREA_M
                                           || pHmiVar->area == MIPROFX_AREA_X || pHmiVar->area == MIPROFX_AREA_Y ) && pHmiVar->dataType != Com_String ) )
    {                              // 位变量长度为位的个数
        int count;
        if ( pHmiVar->area == MIPROFX_AREA_D || pHmiVar->area == MIPROFX_AREA_T || pHmiVar->area == MIPROFX_AREA_C )
        {
            Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
            count = (( pHmiVar->bitOffset + pHmiVar->len + 7 ) / 8 ) * 2;   //字节数
        }
        else
        {
            Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 7 );
            count = ( pHmiVar->bitOffset + pHmiVar->len + 7 ) / 8;  //字节数
        }

        Q_ASSERT( count > 0 && count <= M_MIPROFX_MAX );
        if ( count > M_MIPROFX_MAX )
            count = M_MIPROFX_MAX;
        msgPar.count = count;      // 占用的总字数
        sendedLen = count;
    }
    else if ( msgPar.area <= MIPROTOCOL4_AREA_C200 )
    {
        int count = 0;
        if ( pHmiVar->dataType == Com_Bool || pHmiVar->area == MIPROFX_AREA_M ||
                        pHmiVar->area == MIPROFX_AREA_X || pHmiVar->area == MIPROFX_AREA_Y )
            count = pHmiVar->len;
        else
            count = (( pHmiVar->len + 1 ) ) / 2 * 2;   //字数
        Q_ASSERT( count > 0 && count <= M_MIPROFX_MAX );
        if ( count > M_MIPROFX_MAX )
            count = M_MIPROFX_MAX;
        msgPar.count = count;      // 占用的总字数
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
        //int len = write( m_fd, ( char* ) & msgPar, sizeof( struct MiprofxMsgPar ) );
        int len = m_rwSocket->writeDev( ( char* ) & msgPar, sizeof( struct MiprofxMsgPar ));
        Q_ASSERT( len > 0 );
    }
    else
        m_mutexRw.unlock();
//#endif
    return sendedLen;
}

bool isWaitingReadRsp( const struct MiprofxConnect &connect, struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    if ( connect.waitUpdateVarsList.contains( pHmiVar ) )
        return true;
    else if ( connect.combinVarsList.contains( pHmiVar ) )
        return true;
    else if ( connect.requestVarsList.contains( pHmiVar ) )
        return true;
    else
        return false;
}

bool MiProfxService::trySndWriteReq( )
{
    if ( m_devUseCnt < 1 )                          //没有正在读写, snd read, 一次只能发送一包请求
    {
        if ( m_connect.writingCnt == 0 && m_connect.requestVarsList.count() == 0 ) //这个连接没有读写
        {
            m_mutexWReq.lock();
            if ( m_connect.waitWrtieVarsList.count() > 0 )
            {
                struct HmiVar* pHmiVar = m_connect.waitWrtieVarsList[0];
                if ( pHmiVar->len == 1 && ( pHmiVar->area == MIPROFX_AREA_M ||
                                            pHmiVar->area == MIPROFX_AREA_X || pHmiVar->area == MIPROFX_AREA_Y ) &&
                                !isWaitingReadRsp( m_connect, pHmiVar ) ) // M X Y写单个位直接写
                {
                    if ( sndWriteVarMsgToDriver( pHmiVar ) > 0 )
                    {
                        pHmiVar->lastUpdateTime = m_currTime;//getCurrTime();
                        m_connect.writingCnt = 1;    //进入写状态
                        m_mutexWReq.unlock();
                        return true;
                    }
                    else
                    {
                        m_connect.waitWrtieVarsList.takeAt( 0 );//出列,丢掉写请求
                        pHmiVar->err = VAR_PARAM_ERROR;
                    }
                }
                else if (( pHmiVar->lastUpdateTime == -1 )   //位变量需要先读
                                && ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_String
                                     || pHmiVar->area == MIPROFX_AREA_M || pHmiVar->area == MIPROFX_AREA_X
                                     || pHmiVar->area == MIPROFX_AREA_Y ) )
                {
                    pHmiVar->lastUpdateTime = m_currTime;//getCurrTime();
                    if ( !isWaitingReadRsp( m_connect, pHmiVar ) )
                        m_connect.waitUpdateVarsList.prepend( pHmiVar ); //加入到读队列最前,保证下次最先读
                }
                else if ( !isWaitingReadRsp( m_connect, pHmiVar ) ) //没有正在等待读
                {                                   //直接写,snd write cmd to driver
                    if ( sndWriteVarMsgToDriver( pHmiVar ) > 0 )
                    {
                        pHmiVar->lastUpdateTime = m_currTime;//getCurrTime();
                        m_connect.writingCnt = 1;    //进入写状态
                        m_mutexWReq.unlock();
                        return true;
                    }
                    else
                    {
                        m_connect.waitWrtieVarsList.takeAt( 0 );//出列,丢掉写请求
                        pHmiVar->err = VAR_PARAM_ERROR;
                    }
                }
            }
            m_mutexWReq.unlock();
        }
    }
    return false;
}

bool MiProfxService::trySndReadReq( )
{
    if ( m_devUseCnt < 1 )             //没有正在读写, snd read, 一次只能发送一包请求
    {
        if ( m_connect.writingCnt == 0 && m_connect.requestVarsList.count() == 0 ) //这个连接没有读写
        {
            if ( m_connect.combinVarsList.count() > 0 )
            {
                m_connect.requestVarsList = m_connect.combinVarsList;
                m_connect.requestVar = m_connect.combinVar;
                for ( int i = 0; i < m_connect.combinVarsList.count(); i++ )
                {                      //要把已经请求的变量从等待队列中删除
                    struct HmiVar* pHmiVar = m_connect.combinVarsList[i];
                    m_connect.waitUpdateVarsList.removeAll( pHmiVar );
                    pHmiVar->lastUpdateTime = m_currTime; //这个时候才是变量上次刷新时间
                }
                m_connect.combinVarsList.clear();

                if ( sndReadVarMsgToDriver( &( m_connect.requestVar ) ) > 0 )
                {
                    return true;
                }
                else
                {
                    for ( int i = 0; i < m_connect.requestVarsList.count(); i++ )
                    {
                        struct HmiVar* pHmiVar = m_connect.requestVarsList[i];
                        pHmiVar->err = VAR_PARAM_ERROR;
                    }
                    m_connect.requestVarsList.clear();
                }
            }
        }
    }
    return false;
}

bool MiProfxService::updateWriteReq( )
{
    bool newUpdateVarFlag = false;
    QList<struct HmiVar*> tmpList;
    m_mutexWReq.lock();
    //int currTime = m_currTime;//getCurrTime();
    // 找出需要刷新的变量
    for ( int i = 0; i < m_connect.wrtieVarsList.count(); i++ )
    {
        struct HmiVar* pHmiVar = m_connect.wrtieVarsList[i];
        if ( !m_connect.waitWrtieVarsList.contains( pHmiVar ) )
        {
            /*if ( m_connect.waitUpdateVarsList.contains( pHmiVar ) )
                m_connect.waitUpdateVarsList.removeAll( pHmiVar );

            if ( m_connect.requestVarsList.contains( pHmiVar )  )
                m_connect.requestVarsList.removeAll( pHmiVar );*/

            m_connect.waitWrtieVarsList.append( pHmiVar );
            newUpdateVarFlag = true;
        }
        else
        {
            tmpList.append(pHmiVar);
        }
    }

    m_connect.wrtieVarsList.clear();
    m_connect.wrtieVarsList += tmpList;

    m_mutexWReq.unlock();

    return newUpdateVarFlag;
}
bool MiProfxService::updateReadReq( )
{
    bool newUpdateVarFlag = false;
    m_mutexUpdateVar.lock();
    int currTime = m_currTime;//getCurrTime();
    if ( m_connect.connStatus == CONNECT_LINKED_OK )
    {                         // 找出需要刷新的变量
        for ( int i = 0; i < m_connect.actVarsList.count(); i++ )
        {
            struct HmiVar* pHmiVar = m_connect.actVarsList[i];
            if (( pHmiVar->lastUpdateTime == -1 ) || ( calIntervalTime( pHmiVar->lastUpdateTime, currTime ) >= pHmiVar->cycleTime ) )
            {                              // 需要刷新
                if (( !m_connect.waitUpdateVarsList.contains( pHmiVar ) )
                                && ( !m_connect.waitWrtieVarsList.contains( pHmiVar ) )
                                // && ( !m_connect.combinVarsList.contains( pHmiVar ) )
                                && ( !m_connect.requestVarsList.contains( pHmiVar ) ) )
                {
                    m_connect.waitUpdateVarsList.append( pHmiVar );
                    newUpdateVarFlag = true;
                }
            }
        }
    }
    else
    {
        m_connect.waitUpdateVarsList.clear();
        if ( m_connect.actVarsList.count() > 0 && m_connect.requestVarsList.count() == 0 )
        {//qDebug()<<"connectTestVar  lastUpdateTime  = "<<m_connect.connectTestVar.lastUpdateTime;
            m_connect.actVarsList[0]->lastUpdateTime = m_connect.connectTestVar.lastUpdateTime;
            m_connect.connectTestVar = *( m_connect.actVarsList[0] );
            m_connect.connectTestVar.varId = 65535;
            m_connect.connectTestVar.cycleTime = 2000;
            if (( m_connect.connectTestVar.lastUpdateTime == -1 )
                            || ( calIntervalTime( m_connect.connectTestVar.lastUpdateTime, currTime ) >= m_connect.connectTestVar.cycleTime ) )
            {
                if (( !m_connect.waitUpdateVarsList.contains( &( m_connect.connectTestVar ) ) )
                                && ( !m_connect.waitWrtieVarsList.contains( &( m_connect.connectTestVar ) ) )
                                && ( !m_connect.requestVarsList.contains( &( m_connect.connectTestVar ) ) ) )
                {
                    m_connect.waitUpdateVarsList.append( &( m_connect.connectTestVar ) );
                    newUpdateVarFlag = true;
                }
            }
        }

    }
    m_mutexUpdateVar.unlock();

    //清除正在写的变量
    /*m_mutexWReq.lock();
    for ( int i = 0; i < m_connect.waitWrtieVarsList.count(); i++ )
    {                                  // 找出需要刷新的变量
        struct HmiVar* pHmiVar = m_connect.waitWrtieVarsList[i];

        if ( m_connect.waitUpdateVarsList.contains( pHmiVar ) )
            m_connect.waitUpdateVarsList.removeAll( pHmiVar );

        if ( m_connect.requestVarsList.contains( pHmiVar )  )
            m_connect.requestVarsList.removeAll( pHmiVar );
    }
    m_mutexWReq.unlock();*/
    return newUpdateVarFlag;
}

bool MiProfxService::updateOnceRealReadReq( )
{
    bool newUpdateVarFlag = false;
    m_mutexOnceReal.lock();
    //int currTime = m_currTime;//getCurrTime();
    for ( int i = 0; i < m_connect.onceRealVarsList.count(); i++ )
    {                                  // 找出需要刷新的变量
        struct HmiVar* pHmiVar = m_connect.onceRealVarsList[i];
        if (( !m_connect.waitUpdateVarsList.contains( pHmiVar ) )
                        //&& ( !m_connect.waitWrtieVarsList.contains( pHmiVar ) ) 写的时候还是要去读该变量
                        //&& ( !m_connect.combinVarsList.contains( pHmiVar ) )    有可能被清除所以还是要加入等待读队列
                        && ( !m_connect.requestVarsList.contains( pHmiVar ) ) )
        {
            m_connect.waitUpdateVarsList.prepend( pHmiVar );  //插入队列最前面堆栈操作后进先读
            newUpdateVarFlag = true;
        }
        else
        {
            if ( m_connect.waitUpdateVarsList.contains( pHmiVar ) )
            {
                //提前他
                m_connect.waitUpdateVarsList.removeAll( pHmiVar );
                m_connect.waitUpdateVarsList.prepend( pHmiVar );
            }
        }
    }
    m_connect.onceRealVarsList.clear();// 清楚一次读列表
    m_mutexOnceReal.unlock();

    return newUpdateVarFlag;
}
void MiProfxService::combineReadReq( )
{
    m_connect.combinVarsList.clear();
    if ( m_connect.waitUpdateVarsList.count() >= 1 )
    {
        struct HmiVar* pHmiVar = m_connect.waitUpdateVarsList[0];
        m_connect.combinVarsList.append( pHmiVar );
        m_connect.combinVar = *pHmiVar;
        if ( pHmiVar->dataType == Com_Bool || pHmiVar->area == MIPROFX_AREA_M
                        || pHmiVar->area == MIPROFX_AREA_X || pHmiVar->area == MIPROFX_AREA_Y )
        {
            if ( pHmiVar->area == MIPROFX_AREA_D || pHmiVar->area == MIPROFX_AREA_T || pHmiVar->area == MIPROFX_AREA_C )
            {
                Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
                m_connect.combinVar.len = (( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16 ) * 2;
            }//为兼容汇川添加
            else
            {
                Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 7 );
                m_connect.combinVar.len = ( pHmiVar->bitOffset + pHmiVar->len + 7 ) / 8 ;
            }

            m_connect.combinVar.dataType = Com_String;
            m_connect.combinVar.bitOffset = -1;
        }
    }

    for ( int i = 1; i < m_connect.waitUpdateVarsList.count(); i++ )
    {
        struct HmiVar* pHmiVar = m_connect.waitUpdateVarsList[i];

        if ( pHmiVar->area == m_connect.combinVar.area && m_connect.combinVar.overFlow == pHmiVar->overFlow && ( pHmiVar->overFlow == false ) )
        {                            //没有越界才合并，否则不合并
            int startOffset = qMin( m_connect.combinVar.addOffset, pHmiVar->addOffset );
            int endOffset = 0;
            int newLen = 0;

            int combineCount = m_connect.combinVar.len;
            int varCount = 0;

            if ( pHmiVar->dataType == Com_Bool || pHmiVar->area == MIPROFX_AREA_M
                            || pHmiVar->area == MIPROFX_AREA_X || pHmiVar->area == MIPROFX_AREA_Y )
            {
                if ( pHmiVar->area == MIPROFX_AREA_D || pHmiVar->area == MIPROFX_AREA_T || pHmiVar->area == MIPROFX_AREA_C )
                {
                    Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
                    varCount = (( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16 ) * 2;
                }
                else
                {
                    Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 7 );
                    varCount = ( pHmiVar->bitOffset + pHmiVar->len + 7 ) / 8;
                }
            }
            else
                varCount = (( pHmiVar->len + 1 ) / 2 ) * 2;

            endOffset = qMax( m_connect.combinVar.addOffset + combineCount, pHmiVar->addOffset + varCount );
            newLen = ( endOffset - startOffset );
            if ( newLen <= M_MIPROFX_MAX )  //最多组合 MAX_MBUS_PACKET_LEN 字节
            {
                if ( newLen <= combineCount + varCount + 10 ) //能节省10字节
                {
                    m_connect.combinVar.addOffset = startOffset;
                    m_connect.combinVar.len = newLen;
                    m_connect.combinVarsList.append( pHmiVar );
                }
            }
            else
                continue;
        }
    }
}

bool MiProfxService::hasProcessingReq( )
{
    return ( m_connect.writingCnt > 0 || m_connect.requestVarsList.count() > 0 );
}

bool MiProfxService::initCommDev()
{
    m_devUseCnt = 0;

    if (m_runTimer == NULL)
    {
        m_runTimer = new QTimer;
        connect( m_runTimer, SIGNAL( timeout() ), this,
            SLOT( slot_runTimerTimeout() )/*,Qt::QueuedConnection*/ );
    }

#ifdef MINTERFACE_2
    if ( m_connect.cfg.COM_interface > 1 )
    {
        qDebug() << "no such COM_interface";
        return false;
    }
#endif

    if ( m_rwSocket == NULL )
    {
        struct UartCfg uartCfg;
        uartCfg.COMNum = m_connect.cfg.COM_interface;
        uartCfg.baud = m_connect.cfg.baud;
        uartCfg.dataBit = m_connect.cfg.dataBit;
        uartCfg.stopBit = m_connect.cfg.stopBit;
        uartCfg.parity = m_connect.cfg.parity;

        m_rwSocket = new RWSocket(MIPROFX, &uartCfg, m_connect.cfg.id);
        connect(m_rwSocket, SIGNAL(readyRead()),this,SLOT(slot_readData()));
        m_rwSocket->openDev();
    }
    return true;
}

bool MiProfxService::processRspData( struct MiprofxMsgPar msgPar ,struct ComMsg comMsg )
{
//#ifdef Q_WS_QWS

    bool result = true;


    if ( m_devUseCnt > 0 )
        m_devUseCnt--;
    if ( m_connect.writingCnt > 0 && m_connect.waitWrtieVarsList.count() > 0 && ( msgPar.rw == 1 || msgPar.rw == 2 ) )
    {                                     // 处理写请求
        Q_ASSERT( msgPar.rw == 1 || msgPar.rw == 2 );
        m_connect.writingCnt = 0;
        //qDebug() << "Write err:" << msgPar.err;
        struct HmiVar* pHmiVar = m_connect.waitWrtieVarsList.takeFirst(); //删除写请求记录
        m_mutexRw.lock();
        //pHmiVar->err = msgPar.err;
        pHmiVar->wStatus = msgPar.err;
        m_mutexRw.unlock();
        comMsg.varId = ( pHmiVar->varId & 0x0000ffff ); //防止长变量
        comMsg.type = 1; // 1是write;
        comMsg.err = msgPar.err;
        m_comMsgTmp.insertMulti( comMsg.varId, comMsg );
    }
    else if ( m_connect.requestVarsList.count() > 0 )
    {
        Q_ASSERT( msgPar.rw == 0 );
        Q_ASSERT( msgPar.area == m_connect.requestVar.area );
        //qDebug() << "Read err:" << msgPar.err;
        if ( msgPar.err == VAR_PARAM_ERROR && m_connect.requestVarsList.count() > 1 )
        {
            for ( int j = 0; j < m_connect.requestVarsList.count(); j++ )
            {
                struct HmiVar* pHmiVar = m_connect.requestVarsList[j];
                pHmiVar->overFlow = true;
                pHmiVar->lastUpdateTime = -1; //合并后越界的不算越界，立即再次更新（更新时不合并）
            }
        }
        else
        {
            for ( int j = 0; j < m_connect.requestVarsList.count(); j++ )
            {
                m_mutexRw.lock();
                struct HmiVar* pHmiVar = m_connect.requestVarsList[j];
                Q_ASSERT( msgPar.area == pHmiVar->area );
                pHmiVar->err = msgPar.err;
                if ( msgPar.err == VAR_PARAM_ERROR )
                {
                    pHmiVar->overFlow = true;
                    comMsg.varId = ( pHmiVar->varId & 0x0000ffff ); //防止长变量
                    comMsg.type = 0; //0是read;
                    comMsg.err = VAR_PARAM_ERROR;
                    m_comMsgTmp.insert( comMsg.varId, comMsg );
                }
                else if ( msgPar.err == VAR_NO_ERR )
                {
                    pHmiVar->overFlow = false;
                    Q_ASSERT( msgPar.count <= M_MIPROFX_MAX );
                    int offset = pHmiVar->addOffset - m_connect.requestVar.addOffset;
                    Q_ASSERT( offset >= 0 );
                    Q_ASSERT( offset <= M_MIPROFX_MAX );
                    if ( pHmiVar->dataType == Com_Bool || pHmiVar->area == MIPROFX_AREA_M
                                    || pHmiVar->area == MIPROFX_AREA_X || pHmiVar->area == MIPROFX_AREA_Y )
                    {
                        int len;
                        if ( pHmiVar->area == MIPROFX_AREA_D || pHmiVar->area == MIPROFX_AREA_T || pHmiVar->area == MIPROFX_AREA_C )
                        {
                            len = ((( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16 ) * 2 );
                        }
                        else
                        {
                            len = (( pHmiVar->bitOffset + pHmiVar->len + 7 ) / 8 );
                        }

                        if ( pHmiVar->rData.size() < len )
                            pHmiVar->rData = QByteArray( len, 0 ); //初始化为0

                        if ( offset + len <= M_MIPROFX_MAX ) //保证不越界
                        {
                            memcpy( pHmiVar->rData.data(), &msgPar.data[offset], len );
                            if ( pHmiVar->wStatus != VAR_NO_OPERATION )
                            {
                                comMsg.varId = ( pHmiVar->varId & 0x0000ffff ); //防止长变量
                                comMsg.type = 0; //0是read;
                                comMsg.err = VAR_NO_ERR;
                                m_comMsgTmp.insert( comMsg.varId, comMsg );
                            }
                        }
                        else
                            Q_ASSERT( false );
                    }
                    else
                    {
                        int len = (( pHmiVar->len + 1 ) / 2 ) * 2;
                        if ( pHmiVar->rData.size() < len )
                            pHmiVar->rData = QByteArray( len, 0 ); //初始化为0

                        if ( offset + len <= M_MIPROFX_MAX ) //保证不越界
                        {
                            memcpy( pHmiVar->rData.data(), &msgPar.data[offset], len );
                            if ( pHmiVar->wStatus != VAR_NO_OPERATION )
                            {
                                comMsg.varId = ( pHmiVar->varId & 0x0000ffff ); //防止长变量
                                comMsg.type = 0; //0是read;
                                comMsg.err = VAR_NO_ERR;
                                m_comMsgTmp.insert( comMsg.varId, comMsg );
                            }
                        }
                        else
                            Q_ASSERT( false );
                    }
                }
                else if ( msgPar.err == VAR_COMM_TIMEOUT )
                {

                    if ( pHmiVar->varId == 65535 )
                    {
                        foreach( HmiVar* tempHmiVar, m_connect.actVarsList )
                        {
                            comMsg.type = 0; //0是read;
                            comMsg.err = VAR_DISCONNECT;
                            tempHmiVar->err = VAR_COMM_TIMEOUT;
                            tempHmiVar->lastUpdateTime = -1;
                            comMsg.varId = ( tempHmiVar->varId & 0x0000ffff );
                            m_comMsgTmp.insert( comMsg.varId, comMsg );
                        }
                    }
                    else
                    {
                        comMsg.varId = ( pHmiVar->varId & 0x0000ffff ); //防止长变量
                        comMsg.type = 0; //0是read;
                        comMsg.err = VAR_COMM_TIMEOUT;
                        m_comMsgTmp.insert( comMsg.varId, comMsg );
                    }
                }
                m_mutexRw.unlock();
            }
        }
        m_connect.requestVarsList.clear();
    }
    if ( msgPar.err == VAR_COMM_TIMEOUT )
        m_connect.connStatus = CONNECT_NO_LINK;
    else
        m_connect.connStatus = CONNECT_LINKED_OK;

    return result;
}


void MiProfxService::slot_run()
{
    initCommDev();
    m_runTimer->start(10);
}
void MiProfxService::slot_readData()
{
    struct MiprofxMsgPar msgPar;
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

            //Q_ASSERT( m_connect );
            m_connect.waitWrtieVarsList.clear();
            m_connect.requestVarsList.clear();
            foreach( HmiVar* tempHmiVar, m_connect.actVarsList )
            {
                comMsg.type = 0; //0是read;
                comMsg.err = VAR_DISCONNECT;
                tempHmiVar->err = VAR_COMM_TIMEOUT;
                tempHmiVar->lastUpdateTime = -1;
                comMsg.varId = ( tempHmiVar->varId & 0x0000ffff );
                m_comMsgTmp.insert( comMsg.varId, comMsg );
            }

            break;
        default:
            processRspData( msgPar, comMsg );
            break;
        }
    }
    while ( ret == sizeof( struct MiprofxMsgPar ) );

}
void MiProfxService::slot_runTimerTimeout()
{
    m_runTimer->stop();
    int lastUpdateTime = -1;
        m_currTime = getCurrTime();
        int currTime = m_currTime;
        updateWriteReq();
        trySndWriteReq();


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

        bool hasProcessingReqFlag = false; // 处理读请求
        if ( needUpdateFlag )
        {
            updateOnceRealReadReq( ); //先重组一次更新变量
            updateReadReq( ); //无论如何都要重组请求变量
        }
        if ( !( hasProcessingReq( ) ) )
        {
            combineReadReq( );
        }
        trySndReadReq( );

    }
    m_runTimer->start(20);
}

