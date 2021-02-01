
#include "mpi300Service.h"
#include "ppi/ppi_l7wr.h"
#include "ppi/ppi_l7.h"
#include "rwsocket.h"

#define MAX_PPI_FRAME_LEN  188   //最大一包188字节

INT32U conver_32bit( INT32U data )
{
    INT32U ret;

    ret = (( data << 24 ) & 0xFF000000 ) | (( data << 8 ) & 0xFF0000 ) | (( data >> 8 ) & 0xFF00 ) | (( data >> 24 ) & 0xFF );
    return ret;
}

INT16U conver_16bit( INT16U data )
{
    INT16U ret;

    ret = (( data << 8 ) & 0xFF00 ) | (( data >> 8 ) & 0xFF );
    return ret;
}

void converToDec(INT16U *data)
{
    INT16U tmp;
    tmp = *data;
    //qDebug()<<"tmp "<<tmp;
    *data &= 0x0000;
    *data |= (tmp/1000)<<4;
    *data |= ((tmp%1000)/100)<<0;
    *data |= ((tmp%100)/10)<<12;
    *data |= (tmp%10)<<8;
}

void converToHex(INT16U *data)
{
    INT16U tmp;
    tmp = *data ;//>>8 | *data<<8;/*QByteArray保存的是小端模式*/
    *data &= 0x0000;
    *data |= (tmp>>8 & 0x0F)*100 + (tmp>>4 & 0x0F)*10 + (tmp & 0x0F ) ;
}

Mpi300Service::Mpi300Service( struct Mpi300ConnectCfg& connectCfg )
{
    struct Mpi300Connect* pConnect = new Mpi300Connect;
    pConnect->cfg = connectCfg;
    pConnect->writingCnt = 0;
    pConnect->isLinking = false;
    pConnect->beginTime = -1;
    pConnect->connStatus = CONNECT_NO_OPERATION;
    m_currTime = getCurrTime();
    pConnect->connectTestVar.lastUpdateTime = -1;
    m_isMpi = connectCfg.isMpi;
    m_connectList.append( pConnect );
    m_runTime = new QTimer(this);
    connect(m_runTime, SIGNAL(timeout()), this, SLOT(slot_runTimeout()));
    connect( this, SIGNAL(startRuntime()), this, SLOT( slot_runTimeout() ) );
    m_rwSocket = NULL; //lvyong
    iniDriverFlg = false;
    m_isInitPPIMsgSend = false;
}

Mpi300Service::~Mpi300Service()
{
    if(m_runTime){
        m_runTime->stop();
        delete m_runTime;
    }
    if(m_rwSocket){
        delete m_rwSocket;
        m_rwSocket = NULL;
    }
}

bool Mpi300Service::meetServiceCfg( int serviceType, int COM_interface )
{
    if ( serviceType == SERVICE_TYPE_MPI300 )
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

int Mpi300Service::getConnLinkStatus( int connectId )
{
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct Mpi300Connect* pConnect = m_connectList[i];
        if ( connectId == pConnect->cfg.id )
            return pConnect->connStatus;
    }
    return -1;
}

int Mpi300Service::getComMsg( QList<struct ComMsg> &msgList )
{
    int ret = 0;
    m_mutexGetMsg.lock();

    ret = m_comMsg.count();
    if ( m_comMsg.count() > 0 )
    {
        if ( m_comMsg.count() == 1 && m_comMsg.contains( 65535 ) )
            msgList.append( m_comMsg.values() );
        else if ( m_comMsg.count() > 1 && m_comMsg.contains( 65535 ) )
        {
            msgList.append( m_comMsg.values() );
            m_comMsg.clear();
            struct ComMsg comMsg;
            comMsg.varId = 65535; //防止长变量
            comMsg.type = 0; //0是read;
            comMsg.err = VAR_COMM_TIMEOUT;
            m_comMsg.insert( 65535, comMsg );
        }
        else
        {
            msgList.append( m_comMsg.values() );
            m_comMsg.clear();
        }
    }

    m_mutexGetMsg.unlock();
    return ret;
}
int Mpi300Service::splitLongVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct Mpi300Connect* pConnect = m_connectList[i];
        if ( pHmiVar->connectId == pConnect->cfg.id )
        {
            /*if ( pHmiVar->area == PPI_AP_AREA_T )
            {
                int maxPackLen = 4 * (( MAX_PPI_FRAME_LEN - 12 ) / 5 );
                if ( pHmiVar->len > maxPackLen && pHmiVar->splitVars.count() <= 0 )  //拆分
                {
                    for ( int splitedLen = 0; splitedLen < pHmiVar->len; )
                    {
                        int len = pHmiVar->len - splitedLen;
                        if ( len > maxPackLen )
                            len = maxPackLen;

                        struct HmiVar* pSplitHhiVar = new HmiVar;
                        pSplitHhiVar->varId = pHmiVar->varId + (( pHmiVar->splitVars.count() + 1 ) << 16 );
                        pSplitHhiVar->connectId = pHmiVar->connectId;
                        pSplitHhiVar->cycleTime = pHmiVar->cycleTime;
                        pSplitHhiVar->area = pHmiVar->area;
                        pSplitHhiVar->addOffset = pHmiVar->addOffset + splitedLen / 4;
                        pSplitHhiVar->bitOffset = -1;
                        pSplitHhiVar->len = len;
                        pSplitHhiVar->dataType = pHmiVar->dataType;
                        pSplitHhiVar->err = VAR_NO_OPERATION; //没有数据，初始状态为没有初始
                        pSplitHhiVar->wStatus = VAR_NO_OPERATION;
                        pSplitHhiVar->lastUpdateTime = -1; //立即更新

                        splitedLen += len;
                        pHmiVar->splitVars.append( pSplitHhiVar );
                    }
                }
            }
            else if ( pHmiVar->area == PPI_AP_AREA_C_IEC )
            {
                int maxPackLen = 2 * (( MAX_PPI_FRAME_LEN - 12 ) / 3 );
                if ( pHmiVar->len > maxPackLen && pHmiVar->splitVars.count() <= 0 )  //拆分
                {
                    for ( int splitedLen = 0; splitedLen < pHmiVar->len; )
                    {
                        int len = pHmiVar->len - splitedLen;
                        if ( len > maxPackLen )
                            len = maxPackLen;

                        struct HmiVar* pSplitHhiVar = new HmiVar;
                        pSplitHhiVar->varId = pHmiVar->varId + (( pHmiVar->splitVars.count() + 1 ) << 16 );
                        pSplitHhiVar->connectId = pHmiVar->connectId;
                        pSplitHhiVar->cycleTime = pHmiVar->cycleTime;
                        pSplitHhiVar->area = pHmiVar->area;
                        pSplitHhiVar->addOffset = pHmiVar->addOffset + splitedLen / 2;
                        pSplitHhiVar->bitOffset = -1;
                        pSplitHhiVar->len = len;
                        pSplitHhiVar->dataType = pHmiVar->dataType;
                        pSplitHhiVar->err = VAR_NO_OPERATION; //没有数据，初始状态为没有初始
                        pSplitHhiVar->wStatus = VAR_NO_OPERATION;
                        pSplitHhiVar->lastUpdateTime = -1; //立即更新

                        splitedLen += len;
                        pHmiVar->splitVars.append( pSplitHhiVar );
                    }
                }
            }
            else
            {*/
                if ( pHmiVar->dataType == Com_Bool )
                {
                    Q_ASSERT( pHmiVar->len == 1 );
                }
                else
                {
                    int maxPackLen = MAX_PPI_FRAME_LEN - 12;
                    if ( pHmiVar->len > maxPackLen && pHmiVar->splitVars.count() <= 0 )  //拆分
                    {
                        for ( int splitedLen = 0; splitedLen < pHmiVar->len; )
                        {
                            int len = pHmiVar->len - splitedLen;
                            if ( len > maxPackLen )
                                len = maxPackLen;

                            struct HmiVar* pSplitHhiVar = new HmiVar;
                            pSplitHhiVar->varId = pHmiVar->varId + (( pHmiVar->splitVars.count() + 1 ) << 16 );
                            pSplitHhiVar->connectId = pHmiVar->connectId;
                            pSplitHhiVar->cycleTime = pHmiVar->cycleTime;
                            pSplitHhiVar->area = pHmiVar->area;
                            pSplitHhiVar->subArea = pHmiVar->subArea;
                            pSplitHhiVar->addOffset = pHmiVar->addOffset + splitedLen;
                            pSplitHhiVar->bitOffset = -1;
                            pSplitHhiVar->len = len;
                            pSplitHhiVar->dataType = pHmiVar->dataType;
                            pSplitHhiVar->err = VAR_NO_OPERATION; //没有数据，初始状态为没有初始
                            pSplitHhiVar->wStatus = VAR_NO_OPERATION;
                            pSplitHhiVar->lastUpdateTime = -1; //立即更新

                            splitedLen += len;
                            pHmiVar->splitVars.append( pSplitHhiVar );
                        }
                    }
                }
            //}
            return 0;
        }
    }
    return -1;
}
int Mpi300Service::onceReadUpdateVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    m_mutexOnceReal.lock();
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct Mpi300Connect* pConnect = m_connectList[i];
        if ( pHmiVar->connectId == pConnect->cfg.id )
        {
            pHmiVar->lastUpdateTime = -1; //立即更新
            if ( pHmiVar->splitVars.count() > 0 )
            {
                for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
                {
                    struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                    pSplitHhiVar->lastUpdateTime = -1;
                    pSplitHhiVar->overFlow = false;
                    if ( !pConnect->onceRealVarsList.contains( pSplitHhiVar ) )
                        pConnect->onceRealVarsList.append( pSplitHhiVar );
                }
            }
            else
            {
                pHmiVar->lastUpdateTime = -1;
                pHmiVar->overFlow = false;
                if ( !pConnect->onceRealVarsList.contains( pHmiVar ) )
                    pConnect->onceRealVarsList.append( pHmiVar );
            }
            m_mutexOnceReal.unlock();
            return 0;
        }
    }
    m_mutexOnceReal.unlock();
    return -1;
}

int Mpi300Service::startUpdateVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    m_mutexUpdateVar.lock();
    //printf("subArea %d %04X\n",pHmiVar->subArea,pHmiVar->subArea);

    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct Mpi300Connect* pConnect = m_connectList[i];
        if ( pHmiVar->connectId == pConnect->cfg.id )
        {
            pHmiVar->lastUpdateTime = -1; //立即更新
            if ( pHmiVar->splitVars.count() > 0 )
            {
                for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
                {
                    struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                    pSplitHhiVar->lastUpdateTime = -1;
                    pSplitHhiVar->overFlow = false;
                    if ( !pConnect->actVarsList.contains( pSplitHhiVar ) )
                        pConnect->actVarsList.append( pSplitHhiVar );
                }
            }
            else
            {
                pHmiVar->lastUpdateTime = -1;
                pHmiVar->overFlow = false;
                if ( !pConnect->actVarsList.contains( pHmiVar ) )
                    pConnect->actVarsList.append( pHmiVar );
            }
            m_mutexUpdateVar.unlock();
            readAllVar( pHmiVar->connectId, pHmiVar );
            return 0;
        }
    }
    m_mutexUpdateVar.unlock();
    return -1;
}
void Mpi300Service::readAllVar( int connectId, struct HmiVar* pHmiVar )
{
    if ( getConnLinkStatus( connectId ) != CONNECT_LINKED_OK )
    {
        struct ComMsg comMsg;
        comMsg.varId = pHmiVar->varId; //防止长变
        comMsg.type = 0; //0是read;
        comMsg.err = VAR_DISCONNECT;
        m_mutexGetMsg.lock();
        m_comMsg.insert( comMsg.varId, comMsg );
        m_mutexGetMsg.unlock();
    }
}
int Mpi300Service::stopUpdateVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    m_mutexUpdateVar.lock();
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct Mpi300Connect* pConnect = m_connectList[i];
        if ( pHmiVar->connectId == pConnect->cfg.id )
        {
            if ( pHmiVar->splitVars.count() > 0 )
            {
                for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
                {
                    struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                    pConnect->actVarsList.removeAll( pSplitHhiVar );
                }
            }
            else
                pConnect->actVarsList.removeAll( pHmiVar );
            m_mutexUpdateVar.unlock();
            return 0;
        }
    }
    m_mutexUpdateVar.unlock();
    return -1;
}

