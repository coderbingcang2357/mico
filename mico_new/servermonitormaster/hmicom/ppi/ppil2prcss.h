/******************************************************************************
**
** Copyright (c) 2008-20010 by SHENZHEN CO-TRUST AUTOMATION TECH.
**
** 文 件 名:   ppil2prcss.h
** 说    明:   PPI通信协议网络层头文件
** 版    本:   V1.0
** 创建日期:   2009.3.23
** 创 建 人:   LIUSHENGHONG
** 修改信息： 【可选】
**         修改人    修改日期       修改内容
**
******************************************************************************/

#ifndef PPI_L2_H
#define PPI_L2_H

#include <QByteArray>
#include <QList>
//#include <QtNetwork>
#include <QTimer>
#include <QEvent>
#include <QDebug>
#include <QQueue>
//#include "ppil1prcss.h"
#include "../qextserialport/qextserialport.h"
//#include "ppil7.h"
//#include "localsocket.h"
#include "qobjectdefs.h"

const unsigned char L1_SD2 =	0x68;
const unsigned char L1_SD1 =	0x10;
const unsigned char L1_SD4 =	0xDC;
const unsigned char L1_SC  =	0xE5;
const unsigned char L1_ED  =	0x16;

const unsigned char FC_6C  =	0x6C; //第一条消息应该用6C
const unsigned char FC_5C  =	0x5C;
const unsigned char FC_7C  =	0x7C;
const unsigned char FC_FDL_STATUS = 0x49;

const unsigned char FC_RSP =	0x08;

const char RCVFAIL    = 0;
const char RCVSUCCESS = 1;
const char RCVPENDING = 2;	          //SD2 帧的校验都以此三个值作为返回值

enum
{
    PPI_NOERR,
    PPI_ERR_TIMEROUT,
    COM_OPENERR,
    COM_OPENSUCCESS,
    PPI_BUILD_SD2_ERR,
    PPI_CONNECT_OK,
    PPI_CONNECT_FAIL
};
//通信协议参数
/*struct struCommPar
{
    int protoId;                      //协议ID
    struCommParPPI controlParPPI;     //PPI协议控制参数
    struCommParUDP controlParUDP;     //UDP协议控制参数
};*/

struct L7Msg
{
    QByteArray data;
    unsigned char err;
};

//extern struCommPar g_comm_par;        //全局变量
extern void prcSetInterface();        //全局函数 设置接口

////////////////////////////////////////////////////////////////////////////////////////
class PPIL2Prcss : public QObject
{
    Q_OBJECT

public:
    PPIL2Prcss();
    ~PPIL2Prcss();

    //void prcCmdWithoutPlc( LocalSocket *socket, const QByteArray &pduData );  //处理不需要与PLC通信的指令
    void prcCmdWithPlc();                                                   //处理需要与PLC通信的指令

    void comQueueEnqueue( const QByteArray &array );
    int getComQueueSize();
    bool isComQueueEmpty();
    bool getL7Msg( struct L7Msg &msg );
    bool openCom( const QString &portNumber, long lBaudRate,
                    int dataBit, int stopBit, int parity,
                    unsigned char da, unsigned char sa );
    //void clearQueueEnqueue();  //PPI协议改变为其它协议时先处理掉缓冲区剩余的指令

signals:
    //void noValidCmd( int );
    void oneCmdEnd();
    //void startTimerActiv();
    void startTimerRdy();
    //void stopTimerActive();

private slots:
    void timerSlotTimeout();
    void timerRcvTimeout();
    void timerPollTimeout();
    //void recvData( const QByteArray &dat );
    //void timerActiveTimeout();
    void timerRdyTimeout();
    void timerRdyStart();
    //void timerActiveStop();
    void slot_onReceiveFrame();


private:
    qint64 sendToL1();
    void returnToL7( const QByteArray  &rtnBlock, const unsigned char err);
    void prcRecvData();
    void prcCommFail();
    void prcCommSuccess();
    void prcCommPoll();
    unsigned char setNextFc( const unsigned char currentFC );
    unsigned char getSd2Fcs( const QByteArray &buffIn );
    void buildSd1Msg( const unsigned char sa, const unsigned char da, const unsigned char fc, QByteArray &buffOut );
    void buildSd2Msg( const QByteArray &buffIn, const unsigned char sa, const unsigned char da, const unsigned char fc, QByteArray &buffOut );
    char checkSd2Msg( const QByteArray &buffIn );
    void getL7MsgFromRcvPdu( const QByteArray &buffIn, QByteArray &buffOut );
    bool buildSndPduByL2( const QByteArray &buffIn, QByteArray &buffOut );
    void proActive();

//	LocalSocket *m_socket;
//	PPIL1Prcss *m_ppil1;       //声明串口通信层一处理实例
    QextSerialPort *m_qtcom;
    QQueue<QByteArray> m_comQueue;    //存放待处理命令的缓冲队列
    QQueue<L7Msg> m_L7MsgQueue;  //存放返回给L7的信息队列

    char m_cmdId;              //命令ID，一起返回给L7
    //LocalSocket *m_socketLast; //上一次的m_socket

    bool m_socketLockFlag;     //是否锁定只处理当前m_socket的指令
    bool m_isCommOpen;         //串口打开标志
    unsigned short m_pduRef;   //包参考
    int m_timeoutComm;         //通信超时，跟波特率相关的部分
    int m_timeoutRdyTicks;

    QByteArray m_sendBlock;    //L2打包后待发送的QByteArray
    QByteArray m_recvBlock;    //L2接收的数据

    int m_retryTimes;          //重试次数,目前设置的是重试3次
    int m_pollTimes;           //轮询次数，初步设置不能超过300次，如果300次还没有数据返回则丢弃此次命令

    QTimer *m_timerSlot;       //确认对方接收失败定时器，发送成功后启动，如超时还没接收到第一个字符认为对方接收失败
    QTimer *m_timerRcv;
    QTimer *m_timerPoll;       //轮询定时器,超时后发送轮询命令
    QTimer *m_timerActive;			//没正式发起请求之前每隔一段时间询问是否在线
    QTimer *m_timerRdy;

    unsigned char m_Da;			//目标地址
    unsigned char m_Sa;			//本机地址
    unsigned char m_lastFC;		//最后一次的FC
    bool m_isActive;			//是否在线标志
    //unsigned char m_pollDa;

};

#endif
