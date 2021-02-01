
/**********************************************************************
Copyright (c) 2008-2010 by SHENZHEN CO-TRUST AUTOMATION TECH.
** 文 件 名:  fatekService.cpp
** 说    明:  台达FATEK协议多协议平台实现文件，完成后台台达FATEK协议数据的处理以及链接，变量，读写状态的管理
** 版    本:  【可选】
** 创建日期:  2012.07.05
** 创 建 人:  陶健军
** 修改信息： 【可选】
**         修改人    修改日期       修改内容**
*********************************************************************/
#include "fatekService.h"


#define MAX_FATEK_PACKET_LEN  M_FATEK_MAX  //最大一包字数，因为驱动组包大小只有255字节，超过57个字会导致 写常数组的时候出错

/******************************************************************************
** 函  数  名 ： FatekService
** 说      明 :  构造函数
** 输      入 ： 链接参数
** 返      回 ： NONE
******************************************************************************/
FatekService::FatekService( struct FatekConnectCfg& connectCfg )
{
    struct FatekConnect* connect = new FatekConnect;
    connect->cfg = connectCfg;
    connect->writingCnt = 0;
    connect->connStatus = CONNECT_NO_OPERATION;
    connect->connectTestVar.lastUpdateTime = -1;

    m_connectList.append( connect );
    m_currTime = getCurrTime();
    m_runTimer = NULL;
    m_rwSocket = NULL;
}

FatekService::~FatekService()
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

/******************************************************************************
** 函  数  名 ： meetServiceCfg
** 说      明 :  判断是否存在FATEK类型的链接
** 输      入 ： 链接类型
        串口端口号
** 返      回 ： 存在与否标志
******************************************************************************/
bool FatekService::meetServiceCfg( int serviceType, int COM_interface )
{
    if ( serviceType == SERVICE_TYPE_FATEK )
    {
        if ( m_connectList.count() > 0 )
        {
            if ( m_connectList[0]->cfg.COM_interface == COM_interface )
            {
                return true;
            }
        }
        else
        {
            Q_ASSERT( false );
        }
    }
    return false;
}

/******************************************************************************
** 函  数  名 ： getConnLinkStatus
** 说      明 :  获取链接状态
** 输      入 ： 链接ID
** 返      回 ： 链接状态
******************************************************************************/
int FatekService::getConnLinkStatus( int connectId )
{
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct FatekConnect* connect = m_connectList[i];
        if ( connectId == connect->cfg.id )
        {
            return connect->connStatus;
        }
    }
    return -1;
}

/******************************************************************************
** 函  数  名 ： getConnLinkStatus
** 说      明 :  获取链接状态
** 输      入 ： 链接ID
** 返      回 ： 链接状态
******************************************************************************/
int FatekService::getComMsg( QList<struct ComMsg> &msgList )
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

/******************************************************************************
** 函  数  名 ： splitLongVar
** 说      明 :  分离数组变量，对于组态数组超过协议允许的最大值的变量分离成N个变量
** 输      入 ： 要分离的PLC变量结构体
** 返      回 ： 分离状态标志
******************************************************************************/
int FatekService::splitLongVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );

    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct FatekConnect* fatekConnect = m_connectList[i];
        if ( pHmiVar->connectId == fatekConnect->cfg.id )
        {
            pHmiVar->lastUpdateTime = -1; //立即更新
            if ( pHmiVar->area >= FATEK_AREA_TMR && pHmiVar->area <= FATEK_AREA_WS )
            {
                if ( pHmiVar->dataType == Com_Bool )
                {
                    if ( pHmiVar->len > MAX_FATEK_PACKET_LEN * 16 && pHmiVar->splitVars.count() <= 0 )  //拆分
                    {
                        for ( int splitedLen = 0; splitedLen < pHmiVar->len; )
                        {
                            int len = pHmiVar->len - splitedLen;
                            if ( len > MAX_FATEK_PACKET_LEN * 16 )
                            {
                                len =  MAX_FATEK_PACKET_LEN * 16;
                            }

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
                            pSplitHhiVar->wStatus = VAR_NO_ERR;   //注意这里的wStatus不能为 VAR_NO_OPERATION
                            pSplitHhiVar->lastUpdateTime = -1; //立即更新

                            splitedLen += len;
                            pHmiVar->splitVars.append( pSplitHhiVar );
                        }
                    }
                }
                else
                {
                    if ( pHmiVar->len > MAX_FATEK_PACKET_LEN*2 && pHmiVar->splitVars.count() <= 0 )  //拆分
                    {
                        for ( int splitedLen = 0; splitedLen < pHmiVar->len; )
                        {
                            int len = pHmiVar->len - splitedLen;
                            if ( len > MAX_FATEK_PACKET_LEN*2 )
                            {
                                len  = MAX_FATEK_PACKET_LEN * 2;
                            }

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
                            pSplitHhiVar->wStatus = VAR_NO_ERR;  //注意这里的wStatus不能为 VAR_NO_OPERATION
                            pSplitHhiVar->lastUpdateTime = -1; //立即更新
                            splitedLen += len;
                            pHmiVar->splitVars.append( pSplitHhiVar );
                        }
                    }
                }
            }
            return 0;
        }
    }
    return -1;
}

/******************************************************************************
** 函  数  名 ： onceRealUpdateVar
** 说      明 :  更新只读一次的变量
** 输      入 ： PLC变量结构体
** 返      回 ： 操作状态
******************************************************************************/
int FatekService::onceRealUpdateVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    m_mutexOnceReal.lock();
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct FatekConnect* fatekConnect = m_connectList[i];
        if ( pHmiVar->connectId == fatekConnect->cfg.id )
        {
            pHmiVar->lastUpdateTime = -1;
            if ( pHmiVar->splitVars.count() > 0 )
            {
                for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
                {
                    struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                    pSplitHhiVar->lastUpdateTime = -1;
                    pSplitHhiVar->overFlow = false;
                    if ( !fatekConnect->onceRealVarsList.contains( pSplitHhiVar ) )
                    {
                        fatekConnect->onceRealVarsList.append( pSplitHhiVar );
                    }
                }
            }
            else
            {
                pHmiVar->lastUpdateTime = -1;
                pHmiVar->overFlow = false;
                if ( !fatekConnect->onceRealVarsList.contains( pHmiVar ) )
                {
                    fatekConnect->onceRealVarsList.append( pHmiVar );
                }
            }

            m_mutexOnceReal.unlock();
            return 0;
        }
    }
    m_mutexOnceReal.unlock();
    return -1;
}

/******************************************************************************
** 函  数  名 ： startUpdateVar
** 说      明 :  开始循环读取PLC变量
** 输      入 ： PLC变量结构体
** 返      回 ： 操作状态
******************************************************************************/
int FatekService::startUpdateVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    m_mutexUpdateVar.lock();
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct FatekConnect* fatekConnect = m_connectList[i];
        if ( pHmiVar->connectId == fatekConnect->cfg.id )
        {
            pHmiVar->lastUpdateTime = -1;
            if ( pHmiVar->splitVars.count() > 0 )
            {
                for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
                {
                    struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                    pSplitHhiVar->lastUpdateTime = -1;
                    pSplitHhiVar->overFlow = false;
                    if ( !fatekConnect->actVarsList.contains( pSplitHhiVar ) )
                    {
                        fatekConnect->actVarsList.append( pSplitHhiVar );
                    }
                }
            }
            else
            {
                pHmiVar->lastUpdateTime = -1;
                pHmiVar->overFlow = false;
                if ( pHmiVar->addOffset >= 0 )
                {
                    if ( !fatekConnect->actVarsList.contains( pHmiVar ) )
                    {
                        fatekConnect->actVarsList.append( pHmiVar );
                    }
                }
                else
                {
                    pHmiVar->err = VAR_PARAM_ERROR;
                }
            }

            m_mutexUpdateVar.unlock();
            return 0;
        }
    }
    m_mutexUpdateVar.unlock();
    return -1;
}