int readAreaT( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    int cnt = ( pHmiVar->len + 1 ) / 2;
    if ( bufLen >= pHmiVar->len && bufLen >= cnt*2 && pHmiVar->rData.size() >= cnt*2 )
    {
        for ( int i = 0; i < cnt; i++ )
        {
            int k;
            unsigned int decData;
            k = (pHmiVar->rData[i]>>4) + 1;

            if( ((pHmiVar->rData[i] & 0x0F) > 9) || ((pHmiVar->rData[i+1] >> 4)>9) || ((pHmiVar->rData[i+1] & 0x0F)>9) )
            {
                pHmiVar->err = VAR_COMM_TIMEOUT;
                return 0;
            }

            decData =( (pHmiVar->rData[i] & 0x0F) * 100 + (pHmiVar->rData[i+1] >> 4) * 10 + (pHmiVar->rData[i+1] & 0x0F) );

            for(;k>0;k--)
            {
                decData = decData * 10;
            }
           // qDebug()<<hex<<"decData "<<decData;
            pBuf[4*i+2] = (decData >> 16)  & 0xFF;
            pBuf[4*i+3] = (decData >> 24) & 0xFF;
            pBuf[4*i+0] = (decData >> 0)  & 0xFF;
            pBuf[4*i+1] = (decData >> 8)  & 0xFF;

            if( decData > 9990000 )
            {
                pHmiVar->err = VAR_COMM_TIMEOUT;
                return 0;
            }
           // qDebug()<<hex<<"buf "<<pBuf[0]<<pBuf[1]<<pBuf[2]<<pBuf[3];
        }
    }
    else
        Q_ASSERT( false );
    return 0;
}
int readAreaC( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    int cnt = (pHmiVar->len+1)/2;
    if ( bufLen >= pHmiVar->len && pHmiVar->rData.size() >= pHmiVar->len )
    {
        for ( int i = 0; i < cnt; i++ )
        {
            pBuf[2*i+0] = *(pHmiVar->rData.data()+2*i+1);
            pBuf[2*i+1] = *(pHmiVar->rData.data()+2*i+0);

            if( ((pBuf[2*i+1] & 0x0F) > 9)
                ||((pBuf[2*i+0]>>4) > 9) || ((pBuf[2*i+0] & 0x0F) > 9 ) )
            {
                pHmiVar->err = VAR_COMM_TIMEOUT;
                return 0;
            }
            converToHex((INT16U*)&pBuf[2*i+0]);//conver hex data "0x1234" to dec data "1234"
        }
    }
    else
        Q_ASSERT( false );
    return 0;
}

int Mpi300Service::readVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    if ( pHmiVar->err == VAR_NO_ERR )  //错误才拷贝数据
    {
        if ( pHmiVar->area == PPI_AP_AREA_C )
        {
            readAreaC( pHmiVar, pBuf, bufLen );
        }
        else if ( pHmiVar->area == PPI_AP_AREA_T )
        {
            readAreaT( pHmiVar, pBuf, bufLen );
        }
        else
        {
            if ( pHmiVar->dataType == Com_Bool )
            {
                if ( bufLen >= 1 )
                {
                    if ( pHmiVar->rData.size() > 0 )
                        pBuf[0] = ( pHmiVar->rData.at( 0 ) >> pHmiVar->bitOffset ) & 0x1;
                    else
                        pBuf[0] = 0;
                }
                else
                    Q_ASSERT( false );
            }
            else if( pHmiVar->dataType == Com_S7300_Counter )
            {
                readAreaC( pHmiVar, pBuf, bufLen );
            }
            else if( pHmiVar->dataType == Com_S7300_Timer)
            {
                readAreaT( pHmiVar, pBuf, bufLen );
            }
            else if ( pHmiVar->dataType == Com_DateTime || pHmiVar->dataType == Com_PointArea)
            {
                int dataTypeLen = getDataTypeLen( pHmiVar->dataType );
                Q_ASSERT( dataTypeLen > 0 );
                for ( int j = 0; ; j++ )
                {
                    if ( j < pHmiVar->len && j < bufLen && j < pHmiVar->rData.size() )
                        pBuf[j] = pHmiVar->rData.at( j );
                    else
                        break;
                }
            }
           /*else if ( pHmiVar->dataType == Com_PointArea )
            {
                for ( int j = 0; ; j++ )             //大小数转
                {
                    if ( j < pHmiVar->len && j < bufLen && j < pHmiVar->rData.size() )
                        pBuf[j] = pHmiVar->rData.at( j );
                    else
                        break;
            }*/
            else
            {
                int dataTypeLen = getDataTypeLen( pHmiVar->dataType );
                Q_ASSERT( dataTypeLen > 0 );
                for ( int j = 0; ; j++ )             //大小数转
                {
                    int k;
                    for ( k = 0; k < dataTypeLen; k++ )
                    {
                        int wOffset = dataTypeLen * j + k;
                        int rOffset = dataTypeLen * j + dataTypeLen - 1 - k;

                        if ( wOffset < pHmiVar->len && wOffset < bufLen && rOffset < pHmiVar->rData.size() )
                            pBuf[wOffset] = pHmiVar->rData.at( rOffset );
                        else
                            break;
                    }
                    if ( k < dataTypeLen )
                        break;
                }
            }
        }
    }
    return pHmiVar->err;
}

int Mpi300Service::readDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    Q_ASSERT( pHmiVar && pBuf );
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct Mpi300Connect* pConnect = m_connectList[i];
        if ( pHmiVar->connectId == pConnect->cfg.id )
        {
            int ret;
            m_mutexRw.lock();
            if ( pHmiVar->splitVars.count() > 0 )
            {
                Q_ASSERT( pHmiVar->dataType != Com_Bool );
                for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
                {
                    struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                    ret = readVar( pSplitHhiVar, pBuf, pSplitHhiVar->len );
                    if ( ret != VAR_NO_ERR )
                        break;
                    pBuf += pSplitHhiVar->len;
                    bufLen -= pSplitHhiVar->len;
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

int writeAreaT( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    int count = ( pHmiVar->len + 1 ) / 2;

    if ( pHmiVar->rData.size() < 2*count )
        pHmiVar->rData.resize( 2*count );
    for ( int i = 0; i < count; i++ )
    {
        Q_ASSERT( bufLen/4 >= i );
        int decData = *(unsigned int*)&pBuf[4*i];
        //qDebug()<<"writevar decData "<<decData;
        int k = 0;

        if( decData >= 1000000 )
        {
            k = 3;
            if( ((decData/1000)%10) >= 5 )
            {
                decData = decData + 10000;
                if( decData > 9999999 )
                {
                    continue;
                    //return -1;
                }
            }
            decData = decData / 10000;
        }
        else if( decData >= 100000 )
        {
            if( ((decData/100)%10) >= 5 )
            {
                decData = decData + 1000;
                if( decData > 999999 )
                {
                    k = 3;
                    decData = decData / 10000;
                }
                else
                {
                    k = 2;
                    decData = decData / 1000;
                }
            }
            else
            {
                k = 2;
                decData = decData / 1000;
            }

        }
        else if ( decData >= 10000 )
        {
            if( ((decData/10)%10) >= 5 )
            {
                decData = decData + 100;
                if( decData > 99999 )
                {
                    k = 2;
                    decData = decData / 1000;
                }
                else
                {
                    k = 1;
                    decData = decData / 100;
                }
            }
            else
            {
                k = 1;
                decData = decData / 100;
            }

        }
        else
        {
            if( (decData%10) >= 5 )
            {
                decData = decData + 10;
                if( decData > 9999 )
                {
                    k = 1;
                    decData = decData / 100;
                }
                else
                {
                    k = 0;
                    decData = decData / 10;
                }
            }
            else
            {
                k = 0;
                decData = decData/10 ;
            }
        }

        if ( 2*i <= pHmiVar->wData.size() )
        {
            unsigned char tmp[2] = { 0x00, 0x00 };
            tmp[0] |= k << 4;
            tmp[0] |= (decData % 1000) / 100;
            tmp[1] |= ((decData % 100) / 10) << 4;
            tmp[1] |= (decData % 10);
           // qDebug()<<"writevar tmp "<<decData<<" "<<hex<<tmp[0]<<tmp[1];

            pHmiVar->wData[i]   = tmp[1];
            pHmiVar->wData[i+1] = tmp[0];//clear  first

            pHmiVar->rData[i]   = pHmiVar->wData[i+1];
            pHmiVar->rData[i+1] = pHmiVar->wData[i+0];

          //  qDebug()<<"writevar buf "<<pHmiVar->rData.toHex();
        }
        else
            break;
    }
    return 0;
}
int writeAreaC( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    int count = ( pHmiVar->len + 1 ) / 2;
    if ( pHmiVar->rData.size() < 2*count )
        pHmiVar->rData.resize( 2*count );
    for ( int i = 0; i < count; i++ )
    {
        if ( 2*i + 1 < pHmiVar->wData.size() )
        {
           // qDebug()<<"wData1 "<<pHmiVar->wData.toHex();
            INT8U tmp[2] = {0,0};

            tmp[0] = pHmiVar->wData[2*i];
            tmp[1] = pHmiVar->wData[2*i+1];

            if( *(INT16U*)(tmp) > 1000 )
            {
                continue;
            }
            converToDec(( INT16U* )tmp );
            pHmiVar->wData[2*i]   = tmp[1];
            pHmiVar->wData[2*i+1] = tmp[0];
           // qDebug()<<"wData2 "<<pHmiVar->wData.toHex();
            pHmiVar->rData[2*i]   = pHmiVar->wData[2*i+1];
            pHmiVar->rData[2*i+1] = pHmiVar->wData[2*i+0];
        }
        else
        {
            break;
        }
    }
    return 0;
}
int Mpi300Service::writeVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    int ret;
    Q_ASSERT( pHmiVar && pBuf );
    m_mutexRw.lock();
    if ( pHmiVar->dataType == Com_Bool )
        Q_ASSERT( pHmiVar->len == 1 );
    int dataBytes = qMin( bufLen, pHmiVar->len );
    /*if ( pHmiVar->wData.size() < dataBytes )
        pHmiVar->wData.resize( dataBytes );*/
    if ( pHmiVar->wData.size() < pHmiVar->len )
        pHmiVar->wData.resize( pHmiVar->len );

    memcpy( pHmiVar->wData.data(), pBuf, dataBytes );

    if ( pHmiVar->area == PPI_AP_AREA_C )
    {
        writeAreaC( pHmiVar, pBuf, bufLen );
    }
    else if ( pHmiVar->area == PPI_AP_AREA_T )
    {
        writeAreaT( pHmiVar, pBuf, bufLen );
    }
    else
    {
        if ( pHmiVar->dataType == Com_Bool )
        {
            Q_ASSERT( pHmiVar->len == 1 );
            if ( pHmiVar->rData.size() < 1 )
                pHmiVar->rData.resize( 1 );
            pHmiVar->rData[0] = 0;
            if ( pHmiVar->wData.size() > 0 )
            {
                if ( pHmiVar->wData[0] & 0x1 )
                    pHmiVar->rData[0] = ( 1 << pHmiVar->bitOffset );
            }
        }
        else if( pHmiVar->dataType == Com_S7300_Counter )
        {
            writeAreaC( pHmiVar, pBuf, bufLen );
        }
        else if( pHmiVar->dataType == Com_S7300_Timer)
        {
            writeAreaT( pHmiVar, pBuf, bufLen );
        }
        else if( pHmiVar->dataType == Com_PointArea )
        {
            for ( int j = 0; ; j++ )
            {
                if ( j < pHmiVar->len && j < bufLen && j < pHmiVar->rData.size() )
                {
                    pHmiVar->rData[j] = pHmiVar->wData[j];
                }
                else
                    break;
            }
            //        qDebug()<<"Write PA "<<pHmiVar->rData.toHex();
        }
        else
        {
            if ( pHmiVar->rData.size() < pHmiVar->len )
                pHmiVar->rData.resize( pHmiVar->len );
            int dataTypeLen = getDataTypeLen( pHmiVar->dataType );
            Q_ASSERT( dataTypeLen > 0 );
            for ( int j = 0; ; j++ )             //大小数转
            {
                int k;
                for ( k = 0; k < dataTypeLen; k++ )
                {
                    int wOffset = dataTypeLen * j + k;
                    int rOffset = dataTypeLen * j + dataTypeLen - 1 - k;
                    if ( wOffset < pHmiVar->rData.size() && rOffset < pHmiVar->wData.size() )
                        pHmiVar->rData[wOffset] = pHmiVar->wData[rOffset];
                    else
                        break;
                }
                if ( k < dataTypeLen )
                    break;
            }
        }
    }

    pHmiVar->lastUpdateTime = -1;
    pHmiVar->wStatus = VAR_NO_OPERATION;
    ret = pHmiVar->err;
    m_mutexRw.unlock();

    return ret; //是否要返回成
}

int Mpi300Service::writeDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    Q_ASSERT( pHmiVar && pBuf );
    //qDebug()<<"writeDevVar "<<*(int*)(pBuf)<<" Len "<<bufLen;
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct Mpi300Connect* pConnect = m_connectList[i];
        if ( pHmiVar->connectId == pConnect->cfg.id )
        {
            if ( pConnect->connStatus != CONNECT_LINKED_OK )
                return  VAR_COMM_TIMEOUT;
            int ret = 0;
            m_mutexWReq.lock();          //注意2个互斥锁顺序,防止死锁
            if ( pHmiVar->splitVars.count() > 0 )
            {
                Q_ASSERT( pHmiVar->dataType != Com_Bool );
                for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
                {
                    struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                    ret = writeVar( pSplitHhiVar, pBuf, pSplitHhiVar->len );
                    pConnect->waitWrtieVarsList.append( pSplitHhiVar );
                    pBuf += pSplitHhiVar->len;
                    bufLen -= pSplitHhiVar->len;
                }
            }
            else
            {
                ret = writeVar( pHmiVar, pBuf, bufLen );
                pConnect->waitWrtieVarsList.append( pHmiVar );
            }
            m_mutexWReq.unlock();
            return ret; //是否要返回成
        }
    }
    return -1;
}

