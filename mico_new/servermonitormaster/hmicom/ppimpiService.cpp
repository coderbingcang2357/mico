#include "ppimpiService.h"
#include "ppi/ppi_l7wr.h"
#include "ppi/ppi_l7.h"
#include "rwsocket.h"

#define MAX_PPI_FRAME_LEN  188   //最大一包200字节

PpiMpiService::PpiMpiService( struct PpiMpiConnectCfg& connectCfg )
{
    struct PpiMpiConnect* pConnect = new PpiMpiConnect;
    pConnect->cfg = connectCfg;
    pConnect->writingCnt = 0;
    pConnect->isLinking = false;
    pConnect->beginTime = -1;
    pConnect->connStatus = CONNECT_NO_OPERATION;
    m_currTime = getCurrTime();
    pConnect->connectTestVar.lastUpdateTime = -1;
    m_isMpi = connectCfg.isMpi;
	printf("connectcfg.baud:%u",connectCfg.baud);
    //qDebug() << "m_isMpi = " << m_isMpi;
    m_connectList.append( pConnect );
    m_runTime = new QTimer(this);
    connect(m_runTime, SIGNAL(timeout()), this, SLOT(slot_runTimeout()));
    connect( this, SIGNAL(startRuntime()), this, SLOT( slot_runTimeout() ) );

    //initCommDev();
    m_rwSocket = NULL; //lvyong
    m_iniDriverFlg = false;
    m_isInitPPIMsgSend = false;
}

PpiMpiService::~PpiMpiService()
{
    if(m_runTime){
        m_runTime->stop();
        delete m_runTime;
    }
    if(m_rwSocket){
        delete m_rwSocket;
        m_rwSocket = NULL;
    }
	// add by wubc 2021-01-18
	qDebug() << "~PpiMpiService";
	if(!m_connectList.isEmpty()){
		//qDebug() << "del GateWayService len:" << m_connectList.length();
		QList<struct PpiMpiConnect*>::iterator item = m_connectList.begin();
		while(item != m_connectList.end()){
			//DelGateWayConnect(*item);
			delete *item;
			*item = NULL;
			++item;
		}
		m_connectList.clear();
	}
}

bool PpiMpiService::meetServiceCfg( int serviceType, int COM_interface )
{
    if ( serviceType == SERVICE_TYPE_PPIMPI )
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

int PpiMpiService::getConnLinkStatus( int connectId )
{
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct PpiMpiConnect* pConnect = m_connectList[i];
        if ( connectId == pConnect->cfg.id )
            return pConnect->connStatus;
    }
    return -1;
}

int PpiMpiService::getComMsg( QList<struct ComMsg> &msgList )
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
int PpiMpiService::splitLongVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    //m_mutexUpdateVar.lock();
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct PpiMpiConnect* pConnect = m_connectList[i];
        if ( pHmiVar->connectId == pConnect->cfg.id )
        {
            if ( pHmiVar->area == PPI_AP_AREA_T_IEC )
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
                        pSplitHhiVar->subArea = pHmiVar->subArea;
                        pSplitHhiVar->addOffset = pHmiVar->addOffset + splitedLen / 4;
                        pSplitHhiVar->bitOffset = -1;
                        pSplitHhiVar->len = len;
                        pSplitHhiVar->dataType = pHmiVar->dataType;
                        pSplitHhiVar->err = VAR_NO_OPERATION; //没有数据，初始状态为没有初始化
                        pSplitHhiVar->wStatus = VAR_NO_OPERATION;
                        pSplitHhiVar->lastUpdateTime = -1; //立即更新
                        pSplitHhiVar->writeCnt = 0;
                        pSplitHhiVar->writeCnt = 0;

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
                        pSplitHhiVar->subArea = pHmiVar->subArea;
                        pSplitHhiVar->addOffset = pHmiVar->addOffset + splitedLen / 2;
                        pSplitHhiVar->bitOffset = -1;
                        pSplitHhiVar->len = len;
                        pSplitHhiVar->dataType = pHmiVar->dataType;
                        pSplitHhiVar->err = VAR_NO_OPERATION; //没有数据，初始状态为没有初始化
                        pSplitHhiVar->wStatus = VAR_NO_OPERATION;
                        pSplitHhiVar->lastUpdateTime = -1; //立即更新
                        pSplitHhiVar->writeCnt = 0;

                        splitedLen += len;
                        pHmiVar->splitVars.append( pSplitHhiVar );
                    }
                }
            }
            else
            {
                if ( pHmiVar->dataType == Com_Bool )
                    Q_ASSERT( pHmiVar->len == 1 );
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
                            pSplitHhiVar->err = VAR_NO_OPERATION; //没有数据，初始状态为没有初始化
                            pSplitHhiVar->wStatus = VAR_NO_OPERATION;
                            pSplitHhiVar->lastUpdateTime = -1; //立即更新
                            pSplitHhiVar->writeCnt = 0;

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
int PpiMpiService::onceRealUpdateVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    m_mutexOnceReal.lock();
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct PpiMpiConnect* pConnect = m_connectList[i];
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
            //qDebug() << "startUpdateVar：" << pHmiVar->varId;

            m_mutexOnceReal.unlock();
            return 0;
        }
    }
    m_mutexOnceReal.unlock();
    return -1;
}

int PpiMpiService::startUpdateVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    m_mutexUpdateVar.lock();
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct PpiMpiConnect* pConnect = m_connectList[i];
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
            //qDebug() << "startUpdateVar: " << pHmiVar->varId;

            m_mutexUpdateVar.unlock();
            readAllVar( pHmiVar->connectId, pHmiVar );
            return 0;
        }
    }
    m_mutexUpdateVar.unlock();
    return -1;
}
void PpiMpiService::readAllVar( int connectId, struct HmiVar* pHmiVar )
{
    if ( getConnLinkStatus( connectId ) != CONNECT_LINKED_OK )
    {
        struct ComMsg comMsg;
        comMsg.varId = pHmiVar->varId; //防止长变量
        comMsg.type = 0; //0是read;
        comMsg.err = VAR_DISCONNECT;
        m_mutexGetMsg.lock();
        m_comMsg.insert( comMsg.varId, comMsg );
        m_mutexGetMsg.unlock();
    }
}
int PpiMpiService::stopUpdateVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    m_mutexUpdateVar.lock();
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct PpiMpiConnect* pConnect = m_connectList[i];
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

int PpiMpiService::readVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    if ( pHmiVar->err == VAR_NO_ERR )  //无错误才拷贝数据
    {
        if ( pHmiVar->area == PPI_AP_AREA_T_IEC )
        {
            int cnt = ( pHmiVar->len + 3 ) / 4;
            if ( bufLen >= pHmiVar->len && bufLen >= cnt*4 && pHmiVar->rData.size() >= cnt*5 )
            {
                for ( int i = 0; i < cnt; i++ )
                {
                    pBuf[4*i+0] = pHmiVar->rData[5*i+4];
                    pBuf[4*i+1] = pHmiVar->rData[5*i+3];
                    pBuf[4*i+2] = pHmiVar->rData[5*i+2];
                    pBuf[4*i+3] = pHmiVar->rData[5*i+1];
                }
            }
            else
                Q_ASSERT( false );
        }
        else if ( pHmiVar->area == PPI_AP_AREA_C_IEC )
        {
            int cnt = ( pHmiVar->len + 1 ) / 2;
            if ( bufLen >= pHmiVar->len && bufLen >= cnt*2 && pHmiVar->rData.size() >= cnt*3 )
            {
                for ( int i = 0; i < cnt; i++ )
                {
                    pBuf[2*i+0] = pHmiVar->rData[3*i+2];
                    pBuf[2*i+1] = pHmiVar->rData[3*i+1];
                }
            }
            else
                Q_ASSERT( false );
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
            else
            {
                int dataTypeLen = getDataTypeLen( pHmiVar->dataType );
                Q_ASSERT( dataTypeLen > 0 );
                for ( int j = 0; ; j++ )             //大小数转换
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

int PpiMpiService::readDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    Q_ASSERT( pHmiVar && pBuf );
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct PpiMpiConnect* pConnect = m_connectList[i];
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

int PpiMpiService::writeVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    int ret;

    //qDebug() << "writeVar" << pHmiVar->area << pHmiVar->addOffset << pHmiVar->len;

    Q_ASSERT( pHmiVar && pBuf );
    m_mutexRw.lock();
    QByteArray wdata;
    if ( pHmiVar->dataType == Com_Bool )
        Q_ASSERT( pHmiVar->len == 1 );
    int dataBytes = qMin( bufLen, pHmiVar->len );
    if ( wdata.size() < dataBytes )
        wdata.resize( dataBytes );

    memcpy( wdata.data(), pBuf, dataBytes );

    if ( pHmiVar->area == PPI_AP_AREA_T_IEC )
    {
        int count = ( pHmiVar->len + 3 ) / 4;
        if ( pHmiVar->rData.size() < 5*count )
            pHmiVar->rData.resize( 5*count );
        for ( int i = 0; i < count; i++ )
        {
            if ( 4*i + 3 < wdata.size() )
            {
                pHmiVar->rData[5*i+1] = wdata[4*i+3];
                pHmiVar->rData[5*i+2] = wdata[4*i+2];
                pHmiVar->rData[5*i+3] = wdata[4*i+1];
                pHmiVar->rData[5*i+4] = wdata[4*i+0];
            }
            else
                break;
        }
    }
    else if ( pHmiVar->area == PPI_AP_AREA_C_IEC )
    {
        int count = ( pHmiVar->len + 1 ) / 2;
        if ( pHmiVar->rData.size() < 3*count )
            pHmiVar->rData.resize( 3*count );
        for ( int i = 0; i < count; i++ )
        {
            if ( 2*i + 1 < wdata.size() )
            {
                pHmiVar->rData[3*i+1] = wdata[2*i+1];
                pHmiVar->rData[3*i+2] = wdata[2*i+0];
            }
            else
                break;
        }
    }
    else
    {
        if ( pHmiVar->dataType == Com_Bool )
        {
            Q_ASSERT( pHmiVar->len == 1 );
            if ( pHmiVar->rData.size() < 1 )
                pHmiVar->rData.resize( 1 );
            pHmiVar->rData[0] = 0;
            if ( wdata.size() > 0 )
            {
                if ( wdata[0] & 0x1 )
                    pHmiVar->rData[0] = ( 1 << pHmiVar->bitOffset );
            }
        }
        else
        {
            if ( pHmiVar->rData.size() < pHmiVar->len )
                pHmiVar->rData.resize( pHmiVar->len );
            int dataTypeLen = getDataTypeLen( pHmiVar->dataType );
            Q_ASSERT( dataTypeLen > 0 );
            for ( int j = 0; ; j++ )             //大小数转换
            {
                int k;
                for ( k = 0; k < dataTypeLen; k++ )
                {
                    int wOffset = dataTypeLen * j + k;
                    int rOffset = dataTypeLen * j + dataTypeLen - 1 - k;
                    if ( wOffset < pHmiVar->rData.size() && rOffset < wdata.size() )
                        pHmiVar->rData[wOffset] = wdata[rOffset];
                    else
                        break;
                }
                if ( k < dataTypeLen )
                    break;
            }
        }
    }
    if(pHmiVar->writeCnt == 0)
    {
        pHmiVar->wData.resize(0);
    }
    pHmiVar->wData.append(wdata);
    pHmiVar->writeCnt++;
    

    pHmiVar->lastUpdateTime = -1;
    pHmiVar->wStatus = VAR_NO_OPERATION;
    ret = pHmiVar->err;
    m_mutexRw.unlock();

    return ret; //是否要返回成功
}

int PpiMpiService::writeDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    Q_ASSERT( pHmiVar && pBuf );
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct PpiMpiConnect* pConnect = m_connectList[i];
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
            return ret; //是否要返回成功
        }
    }
    return -1;
}