/******************************************************************************
** 函  数  名 ： stopUpdateVar
** 说      明 :  停止读取PLC变量
** 输      入 ： PLC变量结构体
** 返      回 ： 操作状态
******************************************************************************/
int FatekService::stopUpdateVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    m_mutexUpdateVar.lock();
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct FatekConnect* fatekConnect = m_connectList[i];
        if ( pHmiVar->connectId == fatekConnect->cfg.id )
        {
            if ( pHmiVar->splitVars.count() > 0 )
            {
                for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
                {
                    struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                    fatekConnect->actVarsList.removeAll( pSplitHhiVar );
                }
            }
            else
            {
                fatekConnect->actVarsList.removeAll( pHmiVar );
            }
            m_mutexUpdateVar.unlock();
            return 0;
        }
    }
    m_mutexUpdateVar.unlock();
    return -1;
}

/******************************************************************************
** 函  数  名 ： readVar
** 说      明 :  把驱动读取到的数据传到后台缓冲区
** 输      入 ： PLC变量结构体。
        后台缓冲区，
        数据长度
** 返      回 ： 操作状态
******************************************************************************/
int FatekService::readVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    if ( pHmiVar->err == VAR_NO_ERR )  //无错误才拷贝数据
    {
        if ( pHmiVar->area == FATEK_AREA_CTR200 )
        {
            if ( pHmiVar->dataType == Com_Bool )
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
                        {
                            pBuf[j/7] |= ( 1 << ( j % 8 ) );
                        }
                        else
                        {
                            pBuf[j/7] &= ~( 1 << ( j % 8 ) );
                        }
                    }
                }
                else
                {
                    Q_ASSERT( false );
                }
            }
            else
            {
                if ( bufLen >= pHmiVar->len && pHmiVar->rData.size() >= pHmiVar->len )
                {
                    for ( int j = 0; j < pHmiVar->len; j++ )
                    {
                        if ( pHmiVar->dataType != Com_StringChar )           //奇偶字节调换 并且 高四位和低四位对调
                        {
                            int k = ( j + 2 ) % 4;
                            if ( j & 0x1 )
                            {
                                pBuf[k] = pHmiVar->rData.at( j - 1 );
                            }
                            else
                            {
                                pBuf[k] = pHmiVar->rData.at( j + 1 );
                            }
                        }
                        else
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
                    }
                }
                else
                {
                    Q_ASSERT( false );
                }
            }
        }
        else if ( pHmiVar->area >= FATEK_AREA_TMR && pHmiVar->area <= FATEK_AREA_WS )
        {
            if ( pHmiVar->dataType == Com_Bool )
            {                          // 位变量长度为位的个数
                Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
                int dataBytes = ( pHmiVar->len + 7 ) / 8;
                if ( bufLen >= dataBytes && pHmiVar->rData.size() >= dataBytes )
                {
                    for ( int j = 0; j < pHmiVar->len; j++ )
                    {
                        int byteOffset = ( pHmiVar->bitOffset + j ) / 8;
                        int bitOffset = ( pHmiVar->bitOffset + j ) % 8;
                        if (( byteOffset % 2 ) > 0 )
                        {
                            byteOffset--;
                        }
                        else
                        {
                            byteOffset++;
                        }
                        if ( pHmiVar->rData.at( byteOffset ) & ( 1 << bitOffset ) )
                        {
                            pBuf[j/7] |= ( 1 << ( j % 8 ) );
                        }
                        else
                        {
                            pBuf[j/7] &= ~( 1 << ( j % 8 ) );
                        }
                    }
                }
                else
                {
                    Q_ASSERT( false );
                }
            }
            else
            {
                if ( bufLen >= pHmiVar->len && pHmiVar->rData.size() >= pHmiVar->len )
                {
                    for ( int j = 0; j < pHmiVar->len; j++ )
                    {
                        if ( pHmiVar->dataType != Com_String ) //奇偶字节调换
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
                        {
                            pBuf[j] = pHmiVar->rData.at( j );
                        }
                    }
                }
                else
                {
                    Q_ASSERT( false );
                }
            }
        }
        else
        {
            int dataBytes = ( pHmiVar->len + 7 ) / 8; //字节数
            if ( bufLen >= dataBytes && pHmiVar->rData.size() >= dataBytes )
            {
                memcpy( pBuf, pHmiVar->rData.data(), dataBytes );
            }
            else
            {
                Q_ASSERT( false );
            }
        }
    }
    return pHmiVar->err;
}

/******************************************************************************
** 函  数  名 ： readDevVar
** 说      明 :  comServer 读数据的接口
** 输      入 ： PLC变量结构体。
        后台缓冲区，
        数据长度
** 返      回 ： 操作状态
******************************************************************************/
int FatekService::readDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    Q_ASSERT( pHmiVar && pBuf );
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct FatekConnect* fatekConnect = m_connectList[i];
        if ( pHmiVar->connectId == fatekConnect->cfg.id )
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
                    {
                        len = ( pSplitHhiVar->len + 7 ) / 8;
                    }
                    else
                    {
                        len = pSplitHhiVar->len;
                    }

                    ret = readVar( pSplitHhiVar, pBuf, len );
                    if ( ret != VAR_NO_ERR )
                    {
                        break;
                    }
                    pBuf += len;
                    bufLen -= len;
                }
            }
            else
            {
                ret = readVar( pHmiVar, pBuf, bufLen );
            }

            m_mutexRw.unlock();
            return ret;
        }
    }
    return -1;
}