int Mpi300Service::addConnect( struct Mpi300ConnectCfg& connectCfg )
{                                      //要检查是否重
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct Mpi300Connect* pConnect = m_connectList[i];
        Q_ASSERT( pConnect->cfg.COM_interface == connectCfg.COM_interface );//绝对要是同一个设备接
        if ( connectCfg.id == pConnect->cfg.id )
        {
            Q_ASSERT( false );
            return 0;
        }
    }
    struct Mpi300Connect* pConnect = new Mpi300Connect;
    pConnect->cfg = connectCfg;
    pConnect->writingCnt = 0;
    pConnect->isLinking = false;
    pConnect->connStatus = CONNECT_NO_OPERATION;
    m_connectList.append( pConnect );

    return 0;
}

int Mpi300Service::sndWriteVarMsgToDriver( struct Mpi300Connect* pConnect, struct HmiVar* pHmiVar )
{
    L2_MASTER_MSG_STRU *pL2Msg;
    PPI_PDU_REQ_MULT_READ *pPDU_NETW;   // NETR的PDU结构指针
    PPI_NETR_REQ_VAR_STRU *pVars;
    INT8U buf[MAX_LONG_BUF_LEN];
    PPI_PDU_REQ_HEADER *pPduHeader; //NETW的包
    PPI_NETW_REQ_DAT_STRU *pDats;

    Q_ASSERT( pConnect && pHmiVar );
    int totallen = 0;
    int varCnt = 0;
    for ( int i = 0; i < pConnect->waitWrtieVarsList.count(); i++ )
    {
        struct HmiVar* pHmiVar = pConnect->waitWrtieVarsList[i];
        if ( pHmiVar->len == 0 )
        {
            Q_ASSERT( false );
            return -1;
        }
        if (( pHmiVar->area != PPI_AP_AREA_I ) && ( pHmiVar->area != PPI_AP_AREA_Q )
                        && ( pHmiVar->area != PPI_AP_AREA_M ) && ( pHmiVar->area != PPI_AP_AREA_DB )
                        && ( pHmiVar->area != PPI_AP_AREA_SYS ) && ( pHmiVar->area != PPI_AP_AREA_SM )
                        && ( pHmiVar->area != PPI_AP_AREA_T ) && ( pHmiVar->area != PPI_AP_AREA_C )
                        && ( pHmiVar->area != PPI_AP_AREA_P ) )
        {
            Q_ASSERT( false );
            return -1;
        }
        int dataLen = 12; //
        /*if ( pHmiVar->area == PPI_AP_AREA_T_IEC )
            dataLen += (( pHmiVar->len + 3 ) / 4 ) * 5;
        else if ( pHmiVar->area == PPI_AP_AREA_C_IEC )
            dataLen += (( pHmiVar->len + 1 ) / 2 ) * 3;
        else
        {*/
            if ( pHmiVar->dataType == Com_Bool )
                Q_ASSERT( pHmiVar->len == 1 );
            dataLen += pHmiVar->len;
        //}
        if ( totallen + dataLen <= MAX_PPI_FRAME_LEN && varCnt < 8 )
        {
            varCnt++;
            totallen += dataLen;
        }
        else
            break;
    }

    m_mutexRw.lock();
    pL2Msg = ( L2_MASTER_MSG_STRU * )buf;
    pL2Msg->Da = pConnect->cfg.cpuAddr;
    pL2Msg->MsgMem = LongBufMem;
    pL2Msg->Port = 0;
    pL2Msg->Service = PMS_NETW;
    if(++m_sn == 0)
    {
        m_sn = 1;
    }
    pL2Msg->pTbl = m_sn<<24;

    pPDU_NETW = ( PPI_PDU_REQ_MULT_READ * )( pL2Msg->pPdu );

    //PDU Header Block
    pPduHeader = ( PPI_PDU_REQ_HEADER * )( pL2Msg->pPdu );
    pPduHeader->ProtoID  = PROT_ID_S7_200;
    pPduHeader->Rosctr   = ROSCTR_REQUEST;
    pPduHeader->RedID    = conver_16bit( 0x0000 );
    pPduHeader->PduRef   = conver_16bit( 0 );
    pPduHeader->ParLen   = conver_16bit( sizeof( PPI_NETR_REQ_VAR_STRU ) * varCnt + 2 ); 
    pPduHeader->DatLen   = 0; //需要重新填

    INT8U* Ptr = pL2Msg->pPdu + sizeof( PPI_PDU_REQ_HEADER ); // 指向Service ID 的位
    *Ptr = PPI_SID_WRITE;
    Ptr ++;
    *Ptr = varCnt;
    Ptr ++;         //处理完后指向第一个变量的地
    pVars = ( PPI_NETR_REQ_VAR_STRU * )Ptr;
    int PduDatLen = 0;
    for ( int i = 0; i < varCnt; i++ )
    {
        int VarDatLen = 0;
        pDats = ( PPI_NETW_REQ_DAT_STRU * )( pL2Msg->pPdu + sizeof( PPI_PDU_REQ_HEADER )
                                             + sizeof( PPI_NETR_REQ_VAR_STRU ) * varCnt + 2 + PduDatLen );
        struct HmiVar* pHmiVar = pConnect->waitWrtieVarsList[i];
        //PDU Par Variables
        pVars->VarSpec = 0x12;  //固定
        pVars->AddrLen = 0x0A;  //any pointer 的长

        //设置any pointer.
        pVars->VarAddr.Syntax_ID  = 0x10; //固定
        pVars->VarAddr.Area = pHmiVar->area;

        if ( pHmiVar->area == PPI_AP_AREA_DB )
            pVars->VarAddr.Subarea  = conver_16bit( pHmiVar->subArea );
        else
            pVars->VarAddr.Subarea  = conver_16bit( 0x0000 );

        if ( pHmiVar->area == PPI_AP_AREA_C )
        {
            pVars->VarAddr.Type = PPI_AP_TYPE_C;

            pVars->VarAddr.NumElements = conver_16bit((( pHmiVar->len + 1 ) / 2 ) );
            //pVars->VarAddr.Offset = conv_32bit( pHmiVar->addOffset ) >> 8;
            pVars->VarAddr.Offset[0] = pHmiVar->addOffset >> 16 & 0xFF;
            pVars->VarAddr.Offset[1] = pHmiVar->addOffset >>  8 & 0xFF;
            pVars->VarAddr.Offset[2] = pHmiVar->addOffset       & 0xFF;
            pDats->DataType = D_TYPE_LONG;
            pDats->Length = conver_16bit( pHmiVar->len );
            int count = ( pHmiVar->len + 1 ) / 2;
            for ( int j = 0; j < count; j++ )
            {
                *(( INT8U* )( pDats->Dat ) + 2*j + 0 ) = pHmiVar->wData[2*j+1];
                *(( INT8U* )( pDats->Dat ) + 2*j + 1 ) = pHmiVar->wData[2*j+0];
            }
            VarDatLen = count * 2;
        }
        else if ( pHmiVar->area == PPI_AP_AREA_T  )
        {
            pVars->VarAddr.Type = PPI_AP_TYPE_T;

            pVars->VarAddr.NumElements = conver_16bit((( pHmiVar->len + 1 ) / 2 ) );
            //pVars->VarAddr.Offset = conv_32bit( pHmiVar->addOffset ) >> 8;
            pVars->VarAddr.Offset[0] = pHmiVar->addOffset >> 16 & 0xFF;
            pVars->VarAddr.Offset[1] = pHmiVar->addOffset >>  8 & 0xFF;
            pVars->VarAddr.Offset[2] = pHmiVar->addOffset       & 0xFF;

            pDats->DataType = D_TYPE_LONG;
            pDats->Length = conver_16bit( pHmiVar->len );
            int count = ( pHmiVar->len + 1 ) / 2;
            for ( int j = 0; j < count; j++ )
            {
                *(( INT8U* )( pDats->Dat ) + 2*j + 0 ) = pHmiVar->wData[2*j+1];
                *(( INT8U* )( pDats->Dat ) + 2*j + 1 ) = pHmiVar->wData[2*j+0];
            }
            VarDatLen = count * 2;
        }
        else
        {
            if ( pHmiVar->dataType == Com_Bool )
            {
                pVars->VarAddr.Type = PPI_AP_TYPE_BOOL;
                pVars->VarAddr.NumElements = conver_16bit( 1 );
                //pVars->VarAddr.Offset = conv_32bit( pHmiVar->addOffset * 8 + pHmiVar->bitOffset ) >> 8;
                pVars->VarAddr.Offset[0] = ( pHmiVar->addOffset * 8 + pHmiVar->bitOffset ) >> 16 & 0xFF;
                pVars->VarAddr.Offset[1] = ( pHmiVar->addOffset * 8 + pHmiVar->bitOffset ) >>  8 & 0xFF;
                pVars->VarAddr.Offset[2] = ( pHmiVar->addOffset * 8 + pHmiVar->bitOffset )       & 0xFF;

                pDats->Length = conver_16bit( 1 );
                pDats->DataType = D_TYPE_BIT;
                if ( pHmiVar->wData.size() > 0 )
                    pDats->Dat[0] = pHmiVar->wData[0] & 0x1;
                else
                {
                    Q_ASSERT( false );
                    pDats->Dat[0] = 0;
                }
                VarDatLen = 1;
            }
            else
            {
                pVars->VarAddr.Type = PPI_AP_TYPE_BYTE;
                pVars->VarAddr.NumElements = conver_16bit( pHmiVar->len );
                //pVars->VarAddr.Offset = conv_32bit( pHmiVar->addOffset * 8 ) >> 8;
                pVars->VarAddr.Offset[0] =( pHmiVar->addOffset * 8 ) >> 16 & 0xFF;
                pVars->VarAddr.Offset[1] =( pHmiVar->addOffset * 8 ) >>  8 & 0xFF;
                pVars->VarAddr.Offset[2] =( pHmiVar->addOffset * 8 ) & 0xFF;

                pDats->Length = conver_16bit( pHmiVar->len * 8 );
                pDats->DataType = D_TYPE_BYTES;
                int dataTypeLen = getDataTypeLen( pHmiVar->dataType );
                Q_ASSERT( dataTypeLen > 0 );
                for ( int j = 0; ; j++ )             //大小数转
                {
                    int k;
                    for ( k = 0; k < dataTypeLen; k++ )
                    {
                        int wOffset = dataTypeLen * j + k;
                        int rOffset = dataTypeLen * j + dataTypeLen - 1 - k;
                        if ( wOffset < pHmiVar->len && rOffset < pHmiVar->wData.size() )
                            *(( INT8U* )( pDats->Dat ) + wOffset ) = pHmiVar->wData[rOffset];
                        else
                            break;
                    }
                    if ( k < dataTypeLen )
                        break;
                }
                VarDatLen = pHmiVar->len;
            }
        }
        pDats->Reserved = 0x00;
        if (( VarDatLen & 0x01 ) && ( i + 1 < varCnt ) )
        {                           
            pDats->Dat[VarDatLen] = 0x00; 
            VarDatLen += 1;
        }

        PduDatLen += ( 4 + VarDatLen );  // Reserveed + Data type + Length 刚好4字节
        pDats = ( PPI_NETW_REQ_DAT_STRU * )( pL2Msg->pPdu + sizeof( PPI_PDU_REQ_HEADER ) + pPduHeader->ParLen + PduDatLen );
        pVars++; 
    }
    pPduHeader->DatLen = conver_16bit( PduDatLen ); //把实际的数据block的长度填回去
    pL2Msg->PduLen = sizeof( PPI_PDU_REQ_HEADER ) + ( 2 + sizeof( PPI_NETR_REQ_VAR_STRU ) * varCnt ) + PduDatLen;
    //m_devUseCnt++;
    m_mutexRw.unlock();

    //int ret = write( m_fd, buf, MAX_LONG_BUF_LEN + 1024 );     //填写完毕后，发给L2处理
    int ret = m_rwSocket->writeDev((char *)buf, MAX_SHORT_BUF_LEN+1024/*pL2Msg->PduLen*/);
    Q_ASSERT( ret > 0 );

    return varCnt;
}

