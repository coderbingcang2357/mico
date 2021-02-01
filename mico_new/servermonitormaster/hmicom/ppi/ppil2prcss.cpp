/******************************************************************************
**
** Copyright (c) 2008-20010 by SHENZHEN CO-TRUST AUTOMATION TECH.
**
** 文 件 名:   ppil2prcss.cpp
** 说    明:   PPI通信协议网络层实现文件
** 版    本:   V1.0
** 创建日期:   2009.3.23
** 创 建 人:   LIUSHENGHONG
** 修改信息： 【可选】
**         修改人    修改日期       修改内容
**		   陶健军	2012.09.19		修改部分代码以适应HMI在线模拟，同时采用网络上的QT串口操作类，因为用ppil1prcss类长时间通信的时候发现不能接收串口数据。
******************************************************************************/
#include <QSettings>
#include <QMessageBox>
#ifndef  HMI_ANDROID_IOS
#include <windows.h>
#else
#include <QThread>
#endif
//#include <QThread>
//#include "ppil1prcss.h"
#include "ppil2prcss.h"
//#include "ppi_l7wr.h"
//#include "serverdlg.h"

//PPI协议相关的参数，轮询和重试
static const int MAXPOLLTIMES_PPI = 300;     //最多轮询次数
static const int MAXRETRYTIMES_PPI = 3;      //最多重试次数
static const int RETRYINTERV_PPI = 1000;     //超时重试时间间隔,最小90bit
static const int SENDINTERV_PPI = 25;        //两次发送命令之间的间隔,39bit
static const int POLLINTERV_PPI = 0;        //轮询时间间隔,(39bit,10s)


//PPI协议相关的参数，超时时间
static const int COMMTIMEOUT_PPI = 3000 * 1000;  //(1000*30/baud); 通信超时时间，发送成功后收到第一个字符的最长时间，PPI协议为(22bit,30bit)
static const int RCVTIMEOUT_PPI = 2000;      //接收超时时间，收到第一个字符到接收完成的时间
static const int FDLDELAY_PPI = 65;          //直接使用串口或wUSB转485/232接口时搜索命令超时，与波特率无关，windows的延时，此处用的经验值
static const int FDLDELAY_GPRS = 1000;       //使用GPRS转485接口时搜索命令延时，与波特率无关，windows的延时，此处用的经验值
static const int RDY_TICKS_PPI  = 33 * 1000;		//33bit 发送前延时
//struCommPar g_comm_par;

PPIL2Prcss::PPIL2Prcss()
{
    m_isCommOpen = false;
    m_socketLockFlag = false;
    m_pduRef = 1;
    m_timerSlot = new QTimer( this );
    connect( m_timerSlot, SIGNAL( timeout() ), this, SLOT( timerSlotTimeout() ) );
    m_timerRcv = new QTimer( this );
    connect( m_timerRcv, SIGNAL( timeout() ), this, SLOT( timerRcvTimeout() ) );
    m_timerPoll = new QTimer( this );
    connect( m_timerPoll, SIGNAL( timeout() ), this, SLOT( timerPollTimeout() ) );
    m_timerRdy = new QTimer(this);
    connect( m_timerRdy, SIGNAL( timeout() ), this, SLOT( timerRdyTimeout() ) );

    connect( this, SIGNAL( startTimerRdy( ) ),this, SLOT( timerRdyStart() )) ;

    m_qtcom = new QextSerialPort( QString( "COM1" ), QextSerialPort::EventDriven);
    connect( m_qtcom, SIGNAL( readyRead() ), this, SLOT( slot_onReceiveFrame() ) );
    //qDebug()<<m_qtcom->size();
    //connect( m_qtcom, SIGNAL( readChannelFinished () ), this, SLOT( slot_onReceiveFrame() ) );
}

PPIL2Prcss::~PPIL2Prcss()
{
    if ( m_timerSlot != NULL )
    {
        m_timerSlot->stop();
        delete m_timerSlot;
        m_timerSlot = NULL;
    }

    if ( m_timerRcv != NULL )
    {
        m_timerRcv->stop();
        delete m_timerRcv;
        m_timerRcv = NULL;
    }

    if ( m_timerPoll != NULL )
    {
        m_timerPoll->stop();
        delete m_timerPoll;
        m_timerPoll = NULL;
    }

    if ( m_timerRdy !=NULL )
    {
        m_timerRdy->stop();
        delete m_timerRdy;
        m_timerRdy = NULL;
    }

    if ( m_qtcom != NULL )
    {
        if ( m_qtcom->isOpen() )
        {
            m_qtcom->close();
        }
        delete m_qtcom;
        m_qtcom = NULL;
    }
}

