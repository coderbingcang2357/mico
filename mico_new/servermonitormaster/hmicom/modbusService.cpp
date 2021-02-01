#include "modbusService.h"


#define MAX_MBUS_PACKET_LEN  226  //最大一包160字节，其实能达到240

ModbusService::ModbusService( struct ModbusConnectCfg& connectCfg )
{
    struct ModbusConnect* modbusConnect = new ModbusConnect;

    modbusConnect->cfg = connectCfg;
    modbusConnect->writingCnt = 0;
    modbusConnect->connStatus = CONNECT_NO_OPERATION;
    modbusConnect->connectTestVar.lastUpdateTime = -1;
    m_connectList.append( modbusConnect );
    m_currTime = getCurrTime();
    m_runTimer = NULL;
    m_rwSocket = NULL;
}

ModbusService::~ModbusService()
{
    if(m_runTimer){
        m_runTimer->stop();
        delete m_runTimer;
    }
    if(m_rwSocket){
        delete m_rwSocket;
        m_rwSocket = NULL;
    }
	// add by wubc 2021-01-18
	qDebug() << "~PpiMpiService";
	if(!m_connectList.isEmpty()){
		//qDebug() << "del GateWayService len:" << m_connectList.length();
		QList<struct ModbusConnect*>::iterator item = m_connectList.begin();
		while(item != m_connectList.end()){
			//DelGateWayConnect(*item);
			delete *item;
			*item = NULL;
			++item;
		}
		m_connectList.clear();
	}
}

bool ModbusService::meetServiceCfg( int serviceType, int COM_interface )
{
    if ( serviceType == SERVICE_TYPE_MODBUS )
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

int ModbusService::getConnLinkStatus( int connectId )
{
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct ModbusConnect* modbusConnect = m_connectList[i];
        if ( connectId == modbusConnect->cfg.id )
            return modbusConnect->connStatus;
    }
    return -1;
}

int ModbusService::getComMsg( QList<struct ComMsg> &msgList )
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

int ModbusService:: splitLongVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct ModbusConnect* modbusConnect = m_connectList[i];
        if ( pHmiVar->connectId == modbusConnect->cfg.id )
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
                            //qDebug()<<"splitLongVar  id = "<<pSplitHhiVar->varId;
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
                            //qDebug()<<"splitLongVar  id = "<<pSplitHhiVar->varId;
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
                        //qDebug()<<"splitLongVar  id = "<<pSplitHhiVar->varId;
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

int ModbusService::onceRealUpdateVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    m_mutexOnceReal.lock();
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct ModbusConnect* modbusConnect = m_connectList[i];
        if ( pHmiVar->connectId == modbusConnect->cfg.id )
        {
            pHmiVar->lastUpdateTime = -1; //立即更新
            if ( pHmiVar->splitVars.count() > 0 )
            {
                for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
                {
                    struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                    pSplitHhiVar->lastUpdateTime = -1;
                    pSplitHhiVar->overFlow = false;
                    if ( !modbusConnect->onceRealVarsList.contains( pSplitHhiVar ) )
                        modbusConnect->onceRealVarsList.append( pSplitHhiVar );
                }
            }
            else
            {
                pHmiVar->lastUpdateTime = -1;
                pHmiVar->overFlow = false;
                if ( !modbusConnect->onceRealVarsList.contains( pHmiVar ) )
                    modbusConnect->onceRealVarsList.append( pHmiVar );
            }
            //qDebug() << "startUpdateVar：" << pHmiVar->varId;

            m_mutexOnceReal.unlock();
            return 0;
        }
    }
    m_mutexOnceReal.unlock();
    return -1;
}

int ModbusService::startUpdateVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    m_mutexUpdateVar.lock();
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct ModbusConnect* modbusConnect = m_connectList[i];
        if ( pHmiVar->connectId == modbusConnect->cfg.id )
        {
            pHmiVar->lastUpdateTime = -1; //立即更新
            if ( pHmiVar->splitVars.count() > 0 )
            {
                for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
                {
                    struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                    pSplitHhiVar->lastUpdateTime = -1;
                    pSplitHhiVar->overFlow = false;
                    if ( !modbusConnect->actVarsList.contains( pSplitHhiVar ) )
                        modbusConnect->actVarsList.append( pSplitHhiVar );
                }
            }
            else
            {
                pHmiVar->lastUpdateTime = -1;
                pHmiVar->overFlow = false;
                if ( !modbusConnect->actVarsList.contains( pHmiVar ) )
                    modbusConnect->actVarsList.append( pHmiVar );
            }
            //qDebug() << "startUpdateVar：" << pHmiVar->varId;

            m_mutexUpdateVar.unlock();
            return 0;
        }
    }
    m_mutexUpdateVar.unlock();
    return -1;
}