int PpiMpiService::addConnect( struct PpiMpiConnectCfg& connectCfg )
{                                      //要检查是否重复 add
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct PpiMpiConnect* pConnect = m_connectList[i];
        Q_ASSERT( pConnect->cfg.COM_interface == connectCfg.COM_interface );//绝对要是同一个设备接口
        if ( connectCfg.id == pConnect->cfg.id )
        {
            Q_ASSERT( false );
            return 0;
        }
    }
    struct PpiMpiConnect* pConnect = new PpiMpiConnect;
    pConnect->cfg = connectCfg;
    pConnect->writingCnt = 0;
    pConnect->isLinking = false;
    pConnect->connStatus = CONNECT_NO_OPERATION;
    //pConnect->pollCnt = 0;
    m_connectList.append( pConnect );

    return 0;
}

int PpiMpiService::sndWriteVarMsgToDriver( struct PpiMpiConnect* pConnect, struct HmiVar* pHmiVar )
{
    L2_MASTER_MSG_STRU *pL2Msg;
    PPI_PDU_REQ_MULT_READ *pPDU_NETW;   // NETR的PDU结构指针
    PPI_NETR_REQ_VAR_STRU *pVars;
    INT8U buf[MAX_LONG_BUF_LEN];
    PPI_PDU_REQ_HEADER *pPduHeader; //NETW的包头
    PPI_NETW_REQ_DAT_STRU *pDats;

    //qDebug() << m_devUseCnt << "write here1 = ";
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
                        && ( pHmiVar->area != PPI_AP_AREA_M ) && ( pHmiVar->area != PPI_AP_AREA_V )
                        && ( pHmiVar->area != PPI_AP_AREA_SYS ) && ( pHmiVar->area != PPI_AP_AREA_SM )
                        && ( pHmiVar->area != PPI_AP_AREA_T_IEC ) && ( pHmiVar->area != PPI_AP_AREA_C_IEC )
                        && ( pHmiVar->area != PPI_AP_AREA_DB_H ))
        {
            Q_ASSERT( false );
            return -1;
        }
        int dataLen = 12; //
        if ( pHmiVar->area == PPI_AP_AREA_T_IEC )
            dataLen += (( pHmiVar->len + 3 ) / 4 ) * 5;
        else if ( pHmiVar->area == PPI_AP_AREA_C_IEC )
            dataLen += (( pHmiVar->len + 1 ) / 2 ) * 3;
        else
        {
            if ( pHmiVar->dataType == Com_Bool )
                Q_ASSERT( pHmiVar->len == 1 );
            dataLen += pHmiVar->len;
        }
        if ( totallen + dataLen <= MAX_PPI_FRAME_LEN && varCnt < 1 )
        {
            varCnt++;
            totallen += dataLen;
        }
        else
            break;
    }
    //qDebug() << m_devUseCnt << "write here2 = ";
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
    pPduHeader->RedID    = conv_16bit( 0x0000 );
    pPduHeader->PduRef   = conv_16bit( 0 );
    pPduHeader->ParLen   = conv_16bit( sizeof( PPI_NETR_REQ_VAR_STRU ) * varCnt + 2 ); // 2是sevice id 和 No of Variables
    pPduHeader->DatLen   = 0; //需要重新填写

    INT8U* Ptr = pL2Msg->pPdu + sizeof( PPI_PDU_REQ_HEADER ); // 指向Service ID 的位置
    *Ptr = PPI_SID_WRITE;
    Ptr ++;
    *Ptr = varCnt;
    Ptr ++;         //处理完后指向第一个变量的地址。
    pVars = ( PPI_NETR_REQ_VAR_STRU * )Ptr;
    int PduDatLen = 0;
    for ( int i = 0; i < varCnt; i++ )
    {
        int VarDatLen = 0;
        pDats =  ( PPI_NETW_REQ_DAT_STRU * )( pL2Msg->pPdu + sizeof( PPI_PDU_REQ_HEADER  )
                                             + sizeof( PPI_NETR_REQ_VAR_STRU ) * varCnt + 2 + PduDatLen );
        struct HmiVar* pHmiVar = pConnect->waitWrtieVarsList[i];
        QByteArray wdata = pHmiVar->wData.left(pHmiVar->wData.size()/pHmiVar->writeCnt);
        //PDU Par Variables
        pVars->VarSpec = 0x12;  //固定
        pVars->AddrLen = 0x0A;  //any pointer 的长度

        //设置any pointer.
        pVars->VarAddr.Syntax_ID  = 0x10; //固定
        pVars->VarAddr.Area = pHmiVar->area;

        if ( pHmiVar->area == PPI_AP_AREA_V )
            pVars->VarAddr.Subarea  = conv_16bit( 0x0001 );
        else if ( pHmiVar->area == PPI_AP_AREA_DB_H)
            pVars->VarAddr.Subarea  = conv_16bit( pHmiVar->subArea );
        else
            pVars->VarAddr.Subarea  = conv_16bit( 0x0000 );

        if ( pHmiVar->area == PPI_AP_AREA_T_IEC )
        {
            pVars->VarAddr.Type = PPI_AP_TYPE_T_IEC;
            pVars->VarAddr.NumElements = conv_16bit((( pHmiVar->len + 3 ) / 4 ) );
            //pVars->VarAddr.Offset = conv_32bit( pHmiVar->addOffset ) >> 8;
            pVars->VarAddr.Offset[0] = pHmiVar->addOffset >> 16 & 0xFF;
            pVars->VarAddr.Offset[1] = pHmiVar->addOffset >>  8 & 0xFF;
            pVars->VarAddr.Offset[2] = pHmiVar->addOffset       & 0xFF;
            pDats->DataType = D_TYPE_LONG;
            pDats->Length = conv_16bit((( pHmiVar->len + 3 ) / 4 ) * 5 * 8 );
            int count = ( pHmiVar->len + 3 ) / 4;
            for ( int j = 0; j < count; j++ )
            {
                *(( INT8U* )( pDats->Dat ) + 5*j + 0 ) = 0;
                *(( INT8U* )( pDats->Dat ) + 5*j + 1 ) = 0;
                *(( INT8U* )( pDats->Dat ) + 5*j + 2 ) = 0;
                if ( 4*j + 1 < wdata.size() )
                {
                    *(( INT8U* )( pDats->Dat ) + 5*j + 3 ) = wdata[4*j+1];
                    *(( INT8U* )( pDats->Dat ) + 5*j + 4 ) = wdata[4*j];
                }
                else
                {
                    *(( INT8U* )( pDats->Dat ) + 5*j + 3 ) = 0;
                    *(( INT8U* )( pDats->Dat ) + 5*j + 4 ) = 0;
                }
            }
            VarDatLen = count * 5;
        }
        else if ( pHmiVar->area == PPI_AP_AREA_C_IEC )
        {
            pVars->VarAddr.Type = PPI_AP_TYPE_C_IEC;
            pVars->VarAddr.NumElements = conv_16bit((( pHmiVar->len + 1 ) / 2 ) );
            //pVars->VarAddr.Offset = conv_32bit( pHmiVar->addOffset ) >> 8;
            pVars->VarAddr.Offset[0] = pHmiVar->addOffset >> 16 & 0xFF;
            pVars->VarAddr.Offset[1] = pHmiVar->addOffset >>  8 & 0xFF;
            pVars->VarAddr.Offset[2] = pHmiVar->addOffset       & 0xFF;

            pDats->DataType = D_TYPE_BYTES;
            pDats->Length = conv_16bit((( pHmiVar->len + 1 ) / 2 ) * 3 * 8 );
            int count = ( pHmiVar->len + 1 ) / 2;
            for ( int j = 0; j < count; j++ )
            {
                *(( INT8U* )( pDats->Dat ) + 3*j + 0 ) = 0;
                if ( 2*j + 1 < wdata.size() )
                {
                    *(( INT8U* )( pDats->Dat ) + 3*j + 1 ) = wdata[2*j+1];
                    *(( INT8U* )( pDats->Dat ) + 3*j + 2 ) = wdata[2*j];
                }
                else
                {
                    *(( INT8U* )( pDats->Dat ) + 3*j + 1 ) = 0;
                    *(( INT8U* )( pDats->Dat ) + 3*j + 2 ) = 0;
                }
            }
            VarDatLen = count * 3;
        }
        else
        {
            if ( pHmiVar->dataType == Com_Bool )
            {
                pVars->VarAddr.Type = PPI_AP_TYPE_BOOL;
                pVars->VarAddr.NumElements = conv_16bit( 1 );
                //pVars->VarAddr.Offset = conv_32bit( pHmiVar->addOffset * 8 + pHmiVar->bitOffset ) >> 8;
                pVars->VarAddr.Offset[0] = ( pHmiVar->addOffset * 8 + pHmiVar->bitOffset ) >> 16 & 0xFF;
                pVars->VarAddr.Offset[1] = ( pHmiVar->addOffset * 8 + pHmiVar->bitOffset ) >>  8 & 0xFF;
                pVars->VarAddr.Offset[2] = ( pHmiVar->addOffset * 8 + pHmiVar->bitOffset )       & 0xFF;

                pDats->Length = conv_16bit( 1 );
                pDats->DataType = D_TYPE_BIT;
                if ( wdata.size() > 0 )
                    pDats->Dat[0] = wdata[0] & 0x1;
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
                pVars->VarAddr.NumElements = conv_16bit( pHmiVar->len );
                //pVars->VarAddr.Offset = conv_32bit( pHmiVar->addOffset * 8 ) >> 8;
                pVars->VarAddr.Offset[0] =( pHmiVar->addOffset * 8 ) >> 16 & 0xFF;
                pVars->VarAddr.Offset[1] =( pHmiVar->addOffset * 8 ) >>  8 & 0xFF;
                pVars->VarAddr.Offset[2] =( pHmiVar->addOffset * 8 ) & 0xFF;

                pDats->Length = conv_16bit( pHmiVar->len * 8 );
                pDats->DataType = D_TYPE_BYTES;
                int dataTypeLen = getDataTypeLen( pHmiVar->dataType );
                Q_ASSERT( dataTypeLen > 0 );
                for ( int j = 0; ; j++ )             //大小数转换
                {
                    int k;
                    for ( k = 0; k < dataTypeLen; k++ )
                    {
                        int wOffset = dataTypeLen * j + k;
                        int rOffset = dataTypeLen * j + dataTypeLen - 1 - k;
                        if ( wOffset < pHmiVar->len && rOffset < wdata.size() )
                            *(( INT8U* )( pDats->Dat ) + wOffset ) = wdata[rOffset];
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
        {                           //如果长度是奇数,而且不是最后一个变量
            pDats->Dat[VarDatLen] = 0x00;  //补一个0在数据的最后对齐
            VarDatLen += 1;
        }

        PduDatLen += ( 4 + VarDatLen );  // Reserveed + Data type + Length 刚好4字节
        pDats = ( PPI_NETW_REQ_DAT_STRU * )( pL2Msg->pPdu + sizeof( PPI_PDU_REQ_HEADER ) + pPduHeader->ParLen + PduDatLen );
        pVars++; //指向下一个变量
    }
    pPduHeader->DatLen = conv_16bit( PduDatLen ); //把实际的数据block的长度填回去
    pL2Msg->PduLen = sizeof( PPI_PDU_REQ_HEADER ) + ( 2 + sizeof( PPI_NETR_REQ_VAR_STRU ) * varCnt ) + PduDatLen;
    //m_devUseCnt++;
    m_mutexRw.unlock();

    m_rwSocket->writeDev((char *)buf, MAX_SHORT_BUF_LEN+1024/*pL2Msg->PduLen*/);
    //pConnect->pollCnt = 30;

    return varCnt;
}

int PpiMpiService::sndReadVarMsgToDriver( struct PpiMpiConnect* pConnect, struct HmiVar* pHmiVar )
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

    //填写消息包
    //PDU Header Block
    pPDU_NETR->Header.ProtoID  = PROT_ID_S7_200;
    pPDU_NETR->Header.Rosctr   = ROSCTR_REQUEST;
    pPDU_NETR->Header.RedID    = conv_16bit( 0x0000 );
    pPDU_NETR->Header.PduRef   = conv_16bit( 0 );
    pPDU_NETR->Header.ParLen   = conv_16bit( 0 ); //fill later  sizeof(Heard) + 2 + sizeof() * blkNum;
    pPDU_NETR->Header.DatLen   = conv_16bit( 0 );

    //Parameter Block
    pPDU_NETR->Para.ServiceID  = PPI_SID_READ;
    pPDU_NETR->Para.VarNum     = 0;  //fill later.

    pVars = pPDU_NETR->Para.Vars;

    if ( pHmiVar && pHmiVar->addOffset >= 0 && pHmiVar->len > 0 )  //是合并
    {
        //设置any pointer.
        pPDU_NETR->Para.Vars[0].VarSpec = 0x12; //固定
        pPDU_NETR->Para.Vars[0].AddrLen = 0x0A; //固定 Any Pointer的长度
        pPDU_NETR->Para.Vars[0].VarAddr.Syntax_ID = 0x10;
        int dataLen = 12;// + 4; //?
        switch ( pHmiVar->area )
        {
        case PPI_AP_AREA_I:
        case PPI_AP_AREA_Q:
        case PPI_AP_AREA_M:
        case PPI_AP_AREA_V:
        case PPI_AP_AREA_SM:
        case PPI_AP_AREA_SYS:
        case PPI_AP_AREA_DB_H:
            pPDU_NETR->Para.Vars[0].VarAddr.Type = PPI_AP_TYPE_BYTE;
            //pPDU_NETR->Para.Vars[0].VarAddr.Offset = conv_32bit( pHmiVar->addOffset * 8 ) >> 8;
            pPDU_NETR->Para.Vars[0].VarAddr.Offset[0] = ( pHmiVar->addOffset * 8 )>> 16 & 0xFF;
            pPDU_NETR->Para.Vars[0].VarAddr.Offset[1] = ( pHmiVar->addOffset * 8 )>>  8 & 0xFF;
            pPDU_NETR->Para.Vars[0].VarAddr.Offset[2] = ( pHmiVar->addOffset * 8 )     & 0xFF;
            if ( pHmiVar->dataType == Com_Bool )
            {
                Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 7 );
                pPDU_NETR->Para.Vars[0].VarAddr.NumElements = conv_16bit(( pHmiVar->bitOffset + pHmiVar->len + 7 ) / 8 );
                dataLen += ( pHmiVar->bitOffset + pHmiVar->len + 7 ) / 8;
            }
            else
            {
                pPDU_NETR->Para.Vars[0].VarAddr.NumElements = conv_16bit( pHmiVar->len );
                dataLen += pHmiVar->len;
            }
            break;
        case PPI_AP_AREA_T_IEC:
            pPDU_NETR->Para.Vars[0].VarAddr.Type = PPI_AP_TYPE_T_IEC;
            Q_ASSERT(( pHmiVar->len % 4 ) == 0 );
            pPDU_NETR->Para.Vars[0].VarAddr.NumElements = conv_16bit(( pHmiVar->len + 3 ) / 4 );
            //pPDU_NETR->Para.Vars[0].VarAddr.Offset = conv_32bit( pHmiVar->addOffset ) >> 8;
            pPDU_NETR->Para.Vars[0].VarAddr.Offset[0] = ( pHmiVar->addOffset )>> 16 & 0xFF;
            pPDU_NETR->Para.Vars[0].VarAddr.Offset[1] = ( pHmiVar->addOffset )>>  8 & 0xFF;
            pPDU_NETR->Para.Vars[0].VarAddr.Offset[2] = ( pHmiVar->addOffset )      & 0xFF;
            dataLen += 5 * ( pHmiVar->len + 3 ) / 4;
            break;
        case PPI_AP_AREA_C_IEC:
            pPDU_NETR->Para.Vars[0].VarAddr.Type = PPI_AP_TYPE_C_IEC;
            Q_ASSERT(( pHmiVar->len % 2 ) == 0 );
            pPDU_NETR->Para.Vars[0].VarAddr.NumElements = conv_16bit(( pHmiVar->len + 1 ) / 2 );
            //pPDU_NETR->Para.Vars[0].VarAddr.Offset = conv_32bit( pHmiVar->addOffset ) >> 8;
            pPDU_NETR->Para.Vars[0].VarAddr.Offset[0] = ( pHmiVar->addOffset )>> 16 & 0xFF;
            pPDU_NETR->Para.Vars[0].VarAddr.Offset[1] = ( pHmiVar->addOffset )>>  8 & 0xFF;
            pPDU_NETR->Para.Vars[0].VarAddr.Offset[2] = ( pHmiVar->addOffset )      & 0xFF;
            dataLen += 3 * ( pHmiVar->len + 1 ) / 2;
            break;
        case PPI_AP_AREA_HC_IEC:
        default:
            Q_ASSERT( false );
            return -1;
        }
        if ( pHmiVar->area == PPI_AP_AREA_V )
            pPDU_NETR->Para.Vars[0].VarAddr.Subarea  = conv_16bit( 0x0001 );
        else if( pHmiVar->area == PPI_AP_AREA_DB_H )
            pPDU_NETR->Para.Vars[0].VarAddr.Subarea  = conv_16bit( pHmiVar->subArea );
        else
            pPDU_NETR->Para.Vars[0].VarAddr.Subarea  = conv_16bit( 0x0000 );

        pPDU_NETR->Para.Vars[0].VarAddr.Area = pHmiVar->area;

        pPDU_NETR->Para.VarNum = 1;
        pPDU_NETR->Header.ParLen = conv_16bit( 2 + sizeof( PPI_NETR_REQ_VAR_STRU ) * 1 );
        pL2Msg->PduLen =sizeof( PPI_PDU_REQ_HEADER ) + ( INT8U )( 2 + sizeof( PPI_NETR_REQ_VAR_STRU ) * 1 );
    }
    else                                  //是组合
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
            pPDU_NETR->Para.Vars[i].AddrLen = 0x0A; //固定 Any Pointer的长度
            pPDU_NETR->Para.Vars[i].VarAddr.Syntax_ID = 0x10;
            int dataLen = 12;// + 4;//?
            switch ( pHmiVar->area )
            {
            case PPI_AP_AREA_I:
            case PPI_AP_AREA_Q:
            case PPI_AP_AREA_M:
            case PPI_AP_AREA_V:
            case PPI_AP_AREA_SM:
            case PPI_AP_AREA_SYS:
            case PPI_AP_AREA_DB_H:
                pPDU_NETR->Para.Vars[i].VarAddr.Type = PPI_AP_TYPE_BYTE;
                //pPDU_NETR->Para.Vars[i].VarAddr.Offset = conv_32bit( pHmiVar->addOffset * 8 ) >> 8;
                //qDebug()<<"offset "<<QString::number(pHmiVar->addOffset * 8, 16);
                pPDU_NETR->Para.Vars[i].VarAddr.Offset[0] = ( pHmiVar->addOffset * 8 )>> 16 & 0xFF;
                pPDU_NETR->Para.Vars[i].VarAddr.Offset[1] = ( pHmiVar->addOffset * 8 )>>  8 & 0xFF;
                pPDU_NETR->Para.Vars[i].VarAddr.Offset[2] = ( pHmiVar->addOffset * 8 )      & 0xFF;
                if ( pHmiVar->dataType == Com_Bool )
                {
                    Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 7 );
                    pPDU_NETR->Para.Vars[i].VarAddr.NumElements = conv_16bit(( pHmiVar->bitOffset + pHmiVar->len + 7 ) / 8 );
                    dataLen += ( pHmiVar->bitOffset + pHmiVar->len + 7 ) / 8;
                }
                else
                {
                    pPDU_NETR->Para.Vars[i].VarAddr.NumElements = conv_16bit( pHmiVar->len );
                    dataLen += pHmiVar->len;
                }
                break;
            case PPI_AP_AREA_T_IEC:
                pPDU_NETR->Para.Vars[i].VarAddr.Type = PPI_AP_TYPE_T_IEC;
                Q_ASSERT(( pHmiVar->len % 4 ) == 0 );
                pPDU_NETR->Para.Vars[i].VarAddr.NumElements = conv_16bit(( pHmiVar->len + 3 ) / 4 );
                //pPDU_NETR->Para.Vars[i].VarAddr.Offset = conv_32bit( pHmiVar->addOffset ) >> 8;
                pPDU_NETR->Para.Vars[i].VarAddr.Offset[0] = ( pHmiVar->addOffset )>> 16 & 0xFF;
                pPDU_NETR->Para.Vars[i].VarAddr.Offset[1] = ( pHmiVar->addOffset )>>  8 & 0xFF;
                pPDU_NETR->Para.Vars[i].VarAddr.Offset[2] = ( pHmiVar->addOffset )      & 0xFF;
                dataLen += 5 * (( pHmiVar->len + 3 ) / 4 );
                break;
            case PPI_AP_AREA_C_IEC:
                pPDU_NETR->Para.Vars[i].VarAddr.Type = PPI_AP_TYPE_C_IEC;
                Q_ASSERT(( pHmiVar->len % 2 ) == 0 );
                pPDU_NETR->Para.Vars[i].VarAddr.NumElements = conv_16bit(( pHmiVar->len + 1 ) / 2 );
                //pPDU_NETR->Para.Vars[i].VarAddr.Offset = conv_32bit( pHmiVar->addOffset ) >> 8;
                pPDU_NETR->Para.Vars[i].VarAddr.Offset[0] = ( pHmiVar->addOffset )>> 16 & 0xFF;
                pPDU_NETR->Para.Vars[i].VarAddr.Offset[1] = ( pHmiVar->addOffset )>>  8 & 0xFF;
                pPDU_NETR->Para.Vars[i].VarAddr.Offset[2] = ( pHmiVar->addOffset )      & 0xFF;
                dataLen += 3 * (( pHmiVar->len + 1 ) / 2 );
                break;
            case PPI_AP_AREA_HC_IEC:
            default:
                Q_ASSERT( false );
                return -1;
            }
            if ( pHmiVar->area == PPI_AP_AREA_V )
                pPDU_NETR->Para.Vars[i].VarAddr.Subarea  = conv_16bit( 0x0001 );
            else if ( pHmiVar->area == PPI_AP_AREA_DB_H)
                pPDU_NETR->Para.Vars[i].VarAddr.Subarea  = conv_16bit( pHmiVar->subArea );
            else
                pPDU_NETR->Para.Vars[i].VarAddr.Subarea  = conv_16bit( 0x0000 );

            pPDU_NETR->Para.Vars[i].VarAddr.Area = pHmiVar->area;
            totallen += dataLen;
        }
        Q_ASSERT( totallen <= MAX_PPI_FRAME_LEN );
        pPDU_NETR->Para.VarNum = pConnect->requestVarsList.count();
        //qDebug() << "pPDU_NETR->Para.VarNum" << pPDU_NETR->Para.VarNum;
        pPDU_NETR->Header.ParLen = conv_16bit( 2 + sizeof( PPI_NETR_REQ_VAR_STRU ) * pConnect->requestVarsList.count() );
        pL2Msg->PduLen = sizeof( PPI_PDU_REQ_HEADER ) + ( INT8U )( 2 + sizeof( PPI_NETR_REQ_VAR_STRU ) * pConnect->requestVarsList.count() );
    }
    QByteArray tmpBuf;
    for (int i = 0; i < pL2Msg->PduLen; i++)
    {
        tmpBuf[i] = 0xff & (pL2Msg->pPdu[i]);
    }
    tmpBuf.append(pConnect->cfg.cpuAddr);

