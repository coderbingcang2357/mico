#include "miprotocol4Service.h"


#define MAX_MBUS_PACKET_LEN  160  //最大一包160字节，其实能达到240

MiProtocol4Service::MiProtocol4Service( struct MiProtocol4ConnectCfg& connectCfg )
{
    struct Miprocotol4Connect* mip4Connect = new Miprocotol4Connect;
    mip4Connect->cfg = connectCfg;
    mip4Connect->writingCnt = 0;
    mip4Connect->connStatus = CONNECT_NO_OPERATION;
    mip4Connect->connectTestVar.lastUpdateTime = -1;
    if ( connectCfg.PlcNo == M_MIPROTOCOL4_TYPE_ONE )
    {
        mip4Connect->maxBR = M_MIPROTOCOL4_ONE_BR_MAX;
        mip4Connect->maxBW = M_MIPROTOCOL4_ONE_BW_MAX;
        mip4Connect->maxWR = M_MIPROTOCOL4_ONE_WR_MAX;
        mip4Connect->maxWW = M_MIPROTOCOL4_ONE_WW_MAX;
    }

    if ( connectCfg.PlcNo == M_MIPROTOCOL4_TYPE_TWO )
    {
        mip4Connect->maxBR = M_MIPROTOCOL4_TWO_BR_MAX;
        mip4Connect->maxBW = M_MIPROTOCOL4_TWO_BW_MAX;
        mip4Connect->maxWR = M_MIPROTOCOL4_TWO_WR_MAX;
        mip4Connect->maxWW = M_MIPROTOCOL4_TWO_WW_MAX;
    }

    m_connectList.append( mip4Connect );
    m_currTime = getCurrTime();
    m_runTimer = NULL;
    m_rwSocket = NULL;
}

MiProtocol4Service::~MiProtocol4Service()
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

bool MiProtocol4Service::meetServiceCfg( int serviceType, int COM_interface )
{
    if ( serviceType == SERVICE_TYPE_MIPROTOCOL4 )
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

int MiProtocol4Service::getConnLinkStatus( int connectId )
{
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct Miprocotol4Connect* mip4Connect = m_connectList[i];
        if ( connectId == mip4Connect->cfg.id )
            return mip4Connect->connStatus;
    }
    return -1;
}

