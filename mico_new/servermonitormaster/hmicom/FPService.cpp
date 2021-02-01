
/**********************************************************************
Copyright (c) 2008-2010 by SHENZHEN CO-TRUST AUTOMATION TECH.
** 文 件 名:  cnetService.cpp
** 说    明: LS MasterK fp协议多协议平台实现文件，完成后台LS MasterK fp协议数据的处理以及链接，变量，读写状态的管理
** 版    本:  【可选】
** 创建日期:  2012.07.12
** 创 建 人:  陶健军
** 修改信息： 【可选】
**         修改人    修改日期       修改内容**
*********************************************************************/
#include "FPService.h"
#include "freePortDriver/freePort_driver.h"

#define MAX_CNET_PACKET_LEN  32  //最大一包字数，因为驱动组包大小只有255字节，超过57个字会导致 写常数组的时候出错

/******************************************************************************
** 函  数  名 ： fpService
** 说      明 :  构造函数
** 输      入 ： 链接参数
** 返      回 ： NONE
******************************************************************************/
fpService::fpService( struct fpConnectCfg& connectCfg )
{
    struct fpConnect* fpConn = new fpConnect;
    fpConn->cfg = connectCfg;
    fpConn->writingCnt = 0;
    fpConn->connStatus = CONNECT_NO_OPERATION;
    fpConn->connectTestVar.lastUpdateTime = -1;

    m_connectList.append( fpConn );
    m_currTime = getCurrTime();
    m_runTimer = NULL;
    m_rwSocket = NULL;
}

fpService::~fpService()
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
** 说      明 :  判断是否存在CNET类型的链接
** 输      入 ： 链接类型
        串口端口号