#if 0
	printf("+++++ppimpi++++++++++\n");
		    for (int j = 0; j < tmpBuf.size(); j++)
				        printf("%02X ", (uchar)tmpBuf[j]); 
	printf("\n+++++++++++++++\n");
    qDebug() << "send:" << tmpBuf.toHex();
#endif
   //m_devUseCnt++;
    /*
    printf("SND READ: ");
    for (int k=0; k<pL2Msg->PduLen; k++)
    {
        printf("%02X ",((char*)pL2Msg->pPdu)[k]);
    }
    printf("\n");
    */
    m_rwSocket->writeDev((char *)buf, MAX_SHORT_BUF_LEN+1024/*pL2Msg->PduLen*/);
    //pConnect->pollCnt = 30;
    return pConnect->requestVarsList.count();
}

/******************************************************************************
** 函数名: GetMpiRspParaPtr
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
** 函数名: GetMpiRspDataPtr
** 说明  : 得到SD2应答消息的data开始地址
** 输入  : pMsg : 消息的开始地址
** 输出  : NA
** 返回  : data的开始地址
******************************************************************************/
static INT8U *GetMpiRspDataPtr( INT8U *pMsg )
{
    PPI_PDU_RSP_HEADER *pHeaderAck = ( PPI_PDU_RSP_HEADER * )pMsg;
    return ( pMsg + sizeof( PPI_PDU_RSP_HEADER ) + conv_16bit( pHeaderAck->ParLen ) );
}