/******************************************************************************
** 函  数  名 ： writeVar
** 说      明 : 把后台向驱动写的数据写入到读缓冲区，
** 输      入 ： PLC变量结构体。
        后台缓冲区，
        数据长度
** 返      回 ： 操作状态
******************************************************************************/
int FatekService::writeVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    int ret;
    Q_ASSERT( pHmiVar && pBuf );
    m_mutexRw.lock();
    if ( pHmiVar->area == FATEK_AREA_CTR200 )
    {//大小数等到发送写命令时再处理
        if ( pHmiVar->dataType == Com_Bool )
        {                        //位变量长度为位的个数
            Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 31 );
            int count = ( pHmiVar->bitOffset + pHmiVar->len + 31 ) / 32;  //双字数
            Q_ASSERT( count > 0 && count <= MAX_FATEK_PACKET_LEN );
            if ( count > MAX_FATEK_PACKET_LEN )
            {
                count = MAX_FATEK_PACKET_LEN;
            }
            int dataBytes = 4 * count;
            dataBytes = qMin( bufLen, dataBytes );
            if ( pHmiVar->wData.size() < dataBytes )
            {
                pHmiVar->wData.resize( dataBytes );
            }

            memcpy( pHmiVar->wData.data(), pBuf, bufLen );
            pHmiVar->lastUpdateTime = -1;

            if ( pHmiVar->rData.size() < dataBytes )
            {
                pHmiVar->rData.resize( dataBytes );
            }
            for ( int i = 0; i < pHmiVar->len; i++ )
            {
                int byteW = i / 8;
                int bitW = i % 8;
                int byteR = ( pHmiVar->bitOffset + i ) / 8;
                int bitR = 7 - ( pHmiVar->bitOffset + i ) % 8;
                if ( byteW < pHmiVar->wData.size() && byteR < pHmiVar->rData.size() )
                {
                    if ( pHmiVar->wData[byteW] & ( 1 << bitW ) )
                    {
                        pHmiVar->rData[byteR] = (( uchar )pHmiVar->rData[byteR] ) | ( 1 << bitR );
                    }
                    else
                    {
                        pHmiVar->rData[byteR] = (( uchar )pHmiVar->rData[byteR] ) & ( ~( 1 << bitR ) );
                    }
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            int count = ( pHmiVar->len + 3 ) / 4; //双字数
            Q_ASSERT( count > 0 && count <= MAX_FATEK_PACKET_LEN );
            if ( count > MAX_FATEK_PACKET_LEN )
            {
                count = MAX_FATEK_PACKET_LEN;
            }
            int dataBytes = 4 * count;
            if ( pHmiVar->wData.size() < dataBytes )
            {
                pHmiVar->wData.resize( dataBytes );
            }

            memcpy( pHmiVar->wData.data(), pBuf, bufLen );
            pHmiVar->lastUpdateTime = -1;

            if ( pHmiVar->rData.size() < dataBytes )
            {
                pHmiVar->rData.resize( dataBytes );
            }
            for ( int j = 0; j < pHmiVar->len; j++ )
            {                          // 奇偶字节调换
                if ( j < pHmiVar->wData.size() && j < pHmiVar->rData.size() )
                {
                    pHmiVar->rData[j] = pHmiVar->wData.at( 3 - j );
                }
                else
                {
                    break;
                }
            }
        }
    }
    else if ( pHmiVar->area >= FATEK_AREA_TMR && pHmiVar->area <= FATEK_AREA_WS )
    {                            //大小数等到发送写命令时再处理
        if ( pHmiVar->dataType == Com_Bool )
        {                        //位变量长度为位的个数
            Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
            int count = ( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16;  //字数
            Q_ASSERT( count > 0 && count <= MAX_FATEK_PACKET_LEN );
            if ( count > MAX_FATEK_PACKET_LEN )
            {
                count = MAX_FATEK_PACKET_LEN;
            }
            int dataBytes = 2 * count;
            dataBytes = qMin( bufLen, dataBytes );
            if ( pHmiVar->wData.size() < dataBytes )
            {
                pHmiVar->wData.resize( dataBytes );
            }
            memcpy( pHmiVar->wData.data(), pBuf, bufLen );

            pHmiVar->lastUpdateTime = -1;

            if ( pHmiVar->rData.size() < dataBytes )
            {
                pHmiVar->rData.resize( dataBytes );
            }
            for ( int i = 0; i < dataBytes; i++ )
            {
                int byteW = i / 8;
                int bitW = i % 8;
                int byteR = ( pHmiVar->bitOffset + i ) / 8;
                int bitR = ( pHmiVar->bitOffset + i ) % 8;
                if (( byteR % 2 ) > 0 )
                {
                    byteR--;
                }
                else
                {
                    byteR++;
                }
                if ( byteW < pHmiVar->wData.size() && byteR < pHmiVar->rData.size() )
                {
                    if ( pHmiVar->wData[byteW] & ( 1 << bitW ) )
                    {
                        pHmiVar->rData[byteR] = (( uchar )pHmiVar->rData[byteR] ) | ( 1 << bitR );
                    }
                    else
                    {
                        pHmiVar->rData[byteR] = (( uchar )pHmiVar->rData[byteR] ) & ( ~( 1 << bitR ) );
                    }
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            int count = ( pHmiVar->len + 1 ) / 2; //字数
            Q_ASSERT( count > 0 && count <= MAX_FATEK_PACKET_LEN );
            if ( count > MAX_FATEK_PACKET_LEN )
            {
                count = MAX_FATEK_PACKET_LEN;
            }
            int dataBytes = 2 * count;
            if ( pHmiVar->wData.size() < dataBytes )
            {
                pHmiVar->wData.resize( dataBytes );
            }
            memcpy( pHmiVar->wData.data(), pBuf, bufLen );
            pHmiVar->lastUpdateTime = -1;
            if ( pHmiVar->rData.size() < dataBytes )
            {
                pHmiVar->rData.resize( dataBytes );
            }
            for ( int j = 0; j < pHmiVar->len; j++ )
            {                          // 奇偶字节调换
                if ( j < pHmiVar->wData.size() && j < pHmiVar->rData.size() )
                {
                    if ( pHmiVar->dataType != Com_String )
                    {
                        if ( j & 0x1 )
                        {
                            pHmiVar->rData[j - 1] = pHmiVar->wData.at( j );
                        }
                        else if ( j + 1 < pHmiVar->wData.size() )
                        {
                            pHmiVar->rData[j + 1] = pHmiVar->wData.at( j );
                        }
                    }
                    else
                    {
                        pHmiVar->rData[j] = pHmiVar->wData.at( j );
                    }
                }
                else
                {
                    break;
                }
            }
        }
    }
    else
    {
        Q_ASSERT( pHmiVar->dataType == Com_Bool );
        int count = pHmiVar->len;
        Q_ASSERT( count > 0 &&  count <= MAX_FATEK_PACKET_LEN );
        if ( count > MAX_FATEK_PACKET_LEN )
        {
            count = MAX_FATEK_PACKET_LEN;
        }
        int dataBytes = ( count + 7 ) / 8;  //字节数
        dataBytes = qMin( bufLen, dataBytes );
        if ( pHmiVar->wData.size() < dataBytes )
        {
            pHmiVar->wData.resize( dataBytes );
        }
        memcpy( pHmiVar->wData.data(), pBuf, bufLen );
        pHmiVar->lastUpdateTime = -1;

        if ( pHmiVar->rData.size() < dataBytes )
        {
            pHmiVar->rData.resize( dataBytes );
        }
        memcpy( pHmiVar->rData.data(), pHmiVar->wData.data(), dataBytes );
    }
    pHmiVar->wStatus = VAR_NO_OPERATION;
    ret = pHmiVar->err;
    m_mutexRw.unlock();
    return ret; //是否要返回成功
}

/******************************************************************************
** 函  数  名 ： writeDevVar
** 说      明 : comserver 写数据的接口
** 输      入 ： PLC变量结构体。
        后台缓冲区，
        数据长度
** 返      回 ： 操作状态
******************************************************************************/
int FatekService::writeDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    Q_ASSERT( pHmiVar && pBuf );

    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct FatekConnect* fatekConnect = m_connectList[i];
        if ( pHmiVar->connectId == fatekConnect->cfg.id )
        {
            int ret = 0;
            if ( fatekConnect->connStatus == CONNECT_NO_LINK )//if this var has read and write err, do not execute write, until it is read or wriet correct.
            {
                ret = VAR_PARAM_ERROR;
            }
            else
            {
                m_mutexWReq.lock();          //注意2个互斥锁顺序,防止死锁
                if ( pHmiVar->splitVars.count() > 0 )
                {
                    for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
                    {
                        int len = 0;
                        struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                        if ( pHmiVar->dataType == Com_Bool )
                        {
                            len = ( pSplitHhiVar->len + 7 ) / 8;
                        }
                        else
                        {
                            len = pSplitHhiVar->len;
                        }

                        ret = writeVar( pSplitHhiVar, pBuf, len );
                        fatekConnect->waitWrtieVarsList.append( pSplitHhiVar );
                        pBuf += len;
                        bufLen -= len;
                    }
                }
                else
                {
                    ret = writeVar( pHmiVar, pBuf, bufLen );
                    fatekConnect->waitWrtieVarsList.append( pHmiVar );
                }
                m_mutexWReq.unlock();
            }
            return ret; //是否要返回成功
        }
    }
    return -1;
}

/******************************************************************************
** 函  数  名 ： addConnect
** 说      明 : 增加连接，当连接数量超过1时才会调用
** 输      入 ： struct FatekConnectCfg
** 返      回 ： 操作状态
******************************************************************************/
int FatekService::addConnect( struct FatekConnectCfg& connectCfg )
{                                      //要检查是否重复 add
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct FatekConnect* fatekConnect = m_connectList[i];
        Q_ASSERT( fatekConnect->cfg.COM_interface == connectCfg.COM_interface );//绝对要是同一个设备接口
        if ( connectCfg.id == fatekConnect->cfg.id )
        {
            Q_ASSERT( false );
            return 0;
        }
    }
    struct FatekConnect* fatekConnect = new FatekConnect;
    fatekConnect->cfg = connectCfg;
    fatekConnect->writingCnt = 0;
    fatekConnect->connStatus = CONNECT_NO_OPERATION;
    fatekConnect->connectTestVar.lastUpdateTime = -1;
    m_connectList.append( fatekConnect );
    return 0;
}