void PPIL2Prcss::comQueueEnqueue( const QByteArray &array )
{
    m_comQueue.clear();
    m_comQueue.enqueue( array );
}

int PPIL2Prcss::getComQueueSize()
{
    return m_comQueue.size();
}

bool PPIL2Prcss::isComQueueEmpty()
{
    return m_comQueue.isEmpty();
}

bool PPIL2Prcss::openCom( const QString &portNumber, long lBaudRate, int dataBit,
                         int stopBit, int parity, unsigned char da, unsigned char sa )
{
    m_Da = da;
    m_Sa = sa;
    m_isActive = false;
    m_lastFC = FC_6C;

    if ( !m_isCommOpen )                   //如果端口没打开就打开端口
    {
        {
            m_qtcom->setPortName(portNumber);
            m_qtcom->open(QIODevice::ReadWrite);
            if ( lBaudRate == 9600 )
            {
                m_qtcom->setBaudRate(BAUD9600);
            }
            else if ( lBaudRate == 19200 )
            {
                m_qtcom->setBaudRate(BAUD19200);
            }
            else
            {
                m_qtcom->setBaudRate(BAUD19200);
            }
            m_qtcom->setDataBits(DATA_8);
            m_qtcom->setStopBits(STOP_1);
            m_qtcom->setParity(PAR_EVEN);
            m_qtcom->setFlowControl( FLOW_OFF );
            m_qtcom->setTimeout( 1000 );
        }
        if ( !m_qtcom->isOpen() ) //如果打开端口错误则返回错误码
        {
            returnToL7( 0, COM_OPENERR );
            return false;
        }
        else
        {
            m_isCommOpen = true;

            if ( lBaudRate == 187500 )
            {
                m_timeoutComm = COMMTIMEOUT_PPI / 19200;
                m_timeoutRdyTicks = RDY_TICKS_PPI / 19200;
            }
            else
            {
                m_timeoutComm = COMMTIMEOUT_PPI / lBaudRate;
                m_timeoutRdyTicks = RDY_TICKS_PPI / lBaudRate;
            }
            returnToL7( 0, COM_OPENSUCCESS );
            return true;
        }
    }
    return false;
}

//处理需要与PLC通信的命令   //LIUSHENGHONG_2011-08-26
void PPIL2Prcss::prcCmdWithPlc()
{

    QByteArray msgBlock;    //L7的message，buffOut去掉前面4个字节的socket指针

    msgBlock = m_comQueue.dequeue();  //从缓冲队列中取出一个指令
    m_sendBlock.clear();
    buildSd2Msg( msgBlock, m_Sa, m_Da, m_lastFC, m_sendBlock );
    m_lastFC = setNextFc( m_lastFC );
    m_retryTimes = 1;            //重试次数为1(总共最多重复发送3次)
    m_pollTimes = 0;             //轮询次数为0
    m_recvBlock.clear();         //清空接收缓冲区
    //m_qtcom->write(m_sendBlock);
    emit startTimerRdy();
}

void PPIL2Prcss::timerRdyStart()
{
    m_timerRdy->start(m_timeoutRdyTicks);
}

void PPIL2Prcss::timerSlotTimeout()
{
    m_timerSlot->stop();
    qDebug()<<"slot timeout";
    qDebug()<<m_sendBlock.toHex();
    prcCommFail();
}

void PPIL2Prcss::timerRcvTimeout()
{
    m_timerRcv->stop();
    qDebug()<<"rcv timeout";
    prcCommFail();
}

void PPIL2Prcss::timerPollTimeout()
{
    m_timerPoll->stop();
    prcCommPoll();
}

bool PPIL2Prcss::getL7Msg( struct  L7Msg &msg )
{
    if ( m_L7MsgQueue.size() > 0 )
    {
        msg = m_L7MsgQueue.takeFirst();
        return true;
    }
    return false;
}