int MiProtocol4Service::getComMsg( QList<struct ComMsg> &msgList )
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
int MiProtocol4Service::splitLongVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );

    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct Miprocotol4Connect* mip4Connect = m_connectList[i];
        if ( pHmiVar->connectId == mip4Connect->cfg.id )
        {
            pHmiVar->lastUpdateTime = -1; //立即更新
            if ( pHmiVar->area == MIPROTOCOL4_AREA_D || pHmiVar->area == MIPROTOCOL4_AREA_T )
            {
                if ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup )
                {
                    if ( pHmiVar->len > ( mip4Connect->maxWR )*16 && pHmiVar->splitVars.count() <= 0 )  //拆分
                    {
                        for ( int splitedLen = 0; splitedLen < pHmiVar->len; )
                        {
                            int len = pHmiVar->len - splitedLen;
                            if ( len > ( mip4Connect->maxWR )*16 )
                                len = ( mip4Connect->maxWR ) * 16;

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
                    if ( pHmiVar->len > ( mip4Connect->maxWR )*2 && pHmiVar->splitVars.count() <= 0 )  //拆分
                    {
                        for ( int splitedLen = 0; splitedLen < pHmiVar->len; )
                        {
                            int len = pHmiVar->len - splitedLen;
                            if ( len > ( mip4Connect->maxWR )*2 )
                                len = ( mip4Connect->maxWR ) * 2;

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
            else if (( pHmiVar->area != MIPROTOCOL4_AREA_C ) && ( pHmiVar->area != MIPROTOCOL4_AREA_C200 ) )
            {
                if ( pHmiVar->len > mip4Connect->maxBR && pHmiVar->splitVars.count() <= 0 )  //拆分
                {
                    for ( int splitedLen = 0; splitedLen < pHmiVar->len; )
                    {
                        int len = pHmiVar->len - splitedLen;
                        if ( len > mip4Connect->maxBR )
                            len = mip4Connect->maxBR;

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
int MiProtocol4Service::onceRealUpdateVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    m_mutexOnceReal.lock();
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct Miprocotol4Connect* mip4Connect = m_connectList[i];
        if ( pHmiVar->connectId == mip4Connect->cfg.id )
        {
            pHmiVar->lastUpdateTime = -1;
            if ( pHmiVar->splitVars.count() > 0 )
            {
                for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
                {
                    struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                    pSplitHhiVar->lastUpdateTime = -1;
                    pSplitHhiVar->overFlow = false;
                    if ( !mip4Connect->onceRealVarsList.contains( pSplitHhiVar ) )
                        mip4Connect->onceRealVarsList.append( pSplitHhiVar );
                }
            }
            else
            {
                pHmiVar->lastUpdateTime = -1;
                pHmiVar->overFlow = false;
                if ( !mip4Connect->onceRealVarsList.contains( pHmiVar ) )
                    mip4Connect->onceRealVarsList.append( pHmiVar );
            }
            //qDebug() << "startUpdateVar:" << pHmiVar->varId;

            m_mutexOnceReal.unlock();
            return 0;
        }
    }
    m_mutexOnceReal.unlock();
    return -1;
}

int MiProtocol4Service::startUpdateVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    m_mutexUpdateVar.lock();
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct Miprocotol4Connect* mip4Connect = m_connectList[i];
        if ( pHmiVar->connectId == mip4Connect->cfg.id )
        {
            pHmiVar->lastUpdateTime = -1;
            if ( pHmiVar->splitVars.count() > 0 )
            {
                for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
                {
                    struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                    pSplitHhiVar->lastUpdateTime = -1;
                    pSplitHhiVar->overFlow = false;
                    if ( !mip4Connect->actVarsList.contains( pSplitHhiVar ) )
                        mip4Connect->actVarsList.append( pSplitHhiVar );
                }
            }
            else
            {
                pHmiVar->lastUpdateTime = -1;
                pHmiVar->overFlow = false;
                if ( pHmiVar->addOffset >= 0 )
                {
                    if ( !mip4Connect->actVarsList.contains( pHmiVar ) )
                        mip4Connect->actVarsList.append( pHmiVar );
                }
                else
                    pHmiVar->err = VAR_PARAM_ERROR;
            }
            m_mutexUpdateVar.unlock();
            return 0;
        }
    }
    m_mutexUpdateVar.unlock();
    return -1;
}

int MiProtocol4Service::stopUpdateVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    m_mutexUpdateVar.lock();
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct Miprocotol4Connect* mip4Connect = m_connectList[i];
        if ( pHmiVar->connectId == mip4Connect->cfg.id )
        {
            if ( pHmiVar->splitVars.count() > 0 )
            {
                for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
                {
                    struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                    mip4Connect->actVarsList.removeAll( pSplitHhiVar );
                }
            }
            else
                mip4Connect->actVarsList.removeAll( pHmiVar );
            m_mutexUpdateVar.unlock();
            return 0;
        }
    }
    m_mutexUpdateVar.unlock();
    return -1;
}

int MiProtocol4Service::readVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    if ( pHmiVar->err == VAR_NO_ERR )  //无错误才拷贝数据
    {
        if ( pHmiVar->area == MIPROTOCOL4_AREA_C200 )
        {
            if ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup )
            {                          // 位变量长度为位的个数
                Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 31 );
                int dataBytes = ( pHmiVar->len + 7 ) / 8;
                if ( bufLen >= dataBytes && pHmiVar->rData.size() >= dataBytes )
                {
                    for ( int j = 0; j < pHmiVar->len; j++ )
                    {
                        int byteOffset = ( pHmiVar->bitOffset + j ) / 8;
                        int bitOffset = 7 - ( pHmiVar->bitOffset + j ) % 8;
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
                if ( bufLen >= pHmiVar->len && pHmiVar->rData.size() >= pHmiVar->len )
                {
                    for ( int j = 0; j < pHmiVar->len; j++ )
                    {
                        if ( pHmiVar->dataType != Com_PointArea )           //奇偶字节调换
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
        else if ( pHmiVar->area == MIPROTOCOL4_AREA_D || pHmiVar->area == MIPROTOCOL4_AREA_T || pHmiVar->area == MIPROTOCOL4_AREA_C )
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
                        int bitOffset = 7 - ( pHmiVar->bitOffset + j ) % 8;
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
                    {
                        if ( pHmiVar->dataType != Com_PointArea ) //奇偶字节调换
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
            //qDebug() << pHmiVar->area << pHmiVar->addOffset << "buff[0]" << (int)pBuf[0];
        }
    }
    return pHmiVar->err;
}

int MiProtocol4Service::readDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    Q_ASSERT( pHmiVar && pBuf );
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct Miprocotol4Connect* mip4Connect = m_connectList[i];
        if ( pHmiVar->connectId == mip4Connect->cfg.id )
        {
            int ret;
            m_mutexRw.lock();
            if ( pHmiVar->splitVars.count() > 0 )
            {
                for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
                {
                    struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                    int len = 0;
                    if ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitBlock4 || pHmiVar->dataType == Com_BitBlock8 ||
                                    pHmiVar->dataType == Com_BitBlock12 || pHmiVar->dataType == Com_BitBlock16 || pHmiVar->dataType == Com_BitBlock20 ||
                                    pHmiVar->dataType == Com_BitBlock24 || pHmiVar->dataType == Com_BitBlock28 || pHmiVar->dataType == Com_BitBlock32 )
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

int MiProtocol4Service::writeVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    int ret;

    Q_ASSERT( pHmiVar && pBuf );
    m_mutexRw.lock();
    if ( pHmiVar->area == MIPROTOCOL4_AREA_C200 )
    {//大小数等到发送写命令时再处理
        if ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup )
        {                        //位变量长度为位的个数
            Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 31 );
            int count = ( pHmiVar->bitOffset + pHmiVar->len + 31 ) / 32;  //双字数
            Q_ASSERT( count > 0 && count <= 29 );
            if ( count > 29 )
                count = 29;
            int dataBytes = 4 * count;
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
                int bitR = 7 - ( pHmiVar->bitOffset + i ) % 8;
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
            int count = ( pHmiVar->len + 3 ) / 4; //双字数
            Q_ASSERT( count > 0 && count <= 29 );
            if ( count > 29 )
                count = 29;
            int dataBytes = 4 * count;
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
                    if ( pHmiVar->dataType != Com_PointArea )
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
    else if ( pHmiVar->area == MIPROTOCOL4_AREA_D || pHmiVar->area == MIPROTOCOL4_AREA_T || pHmiVar->area == MIPROTOCOL4_AREA_C )
    {                            //大小数等到发送写命令时再处理
        if ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup )
        {                        //位变量长度为位的个数
            Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
            int count = ( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16;  //字数
            Q_ASSERT( count > 0 && count <= 58 );
            if ( count > 58 )
                count = 58;
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
                int bitR = 7 - ( pHmiVar->bitOffset + i ) % 8;
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
            Q_ASSERT( count > 0 && count <= 58 );
            if ( count > 58 )
                count = 58;
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
                    if ( pHmiVar->dataType != Com_PointArea )
                    {
                        if ( j & 0x1 )
                            pHmiVar->rData[j - 1] = pHmiVar->wData.at( j );
                        else if ( j + 1 < pHmiVar->wData.size() )
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
    else
    {
        Q_ASSERT( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitBlock4 || pHmiVar->dataType == Com_BitBlock8 ||
                  pHmiVar->dataType == Com_BitBlock12 || pHmiVar->dataType == Com_BitBlock16 || pHmiVar->dataType == Com_BitBlock20 ||
                  pHmiVar->dataType == Com_BitBlock24 || pHmiVar->dataType == Com_BitBlock28 || pHmiVar->dataType == Com_BitBlock32 );
        int count = pHmiVar->len;
        Q_ASSERT( count > 0 &&  count <= 120 );
        if ( count > 120 )
            count = 120;
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
    pHmiVar->wStatus = VAR_NO_OPERATION;
    ret = pHmiVar->err;
    m_mutexRw.unlock();

    return ret; //是否要返回成功
}

int MiProtocol4Service::writeDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    Q_ASSERT( pHmiVar && pBuf );

    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct Miprocotol4Connect* mip4Connect = m_connectList[i];
        if ( pHmiVar->connectId == mip4Connect->cfg.id )
        {
            int ret = 0;

            if ( mip4Connect->connStatus == CONNECT_NO_LINK )//if this var has read and write err, do not execute write, until it is read or wriet correct.
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
                        if ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitBlock4 || pHmiVar->dataType == Com_BitBlock8 ||
                                        pHmiVar->dataType == Com_BitBlock12 || pHmiVar->dataType == Com_BitBlock16 || pHmiVar->dataType == Com_BitBlock20 ||
                                        pHmiVar->dataType == Com_BitBlock24 || pHmiVar->dataType == Com_BitBlock28 || pHmiVar->dataType == Com_BitBlock32 )
                            len = ( pSplitHhiVar->len + 7 ) / 8;
                        else
                            len = pSplitHhiVar->len;

                        ret = writeVar( pSplitHhiVar, pBuf, len );
                        mip4Connect->waitWrtieVarsList.append( pSplitHhiVar );
                        pBuf += len;
                        bufLen -= len;
                    }
                }
                else
                {
                    ret = writeVar( pHmiVar, pBuf, bufLen );
                    mip4Connect->waitWrtieVarsList.append( pHmiVar );
                }
                m_mutexWReq.unlock();
            }
            return ret; //是否要返回成功
        }
    }
    return -1;
}

int MiProtocol4Service::addConnect( struct MiProtocol4ConnectCfg& connectCfg )
{                                      //要检查是否重复 add
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct Miprocotol4Connect* mip4Connect = m_connectList[i];
        Q_ASSERT( mip4Connect->cfg.COM_interface == connectCfg.COM_interface );//绝对要是同一个设备接口
        if ( connectCfg.id == mip4Connect->cfg.id )
        {
            Q_ASSERT( false );
            return 0;
        }
    }
    struct Miprocotol4Connect* mip4Connect = new Miprocotol4Connect;
    mip4Connect->cfg = connectCfg;
    mip4Connect->writingCnt = 0;
    mip4Connect->connStatus = CONNECT_NO_OPERATION;
    mip4Connect->connectTestVar.lastUpdateTime = -1;

    if ( connectCfg.PlcNo == M_MIPROTOCOL4_TYPE_ONE )
    {
        mip4Connect->maxBR = M_MIPROTOCOL4_ONE_BR_MAX;
        mip4Connect->maxBW = M_MIPROTOCOL4_ONE_BW_MAX;
        mip4Connect->maxWR = M_MIPROTOCOL4_ONE_WR_MAX;
        mip4Connect->maxWW = M_MIPROTOCOL4_ONE_WW_MAX;
    }

    if ( connectCfg.PlcNo == M_MIPROTOCOL4_TYPE_TWO )
    {
        mip4Connect->maxBR = M_MIPROTOCOL4_TWO_BR_MAX;
        mip4Connect->maxBW = M_MIPROTOCOL4_TWO_BW_MAX;
        mip4Connect->maxWR = M_MIPROTOCOL4_TWO_WR_MAX;
        mip4Connect->maxWW = M_MIPROTOCOL4_TWO_WW_MAX;
    }

    m_connectList.append( mip4Connect );
    return 0;
}

int MiProtocol4Service::sndWriteVarMsgToDriver( struct Miprocotol4Connect* mip4Connect, struct HmiVar* pHmiVar )
{
    int sendedLen = 0;
    Q_ASSERT( mip4Connect && pHmiVar );

    struct MiProtocol4MsgPar msgPar;

    m_mutexRw.lock();
    msgPar.rw = 1;   //写
    msgPar.slave    = mip4Connect->cfg.cpuAddr;
    msgPar.area     = pHmiVar->area;
    msgPar.addr     = pHmiVar->addOffset;
    msgPar.PlcNo    = mip4Connect->cfg.PlcNo;
    msgPar.checkSum = mip4Connect->cfg.checkSum;
    msgPar.err = 0;

    if ( pHmiVar->area == MIPROTOCOL4_AREA_C200 )
    {
        if ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup )
        {                              // 位变量长度为位的个数
            Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 31 );
            int count = ( pHmiVar->bitOffset + pHmiVar->len + 31 ) / 32;  //双字数
            Q_ASSERT( count > 0 && count*2 <= mip4Connect->maxWR );
            if ( count*2 > mip4Connect->maxWR )
                count = ( mip4Connect->maxWR / 2 );
            msgPar.count = count;      // 占用的总双字数,但是到驱动要转换成字数
            int dataBytes = 4 * count;
            if ( pHmiVar->rData.size() < dataBytes )
                pHmiVar->rData.resize( dataBytes );

            for ( int i = 0; i < pHmiVar->len; i++ )
            {
                int byteW = i / 8;
                int bitW = i % 8;
                int byteR = ( pHmiVar->bitOffset + i ) / 8;
                int bitR = 7 - ( pHmiVar->bitOffset + i ) % 8;

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
            sendedLen = count * 32;
        }
        else
        {
            int count = ( pHmiVar->len + 3 ) / 4; //双字数
            Q_ASSERT( count > 0 && count*2 <= mip4Connect->maxWR );
            if ( count*2 > mip4Connect->maxWR )
                count = ( mip4Connect->maxWR / 2 );
            msgPar.count = count;      // 占用的总双字数
            int dataBytes = 4 * count;
            if ( pHmiVar->rData.size() < dataBytes )
                pHmiVar->rData.resize( dataBytes );

            for ( int j = 0; j < pHmiVar->len; j++ )
            {                          // 奇偶字节调换
                if ( j < pHmiVar->wData.size() && j < pHmiVar->rData.size() )
                {
                    if ( pHmiVar->dataType != Com_PointArea )
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
            sendedLen = count * 4;
        }
    }
    else if ( msgPar.area == MIPROTOCOL4_AREA_D || msgPar.area == MIPROTOCOL4_AREA_T || pHmiVar->area == MIPROTOCOL4_AREA_C )
    {
        if ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup )
        {                              // 位变量长度为位的个数
            Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
            int count = ( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16;  //字数
            Q_ASSERT( count > 0 && count <= mip4Connect->maxWR );
            if ( count > mip4Connect->maxWR )
                count = mip4Connect->maxWR;
            msgPar.count = count;      // 占用的总字数
            int dataBytes = 2 * count;
            if ( pHmiVar->rData.size() < dataBytes )
                pHmiVar->rData.resize( dataBytes );

            for ( int i = 0; i < pHmiVar->len; i++ )
            {
                int byteW = i / 8;
                int bitW = i % 8;
                int byteR = ( pHmiVar->bitOffset + i ) / 8;
                int bitR = 7 - ( pHmiVar->bitOffset + i ) % 8;

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
            Q_ASSERT( count > 0 && count <= mip4Connect->maxWR );
            if ( count > mip4Connect->maxWR )
                count = mip4Connect->maxWR;
            msgPar.count = count;      // 占用的总字数
            int dataBytes = 2 * count;
            if ( pHmiVar->rData.size() < dataBytes )
                pHmiVar->rData.resize( dataBytes );
            for ( int j = 0; j < pHmiVar->len; j++ )
            {                       // 奇偶字节调换
                if ( j < pHmiVar->wData.size() && j < pHmiVar->rData.size() )
                {
                    if ( pHmiVar->dataType != Com_PointArea )
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
    else if ( msgPar.area == MIPROTOCOL4_AREA_M || msgPar.area == MIPROTOCOL4_AREA_X || msgPar.area == MIPROTOCOL4_AREA_Y || msgPar.area == MIPROTOCOL4_AREA_S )
    {                                  // 位变量长度为位的个数
        Q_ASSERT( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitBlock4 || pHmiVar->dataType == Com_BitBlock8 ||
                  pHmiVar->dataType == Com_BitBlock12 || pHmiVar->dataType == Com_BitBlock16 || pHmiVar->dataType == Com_BitBlock20 ||
                  pHmiVar->dataType == Com_BitBlock24 || pHmiVar->dataType == Com_BitBlock28 || pHmiVar->dataType == Com_BitBlock32 );
        int count = pHmiVar->len;
        Q_ASSERT( count > 0 &&  count <= mip4Connect->maxBR );
        if ( count > mip4Connect->maxBR )
            count = mip4Connect->maxBR;
        msgPar.count = count;
        int dataBytes = ( count + 7 ) / 8;
        if ( pHmiVar->rData.size() < dataBytes )
            pHmiVar->rData.resize( dataBytes );

        memcpy( pHmiVar->rData.data(), pHmiVar->wData.data(), qMin( dataBytes, pHmiVar->wData.size() ) );
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
        //int len = write( m_fd, ( char* ) & msgPar, sizeof( struct MiProtocol4MsgPar ) );
        int len = m_rwSocket->writeDev(( char* ) & msgPar, sizeof( struct MiProtocol4MsgPar ));

        Q_ASSERT( len > 0 );
    }
    else
        m_mutexRw.unlock();

    return sendedLen;
}

int MiProtocol4Service::sndReadVarMsgToDriver( struct Miprocotol4Connect* mip4Connect, struct HmiVar* pHmiVar )
{
    int sendedLen = 0;
    Q_ASSERT( mip4Connect && pHmiVar );

    struct MiProtocol4MsgPar msgPar;

    m_mutexRw.lock();
    msgPar.rw = 0;    // 读
    msgPar.slave = mip4Connect->cfg.cpuAddr;
    msgPar.area = pHmiVar->area;
    msgPar.addr = pHmiVar->addOffset;
    msgPar.PlcNo    = mip4Connect->cfg.PlcNo;
    msgPar.checkSum = mip4Connect->cfg.checkSum;
    msgPar.err = 0;
    if ( pHmiVar->area == MIPROTOCOL4_AREA_C200 )
    {
        if ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup )
        {                              // 位变量长度为位的个数
            Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 31 );
            int count = ( pHmiVar->bitOffset + pHmiVar->len + 31 ) / 32;  //字数
            Q_ASSERT( count > 0 && count*2 <= mip4Connect->maxWR );
            if ( count*2 > mip4Connect->maxWR )
                count = mip4Connect->maxWR / 2;
            msgPar.count = count;      // 占用的总双字数到底层驱动要转换成字数
            sendedLen = count * 32;
        }
        else
        {
            int count = ( pHmiVar->len + 3 ) / 4;  //字数
            Q_ASSERT( count > 0 && count*2 <= mip4Connect->maxWR );
            if ( count*2 > mip4Connect->maxWR )
                count = mip4Connect->maxWR / 2;
            msgPar.count = count;      // 占用的总双字数到底层驱动要转换成字数
            sendedLen = count * 4;
        }
    }
    else if ( msgPar.area == MIPROTOCOL4_AREA_D || msgPar.area == MIPROTOCOL4_AREA_T || pHmiVar->area == MIPROTOCOL4_AREA_C )
    {
        if ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup )
        {                              // 位变量长度为位的个数
            Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
            int count = ( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16;  //字数
            Q_ASSERT( count > 0 && count <= mip4Connect->maxWR );
            if ( count > mip4Connect->maxWR )
                count = mip4Connect->maxWR;
            msgPar.count = count;      // 占用的总字数
            sendedLen = count * 16;
        }
        else
        {
            int count = ( pHmiVar->len + 1 ) / 2;  //字数
            Q_ASSERT( count > 0 && count <= mip4Connect->maxWR );
            if ( count > mip4Connect->maxWR )
                count = mip4Connect->maxWR;
            msgPar.count = count;      // 占用的总字数
            sendedLen = count * 4;
        }
    }
    else if ( msgPar.area == MIPROTOCOL4_AREA_M || msgPar.area == MIPROTOCOL4_AREA_X || msgPar.area == MIPROTOCOL4_AREA_Y || msgPar.area == MIPROTOCOL4_AREA_S )
    {                                  // 位变量长度为位的个数
        Q_ASSERT( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitBlock4 || pHmiVar->dataType == Com_BitBlock8 ||
                  pHmiVar->dataType == Com_BitBlock12 || pHmiVar->dataType == Com_BitBlock16 || pHmiVar->dataType == Com_BitBlock20 ||
                  pHmiVar->dataType == Com_BitBlock24 || pHmiVar->dataType == Com_BitBlock28 || pHmiVar->dataType == Com_BitBlock32 );
        int count = pHmiVar->len;
        Q_ASSERT( count > 0 &&  count <= mip4Connect->maxBR );
        if ( count > mip4Connect->maxBR )
            count = mip4Connect->maxBR;
        msgPar.count = count;
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
        //int len = write( m_fd, ( char* ) & msgPar, sizeof( struct MiProtocol4MsgPar ) );
        int len = m_rwSocket->writeDev( ( char* ) & msgPar, sizeof( struct MiProtocol4MsgPar ));

        Q_ASSERT( len > 0 );
    }
    else
        m_mutexRw.unlock();

    return sendedLen;
}

bool isWaitingReadRsp( struct Miprocotol4Connect* mip4Connect, struct HmiVar* pHmiVar )
{
    Q_ASSERT( mip4Connect && pHmiVar );
    if ( mip4Connect->waitUpdateVarsList.contains( pHmiVar ) )
        return true;
    else if ( mip4Connect->combinVarsList.contains( pHmiVar ) )
        return true;
    else if ( mip4Connect->requestVarsList.contains( pHmiVar ) )
        return true;
    else
        return false;
}

bool MiProtocol4Service::trySndWriteReq( struct Miprocotol4Connect* mip4Connect ) //不处理超长变量
{
    if ( m_devUseCnt < MAXDEVCNT )                          //没有正在读写, snd read, 一次只能发送一包请求
    {
        if ( mip4Connect->writingCnt == 0 && mip4Connect->requestVarsList.count() == 0 ) //这个连接没有读写
        {
            m_mutexWReq.lock();
            if ( mip4Connect->waitWrtieVarsList.count() > 0 )
            {
                struct HmiVar* pHmiVar = mip4Connect->waitWrtieVarsList[0];

                if (( pHmiVar->lastUpdateTime == -1 )  //3X,4X 位变量需要先读
                                && ( pHmiVar->area == MIPROTOCOL4_AREA_D || pHmiVar->area == MIPROTOCOL4_AREA_T || pHmiVar->area == MIPROTOCOL4_AREA_C || pHmiVar->area == MIPROTOCOL4_AREA_C200 )
                                && ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup || pHmiVar->dataType == Com_String ) )
                {
                    //pHmiVar->lastUpdateTime = m_currTime;//getCurrTime();
                    if ( !isWaitingReadRsp( mip4Connect, pHmiVar ) )
                        mip4Connect->waitUpdateVarsList.prepend( pHmiVar ); //加入到读队列最前,保证下次最先读
                }
                else if ( !isWaitingReadRsp( mip4Connect, pHmiVar ) ) //没有正在等待读
                {                                   //直接写,snd write cmd to driver
                    if ( sndWriteVarMsgToDriver( mip4Connect, pHmiVar ) > 0 )
                    {
                        //pHmiVar->lastUpdateTime = m_currTime;//getCurrTime();
                        mip4Connect->writingCnt = 1;    //进入写状态
                        m_mutexWReq.unlock();
                        return true;
                    }
                    else
                    {
                        mip4Connect->waitWrtieVarsList.takeAt( 0 );//出列,丢掉写请求
                        pHmiVar->err = VAR_PARAM_ERROR;
                    }
                }
            }
            m_mutexWReq.unlock();
        }
    }
    return false;
}

bool MiProtocol4Service::trySndReadReq( struct Miprocotol4Connect* mip4Connect ) //不处理超长变量
{
    if ( m_devUseCnt < MAXDEVCNT )             //没有正在读写, snd read, 一次只能发送一包请求
    {
        if ( mip4Connect->writingCnt == 0 && mip4Connect->requestVarsList.count() == 0 ) //这个连接没有读写
        {
            if ( mip4Connect->combinVarsList.count() > 0 )
            {
                mip4Connect->requestVarsList = mip4Connect->combinVarsList;
                mip4Connect->requestVar = mip4Connect->combinVar;
                for ( int i = 0; i < mip4Connect->combinVarsList.count(); i++ )
                {                      //要把已经请求的变量从等待队列中删除
                    struct HmiVar* pHmiVar = mip4Connect->combinVarsList[i];
                    mip4Connect->waitUpdateVarsList.removeAll( pHmiVar );
                    pHmiVar->lastUpdateTime = m_currTime;
                }
                mip4Connect->combinVarsList.clear();

                if ( sndReadVarMsgToDriver( mip4Connect, &( mip4Connect->requestVar ) ) > 0 )
                {
                    return true;
                }
                else
                {
                    for ( int i = 0; i < mip4Connect->requestVarsList.count(); i++ )
                    {
                        struct HmiVar* pHmiVar = mip4Connect->requestVarsList[i];
                        pHmiVar->err = VAR_PARAM_ERROR;
                    }
                    mip4Connect->requestVarsList.clear();
                }
            }
        }
    }
    return false;
}

bool MiProtocol4Service::updateWriteReq( struct Miprocotol4Connect* mip4Connect )
{
    bool newUpdateVarFlag = false;
    QList<struct HmiVar*> tmpList;
    m_mutexWReq.lock();
    //int currTime = m_currTime;//getCurrTime();
    for ( int i = 0; i < mip4Connect->wrtieVarsList.count(); i++ )
    {
        struct HmiVar* pHmiVar = mip4Connect->wrtieVarsList[i];


        if ( !mip4Connect->waitWrtieVarsList.contains( pHmiVar ) )
        {
            /*if ( mip4Connect->waitUpdateVarsList.contains( pHmiVar ) )
                mip4Connect->waitUpdateVarsList.removeAll( pHmiVar );

            if ( mip4Connect->requestVarsList.contains( pHmiVar )  )
                mip4Connect->requestVarsList.removeAll( pHmiVar );*/

            mip4Connect->waitWrtieVarsList.append( pHmiVar );
            newUpdateVarFlag = true;
        }
        {
            tmpList.append(pHmiVar);
        }
    }

    mip4Connect->wrtieVarsList.clear() ;
    mip4Connect->wrtieVarsList += tmpList;
    m_mutexWReq.unlock();

    return newUpdateVarFlag;
}


bool MiProtocol4Service::updateReadReq( struct Miprocotol4Connect* mip4Connect )
{
    bool newUpdateVarFlag = false;
    m_mutexUpdateVar.lock();
    int currTime = m_currTime;//getCurrTime();
    if ( mip4Connect->connStatus == CONNECT_LINKED_OK )
    {
        for ( int i = 0; i < mip4Connect->actVarsList.count(); i++ )
        {                                  // 找出需要刷新的变量
            struct HmiVar* pHmiVar = mip4Connect->actVarsList[i];
            if (( pHmiVar->lastUpdateTime == -1 ) || ( calIntervalTime( pHmiVar->lastUpdateTime, currTime ) >= pHmiVar->cycleTime ) )
            {                              // 需要刷新
                if (( !mip4Connect->waitUpdateVarsList.contains( pHmiVar ) )
                                && ( !mip4Connect->waitWrtieVarsList.contains( pHmiVar ) )
                                // && ( !mip4Connect->combinVarsList.contains( pHmiVar ) )
                                && ( !mip4Connect->requestVarsList.contains( pHmiVar ) ) )
                {
                    //pHmiVar->lastUpdateTime = currTime;
                    mip4Connect->waitUpdateVarsList.append( pHmiVar );
                    newUpdateVarFlag = true;
                }
            }
        }
    }
    else
    {
        mip4Connect->waitUpdateVarsList.clear();
        if ( mip4Connect->actVarsList.count() > 0  && mip4Connect->requestVarsList.count() == 0 )
        {//qDebug()<<"connectTestVar  lastUpdateTime  = "<<mip4Connect->connectTestVar.lastUpdateTime;
            mip4Connect->actVarsList[0]->lastUpdateTime = mip4Connect->connectTestVar.lastUpdateTime;
            mip4Connect->connectTestVar = *( mip4Connect->actVarsList[0] );
            mip4Connect->connectTestVar.varId = 65535;
            mip4Connect->connectTestVar.cycleTime = 2000;
            if (( mip4Connect->connectTestVar.lastUpdateTime == -1 )
                            || ( calIntervalTime( mip4Connect->connectTestVar.lastUpdateTime, currTime ) >= mip4Connect->connectTestVar.cycleTime ) )
            {
                if (( !mip4Connect->waitUpdateVarsList.contains( &( mip4Connect->connectTestVar ) ) )
                                && ( !mip4Connect->waitWrtieVarsList.contains( &( mip4Connect->connectTestVar ) ) )
                                && ( !mip4Connect->requestVarsList.contains( &( mip4Connect->connectTestVar ) ) ) )
                {
                    mip4Connect->waitUpdateVarsList.append( &( mip4Connect->connectTestVar ) );
                    newUpdateVarFlag = true;
                }
            }


        }
    }
    m_mutexUpdateVar.unlock();

    /*m_mutexWReq.lock();
    for ( int i = 0; i < mip4Connect->waitWrtieVarsList.count(); i++ )
    {
        struct HmiVar* pHmiVar = mip4Connect->waitWrtieVarsList[i];

        if ( mip4Connect->waitUpdateVarsList.contains( pHmiVar ) )
            mip4Connect->waitUpdateVarsList.removeAll( pHmiVar );

        if ( mip4Connect->requestVarsList.contains( pHmiVar )  )
            mip4Connect->requestVarsList.removeAll( pHmiVar );
    }
    m_mutexWReq.unlock();*/
    return newUpdateVarFlag;
}

bool MiProtocol4Service::updateOnceRealReadReq( struct Miprocotol4Connect* mip4Connect )
{
    bool newUpdateVarFlag = false;
    m_mutexOnceReal.lock();
    //int currTime = m_currTime;//getCurrTime();
    for ( int i = 0; i < mip4Connect->onceRealVarsList.count(); i++ )
    {
        struct HmiVar* pHmiVar = mip4Connect->onceRealVarsList[i];
        if (( !mip4Connect->waitUpdateVarsList.contains( pHmiVar ) )
                        && ( !mip4Connect->requestVarsList.contains( pHmiVar ) ) )
        {
            mip4Connect->waitUpdateVarsList.prepend( pHmiVar );
            newUpdateVarFlag = true;
        }
        else
        {
            if ( mip4Connect->waitUpdateVarsList.contains( pHmiVar ) )
            {
                mip4Connect->waitUpdateVarsList.removeAll( pHmiVar );
                mip4Connect->waitUpdateVarsList.prepend( pHmiVar );
            }
        }
    }
    mip4Connect->onceRealVarsList.clear();
    m_mutexOnceReal.unlock();

    return newUpdateVarFlag;
}
void MiProtocol4Service::combineReadReq( struct Miprocotol4Connect* mip4Connect )
{
    mip4Connect->combinVarsList.clear();
    if ( mip4Connect->waitUpdateVarsList.count() >= 1 )
    {
        struct HmiVar* pHmiVar = mip4Connect->waitUpdateVarsList[0];
        mip4Connect->combinVarsList.append( pHmiVar );
        mip4Connect->combinVar = *pHmiVar;
        if ( mip4Connect->combinVar.area == MIPROTOCOL4_AREA_D || mip4Connect->combinVar.area == MIPROTOCOL4_AREA_T || mip4Connect->combinVar.area == MIPROTOCOL4_AREA_C )
        {
            if ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup )
            {
                Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
                mip4Connect->combinVar.dataType = Com_Word;
                mip4Connect->combinVar.len = 2 * (( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16 ); //2*count
                mip4Connect->combinVar.bitOffset = -1;
            }
        }
        else if ( mip4Connect->combinVar.area == MIPROTOCOL4_AREA_C200 )
        {
            if ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup )
            {
                Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 31 );
                mip4Connect->combinVar.dataType = Com_DWord;
                mip4Connect->combinVar.len = 4 * (( pHmiVar->bitOffset + pHmiVar->len + 31 ) / 32 ); //4*count
                mip4Connect->combinVar.bitOffset = -1;
            }
        }
    }
    for ( int i = 1; i < mip4Connect->waitUpdateVarsList.count(); i++ )
    {
        struct HmiVar* pHmiVar = mip4Connect->waitUpdateVarsList[i];
        if ( pHmiVar->area == mip4Connect->combinVar.area && mip4Connect->combinVar.overFlow == pHmiVar->overFlow && ( pHmiVar->overFlow == false ) )
        {                            //没有越界才合并，否则不合并
            int startOffset = qMin( mip4Connect->combinVar.addOffset, pHmiVar->addOffset );
            int endOffset = 0;
            int newLen = 0;
            if ( mip4Connect->combinVar.area == MIPROTOCOL4_AREA_C200 )
            {
                int combineCount = ( mip4Connect->combinVar.len + 3 ) / 4;
                int varCount = 0;

                if ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup )
                {
                    Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 31 );
                    varCount = ( pHmiVar->bitOffset + pHmiVar->len + 31 ) / 32;
                }
                else
                    varCount = ( pHmiVar->len + 3 ) / 4;

                endOffset = qMax( mip4Connect->combinVar.addOffset + combineCount, pHmiVar->addOffset + varCount );
                newLen = 4 * ( endOffset - startOffset );
                if ( newLen <= ( mip4Connect->maxWR ) * 2 )  //最多组合 mip4Connect->maxWR 字
                {
                    if ( newLen <= 4*( combineCount + varCount ) + 12 ) //能节省12字节
                    {
                        mip4Connect->combinVar.addOffset = startOffset;
                        mip4Connect->combinVar.len = newLen;
                        mip4Connect->combinVarsList.append( pHmiVar );
                    }
                }
                else
                    continue;
            }
            else if ( mip4Connect->combinVar.area == MIPROTOCOL4_AREA_D || mip4Connect->combinVar.area == MIPROTOCOL4_AREA_T || mip4Connect->combinVar.area == MIPROTOCOL4_AREA_C )
            {
                int combineCount = ( mip4Connect->combinVar.len + 1 ) / 2;
                int varCount = 0;

                if ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup )
                {
                    Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
                    varCount = ( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16;
                }
                else
                    varCount = ( pHmiVar->len + 1 ) / 2;

                endOffset = qMax( mip4Connect->combinVar.addOffset + combineCount, pHmiVar->addOffset + varCount );
                newLen = 2 * ( endOffset - startOffset );
                if ( newLen <= ( mip4Connect->maxWR ) * 2 )  //最多组合 mip4Connect->maxWR 字
                {
                    if ( newLen <= 2*( combineCount + varCount ) + 10 ) //能节省10字节
                    {
                        mip4Connect->combinVar.addOffset = startOffset;
                        mip4Connect->combinVar.len = newLen;
                        mip4Connect->combinVarsList.append( pHmiVar );
                    }
                }
                else
                    continue;
            }
            else
            {
                endOffset = qMax( mip4Connect->combinVar.addOffset + mip4Connect->combinVar.len, pHmiVar->addOffset + pHmiVar->len );
                int newLen = endOffset - startOffset;
                if ( newLen <= mip4Connect->maxBR )
                {
                    if ( newLen <= mip4Connect->combinVar.len + pHmiVar->len + 8 ) //能节省8字节
                    {
                        mip4Connect->combinVar.addOffset = startOffset;
                        mip4Connect->combinVar.len = newLen;
                        mip4Connect->combinVarsList.append( pHmiVar );
                    }
                }
                else
                    continue;
            }
        }
    }
}

bool MiProtocol4Service::hasProcessingReq( struct Miprocotol4Connect* mip4Connect )
{
    return ( mip4Connect->writingCnt > 0 || mip4Connect->requestVarsList.count() > 0 );
}

bool MiProtocol4Service::initCommDev()
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
        struct Miprocotol4Connect* conn = m_connectList[0];

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

            m_rwSocket = new RWSocket(MIPROTOCOL4, &uartCfg, conn->cfg.id);
            connect(m_rwSocket, SIGNAL(readyRead()),this,SLOT(slot_readData()));
            m_rwSocket->openDev();
        }
    }
    return true;
}