/******************************************************************************
** 函  数  名 ： sndWriteVarMsgToDriver
** 说      明 : 把写缓冲区数据打包到 struct FatekMsgPar中，然后写入到驱动
** 输      入 ： struct FatekConnect*
        struct HmiVar*
** 返      回 ： 操作状态
******************************************************************************/
int FatekService::sndWriteVarMsgToDriver( struct FatekConnect* fatekConnect, struct HmiVar* pHmiVar )
{
    int sendedLen = 0;
    Q_ASSERT( fatekConnect && pHmiVar );
//#ifdef Q_WS_QWS
    struct FatekMsgPar msgPar;

    m_mutexRw.lock();
    msgPar.rw = 1;   //写
    msgPar.slave    = fatekConnect->cfg.cpuAddr;
    msgPar.area     = pHmiVar->area;
    msgPar.addr     = pHmiVar->addOffset;
    msgPar.err = 0;

    if ( pHmiVar->area == FATEK_AREA_CTR200 )
    {
        if ( pHmiVar->dataType == Com_Bool )
        {                              // 位变量长度为位的个数
            Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 31 );
            int count = ( pHmiVar->bitOffset + pHmiVar->len + 31 ) / 32;  //双字数
            Q_ASSERT( count > 0 && count*2 <= MAX_FATEK_PACKET_LEN );
            if ( count*2 > MAX_FATEK_PACKET_LEN )
            {
                count = ( MAX_FATEK_PACKET_LEN / 2 );
            }
            msgPar.count = count;      // 占用的总双字数,但是到驱动要转换成字数
            int dataBytes = 4 * count;
            if ( pHmiVar->rData.size() < dataBytes )
            {
                pHmiVar->rData.resize( dataBytes );
            }

            for ( int i = 0; i < pHmiVar->len; i++ )
            {
                int byteW = i / 8;
                int bitW = i % 8;
                int byteR = ( pHmiVar->bitOffset + i ) / 8;
                int bitR = 7 - ( pHmiVar->bitOffset + i ) % 8;

                if ( byteW < pHmiVar->wData.size() && byteR < pHmiVar->rData.size() )
                {
                    if ( pHmiVar->wData[byteW] & ( 1 << bitW ) )
                    {
                        pHmiVar->rData[byteR] = (( uchar )pHmiVar->rData[byteR] ) | ( 1 << bitR );
                    }
                    else
                    {
                        pHmiVar->rData[byteR] = (( uchar )pHmiVar->rData[byteR] ) & ( ~( 1 << bitR ) );
                    }
                }
                else
                {
                    break;
                }
            }
            memcpy( msgPar.data, pHmiVar->rData.data(), dataBytes );
            sendedLen = count * 32;
        }
        else
        {
            int count = ( pHmiVar->len + 3 ) / 4; //双字数
            Q_ASSERT( count > 0 && count*2 <= MAX_FATEK_PACKET_LEN );
            if ( count*2 > MAX_FATEK_PACKET_LEN )
            {
                count = ( MAX_FATEK_PACKET_LEN / 2 );
            }
            msgPar.count = count;      // 占用的总双字数
            int dataBytes = 4 * count;
            if ( pHmiVar->rData.size() < dataBytes )
            {
                pHmiVar->rData.resize( dataBytes );
            }

            for ( int j = 0; j < pHmiVar->len; j++ )
            {                          // 奇偶字节调换
                if ( j < pHmiVar->wData.size() && j < pHmiVar->rData.size() )
                {
                    pHmiVar->rData[j] = pHmiVar->wData.at( 3 - j );
                }
                else
                {
                    break;
                }
            }
            memcpy( msgPar.data, pHmiVar->rData.data(), dataBytes );
            sendedLen = count * 4;
        }
    }
    else if ( msgPar.area >= FATEK_AREA_TMR && msgPar.area <= FATEK_AREA_WS  )
    {
        if ( pHmiVar->dataType == Com_Bool )
        {                              // 位变量长度为位的个数
            Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
            int count = ( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16;  //字数
            Q_ASSERT( count > 0 && count <= MAX_FATEK_PACKET_LEN );
            if ( count > MAX_FATEK_PACKET_LEN )
            {
                count = MAX_FATEK_PACKET_LEN;
            }
            msgPar.count = count;      // 占用的总字数
            int dataBytes = 2 * count;
            if ( pHmiVar->rData.size() < dataBytes )
            {
                pHmiVar->rData.resize( dataBytes );
            }

            //把数据 0201 0304 转换成 0403 0201格式
            for ( int i = 0; i < pHmiVar->len; i++ )
            {
                int byteW = i / 8;
                int bitW = i % 8;
                int byteR = ( pHmiVar->bitOffset + i ) / 8;
                int bitR = ( pHmiVar->bitOffset + i ) % 8;
                if (( byteR % 2 ) > 0 )
                {
                    byteR--;
                }
                else
                {
                    byteR++;
                }
                if ( byteW < pHmiVar->wData.size() && byteR < pHmiVar->rData.size() )
                {
                    if ( pHmiVar->wData[byteW] & ( 1 << bitW ) )
                    {
                        pHmiVar->rData[byteR] = (( uchar )pHmiVar->rData[byteR] ) | ( 1 << bitR );
                    }
                    else
                    {
                        pHmiVar->rData[byteR] = (( uchar )pHmiVar->rData[byteR] ) & ( ~( 1 << bitR ) );
                    }
                }
                else
                {
                    break;
                }
            }
            memcpy( msgPar.data, pHmiVar->rData.data(), dataBytes );
            sendedLen = count * 16;
        }
        else
        {
            int count = ( pHmiVar->len + 1 ) / 2; //字数
            Q_ASSERT( count > 0 && count <= MAX_FATEK_PACKET_LEN );
            if ( count > MAX_FATEK_PACKET_LEN )
            {
                count = MAX_FATEK_PACKET_LEN;
            }
            msgPar.count = count;      // 占用的总字数
            int dataBytes = 2 * count;
            if ( pHmiVar->rData.size() < dataBytes )
            {
                pHmiVar->rData.resize( dataBytes );
            }

            for ( int j = 0; j < pHmiVar->len; j++ )
            {                       // 奇偶字节调换
                if ( j < pHmiVar->wData.size() && j < pHmiVar->rData.size() )
                {
                    if ( pHmiVar->dataType != Com_String )
                    {
                        if ( j & 0x1 )
                        {
                            pHmiVar->rData[j - 1] = pHmiVar->wData.at( j );
                        }
                        else
                        {
                            pHmiVar->rData[j + 1] = pHmiVar->wData.at( j );
                        }
                    }
                    else
                    {
                        pHmiVar->rData[j] = pHmiVar->wData.at( j );
                    }
                }
                else
                {
                    break;
                }
            }
            memcpy( msgPar.data, pHmiVar->rData.data(), dataBytes );
            sendedLen = count * 2;
        }
    }
    else if ( /*msgPar.area >= FATEK_AREA_S && */msgPar.area <= FATEK_AREA_C )
    {                                  // 位变量长度为位的个数
        Q_ASSERT( pHmiVar->dataType == Com_Bool );
        int count = pHmiVar->len;
        Q_ASSERT( count > 0 &&  count <= (( MAX_FATEK_PACKET_LEN - 1 )*16 ) );
        if ( count > (( MAX_FATEK_PACKET_LEN - 1 )*16 ) )
        {
            count = ( MAX_FATEK_PACKET_LEN - 1 ) * 16;
        }
        msgPar.count = count;
        int dataBytes = ( count + 7 ) / 8;
        if ( pHmiVar->rData.size() < dataBytes )
        {
            pHmiVar->rData.resize( dataBytes );
        }

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
        //int len = write( m_fd, ( char* ) & msgPar, sizeof( struct FatekMsgPar ) );
        int len = m_rwSocket->writeDev(( char* ) & msgPar, sizeof( struct FatekMsgPar ));
        Q_ASSERT( len > 0 );
    }
    else
        m_mutexRw.unlock();
//#endif
    return sendedLen;
}