/******************************************************************************
** 函数名: GetRaundSyc
** 说明  : 获取返回值的类型
** 输入  :
** 输出  :
** 返回  : 失败 －1
******************************************************************************/
static int GetRaundSyc( INT8U datatype )
{
    switch ( datatype )
    {
    case D_TYPE_ERROR:  //有错误
        return -1;
    case D_TYPE_BIT:    //是位访问//位变量访问已经整合到字节变量中
        return -1;
    case D_TYPE_BYTES:  //字节变量
        return 3;
    case D_TYPE_LONG:   //针对西门子的T变量
        return 0;
    default:
        return -1;
    }
}

int PpiMpiService::processMulReadRsp( char *pCharMsg, struct PpiMpiConnect* pConnect)
{
    L7_MSG_STRU *pMsg = (L7_MSG_STRU*)pCharMsg;
    PPI_PDU_RSP_HEADER *pHeaderAck = NULL;
    PPI_PDU_RSP_PARA_READ * pParaAck = NULL;
    INT8U *pDataAck =  NULL;
    VAR_VALUE_STRU *pValue;
    INT8U *pPdu = pMsg->pPdu;
    struct ComMsg comMsg;
    int index = 0;
    //Q_ASSERT( pMsg->Saddr == pConnect->cfg.cpuAddr );
    //Q_ASSERT( pMsg && pConnect );
    //qDebug() << "processMulReadRsp1";
    if ( pMsg->Err == NETWR_NO_ERR )
    {
        //qDebug() << "processMulReadRsp2";
        pHeaderAck = ( PPI_PDU_RSP_HEADER * )( pPdu );
        pParaAck = ( PPI_PDU_RSP_PARA_READ * )GetMpiRspParaPtr( pPdu );
        pDataAck = GetMpiRspDataPtr( pPdu );
        if (( pHeaderAck->ErrCls == 0 ) && ( pHeaderAck->ErrCod == 0 ) )
        {
            //qDebug() << "processMulReadRsp3";
            if ( pConnect->requestVar.addOffset >= 0 && pConnect->requestVar.len > 0 ) //合并
            {
                if ( pParaAck->VarNum == 1 )
                {
                    pValue = ( VAR_VALUE_STRU * )( pDataAck + 0 );
                    int RaundSyc = GetRaundSyc( pValue->DataType );
                    if ( pValue->AccRslt == ACC_RSLT_NO_ERR && RaundSyc >= 0 )
                    {
                        INT8U len = ( INT8U )( conv_16bit( pValue->Length ) >> RaundSyc );
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
                            if ( pHmiVar->area == PPI_AP_AREA_T_IEC )
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
                            {
                                if ( pHmiVar->dataType == Com_Bool )
                                    dataBytes = ( pHmiVar->bitOffset + pHmiVar->len + 7 ) / 8;
                                else
                                    dataBytes = pHmiVar->len;
                            }
                            if ( offset + dataBytes <= len )
                            {
                                if ( !pConnect->waitWrtieVarsList.contains( pHmiVar ) )
                                {
                                    if ( pHmiVar->rData.size() < dataBytes )
                                        pHmiVar->rData = QByteArray( dataBytes, 0 );
                                    memcpy( pHmiVar->rData.data(), ( INT8U* )pValue->Var + offset, dataBytes );
                                    //qDebug() << "read ok1" << (uchar)pHmiVar->rData[0] << (uchar)pHmiVar->rData[1];
                                    comMsg.varId = ( pHmiVar->varId & 0x0000ffff ); //防止长变量
                                    comMsg.type = 0; //0是read;
                                    comMsg.err = pHmiVar->err;
                                    m_mutexGetMsg.lock();
                                    m_comMsgTmp.insert( comMsg.varId, comMsg );
                                    m_mutexGetMsg.unlock();
                                    //qDebug()<<"read ok1   id = "<<comMsg.varId;
                                }
                            }
                            else
                            {
                                qDebug() << "data length did not match";
                                qDebug() << "len" << len;
                                qDebug() << "pConnect->requestVar.len" << pConnect->requestVar.len;

                                //for ( int loop = 0; loop < pMsg->data.size(); loop ++ )
                                //{
                                //qDebug() << QString::number( *pPdu[loop], 16 );
                                //}

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
                                pHmiVar->lastUpdateTime = -1; //合并后越界的不算越界，立即再次更新（更新时不合并）
                            else
                            {
                                pHmiVar->err = VAR_PARAM_ERROR;
                                comMsg.varId = ( pHmiVar->varId & 0x0000ffff ); //防止长变量
                                comMsg.type = 0; //0是read;
                                comMsg.err = VAR_PARAM_ERROR;
                                m_mutexGetMsg.lock();
                                m_comMsgTmp.insert( comMsg.varId, comMsg );
                                m_mutexGetMsg.unlock();
                                //qDebug()<<"read ok2   id = "<<comMsg.varId;
                            }
                        }
                    }
                }
            }
            else
            {
                if ( pParaAck->VarNum == pConnect->requestVarsList.count() )
                {
                    //qDebug() << "processMulReadRsp4" << pParaAck->VarNum;
                    for ( INT8U offset = 0; index < pParaAck->VarNum; index++ )
                    {
                        struct HmiVar* pHmiVar = pConnect->requestVarsList[index];
                        pValue = ( VAR_VALUE_STRU * )( pDataAck + offset );
                        if ( pValue->AccRslt == ACC_RSLT_NO_ERR )
                        {
                            //qDebug() << "processMulReadRsp5";
                            int RaundSyc = GetRaundSyc( pValue->DataType );
                            if ( RaundSyc >= 0 )
                            {
                                INT8U len = ( INT8U )( conv_16bit( pValue->Length ) >> RaundSyc );
                                if ( len & 0x01 ) // 如果长度是奇数,需要跳过一个字节
                                    offset += len + 4 + 1;  // 4 is header
                                else
                                    offset += len + 4;
                                if ( !pConnect->waitWrtieVarsList.contains( pHmiVar ) )
                                {
                                    if ( pHmiVar->rData.size() < len )
                                        pHmiVar->rData = QByteArray( len, 0 );
                                    memcpy( pHmiVar->rData.data(), ( INT8U * )pValue->Var, len );
                                    comMsg.varId = ( pHmiVar->varId & 0x0000ffff ); //防止长变量
                                    comMsg.type = 0; //0是read;
                                    comMsg.err = pHmiVar->err;
                                    m_mutexGetMsg.lock();
                                    m_comMsgTmp.insert( comMsg.varId, comMsg );
                                    m_mutexGetMsg.unlock();
                                    //qDebug()<<"read ok3   id = "<<comMsg.varId<<"comMsg.err = "<<comMsg.err;
                                }
                                pHmiVar->err = VAR_NO_ERR;
                                pHmiVar->overFlow = false;
                                pConnect->connStatus = CONNECT_LINKED_OK;
                                //qDebug() << "read ok2";
                                continue;
                            }
                        }
                        offset += 4;  // 4 is header,单个变量的错误不能影响后面变量
                        pHmiVar->err = VAR_PARAM_ERROR;
                        pHmiVar->overFlow = true;
                    }
                }
                else
                {
                    qDebug() << "varible Num did not match";
                    qDebug() << "pParaAck->VarNum" << pParaAck->VarNum;
                    qDebug() << "pConnect->requestVarsList.count()" << pConnect->requestVarsList.count();

                    //for ( int loop = 0; loop < pMsg->data.size(); loop ++ )
                    //{
                    //qDebug() << QString::number( *pPdu[loop], 16 );
                    //}
                    //Q_ASSERT( false );
                }
            }
        }
    }
    else if ( pMsg->Err == PPI_ERR_TIMEROUT )
    {
        pConnect->connStatus = CONNECT_NO_LINK;
        //qDebug()<<"here 1";
        comMsg.varId = 65535; //防止长变量
        comMsg.type = 0; //0是read;
        comMsg.err = VAR_DISCONNECT;
        m_mutexGetMsg.lock();
        m_comMsgTmp.insert( comMsg.varId, comMsg );
        foreach( struct HmiVar* tmpHmiVar, pConnect->actVarsList )
        {
            comMsg.varId = tmpHmiVar->varId; //防止长变量
            comMsg.type = 0; //0是read;
            comMsg.err = VAR_DISCONNECT;
            tmpHmiVar->err = VAR_COMM_TIMEOUT;
            tmpHmiVar->overFlow = false;
            tmpHmiVar->lastUpdateTime = -1;
            m_comMsgTmp.insert( comMsg.varId, comMsg );
        }
        m_mutexGetMsg.unlock();
    }

    for ( ; index < pConnect->requestVarsList.count(); index++ )
    {
        struct HmiVar* pHmiVar = pConnect->requestVarsList[index];
        pHmiVar->err = VAR_COMM_TIMEOUT;
        //pConnect->connStatus = CONNECT_NO_LINK;
        /*if(  pHmiVar->varId == 65535 )
        {
            qDebug()<<"time out!";
            for ( ; index < pConnect->requestVarsList.count(); index++ )
            {
                comMsg.varId = ( pHmiVar->varId & 0x0000ffff ); //防止长变量
                comMsg.type = 0; //0是read;
                comMsg.err = VAR_DISCONNECT;
                m_comMsgTmp->insert( comMsg.varId, comMsg );
            }
            break;
        }
        else
        {*/
        comMsg.varId = ( pHmiVar->varId & 0x0000ffff ); //防止长变量
        comMsg.type = 0; //0是read;
        comMsg.err = VAR_DISCONNECT;
        m_mutexGetMsg.lock();
        m_comMsgTmp.insert( comMsg.varId, comMsg );
        m_mutexGetMsg.unlock();
        //}
    }
    m_mutexUpdateVar.lock();
    pConnect->requestVarsList.clear();
    m_mutexUpdateVar.unlock();
    return 0;
}

