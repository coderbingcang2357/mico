
/**********************************************************************
Copyright (c) 2008-2010 by SHENZHEN CO-TRUST AUTOMATION TECH.
** 文 件 名:  dvpService.cpp
** 说    明:  台达DVP协议多协议平台实现文件，完成后台台达DVP协议数据的处理以及链接，变量，读写状态的管理
** 版    本:  【可选】
** 创建日期:  2012.06.05
** 创 建 人:  陶健军
** 修改信息： 【可选】
**         修改人    修改日期       修改内容**
*********************************************************************/
#include "dvpService.h"


#define MAX_DVP_PACKET_LEN  50  //最大一包字数，因为驱动组包大小只有255字节，超过57个字会导致 写常数组的时候出错

/******************************************************************************
** 函  数  名 ： DvpService
** 说      明 :  构造函数
** 输      入 ： 链接参数
** 返      回 ： NONE
******************************************************************************/
DvpService::DvpService( struct DvpConnectCfg& connectCfg )
{
    struct DvpConnect* connect = new DvpConnect;
    connect->cfg = connectCfg;
    connect->writingCnt = 0;
    connect->connStatus = CONNECT_NO_OPERATION;
    connect->connectTestVar.lastUpdateTime = -1;

    m_connectList.append( connect );
    m_currTime = getCurrTime();
    m_runTimer = NULL;
    m_rwSocket = NULL;
}

DvpService::~DvpService()
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
** 说      明 :  判断是否存在DVP类型的链接
** 输      入 ： 链接类型
        串口端口号