int Mpi300Service::sndReadVarMsgToDriver( struct Mpi300Connect* pConnect, struct HmiVar* pHmiVar )
{
    L2_MASTER_MSG_STRU *pL2Msg;
    PPI_PDU_REQ_MULT_READ *pPDU_NETR;   //NETR的PDU结构指针
    PPI_NETR_REQ_VAR_STRU *pVars;
    INT8U buf[MAX_LONG_BUF_LEN];
    memset(buf,0,MAX_LONG_BUF_LEN);

    Q_ASSERT( pHmiVar );
    pL2Msg = ( L2_MASTER_MSG_STRU * )buf;

    pL2Msg->Da = pConnect->cfg.cpuAddr;
    pL2Msg->MsgMem = LongBufMem;
    pL2Msg->Port = 0;
    pL2Msg->Service = PMS_NETR;
    if(++m_sn == 0)
    {
        m_sn = 1;
    }
    pL2Msg->pTbl = m_sn<<24;
    pPDU_NETR = ( PPI_PDU_REQ_MULT_READ * )( pL2Msg->pPdu );

    
    //PDU Header Block
    pPDU_NETR->Header.ProtoID  = PROT_ID_S7_200;
    pPDU_NETR->Header.Rosctr   = ROSCTR_REQUEST;
    pPDU_NETR->Header.RedID    = conver_16bit( 0x0000 );
    pPDU_NETR->Header.PduRef   = conver_16bit( 0 );
    pPDU_NETR->Header.ParLen   = conver_16bit( 0 ); //fill later  sizeof(Heard) + 2 + sizeof() * blkNum;
    pPDU_NETR->Header.DatLen   = conver_16bit( 0 );

    //Parameter Block
    pPDU_NETR->Para.ServiceID  = PPI_SID_READ;
    pPDU_NETR->Para.VarNum     = 0;  //fill later.

    pVars = pPDU_NETR->Para.Vars;

    if ( pHmiVar && pHmiVar->addOffset >= 0 && pHmiVar->len > 0 ) 
    {
        //设置any pointer.
        pPDU_NETR->Para.Vars[0].VarSpec = 0x12; //固定
        pPDU_NETR->Para.Vars[0].AddrLen = 0x0A; 
        pPDU_NETR->Para.Vars[0].VarAddr.Syntax_ID = 0x10;
        int dataLen = 12;// + 4; //?
        switch ( pHmiVar->area )
        {
        case PPI_AP_AREA_I:
        case PPI_AP_AREA_Q:
        case PPI_AP_AREA_M:
        case PPI_AP_AREA_DB:
        case PPI_AP_AREA_SM:
        case PPI_AP_AREA_SYS:
        case PPI_AP_AREA_P:
            pPDU_NETR->Para.Vars[0].VarAddr.Type = PPI_AP_TYPE_BYTE;
            //pPDU_NETR->Para.Vars[0].VarAddr.Offset = conv_32bit( pHmiVar->addOffset * 8 ) >> 8;
            pPDU_NETR->Para.Vars[0].VarAddr.Offset[0] = ( pHmiVar->addOffset * 8 )>> 16 & 0xFF;
            pPDU_NETR->Para.Vars[0].VarAddr.Offset[1] = ( pHmiVar->addOffset * 8 )>>  8 & 0xFF;
            pPDU_NETR->Para.Vars[0].VarAddr.Offset[2] = ( pHmiVar->addOffset * 8 )     & 0xFF;
            if ( pHmiVar->dataType == Com_Bool )
            {
                Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 7 );
                pPDU_NETR->Para.Vars[0].VarAddr.NumElements = conver_16bit(( pHmiVar->bitOffset + pHmiVar->len + 7 ) / 8 );
                dataLen += ( pHmiVar->bitOffset + pHmiVar->len + 7 ) / 8;
            }
            else
            {
                pPDU_NETR->Para.Vars[0].VarAddr.NumElements = conver_16bit( pHmiVar->len );
                dataLen += pHmiVar->len;
            }
            break;
        /*case PPI_AP_AREA_T_IEC:
            pPDU_NETR->Para.Vars[0].VarAddr.Type = PPI_AP_TYPE_T_IEC;
            Q_ASSERT(( pHmiVar->len % 4 ) == 0 );
            pPDU_NETR->Para.Vars[0].VarAddr.NumElements = conver_16bit(( pHmiVar->len + 3 ) / 4 );
            pPDU_NETR->Para.Vars[0].VarAddr.Offset = conver_32bit( pHmiVar->addOffset ) >> 8;
            dataLen += 5 * ( pHmiVar->len + 3 ) / 4;
            break;
        case PPI_AP_AREA_C_IEC:
            pPDU_NETR->Para.Vars[0].VarAddr.Type = PPI_AP_TYPE_C_IEC;
            Q_ASSERT(( pHmiVar->len % 2 ) == 0 );
            pPDU_NETR->Para.Vars[0].VarAddr.NumElements = conver_16bit(( pHmiVar->len + 1 ) / 2 );
    //pPDU_NETR->Para.Vars[0].VarAddr.Offset = conv_32bit( pHmiVar->addOffset ) >> 8;
            pPDU_NETR->Para.Vars[0].VarAddr.Offset[0] = ( pHmiVar->addOffset )>> 16 & 0xFF;
            pPDU_NETR->Para.Vars[0].VarAddr.Offset[1] = ( pHmiVar->addOffset )>>  8 & 0xFF;
            pPDU_NETR->Para.Vars[0].VarAddr.Offset[2] = ( pHmiVar->addOffset )      & 0xFF;
            dataLen += 3 * ( pHmiVar->len + 1 ) / 2;
            break;*/
        case PPI_AP_AREA_T:
            pPDU_NETR->Para.Vars[0].VarAddr.Type = PPI_AP_TYPE_T;
            //pPDU_NETR->Para.Vars[0].VarAddr.Offset = conver_32bit( pHmiVar->addOffset ) >> 8;
            pPDU_NETR->Para.Vars[0].VarAddr.Offset[0] = ( pHmiVar->addOffset )>> 16 & 0xFF;
            pPDU_NETR->Para.Vars[0].VarAddr.Offset[1] = ( pHmiVar->addOffset )>>  8 & 0xFF;
            pPDU_NETR->Para.Vars[0].VarAddr.Offset[2] = ( pHmiVar->addOffset )      & 0xFF;
            if ( pHmiVar->dataType == Com_Bool )
            {
                Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 7 );
                pPDU_NETR->Para.Vars[0].VarAddr.NumElements = conver_16bit((( pHmiVar->bitOffset + pHmiVar->len + 7 ) / 8 + 1) / 2 );
                dataLen += ( pHmiVar->bitOffset + pHmiVar->len + 7 ) / 8;
            }
            else
            {
                pPDU_NETR->Para.Vars[0].VarAddr.NumElements = conver_16bit( (pHmiVar->len + 1)/2 );
                dataLen += pHmiVar->len;
            }
            break;
        case PPI_AP_AREA_C:
            pPDU_NETR->Para.Vars[0].VarAddr.Type = PPI_AP_TYPE_C;
        //    pPDU_NETR->Para.Vars[0].VarAddr.Offset = conver_32bit( pHmiVar->addOffset ) >> 8;
            pPDU_NETR->Para.Vars[0].VarAddr.Offset[0] = ( pHmiVar->addOffset )>> 16 & 0xFF;
            pPDU_NETR->Para.Vars[0].VarAddr.Offset[1] = ( pHmiVar->addOffset )>>  8 & 0xFF;
            pPDU_NETR->Para.Vars[0].VarAddr.Offset[2] = ( pHmiVar->addOffset )      & 0xFF;
            if ( pHmiVar->dataType == Com_Bool )
            {
                Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 7 );
                pPDU_NETR->Para.Vars[0].VarAddr.NumElements = conver_16bit((( pHmiVar->bitOffset + pHmiVar->len + 7 ) / 8 + 1)/2 );
                dataLen += ( pHmiVar->bitOffset + pHmiVar->len + 7 ) / 8;
            }
            else
            {
                pPDU_NETR->Para.Vars[0].VarAddr.NumElements = conver_16bit(( pHmiVar->len+1)/2 );
                dataLen += pHmiVar->len;
            }
            break;
        case PPI_AP_AREA_HC_IEC:
        default:
            Q_ASSERT( false );
            return -1;
        }
        if ( pHmiVar->area == PPI_AP_AREA_DB )
            pPDU_NETR->Para.Vars[0].VarAddr.Subarea  = conver_16bit( pHmiVar->subArea );
        else
            pPDU_NETR->Para.Vars[0].VarAddr.Subarea  = conver_16bit( 0x0000 );

        pPDU_NETR->Para.Vars[0].VarAddr.Area = pHmiVar->area;

        pPDU_NETR->Para.VarNum = 1;
        pPDU_NETR->Header.ParLen = conver_16bit( 2 + sizeof( PPI_NETR_REQ_VAR_STRU ) * 1 );
        pL2Msg->PduLen = sizeof( PPI_PDU_REQ_HEADER ) + ( INT8U )( 2 + sizeof( PPI_NETR_REQ_VAR_STRU ) * 1 );
    }
    else                                  
    {
        int totallen = 0;
        for ( int i = 0; i < pConnect->requestVarsList.count(); i++ )
        {
            struct HmiVar* pHmiVar = pConnect->requestVarsList[i];
            if ( pHmiVar->len == 0 )
            {
                Q_ASSERT( false );
                return -1;
            }
            //设置any pointer.
            pPDU_NETR->Para.Vars[i].VarSpec = 0x12; //固定
            pPDU_NETR->Para.Vars[i].AddrLen = 0x0A; 
            pPDU_NETR->Para.Vars[i].VarAddr.Syntax_ID = 0x10;
            int dataLen = 12;// + 4;//?
            switch ( pHmiVar->area )
            {
            case PPI_AP_AREA_I:
            case PPI_AP_AREA_Q:
            case PPI_AP_AREA_M:
            case PPI_AP_AREA_DB:
            case PPI_AP_AREA_SM:
            case PPI_AP_AREA_SYS:
            case PPI_AP_AREA_P:
                pPDU_NETR->Para.Vars[i].VarAddr.Type = PPI_AP_TYPE_BYTE;
                //pPDU_NETR->Para.Vars[i].VarAddr.Offset = conv_32bit( pHmiVar->addOffset * 8 ) >> 8;
            //    qDebug()<<"offset "<<QString::number(pHmiVar->addOffset * 8, 16);
                pPDU_NETR->Para.Vars[i].VarAddr.Offset[0] = ( pHmiVar->addOffset * 8 )>> 16 & 0xFF;
                pPDU_NETR->Para.Vars[i].VarAddr.Offset[1] = ( pHmiVar->addOffset * 8 )>>  8 & 0xFF;
                pPDU_NETR->Para.Vars[i].VarAddr.Offset[2] = ( pHmiVar->addOffset * 8 )      & 0xFF;
                if ( pHmiVar->dataType == Com_Bool )
                {
                    Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 7 );
                    pPDU_NETR->Para.Vars[i].VarAddr.NumElements = conver_16bit(( pHmiVar->bitOffset + pHmiVar->len + 7 ) / 8 );
                    dataLen += ( pHmiVar->bitOffset + pHmiVar->len + 7 ) / 8;
                }
                else
                {
                    pPDU_NETR->Para.Vars[i].VarAddr.NumElements = conver_16bit( pHmiVar->len );
                    dataLen += pHmiVar->len;
                }
                break;
            /*case PPI_AP_AREA_T_IEC:
                pPDU_NETR->Para.Vars[i].VarAddr.Type = PPI_AP_TYPE_T_IEC;
                Q_ASSERT(( pHmiVar->len % 4 ) == 0 );
                pPDU_NETR->Para.Vars[i].VarAddr.NumElements = conver_16bit(( pHmiVar->len + 3 ) / 4 );
                pPDU_NETR->Para.Vars[i].VarAddr.Offset = conver_32bit( pHmiVar->addOffset ) >> 8;
                dataLen += 5 * (( pHmiVar->len + 3 ) / 4 );
                break;
            case PPI_AP_AREA_C_IEC:
                pPDU_NETR->Para.Vars[i].VarAddr.Type = PPI_AP_TYPE_C_IEC;
                Q_ASSERT(( pHmiVar->len % 2 ) == 0 );
                pPDU_NETR->Para.Vars[i].VarAddr.NumElements = conver_16bit(( pHmiVar->len + 1 ) / 2 );
                pPDU_NETR->Para.Vars[i].VarAddr.Offset = conver_32bit( pHmiVar->addOffset ) >> 8;
                dataLen += 3 * (( pHmiVar->len + 1 ) / 2 );
                break;*/
            case PPI_AP_AREA_T:
                pPDU_NETR->Para.Vars[i].VarAddr.Type = PPI_AP_TYPE_T;
                //pPDU_NETR->Para.Vars[i].VarAddr.Offset = conver_32bit( pHmiVar->addOffset ) >> 8;
                pPDU_NETR->Para.Vars[i].VarAddr.Offset[0] = ( pHmiVar->addOffset * 8 )>> 16 & 0xFF;
                pPDU_NETR->Para.Vars[i].VarAddr.Offset[1] = ( pHmiVar->addOffset * 8 )>>  8 & 0xFF;
                pPDU_NETR->Para.Vars[i].VarAddr.Offset[2] = ( pHmiVar->addOffset * 8 )      & 0xFF;
                if ( pHmiVar->dataType == Com_Bool )
                {
                    Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 7 );
                    pPDU_NETR->Para.Vars[i].VarAddr.NumElements = conver_16bit((( pHmiVar->bitOffset + pHmiVar->len + 7 ) / 8 + 1) / 2 );
                    dataLen += ( pHmiVar->bitOffset + pHmiVar->len + 7 ) / 8;
                }
                else
                {
                    pPDU_NETR->Para.Vars[i].VarAddr.NumElements = conver_16bit( (pHmiVar->len + 1)/2 );
                    dataLen += pHmiVar->len;
                }
                break;
            case PPI_AP_AREA_C:
                pPDU_NETR->Para.Vars[i].VarAddr.Type = PPI_AP_TYPE_C;
                //pPDU_NETR->Para.Vars[i].VarAddr.Offset = conver_32bit( pHmiVar->addOffset ) >> 8;
                pPDU_NETR->Para.Vars[i].VarAddr.Offset[0] = ( pHmiVar->addOffset * 8 )>> 16 & 0xFF;
                pPDU_NETR->Para.Vars[i].VarAddr.Offset[1] = ( pHmiVar->addOffset * 8 )>>  8 & 0xFF;
                pPDU_NETR->Para.Vars[i].VarAddr.Offset[2] = ( pHmiVar->addOffset * 8 )      & 0xFF;
                if ( pHmiVar->dataType == Com_Bool )
                {
                    Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 7 );
                    pPDU_NETR->Para.Vars[i].VarAddr.NumElements = conver_16bit((( pHmiVar->bitOffset + pHmiVar->len + 7 ) / 8 + 1)/2 );
                    dataLen += ( pHmiVar->bitOffset + pHmiVar->len + 7 ) / 8;
                }
                else
                {
                    pPDU_NETR->Para.Vars[i].VarAddr.NumElements = conver_16bit(( pHmiVar->len+1)/2 );
                    dataLen += pHmiVar->len;
                }
                break;
            case PPI_AP_AREA_HC_IEC:
            default:
                qDebug()<<"err AREA "<<pHmiVar->area;
                Q_ASSERT( false );
                return -1;
            }
            if ( pHmiVar->area == PPI_AP_AREA_DB )
                pPDU_NETR->Para.Vars[i].VarAddr.Subarea  = conver_16bit( pHmiVar->subArea );
            else
                pPDU_NETR->Para.Vars[i].VarAddr.Subarea  = conver_16bit( 0x0000 );

            pPDU_NETR->Para.Vars[i].VarAddr.Area = pHmiVar->area;
            totallen += dataLen;
        }
        Q_ASSERT( totallen <= MAX_PPI_FRAME_LEN );
        pPDU_NETR->Para.VarNum = pConnect->requestVarsList.count();

        pPDU_NETR->Header.ParLen = conver_16bit( 2 + sizeof( PPI_NETR_REQ_VAR_STRU ) * pConnect->requestVarsList.count() );
        pL2Msg->PduLen = sizeof( PPI_PDU_REQ_HEADER ) + ( INT8U )( 2 + sizeof( PPI_NETR_REQ_VAR_STRU ) * pConnect->requestVarsList.count() );
    }
    //m_devUseCnt++;