int PpiMpiService::processMulWriteRsp( char *pCharMsg, struct PpiMpiConnect* pConnect )
{
    L7_MSG_STRU *pMsg =(L7_MSG_STRU*)pCharMsg;
    PPI_PDU_RSP_HEADER *pHeaderAck = NULL;
    PPI_PDU_RSP_PARA_READ * pParaAck = NULL;
    INT8U *pDataAck =  NULL;
    INT8U *pPdu = pMsg->pPdu;
    struct ComMsg comMsg;
    //Q_ASSERT( pMsg->Saddr == pConnect->cfg.cpuAddr );
    //Q_ASSERT( pMsg && pConnect );
    //qDebug() << "processMulWriteRsp1";
    int index = 0;

    if ( pMsg->Err == NETWR_NO_ERR )
    {
        //qDebug() << "processMulWriteRsp2";
        pHeaderAck = ( PPI_PDU_RSP_HEADER * )( pPdu );
        pParaAck = ( PPI_PDU_RSP_PARA_READ * )GetMpiRspParaPtr( pPdu );
        pDataAck = GetMpiRspDataPtr( pPdu );
        if (( pHeaderAck->ErrCls == 0 ) && ( pHeaderAck->ErrCod == 0 ) )
        {
            //qDebug()<<"pParaAck->VarNum = "<<pParaAck->VarNum<<"  pConnect->writingCnt = "<<pConnect->writingCnt;
            if ( pParaAck->VarNum == pConnect->writingCnt )
            {
                for ( ; index < pParaAck->VarNum; index++ )
                {
                    struct HmiVar* pHmiVar = pConnect->waitWrtieVarsList[index];
                    //qDebug() << "processMulWriteRsp22222";
                    if ( pDataAck[index] == ACC_RSLT_NO_ERR )
                    {
                        pHmiVar->wStatus = VAR_NO_ERR;
                        //pConnect->connStatus = CONNECT_LINKED_OK;
                    }
                    else
                    {
                        pHmiVar->wStatus = VAR_PARAM_ERROR;
                    }
                    comMsg.varId = ( pHmiVar->varId & 0x0000ffff ); //防止长变量
                    comMsg.type = 0; //0是read;
                    comMsg.err = pHmiVar->err;
                    m_mutexGetMsg.lock();
                    m_comMsgTmp.insert( comMsg.varId, comMsg );
                    m_mutexGetMsg.unlock();
                }
            }
            else
                Q_ASSERT( false );
        }
    }
    else
    {
        /*修改了文件 ppimpiService.cpp，解决写变量特别多的时候断开485总线，画面不报警的问题*/
        if( pMsg->Err == PPI_ERR_TIMEROUT )
        {
            pConnect->connStatus = CONNECT_NO_LINK;
            comMsg.varId = 65535; //防止长变量
            comMsg.type = 0; //0是read;
            comMsg.err = VAR_DISCONNECT;
            m_mutexGetMsg.lock();
            m_comMsgTmp.insert( comMsg.varId, comMsg );
            m_mutexGetMsg.unlock();
        }

        m_mutexGetMsg.lock();
        for ( ; index < pConnect->writingCnt; index++ )
        {
            struct HmiVar* pHmiVar = pConnect->waitWrtieVarsList[index];
            //pHmiVar->err = VAR_COMM_TIMEOUT;
            pHmiVar->wStatus = VAR_COMM_TIMEOUT;

            if(pHmiVar->writeCnt > 0)
            {
                pHmiVar->wData.remove(1, (pHmiVar->wData.size()/pHmiVar->writeCnt));
                pHmiVar->writeCnt--;
                //qDebug("%d", __LINE__);
            }
            //pConnect->connStatus = CONNECT_NO_LINK;
            comMsg.varId = ( pHmiVar->varId & 0x0000ffff ); //防止长变量
            comMsg.type = 0; //0是read;
            comMsg.err = pHmiVar->err;
            m_comMsgTmp.insert( comMsg.varId, comMsg );
            /*foreach( struct HmiVar* tmpHmiVar, pConnect->actVarsList )
            {
                comMsg.varId = tmpHmiVar->varId; //防止长变量
                comMsg.type = 0; //0是read;
                comMsg.err = VAR_DISCONNECT;
                m_comMsgTmp.insert( comMsg.varId, comMsg );
            }*/
        }
        m_mutexGetMsg.unlock();
    }

    for ( int i = 0; i < pConnect->writingCnt; i++ )
    {
        struct HmiVar* pHmiVar = pConnect->waitWrtieVarsList.takeAt( 0 );

        if(pHmiVar->writeCnt > 0)
        {
            //qDebug()<<"b "<<pHmiVar->wData.toHex();
            pHmiVar->wData.remove(0, (pHmiVar->wData.size()/pHmiVar->writeCnt));
            //qDebug("%d pHmiVar->wData.size() %d", __LINE__, pHmiVar->wData.size());
            //qDebug()<<"a "<<pHmiVar->wData.toHex();

            pHmiVar->writeCnt--;
        }
    }
    pConnect->writingCnt = 0;

    return 0;
}