int ModbusService::stopUpdateVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    m_mutexUpdateVar.lock();
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct ModbusConnect* modbusConnect = m_connectList[i];
        if ( pHmiVar->connectId == modbusConnect->cfg.id )
        {
            if ( pHmiVar->splitVars.count() > 0 )
            {
                for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
                {
                    struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                    modbusConnect->actVarsList.removeAll( pSplitHhiVar );
                }
            }
            else
                modbusConnect->actVarsList.removeAll( pHmiVar );
            m_mutexUpdateVar.unlock();
            return 0;
        }
    }
    m_mutexUpdateVar.unlock();
    return -1;
}

int ModbusService::readVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
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
                    {                  //奇偶字节调换
                        if ( pHmiVar->dataType != Com_String )
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

int ModbusService::readDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    Q_ASSERT( pHmiVar && pBuf );
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct ModbusConnect* modbusConnect = m_connectList[i];
        if ( pHmiVar->connectId == modbusConnect->cfg.id )
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

int ModbusService::writeVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
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
                    if ( pHmiVar->dataType != Com_String )
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

int ModbusService::writeDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    Q_ASSERT( pHmiVar && pBuf );
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct ModbusConnect* modbusConnect = m_connectList[i];
        if ( pHmiVar->connectId == modbusConnect->cfg.id )
        {
            if ( modbusConnect->connStatus != CONNECT_LINKED_OK )
            {
                return  VAR_COMM_TIMEOUT;
            }
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
                    modbusConnect->wrtieVarsList.append( pSplitHhiVar );
                    pBuf += len;
                    bufLen -= len;
                }
            }
            else
            {
                ret = writeVar( pHmiVar, pBuf, bufLen );
                modbusConnect->wrtieVarsList.append( pHmiVar );
            }
            m_mutexWReq.unlock();

            return ret; //是否要返回成功
        }
    }
    return -1;
}

int ModbusService::addConnect( struct ModbusConnectCfg& connectCfg )
{                                      //要检查是否重复 add
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct ModbusConnect* modbusConnect = m_connectList[i];
        Q_ASSERT( modbusConnect->cfg.COM_interface == connectCfg.COM_interface );//绝对要是同一个设备接口
        if ( connectCfg.id == modbusConnect->cfg.id )
        {
            Q_ASSERT( false );
            return 0;
        }
    }
    struct ModbusConnect* modbusConnect = new ModbusConnect;
    modbusConnect->cfg = connectCfg;
    modbusConnect->writingCnt = 0;
    modbusConnect->connStatus = CONNECT_NO_OPERATION;
    modbusConnect->connectTestVar.lastUpdateTime = -1;
    m_connectList.append( modbusConnect );
    return 0;
}