** 返      回 ： 存在与否标志
******************************************************************************/
bool DvpService::meetServiceCfg( int serviceType, int COM_interface )
{
    if ( serviceType == SERVICE_TYPE_DVP )
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
int DvpService::getConnLinkStatus( int connectId )
{
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct DvpConnect* dvpConnect = m_connectList[i];
        if ( connectId == dvpConnect->cfg.id )
        {
            return dvpConnect->connStatus;
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
int DvpService::getComMsg( QList<struct ComMsg> &msgList )
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
int DvpService::splitLongVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );

    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct DvpConnect* dvpConnect = m_connectList[i];
        if ( pHmiVar->connectId == dvpConnect->cfg.id )
        {
            pHmiVar->lastUpdateTime = -1; //立即更新
            if ( pHmiVar->area == DVP_AREA_D )
            {
                if ( pHmiVar->dataType == Com_Bool )
                {
                    if ( pHmiVar->len > MAX_DVP_PACKET_LEN*16 && pHmiVar->splitVars.count() <= 0 )  //拆分
                    {
                        for ( int splitedLen = 0; splitedLen < pHmiVar->len; )
                        {
                            int len = pHmiVar->len - splitedLen;
                            if ( len > MAX_DVP_PACKET_LEN * 16 )
                            {
                                len =  MAX_DVP_PACKET_LEN * 16;
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
                            pSplitHhiVar->wStatus = VAR_NO_ERR;
                            pSplitHhiVar->lastUpdateTime = -1; //立即更新

                            splitedLen += len;
                            pHmiVar->splitVars.append( pSplitHhiVar );
                        }
                    }
                }
                else
                {
                    if ( pHmiVar->len > MAX_DVP_PACKET_LEN*2 && pHmiVar->splitVars.count() <= 0 )  //拆分
                    {
                        for ( int splitedLen = 0; splitedLen < pHmiVar->len; )
                        {
                            int len = pHmiVar->len - splitedLen;
                            if ( len > MAX_DVP_PACKET_LEN*2 )
                            {
                                len  = MAX_DVP_PACKET_LEN * 2;
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
                            pSplitHhiVar->wStatus = VAR_NO_ERR;
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
int DvpService::onceRealUpdateVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    m_mutexOnceReal.lock();
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct DvpConnect* dvpConnect = m_connectList[i];
        if ( pHmiVar->connectId == dvpConnect->cfg.id )
        {
            pHmiVar->lastUpdateTime = -1;
            if ( pHmiVar->splitVars.count() > 0 )
            {
                for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
                {
                    struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                    pSplitHhiVar->lastUpdateTime = -1;
                    pSplitHhiVar->overFlow = false;
                    if ( !dvpConnect->onceRealVarsList.contains( pSplitHhiVar ) )
                    {
                        dvpConnect->onceRealVarsList.append( pSplitHhiVar );
                    }
                }
            }
            else
            {
                pHmiVar->lastUpdateTime = -1;
                pHmiVar->overFlow = false;
                if ( !dvpConnect->onceRealVarsList.contains( pHmiVar ) )
                {
                    dvpConnect->onceRealVarsList.append( pHmiVar );
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
int DvpService::startUpdateVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    m_mutexUpdateVar.lock();
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct DvpConnect* dvpConnect = m_connectList[i];
        if ( pHmiVar->connectId == dvpConnect->cfg.id )
        {
            pHmiVar->lastUpdateTime = -1;
            if ( pHmiVar->splitVars.count() > 0 )
            {
                for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
                {
                    struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                    pSplitHhiVar->lastUpdateTime = -1;
                    pSplitHhiVar->overFlow = false;
                    if ( !dvpConnect->actVarsList.contains( pSplitHhiVar ) )
                    {
                        dvpConnect->actVarsList.append( pSplitHhiVar );
                    }
                }
            }
            else
            {
                pHmiVar->lastUpdateTime = -1;
                pHmiVar->overFlow = false;
                if ( pHmiVar->addOffset >= 0 )
                {
                    if ( !dvpConnect->actVarsList.contains( pHmiVar ) )
                    {
                        dvpConnect->actVarsList.append( pHmiVar );
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
int DvpService::stopUpdateVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    m_mutexUpdateVar.lock();
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct DvpConnect* dvpConnect = m_connectList[i];
        if ( pHmiVar->connectId == dvpConnect->cfg.id )
        {
            if ( pHmiVar->splitVars.count() > 0 )
            {
                for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
                {
                    struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                    dvpConnect->actVarsList.removeAll( pSplitHhiVar );
                }
            }
            else
            {
                dvpConnect->actVarsList.removeAll( pHmiVar );
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
int DvpService::readVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    if ( pHmiVar->err == VAR_NO_ERR )  //无错误才拷贝数据
    {
        if ( pHmiVar->area == DVP_AREA_CV200 )
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
        else if ( pHmiVar->area == DVP_AREA_D || pHmiVar->area == DVP_AREA_TV || pHmiVar->area == DVP_AREA_CV )
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
                        if ( pHmiVar->dataType != Com_PointArea ) //奇偶字节调换
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
int DvpService::readDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    Q_ASSERT( pHmiVar && pBuf );
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct DvpConnect* dvpConnect = m_connectList[i];
        if ( pHmiVar->connectId == dvpConnect->cfg.id )
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
int DvpService::writeVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    int ret;
    Q_ASSERT( pHmiVar && pBuf );
    m_mutexRw.lock();
    if ( pHmiVar->area == DVP_AREA_CV200 )
    {//大小数等到发送写命令时再处理
        if ( pHmiVar->dataType == Com_Bool )
        {                        //位变量长度为位的个数
            Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 31 );
            int count = ( pHmiVar->bitOffset + pHmiVar->len + 31 ) / 32;  //双字数
            Q_ASSERT( count > 0 && count <= MAX_DVP_PACKET_LEN );
            if ( count > MAX_DVP_PACKET_LEN )
            {
                count = MAX_DVP_PACKET_LEN;
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
            Q_ASSERT( count > 0 && count <= MAX_DVP_PACKET_LEN );
            if ( count > MAX_DVP_PACKET_LEN )
            {
                count = MAX_DVP_PACKET_LEN;
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
    else if ( pHmiVar->area == DVP_AREA_D || pHmiVar->area == DVP_AREA_TV || pHmiVar->area == DVP_AREA_CV )
    {                            //大小数等到发送写命令时再处理
        if ( pHmiVar->dataType == Com_Bool )
        {                        //位变量长度为位的个数
            Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
            int count = ( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16;  //字数
            Q_ASSERT( count > 0 && count <= MAX_DVP_PACKET_LEN );
            if ( count > MAX_DVP_PACKET_LEN )
            {
                count = MAX_DVP_PACKET_LEN;
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
            Q_ASSERT( count > 0 && count <= MAX_DVP_PACKET_LEN );
            if ( count > MAX_DVP_PACKET_LEN )
            {
                count = MAX_DVP_PACKET_LEN;
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
                    if ( pHmiVar->dataType != Com_PointArea )
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
        Q_ASSERT( count > 0 &&  count <= MAX_DVP_PACKET_LEN );
        if ( count > MAX_DVP_PACKET_LEN )
        {
            count = MAX_DVP_PACKET_LEN;
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
int DvpService::writeDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    Q_ASSERT( pHmiVar && pBuf );

    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct DvpConnect* dvpConnect = m_connectList[i];
        if ( pHmiVar->connectId == dvpConnect->cfg.id )
        {
            int ret;
            if ( dvpConnect->connStatus == CONNECT_NO_LINK )//if this var has read and write err, do not execute write, until it is read or wriet correct.
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
                        dvpConnect->waitWrtieVarsList.append( pSplitHhiVar );
                        pBuf += len;
                        bufLen -= len;
                    }
                }
                else
                {
                    ret = writeVar( pHmiVar, pBuf, bufLen );
                    dvpConnect->waitWrtieVarsList.append( pHmiVar );
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
** 输      入 ： struct DvpConnectCfg
** 返      回 ： 操作状态
******************************************************************************/
int DvpService::addConnect( struct DvpConnectCfg& connectCfg )
{                                      //要检查是否重复 add
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct DvpConnect* dvpConnect = m_connectList[i];
        Q_ASSERT( dvpConnect->cfg.COM_interface == connectCfg.COM_interface );//绝对要是同一个设备接口
        if ( connectCfg.id == dvpConnect->cfg.id )
        {
            Q_ASSERT( false );
            return 0;
        }
    }
    struct DvpConnect* dvpConnect = new DvpConnect;
    dvpConnect->cfg = connectCfg;
    dvpConnect->writingCnt = 0;
    dvpConnect->connStatus = CONNECT_NO_OPERATION;
    dvpConnect->connectTestVar.lastUpdateTime = -1;
    m_connectList.append( dvpConnect );
    return 0;
}

/******************************************************************************
** 函  数  名 ： sndWriteVarMsgToDriver
** 说      明 : 把写缓冲区数据打包到 struct DvpMsgPar中，然后写入到驱动
** 输      入 ： struct DvpConnect*
        struct HmiVar*
** 返      回 ： 操作状态
******************************************************************************/
int DvpService::sndWriteVarMsgToDriver( struct DvpConnect* dvpConnect, struct HmiVar* pHmiVar )
{
    int sendedLen = 0;
    Q_ASSERT( dvpConnect && pHmiVar );
//#ifdef Q_WS_QWS
    struct DvpMsgPar msgPar;

    m_mutexRw.lock();
    msgPar.rw = 1;   //写
    msgPar.slave    = dvpConnect->cfg.cpuAddr;
    msgPar.area     = pHmiVar->area;
    msgPar.addr     = pHmiVar->addOffset;
    msgPar.err = 0;

    if ( pHmiVar->area == DVP_AREA_CV200 )
    {
        if ( pHmiVar->dataType == Com_Bool )
        {                              // 位变量长度为位的个数
            Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 31 );
            int count = ( pHmiVar->bitOffset + pHmiVar->len + 31 ) / 32;  //双字数
            Q_ASSERT( count > 0 && count*2 <= MAX_DVP_PACKET_LEN );
            if ( count*2 > MAX_DVP_PACKET_LEN )
            {
                count = ( MAX_DVP_PACKET_LEN / 2 );
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
            Q_ASSERT( count > 0 && count*2 <= MAX_DVP_PACKET_LEN );
            if ( count*2 > MAX_DVP_PACKET_LEN )
            {
                count = ( MAX_DVP_PACKET_LEN / 2 );
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
    else if ( msgPar.area == DVP_AREA_D || msgPar.area == DVP_AREA_TV || pHmiVar->area == DVP_AREA_CV )
    {
        if ( pHmiVar->dataType == Com_Bool )
        {                              // 位变量长度为位的个数
            Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
            int count = ( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16;  //字数
            Q_ASSERT( count > 0 && count <= MAX_DVP_PACKET_LEN );
            if ( count > MAX_DVP_PACKET_LEN )
            {
                count = MAX_DVP_PACKET_LEN;
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
            Q_ASSERT( count > 0 && count <= MAX_DVP_PACKET_LEN );
            if ( count > MAX_DVP_PACKET_LEN )
            {
                count = MAX_DVP_PACKET_LEN;
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
                    if ( pHmiVar->dataType != Com_PointArea )
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
    else if ( msgPar.area == DVP_AREA_M || msgPar.area == DVP_AREA_X || msgPar.area == DVP_AREA_Y ||
                    msgPar.area == DVP_AREA_S || msgPar.area == DVP_AREA_T || msgPar.area == DVP_AREA_C )
    {                                  // 位变量长度为位的个数
        Q_ASSERT( pHmiVar->dataType == Com_Bool );
        int count = pHmiVar->len;
        Q_ASSERT( count > 0 &&  count <= (( MAX_DVP_PACKET_LEN - 1 )*16 ) );
        if ( count > (( MAX_DVP_PACKET_LEN - 1 )*16 ) )
        {
            count = ( MAX_DVP_PACKET_LEN - 1 ) * 16;
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
        //int len = write( m_fd, ( char* ) & msgPar, sizeof( struct DvpMsgPar ) );
        int len = m_rwSocket->writeDev(( char* ) & msgPar, sizeof( struct DvpMsgPar ));
        Q_ASSERT( len > 0 );
    }
    else
        m_mutexRw.unlock();
//#endif
    return sendedLen;
}

/******************************************************************************
** 函  数  名 ： sndReadVarMsgToDriver
** 说      明 : 把读缓冲区数据打包到 struct DvpMsgPar中，然后写入到驱动
** 输      入 ： struct DvpConnect*
        struct HmiVar*
** 返      回 ： 操作状态
******************************************************************************/
int DvpService::sndReadVarMsgToDriver( struct DvpConnect* dvpConnect, struct HmiVar* pHmiVar )
{//printf("sndReadVarMsgToDriver\n");
    int sendedLen = 0;
    Q_ASSERT( dvpConnect && pHmiVar );
//#ifdef Q_WS_QWS
    struct DvpMsgPar msgPar;

    m_mutexRw.lock();
    msgPar.rw = 0;    // 读
    msgPar.slave = dvpConnect->cfg.cpuAddr;
    msgPar.area = pHmiVar->area;
    msgPar.addr = pHmiVar->addOffset;
    msgPar.err = 0;
    if ( pHmiVar->area == DVP_AREA_CV200 )
    {
        if ( pHmiVar->dataType == Com_Bool )
        {                              // 位变量长度为位的个数
            Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 31 );
            int count = ( pHmiVar->bitOffset + pHmiVar->len + 31 ) / 32;  //字数
            Q_ASSERT( count > 0 && count*2 <= MAX_DVP_PACKET_LEN );
            if ( count*2 > MAX_DVP_PACKET_LEN )
            {
                count = MAX_DVP_PACKET_LEN / 2;
            }
            msgPar.count = count;      // 占用的总双字数到底层驱动要转换成字数
            sendedLen = count * 32;
        }
        else
        {
            int count = ( pHmiVar->len + 3 ) / 4;  //字数
            Q_ASSERT( count > 0 && count*2 <= MAX_DVP_PACKET_LEN );
            if ( count*2 > MAX_DVP_PACKET_LEN )
            {
                count = MAX_DVP_PACKET_LEN / 2;
            }
            msgPar.count = count;      // 占用的总双字数到底层驱动要转换成字数
            sendedLen = count * 4;
        }
    }
    else if ( msgPar.area == DVP_AREA_D || msgPar.area == DVP_AREA_TV || pHmiVar->area == DVP_AREA_CV )
    {
        if ( pHmiVar->dataType == Com_Bool )
        {                              // 位变量长度为位的个数
            Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
            int count = ( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16;  //字数
            Q_ASSERT( count > 0 && count <= MAX_DVP_PACKET_LEN );
            if ( count > MAX_DVP_PACKET_LEN )
            {
                count = MAX_DVP_PACKET_LEN;
            }
            msgPar.count = count;      // 占用的总字数
            sendedLen = count * 16;
        }
        else
        {
            int count = ( pHmiVar->len + 1 ) / 2;  //字数
            Q_ASSERT( count > 0 && count <= MAX_DVP_PACKET_LEN );
            if ( count > MAX_DVP_PACKET_LEN )
            {
                count = MAX_DVP_PACKET_LEN;
            }
            msgPar.count = count;      // 占用的总字数
            sendedLen = count * 4;
        }
    }
    else if ( msgPar.area == DVP_AREA_M || msgPar.area == DVP_AREA_X || msgPar.area == DVP_AREA_Y ||
                    msgPar.area == DVP_AREA_S || msgPar.area == DVP_AREA_T || msgPar.area == DVP_AREA_C )
    {                                  // 位变量长度为位的个数
        Q_ASSERT( pHmiVar->dataType == Com_Bool );
        int count = pHmiVar->len;
        Q_ASSERT( count > 0 &&  count <= (( MAX_DVP_PACKET_LEN - 1 )*16 ) );
        if ( count > (( MAX_DVP_PACKET_LEN - 1 )*16 ) )
        {
            count = (( MAX_DVP_PACKET_LEN - 1 ) * 16 );
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
        //int len = write( m_fd, ( char* ) & msgPar, sizeof( struct DvpMsgPar ) );
        int len = m_rwSocket->writeDev( ( char* ) & msgPar, sizeof( struct DvpMsgPar ));

        Q_ASSERT( len > 0 );
        if ( len<=0 )
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
** 输      入 ： struct DvpConnect*
        struct HmiVar*
** 返      回 ： 操作状态
******************************************************************************/
bool isWaitingReadRsp( struct DvpConnect* dvpConnect, struct HmiVar* pHmiVar )
{
    Q_ASSERT( dvpConnect && pHmiVar );
    if ( dvpConnect->waitUpdateVarsList.contains( pHmiVar ) )
    {
        return true;
    }
    else if ( dvpConnect->combinVarsList.contains( pHmiVar ) )
    {
        return true;
    }
    else if ( dvpConnect->requestVarsList.contains( pHmiVar ) )
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
** 输      入 ： struct DvpConnect*
** 返      回 ： 操作状态
******************************************************************************/
bool DvpService::trySndWriteReq( struct DvpConnect* dvpConnect ) //不处理超长变量
{
    if ( m_devUseCnt < MAXDEVCNT )                          //没有正在读写, snd read, 一次只能发送一包请求
    {
        if ( dvpConnect->writingCnt == 0 && dvpConnect->requestVarsList.count() == 0 ) //这个连接没有读写
        {
            m_mutexWReq.lock();
            if ( dvpConnect->waitWrtieVarsList.count() > 0 )
            {
                struct HmiVar* pHmiVar = dvpConnect->waitWrtieVarsList[0];
                if (( pHmiVar->lastUpdateTime == -1 )  //3X,4X 位变量需要先读
                                && ( pHmiVar->area == DVP_AREA_D || pHmiVar->area == DVP_AREA_TV || pHmiVar->area == DVP_AREA_CV || pHmiVar->area == DVP_AREA_CV200 )
                                && ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_String ) )
                {
                    if ( !isWaitingReadRsp( dvpConnect, pHmiVar ) )
                    {
                        dvpConnect->waitUpdateVarsList.prepend( pHmiVar ); //加入到读队列最前,保证下次最先读
                    }
                }
                else if ( !isWaitingReadRsp( dvpConnect, pHmiVar ) ) //没有正在等待读
                {                                   //直接写,snd write cmd to driver
                    if ( sndWriteVarMsgToDriver( dvpConnect, pHmiVar ) > 0 )
                    {
                        dvpConnect->writingCnt = 1;    //进入写状态
                        m_mutexWReq.unlock();
                        return true;
                    }
                    else
                    {
                        dvpConnect->waitWrtieVarsList.takeAt( 0 );//出列,丢掉写请求
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
** 输      入 ： struct DvpConnect*
** 返      回 ： 操作状态
******************************************************************************/
bool DvpService::trySndReadReq( struct DvpConnect* dvpConnect ) //不处理超长变量
{
    if ( m_devUseCnt < MAXDEVCNT )             //没有正在读写, snd read, 一次只能发送一包请求
    {
        if ( dvpConnect->writingCnt == 0 && dvpConnect->requestVarsList.count() == 0 ) //这个连接没有读写
        {
            if ( dvpConnect->combinVarsList.count() > 0 )
            {
                dvpConnect->requestVarsList = dvpConnect->combinVarsList;
                dvpConnect->requestVar = dvpConnect->combinVar;
                for ( int i = 0; i < dvpConnect->combinVarsList.count(); i++ )
                {                      //要把已经请求的变量从等待队列中删除
                    struct HmiVar* pHmiVar = dvpConnect->combinVarsList[i];
                    dvpConnect->waitUpdateVarsList.removeAll( pHmiVar );
                    pHmiVar->lastUpdateTime = m_currTime;
                }
                dvpConnect->combinVarsList.clear();

                if ( sndReadVarMsgToDriver( dvpConnect, &( dvpConnect->requestVar ) ) > 0 )
                {
                    return true;
                }
                else
                {
                    for ( int i = 0; i < dvpConnect->requestVarsList.count(); i++ )
                    {
                        struct HmiVar* pHmiVar = dvpConnect->requestVarsList[i];
                        pHmiVar->err = VAR_PARAM_ERROR;
                    }
                    dvpConnect->requestVarsList.clear();
                }
            }
        }
    }
    return false;
}

/******************************************************************************
** 函  数  名 ： updateWriteReq
** 说      明 : 更新写请求
** 输      入 ： struct DvpConnect*
** 返      回 ： 操作状态
******************************************************************************/
bool DvpService::updateWriteReq( struct DvpConnect* dvpConnect )
{
    bool newUpdateVarFlag = false;
    QList<struct HmiVar*> tmpList;
    m_mutexWReq.lock();

    for ( int i = 0; i < dvpConnect->wrtieVarsList.count(); i++ )
    {
        struct HmiVar* pHmiVar = dvpConnect->wrtieVarsList[i];

        if ( !dvpConnect->waitWrtieVarsList.contains( pHmiVar ) )
        {
            dvpConnect->waitWrtieVarsList.append( pHmiVar );
            newUpdateVarFlag = true;
        }
        else
        {
            tmpList.append(pHmiVar);
        }
    }
    dvpConnect->wrtieVarsList.clear() ;
    dvpConnect->wrtieVarsList += tmpList;
    m_mutexWReq.unlock();
    return newUpdateVarFlag;
}

/******************************************************************************
** 函  数  名 ： updateReadReq
** 说      明 : 更新读请求
** 输      入 ： struct DvpConnect*
** 返      回 ： 操作状态
******************************************************************************/
bool DvpService::updateReadReq( struct DvpConnect* dvpConnect )
{
    bool newUpdateVarFlag = false;
    m_mutexUpdateVar.lock();
    int currTime = m_currTime;//getCurrTime();
    if ( dvpConnect->connStatus == CONNECT_LINKED_OK )
    {
        for ( int i = 0; i < dvpConnect->actVarsList.count(); i++ )
        {                                  // 找出需要刷新的变量
            struct HmiVar* pHmiVar = dvpConnect->actVarsList[i];
            if (( pHmiVar->lastUpdateTime == -1 ) || ( calIntervalTime( pHmiVar->lastUpdateTime, currTime ) >= pHmiVar->cycleTime ) )
            {                              // 需要刷新
                if (( !dvpConnect->waitUpdateVarsList.contains( pHmiVar ) )
                                && ( !dvpConnect->waitWrtieVarsList.contains( pHmiVar ) )
                                && ( !dvpConnect->requestVarsList.contains( pHmiVar ) ) )
                {
                    dvpConnect->waitUpdateVarsList.append( pHmiVar );
                    newUpdateVarFlag = true;
                }
            }
        }
    }
    else
    {
        dvpConnect->waitUpdateVarsList.clear();
        if ( dvpConnect->actVarsList.count() > 0  && dvpConnect->requestVarsList.count() == 0 )
        {
            dvpConnect->actVarsList[0]->lastUpdateTime = dvpConnect->connectTestVar.lastUpdateTime;
            dvpConnect->connectTestVar = *( dvpConnect->actVarsList[0] );
            dvpConnect->connectTestVar.varId = 65535;
            dvpConnect->connectTestVar.cycleTime = 2000;
            if (( dvpConnect->connectTestVar.lastUpdateTime == -1 )
                            || ( calIntervalTime( dvpConnect->connectTestVar.lastUpdateTime, currTime ) >= dvpConnect->connectTestVar.cycleTime ) )
            {
                if (( !dvpConnect->waitUpdateVarsList.contains( &( dvpConnect->connectTestVar ) ) )
                                && ( !dvpConnect->waitWrtieVarsList.contains( &( dvpConnect->connectTestVar ) ) )
                                && ( !dvpConnect->requestVarsList.contains( &( dvpConnect->connectTestVar ) ) ) )
                {
                    dvpConnect->waitUpdateVarsList.append( &( dvpConnect->connectTestVar ) );
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
** 输      入 ： struct DvpConnect*
** 返      回 ： 操作状态
******************************************************************************/
bool DvpService::updateOnceRealReadReq( struct DvpConnect* dvpConnect )
{
    bool newUpdateVarFlag = false;
    m_mutexOnceReal.lock();
    for ( int i = 0; i < dvpConnect->onceRealVarsList.count(); i++ )
    {
        struct HmiVar* pHmiVar = dvpConnect->onceRealVarsList[i];
        if (( !dvpConnect->waitUpdateVarsList.contains( pHmiVar ) )
                        && ( !dvpConnect->requestVarsList.contains( pHmiVar ) ) )
        {
            dvpConnect->waitUpdateVarsList.prepend( pHmiVar );
            newUpdateVarFlag = true;
        }
        else
        {
            if ( dvpConnect->waitUpdateVarsList.contains( pHmiVar ) )
            {
                dvpConnect->waitUpdateVarsList.removeAll( pHmiVar );
                dvpConnect->waitUpdateVarsList.prepend( pHmiVar );
            }
        }
    }
    dvpConnect->onceRealVarsList.clear();
    m_mutexOnceReal.unlock();

    return newUpdateVarFlag;
}

/******************************************************************************
** 函  数  名 ： combineReadReq
** 说      明 : 把相同区域的 可组合的变量组合再一起，可以提高通讯速度
** 输      入 ： struct DvpConnect*
** 返      回 ： 操作状态
******************************************************************************/
void DvpService::combineReadReq( struct DvpConnect* dvpConnect )
{//printf("combineReadReq\n");
    dvpConnect->combinVarsList.clear();
    if ( dvpConnect->waitUpdateVarsList.count() >= 1 )
    {
        struct HmiVar* pHmiVar = dvpConnect->waitUpdateVarsList[0];
        dvpConnect->combinVarsList.append( pHmiVar );
        dvpConnect->combinVar = *pHmiVar;
        if ( dvpConnect->combinVar.area == DVP_AREA_D || dvpConnect->combinVar.area == DVP_AREA_TV || dvpConnect->combinVar.area == DVP_AREA_CV )
        {
            if ( pHmiVar->dataType == Com_Bool )
            {
                Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
                dvpConnect->combinVar.dataType = Com_Word;
                dvpConnect->combinVar.len = 2 * (( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16 ); //2*count
                dvpConnect->combinVar.bitOffset = -1;
            }
        }
        else if ( dvpConnect->combinVar.area == DVP_AREA_CV200 )
        {
            if ( pHmiVar->dataType == Com_Bool )
            {
                Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 31 );
                dvpConnect->combinVar.dataType = Com_DWord;
                dvpConnect->combinVar.len = 4 * (( pHmiVar->bitOffset + pHmiVar->len + 31 ) / 32 ); //4*count
                dvpConnect->combinVar.bitOffset = -1;
            }
        }
    }
    for ( int i = 1; i < dvpConnect->waitUpdateVarsList.count(); i++ )
    {
        struct HmiVar* pHmiVar = dvpConnect->waitUpdateVarsList[i];
        if ( pHmiVar->area == dvpConnect->combinVar.area && dvpConnect->combinVar.overFlow == pHmiVar->overFlow && ( pHmiVar->overFlow == false ) )
        {                            //没有越界才合并，否则不合并
            int startOffset = qMin( dvpConnect->combinVar.addOffset, pHmiVar->addOffset );
            int endOffset = 0;
            int newLen = 0;
            if ( dvpConnect->combinVar.area == DVP_AREA_CV200 )
            {
                int combineCount = ( dvpConnect->combinVar.len + 3 ) / 4;
                int varCount = 0;

                if ( pHmiVar->dataType == Com_Bool )
                {
                    Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 31 );
                    varCount = ( pHmiVar->bitOffset + pHmiVar->len + 31 ) / 32;
                }
                else
                    varCount = ( pHmiVar->len + 3 ) / 4;

                endOffset = qMax( dvpConnect->combinVar.addOffset + combineCount, pHmiVar->addOffset + varCount );
                newLen = 4 * ( endOffset - startOffset );
                if ( newLen <= MAX_DVP_PACKET_LEN * 2 )  //最多组合 MAX_DVP_PACKET_LEN 字
                {
                    if ( newLen <= 4*( combineCount + varCount ) + 12 ) //能节省12字节
                    {
                        dvpConnect->combinVar.addOffset = startOffset;
                        dvpConnect->combinVar.len = newLen;
                        dvpConnect->combinVarsList.append( pHmiVar );
                    }
                }
                else
                    continue;
            }
            else if ( dvpConnect->combinVar.area == DVP_AREA_D || dvpConnect->combinVar.area == DVP_AREA_TV || dvpConnect->combinVar.area == DVP_AREA_CV )
            {
                int combineCount = ( dvpConnect->combinVar.len + 1 ) / 2;
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

                endOffset = qMax( dvpConnect->combinVar.addOffset + combineCount, pHmiVar->addOffset + varCount );
                newLen = 2 * ( endOffset - startOffset );
                if ( newLen <= MAX_DVP_PACKET_LEN * 2 )  //最多组合 MAX_DVP_PACKET_LEN 字
                {
                    if ( newLen <= 2*( combineCount + varCount ) + 10 ) //能节省10字节
                    {
                        dvpConnect->combinVar.addOffset = startOffset;
                        dvpConnect->combinVar.len = newLen;
                        dvpConnect->combinVarsList.append( pHmiVar );
                    }
                }
                else
                {
                    continue;
                }
            }
            else
            {
                endOffset = qMax( dvpConnect->combinVar.addOffset + dvpConnect->combinVar.len, pHmiVar->addOffset + pHmiVar->len );
                int newLen = endOffset - startOffset;
                if ( newLen <= MAX_DVP_PACKET_LEN )
                {
                    if ( newLen <= dvpConnect->combinVar.len + pHmiVar->len + 8 ) //能节省8字节
                    {
                        dvpConnect->combinVar.addOffset = startOffset;
                        dvpConnect->combinVar.len = newLen;
                        dvpConnect->combinVarsList.append( pHmiVar );
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
** 输      入 ： struct DvpConnect*
** 返      回 ： 操作状态
******************************************************************************/
bool DvpService::hasProcessingReq( struct DvpConnect* dvpConnect )
{
    return ( dvpConnect->writingCnt > 0 || dvpConnect->requestVarsList.count() > 0 );
}

/******************************************************************************
** 函  数  名 ： initCommDev
** 说      明 : 初始化设备
** 输      入 ： void
** 返      回 ： 操作状态
******************************************************************************/
bool DvpService::initCommDev()
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
        struct DvpConnect* conn = m_connectList[0];
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

            m_rwSocket = new RWSocket(DVP, &uartCfg, conn->cfg.id);
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
bool DvpService::processRspData( struct DvpMsgPar msgPar ,struct ComMsg comMsg  )
{
    bool result = true;

    if ( m_devUseCnt > 0 )
    {
        m_devUseCnt--;
    }
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct DvpConnect* dvpConnect = m_connectList[i];
        Q_ASSERT( dvpConnect );
        if ( dvpConnect->cfg.cpuAddr == msgPar.slave ) //属于这个连接
        {
            if ( dvpConnect->writingCnt > 0 && dvpConnect->waitWrtieVarsList.count() > 0 && msgPar.rw == 1 )
            {                                     // 处理写请求
                Q_ASSERT( msgPar.rw == 1 );
                dvpConnect->writingCnt = 0;
                struct HmiVar* pHmiVar = dvpConnect->waitWrtieVarsList.takeFirst(); //删除写请求记录
                m_mutexRw.lock();
                pHmiVar->wStatus = msgPar.err;
                m_mutexRw.unlock();
                comMsg.varId = ( pHmiVar->varId & 0x0000ffff );
                comMsg.type = 1;
                comMsg.err = msgPar.err;
                m_comMsgTmp.insertMulti( comMsg.varId, comMsg );
            }
            else if ( dvpConnect->requestVarsList.count() > 0 && msgPar.rw == 0 )
            {
                Q_ASSERT( msgPar.rw == 0 );
                Q_ASSERT( msgPar.area == dvpConnect->requestVar.area );
                if ( msgPar.err == VAR_PARAM_ERROR && dvpConnect->requestVarsList.count() > 1 )
                {
                    for ( int j = 0; j < dvpConnect->requestVarsList.count(); j++ )
                    {
                        struct HmiVar* pHmiVar = dvpConnect->requestVarsList[j];
                        pHmiVar->overFlow = true;
                        pHmiVar->lastUpdateTime = -1; //合并后越界的不算越界，立即再次更新（更新时不合并）
                    }
                }
                else
                {
                    for ( int j = 0; j < dvpConnect->requestVarsList.count(); j++ )
                    {
                        m_mutexRw.lock();
                        struct HmiVar* pHmiVar = dvpConnect->requestVarsList[j];
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
                            if ( msgPar.area == DVP_AREA_CV200 )
                            {
                                Q_ASSERT( msgPar.count <= MAX_DVP_PACKET_LEN );
                                int offset = pHmiVar->addOffset - dvpConnect->requestVar.addOffset;
                                Q_ASSERT( offset >= 0 && offset*2 <= MAX_DVP_PACKET_LEN );
                                if ( pHmiVar->dataType == Com_Bool )
                                {
                                    int len = 4 * (( pHmiVar->bitOffset + pHmiVar->len + 31 ) / 32 );
                                    if ( pHmiVar->rData.size() < len )
                                    {
                                        pHmiVar->rData = QByteArray( len, 0 ); //初始化为0
                                    }
                                    if ( 4*offset + len <= MAX_DVP_PACKET_LEN * 2 )
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
                                    if ( 4*offset + len <= MAX_DVP_PACKET_LEN * 2 )
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
                            else if ( msgPar.area == DVP_AREA_D || msgPar.area == DVP_AREA_TV || msgPar.area == DVP_AREA_CV )
                            {
                                Q_ASSERT( msgPar.count <= MAX_DVP_PACKET_LEN );
                                int offset = pHmiVar->addOffset - dvpConnect->requestVar.addOffset;
                                Q_ASSERT( offset >= 0 );
                                Q_ASSERT( offset <= MAX_DVP_PACKET_LEN );
                                if ( pHmiVar->dataType == Com_Bool )
                                {
                                    int len = 2 * (( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16 );
                                    if ( pHmiVar->rData.size() < len )
                                    {
                                        pHmiVar->rData = QByteArray( len, 0 ); //初始化为0
                                    }
                                    if ( 2*offset + len <= MAX_DVP_PACKET_LEN * 2 )
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
                                    if ( 2 * offset + len <= MAX_DVP_PACKET_LEN * 2 ) //保证不越界
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
                                Q_ASSERT( msgPar.count <= MAX_DVP_PACKET_LEN );
                                if ( !dvpConnect->waitWrtieVarsList.contains( pHmiVar ) )
                                {
                                    int startBitOffset = pHmiVar->addOffset - dvpConnect->requestVar.addOffset;
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
                                foreach( HmiVar* tempHmiVar, dvpConnect->actVarsList )
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
                dvpConnect->requestVarsList.clear();
            }
            if ( msgPar.err == VAR_COMM_TIMEOUT )
            {
                dvpConnect->connStatus = CONNECT_NO_LINK;
            }
            else
            {
                dvpConnect->connStatus = CONNECT_LINKED_OK;
            }
            break;
        }
    }
    return result;
}

/******************************************************************************
** 函  数  名 ： run
** 说      明 : 线程固定函数，线程Start后进入run（）处理
** 输      入 ： void
** 返      回 ： 操作状态
******************************************************************************/
void DvpService::slot_run()
{
    initCommDev();
    m_runTimer->start(10);
}

void DvpService::slot_readData()
{
    struct DvpMsgPar msgPar;
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
                struct DvpConnect* dvpConnect = m_connectList[i];
                Q_ASSERT( dvpConnect );
                dvpConnect->waitWrtieVarsList.clear();
                dvpConnect->requestVarsList.clear();
                foreach( HmiVar* tempHmiVar, dvpConnect->actVarsList )
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
    while ( ret == sizeof( struct DvpMsgPar ) );

}

void DvpService::slot_runTimerTimeout()
{
    m_runTimer->stop();
    int lastUpdateTime = -1;

    int connectCount = m_connectList.count();
    m_currTime = getCurrTime();
    int currTime = m_currTime;

    for ( int i = 0; i < connectCount; i++ )
    {                              //处理写请求
        struct DvpConnect* dvpConnect = m_connectList[i];
        updateWriteReq( dvpConnect );
        trySndWriteReq( dvpConnect );
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
        struct DvpConnect* dvpConnect = m_connectList[i];
        if ( needUpdateFlag )
        {
            updateOnceRealReadReq( dvpConnect );  //先重组一次更新变量
            updateReadReq( dvpConnect ); //无论如何都要重组请求变量
        }
        if ( !( hasProcessingReq( dvpConnect ) ) )
        {
            combineReadReq( dvpConnect );
        }
        trySndReadReq( dvpConnect );
    }
    m_runTimer->start(20);


}