//void PPIL2Prcss::recvData( const QByteArray &dat )
//{
    /*if ( m_timerSlot->isActive() )
    {
        m_timerSlot->stop();
        m_timerRcv->start( RCVTIMEOUT_PPI );
    }

    m_recvBlock += dat;
    if ( (( unsigned char )m_recvBlock[0] == L1_SC) && m_recvBlock.size() > 1 )
    {
        m_recvBlock = m_recvBlock.right( m_recvBlock.size() - 1 );
    }
    prcRecvData();	*/
//}
void PPIL2Prcss::slot_onReceiveFrame()
{
    if ( m_timerSlot->isActive() )
    {
        m_timerSlot->stop();
        m_timerRcv->start( RCVTIMEOUT_PPI );
    }
    QByteArray temp = m_qtcom->readAll();
    if ( temp.size() > 0 )
    {
        m_recvBlock += temp;
        //qDebug()<<"rcv "<<m_recvBlock.toHex();
        prcRecvData();
    }
}

void PPIL2Prcss::prcRecvData()
{
    char rslt;
    switch (( unsigned char )m_recvBlock[0] )
    {
    case L1_SD1:
        if ( m_recvBlock.size() == 6 )
        {
            m_timerRcv->stop();

            if( ( unsigned char )m_recvBlock[1] == m_Sa && ( unsigned char )m_recvBlock[2] == m_Da )
            {
                if ( ( unsigned char )m_recvBlock[3] == 0x00 )
                {
                    if( m_timerActive->isActive() )
                    {
                        m_timerActive->stop();
                    }

                    m_isActive = true;
                    returnToL7(0,PPI_CONNECT_OK);
                    m_recvBlock.clear();
                    if ( getComQueueSize() > 0 )
                    {
                        prcCmdWithPlc();
                    }
                }
                else //收到FC为 02  03的时候表示PLC没有可用资源
                {
                    returnToL7(0,PPI_CONNECT_FAIL);
                    //Beep(4000,5000);
#ifdef HMI_ANDROID_IOS
                    QThread::msleep(3000);
#else
                    Sleep(3000);
#endif
                }
            }
        }
        else if ( m_recvBlock.size() > 6 )
        {
            prcCommFail();
        }
        break;

    case L1_SD2:
        rslt = checkSd2Msg( m_recvBlock );
        if ( rslt == RCVFAIL )
        {
            prcCommFail();
        }
        else if ( rslt == RCVSUCCESS )
        {
            prcCommSuccess();
        }
        break;

    case L1_SC:               //0xE5//短应答应该给予轮询
        m_timerRcv->stop();
        //m_timerPoll->start( POLLINTERV_PPI );
        m_recvBlock.clear();
        prcCommPoll();
        break;

    default:                  //属于出错
        prcCommFail();
        break;
    }
}

void PPIL2Prcss::prcCommSuccess()
{
    m_timerRcv->stop();
    QByteArray pMsg;
    unsigned char err;
    getL7MsgFromRcvPdu( m_recvBlock, pMsg );
    err = PPI_NOERR;
    returnToL7( pMsg, err );
}

void PPIL2Prcss::prcCommFail()
{
    m_timerRcv->stop();
        m_retryTimes++;
        if ( m_retryTimes <= MAXRETRYTIMES_PPI )                  //重试次数还没到3次，重发
        {
#ifdef HMI_ANDROID_IOS
                    QThread::msleep(RETRYINTERV_PPI);
#else
                    Sleep(RETRYINTERV_PPI);
#endif
            m_pollTimes = 0;
            m_lastFC = setNextFc( m_sendBlock[6] );
            m_recvBlock.clear();

            if ( sendToL1() < 0 )  //发送失败
            {
                prcCommFail();
            }
            else
            {
                m_timerSlot->start( m_timeoutComm /*+ g_comm_par.controlParPPI.parL1.timeout */);
            }

        }
        else
        {
            unsigned char err = PPI_ERR_TIMEROUT;                 //通信超时
            returnToL7( 0 , err);
        }
}