int ModbusService::sndWriteVarMsgToDriver( struct ModbusConnect* modbusConnect, struct HmiVar* pHmiVar )
{
    int sendedLen = 0;
    Q_ASSERT( modbusConnect && pHmiVar );
//#ifdef Q_WS_QWS
    struct MbusMsgPar msgPar;

    m_mutexRw.lock();
    msgPar.sfd = 1;   //写
    msgPar.rw = 1;   //写
    msgPar.slave = modbusConnect->cfg.cpuAddr;
    msgPar.area = pHmiVar->area;
    msgPar.addr = pHmiVar->addOffset;
    msgPar.err = 0;

    if ( msgPar.area == MODBUX_AREA_3X || msgPar.area == MODBUX_AREA_4X )
    {
        if ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup )
        {                              // 位变量长度为位的个数
            Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
            int count = ( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16;  //字数
            Q_ASSERT( count > 0 && count <= 120 );
            if ( count > 120 )
                count = 120;
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
            Q_ASSERT( count > 0 && count <= 120 );
            if ( count > 120 )
                count = 120;
            msgPar.count = count;      // 占用的总字数
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
    else if ( msgPar.area == MODBUX_AREA_0X || msgPar.area == MODBUX_AREA_1X )
    {                                  // 位变量长度为位的个数
        Q_ASSERT( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup );
        int count = pHmiVar->len;
        Q_ASSERT( count > 0 &&  count <= 120*8 );
        if ( count > 8*120 )
            count = 8 * 120;
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
    //qDebug()<<"sendedLen = "<<sendedLen;
    if ( sendedLen > 0 )
    {
        m_devUseCnt++;
        m_mutexRw.unlock();
        //int len = m_modbus->write( ( char* ) & msgPar, sizeof( struct MbusMsgPar ) );
        int len = m_rwSocket->writeDev(( char* ) & msgPar, sizeof( struct MbusMsgPar ));
        Q_ASSERT( len > 0 );
        //qDebug()<<"\nWrite Len = \n"<<len;
    }
    else
        m_mutexRw.unlock();
//#endif
    return sendedLen;
}

int ModbusService::sndReadVarMsgToDriver( struct ModbusConnect* modbusConnect, struct HmiVar* pHmiVar )
{
    //qDebug()<<"in sndReadVarMsgToDriver";
    int sendedLen = 0;
    Q_ASSERT( modbusConnect && pHmiVar );
//#ifdef Q_WS_QWS
    struct MbusMsgPar msgPar;

    m_mutexRw.lock();
    msgPar.sfd = 0;    // 读
    msgPar.rw = 0;    // 读
    msgPar.slave = modbusConnect->cfg.cpuAddr;
    msgPar.area = pHmiVar->area;
    msgPar.addr = pHmiVar->addOffset & 0xffff;
    msgPar.err = 0;
    //qDebug()<<"msgPar.slave == "<<msgPar.slave<<"  msgPar.area == "<<msgPar.area<<"  msgPar.addr == "<<msgPar.addr;
    if ( msgPar.area == MODBUX_AREA_3X || msgPar.area == MODBUX_AREA_4X )
    {
        if ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup )
        {                              // 位变量长度为位的个数
            Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
            int count = ( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16;  //字数
            Q_ASSERT( count > 0 && count <= 120 );
            if ( count > 120 )
                count = 120;
            msgPar.count = count;      // 占用的总字数
            sendedLen = count * 16;
        }
        else
        {
            int count = ( pHmiVar->len + 1 ) / 2;  //字数
            Q_ASSERT( count > 0 && count <= 120 );
            if ( count > 120 )
                count = 120;
            msgPar.count = count;      // 占用的总字数
            sendedLen = count * 2;
        }
    }
    else if ( msgPar.area == MODBUX_AREA_0X || msgPar.area == MODBUX_AREA_1X )
    {                                  // 位变量长度为位的个数
        Q_ASSERT( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup );
        int count = pHmiVar->len;
        Q_ASSERT( count > 0 &&  count <= 120*8 );
        if ( count > 8*120 )
            count = 8 * 120;
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
        //int len = m_modbus->write( ( char* ) & msgPar, sizeof( struct MbusMsgPar ) );
        int len = m_rwSocket->writeDev( ( char* ) & msgPar, sizeof( struct MbusMsgPar ));
        //qDebug()<<"sendedLen = "<<len;
        Q_ASSERT( len > 0 );
    }
    else
    {
        m_mutexRw.unlock();
    }
//#endif
    return sendedLen;
}

bool isWaitingReadRsp( struct ModbusConnect* modbusConnect, struct HmiVar* pHmiVar )
{
    Q_ASSERT( modbusConnect && pHmiVar );
    if ( modbusConnect->waitUpdateVarsList.contains( pHmiVar ) )
        return true;
    else if ( modbusConnect->combinVarsList.contains( pHmiVar ) )
        return true;
    else if ( modbusConnect->requestVarsList.contains( pHmiVar ) )
        return true;
    else
        return false;
}

bool ModbusService::trySndWriteReq( struct ModbusConnect* modbusConnect ) //不处理超长变量
{
    if ( m_devUseCnt < MAXDEVCNT )                          //没有正在读写, snd read, 一次只能发送一包请求
    {
        //qDebug()<<"writingCnt = "<<modbusConnect->writingCnt<<"  requestVarsList.count = "<<modbusConnect->requestVarsList.count();
        if ( modbusConnect->writingCnt == 0 && modbusConnect->requestVarsList.count() == 0 ) //这个连接没有读写
        {
            //qDebug()<<"waitWrtieVarsList.count = "<<modbusConnect->waitWrtieVarsList.count();
            m_mutexWReq.lock();
            if ( modbusConnect->waitWrtieVarsList.count() > 0 )
            {
                struct HmiVar* pHmiVar = modbusConnect->waitWrtieVarsList[0];
                if (( pHmiVar->lastUpdateTime == -1 )  //3X,4X 位变量需要先读
                                && ( pHmiVar->area == MODBUX_AREA_3X || pHmiVar->area == MODBUX_AREA_4X )
                                && ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup || pHmiVar->dataType == Com_String ) )
                {
                    //pHmiVar->lastUpdateTime = m_currTime;//getCurrTime();有可能还没读上来就开始写
                    if ( !isWaitingReadRsp( modbusConnect, pHmiVar ) )
                        modbusConnect->waitUpdateVarsList.prepend( pHmiVar ); //加入到读队列最前,保证下次最先读
                }
                else if ( !isWaitingReadRsp( modbusConnect, pHmiVar ) ) //没有正在等待读
                {                                   //直接写,snd write cmd to driver
                    if ( sndWriteVarMsgToDriver( modbusConnect, pHmiVar ) > 0 )
                    {
                        //pHmiVar->lastUpdateTime = m_currTime;//getCurrTime(); 变量很多的时候有可能等太久
                        modbusConnect->writingCnt = 1;    //进入写状态
                        m_mutexWReq.unlock();

                        return true;
                    }
                    else
                    {
                        modbusConnect->waitWrtieVarsList.takeAt( 0 );//出列,丢掉写请求
                        pHmiVar->err = VAR_PARAM_ERROR;
                    }
                }
            }
            m_mutexWReq.unlock();
        }
    }
    return false;
}

bool ModbusService::trySndReadReq( struct ModbusConnect* modbusConnect ) //不处理超长变量
{
    if ( m_devUseCnt < MAXDEVCNT )             //没有正在读写, snd read, 一次只能发送一包请求
    {
        if ( modbusConnect->writingCnt == 0 && modbusConnect->requestVarsList.count() == 0 ) //这个连接没有读写
        {
            if ( modbusConnect->combinVarsList.count() > 0 )
            {
                modbusConnect->requestVarsList = modbusConnect->combinVarsList;
                modbusConnect->requestVar = modbusConnect->combinVar;
                for ( int i = 0; i < modbusConnect->combinVarsList.count(); i++ )
                {                      //要把已经请求的变量从等待队列中删除
                    struct HmiVar* pHmiVar = modbusConnect->combinVarsList[i];
                    modbusConnect->waitUpdateVarsList.removeAll( pHmiVar );
                    pHmiVar->lastUpdateTime = m_currTime; //这个时候才是变量上次刷新时间
                }
                modbusConnect->combinVarsList.clear();

                if ( sndReadVarMsgToDriver( modbusConnect, &( modbusConnect->requestVar ) ) > 0 )
                {
                    return true;
                }
                else
                {
                    for ( int i = 0; i < modbusConnect->requestVarsList.count(); i++ )
                    {
                        struct HmiVar* pHmiVar = modbusConnect->requestVarsList[i];
                        pHmiVar->err = VAR_PARAM_ERROR;
                    }
                    modbusConnect->requestVarsList.clear();
                }
            }
        }
    }
    return false;
}

bool ModbusService::updateWriteReq( struct ModbusConnect* modbusConnect )
{
    bool newUpdateVarFlag = false;
    QList<struct HmiVar*> tmpList;
    m_mutexWReq.lock();
    //int currTime = m_currTime;//getCurrTime();
    for ( int i = 0; i < modbusConnect->wrtieVarsList.count(); i++ )
    {                                  // 找出需要刷新的变量
        struct HmiVar* pHmiVar = modbusConnect->wrtieVarsList[i];


        if ( !modbusConnect->waitWrtieVarsList.contains( pHmiVar ) )
        {
            /*if ( modbusConnect->waitUpdateVarsList.contains( pHmiVar ) )
                modbusConnect->waitUpdateVarsList.removeAll( pHmiVar );

            if ( modbusConnect->requestVarsList.contains( pHmiVar )  )
                modbusConnect->requestVarsList.removeAll( pHmiVar );*/

            modbusConnect->waitWrtieVarsList.append( pHmiVar );
            newUpdateVarFlag = true;
        }
        else
        {
            tmpList.append(pHmiVar);
        }
    }
    modbusConnect->wrtieVarsList.clear() ;
    modbusConnect->wrtieVarsList += tmpList;
    m_mutexWReq.unlock();

    return newUpdateVarFlag;
}

bool ModbusService::updateReadReq( struct ModbusConnect* modbusConnect )
{
    bool newUpdateVarFlag = false;
    m_mutexUpdateVar.lock();
    int currTime = m_currTime;//getCurrTime();
    if ( modbusConnect->connStatus == CONNECT_LINKED_OK )
    {
        for ( int i = 0; i < modbusConnect->actVarsList.count(); i++ )
        {                                  // 找出需要刷新的变量
            struct HmiVar* pHmiVar = modbusConnect->actVarsList[i];
            if (( pHmiVar->lastUpdateTime == -1 ) || ( calIntervalTime( pHmiVar->lastUpdateTime, currTime ) >= pHmiVar->cycleTime ) )
            {                              // 需要刷新
                if (( !modbusConnect->waitUpdateVarsList.contains( pHmiVar ) )
                                && ( !modbusConnect->waitWrtieVarsList.contains( pHmiVar ) )
                                //&& ( !modbusConnect->combinVarsList.contains( pHmiVar ) ) 好像多余，每次发送前都会清空一次
                                && ( !modbusConnect->requestVarsList.contains( pHmiVar ) ) )
                {
                    //pHmiVar->lastUpdateTime = currTime; 应该确定已经发送到驱动了才更新
                    modbusConnect->waitUpdateVarsList.append( pHmiVar );
                    newUpdateVarFlag = true;
                }
            }
        }
    }
    else
    {
        modbusConnect->waitUpdateVarsList.clear();
        if ( modbusConnect->actVarsList.count() > 0 && modbusConnect->requestVarsList.count() == 0 )
        {    //qDebug()<<"connectTestVar  lastUpdateTime  = "<<modbusConnect->connectTestVar.lastUpdateTime;
            modbusConnect->actVarsList[0]->lastUpdateTime = modbusConnect->connectTestVar.lastUpdateTime;
            modbusConnect->connectTestVar = *( modbusConnect->actVarsList[0] );
            modbusConnect->connectTestVar.varId = 65535;
            modbusConnect->connectTestVar.cycleTime = 2000;
            if (( modbusConnect->connectTestVar.lastUpdateTime == -1 )
                            || ( calIntervalTime( modbusConnect->connectTestVar.lastUpdateTime, currTime ) >= modbusConnect->connectTestVar.cycleTime ) )
            {
                if (( !modbusConnect->waitUpdateVarsList.contains( &( modbusConnect->connectTestVar ) ) )
                                && ( !modbusConnect->waitWrtieVarsList.contains( &( modbusConnect->connectTestVar ) ) )
                                && ( !modbusConnect->requestVarsList.contains( &( modbusConnect->connectTestVar ) ) ) )
                {
                    modbusConnect->waitUpdateVarsList.append( &( modbusConnect->connectTestVar ) );
                    newUpdateVarFlag = true;
                }
            }


        }
    }
    m_mutexUpdateVar.unlock();

    //清除正在写的变量
    /*m_mutexWReq.lock();
    for ( int i = 0; i < modbusConnect->waitWrtieVarsList.count(); i++ )
    {                                  // 找出需要刷新的变量
        struct HmiVar* pHmiVar = modbusConnect->waitWrtieVarsList[i];

        if ( modbusConnect->waitUpdateVarsList.contains( pHmiVar ) )
            modbusConnect->waitUpdateVarsList.removeAll( pHmiVar );

        if ( modbusConnect->requestVarsList.contains( pHmiVar )  )
            modbusConnect->requestVarsList.removeAll( pHmiVar );
    }
    m_mutexWReq.unlock();*/

    return newUpdateVarFlag;
}

bool ModbusService::updateOnceRealReadReq( struct ModbusConnect* modbusConnect )
{
    bool newUpdateVarFlag = false;
    m_mutexOnceReal.lock();
//    int currTime = m_currTime;//getCurrTime();
    for ( int i = 0; i < modbusConnect->onceRealVarsList.count(); i++ )
    {                                  // 找出需要刷新的变量
        struct HmiVar* pHmiVar = modbusConnect->onceRealVarsList[i];
        if (( !modbusConnect->waitUpdateVarsList.contains( pHmiVar ) )
                        && ( !modbusConnect->waitWrtieVarsList.contains( pHmiVar ) ) //写的时候还是要去读该变量
                        //&& ( !modbusConnect->combinVarsList.contains( pHmiVar ) )    有可能被清除所以还是要加入等待读队列
                        && ( !modbusConnect->requestVarsList.contains( pHmiVar ) ) )
        {
            modbusConnect->waitUpdateVarsList.prepend( pHmiVar );  //插入队列最前面堆栈操作后进先读
            newUpdateVarFlag = true;
        }
        else
        {
            if ( modbusConnect->waitUpdateVarsList.contains( pHmiVar ) )
            {
                //提前他
                modbusConnect->waitUpdateVarsList.removeAll( pHmiVar );
                modbusConnect->waitUpdateVarsList.prepend( pHmiVar );
            }
        }
    }
    modbusConnect->onceRealVarsList.clear();// 清楚一次读列表
    m_mutexOnceReal.unlock();

    return newUpdateVarFlag;
}

void ModbusService::combineReadReq( struct ModbusConnect* modbusConnect )
{
    modbusConnect->combinVarsList.clear();
    //qDebug()<<"waitUpdateVarsList.count() = "<<waitUpdateVarsList.count();
    if ( modbusConnect->waitUpdateVarsList.count() >= 1 )
    {
        struct HmiVar* pHmiVar = modbusConnect->waitUpdateVarsList[0];
        modbusConnect->combinVarsList.append( pHmiVar );
        modbusConnect->combinVar = *pHmiVar;
        if ( modbusConnect->combinVar.area == MODBUX_AREA_3X || modbusConnect->combinVar.area == MODBUX_AREA_4X )
        {
            if ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup )
            {
                Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
                modbusConnect->combinVar.dataType = Com_Word;
                modbusConnect->combinVar.len = 2 * (( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16 ); //2*count
                modbusConnect->combinVar.bitOffset = -1;
            }
        }
    }
    for ( int i = 1; i < modbusConnect->waitUpdateVarsList.count(); i++ )
    {
        struct HmiVar* pHmiVar = modbusConnect->waitUpdateVarsList[i];
        if ( pHmiVar->area == modbusConnect->combinVar.area && modbusConnect->combinVar.overFlow == pHmiVar->overFlow && ( pHmiVar->overFlow == false ) )
        {                            //没有越界才合并，否则不合并
            int startOffset = qMin( modbusConnect->combinVar.addOffset, pHmiVar->addOffset );
            int endOffset = 0;
            int newLen = 0;
            if ( modbusConnect->combinVar.area == MODBUX_AREA_3X || modbusConnect->combinVar.area == MODBUX_AREA_4X )
            {
                int combineCount = ( modbusConnect->combinVar.len + 1 ) / 2;
                int varCount = 0;

                if ( pHmiVar->dataType == Com_Bool || pHmiVar->dataType == Com_BitGroup )
                {
                    Q_ASSERT( pHmiVar->bitOffset >= 0 && pHmiVar->bitOffset <= 15 );
                    varCount = ( pHmiVar->bitOffset + pHmiVar->len + 15 ) / 16;
                }
                else
                    varCount = ( pHmiVar->len + 1 ) / 2;

                endOffset = qMax( modbusConnect->combinVar.addOffset + combineCount, pHmiVar->addOffset + varCount );
                newLen = 2 * ( endOffset - startOffset );
                if ( newLen <= MAX_MBUS_PACKET_LEN )  //最多组合 MAX_MBUS_PACKET_LEN 字节
                {
                    if ( newLen <= 2*( combineCount + varCount ) + 10 ) //能节省10字节
                    {
                        modbusConnect->combinVar.addOffset = startOffset;
                        modbusConnect->combinVar.len = newLen;
                        modbusConnect->combinVarsList.append( pHmiVar );
                    }
                }
                else
                    continue;
            }
            else
            {
                endOffset = qMax( modbusConnect->combinVar.addOffset + modbusConnect->combinVar.len, pHmiVar->addOffset + pHmiVar->len );
                int newLen = endOffset - startOffset;
                if ( newLen <= MAX_MBUS_PACKET_LEN*8 )
                {
                    if ( newLen <= modbusConnect->combinVar.len + pHmiVar->len + 8*8 ) //能节省8字节
                    {
                        modbusConnect->combinVar.addOffset = startOffset;
                        modbusConnect->combinVar.len = newLen;
                        modbusConnect->combinVarsList.append( pHmiVar );
                    }
                }
                else
                    continue;
            }
        }
    }
}

bool ModbusService::hasProcessingReq( struct ModbusConnect* modbusConnect )
{
    return ( modbusConnect->writingCnt > 0 || modbusConnect->requestVarsList.count() > 0 );
}

bool ModbusService::initCommDev()
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
        struct ModbusConnect* conn = m_connectList[0];
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

            m_rwSocket = new RWSocket(MODBUS_RTU, &uartCfg, conn->cfg.id);
            connect(m_rwSocket, SIGNAL(readyRead()),this,SLOT(slot_readData()), Qt::QueuedConnection);
            m_rwSocket->openDev();
            m_rwSocket->setTimeoutPollCnt(1);
        }
    }
    return true;
}