bool MiProtocol4Service::processRspData(struct MiProtocol4MsgPar msgPar ,struct ComMsg comMsg)
{
//#ifdef Q_WS_QWS
    bool result = true;
    if ( m_devUseCnt > 0 )
        m_devUseCnt--;
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct Miprocotol4Connect* mip4Connect = m_connectList[i];
        Q_ASSERT( mip4Connect );
        if ( mip4Connect->cfg.cpuAddr == msgPar.slave ) //属于这个连接
        {
            if ( mip4Connect->writingCnt > 0 && mip4Connect->waitWrtieVarsList.count() > 0 && msgPar.rw == 1 )
            {                                     // 处理写请求
                Q_ASSERT( msgPar.rw == 1 );
                mip4Connect->writingCnt = 0;
                //qDebug() << "Write err:" << msgPar.err;
                struct HmiVar* pHmiVar = mip4Connect->waitWrtieVarsList.takeFirst(); //删除写请求记录
                m_mutexRw.lock();
                //pHmiVar->err = msgPar.err;
                pHmiVar->wStatus = msgPar.err;
                m_mutexRw.unlock();
                comMsg.varId = ( pHmiVar->varId & 0x0000ffff );
                comMsg.type = 1;
                comMsg.err = msgPar.err;
                m_comMsgTmp.insertMulti( comMsg.varId, comMsg );
            }
            else if ( mip4Connect->requestVarsList.count() > 0 && msgPar.rw == 0 )
            {
                Q_ASSERT( msgPar.rw == 0 );
                //qDebug()<<"msgPar.area = "<<msgPar.area<<"   mip4Connect->requestVar.area"<<mip4Connect->requestVar.area;
                Q_ASSERT( msgPar.area == mip4Connect->requestVar.area );
                //qDebug() << "Read err:" << msgPar.err;
                if ( msgPar.err == VAR_PARAM_ERROR && mip4Connect->requestVarsList.count() > 1 )
                {
                    for ( int j = 0; j < mip4Connect->requestVarsList.count(); j++ )
                    {
                        struct HmiVar* pHmiVar = mip4Connect->requestVarsList[j];
                        pHmiVar->overFlow = true;
                        pHmiVar->lastUpdateTime = -1; //合并后越界的不算越界，立即再次更新（更新时不合并）
                    }
                }
                else
                {
                    for ( int j = 0; j < mip4Connect->requestVarsList.count(); j++ )
                    {
                        m_mutexRw.lock();
                        struct HmiVar* pHmiVar = mip4Connect->requestVarsList[j];
                        Q_ASSERT( msgPar.area == pHmiVar->area );
                        pHmiVar->err = msgPar.err;
                        if ( msgPar.err == VAR_PARAM_ERROR )
                        {
                            pHmiVar->overFlow = true;
                            comMsg.varId = ( pHmiVar->varId & 0x0000ffff );
                            comMsg.type = 0;
                            comMsg.err = VAR_PARAM_ERROR;
                            m_comMsgTmp.insert( comMsg.varId, comMsg );

                        }
                        else if ( msgPar.err == VAR_NO_ERR )
                        {
                            pHmiVar->overFlow = false;
                            if ( msgPar.area == MIPROTOCOL4_AREA_C200 )
                            {
                                Q_ASSERT( msgPar.count <= mip4Connect->maxWR );
                                int offset = pHmiVar->addOffset - mip4Connect->requestVar.addOffset;
                                Q_ASSERT( offset >= 0 && offset*2 <= mip4Connect->maxWR );
                                if ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup )
                                {
                                    int len = 4 * (( pHmiVar->bitOffset + pHmiVar->len + 31 ) / 32 );
                                    if ( pHmiVar->rData.size() < len )
                                        pHmiVar->rData = QByteArray( len, 0 ); //初始化为0
                                    //pHmiVar->rData.resize(len);
                                    if ( 4*offset + len <= mip4Connect->maxWR * 2 )
                                    {
                                        memcpy( pHmiVar->rData.data(), &msgPar.data[4*offset], len );
                                        if ( pHmiVar->wStatus != VAR_NO_OPERATION )
                                        {
                                            comMsg.varId = ( pHmiVar->varId & 0x0000ffff );
                                            comMsg.type = 0;
                                            comMsg.err = VAR_NO_ERR;
                                            m_comMsgTmp.insert( comMsg.varId, comMsg );
                                        }
                                    }
                                    else
                                        Q_ASSERT( false );
                                }
                                else
                                {
                                    int len = 4 * (( pHmiVar->len + 3 ) / 4 );
                                    if ( pHmiVar->rData.size() < len )
                                        pHmiVar->rData = QByteArray( len, 0 ); //初始化为0
                                    //pHmiVar->rData.resize(len);
                                    if ( 4*offset + len <= mip4Connect->maxWR * 2 )
                                    {
                                        memcpy( pHmiVar->rData.data(), &msgPar.data[4*offset], len );
                                        if ( pHmiVar->wStatus != VAR_NO_OPERATION )
                                        {
                                            comMsg.varId = ( pHmiVar->varId & 0x0000ffff );
                                            comMsg.type = 0;
                                            comMsg.err = VAR_NO_ERR;
                                            m_comMsgTmp.insert( comMsg.varId, comMsg );
                                        }
                                    }
                                    else
                                        Q_ASSERT( false );
                                }
                            }
                            else if ( msgPar.area == MIPROTOCOL4_AREA_D || msgPar.area == MIPROTOCOL4_AREA_T || msgPar.area == MIPROTOCOL4_AREA_C )
                            {
                                Q_ASSERT( msgPar.count <= mip4Connect->maxWR );
                                int offset = pHmiVar->addOffset - mip4Connect->requestVar.addOffset;
                                Q_ASSERT( offset >= 0 );
                                Q_ASSERT( offset <= mip4Connect->maxWR );
                                if ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup )
                                {
                                    int len = 2 * (( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16 );
                                    if ( pHmiVar->rData.size() < len )
                                        pHmiVar->rData = QByteArray( len, 0 ); //初始化为0
                                    if ( 2*offset + len <= mip4Connect->maxWR * 2 )
                                    {
                                        memcpy( pHmiVar->rData.data(), &msgPar.data[2*offset], len );
                                        if ( pHmiVar->wStatus != VAR_NO_OPERATION )
                                        {
                                            comMsg.varId = ( pHmiVar->varId & 0x0000ffff );
                                            comMsg.type = 0;
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
                                    if ( 2*offset + len <= mip4Connect->maxWR * 2 ) //保证不越界
                                    {
                                        memcpy( pHmiVar->rData.data(), &msgPar.data[2*offset], len );
                                        if ( pHmiVar->wStatus != VAR_NO_OPERATION )
                                        {
                                            comMsg.varId = ( pHmiVar->varId & 0x0000ffff );
                                            comMsg.type = 0;
                                            comMsg.err = VAR_NO_ERR;
                                            m_comMsgTmp.insert( comMsg.varId, comMsg );
                                        }
                                    }
                                    else
                                        Q_ASSERT( false );
                                }
                            }
                            else
                            {
                                Q_ASSERT( msgPar.count <= mip4Connect->maxBR );
                                if ( !mip4Connect->waitWrtieVarsList.contains( pHmiVar ) )
                                {
                                    int startBitOffset = pHmiVar->addOffset - mip4Connect->requestVar.addOffset;
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
                                        comMsg.type = 0;
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
                                foreach( HmiVar* tempHmiVar, mip4Connect->actVarsList )
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
                mip4Connect->requestVarsList.clear();
            }
            if ( msgPar.err == VAR_COMM_TIMEOUT )
                mip4Connect->connStatus = CONNECT_NO_LINK;
            else
                mip4Connect->connStatus = CONNECT_LINKED_OK;
            break;
        }
    }
    return result;
}


void MiProtocol4Service::slot_run()
{
    initCommDev();
    m_runTimer->start(10);
}


void MiProtocol4Service::slot_readData()
{
    struct MiProtocol4MsgPar msgPar;
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
                struct Miprocotol4Connect* mip4Connect = m_connectList[i];
                Q_ASSERT( mip4Connect );
                mip4Connect->waitWrtieVarsList.clear();
                mip4Connect->requestVarsList.clear();
                foreach( HmiVar* tempHmiVar, mip4Connect->actVarsList )
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
    while ( ret == sizeof( struct MbusMsgPar ) );

}

void MiProtocol4Service::slot_runTimerTimeout()
{
    m_runTimer->stop();
    int lastUpdateTime = -1;

    int connectCount = m_connectList.count();
    m_currTime = getCurrTime();
    int currTime = m_currTime;

    for ( int i = 0; i < connectCount; i++ )
    {                              //处理写请求 struct Miprocotol4Connect* mip4Connect = m_connectList[curIndex];
        struct Miprocotol4Connect* mip4Connect = m_connectList[i];
        updateWriteReq( mip4Connect );
        trySndWriteReq( mip4Connect );
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
        struct Miprocotol4Connect* mip4Connect = m_connectList[i];

        if ( needUpdateFlag )
        {
            updateOnceRealReadReq( mip4Connect );
            updateReadReq( mip4Connect );
        }
        if ( !( hasProcessingReq( mip4Connect ) ) )
        {
            combineReadReq( mip4Connect );
        }
        trySndReadReq( mip4Connect );
    }
    m_runTimer->start(20);
}