//    int ret = write( m_fd, buf, MAX_LONG_BUF_LEN + 1024 );     //填写完毕后，发给L2处理
    m_rwSocket->writeDev((char *)buf, MAX_SHORT_BUF_LEN+1024/*pL2Msg->PduLen*/);
//    Q_ASSERT( ret >= 0 );

    return pConnect->requestVarsList.count();
}

/******************************************************************************
** 函数GetMpiRspParaPtr
** 说明  : 得到SD2应答消息的Para开始地址
** 输入  : pMsg : 消息的开始地址
** 输出  : NA
** 返回  : Para的开始地址
******************************************************************************/
static INT8U *GetMpiRspParaPtr( INT8U *pMsg )
{
    return ( pMsg + sizeof( PPI_PDU_RSP_HEADER ) );
}

/******************************************************************************
** 函数GetMpiRspDataPtr
** 说明  : 得到SD2应答消息的data开始地址
** 输入  : pMsg : 消息的开始地址
** 输出  : NA
** 返回  : data的开始地址
******************************************************************************/
static INT8U *GetMpiRspDataPtr( INT8U *pMsg )
{
    PPI_PDU_RSP_HEADER *pHeaderAck = ( PPI_PDU_RSP_HEADER * )pMsg;
    return ( pMsg + sizeof( PPI_PDU_RSP_HEADER ) + conver_16bit( pHeaderAck->ParLen ) );
}

/******************************************************************************
** 函数 GetRaundSyc
** 说明  : 获取返回值的类型
** 输入  :
** 输出  :
** 返回  : 失败
******************************************************************************/
static int GetRaundSyc( INT8U datatype )
{
    switch ( datatype )
    {
    case D_TYPE_ERROR:
        return -1;
    case D_TYPE_BIT:    //是位访问//位变量访问已经整合到字节变量
        return -1;
    case D_TYPE_BYTES:    //字节变量
        return 3;
    case D_TYPE_LONG:    //针对西门子的T变量
        return 0;
    default:
        return -1;
    }
}

int Mpi300Service::processMulReadRsp( char *pCharMsg, struct Mpi300Connect* pConnect )
{
    L7_MSG_STRU *pMsg = (L7_MSG_STRU*)pCharMsg;
    PPI_PDU_RSP_HEADER *pHeaderAck = NULL;
    PPI_PDU_RSP_PARA_READ * pParaAck = NULL;
    INT8U *pDataAck =  NULL;
    VAR_VALUE_STRU *pValue;
    INT8U *pPdu = pMsg->pPdu;
    struct ComMsg comMsg;
    int index = 0;

    Q_ASSERT( pMsg->Saddr == pConnect->cfg.cpuAddr );
    Q_ASSERT( pMsg && pConnect );

    if ( pMsg->Err == NETWR_NO_ERR )
    {
        pHeaderAck = ( PPI_PDU_RSP_HEADER * )( pPdu );
        pParaAck = ( PPI_PDU_RSP_PARA_READ * )GetMpiRspParaPtr( pPdu );
        pDataAck = GetMpiRspDataPtr( pPdu );
        if (( pHeaderAck->ErrCls == 0 ) && ( pHeaderAck->ErrCod == 0 ) )
        {
            if ( pConnect->requestVar.addOffset >= 0 && pConnect->requestVar.len > 0 ) //合并
            {
                if ( pParaAck->VarNum == 1 )
                {
                    pValue = ( VAR_VALUE_STRU * )( pDataAck + 0 );
                    int RaundSyc = GetRaundSyc( pValue->DataType );
                    if ( pValue->AccRslt == ACC_RSLT_NO_ERR && RaundSyc >= 0 )
                    {
                        INT8U len = ( INT8U )( conver_16bit( pValue->Length ) >> RaundSyc );
                        for ( ; index < pConnect->requestVarsList.count(); index++ )
                        {
                            struct HmiVar* pHmiVar = pConnect->requestVarsList[index];
                            Q_ASSERT( pConnect->requestVar.area == pHmiVar->area );
                            pHmiVar->err = VAR_NO_ERR;
                            pHmiVar->overFlow = false;
                            pConnect->connStatus = CONNECT_LINKED_OK;

                            int offset = pHmiVar->addOffset - pConnect->requestVar.addOffset;
                            Q_ASSERT( offset >= 0 );
                            int dataBytes = 0;
                            /*if ( pHmiVar->area == PPI_AP_AREA_T_IEC )
                            {
                                offset = 5 * offset;
                                dataBytes = (( pHmiVar->len + 3 ) / 4 ) * 5;
                            }
                            else if ( pHmiVar->area == PPI_AP_AREA_C_IEC )
                            {
                                offset = 3 * offset;
                                dataBytes = (( pHmiVar->len + 1 ) / 2 ) * 3;
                            }
                            else
                            {*/
                                if ( pHmiVar->dataType == Com_Bool )
                                    dataBytes = ( pHmiVar->bitOffset + pHmiVar->len + 7 ) / 8;
                                else
                                    dataBytes = pHmiVar->len;
                            //}
                            if ( offset + dataBytes <= len )
                            {
                                if ( !pConnect->waitWrtieVarsList.contains( pHmiVar ) )
                                {
                                    if ( pHmiVar->rData.size() < dataBytes )
                                        pHmiVar->rData = QByteArray( dataBytes, 0 );
                                    memcpy( pHmiVar->rData.data(), ( INT8U* )pValue->Var + offset, dataBytes );
                                    //printf("get2 %04X",( INT8U * )pValue->Var + offset);
                                    //qDebug()<<"get data "<<pHmiVar->rData.toHex();
                                    //printf("getp data %02X%02X\n",*( INT8U* )pValue->Var,*((( INT8U* )pValue->Var)+1));
                                    comMsg.varId = ( pHmiVar->varId & 0x0000ffff ); //防止长变
                                    comMsg.type = 0; //0是read;
                                    comMsg.err = pHmiVar->err;
                                    m_mutexGetMsg.lock();
                                    m_comMsgTmp.insert( comMsg.varId, comMsg );
                                    m_mutexGetMsg.unlock();
                                }
                            }
                            else
                            {
                                qDebug() << "data length did not match";
                                qDebug() << "len" << len;
                                qDebug() << "pConnect->requestVar.len" << pConnect->requestVar.len;
                                qDebug() << "unexpect read data packet"<<" MsgType "<<pMsg->MsgType<<" addr "<<pMsg->Saddr<<" err "<<pMsg->Err<<" isLinking "<<pConnect->isLinking;

                                for ( int loop = 0; loop < pMsg->PduLen; loop ++ )
                                {
                                    qDebug() << QString::number( pMsg->pPdu[loop], 16 );
                                }
                                //Q_ASSERT( false );
                            }
                        }
                    }
                    else                     //参数错误
                    {
                        for ( ; index < pConnect->requestVarsList.count(); index++ )
                        {
                            struct HmiVar* pHmiVar = pConnect->requestVarsList[index];
                            pHmiVar->overFlow = true;
                            if ( pConnect->requestVarsList.count() > 1 )
                                pHmiVar->lastUpdateTime = -1; //合并后越界的不算越界，立即再次更新（更新时不合并
                            else
                            {
                                pHmiVar->err = VAR_PARAM_ERROR;
                                comMsg.varId = ( pHmiVar->varId & 0x0000ffff ); //防止长变
                                comMsg.type = 0; //0是read;
                                comMsg.err = VAR_PARAM_ERROR;
                                m_mutexGetMsg.lock();
                                m_comMsgTmp.insert( comMsg.varId, comMsg );
                                m_mutexGetMsg.unlock();
                            }
                        }
                    }
                }
            }
            else
            {
                if ( pParaAck->VarNum == pConnect->requestVarsList.count() )
                {
                    for ( INT8U offset = 0; index < pParaAck->VarNum; index++ )
                    {
                        struct HmiVar* pHmiVar = pConnect->requestVarsList[index];
                        pValue = ( VAR_VALUE_STRU * )( pDataAck + offset );
                        if ( pValue->AccRslt == ACC_RSLT_NO_ERR )
                        {
                            int RaundSyc = GetRaundSyc( pValue->DataType );
                            if ( RaundSyc >= 0 )
                            {
                                INT8U len = ( INT8U )( conver_16bit( pValue->Length ) >> RaundSyc );
                                if ( len & 0x01 ) // 如果长度是奇
                                    offset += len + 4 + 1;  // 4 is header
                                else
                                    offset += len + 4;
                                if ( !pConnect->waitWrtieVarsList.contains( pHmiVar ) )
                                {
                                    if ( pHmiVar->rData.size() < len )
                                        pHmiVar->rData = QByteArray( len, 0 );
                                    memcpy( pHmiVar->rData.data(), ( INT8U * )pValue->Var, len );
                                    //printf("get %04X",( INT8U * )pValue->Var);
                                    //qDebug()<<"get data2 "<<pHmiVar->rData.toHex();
                                    comMsg.varId = ( pHmiVar->varId & 0x0000ffff ); 
                                    comMsg.type = 0; //0是read;
                                    comMsg.err = pHmiVar->err;
                                    m_mutexGetMsg.lock();
                                    m_comMsgTmp.insert( comMsg.varId, comMsg );
                                    m_mutexGetMsg.unlock();
                                }
                                pHmiVar->err = VAR_NO_ERR;
                                pHmiVar->overFlow = false;
                                pConnect->connStatus = CONNECT_LINKED_OK;
                                continue;
                            }
                        }
                        offset += 4;  // 4 is header,单个变量的错误不能影响后面变
                        pHmiVar->err = VAR_PARAM_ERROR;
                        pHmiVar->overFlow = true;
                    }
                }
                else
                {
                    qDebug() << "varible Num did not match2";
                    qDebug() << "pParaAck->VarNum" << pParaAck->VarNum;
                    qDebug() << "pConnect->requestVarsList.count()" << pConnect->requestVarsList.count();
                    qDebug() << "unexpect read data packet"<<" MsgType "<<pMsg->MsgType<<" addr "<<pMsg->Saddr<<" err "<<pMsg->Err<<" isLinking "<<pConnect->isLinking;

                    for ( int loop = 0; loop < pMsg->PduLen; loop ++ )
                    {
                        qDebug() << QString::number( pMsg->pPdu[loop], 16 );
                    }
                    //Q_ASSERT( false );
                }
            }
        }
    }
    else if ( pMsg->Err == PPI_ERR_TIMEROUT )
    {
        pConnect->connStatus = CONNECT_NO_LINK;
        comMsg.varId = 65535; //防止长变
        comMsg.type = 0; //0是read;
        comMsg.err = VAR_DISCONNECT;
        m_mutexGetMsg.lock();
        m_comMsgTmp.insert( comMsg.varId, comMsg );
        m_mutexGetMsg.unlock();
        foreach( struct HmiVar* tmpHmiVar, pConnect->actVarsList )
        {
            comMsg.varId = tmpHmiVar->varId; //防止长变
            comMsg.type = 0; //0是read;
            comMsg.err = VAR_DISCONNECT;
            //tmpHmiVar->err = VAR_COMM_TIMEOUT;
            //tmpHmiVar->overFlow = false;
            tmpHmiVar->lastUpdateTime = -1;
            m_mutexGetMsg.lock();
            m_comMsgTmp.insert( comMsg.varId, comMsg );
            m_mutexGetMsg.unlock();
        }
    }
    else /*if ( pMsg->Err == PPI_ERR_OFFLINE )*/
    {    qDebug()<<"Err 300";
        pConnect->connStatus = CONNECT_NO_LINK;
        pConnect->requestVarsList.clear();
        return -1;
    }

    for ( ; index < pConnect->requestVarsList.count(); index++ )
    {
        struct HmiVar* pHmiVar = pConnect->requestVarsList[index];
        pHmiVar->err = VAR_COMM_TIMEOUT;
        comMsg.varId = ( pHmiVar->varId & 0x0000ffff ); //防止长变
        comMsg.type = 0; //0是read;
        comMsg.err = VAR_DISCONNECT;
        m_mutexGetMsg.lock();
        m_comMsgTmp.insert( comMsg.varId, comMsg );
        m_mutexGetMsg.unlock();
    }
    pConnect->requestVarsList.clear();
    return 0;
}