void PPIL2Prcss::prcCommPoll()
{
    m_pollTimes++;

    if ( m_pollTimes > MAXPOLLTIMES_PPI )
    {
        prcCommFail();
    }
    else
    {
        m_sendBlock.clear();
        buildSd1Msg( m_Sa, m_Da, m_lastFC, m_sendBlock );
        m_lastFC = setNextFc(m_lastFC );
        if ( sendToL1() < 0 )  //发送失败
        {
            prcCommFail();
        }
        else
        {
            m_timerSlot->start( m_timeoutComm );
        }
    }
}

qint64 PPIL2Prcss::sendToL1()
{
    //qDebug()<<"snd "<<m_sendBlock.toHex();
    return m_qtcom->write(m_sendBlock);
     //m_sendBlock.size();
}

void PPIL2Prcss::timerRdyTimeout()
{
    m_timerRdy->stop();
    if ( sendToL1() < 0 )  //发送失败
    {
        prcCommFail();
    }
    else
    {
        m_timerSlot->start( m_timeoutComm /*+ g_comm_par.controlParPPI.parL1.timeout*/ );
    }
}

void PPIL2Prcss::returnToL7( const QByteArray  &rtnBlock, unsigned char err )
{
    L7Msg msg;
    msg.data = rtnBlock;
    msg.err = err;
    m_L7MsgQueue.enqueue(msg);
    if ( err == PPI_ERR_TIMEROUT )
    {
        m_comQueue.clear();
    }
    emit oneCmdEnd();
}



/******************************************************************************
** 函 数 名： setNextFc
** 说    明:  生成下一次PDU要使用的FC
** 输    入： 当前使用的FC
** 输    出： NA
** 返    回： 下一次要使用的FC
** 异    常： NA
******************************************************************************/
unsigned char PPIL2Prcss::setNextFc( unsigned char currentFC )
{
    if ( currentFC == FC_6C )
    {
        return FC_5C;
    }
    else if ( currentFC == FC_5C )
    {
        return FC_7C;
    }
    else
    {
        return FC_5C;
    }

}

/******************************************************************************
** 函数名: getSd2Fcs
** 说  明: 计算PPI 帧的校验码
** 输  入: pBuff : PPI帧；
** 输  出: NA
** 返  回: 校验和
** 异　常: NA(校验方法: 把帧内容按字节累加，进位丢掉，最后的余数就是校验和。）
******************************************************************************/
unsigned char PPIL2Prcss::getSd2Fcs( const QByteArray &buffIn )
{
    int i;
    unsigned char fcs = 0;
    int buffSize = buffIn.size();

    for ( i = 0; i < buffSize; i++ )
    {
        fcs += buffIn[i];
    }
    return fcs;
}

/******************************************************************************
** 函 数 名： buildSd1Msg
** 说    明:  生成SD1帧
** 输    入： sa：本地地址；da：目标地址；fc：帧控制字节
** 输    出： pBuff：生成L2的SD1帧；
** 返    回： NA
** 异    常： NA
******************************************************************************/
void PPIL2Prcss::buildSd1Msg( const unsigned char sa, const unsigned char da, const unsigned char fc, QByteArray &buffOut )
{
    buffOut.resize( 6 );
    buffOut[0] = L1_SD1;
    buffOut[1] = da;
    buffOut[2] = sa;
    buffOut[3] = fc;
    buffOut[4] = da + sa + fc;
    buffOut[5] = L1_ED;
}

/******************************************************************************
** 函 数 名： buildSd2Msg
** 说    明:  生成SD2帧
** 输    入： sa：本地地址；da：目标地址；fc：帧控制字节；pL7Msg：来自L7的帧
** 输    出： pBuff：生成L2的SD1帧；
** 返    回： NA
** 异    常： NA
******************************************************************************/
void PPIL2Prcss::buildSd2Msg( const QByteArray &buffIn, const unsigned char sa, const unsigned char da,
                              const unsigned char fc, QByteArray &buffOut )
{
    int l7MsgLen = buffIn.size();
    buffOut[0] = L1_SD2;
    buffOut[1] = l7MsgLen + 3;
    buffOut[2] = buffOut[1];
    buffOut[3] = L1_SD2;
    buffOut[4] = da;
    buffOut[5] = sa;
    buffOut[6] = fc;
    buffOut += buffIn;
    buffOut[l7MsgLen + 7] = getSd2Fcs( buffOut.mid( 4, buffOut[1] ) );
    buffOut[l7MsgLen + 8] = L1_ED;
}