bool PpiMpiService::trySndWriteReq( struct PpiMpiConnect* pConnect, QHash<int , struct ComMsg> &m_comMsgTmp ) //不处理超长变量
{
    if ( pConnect->writingCnt == 0 && pConnect->requestVarsList.count() == 0 ) //这个连接没有读写
    {
        if ( m_isMpi )
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
                    int currTime = m_currTime; //2.5秒钟没有建立连接成功就认为连接失败
                    if ( calIntervalTime( pConnect->beginTime, currTime ) >= 2500 )
                    {
                        struct ComMsg comMsg;
                        pConnect->connStatus = CONNECT_NO_LINK;
                        comMsg.varId = 65535; //防止长变量
                        comMsg.type = 0; //0是read;
                        comMsg.err = VAR_DISCONNECT;
                        m_mutexGetMsg.lock();
                        m_comMsgTmp.insert( comMsg.varId, comMsg );
                        foreach( struct HmiVar* tmpHmiVar, pConnect->actVarsList )
                        {
                            comMsg.varId = tmpHmiVar->varId; //防止长变量
                            comMsg.type = 0; //0是read;
                            comMsg.err = VAR_DISCONNECT;
                            tmpHmiVar->err = VAR_COMM_TIMEOUT;
                            tmpHmiVar->overFlow = false;
                            tmpHmiVar->lastUpdateTime = -1;
                            m_comMsgTmp.insert( comMsg.varId, comMsg );
                        }
                        m_mutexGetMsg.unlock();
                        //qDebug()<<"here 2";
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
                        pConnect->writingCnt = writeCnt;    //进入写状态
                        m_mutexWReq.unlock();
                        return true;
                    }
                    else
                    {
                        pConnect->waitWrtieVarsList.takeAt( 0 );//出列,丢掉写请求
                        if(pHmiVar->writeCnt > 0)
                        {
                            pHmiVar->wData.remove(1, (pHmiVar->wData.size()/pHmiVar->writeCnt));
                            pHmiVar->writeCnt--;
                        }
                        //pHmiVar->err = VAR_PARAM_ERROR;
                        pHmiVar->wStatus = VAR_PARAM_ERROR;
                    }
                }
            }
            else if ( pConnect->connStatus == CONNECT_NO_LINK )
            {
                if ( pConnect->waitWrtieVarsList.count() > 0 )
                {
                    struct HmiVar* pHmiVar = pConnect->waitWrtieVarsList.takeAt( 0 );//出列,丢掉写请求
                    if(pHmiVar->writeCnt > 0)
                    {
                        pHmiVar->wData.remove(1, (pHmiVar->wData.size()/pHmiVar->writeCnt));
                        pHmiVar->writeCnt--;
                    }
                    pHmiVar->err = VAR_COMM_TIMEOUT;
                    pHmiVar->wStatus = VAR_COMM_TIMEOUT;
                    /*comMsg.varId = ( pHmiVar->varId & 0x0000ffff ); //防止长变量
                    comMsg.type = 0; //0是read;
                    comMsg.err = VAR_COMM_TIMEOUT;
                    m_comMsgTmp->insert( comMsg.varId, comMsg );*/
                }
            }
            m_mutexWReq.unlock();
        }
        else
        {
            m_mutexWReq.lock();
            if ( pConnect->waitWrtieVarsList.count() > 0 )
            {
                struct HmiVar* pHmiVar = pConnect->waitWrtieVarsList[0];
                int writeCnt = sndWriteVarMsgToDriver( pConnect, pHmiVar );
                Q_ASSERT( writeCnt > 0 );
                if ( writeCnt > 0 )
                {
                    //pHmiVar->lastUpdateTime = getCurrTime();
                    pConnect->writingCnt = writeCnt;    //进入写状态
                    m_mutexWReq.unlock();
                    return true;
                }
                else
                {
                    pConnect->waitWrtieVarsList.takeAt( 0 );//出列,丢掉写请求
                    if(pHmiVar->writeCnt > 0)
                    {
                        pHmiVar->wData.remove(1, (pHmiVar->wData.size()/pHmiVar->writeCnt));
                        pHmiVar->writeCnt--;
                    }
                    //pHmiVar->err = VAR_PARAM_ERROR;
                    pHmiVar->wStatus = VAR_PARAM_ERROR;
                }
            }
            m_mutexWReq.unlock();
        }
    }
    return false;
}

bool PpiMpiService::trySndReadReq( struct PpiMpiConnect* pConnect, QHash<int , struct ComMsg> &m_comMsgTmp ) //不处理超长变量
{//qDebug()<<"trySndReadReq"<<"  pConnect->writingCnt = "<<pConnect->writingCnt<<"   pConnect->requestVarsList.count()"<<pConnect->requestVarsList.count();
    struct ComMsg comMsg;

        if ( pConnect->writingCnt == 0 && pConnect->requestVarsList.count() == 0 )
        {                   // qDebug()<<"pConnect->combinVarsList.count()  = "<<pConnect->combinVarsList.count();                                    //这个连接没有读写
            if ( pConnect->combinVarsList.count() > 0 )
            {
                if ( m_isMpi )
                {//qDebug()<<"\nm is mpi"<<"  pConnect->isLinking = "<<pConnect->isLinking;
                    if ( pConnect->isLinking == false )
                    {
                        if ( pConnect->connStatus != CONNECT_LINKED_OK  && pConnect->connStatus != CONNECT_LINKING  )
                        {                        //需要先建立连接
                            createMpiConnLink( pConnect );
                            //qDebug()<<" =======================================> createMpiConnLink";
                            return true;
                        }
                    }
                    else
                    {
                        if ( pConnect->connStatus == CONNECT_NO_OPERATION )
                        {//qDebug()<<"pConnect->connStatus == CONNECT_NO_OPERATION";
                            int currTime = m_currTime; //2.5秒钟没有建立连接成功就认为连接失败
                            if ( calIntervalTime( pConnect->beginTime, currTime ) >= 2500 )
                            {
                                pConnect->connStatus = CONNECT_NO_LINK;
                                //qDebug()<<"here 3";
                                comMsg.varId = 65535; //防止长变量
                                comMsg.type = 0; //0是read;
                                comMsg.err = VAR_DISCONNECT;
                                m_mutexGetMsg.lock();
                                m_comMsgTmp.insert( comMsg.varId, comMsg );
                                foreach( struct HmiVar* tmpHmiVar, pConnect->actVarsList )
                                {
                                    comMsg.varId = tmpHmiVar->varId; //防止长变量
                                    comMsg.type = 0; //0是read;
                                    comMsg.err = VAR_DISCONNECT;
                                    tmpHmiVar->err = VAR_COMM_TIMEOUT;
                                    tmpHmiVar->lastUpdateTime = -1;
                                    tmpHmiVar->overFlow = false;
                                    m_comMsgTmp.insert( comMsg.varId, comMsg );
                                }
                                m_mutexGetMsg.unlock();
                            }
                        }
                    }
                    if ( pConnect->connStatus == CONNECT_LINKED_OK || pConnect->connStatus == CONNECT_LINKING )
                    {//qDebug()<<"CONNECT_LINKED_OK";
                        pConnect->requestVarsList = pConnect->combinVarsList;
                        pConnect->requestVar = pConnect->combinVar;
                        for ( int i = 0; i < pConnect->combinVarsList.count(); i++ )
                        {                      //要把已经请求的变量从等待队列中删除
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
                            m_mutexUpdateVar.lock();
                            pConnect->requestVarsList.clear();
                            m_mutexUpdateVar.unlock();
                        }
                    }
                    else if ( pConnect->connStatus == CONNECT_NO_LINK )
                    {//qDebug()<<"CONNECT_NO_LINK";
                        m_mutexUpdateVar.lock();
                        pConnect->requestVarsList = pConnect->combinVarsList;
                        m_mutexUpdateVar.unlock();
                        for ( int i = 0; i < pConnect->combinVarsList.count(); i++ )
                        {                      //要把已经请求的变量从等待队列中删除
                            struct HmiVar* pHmiVar = pConnect->combinVarsList[i];
                            pConnect->waitUpdateVarsList.removeAll( pHmiVar );
                        }
                        pConnect->combinVarsList.clear();
                        for ( int i = 0; i < pConnect->requestVarsList.count(); i++ )
                        {
                            struct HmiVar* pHmiVar = pConnect->requestVarsList[i];
                            pHmiVar->err = VAR_COMM_TIMEOUT;
                            comMsg.varId = ( pHmiVar->varId & 0x0000ffff ); //防止长变量
                            comMsg.type = 0; //0是read;
                            comMsg.err = VAR_COMM_TIMEOUT;
                            m_mutexGetMsg.lock();
                            m_comMsgTmp.insert( comMsg.varId, comMsg );
                            m_mutexGetMsg.unlock();
                        }
                        m_mutexUpdateVar.lock();
                        pConnect->requestVarsList.clear();
                        m_mutexUpdateVar.unlock();
                    }
                }
                else
                {//qDebug()<<"\nm is ppi";
                    m_mutexUpdateVar.lock();
                    pConnect->requestVarsList = pConnect->combinVarsList;
                    m_mutexUpdateVar.unlock();
                    pConnect->requestVar = pConnect->combinVar;
                    for ( int i = 0; i < pConnect->combinVarsList.count(); i++ )
                    {                      //要把已经请求的变量从等待队列中删除
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
                        m_mutexUpdateVar.lock();
                        pConnect->requestVarsList.clear();
                        m_mutexUpdateVar.unlock();
                }
            }
        }
    }
    return false;
}

