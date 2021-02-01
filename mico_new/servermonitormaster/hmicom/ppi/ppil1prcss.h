/******************************************************************************
**
** Copyright (c) 2008-20010 by SHENZHEN CO-TRUST AUTOMATION TECH.
**
** 文 件 名:  ppil1prcss.h
** 说    明:  PPI通信协议物理层的头文件
** 版    本:  V1.0
** 创建日期:  2009.03.20
** 创 建 人:  liushenghong
** 修改信息： 修改人    修改日期       修改内容
**
******************************************************************************/
#ifndef PPI_L1_H
#define PPI_L1_H

#include <tchar.h>
#include <iostream>
#include <windows.h>
#include <QByteArray>
#include <QThread>
#include <QObject>

const int MAXBLOCK = 2048;             //输入缓冲区的大小

/******************************************************************************
** 类    名:   PrcssCommThread
** 功    能:   从缓冲队列取出PDU打包并发送给PLC，并将返回结果发送给L7，发射数据已返回的信号
** 接口说明:   见注释
** 修改信息： 【若修改过则必需注明】
******************************************************************************/
class ReadCommThread : public QThread
{
    Q_OBJECT

public:
    ReadCommThread( HANDLE&, QObject *parent = 0 );
    ~ReadCommThread();
    void stop();

signals:
    void readComm( const QByteArray & );

protected:
    void run();

private:
    HANDLE m_stopEvent;
    HANDLE m_hCommPort;
    OVERLAPPED m_overlappedr;
};

/////////////////////////////////////////////////////////////////////////////////////////////
class PPIL1Prcss : public QObject
{
    Q_OBJECT

public:
    PPIL1Prcss();
    ~PPIL1Prcss();

    bool createComm( const QString &portNumber, long lBaudRate, int dataBit, int stopBit, int parity ); //创建串口
    bool closeComm();                                           //关闭串口
    bool writeComm( const char *pSnd, long lBytesToWrite );    //写串口

signals:
    void readyReadComm( const QByteArray & );

private slots:
    void recvData( const QByteArray & );

private:
    HANDLE m_hComm;                            //串口句柄
    OVERLAPPED m_osWrite;                      //写串口结构
    ReadCommThread *m_readCommThread;
};

#endif