/******************************************************************************
** 函  数  名 ： sndReadVarMsgToDriver
** 说      明 : 把读缓冲区数据打包到 struct FatekMsgPar中，然后写入到驱动
** 输      入 ： struct FatekConnect*
        struct HmiVar*
** 返      回 ： 操作状态
******************************************************************************/
int FatekService::sndReadVarMsgToDriver( struct FatekConnect* fatekConnect, struct HmiVar* pHmiVar )
{//printf("sndReadVarMsgToDriver\n");
    int sendedLen = 0;
    Q_ASSERT( fatekConnect && pHmiVar );
//#ifdef Q_WS_QWS
    struct FatekMsgPar msgPar;

    m_mutexRw.lock();
    msgPar.rw = 0;    // 读
    msgPar.slave = fatekConnect->cfg.cpuAddr;
    msgPar.area = pHmiVar->area;
    msgPar.addr = pHmiVar->addOffset;
    msgPar.err = 0;
    if ( pHmiVar->area == FATEK_AREA_CTR200 )
    {
        if ( pHmiVar->dataType == Com_Bool )
        {                              // 位变量长度为位的个数
            Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 31 );
            int count = ( pHmiVar->bitOffset + pHmiVar->len + 31 ) / 32;  //字数
            Q_ASSERT( count > 0 && count*2 <= MAX_FATEK_PACKET_LEN );
            if ( count*2 > MAX_FATEK_PACKET_LEN )
            {
                count = MAX_FATEK_PACKET_LEN / 2;
            }
            msgPar.count = count;      // 占用的总双字数到底层驱动要转换成字数
            sendedLen = count * 32;
        }
        else
        {
            int count = ( pHmiVar->len + 3 ) / 4;  //字数
            Q_ASSERT( count > 0 && count*2 <= MAX_FATEK_PACKET_LEN );
            if ( count*2 > MAX_FATEK_PACKET_LEN )
            {
                count = MAX_FATEK_PACKET_LEN / 2;
            }
            msgPar.count = count;      // 占用的总双字数到底层驱动要转换成字数
            sendedLen = count * 4;
        }
    }
    else if ( msgPar.area >= FATEK_AREA_TMR && msgPar.area <= FATEK_AREA_WS )
    {
        if ( pHmiVar->dataType == Com_Bool )
        {                              // 位变量长度为位的个数
            Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
            int count = ( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16;  //字数
            Q_ASSERT( count > 0 && count <= MAX_FATEK_PACKET_LEN );
            if ( count > MAX_FATEK_PACKET_LEN )
            {
                count = MAX_FATEK_PACKET_LEN;
            }
            msgPar.count = count;      // 占用的总字数
            sendedLen = count * 16;
        }
        else
        {
            int count = ( pHmiVar->len + 1 ) / 2;  //字数
            Q_ASSERT( count > 0 && count <= MAX_FATEK_PACKET_LEN );
            if ( count > MAX_FATEK_PACKET_LEN )
            {
                count = MAX_FATEK_PACKET_LEN;
            }
            msgPar.count = count;      // 占用的总字数
            sendedLen = count * 4;
        }
    }
    else if ( /*msgPar.area >= FATEK_AREA_S &&*/ msgPar.area <= FATEK_AREA_C )
    {                                  // 位变量长度为位的个数
        Q_ASSERT( pHmiVar->dataType == Com_Bool );
        int count = pHmiVar->len;
        Q_ASSERT( count > 0 &&  count <= (( MAX_FATEK_PACKET_LEN - 1 )*16 ) );
        if ( count > (( MAX_FATEK_PACKET_LEN - 1 )*16 ) )
        {
            count = (( MAX_FATEK_PACKET_LEN - 1 ) * 16 );
        }
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
        //int len = write( m_fd, ( char* ) & msgPar, sizeof( struct FatekMsgPar ) );
        int len = m_rwSocket->writeDev( ( char* ) & msgPar, sizeof( struct FatekMsgPar ));
        //Q_ASSERT( len > 0 );
        if ( len <= 0 )
        {
            qDebug()<<"send read MSG to driver err! return len:"<<len;
        }
    }
    else
    {
        m_mutexRw.unlock();
    }
//#endif
    return sendedLen;
}

/******************************************************************************
** 函  数  名 ： isWaitingReadRsp
** 说      明 : 是否有读数据请求已发往驱动
** 输      入 ： struct FatekConnect*
        struct HmiVar*
** 返      回 ： 操作状态
******************************************************************************/
bool isWaitingReadRsp( struct FatekConnect* fatekConnect, struct HmiVar* pHmiVar )
{
    Q_ASSERT( fatekConnect && pHmiVar );
    if ( fatekConnect->waitUpdateVarsList.contains( pHmiVar ) )
    {
        return true;
    }
    else if ( fatekConnect->combinVarsList.contains( pHmiVar ) )
    {
        return true;
    }
    else if ( fatekConnect->requestVarsList.contains( pHmiVar ) )
    {
        return true;
    }
    else
    {
        return false;
    }
}