int Mpi300Service::processMulWriteRsp(  char *pCharMsg, struct Mpi300Connect* pConnect )
{
    L7_MSG_STRU *pMsg =(L7_MSG_STRU*)pCharMsg;
    PPI_PDU_RSP_HEADER *pHeaderAck = NULL;
    PPI_PDU_RSP_PARA_READ * pParaAck = NULL;
    INT8U *pDataAck =  NULL;
    INT8U *pPdu = pMsg->pPdu;
    struct ComMsg comMsg;
    Q_ASSERT( pMsg->Saddr == pConnect->cfg.cpuAddr );
    Q_ASSERT( pMsg && pConnect );

    int index = 0;

    if ( pMsg->Err == NETWR_NO_ERR )
    {
        pHeaderAck = ( PPI_PDU_RSP_HEADER * )( pPdu );
        pParaAck = ( PPI_PDU_RSP_PARA_READ * )GetMpiRspParaPtr( pPdu );
        pDataAck = GetMpiRspDataPtr( pPdu );
        if (( pHeaderAck->ErrCls == 0 ) && ( pHeaderAck->ErrCod == 0 ) )
        {
            if ( pParaAck->VarNum == pConnect->writingCnt )
            {
                for ( ; index < pParaAck->VarNum; index++ )
                {
                    struct HmiVar* pHmiVar = pConnect->waitWrtieVarsList[index];
                    if ( pDataAck[index] == ACC_RSLT_NO_ERR )
                    {
                        pHmiVar->wStatus = VAR_NO_ERR;
                    }
                    else
                    {
                        pHmiVar->wStatus = VAR_PARAM_ERROR;
                    }
                    comMsg.varId = ( pHmiVar->varId & 0x0000ffff ); //防止长变
                    comMsg.type = 0; //0是read;
                    comMsg.err = pHmiVar->err;
                    m_mutexGetMsg.lock();
                    m_comMsgTmp.insert( comMsg.varId, comMsg );
                    m_mutexGetMsg.unlock();
                }
            }
            else
            {
                qDebug() << "unexpect write data packet"<<" MsgType "<<pMsg->MsgType<<" addr "<<pMsg->Saddr<<" err "<<pMsg->Err<<" isLinking "<<pConnect->isLinking;
                //Q_ASSERT( false );
            }
        }
    }
    else
    {
        for ( ; index < pConnect->writingCnt; index++ )
        {
            struct HmiVar* pHmiVar = pConnect->waitWrtieVarsList[index];
            pHmiVar->wStatus = VAR_COMM_TIMEOUT;
            comMsg.varId = ( pHmiVar->varId & 0x0000ffff ); //防止长变
            comMsg.type = 0; //0是read;
            comMsg.err = pHmiVar->err;
            m_mutexGetMsg.lock();
            m_comMsgTmp.insert( comMsg.varId, comMsg );
            m_mutexGetMsg.unlock();
        }
    }

    for ( int i = 0; i < pConnect->writingCnt; i++ )
        pConnect->waitWrtieVarsList.takeAt( 0 );
    pConnect->writingCnt = 0;

    return 0;
}

bool Mpi300Service::trySndWriteReq( struct Mpi300Connect* pConnect, QHash<int , struct ComMsg> &comMsgTmp ) //不处理超长变
{
    if ( pConnect->writingCnt == 0 && pConnect->requestVarsList.count() == 0 ) //这个连接没有读写
    {
        if ( pConnect->isLinking == false )
        {
            if ( pConnect->connStatus != CONNECT_LINKED_OK && pConnect->connStatus != CONNECT_LINKING )
            {                        //需要先建立连接
                createMpiConnLink( pConnect );
                return true;
            }
        }
        else
        {
            if ( pConnect->connStatus == CONNECT_NO_OPERATION )
            {
                int currTime = m_currTime; //2.5秒钟没有建立连接成功就认为连接失
                if ( calIntervalTime( pConnect->beginTime, currTime ) >= 2500 )
                {
                    struct ComMsg comMsg;
                    pConnect->connStatus = CONNECT_NO_LINK;
                    comMsg.varId = 65535; //防止长变
                    comMsg.type = 0; //0是read;
                    comMsg.err = VAR_DISCONNECT;
                    m_mutexGetMsg.lock();
                    m_comMsgTmp.insert( comMsg.varId, comMsg );
                    m_mutexGetMsg.unlock();
                    foreach( struct HmiVar* tmpHmiVar, pConnect->actVarsList )
                    {
                        comMsg.varId = tmpHmiVar->varId; //防止长变
                        comMsg.type = 0; //0是read;
                        comMsg.err = VAR_DISCONNECT;
                        //tmpHmiVar->err = VAR_COMM_TIMEOUT;
                        //tmpHmiVar->overFlow = false;
                        tmpHmiVar->lastUpdateTime = -1;
                        m_mutexGetMsg.lock();
                        m_comMsgTmp.insert( comMsg.varId, comMsg );
                        m_mutexGetMsg.unlock();
                    }
                }
            }
        }
        m_mutexWReq.lock();
        if ( pConnect->connStatus == CONNECT_LINKED_OK || pConnect->connStatus == CONNECT_LINKING )
        {
            if ( pConnect->waitWrtieVarsList.count() > 0 )
            {
                struct HmiVar* pHmiVar = pConnect->waitWrtieVarsList[0];
                int writeCnt = sndWriteVarMsgToDriver( pConnect, pHmiVar );

                Q_ASSERT( writeCnt > 0 );

                if ( writeCnt > 0 )
                {
                    pConnect->writingCnt = writeCnt;    //进入写状
                    m_mutexWReq.unlock();
                    return true;
                }
                else
                {
                    pConnect->waitWrtieVarsList.takeAt( 0 );//出列,丢掉写请
                    pHmiVar->wStatus = VAR_PARAM_ERROR;
                }
            }
        }
        else if ( pConnect->connStatus == CONNECT_NO_LINK )
        {
            if ( pConnect->waitWrtieVarsList.count() > 0 )
            {
                struct HmiVar* pHmiVar = pConnect->waitWrtieVarsList.takeAt( 0 );//出列,丢掉写请
                pHmiVar->err = VAR_COMM_TIMEOUT;
                pHmiVar->wStatus = VAR_COMM_TIMEOUT;
            }
        }
        m_mutexWReq.unlock();

    }
    return false;
}

bool Mpi300Service::trySndReadReq( struct Mpi300Connect* pConnect, QHash<int , struct ComMsg> &comMsgTmp ) //不处理超长变
{
    struct ComMsg comMsg;

    if ( pConnect->writingCnt == 0 && pConnect->requestVarsList.count() == 0 )
    {                                                    //这个连接没有读写
        if ( pConnect->combinVarsList.count() > 0 )
        {
            if ( pConnect->isLinking == false )
            {
                if ( pConnect->connStatus != CONNECT_LINKED_OK  && pConnect->connStatus != CONNECT_LINKING  )
                {                        //需要先建立连接
                    createMpiConnLink( pConnect );
                    return true;
                }
            }
            else
            {
                if ( pConnect->connStatus == CONNECT_NO_OPERATION )
                {
                    int currTime = m_currTime; //2.5秒钟没有建立连接成功就认为连接失
                    if ( calIntervalTime( pConnect->beginTime, currTime ) >= 2500 )
                    {
                        pConnect->connStatus = CONNECT_NO_LINK;
                        comMsg.varId = 65535; //防止长变
                        comMsg.type = 0; //0是read;
                        comMsg.err = VAR_DISCONNECT;
                        m_mutexGetMsg.lock();
                        m_comMsgTmp.insert( comMsg.varId, comMsg );
                        m_mutexGetMsg.unlock();
                        foreach( struct HmiVar* tmpHmiVar, pConnect->actVarsList )
                        {
                            comMsg.varId = tmpHmiVar->varId; //防止长变
                            comMsg.type = 0; //0是read;
                            comMsg.err = VAR_DISCONNECT;
                            //tmpHmiVar->err = VAR_COMM_TIMEOUT;
                            //tmpHmiVar->overFlow = false;
                            tmpHmiVar->lastUpdateTime = -1;
                            m_mutexGetMsg.lock();
                            m_comMsgTmp.insert( comMsg.varId, comMsg );
                            m_mutexGetMsg.unlock();
                        }
                    }
                }
            }
            if ( pConnect->connStatus == CONNECT_LINKED_OK || pConnect->connStatus == CONNECT_LINKING )
            {
                pConnect->requestVarsList = pConnect->combinVarsList;
                pConnect->requestVar = pConnect->combinVar;
                for ( int i = 0; i < pConnect->combinVarsList.count(); i++ )
                {                      //要把已经请求的变量从等待队列中
                    struct HmiVar* pHmiVar = pConnect->combinVarsList[i];
                    pConnect->waitUpdateVarsList.removeAll( pHmiVar );
                }
                pConnect->combinVarsList.clear();

                if ( sndReadVarMsgToDriver( pConnect, &( pConnect->requestVar ) ) > 0 )
                {
                    return true;
                }
                else
                {
                    for ( int i = 0; i < pConnect->requestVarsList.count(); i++ )
                    {
                        struct HmiVar* pHmiVar = pConnect->requestVarsList[i];
                        pHmiVar->err = VAR_PARAM_ERROR;
                    }
                    pConnect->requestVarsList.clear();
                }
            }
            else if ( pConnect->connStatus == CONNECT_NO_LINK )
            {
                pConnect->requestVarsList = pConnect->combinVarsList;
                for ( int i = 0; i < pConnect->combinVarsList.count(); i++ )
                {                      //要把已经请求的变量从等待队列中
                    struct HmiVar* pHmiVar = pConnect->combinVarsList[i];
                    pConnect->waitUpdateVarsList.removeAll( pHmiVar );
                }
                pConnect->combinVarsList.clear();
                for ( int i = 0; i < pConnect->requestVarsList.count(); i++ )
                {
                    struct HmiVar* pHmiVar = pConnect->requestVarsList[i];
                    pHmiVar->err = VAR_COMM_TIMEOUT;
                    comMsg.varId = ( pHmiVar->varId & 0x0000ffff ); //防止长变
                    comMsg.type = 0; //0是read;
                    comMsg.err = VAR_COMM_TIMEOUT;
                    m_mutexGetMsg.lock();
                    m_comMsgTmp.insert( comMsg.varId, comMsg );
                    m_mutexGetMsg.unlock();
                }
                pConnect->requestVarsList.clear();
            }
        }
    }
    return false;
}