bool PpiMpiService::updateWriteReq( struct PpiMpiConnect* pConnect )
{
    bool newUpdateVarFlag = false;
    QList<struct HmiVar*> tmpList;
    m_mutexWReq.lock();
    //int currTime = m_currTime;//getCurrTime();
    for ( int i = 0; i < pConnect->wrtieVarsList.count(); i++ )
    {                                  // 找出需要刷新的变量
        struct HmiVar* pHmiVar = pConnect->wrtieVarsList[i];


        if ( !pConnect->waitWrtieVarsList.contains( pHmiVar ) )
        {
            /*if ( pConnect->waitUpdateVarsList.contains( pHmiVar ) )
                pConnect->waitUpdateVarsList.removeAll( pHmiVar );

            if ( pConnect->requestVarsList.contains( pHmiVar )  )
                pConnect->requestVarsList.removeAll( pHmiVar );*/

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

bool PpiMpiService::updateReadReq( struct PpiMpiConnect* pConnect )
{
    //qDebug()<<"updateReadReq";
    bool newUpdateVarFlag = false;
    m_mutexUpdateVar.lock();
    int currTime = m_currTime;//getCurrTime();
    //if( pConnect->connStatus == CONNECT_LINKED_OK )
    //{
    for ( int i = 0; i < pConnect->actVarsList.count(); i++ )
    {                                  // 找出需要刷新的变量
        struct HmiVar* pHmiVar = pConnect->actVarsList[i];
        if ( pHmiVar->lastUpdateTime == -1 || ( calIntervalTime( pHmiVar->lastUpdateTime, currTime ) >= pHmiVar->cycleTime ) )
        {                              // 需要刷新
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
    /*}
    else
    {
        pConnect->waitUpdateVarsList.clear();
        if( pConnect->actVarsList.count()>0 )
        {//qDebug()<<"connectTestVar  lastUpdateTime  = "<<pConnect->connectTestVar.lastUpdateTime;
            pConnect->actVarsList[0]->lastUpdateTime = pConnect->connectTestVar.lastUpdateTime;
            pConnect->connectTestVar = *(pConnect->actVarsList[0]);
            pConnect->connectTestVar.varId = 65535;
            pConnect->connectTestVar.cycleTime = 2000;
            if (( pConnect->connectTestVar.lastUpdateTime == -1 )
                    || ( calIntervalTime( pConnect->connectTestVar.lastUpdateTime, currTime ) >= pConnect->connectTestVar.cycleTime ) )
            {
                if (( !pConnect->waitUpdateVarsList.contains( &( pConnect->connectTestVar ) ) )
                        && ( !pConnect->waitWrtieVarsList.contains( &( pConnect->connectTestVar ) ) )
                        && ( !pConnect->requestVarsList.contains( &( pConnect->connectTestVar ) ) ) )
                {
                    pConnect->waitUpdateVarsList.append( &( pConnect->connectTestVar ) );
                        newUpdateVarFlag = true;
                }
            }
        }
    }*/
    m_mutexUpdateVar.unlock();

    //清除正在写的变量
    /*m_mutexWReq.lock();
    for ( int i = 0; i < pConnect->waitWrtieVarsList.count(); i++ )
    {                                  // 找出需要刷新的变量
        struct HmiVar* pHmiVar = pConnect->waitWrtieVarsList[i];

        if ( pConnect->waitUpdateVarsList.contains( pHmiVar ) )
            pConnect->waitUpdateVarsList.removeAll( pHmiVar );

        if ( pConnect->requestVarsList.contains( pHmiVar )  )
            pConnect->requestVarsList.removeAll( pHmiVar );
    }
    m_mutexWReq.unlock();*/
    return newUpdateVarFlag;
}

bool PpiMpiService::updateOnceRealReadReq( struct PpiMpiConnect* pConnect )
{
    bool newUpdateVarFlag = false;
    m_mutexOnceReal.lock();
    //int currTime = m_currTime;//getCurrTime();
    for ( int i = 0; i < pConnect->onceRealVarsList.count(); i++ )
    {                                  // 找出需要刷新的变量
        struct HmiVar* pHmiVar = pConnect->onceRealVarsList[i];
        if (( !pConnect->waitUpdateVarsList.contains( pHmiVar ) )
                        && ( !pConnect->waitWrtieVarsList.contains( pHmiVar ) ) //写的时候还是要去读该变量
                        //&& ( !pConnect->combinVarsList.contains( pHmiVar ) )    有可能被清除所以还是要加入等待读队列
                        && ( !pConnect->requestVarsList.contains( pHmiVar ) ) )
        {
            pConnect->waitUpdateVarsList.prepend( pHmiVar );  //插入队列最前面堆栈操作后进先读
            newUpdateVarFlag = true;
        }
        else
        {
            if ( pConnect->waitUpdateVarsList.contains( pHmiVar ) )
            {
                //提前他
                pConnect->waitUpdateVarsList.removeAll( pHmiVar );
                pConnect->waitUpdateVarsList.prepend( pHmiVar );
            }
        }
    }
    pConnect->onceRealVarsList.clear();// 清楚一次读列表
    m_mutexOnceReal.unlock();

    return newUpdateVarFlag;
}

void PpiMpiService::combineReadReq( struct PpiMpiConnect* pConnect )
{
    QList<struct HmiVar*> combinVarsList; //合并待请求的变量List
    QList<struct HmiVar*> groupVarsList;  //组合待请求的变量List
    int lenCombine = 0;
    int lenGroup = 0;
    //pConnect->combinVarsList.clear();
    if ( pConnect->waitUpdateVarsList.count() >= 1 )  //暂时先组一个
    {
        struct HmiVar* pHmiVar = pConnect->waitUpdateVarsList[0];
        combinVarsList.append( pHmiVar );
        groupVarsList.append( pHmiVar );
        pConnect->combinVar = *pHmiVar;

        if ( pHmiVar->area == PPI_AP_AREA_T_IEC )
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
        {
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
        }
    }
    for ( int i = 1; i < pConnect->waitUpdateVarsList.count(); i++ )
    {
        struct HmiVar* pHmiVar = pConnect->waitUpdateVarsList[i];
        if ( pHmiVar->area == pConnect->combinVar.area &&
             pHmiVar->subArea == pConnect->combinVar.subArea &&
             pConnect->combinVar.overFlow == pHmiVar->overFlow &&
             ( pHmiVar->overFlow == false ) )
        {                               //没有越界才合并，否则不合并
            int startOffset = qMin( pConnect->combinVar.addOffset, pHmiVar->addOffset );
            int endOffset = 0;
            int newLen = 0;
            if ( pConnect->combinVar.area == PPI_AP_AREA_T_IEC )
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
            {
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
            }
        }
        //组合
        int dataLen; // = 0;//= 12 + 4;
        if ( /*pConnect->combinVar.area*/ pHmiVar->area == PPI_AP_AREA_T_IEC )
            dataLen = 5 * (( pHmiVar->len + 3 ) / 4 );
        else if ( /*pConnect->combinVar.area*/ pHmiVar->area == PPI_AP_AREA_C_IEC )
            dataLen = 3 * (( pHmiVar->len + 1 ) / 2 );
        else
        {
            Q_ASSERT( pHmiVar->area != PPI_AP_AREA_HC_IEC );
            if ( pHmiVar->dataType == Com_Bool )
                dataLen = ( pHmiVar->bitOffset + pHmiVar->len + 7 ) / 8;
            else
                dataLen = pHmiVar->len;
        }
        if (( lenGroup + dataLen + ( i + 1 )*12 <= MAX_PPI_FRAME_LEN ) && groupVarsList.count() < 10 )
        {
            groupVarsList.append( pHmiVar );
            lenGroup += dataLen;
        }
    }
    if ( lenCombine >= lenGroup )  //哪个长就用哪种合并
        pConnect->combinVarsList = combinVarsList;
    else
    {
        pConnect->combinVarsList = groupVarsList;
        pConnect->combinVar.addOffset = -1;
        pConnect->combinVar.len = -1;
    }
}

bool PpiMpiService::hasProcessingReq( struct PpiMpiConnect* pConnect )
{
    return ( pConnect->writingCnt > 0 || pConnect->requestVarsList.count() > 0 /*|| pConnect->isLinking */);
}

void  PpiMpiService::initPpiMpiDriver()
{
    INT8U buf[MAX_SHORT_BUF_LEN];
    memset(buf, 0, MAX_SHORT_BUF_LEN);
    L2_MSG_STRU *pL2Msg = ( L2_MSG_STRU * ) & buf;
    struct PpiMpiConnect* conn = m_connectList[0];
    //printf("from %d %d\n",&(pL2Msg->MsgMem),&(pL2Msg->Frome));
    //printf("unit %d\n",&(pL2Msg->unit.L2FmaPara));
    pL2Msg->Port = 0; //默认固定为0就行了
    pL2Msg->MsgMem = ShortBufMem;
    pL2Msg->MsgType = 6;
    pL2Msg->unit.L2FmaPara.IfMaster = true;
    pL2Msg->unit.L2FmaPara.RetryCount = 3;
    pL2Msg->unit.L2FmaPara.BaudRate = conn->cfg.baud;
    pL2Msg->unit.L2FmaPara.Addr = conn->cfg.hmiAddr;
    pL2Msg->unit.L2FmaPara.Hsa = PPI_HSA;//32;conn->cfg.hsa;
    pL2Msg->unit.L2FmaPara.GapFactor = 1;//GAP_FACTOR;
    pL2Msg->unit.L2FmaPara.DA =0xFF;//conn->cfg.cpuAddr;//PPI_NULL_ADDR;
    if ( m_isMpi )
        pL2Msg->unit.L2FmaPara.SetProtol = 0;//MPI_PROTO_M;
    else
        pL2Msg->unit.L2FmaPara.SetProtol = 1;//PPI_PROTO_M;
    pL2Msg->Saddr = 0xFF; /*conn->cfg.cpuAddr;*///PPI_NULL_ADDR;
    pL2Msg->Frome = 0xAA;
    m_isInitPPIMsgSend = true;
    m_rwSocket->writeDev((char*)buf,(int)MAX_SHORT_BUF_LEN );
}

bool PpiMpiService::initCommDev()
{

    if ( m_connectList.count() > 0 )
    {
        struct PpiMpiConnect* conn = m_connectList[0];

        UartCfg uartCfg;
        uartCfg.COMNum = conn->cfg.COM_interface/* == 0*/;
        uartCfg.baud = conn->cfg.baud;
        uartCfg.dataBit = 8;
        uartCfg.parity = 2;
        uartCfg.stopBit = 1;

        m_rwSocket = new RWSocket(PPIMPI, &uartCfg, conn->cfg.id);
        connect( m_rwSocket, SIGNAL( readyRead() ),this, SLOT( slot_readCOMData( ) ), Qt::QueuedConnection) ;
        m_rwSocket->openDev();
        m_iniDriverFlg = false;
        m_isInitPPIMsgSend = false;
/*
        INT8U buf[MAX_SHORT_BUF_LEN];
        //memset(buf, 0, sizeof(buf));
        L2_MSG_STRU *pL2Msg = ( L2_MSG_STRU * ) & buf;
        pL2Msg->Port = 0; //默认固定为0就行了
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

bool PpiMpiService::createMpiConnLink( struct PpiMpiConnect* pConnect )
{
    INT8U buf[MAX_SHORT_BUF_LEN];
    L2_MSG_STRU *pL2Msg = ( L2_MSG_STRU * ) & buf;
    memset(buf,0,MAX_SHORT_BUF_LEN);

    Q_ASSERT( m_isMpi );
    if ( pConnect->isLinking == false )
    {
        pConnect->isLinking = true;
        pConnect->beginTime = m_currTime;
        //memset(buf, 0, sizeof(buf));
        pL2Msg->Port = 0;
        pL2Msg->MsgMem = ShortBufMem;
        pL2Msg->MsgType = L2_MSG_CREAT_LINK;
        pL2Msg->unit.L2FmaPara.DA = pConnect->cfg.cpuAddr;
        pL2Msg->Saddr = pConnect->cfg.cpuAddr;
        pL2Msg->Frome = 0xAA;
        //devUseCnt++;

        //int ret = write( m_fd, ( char* )buf, MAX_SHORT_BUF_LEN );
        //qDebug()<<"MAX_SHORT_BUF_LEN "<<MAX_SHORT_BUF_LEN<<" sizeof(L7) "<<sizeof(L7_MSG_STRU)<<" L2 "<<sizeof(L2_MSG_STRU);

        m_rwSocket->writeDev(( char* )buf, MAX_SHORT_BUF_LEN);
        //Q_ASSERT( ret == MAX_SHORT_BUF_LEN );
    }
    return true;
}

void PpiMpiService::slot_readCOMData()
{
    char buf[300];
    int len;
    L7_MSG_STRU *L7Msg = (L7_MSG_STRU *)buf;
    //PpiMpiConnect *conn = m_connectList[0];
    //uchar pBuf[2] = {0x00,0x00};
    len = m_rwSocket->readDev((char*)L7Msg);
    while (len > 0)
    {
        if ( len > 0 && L7Msg->Err == COM_OPEN_ERR )
        {
            QThread::currentThread()->msleep(1000);
           // qDebug()<<"slot "<<QThread::currentThread();
            for ( int i = 0; i < m_connectList.count(); i++ )
            {
                struct PpiMpiConnect* pConnect = m_connectList[i];
                Q_ASSERT( pConnect );

                pConnect->connStatus = CONNECT_NO_LINK;//qDebug()<<"here 4";
                struct ComMsg comMsg;
                comMsg.varId = 65535; //防止长变量
                comMsg.type = 0; //0是read;
                comMsg.err = VAR_DISCONNECT;
                m_mutexGetMsg.lock();
                m_comMsgTmp.insert( comMsg.varId, comMsg );
                foreach( struct HmiVar* tmpHmiVar, pConnect->actVarsList )
                {
                    comMsg.varId = tmpHmiVar->varId; //防止长变量
                    comMsg.type = 0; //0是read;
                    comMsg.err = VAR_DISCONNECT;
                    tmpHmiVar->err = VAR_COMM_TIMEOUT;
                    tmpHmiVar->overFlow = false;
                    tmpHmiVar->lastUpdateTime = -1;
                    m_comMsgTmp.insert( comMsg.varId, comMsg );
                }
                m_mutexGetMsg.unlock();

                pConnect->isLinking = false;
                m_mutexUpdateVar.lock();
                pConnect->requestVarsList.clear();
                m_mutexUpdateVar.unlock();
                pConnect->wrtieVarsList.clear();
                pConnect->writingCnt = 0;

            }
            //initPpiMpiDriver();
            m_iniDriverFlg = false;
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

bool PpiMpiService::processRspData( char *pCharMsg, QHash<int, struct ComMsg> &m_comMsgTmp )
{
    int i= 0;
    //INT8U buf[MAX_LONG_BUF_LEN];
    L7_MSG_STRU *pMsg =(L7_MSG_STRU*)pCharMsg;
    struct PpiMpiConnect* pConnect = NULL;


    for ( i = 0; i < m_connectList.count(); i++ )
    {
        pConnect = m_connectList[i];
        Q_ASSERT( pConnect );

        if ( pConnect->cfg.cpuAddr == pMsg->Saddr ) //属于这个连接
        {
            if ( m_isMpi && ( pMsg->MsgType == L7_OneLink_IsLinked_Ok ) )
            {
                if ( pMsg->Err == PPI_ERR_NO_ERR && pMsg->Saddr <= PPI_HSA ) // 连接成功
                {
                    pConnect->connStatus = CONNECT_LINKING;
                }
                else
                {
                    pConnect->connStatus = CONNECT_NO_LINK;//qDebug()<<"here 4";
                    struct ComMsg comMsg;
                    comMsg.varId = 65535; //防止长变量
                    comMsg.type = 0; //0是read;
                    comMsg.err = VAR_DISCONNECT;
                    m_mutexGetMsg.lock();
                    m_comMsgTmp.insert( comMsg.varId, comMsg );
                    foreach( struct HmiVar* tmpHmiVar, pConnect->actVarsList )
                    {
                        comMsg.varId = tmpHmiVar->varId; //防止长变量
                        comMsg.type = 0; //0是read;
                        comMsg.err = VAR_DISCONNECT;
                        tmpHmiVar->err = VAR_COMM_TIMEOUT;
                        tmpHmiVar->overFlow = false;
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
                qDebug() << "unexpect data packet";
                //Q_ASSERT( false );
            }
            break;
        }
        else
        {
            if (pMsg->Saddr == 0xFF)
            {
                /*HMI ppi device had been init  */
                m_iniDriverFlg = true;

                //m_runTime->start(10);
                qDebug(">>>>>>>0xff iniDriverFlg = true;");
                break;
            }
            else if(pMsg->Saddr == 0xFE)
            {
                /*HMI ppi device had not been init  */
                m_iniDriverFlg = false;
                pConnect->isLinking = false;
                m_isInitPPIMsgSend = false;
                //m_runTime->start(10);
                qDebug(">>>>>>>0xfe iniDriverFlg = false;");
                break;
            }
            else
            {
            }
        }
    }

    if(i == m_connectList.count())
    {
        /*发送初始化L1L2主站 和 发送 CreateMPILink 消息的时候超时进入这里*/
        if(pMsg->Err == VAR_TIMEOUT)
        {
            m_iniDriverFlg = false;
            m_isInitPPIMsgSend = false;
            for ( i = 0; i < m_connectList.count(); i++ )
            {
                pConnect = m_connectList[i];
                pConnect->isLinking = false;
            }
#if 0
            qDebug()<<">>>>>>>send initmsg or CreateMPILink msg err iniDriverFlg = false  ";
            qDebug()<<"connect addr = "<<pConnect->cfg.cpuAddr
                    <<" pMsg->Saddr "<<pMsg->Saddr
					<<" connnect baud = "<<pConnect->cfg.baud
                    <<" pMsg->Err "<<pMsg->Err;
#endif
        }
    }

    return 0;//bRet;
}

void PpiMpiService::slot_run()
{
    initCommDev();
    m_runTime->start(1000);
    qDebug()<<"run "<<QThread::currentThread();
}

void PpiMpiService::slot_runTimeout()
{
    m_runTime->stop();
    int connectCount = m_connectList.count();
    m_currTime = getCurrTime();

    if (!m_iniDriverFlg)
    {
        if(!m_isInitPPIMsgSend)
        {
             initPpiMpiDriver();
             m_rwSocket->setTimeoutPollCnt(connectCount);
        }        
        m_mutexGetMsg.lock();
        if ( m_comMsgTmp.count() > 0 )
        {
            foreach( int key, m_comMsgTmp.keys() )
            {
                m_comMsg.insert( key, m_comMsgTmp.value( key ) );
            }
            m_comMsgTmp.clear();
        }
        m_mutexGetMsg.unlock();
        m_runTime->start(1000);
        return;
    }

    for ( int i = 0; i < connectCount; i++ )
    {                              // 处理写请求
        struct PpiMpiConnect* pConnect = m_connectList[i];
        /*if (pConnect->pollCnt--<0)
        {
            pConnect->requestVarsList.clear();
            pConnect->wrtieVarsList.clear();
            pConnect->writingCnt = 0;
        }*/
        updateWriteReq( pConnect );
        trySndWriteReq( pConnect, m_comMsgTmp );
    }




    m_mutexGetMsg.lock();
        if ( m_comMsgTmp.count() > 0 )
        {
            foreach( int key, m_comMsgTmp.keys() )
            {
                m_comMsg.insert( key, m_comMsgTmp.value( key ) );
            }
            m_comMsgTmp.clear();
    }
    m_mutexGetMsg.unlock();
    for ( int i = 0; i < connectCount; i++ )
    {                              // 处理读请求
        struct PpiMpiConnect* pConnect = m_connectList[i];

        updateOnceRealReadReq( pConnect );  //先重组一次更新变量
        updateReadReq( pConnect ); //无论如何都要重组请求变量

        if ( !( hasProcessingReq( pConnect ) ) )
            combineReadReq( pConnect );
        trySndReadReq( pConnect , m_comMsgTmp );

        //if ( hasProcessingReq( pConnect ) )
         //   hasProcessingReqFlag = true; //有正在处理的请求
    }
    m_runTime->start( 50 );         //休眠100ms
        //回去继续尝试更新发送，直到1个也发送不下去
}