** 返      回 ： 存在与否标志
******************************************************************************/
bool fpService::meetServiceCfg( int serviceType, int COM_interface )
{
    if ( serviceType == SERVICE_TYPE_FP )
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
int fpService::getConnLinkStatus( int connectId )
{
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct fpConnect* fpConn = m_connectList[i];
        if ( connectId == fpConn->cfg.id )
        {
            return fpConn->connStatus;
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
int fpService::getComMsg( QList<struct ComMsg> &msgList )
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
int fpService::splitLongVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    return 0;
}

/******************************************************************************
** 函  数  名 ： onceRealUpdateVar
** 说      明 :  更新只读一次的变量
** 输      入 ： PLC变量结构体
** 返      回 ： 操作状态
******************************************************************************/
int fpService::onceRealUpdateVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    return 0;
}

/******************************************************************************
** 函  数  名 ： startUpdateVar
** 说      明 :  开始循环读取PLC变量
** 输      入 ： PLC变量结构体
** 返      回 ： 操作状态
******************************************************************************/
int fpService::startUpdateVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    m_mutexUpdateVar.lock();
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct fpConnect* fpConn = m_connectList[i];
        if ( pHmiVar->connectId == fpConn->cfg.id )
        {
            pHmiVar->lastUpdateTime = -1;
            if ( pHmiVar->splitVars.count() > 0 )
            {
                for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
                {
                    struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                    pSplitHhiVar->lastUpdateTime = -1;
                    pSplitHhiVar->overFlow = false;
                    if ( !fpConn->actVarsList.contains( pSplitHhiVar ) )
                    {
                        fpConn->actVarsList.append( pSplitHhiVar );
                    }
                }
            }
            else
            {
                pHmiVar->lastUpdateTime = -1;
                pHmiVar->overFlow = false;
                if ( pHmiVar->addOffset >= 0 )
                {
                    if ( !fpConn->actVarsList.contains( pHmiVar ) )
                    {
                        fpConn->actVarsList.append( pHmiVar );
                    }
                }
                else
                {
                    pHmiVar->err = VAR_PARAM_ERROR;
                }
            }

            m_mutexUpdateVar.unlock();
            //qDebug()<<"Connect: "<<fpConn->cfg.id<<"Active Num: "<<fpConn->actVarsList.count();
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
int fpService::stopUpdateVar( struct HmiVar* pHmiVar )
{
    Q_ASSERT( pHmiVar );
    m_mutexUpdateVar.lock();
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct fpConnect* fpConn = m_connectList[i];
        if ( pHmiVar->connectId == fpConn->cfg.id )
        {
            if ( pHmiVar->splitVars.count() > 0 )
            {
                for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
                {
                    struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                    fpConn->actVarsList.removeAll( pSplitHhiVar );
                }
            }
            else
            {
                fpConn->actVarsList.removeAll( pHmiVar );
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
int fpService::readVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
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
int fpService::readDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    Q_ASSERT( pHmiVar && pBuf );
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
int fpService::writeVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{
    int ret;
    Q_ASSERT( pHmiVar && pBuf );
    m_mutexRw.lock();


    if ( pHmiVar->wData.size() < bufLen )
    {
        pHmiVar->wData.resize( bufLen );
    }
    memcpy( pHmiVar->wData.data(), pBuf, bufLen );
    pHmiVar->lastUpdateTime = -1;

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
int fpService::writeDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen )
{


    Q_ASSERT( pHmiVar && pBuf );
    QString str;
    str = QString::fromLocal8Bit((char*)pBuf);

    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct fpConnect* fpConn = m_connectList[i];
        if ( pHmiVar->connectId == fpConn->cfg.id )
        {
            m_mutexRw.lock();
            m_writeList.append(str);
            m_mutexRw.unlock();
            return VAR_NO_ERR;
        }
    }
    return -1;

    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct fpConnect* fpConnect = m_connectList[i];
        if ( pHmiVar->connectId == fpConnect->cfg.id )
        {
            int ret;
            if ( fpConnect->connStatus == CONNECT_NO_LINK )//if this var has read and write err, do not execute write, until it is read or wriet correct.
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
                        fpConnect->waitWrtieVarsList.append( pSplitHhiVar );
                        pBuf += len;
                        bufLen -= len;
                    }
                }
                else
                {
                    //ret = writeVar( pHmiVar, pBuf, bufLen );
                    //qDebug()<<"write "<<bufLen;
                    //fpConnect->waitWrtieVarsList.append( pHmiVar );
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
** 输      入 ： struct fpConnectCfg
** 返      回 ： 操作状态
******************************************************************************/
int fpService::addConnect( struct fpConnectCfg& connectCfg )
{                                     //要检查是否重复 add
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct fpConnect* fpConn = m_connectList[i];
        Q_ASSERT( fpConn->cfg.COM_interface == connectCfg.COM_interface );//绝对要是同一个设备接口
        if ( connectCfg.id == fpConn->cfg.id )
        {
            Q_ASSERT( false );
            return 0;
        }
    }
    struct fpConnect* fpConn = new fpConnect;
    fpConn->cfg = connectCfg;
    fpConn->writingCnt = 0;
    fpConn->connStatus = CONNECT_NO_OPERATION;
    fpConn->connectTestVar.lastUpdateTime = -1;
    m_connectList.append( fpConn );
    return 0;
}

/******************************************************************************
** 函  数  名 ： sndWriteVarMsgToDriver
** 说      明 : 把写缓冲区数据打包到 struct fpMsgPar中，然后写入到驱动
** 输      入 ： struct fpConnect*
        struct HmiVar*
** 返      回 ： 操作状态
******************************************************************************/
int fpService::sndWriteVarMsgToDriver( struct fpConnect* fpConnect, struct HmiVar* pHmiVar )
{
    int sendedLen = 0;
    Q_ASSERT( fpConnect && pHmiVar );
//#ifdef Q_WS_QWS
    FpMsgPar msg;
    char *buf = (char *)msg.data;

    m_mutexRw.lock();
    sendedLen = pHmiVar->wData.size();
    if( sendedLen > 255 )
    {
        sendedLen = 255;
    }
    memcpy( buf, pHmiVar->wData.data(), sendedLen );

    if ( sendedLen > 0 )
    {
        m_mutexRw.unlock();
        //qDebug()<<"write driver";
        int len = -1;
        while( len < 0 )
        {
            //len = write( m_fd, buf , sendedLen );
            msg.count = sendedLen;
            len = m_rwSocket->writeDev((char *)&msg, sizeof(struct FpMsgPar));
        }
        //qDebug()<<"write driver len"<<len;
        Q_ASSERT( len >= 0 );
        sendedLen = len;
    }
    else
        m_mutexRw.unlock();
//#endif
    return sendedLen;
}

/******************************************************************************
** 函  数  名 ： sndReadVarMsgToDriver
** 说      明 : 把读缓冲区数据打包到 struct fpMsgPar中，然后写入到驱动
** 输      入 ： struct fpConnect*
        struct HmiVar*
** 返      回 ： 操作状态
******************************************************************************/
int fpService::sndReadVarMsgToDriver( struct fpConnect* fpConnect, struct HmiVar* pHmiVar )
{//printf("sndReadVarMsgToDriver\n");
    Q_ASSERT( fpConnect && pHmiVar );
    return 0;
}

/******************************************************************************
** 函  数  名 ： isWaitingReadRsp
** 说      明 : 是否有读数据请求已发往驱动
** 输      入 ： struct fpConnect*
        struct HmiVar*
** 返      回 ： 操作状态
******************************************************************************/
bool isWaitingReadRsp( struct fpConnect* fpConnect, struct HmiVar* pHmiVar )
{
    Q_ASSERT( fpConnect && pHmiVar );
    if ( fpConnect->waitUpdateVarsList.contains( pHmiVar ) )
    {
        return true;
    }
    else if ( fpConnect->combinVarsList.contains( pHmiVar ) )
    {
        return true;
    }
    else if ( fpConnect->requestVarsList.contains( pHmiVar ) )
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
** 输      入 ： struct fpConnect*
** 返      回 ： 操作状态
******************************************************************************/
bool fpService::trySndWriteReq( struct fpConnect* fpConnect ) //不处理超长变量
{
    if ( m_devUseCnt < MAXDEVCNT )                          //没有正在读写, snd read, 一次只能发送一包请求
    {
        if ( fpConnect->writingCnt == 0 && fpConnect->requestVarsList.count() == 0 ) //这个连接没有读写
        {
            m_mutexWReq.lock();
            if ( fpConnect->waitWrtieVarsList.count() > 0 )
            {
                struct HmiVar* pHmiVar = fpConnect->waitWrtieVarsList[0];
                                                //直接写,snd write cmd to driver
                if ( sndWriteVarMsgToDriver( fpConnect, pHmiVar ) > 0 )
                {
                    //fpConnect->writingCnt = 1;    //进入写状态
                    fpConnect->waitWrtieVarsList.takeAt( 0 );
                    m_mutexWReq.unlock();
                    return true;
                }
                else
                {
                    fpConnect->waitWrtieVarsList.takeAt( 0 );//出列,丢掉写请求
                    pHmiVar->err = VAR_PARAM_ERROR;
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
** 输      入 ： struct fpConnect*
** 返      回 ： 操作状态
******************************************************************************/
bool fpService::trySndReadReq( struct fpConnect* fpConn ) //不处理超长变量
{
    return false;
}

/******************************************************************************
** 函  数  名 ： updateWriteReq
** 说      明 : 更新写请求
** 输      入 ： struct fpConnect*
** 返      回 ： 操作状态
******************************************************************************/
bool fpService::updateWriteReq( struct fpConnect* fpConnect )
{
    bool newUpdateVarFlag = false;
    QList<struct HmiVar*> tmpList;
    m_mutexWReq.lock();

    for ( int i = 0; i < fpConnect->wrtieVarsList.count(); i++ )
    {
        struct HmiVar* pHmiVar = fpConnect->wrtieVarsList[i];

        if ( !fpConnect->waitWrtieVarsList.contains( pHmiVar ) )
        {
            fpConnect->waitWrtieVarsList.append( pHmiVar );
            newUpdateVarFlag = true;
        }
        else
        {
            tmpList.append(pHmiVar);
        }
    }
    fpConnect->wrtieVarsList.clear() ;
    fpConnect->wrtieVarsList += tmpList;
    m_mutexWReq.unlock();
    return newUpdateVarFlag;
}

/******************************************************************************
** 函  数  名 ： updateReadReq
** 说      明 : 更新读请求
** 输      入 ： struct fpConnect*
** 返      回 ： 操作状态
******************************************************************************/
bool fpService::updateReadReq( struct fpConnect* fpConnect )
{

    return 0;
}

/******************************************************************************
** 函  数  名 ： updateOnceRealReadReq
** 说      明 : 更新只读一次的读请求
** 输      入 ： struct fpConnect*
** 返      回 ： 操作状态
******************************************************************************/
bool fpService::updateOnceRealReadReq( struct fpConnect* fpConnect )
{
    return 0;
}

/******************************************************************************
** 函  数  名 ： combineReadReq
** 说      明 : 把相同区域的 可组合的变量组合再一起，可以提高通讯速度
** 输      入 ： struct fpConnect*
** 返      回 ： 操作状态
******************************************************************************/
void fpService::combineReadReq( struct fpConnect* fpConnect )
{//printf("combineReadReq\n");
    return;
}

/******************************************************************************
** 函  数  名 ： hasProcessingReq
** 说      明 : 是否正有数据在进行读或者写
** 输      入 ： struct fpConnect*
** 返      回 ： 操作状态
******************************************************************************/
bool fpService::hasProcessingReq( struct fpConnect* fpConnect )
{
    return ( fpConnect->writingCnt > 0 || fpConnect->requestVarsList.count() > 0 );
}

/******************************************************************************
** 函  数  名 ： initCommDev
** 说      明 : 初始化设备
** 输      入 ： void
** 返      回 ： 操作状态
******************************************************************************/
bool fpService::initCommDev()
{
    m_devUseCnt = 0;
    struct ComMsg comMsg;

    if (m_runTimer == NULL)
    {
        m_runTimer = new QTimer;
        connect( m_runTimer, SIGNAL( timeout() ), this,
            SLOT( slot_runTimerTimeout() )/*,Qt::QueuedConnection*/ );
    }

    if ( m_connectList.count() > 0 )
    {
        struct fpConnect* conn = m_connectList[0];

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

            m_rwSocket = new RWSocket(FREE_PORT, &uartCfg, conn->cfg.id);
            connect(m_rwSocket, SIGNAL(readyRead()),this,SLOT(slot_readData()));
            m_rwSocket->openDev();
        }
    }
    for ( int i = 0; i < m_connectList.count(); i++ )
    {
        struct fpConnect* connect = m_connectList[i];
        connect->connStatus = CONNECT_LINKED_OK;
        comMsg.varId = 65535;
        comMsg.type = 0;
        comMsg.err = VAR_NO_ERR;
        m_comMsgTmp.insert( comMsg.varId, comMsg );
    }

    return true;

}

/******************************************************************************
** 函  数  名 ： processRspData
** 说      明 : 读取驱动，成功读取后，根据都会来MSG的内容进行相应的处理
** 输      入 ： QHash<int, struct ComMsg> &
** 返      回 ： 操作状态
******************************************************************************/
bool fpService::processRspData( struct FpMsgPar msg, struct ComMsg comMsgTmp )
{
    return 0;
}

void fpService::sndWriteVarMsgToDriver(void)
{
    int sendedLen = 0;
    QString sndStr;
    FpMsgPar msg;
    char *buf = (char *)msg.data;
    memset(buf, 0, 256);

    m_mutexRw.lock();
    sndStr = m_writeList[0];

    sendedLen = sndStr.toLocal8Bit().size();

    //打印的时候要把UTF8 转化到GBK  by Emoson
    QByteArray ba = sndStr.toLocal8Bit();
    QTextCodec *xcode = QTextCodec::codecForLocale();

    QTextCodec *gbk = QTextCodec::codecForName("GBK");

    QString strUnicode= xcode->toUnicode(ba);
    QByteArray gb_bytes= gbk->fromUnicode(strUnicode);
    sendedLen = gb_bytes.size();

    if ( sendedLen > 0 && sendedLen < 255 )
    {
        memcpy(buf, gb_bytes.data(), sendedLen);
        //memcpy(buf, sndStr.toLocal8Bit().data(), sendedLen);
        m_writeList.takeAt(0);
        m_mutexRw.unlock();
        //qDebug()<<"write driver";

        int len = -1;
        while( len < 0 )
        {
            //len = write( m_fd, buf , sendedLen );
            msg.count = sendedLen;
            len =  m_rwSocket->writeDev((char *)&msg, sizeof(struct FpMsgPar));
        }
        //qDebug()<<"write driver len"<<len;
    }
    else
    {
        m_writeList.takeAt(0);
        m_mutexRw.unlock();
    }
}
/******************************************************************************
** 函  数  名 ： run
** 说      明 : 线程固定函数，线程Start后进入run（）处理
** 输      入 ： void
** 返      回 ： 操作状态
******************************************************************************/
void fpService::slot_run()
{
    //QHash<int , struct ComMsg> comMsgTmp;
    initCommDev();
    m_runTimer->start(10);
}

void fpService::slot_runTimerTimeout()
{
    m_runTimer->stop();
    if (m_writeList.count() > 0)
    {
        /* code */
        sndWriteVarMsgToDriver();
    }
    m_runTimer->start(100);
}

void fpService::slot_readData()
{
    struct FpMsgPar msgPar;
    struct ComMsg comMsg;
    int ret;
    while (m_rwSocket->readDev((char *)&msgPar) > 0)
    {
        switch (msgPar.err)
        {
        case COM_OPEN_ERR:
            qDebug()<<"COM_OPEN_ERR";
            m_devUseCnt = 0;
            for ( int i = 0; i < m_connectList.count(); i++ )
            {
                struct fpConnect* fpConnect = m_connectList[i];
                Q_ASSERT( fpConnect );
                fpConnect->waitWrtieVarsList.clear();
                fpConnect->requestVarsList.clear();
                foreach( HmiVar* tempHmiVar, fpConnect->actVarsList )
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