bool Mpi300Service::updateWriteReq( struct Mpi300Connect* pConnect )
{
    bool newUpdateVarFlag = false;
    QList<struct HmiVar*> tmpList;
    m_mutexWReq.lock();

    for ( int i = 0; i < pConnect->wrtieVarsList.count(); i++ )
    {                                  // 找出需要刷新的变量
        struct HmiVar* pHmiVar = pConnect->wrtieVarsList[i];


        if ( !pConnect->waitWrtieVarsList.contains( pHmiVar ) )
        {
            pConnect->waitWrtieVarsList.append( pHmiVar );
            newUpdateVarFlag = true;
        }
        else
        {
            tmpList.append(pHmiVar);
        }
    }

    pConnect->wrtieVarsList.clear() ;
    pConnect->wrtieVarsList += tmpList;

    m_mutexWReq.unlock();

    return newUpdateVarFlag;
}

bool Mpi300Service::updateReadReq( struct Mpi300Connect* pConnect )
{
    bool newUpdateVarFlag = false;
    m_mutexUpdateVar.lock();
    int currTime = m_currTime;
    for ( int i = 0; i < pConnect->actVarsList.count(); i++ )
    {                                  // 找出需要刷新的变量
        struct HmiVar* pHmiVar = pConnect->actVarsList[i];
        if ( pHmiVar->lastUpdateTime == -1 || ( calIntervalTime( pHmiVar->lastUpdateTime, currTime ) >= pHmiVar->cycleTime ) )
        {                              // 需要刷
            if (( !pConnect->waitUpdateVarsList.contains( pHmiVar ) )
                            && ( !pConnect->waitWrtieVarsList.contains( pHmiVar ) )
                            // && ( !pConnect->combinVarsList.contains( pHmiVar ) )
                            && ( !pConnect->requestVarsList.contains( pHmiVar ) ) )
            {
                pHmiVar->lastUpdateTime = currTime;
                pConnect->waitUpdateVarsList.append( pHmiVar );
                newUpdateVarFlag = true;
            }
        }
    }

    m_mutexUpdateVar.unlock();
    return newUpdateVarFlag;
}

bool Mpi300Service::updateOnceReadReadReq( struct Mpi300Connect* pConnect )
{
    bool newUpdateVarFlag = false;
    m_mutexOnceReal.lock();

    for ( int i = 0; i < pConnect->onceRealVarsList.count(); i++ )
    {                                  // 找出需要刷新的变量
        struct HmiVar* pHmiVar = pConnect->onceRealVarsList[i];
        if (( !pConnect->waitUpdateVarsList.contains( pHmiVar ) )
                        && ( !pConnect->waitWrtieVarsList.contains( pHmiVar ) ) //写的时候还是要去读该变
                        //&& ( !pConnect->combinVarsList.contains( pHmiVar ) )    有可能被清除
                        && ( !pConnect->requestVarsList.contains( pHmiVar ) ) )
        {
            pConnect->waitUpdateVarsList.prepend( pHmiVar );  //插入队列最
            newUpdateVarFlag = true;
        }
        else
        {
            if ( pConnect->waitUpdateVarsList.contains( pHmiVar ) )
            {
                //提前
                pConnect->waitUpdateVarsList.removeAll( pHmiVar );
                pConnect->waitUpdateVarsList.prepend( pHmiVar );
            }
        }
    }
    pConnect->onceRealVarsList.clear();// 清楚一次读列表
    m_mutexOnceReal.unlock();

    return newUpdateVarFlag;
}

void Mpi300Service::combineReadReq( struct Mpi300Connect* pConnect )
{
    QList<struct HmiVar*> combinVarsList; //合并待请求的变量List
    QList<struct HmiVar*> groupVarsList;  //组合待请求的变量List
    int lenCombine = 0;
    int lenGroup = 0;

    if ( pConnect->waitUpdateVarsList.count() >= 1 )  //暂时先组
    {
        struct HmiVar* pHmiVar = pConnect->waitUpdateVarsList[0];
        combinVarsList.append( pHmiVar );
        groupVarsList.append( pHmiVar );
        pConnect->combinVar = *pHmiVar;

        /*if ( pHmiVar->area == PPI_AP_AREA_T_IEC )
        {
            pConnect->combinVar.len = (( pHmiVar->len + 3 ) / 4 ) * 4;
            lenCombine = (( pHmiVar->len + 3 ) / 4 ) * 5;
            lenGroup = (( pHmiVar->len + 3 ) / 4 ) * 5;
        }
        else if ( pHmiVar->area == PPI_AP_AREA_C_IEC )
        {
            pConnect->combinVar.len = (( pHmiVar->len + 1 ) / 2 ) * 2;
            lenCombine = (( pHmiVar->len + 1 ) / 2 ) * 3;
            lenGroup = (( pHmiVar->len + 1 ) / 2 ) * 3;
        }
        else
        {*/
            Q_ASSERT( pHmiVar->area != PPI_AP_AREA_HC_IEC );
            if ( pHmiVar->dataType == Com_Bool )
            {
                Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 7 );


                pConnect->combinVar.dataType = Com_Byte;
                pConnect->combinVar.len = ( pHmiVar->bitOffset + pHmiVar->len + 7 ) / 8;
                pConnect->combinVar.bitOffset = -1;
            }
            lenCombine = pConnect->combinVar.len ;
            lenGroup = pConnect->combinVar.len;
        //}
    }
    for ( int i = 1; i < pConnect->waitUpdateVarsList.count(); i++ )
    {
        struct HmiVar* pHmiVar = pConnect->waitUpdateVarsList[i];
        if ( pHmiVar->area == pConnect->combinVar.area &&
             pHmiVar->subArea == pConnect->combinVar.subArea &&
            pConnect->combinVar.overFlow == pHmiVar->overFlow &&
            ( pHmiVar->overFlow == false ) )
        {                               //没有越界才合并，否则不
            int startOffset = qMin( pConnect->combinVar.addOffset, pHmiVar->addOffset );
            int endOffset = 0;
            int newLen = 0;
            /*if ( pConnect->combinVar.area == PPI_AP_AREA_T_IEC )
            {
                int combineCount = ( pConnect->combinVar.len + 3 ) / 4;
                int varCount = (( pHmiVar->len + 3 ) / 4 );
                endOffset = qMax( pConnect->combinVar.addOffset + combineCount, pHmiVar->addOffset + varCount );
                newLen = 5 * ( endOffset - startOffset );
                if ( newLen <= MAX_PPI_FRAME_LEN && newLen <= 5*( combineCount + varCount ) + 12 )
                {
                    pConnect->combinVar.addOffset = startOffset;
                    pConnect->combinVar.len = 4 * ( endOffset - startOffset );
                    combinVarsList.append( pHmiVar );
                    lenCombine += 5 * varCount;
                }
            }
            else if ( pConnect->combinVar.area == PPI_AP_AREA_C_IEC )
            {
                int combineCount = ( pConnect->combinVar.len + 1 ) / 2;
                int varCount = (( pHmiVar->len + 1 ) / 2 );
                endOffset = qMax( pConnect->combinVar.addOffset + combineCount, pHmiVar->addOffset + varCount );
                newLen = 3 * ( endOffset - startOffset );
                if ( newLen <= MAX_PPI_FRAME_LEN && newLen <= 3*( combineCount + varCount ) + 12 )
                {
                    pConnect->combinVar.addOffset = startOffset;
                    pConnect->combinVar.len = 2 * ( endOffset - startOffset );
                    combinVarsList.append( pHmiVar );
                    lenCombine += 3 * varCount;
                }
            }
            else
            {*/
                Q_ASSERT( pHmiVar->area != PPI_AP_AREA_HC_IEC );
                int combineCount = pConnect->combinVar.len;
                int varCount = 0;
                if ( pHmiVar->dataType == Com_Bool )
                    varCount = ( pHmiVar->bitOffset + pHmiVar->len + 7 ) / 8;
                else
                    varCount = pHmiVar->len;
                endOffset = qMax( pConnect->combinVar.addOffset + combineCount, pHmiVar->addOffset + varCount );
                newLen = ( endOffset - startOffset );
                if ( newLen <= MAX_PPI_FRAME_LEN && newLen <= ( combineCount + varCount ) + 12 )
                {
                    pConnect->combinVar.addOffset = startOffset;
                    pConnect->combinVar.len = newLen;
                    combinVarsList.append( pHmiVar );
                    lenCombine += varCount;
                }
            //}
        }
        //组合
        int dataLen; // = 0;//= 12 + 4;
        /*if (  pHmiVar->area == PPI_AP_AREA_T_IEC )
            dataLen = 5 * (( pHmiVar->len + 3 ) / 4 );
        else if (pHmiVar->area == PPI_AP_AREA_C_IEC )
            dataLen = 3 * (( pHmiVar->len + 1 ) / 2 );
        else
        {*/
            Q_ASSERT( pHmiVar->area != PPI_AP_AREA_HC_IEC );
            if ( pHmiVar->dataType == Com_Bool )
                dataLen = ( pHmiVar->bitOffset + pHmiVar->len + 7 ) / 8;
            else
                dataLen = pHmiVar->len;
        //}
        if (( lenGroup + dataLen + ( i + 1 )*12 <= MAX_PPI_FRAME_LEN ) && groupVarsList.count() < 10 )
        {
            groupVarsList.append( pHmiVar );
            lenGroup += dataLen;
        }
    }
    if ( lenCombine >= lenGroup )  //哪个长就用哪
        pConnect->combinVarsList = combinVarsList;
    else
    {
        pConnect->combinVarsList = groupVarsList;
        pConnect->combinVar.addOffset = -1;
        pConnect->combinVar.len = -1;
    }
}

bool Mpi300Service::hasProcessingReq( struct Mpi300Connect* pConnect )
{
    return ( pConnect->writingCnt > 0 || pConnect->requestVarsList.count() > 0 /*|| pConnect->isLinking */);
}

void Mpi300Service::initPpiMpiDriver()
{

    INT8U buf[MAX_SHORT_BUF_LEN];
    memset(buf, 0, MAX_SHORT_BUF_LEN);
    L2_MSG_STRU *pL2Msg = ( L2_MSG_STRU * ) & buf;
    struct Mpi300Connect* conn = m_connectList[0];
    pL2Msg->Port = 0;    //默认固
    pL2Msg->MsgMem = ShortBufMem;
    pL2Msg->MsgType = 6;
    pL2Msg->unit.L2FmaPara.IfMaster = true;
    pL2Msg->unit.L2FmaPara.RetryCount = 3;
    pL2Msg->unit.L2FmaPara.BaudRate = conn->cfg.baud;
    pL2Msg->unit.L2FmaPara.Addr = conn->cfg.hmiAddr;
    pL2Msg->unit.L2FmaPara.Hsa = PPI_HSA;//32;conn->cfg.hsa;
    pL2Msg->unit.L2FmaPara.GapFactor = 1;//GAP_FACTOR;
    pL2Msg->unit.L2FmaPara.DA =0xFF;//conn->cfg.cpuAddr;//PPI_NULL_ADDR;
    pL2Msg->unit.L2FmaPara.SetProtol = 0;//MPI_PROTO_M;

    pL2Msg->Saddr = 0xFF; /*conn->cfg.cpuAddr;*///PPI_NULL_ADDR;
    pL2Msg->Frome = 0xAA;

    m_isInitPPIMsgSend = true;
    m_rwSocket->writeDev((char*)buf,MAX_SHORT_BUF_LEN );
}