/******************************************************************************
** 函  数  名 ： trySndWriteReq
** 说      明 : 检测现有状态能否发送写请求到驱动
** 输      入 ： struct FatekConnect*
** 返      回 ： 操作状态
******************************************************************************/
bool FatekService::trySndWriteReq( struct FatekConnect* fatekConnect ) //不处理超长变量
{
    if ( m_devUseCnt < MAXDEVCNT )                          //没有正在读写, snd read, 一次只能发送一包请求
    {
        if ( fatekConnect->writingCnt == 0 && fatekConnect->requestVarsList.count() == 0 ) //这个连接没有读写
        {
            m_mutexWReq.lock();
            if ( fatekConnect->waitWrtieVarsList.count() > 0 )
            {
                struct HmiVar* pHmiVar = fatekConnect->waitWrtieVarsList[0];
                if (( pHmiVar->lastUpdateTime == -1 )  //3X,4X 位变量需要先读
                                && ( pHmiVar->area >= FATEK_AREA_TMR && pHmiVar->area <= FATEK_AREA_WS )
                                && ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_String ) )
                {
                    if ( !isWaitingReadRsp( fatekConnect, pHmiVar ) )
                    {
                        fatekConnect->waitUpdateVarsList.prepend( pHmiVar ); //加入到读队列最前,保证下次最先读
                    }
                }
                else if ( !isWaitingReadRsp( fatekConnect, pHmiVar ) ) //没有正在等待读
                {                                   //直接写,snd write cmd to driver
                    if ( sndWriteVarMsgToDriver( fatekConnect, pHmiVar ) > 0 )
                    {
                        fatekConnect->writingCnt = 1;    //进入写状态
                        m_mutexWReq.unlock();
                        return true;
                    }
                    else
                    {
                        fatekConnect->waitWrtieVarsList.takeAt( 0 );//出列,丢掉写请求
                        pHmiVar->err = VAR_PARAM_ERROR;
                    }
                }
            }

            m_mutexWReq.unlock();
        }
    }
    return false;
}

/******************************************************************************
** 函  数  名 ： trySndReadReq
** 说      明 : 检测现有状态能否发送读数据请求到驱动
** 输      入 ： struct FatekConnect*
** 返      回 ： 操作状态
******************************************************************************/
bool FatekService::trySndReadReq( struct FatekConnect* fatekConnect ) //不处理超长变量
{
    if ( m_devUseCnt < MAXDEVCNT )             //没有正在读写, snd read, 一次只能发送一包请求
    {
        if ( fatekConnect->writingCnt == 0 && fatekConnect->requestVarsList.count() == 0 ) //这个连接没有读写
        {
            if ( fatekConnect->combinVarsList.count() > 0 )
            {
                fatekConnect->requestVarsList = fatekConnect->combinVarsList;
                fatekConnect->requestVar = fatekConnect->combinVar;
                for ( int i = 0; i < fatekConnect->combinVarsList.count(); i++ )
                {                      //要把已经请求的变量从等待队列中删除
                    struct HmiVar* pHmiVar = fatekConnect->combinVarsList[i];
                    fatekConnect->waitUpdateVarsList.removeAll( pHmiVar );
                    pHmiVar->lastUpdateTime = m_currTime;
                }
                fatekConnect->combinVarsList.clear();

                if ( sndReadVarMsgToDriver( fatekConnect, &( fatekConnect->requestVar ) ) > 0 )
                {
                    return true;
                }
                else
                {
                    for ( int i = 0; i < fatekConnect->requestVarsList.count(); i++ )
                    {
                        struct HmiVar* pHmiVar = fatekConnect->requestVarsList[i];
                        pHmiVar->err = VAR_PARAM_ERROR;
                    }
                    fatekConnect->requestVarsList.clear();
                }
            }
        }
    }
    return false;
}

/******************************************************************************
** 函  数  名 ： updateWriteReq
** 说      明 : 更新写请求
** 输      入 ： struct FatekConnect*
** 返      回 ： 操作状态
******************************************************************************/
bool FatekService::updateWriteReq( struct FatekConnect* fatekConnect )
{
    bool newUpdateVarFlag = false;
    QList<struct HmiVar*> tmpList;
    m_mutexWReq.lock();

    for ( int i = 0; i < fatekConnect->wrtieVarsList.count(); i++ )
    {
        struct HmiVar* pHmiVar = fatekConnect->wrtieVarsList[i];

        if ( !fatekConnect->waitWrtieVarsList.contains( pHmiVar ) )
        {
            fatekConnect->waitWrtieVarsList.append( pHmiVar );
            newUpdateVarFlag = true;
        }
        else
        {
            tmpList.append(pHmiVar);
        }
    }
    fatekConnect->wrtieVarsList.clear() ;
    fatekConnect->wrtieVarsList += tmpList;
    m_mutexWReq.unlock();
    return newUpdateVarFlag;
}

/******************************************************************************
** 函  数  名 ： updateReadReq
** 说      明 : 更新读请求
** 输      入 ： struct FatekConnect*
** 返      回 ： 操作状态
******************************************************************************/
bool FatekService::updateReadReq( struct FatekConnect* fatekConnect )
{
    bool newUpdateVarFlag = false;
    m_mutexUpdateVar.lock();
    int currTime = m_currTime;//getCurrTime();
    if ( fatekConnect->connStatus == CONNECT_LINKED_OK )
    {
        for ( int i = 0; i < fatekConnect->actVarsList.count(); i++ )
        {                                  // 找出需要刷新的变量
            struct HmiVar* pHmiVar = fatekConnect->actVarsList[i];
            if (( pHmiVar->lastUpdateTime == -1 ) || ( calIntervalTime( pHmiVar->lastUpdateTime, currTime ) >= pHmiVar->cycleTime ) )
            {                              // 需要刷新
                if (( !fatekConnect->waitUpdateVarsList.contains( pHmiVar ) )
                                && ( !fatekConnect->waitWrtieVarsList.contains( pHmiVar ) )
                                && ( !fatekConnect->requestVarsList.contains( pHmiVar ) ) )
                {
                    fatekConnect->waitUpdateVarsList.append( pHmiVar );
                    newUpdateVarFlag = true;
                }
            }
        }
    }
    else
    {
        fatekConnect->waitUpdateVarsList.clear();
        if ( fatekConnect->actVarsList.count() > 0  && fatekConnect->requestVarsList.count() == 0 )
        {
            fatekConnect->actVarsList[0]->lastUpdateTime = fatekConnect->connectTestVar.lastUpdateTime;
            fatekConnect->connectTestVar = *( fatekConnect->actVarsList[0] );
            fatekConnect->connectTestVar.varId = 65535;
            fatekConnect->connectTestVar.cycleTime = 2000;
            if (( fatekConnect->connectTestVar.lastUpdateTime == -1 )
                            || ( calIntervalTime( fatekConnect->connectTestVar.lastUpdateTime, currTime ) >= fatekConnect->connectTestVar.cycleTime ) )
            {
                if (( !fatekConnect->waitUpdateVarsList.contains( &( fatekConnect->connectTestVar ) ) )
                                && ( !fatekConnect->waitWrtieVarsList.contains( &( fatekConnect->connectTestVar ) ) )
                                && ( !fatekConnect->requestVarsList.contains( &( fatekConnect->connectTestVar ) ) ) )
                {
                    fatekConnect->waitUpdateVarsList.append( &( fatekConnect->connectTestVar ) );
                    newUpdateVarFlag = true;
                }

            }
        }
    }
    m_mutexUpdateVar.unlock();
    return newUpdateVarFlag;
}

/******************************************************************************
** 函  数  名 ： updateOnceRealReadReq
** 说      明 : 更新只读一次的读请求
** 输      入 ： struct FatekConnect*
** 返      回 ： 操作状态
******************************************************************************/
bool FatekService::updateOnceRealReadReq( struct FatekConnect* fatekConnect )
{
    bool newUpdateVarFlag = false;
    m_mutexOnceReal.lock();
    for ( int i = 0; i < fatekConnect->onceRealVarsList.count(); i++ )
    {
        struct HmiVar* pHmiVar = fatekConnect->onceRealVarsList[i];
        if (( !fatekConnect->waitUpdateVarsList.contains( pHmiVar ) )
                        && ( !fatekConnect->requestVarsList.contains( pHmiVar ) ) )
        {
            fatekConnect->waitUpdateVarsList.prepend( pHmiVar );
            newUpdateVarFlag = true;
        }
        else
        {
            if ( fatekConnect->waitUpdateVarsList.contains( pHmiVar ) )
            {
                fatekConnect->waitUpdateVarsList.removeAll( pHmiVar );
                fatekConnect->waitUpdateVarsList.prepend( pHmiVar );
            }
        }
    }
    fatekConnect->onceRealVarsList.clear();
    m_mutexOnceReal.unlock();

    return newUpdateVarFlag;
}