bool ModbusService::processRspData(struct MbusMsgPar msgPar ,struct ComMsg comMsg )
{
//#ifdef Q_WS_QWS
    bool result = true;

    if ( m_devUseCnt > 0 )
        m_devUseCnt--;

    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct ModbusConnect* modbusConnect = m_connectList[i];
        //qDebug()<<"processRspData  waitWrtieVarsList.count() = "<<modbusConnect->waitWrtieVarsList.count();
       /* qDebug()<<"processRspData  requestVarsList.count()   = "<<modbusConnect->requestVarsList.count();
        qDebug()<<"processRspData  waitWrtieVarsList.count() = "<<modbusConnect->waitWrtieVarsList.count();
        qDebug()<<"msgPar.rw == "<<msgPar.rw<<"   msgPar.slave == "
               <<msgPar.slave<<"  msgPar.area == "
               <<msgPar.area<<"  msgPar.addr == "
               <<msgPar.addr;*/
        Q_ASSERT( modbusConnect );
        if ( modbusConnect->cfg.cpuAddr == msgPar.slave ) //属于这个连接
        {
            if ( modbusConnect->writingCnt > 0 && modbusConnect->waitWrtieVarsList.count() > 0 && msgPar.rw == 1 )
            {
                Q_ASSERT( msgPar.rw == 1 );
                modbusConnect->writingCnt = 0;
                //qDebug() << "Write err:" << msgPar.err;
                struct HmiVar* pHmiVar = modbusConnect->waitWrtieVarsList.takeFirst(); //删除写请求记录
                m_mutexRw.lock();
                //pHmiVar->err = msgPar.err;
                pHmiVar->wStatus = msgPar.err;
                m_mutexRw.unlock();

                //加入消息队列
                comMsg.varId = ( pHmiVar->varId & 0x0000ffff ); //防止长变量
                comMsg.type = 1; // 1是write;
                comMsg.err = msgPar.err;
                m_mutexGetMsg.lock();
                m_comMsgTmp.insertMulti( comMsg.varId, comMsg );
                m_mutexGetMsg.unlock();
            }
            else if ( modbusConnect->requestVarsList.count() > 0 && msgPar.rw == 0 )
            {
                Q_ASSERT( msgPar.rw == 0 );
                if(msgPar.area != modbusConnect->requestVar.area)
                {
                    /*
                     *  第一包延迟，重发后收到，发第二包，此时来重发返回包就会出错！
                     *  因此不能用ASSERT,直接忽略就可以了
                     */
                    qDebug()<<"erea error"<<msgPar.area << modbusConnect->requestVar.area;
                    break;
                }
                //Q_ASSERT( msgPar.area == modbusConnect->requestVar.area );
                if ( msgPar.err == VAR_PARAM_ERROR && modbusConnect->requestVarsList.count() > 1 )
                {
                    for ( int j = 0; j < modbusConnect->requestVarsList.count(); j++ )
                    {
                        struct HmiVar* pHmiVar = modbusConnect->requestVarsList[j];
                        pHmiVar->overFlow = true;
                        pHmiVar->lastUpdateTime = -1; //合并后越界的不算越界，立即再次更新（更新时不合并）
                    }
                }
                else
                {
                    for ( int j = 0; j < modbusConnect->requestVarsList.count(); j++ )
                    {
                        m_mutexRw.lock();
                        struct HmiVar* pHmiVar = modbusConnect->requestVarsList[j];
                        Q_ASSERT( msgPar.area == pHmiVar->area );
                        pHmiVar->err = msgPar.err;
                        if ( msgPar.err == VAR_PARAM_ERROR )
                        {
                            pHmiVar->overFlow = true;

                            //加入消息队列
                            comMsg.varId = ( pHmiVar->varId & 0x0000ffff ); //防止长变量
                            comMsg.type = 0; //0是read;
                            comMsg.err = VAR_PARAM_ERROR;
                            m_mutexGetMsg.lock();
                            m_comMsgTmp.insert( comMsg.varId, comMsg );
                            m_mutexGetMsg.unlock();
                        }
                        else if ( msgPar.err == VAR_NO_ERR ) //没有写的时候才能更新
                        {
                            pHmiVar->overFlow = false;
                            if ( msgPar.area == MODBUX_AREA_3X || msgPar.area == MODBUX_AREA_4X )
                            {
                                Q_ASSERT( msgPar.count < 120 );
                                int offset = pHmiVar->addOffset - modbusConnect->requestVar.addOffset;
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
                                        memcpy( pHmiVar->rData.data(), &msgPar.data[2*offset], len );

                                        //加入消息队列
                                        if ( pHmiVar->wStatus != VAR_NO_OPERATION )
                                        {
                                            comMsg.varId = ( pHmiVar->varId & 0x0000ffff ); //防止长变量
                                            comMsg.type = 0; //0是read;
                                            comMsg.err = VAR_NO_ERR;
                                            m_mutexGetMsg.lock();
                                            m_comMsgTmp.insert( comMsg.varId, comMsg );
                                            m_mutexGetMsg.unlock();
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
                                        memcpy( pHmiVar->rData.data(), &msgPar.data[2*offset], len );
                                        //qDebug()<<pHmiVar->rData.toHex();
                                        //加入消息队列
                                        if ( pHmiVar->wStatus != VAR_NO_OPERATION )
                                        {
                                            comMsg.varId = ( pHmiVar->varId & 0x0000ffff ); //防止长变量
                                            comMsg.type = 0; //0是read;
                                            comMsg.err = VAR_NO_ERR;
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
                                Q_ASSERT( msgPar.count < 240*8 );
                                if ( !modbusConnect->waitWrtieVarsList.contains( pHmiVar ) )
                                {
                                    int startBitOffset = pHmiVar->addOffset - modbusConnect->requestVar.addOffset;
                                    Q_ASSERT( startBitOffset >= 0 );
                                    int len = ( pHmiVar->len + 7 ) / 8;
                                    if ( pHmiVar->rData.size() < len )
                                        pHmiVar->rData = QByteArray( len, 0 ); //初始化为0
                                    //    pHmiVar->rData.resize(len);
                                    for ( int k = 0; k < pHmiVar->len; k++ )
                                    {
                                        int rOffset = startBitOffset + k;
                                        if ( msgPar.data[rOffset/8] & ( 1 << ( rOffset % 8 ) ) )
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
                                        m_mutexGetMsg.lock();
                                        m_comMsgTmp.insert( comMsg.varId, comMsg );
                                        m_mutexGetMsg.unlock();
                                    }
                                }
                            }
                        }
                        else if ( msgPar.err == VAR_COMM_TIMEOUT )
                        {

                            if ( pHmiVar->varId == 65535 )
                            {
                                foreach( HmiVar* tempHmiVar, modbusConnect->actVarsList )
                                {
                                    comMsg.type = 0; //0是read;
                                    comMsg.err = VAR_DISCONNECT;

                                    tempHmiVar->err = VAR_COMM_TIMEOUT;
                                    tempHmiVar->lastUpdateTime = -1;
                                    comMsg.varId = ( tempHmiVar->varId & 0x0000ffff );
                                    m_mutexGetMsg.lock();
                                    m_comMsgTmp.insert( comMsg.varId, comMsg );
                                    m_mutexGetMsg.unlock();
                                }
                            }
                            else
                            {
                                comMsg.varId = ( pHmiVar->varId & 0x0000ffff ); //防止长变量
                                comMsg.type = 0; //0是read;

                                comMsg.err = VAR_COMM_TIMEOUT;
                                m_mutexGetMsg.lock();
                                m_comMsgTmp.insert( comMsg.varId, comMsg );
                                m_mutexGetMsg.unlock();
                            }
                        }
                        m_mutexRw.unlock();
                    }
                }
                modbusConnect->requestVarsList.clear();
            }
            if ( msgPar.err == VAR_COMM_TIMEOUT )
            {
                modbusConnect->writingCnt = 0;
                modbusConnect->waitWrtieVarsList.clear();
                modbusConnect->requestVarsList.clear();
                modbusConnect->connStatus = CONNECT_NO_LINK;
            }
            else
            {
                modbusConnect->connStatus = CONNECT_LINKED_OK;
            }
            break;
        }
    }
    return result;
}


void ModbusService::slot_run()
{
    //emit startRunTimer();
    qDebug()<<"run: "<<QThread::currentThread();
    initCommDev();
    m_runTimer->start(0);
}

void ModbusService::slot_readData()
{
    struct MbusMsgPar msgPar;
    struct ComMsg comMsg;
    int ret;
    //qDebug()<<"slot_readData";
    m_mutexThreadRW.lock();
    do
    {
        //ret =m_modbus->read( (char *)&msgPar, sizeof( struct MbusMsgPar ) ); //接受来自Driver的消息
        ret = m_rwSocket->readDev((char *)&msgPar);
        switch (msgPar.err)
        {
        case COM_OPEN_ERR:
            m_devUseCnt = 0;
            for ( int i = 0; i < m_connectList.count(); i++ )
            {
                struct ModbusConnect* modbusConnect = m_connectList[i];
                Q_ASSERT( modbusConnect );
                modbusConnect->waitWrtieVarsList.clear();
                modbusConnect->requestVarsList.clear();
                foreach( HmiVar* tempHmiVar, modbusConnect->actVarsList )
                {
                    comMsg.type = 0; //0是read;
                    comMsg.err = VAR_DISCONNECT;
                    tempHmiVar->err = VAR_COMM_TIMEOUT;
                    tempHmiVar->lastUpdateTime = -1;
                    comMsg.varId = ( tempHmiVar->varId & 0x0000ffff );
                    m_mutexGetMsg.lock();
                    m_comMsgTmp.insert( comMsg.varId, comMsg );
                    m_mutexGetMsg.unlock();
                }
            }
            m_rwSocket->clearAllMsgCache();
            while ( ret == sizeof( struct MbusMsgPar ) )
            {
                ret = m_rwSocket->readDev((char *)&msgPar);
            }
            break;
        default:
            processRspData( msgPar, comMsg );
            break;
        }
    }
    while ( ret == sizeof( struct MbusMsgPar ) );
    m_mutexThreadRW.unlock();
}

void ModbusService::slot_runTimerTimeout()
{
    m_runTimer->stop();

    int connectCount = m_connectList.count();
    m_currTime = getCurrTime();

    m_mutexThreadRW.lock();
    for ( int i = 0; i < connectCount; i++ )
    {                              //处理写请求
        struct ModbusConnect* modbusConnect = m_connectList[i];
        updateWriteReq( modbusConnect );
        trySndWriteReq( modbusConnect );
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

    for ( int i = 0; i < connectCount; i++ )
    {
        struct ModbusConnect* modbusConnect = m_connectList[i];
        updateOnceRealReadReq( modbusConnect );  //先重组一次更新变量
        updateReadReq( modbusConnect ); //无论如何都要重组请求变量
        if ( !( hasProcessingReq( modbusConnect ) ) )
        {
            combineReadReq( modbusConnect );
        }
        trySndReadReq( modbusConnect );
    }
    m_mutexThreadRW.unlock();
    m_runTimer->start(20);


}