bool Mpi300Service::initCommDev()
{

    if ( m_connectList.count() > 0 )
    {
        struct Mpi300Connect* conn = m_connectList[0];

        UartCfg uartCfg;
        uartCfg.COMNum = conn->cfg.COM_interface/* == 0*/;
        uartCfg.baud = conn->cfg.baud;
        uartCfg.dataBit = 8;
        uartCfg.parity = 2;
        uartCfg.stopBit = 1;

        m_rwSocket = new RWSocket(MPI300, &uartCfg, conn->cfg.id);
        connect( m_rwSocket, SIGNAL( readyRead() ),this, SLOT( slot_readCOMData( ) ), Qt::QueuedConnection) ;
        m_rwSocket->openDev();
        iniDriverFlg = false;
        m_isInitPPIMsgSend = false;
        /*
        INT8U buf[MAX_SHORT_BUF_LEN];
        //memset(buf, 0, sizeof(buf));
        L2_MSG_STRU *pL2Msg = ( L2_MSG_STRU * ) & buf;
        pL2Msg->Port = 0;
        pL2Msg->MsgMem = (int*)ShortBufMem;
        pL2Msg->MsgType = 6;
        pL2Msg->unit.L2FmaPara.IfMaster = true;
        pL2Msg->unit.L2FmaPara.RetryCount = 3;
        pL2Msg->unit.L2FmaPara.BaudRate = conn->cfg.baud;
        pL2Msg->unit.L2FmaPara.Addr = conn->cfg.hmiAddr;
        pL2Msg->unit.L2FmaPara.Hsa = PPI_HSA;//32;conn->cfg.hsa;
        pL2Msg->unit.L2FmaPara.GapFactor = 1;//GAP_FACTOR;
        pL2Msg->unit.L2FmaPara.DA = 0xff;//PPI_NULL_ADDR;
        if ( m_isMpi )
            pL2Msg->unit.L2FmaPara.SetProtol = 0;//MPI_PROTO_M;
        else
            pL2Msg->unit.L2FmaPara.SetProtol = 1;//PPI_PROTO_M;
        pL2Msg->Saddr = 0xff;//PPI_NULL_ADDR;
        pL2Msg->Frome = 0xAA;
        m_rwSocket->writeDev((char*)buf,MAX_SHORT_BUF_LEN );
        */
        /*if ( conn->cfg.COM_interface == 0 )
        {
            m_fd = open( "/dev/mpi1", O_RDWR | O_NOCTTY | O_NONBLOCK );
            qDebug() << "open mpi dev1\n";
        }
        else if ( conn->cfg.COM_interface == 1 )
        {
            m_fd = open( "/dev/mpi2", O_RDWR | O_NOCTTY | O_NONBLOCK );
        }
        else
        {
            return false;
        }

        if ( m_fd < 0 )
        {
            qDebug() << "mpi1_dev busy!\n";
            return false;
        }*/
    }
    else
        Q_ASSERT( false );
    return false;
}

bool Mpi300Service::createMpiConnLink( struct Mpi300Connect* pConnect )
{
    INT8U buf[MAX_SHORT_BUF_LEN];
    L2_MSG_STRU *pL2Msg = ( L2_MSG_STRU * ) & buf;
    memset(buf,0,MAX_SHORT_BUF_LEN);
    qDebug("createMpiConnLink %d", pConnect->isLinking);
    if ( pConnect->isLinking == false )
    {
        pConnect->isLinking = true;
        pConnect->beginTime = m_currTime;
        pL2Msg->Port = 0;
        pL2Msg->MsgMem = ShortBufMem;
        pL2Msg->MsgType = L2_MSG_CREAT_LINK;
        pL2Msg->unit.L2FmaPara.DA = pConnect->cfg.cpuAddr;
        pL2Msg->Saddr = pConnect->cfg.cpuAddr;
        pL2Msg->Frome = 0xAA;
        // m_devUseCnt++;
        //int ret = write( m_fd, ( char* )buf, MAX_SHORT_BUF_LEN );
        m_rwSocket->writeDev(( char* )buf, MAX_SHORT_BUF_LEN);
        //Q_ASSERT( ret == MAX_SHORT_BUF_LEN );
    }
    return true;
}
void Mpi300Service::slot_readCOMData()
{
    char buf[300];
    int len;
    L7_MSG_STRU *L7Msg = (L7_MSG_STRU *)buf;
    //uchar pBuf[2] = {0x00,0x00};
    len = m_rwSocket->readDev((char*)L7Msg);
    while (len > 0)
    {
        if ( len > 0 && L7Msg->Err == COM_OPEN_ERR )
        {
            QThread::currentThread()->msleep(1000);
            // qDebug()<<"slot "<<QThread::currentThread();
            qDebug("mpi300 COM_OPEN_ERR");
            for ( int i = 0; i < m_connectList.count(); i++ )
            {
                struct Mpi300Connect* pConnect = m_connectList[i];
                Q_ASSERT( pConnect );

                pConnect->connStatus = CONNECT_NO_LINK;//qDebug()<<"here 4";
                struct ComMsg comMsg;
                comMsg.varId = 65535; //防止
                comMsg.type = 0; //0是read;
                comMsg.err = VAR_DISCONNECT;
                m_mutexGetMsg.lock();
                m_comMsgTmp.insert( comMsg.varId, comMsg );
                foreach( struct HmiVar* tmpHmiVar, pConnect->actVarsList )
                {
                    comMsg.varId = tmpHmiVar->varId; //防止长
                    comMsg.type = 0; //0是read;
                    comMsg.err = VAR_DISCONNECT;
                    tmpHmiVar->err = VAR_COMM_TIMEOUT;
                    tmpHmiVar->lastUpdateTime = -1;
                    tmpHmiVar->overFlow = false;
                    m_comMsgTmp.insert( comMsg.varId, comMsg );
                }
                m_mutexGetMsg.unlock();

                pConnect->isLinking = false;
                pConnect->requestVarsList.clear();
                pConnect->wrtieVarsList.clear();
                pConnect->writingCnt = 0;

            }
            //initPpiMpiDriver();
            iniDriverFlg = false;
            m_isInitPPIMsgSend = false;
        }
        else
        {
            if( len > 0 )
            {
                processRspData((char*)L7Msg, m_comMsgTmp);
            }
        }
        len = m_rwSocket->readDev((char*)L7Msg);
    }

}
bool Mpi300Service::processRspData( char *pCharMsg, QHash<int, struct ComMsg> &comMsgTmp )
{
    L7_MSG_STRU *pMsg =(L7_MSG_STRU*)pCharMsg;
    bool bRet;
    struct Mpi300Connect* pConnect = NULL;
    int i = 0;

    bRet = false;

    //qDebug()<<"Type "<<pMsg->MsgType<<" Err "<<pMsg->Err;
    for ( i = 0; i < m_connectList.count(); i++ )
    {
        pConnect = m_connectList[i];
        Q_ASSERT( pConnect );

        if ( pConnect->cfg.cpuAddr == pMsg->Saddr ) //属于这个连接
        {
            //qDebug()<<"readlist "<<pConnect->requestVarsList.count()<<" writelist "<<pConnect->wrtieVarsList.count();
            if (  pMsg->MsgType == L7_OneLink_IsLinked_Ok )
            {
                if ( pMsg->Err == PPI_ERR_NO_ERR && pMsg->Saddr <= PPI_HSA ) // 连接成功
                {
                    //pConnect->requestVarsList.clear();
                    // pConnect->wrtieVarsList.clear();
                    //pConnect->waitUpdateVarsList.clear();
                    //pConnect->waitWrtieVarsList.clear();
                    //qDebug()<<"do clear all lists";
                    //pConnect->connStatus = CONNECT_LINKED_OK;
                    pConnect->connStatus = CONNECT_LINKING;
                }
                else
                {
                    pConnect->connStatus = CONNECT_NO_LINK;//qDebug()<<"here 4";
                    struct ComMsg comMsg;
                    comMsg.varId = 65535; //防止长
                    comMsg.type = 0; //0是read;
                    comMsg.err = VAR_DISCONNECT;
                    m_mutexGetMsg.lock();
                    m_comMsgTmp.insert( comMsg.varId, comMsg );
                    foreach( struct HmiVar* tmpHmiVar, pConnect->actVarsList )
                    {
                        comMsg.varId = tmpHmiVar->varId; //防止长变量
                        comMsg.type = 0; //0是read;
                        comMsg.err = VAR_DISCONNECT;
                        //tmpHmiVar->err = VAR_COMM_TIMEOUT;
                        //tmpHmiVar->overFlow = false;
                        tmpHmiVar->lastUpdateTime = -1;
                        m_comMsgTmp.insert( comMsg.varId, comMsg );
                    }
                    m_mutexGetMsg.unlock();
                }

                pConnect->isLinking = false;
            }
            else if ( pConnect->requestVarsList.count() > 0 )
            {
                //qDebug()<<"read";
                m_mutexRw.lock();
                processMulReadRsp( (char *)pMsg, pConnect );
                m_mutexRw.unlock();
            }
            else if ( pConnect->writingCnt > 0 )
            {
                //qDebug()<<"write";
                m_mutexRw.lock();
                processMulWriteRsp( (char *)pMsg, pConnect );
                m_mutexRw.unlock();
            }
            else
            {
                /* pConnect->connStatus = CONNECT_NO_LINK;//qDebug()<<"here 4";
                if ( pMsg->Err == PPI_ERR_TIMEROUT )
                {
                    struct ComMsg comMsg;
                    comMsg.varId = 65535; //防止长
                    comMsg.type = 0; //0是read;
                    comMsg.err = VAR_DISCONNECT;
                    comMsgTmp.insert( comMsg.varId, comMsg );
                    foreach( struct HmiVar* tmpHmiVar, pConnect->actVarsList )
                    {
                        comMsg.varId = tmpHmiVar->varId; //防止长
                        comMsg.type = 0; //0是read;
                        comMsg.err = VAR_DISCONNECT;
                        //tmpHmiVar->err = VAR_COMM_TIMEOUT;
                        //tmpHmiVar->overFlow = false;
                        comMsgTmp.insert( comMsg.varId, comMsg );
                    }
                }*/
                qDebug() << "unexpect data packet"<<" MsgType "<<pMsg->MsgType<<" addr "<<pMsg->Saddr<<" err "<<pMsg->Err<<" isLinking "<<pConnect->isLinking;
            }
            break;
        }
        else if (pMsg->Saddr == 0xFF)
        {
            /*HMI ppi device had been init  */
            iniDriverFlg = true;

            //m_runTime->start(10);
            qDebug(">>>>>>>0xff iniDriverFlg = true;");
            break;
        }
        else if(pMsg->Saddr == 0xFE)
        {
            iniDriverFlg = false;
            pConnect->isLinking = false;
            m_isInitPPIMsgSend = false;
            //m_runTime->start(10);
            qDebug(">>>>>>>0xfe iniDriverFlg = false;");
            break;
        }
    }
    if(i == m_connectList.count())
    {
        /*发送初始化L1L2主站 和 发送 CreateMPILink 消息的时候超时进入这里*/
        if(pMsg->Err == VAR_TIMEOUT)
        {
            iniDriverFlg = false;
            m_isInitPPIMsgSend = false;
            for ( i = 0; i < m_connectList.count(); i++ )
            {
                pConnect = m_connectList[i];
                pConnect->isLinking = false;
            }
            qDebug()<<">>>>>>>send initmsg or CreateMPILink msg err iniDriverFlg = false  ";
            qDebug()<<"connect addr = "<<pConnect->cfg.cpuAddr
                   <<" pMsg->Saddr "<<pMsg->Saddr
                  <<" pMsg->Err "<<pMsg->Err;
        }
    }
    bRet = true;
    return 0;
}


void Mpi300Service::slot_run()
{
    initCommDev();
    m_runTime->start(100);
    // qDebug()<<"run "<<QThread::currentThread();
}
void Mpi300Service::slot_runTimeout()
{
    m_runTime->stop();

    int connectCount = m_connectList.count();
    m_currTime = getCurrTime();

    if (!iniDriverFlg)
    {
        if(!m_isInitPPIMsgSend)
        {
            initPpiMpiDriver();
            m_rwSocket->setTimeoutPollCnt(connectCount);
        }

        if ( m_comMsgTmp.count() > 0 )
        {
            m_mutexGetMsg.lock();
            foreach( int key, m_comMsgTmp.keys() )
            {
                m_comMsg.insert( key, m_comMsgTmp.value( key ) );
            }
            m_comMsgTmp.clear();
            m_mutexGetMsg.unlock();
        }
        m_runTime->start(1000);
        return;
    }
    for ( int i = 0; i < connectCount; i++ )
    {                              // 处理写
        struct Mpi300Connect* pConnect = m_connectList[i];
        updateWriteReq( pConnect );
        trySndWriteReq( pConnect, m_comMsgTmp );
    }

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

    bool hasProcessingReqFlag = false;
    for ( int i = 0; i < connectCount; i++ )
    {                              // 处理读请
        struct Mpi300Connect* pConnect = m_connectList[i];

        updateOnceReadReadReq( pConnect );  //先重组一次更新变量
        updateReadReq( pConnect ); //无论如何都要重组请求变量

        if ( !( hasProcessingReq( pConnect ) ) )
            combineReadReq( pConnect );
        trySndReadReq( pConnect , m_comMsgTmp );

        if ( hasProcessingReq( pConnect ) )
            hasProcessingReqFlag = true; //有正在处理的请求
    }
    m_runTime->start( 50 );         //休ms



        //回去继续尝试更新发送，直到1个也发送不下去
}