/******************************************************************************
** 函  数  名 ： combineReadReq
** 说      明 : 把相同区域的 可组合的变量组合再一起，可以提高通讯速度
** 输      入 ： struct FatekConnect*
** 返      回 ： 操作状态
******************************************************************************/
void FatekService::combineReadReq( struct FatekConnect* fatekConnect )
{//printf("combineReadReq\n");
    fatekConnect->combinVarsList.clear();
    if ( fatekConnect->waitUpdateVarsList.count() >= 1 )
    {
        struct HmiVar* pHmiVar = fatekConnect->waitUpdateVarsList[0];
        fatekConnect->combinVarsList.append( pHmiVar );
        fatekConnect->combinVar = *pHmiVar;
        if ( fatekConnect->combinVar.area >= FATEK_AREA_TMR && fatekConnect->combinVar.area <= FATEK_AREA_WS )
        {
            if ( pHmiVar->dataType == Com_Bool )
            {
                Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
                fatekConnect->combinVar.dataType = Com_Word;
                fatekConnect->combinVar.len = 2 * (( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16 ); //2*count
                fatekConnect->combinVar.bitOffset = -1;
            }
        }
        else if ( fatekConnect->combinVar.area == FATEK_AREA_CTR200 )
        {
            if ( pHmiVar->dataType == Com_Bool )
            {
                Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 31 );
                fatekConnect->combinVar.dataType = Com_DWord;
                fatekConnect->combinVar.len = 4 * (( pHmiVar->bitOffset + pHmiVar->len + 31 ) / 32 ); //4*count
                fatekConnect->combinVar.bitOffset = -1;
            }
        }
    }
    for ( int i = 1; i < fatekConnect->waitUpdateVarsList.count(); i++ )
    {
        struct HmiVar* pHmiVar = fatekConnect->waitUpdateVarsList[i];
        if ( pHmiVar->area == fatekConnect->combinVar.area && fatekConnect->combinVar.overFlow == pHmiVar->overFlow && ( pHmiVar->overFlow == false ) )
        {                            //没有越界才合并，否则不合并
            int startOffset = qMin( fatekConnect->combinVar.addOffset, pHmiVar->addOffset );
            int endOffset = 0;
            int newLen = 0;
            if ( fatekConnect->combinVar.area == FATEK_AREA_CTR200 )
            {
                int combineCount = ( fatekConnect->combinVar.len + 3 ) / 4;
                int varCount = 0;

                if ( pHmiVar->dataType == Com_Bool )
                {
                    Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 31 );
                    varCount = ( pHmiVar->bitOffset + pHmiVar->len + 31 ) / 32;
                }
                else
                    varCount = ( pHmiVar->len + 3 ) / 4;

                endOffset = qMax( fatekConnect->combinVar.addOffset + combineCount, pHmiVar->addOffset + varCount );
                newLen = 4 * ( endOffset - startOffset );
                if ( newLen <= MAX_FATEK_PACKET_LEN * 2 )  //最多组合 MAX_FATEK_PACKET_LEN 字
                {
                    if ( newLen <= 4*( combineCount + varCount ) + 12 ) //能节省12字节
                    {
                        fatekConnect->combinVar.addOffset = startOffset;
                        fatekConnect->combinVar.len = newLen;
                        fatekConnect->combinVarsList.append( pHmiVar );
                    }
                }
                else
                    continue;
            }
            else if ( fatekConnect->combinVar.area >= FATEK_AREA_TMR && fatekConnect->combinVar.area <= FATEK_AREA_WS )
            {
                int combineCount = ( fatekConnect->combinVar.len + 1 ) / 2;
                int varCount = 0;

                if ( pHmiVar->dataType == Com_Bool )
                {
                    Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
                    varCount = ( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16;
                }
                else
                {
                    varCount = ( pHmiVar->len + 1 ) / 2;
                }

                endOffset = qMax( fatekConnect->combinVar.addOffset + combineCount, pHmiVar->addOffset + varCount );
                newLen = 2 * ( endOffset - startOffset );
                if ( newLen <= MAX_FATEK_PACKET_LEN * 2 )  //最多组合 MAX_FATEK_PACKET_LEN 字
                {
                    if ( newLen <= 2*( combineCount + varCount ) + 10 ) //能节省10字节
                    {
                        fatekConnect->combinVar.addOffset = startOffset;
                        fatekConnect->combinVar.len = newLen;
                        fatekConnect->combinVarsList.append( pHmiVar );
                    }
                }
                else
                {
                    continue;
                }
            }
            else
            {
                endOffset = qMax( fatekConnect->combinVar.addOffset + fatekConnect->combinVar.len, pHmiVar->addOffset + pHmiVar->len );
                int newLen = endOffset - startOffset;
                if ( newLen <= MAX_FATEK_PACKET_LEN )
                {
                    if ( newLen <= fatekConnect->combinVar.len + pHmiVar->len + 8 ) //能节省8字节
                    {
                        fatekConnect->combinVar.addOffset = startOffset;
                        fatekConnect->combinVar.len = newLen;
                        fatekConnect->combinVarsList.append( pHmiVar );
                    }
                }
                else
                {
                    continue;
                }
            }
        }
    }
}

/******************************************************************************
** 函  数  名 ： hasProcessingReq
** 说      明 : 是否正有数据在进行读或者写
** 输      入 ： struct FatekConnect*
** 返      回 ： 操作状态
******************************************************************************/
bool FatekService::hasProcessingReq( struct FatekConnect* fatekConnect )
{
    return ( fatekConnect->writingCnt > 0 || fatekConnect->requestVarsList.count() > 0 );
}

/******************************************************************************
** 函  数  名 ： initCommDev
** 说      明 : 初始化设备
** 输      入 ： void
** 返      回 ： 操作状态
******************************************************************************/
bool FatekService::initCommDev()
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
        struct FatekConnect* conn = m_connectList[0];
#ifdef MINTERFACE_2
//        if ( conn->cfg.COM_interface > 1 )
//        {
//            qDebug() << "no such COM_interface";
//            return false;
//        }
#endif
        if ( m_rwSocket == NULL )
        {
            struct UartCfg uartCfg;
            uartCfg.COMNum = conn->cfg.COM_interface;
            uartCfg.baud = conn->cfg.baud;
            uartCfg.dataBit = conn->cfg.dataBit;
            uartCfg.stopBit = conn->cfg.stopBit;
            uartCfg.parity = conn->cfg.parity;

            m_rwSocket = new RWSocket(FATEK, &uartCfg, conn->cfg.id);
            connect(m_rwSocket, SIGNAL(readyRead()),this,SLOT(slot_readData()));
            m_rwSocket->openDev();
        }
    }
    return true;
}