/******************************************************************************
** 函 数 名： checkSd2Msg
** 说    明:  检查从PLC接收的消息的合法性
** 输    入： pdu : 接收的PPI数据；sa:本地地址；da：目标地址
** 输    出： NA
** 返    回： 返回判断结果的错误码
** 异    常： NA
******************************************************************************/
char PPIL2Prcss::checkSd2Msg( const QByteArray &buffIn )
{
    int actualLen = buffIn.size();

    if ( actualLen < 2 )
    {
        return RCVPENDING;
    }

    int correctLen = ( unsigned char )buffIn[1] + 6;
    if ( actualLen > correctLen )
    {
        return RCVFAIL;
    }
    else if ( actualLen < correctLen )
    {
        return RCVPENDING;
    }
    else           //if (actualLen == correctLen)
    {
        unsigned char fcs = getSd2Fcs( buffIn.mid( 4, ( unsigned char )buffIn[1] ));
        if ((( unsigned char )buffIn[actualLen - 1] == L1_ED ) //没有判断返回的pduref是否跟发送的一致？？？
                        && (( unsigned char )buffIn[4] == m_Sa )
                        && (( unsigned char )buffIn[6] == FC_RSP )
                        && (( unsigned char )buffIn[correctLen - 2] == getSd2Fcs( buffIn.mid( 4, ( unsigned char )buffIn[1] ) ) ) )
        {
            return RCVSUCCESS;
        }
        else
        {
            return  RCVFAIL;
        }
    }
}

/******************************************************************************
** 函 数 名： getL7MsgFromRcvPdu
** 说    明:  分解接收数据帧的L2与L7部分
** 输    入： pdu : 接收的PPI数据；
** 输    出： pMsg:L7部分的消息包
** 返    回： NA
** 异    常： NA
******************************************************************************/
void PPIL2Prcss::getL7MsgFromRcvPdu( const QByteArray &buffIn, QByteArray &buffOut )
{
    if (( unsigned char )buffIn[0] == L1_SD1 )
    {
        buffOut.resize( 1 );
        buffOut[0] = buffIn[3];
    }
    else
    {
        buffOut.resize( buffIn.size() - 9 );
        buffOut = buffIn.mid( 7, buffIn.size() - 9 );
    }
}

/******************************************************************************
** 函 数 名： buildSndPduByL2
** 说    明:  将L7的数据打包生成要发送的PDU
** 输    入： pMsg:L7部分的消息包
** 输    出： pdu : 接收的PPI数据
** 返    回： NA
** 异    常： NA
******************************************************************************/
bool PPIL2Prcss::buildSndPduByL2( const QByteArray &buffIn, QByteArray &buffOut )
{
    /*int totalSize = ( unsigned char )buffIn[0];   //[ToltalLen]  [CmdID]  [MsgLen]  [AddrLen]  {Msg}  {Addr}
    int msgSize = ( unsigned char )buffIn[2];	    //MSG长度
    int addrSize = ( unsigned char )buffIn[3];	  //ADDR长度
    if ( totalSize != buffIn.size() || totalSize != ( msgSize + addrSize + 4 ) )
        return false;

    switch ( g_comm_par.protoId )
    {
    case PROTC_PPI:                           //PPI协议
        if ( addrSize != 0 && addrSize != 1 )
            return false;

            if ( addrSize == 0 )	                               //默认目标地址
                g_comm_par.controlParPPI.parL2.remoteAddr = 2;
            else                                                   //给定的目标地址,PPI协议为一个字节
                g_comm_par.controlParPPI.parL2.remoteAddr = ( unsigned char )buffIn[totalSize - 1];

            buildSd2Msg( buffIn.mid( 4, msgSize ), g_comm_par.controlParPPI.parL1.localA,
                         g_comm_par.controlParPPI.parL2.remoteAddr, g_comm_par.controlParPPI.parL2.pduFC, buffOut );

            if ( m_cmdId != CMDID_ESTABASSOCIA )
                m_pduRef++;

            g_comm_par.controlParPPI.parL2.pduFC = setNextFc( g_comm_par.controlParPPI.parL2.pduFC );
        //}
        return true;
        //break;

    default:
        return false;
        //break;
    }*/
    return false;
}