/******************************************************************************
** 函  数  名 ： processRspData
** 说      明 : 读取驱动，成功读取后，根据都会来MSG的内容进行相应的处理
** 输      入 ： QHash<int, struct ComMsg> &
** 返      回 ： 操作状态
******************************************************************************/
bool FatekService::processRspData( struct FatekMsgPar msgPar ,struct ComMsg comMsg )
{
    bool result = true;

    if ( m_devUseCnt > 0 )
    {
        m_devUseCnt--;
    }
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct FatekConnect* fatekConnect = m_connectList[i];
        Q_ASSERT( fatekConnect );
        if ( fatekConnect->cfg.cpuAddr == msgPar.slave ) //属于这个连接
        {
            if ( fatekConnect->writingCnt > 0 && fatekConnect->waitWrtieVarsList.count() > 0 && msgPar.rw == 1 )
            {                                     // 处理写请求
                Q_ASSERT( msgPar.rw == 1 );
                fatekConnect->writingCnt = 0;
                struct HmiVar* pHmiVar = fatekConnect->waitWrtieVarsList.takeFirst(); //删除写请求记录
                m_mutexRw.lock();
                pHmiVar->wStatus = msgPar.err;
                m_mutexRw.unlock();
                comMsg.varId = ( pHmiVar->varId & 0x0000ffff );
                comMsg.type = 1;
                comMsg.err = msgPar.err;
                m_comMsgTmp.insertMulti( comMsg.varId, comMsg );
            }
            else if ( fatekConnect->requestVarsList.count() > 0 && msgPar.rw == 0 )
            {
                Q_ASSERT( msgPar.rw == 0 );
                Q_ASSERT( msgPar.area == fatekConnect->requestVar.area );
                if ( msgPar.err == VAR_PARAM_ERROR && fatekConnect->requestVarsList.count() > 1 )
                {
                    for ( int j = 0; j < fatekConnect->requestVarsList.count(); j++ )
                    {
                        struct HmiVar* pHmiVar = fatekConnect->requestVarsList[j];
                        pHmiVar->overFlow = true;
                        pHmiVar->lastUpdateTime = -1; //合并后越界的不算越界，立即再次更新（更新时不合并）
                    }
                }
                else
                {
                    for ( int j = 0; j < fatekConnect->requestVarsList.count(); j++ )
                    {
                        m_mutexRw.lock();
                        struct HmiVar* pHmiVar = fatekConnect->requestVarsList[j];
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
                            if ( msgPar.area == FATEK_AREA_CTR200 )
                            {
                                Q_ASSERT( msgPar.count <= MAX_FATEK_PACKET_LEN );
                                int offset = pHmiVar->addOffset - fatekConnect->requestVar.addOffset;
                                Q_ASSERT( offset >= 0 && offset*2 <= MAX_FATEK_PACKET_LEN );
                                if ( pHmiVar->dataType == Com_Bool )
                                {
                                    int len = 4 * (( pHmiVar->bitOffset + pHmiVar->len + 31 ) / 32 );
                                    if ( pHmiVar->rData.size() < len )
                                    {
                                        pHmiVar->rData = QByteArray( len, 0 ); //初始化为0
                                    }
                                    if ( 4*offset + len <= MAX_FATEK_PACKET_LEN * 2 )
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
                                    {
                                        pHmiVar->rData = QByteArray( len, 0 ); //初始化为0
                                    }
                                    if ( 4*offset + len <= MAX_FATEK_PACKET_LEN * 2 )
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
                                    {
                                        Q_ASSERT( false );
                                    }
                                }
                            }
                            else if ( msgPar.area >= FATEK_AREA_TMR && msgPar.area <= FATEK_AREA_WS )
                            {
                                Q_ASSERT( msgPar.count <= MAX_FATEK_PACKET_LEN );
                                int offset = pHmiVar->addOffset - fatekConnect->requestVar.addOffset;
                                Q_ASSERT( offset >= 0 );
                                Q_ASSERT( offset <= MAX_FATEK_PACKET_LEN );
                                if ( pHmiVar->dataType == Com_Bool )
                                {
                                    int len = 2 * (( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16 );
                                    if ( pHmiVar->rData.size() < len )
                                    {
                                        pHmiVar->rData = QByteArray( len, 0 ); //初始化为0
                                    }
                                    if ( 2*offset + len <= MAX_FATEK_PACKET_LEN * 2 )
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
                                    {
                                        Q_ASSERT( false );
                                    }
                                }
                                else
                                {
                                    int len = 2 * (( pHmiVar->len + 1 ) / 2 );
                                    if ( pHmiVar->rData.size() < len )
                                    {
                                        pHmiVar->rData = QByteArray( len, 0 ); //初始化为0
                                    }
                                    if ( 2 * offset + len <= MAX_FATEK_PACKET_LEN * 2 ) //保证不越界
                                    {
                                        memcpy( pHmiVar->rData.data(), &msgPar.data[2 * offset], len );
                                        if ( pHmiVar->wStatus != VAR_NO_OPERATION )
                                        {
                                            comMsg.varId = ( pHmiVar->varId & 0x0000ffff );
                                            comMsg.type = 0;
                                            comMsg.err = VAR_NO_ERR;
                                            m_comMsgTmp.insert( comMsg.varId, comMsg );
                                        }
                                    }
                                    else
                                    {
                                        Q_ASSERT( false );
                                    }
                                }
                            }
                            else
                            {
                                Q_ASSERT( msgPar.count <= MAX_FATEK_PACKET_LEN );
                                if ( !fatekConnect->waitWrtieVarsList.contains( pHmiVar ) )
                                {
                                    int startBitOffset = pHmiVar->addOffset - fatekConnect->requestVar.addOffset;
                                    Q_ASSERT( startBitOffset >= 0 );
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
                                foreach( HmiVar* tempHmiVar, fatekConnect->actVarsList )
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
                fatekConnect->requestVarsList.clear();
            }
            if ( msgPar.err == VAR_COMM_TIMEOUT )
            {
                fatekConnect->connStatus = CONNECT_NO_LINK;
            }
            else
            {
                fatekConnect->connStatus = CONNECT_LINKED_OK;
            }
            break;
        }
    }
    return result;
}

/******************************************************************************
** 函  数  名 ： run
** 说      明 : QT多线程固定函数，线程Start后进入run（）处理
** 输      入 ： void
** 返      回 ： 操作状态
******************************************************************************/
void FatekService::slot_run()
{
    initCommDev();
    m_runTimer->start(10);
}

void FatekService::slot_readData()
{
    struct FatekMsgPar msgPar;
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
                struct FatekConnect* fatekConnect = m_connectList[i];
                Q_ASSERT( fatekConnect );
                fatekConnect->waitWrtieVarsList.clear();
                fatekConnect->requestVarsList.clear();
                foreach( HmiVar* tempHmiVar, fatekConnect->actVarsList )
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
    while ( ret == sizeof( struct FatekMsgPar ) );

}

void FatekService::slot_runTimerTimeout()
{
    m_runTimer->stop();
    int lastUpdateTime = -1;

    int connectCount = m_connectList.count();
    m_currTime = getCurrTime();
    int currTime = m_currTime;

    for ( int i = 0; i < connectCount; i++ )
    {                              //处理写请求
        struct FatekConnect* fatekConnect = m_connectList[i];
        updateWriteReq( fatekConnect );
        trySndWriteReq( fatekConnect );
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
    //bool hasProcessingReqFlag = false;
    for ( int i = 0; i < connectCount; i++ )
    {
        struct FatekConnect* fatekConnect = m_connectList[i];
        if ( needUpdateFlag )
        {
            updateOnceRealReadReq( fatekConnect );  //先重组一次更新变量
            updateReadReq( fatekConnect ); //无论如何都要重组请求变量
        }
        if ( !( hasProcessingReq( fatekConnect ) ) )
        {
            combineReadReq( fatekConnect );
        }
        trySndReadReq( fatekConnect );
    }
    m_runTimer->start(20);


}
