/******************************************************************************
**
** Copyright (c) 2008-20010 by SHENZHEN CO-TRUST AUTOMATION TECH.
**
** 文 件 名:  ppil1.cpp
** 说    明:  PPI通信协议物理层的实现文件
** 版    本:  V1.0
** 创建日期:  2009.03.20
** 创 建 人:  liushenghong
** 修改信息： 修改人    修改日期       修改内容
**
******************************************************************************/
#include <windows.h>
#include <time.h>
#include <winbase.h>
#include <string.h>
#include <tchar.h>
#include <QSettings>
#include <QDebug>
#include "ppil1prcss.h"
//#include "serverdlg.h"

static const int READINTVTIMEOUT = 1;       //读字符间隔超时，ms
static const int WTOTALTIMEOUTCNST = 4000;  //写超时常量，包最大长度229*11*1000/9600 = 262

ReadCommThread::ReadCommThread( HANDLE& hComm, QObject *parent )
        : QThread( parent )
{
    m_hCommPort = hComm;
    memset( &m_overlappedr, 0, sizeof( OVERLAPPED ) );
    m_overlappedr.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
    m_stopEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
}

ReadCommThread::~ReadCommThread()
{
    stop();
    CloseHandle( m_stopEvent );
    CloseHandle( m_overlappedr.hEvent );
}

void ReadCommThread::stop()
{
    if ( isRunning() )
    {
        SetEvent( m_stopEvent );
        wait();
    }
}

void ReadCommThread::run()
{
    COMSTAT ComStat;
    DWORD dwErrorFlags;

    HANDLE      handleArray[2];
    handleArray[0] = m_overlappedr.hEvent;
    handleArray[1] = m_stopEvent;

    for ( ;; )
    {
        unsigned char pInBuff[255];
        DWORD dwBytesRead = 255;
        BOOL bReadStatus = false;

        ClearCommError( m_hCommPort, &dwErrorFlags, &ComStat );      //清除错误并取得串口信息
        bReadStatus = ReadFile( m_hCommPort, pInBuff, dwBytesRead, &dwBytesRead, &m_overlappedr );

        if ( bReadStatus )
        {
            QByteArray recvBlock(( char * )pInBuff, dwBytesRead );
                        emit readComm( recvBlock );
        }
        else
        {
            if ( GetLastError() == ERROR_IO_PENDING )
            {
                switch ( WaitForMultipleObjects( 2, handleArray, FALSE, INFINITE ) )
                {
                case WAIT_OBJECT_0 + 1:  //STOP
                    PurgeComm( m_hCommPort, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR );
                    return;

                case WAIT_OBJECT_0:      //RECV
                    if ( GetOverlappedResult( m_hCommPort, &m_overlappedr, &dwBytesRead, TRUE ) ) //更新bytesRead
                    {
                        QByteArray recvBlock(( char * )pInBuff, dwBytesRead );
                        emit readComm( recvBlock );
                    }
                    break;

                default:                 //ERROR
                    PurgeComm( m_hCommPort, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR );
                    return;

                }
            }
            else                         //ERROR ELSE
            {
                PurgeComm( m_hCommPort, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR );
                return;
            }

        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
PPIL1Prcss::PPIL1Prcss()
{
    m_readCommThread = NULL;
    m_hComm = NULL;
    m_osWrite.hEvent = NULL;
}

PPIL1Prcss::~PPIL1Prcss()
{
    if ( m_readCommThread != NULL )
    {
        delete m_readCommThread;
        m_readCommThread = NULL;
    }
}

bool PPIL1Prcss::createComm( const QString &portNumber, long lBaudRate, int dataBit, int stopBit, int parity )
{
    COMMTIMEOUTS TimeOuts;           //超时控制块
    DCB dcb;                         //设备控制块

    QT_WA(
    {
        m_hComm = CreateFileW(( TCHAR* )portNumber.utf16(), //端口号
        GENERIC_READ | GENERIC_WRITE,                 //可读写
        0,                                            //
        NULL,                                         //
        OPEN_EXISTING,                                //
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, //非阻塞模式
        NULL );
    },
    {
        m_hComm = CreateFileA( portNumber.toLocal8Bit().constData(), //端口号
        GENERIC_READ | GENERIC_WRITE,                 //可读写
        0,                                            //
        NULL,                                         //
        OPEN_EXISTING,                                //
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, //非阻塞模式
        NULL );
    } );

    if ( m_hComm == INVALID_HANDLE_VALUE )
    {
        return false;
    }

    SetupComm(m_hComm, 256, 256); //MAXBLOCK, MAXBLOCK);      //设置输入输出缓冲区大小

    TimeOuts.ReadIntervalTimeout = 1;			// READINTVTIMEOUT;         //两个字符间隔超时，ms
    TimeOuts.ReadTotalTimeoutMultiplier = 0;    //每字节读超时
    TimeOuts.ReadTotalTimeoutConstant = 0;      //超时常量
    TimeOuts.WriteTotalTimeoutMultiplier = 0;   //写每字节超时
    TimeOuts.WriteTotalTimeoutConstant = WTOTALTIMEOUTCNST;  //写超时常量，包最大长度229*11*1000/9600 = 262

    SetCommTimeouts( m_hComm, &TimeOuts );

    if ( !GetCommState( m_hComm, &dcb ) )
    {
        return false;
    }

    SetCommMask( m_hComm, EV_RXCHAR );

    if ( parity < 0 || parity >4 || stopBit < 0 || stopBit > 2 || dataBit < 5 || dataBit > 8 )//检查参数
    {
        return false;
    }
    dcb.BaudRate = lBaudRate;                //波特率
    dcb.ByteSize = dataBit;                  //8位数字位
    switch(parity)							//偶校验
    {
    case 0:
        dcb.Parity = NOPARITY;
        break;
    case 1:
        dcb.Parity = ODDPARITY;
        break;
    case 2:
        dcb.Parity = EVENPARITY;
        break;
    case 3:
        dcb.Parity = MARKPARITY;
        break;
    case 4:
        dcb.Parity = SPACEPARITY;
        break;
    default:
        dcb.Parity = NOPARITY;
        break;
    }

    switch(stopBit)					//停止位
    {
    case 0:
        dcb.StopBits = ONESTOPBIT;
        break;
    case 1:
        dcb.StopBits = ONESTOPBIT;
        break;
    case 2:
        dcb.StopBits = TWOSTOPBITS;
        break;
    default:
        dcb.StopBits = ONESTOPBIT;
        break;
    }

    if ( !SetCommState( m_hComm, &dcb ) )
    {
        return false;
    }

    //清空串口事件与串口缓冲区
    PurgeComm( m_hComm, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR );

    //创建写串口事件
    memset( &m_osWrite, 0, sizeof( OVERLAPPED ) );
    m_osWrite.hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

    m_readCommThread = new ReadCommThread( m_hComm );
    connect( m_readCommThread, SIGNAL( readComm( const QByteArray & ) ), this,
             SLOT( recvData( const QByteArray & ) ), Qt::QueuedConnection );
    m_readCommThread->start();

    return true;
}

bool PPIL1Prcss::closeComm()
{
    if ( m_readCommThread != NULL )
    {
        delete m_readCommThread;
        m_readCommThread = NULL;
    }
    if ( m_osWrite.hEvent != NULL )
    {
        if ( !CloseHandle( m_osWrite.hEvent ) ) //关闭失败
            return false;
        m_osWrite.hEvent = NULL;
    }

    Q_ASSERT( m_hComm );
    if ( m_hComm != INVALID_HANDLE_VALUE )  //串口存在
    {
        if ( CloseHandle( m_hComm ) )
            m_hComm = INVALID_HANDLE_VALUE;
    }
    return true;
}

void PPIL1Prcss::recvData( const QByteArray &buf )
{
    emit readyReadComm( buf );
}

bool PPIL1Prcss::writeComm( const char *pSnd, long lBytesToWrite )
{
    BOOL bWriteState;
    COMSTAT ComStat;
    DWORD dwErrorFlags;
    DWORD dwBytesWritten;

    if ( INVALID_HANDLE_VALUE == m_hComm )                     //串口不存在返回false
    {
        return false;
    }

    if ( NULL == m_osWrite.hEvent )                            //写串口事件创建失败
    {
        return false;
    }

    ClearCommError(m_hComm, &dwErrorFlags, &ComStat);        //清除错误并取得串口信息
    PurgeComm(m_hComm, PURGE_TXABORT | PURGE_TXCLEAR);       //清空串口事件和缓冲区
    //PurgeComm( m_hComm, PURGE_TXABORT | PURGE_TXCLEAR );   //清空串口事件和缓冲区 //LIUSHENGHONG_2012-6-5 屏蔽此语句，

    bWriteState = WriteFile( m_hComm, pSnd, lBytesToWrite, &dwBytesWritten, &m_osWrite );

    if ( !bWriteState )
    {
        if ( GetLastError() == ERROR_IO_PENDING )             //如果还没写完
        {
            if ( !GetOverlappedResult( m_hComm, &m_osWrite, &dwBytesWritten, TRUE ) ) //阻塞到写完成或发生超时
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    }

    PurgeComm(m_hComm, PURGE_RXABORT | PURGE_RXCLEAR); //清除接收缓冲区,放在此处有可能导致接收数据丢失
    //(伺服上位机软件发现的)，因此挪到发送之前 LIUSHENGHONG_2011-03-16
    //3月17日又改回来，伺服发现会引入新问题，导致两个发送帧一起发送
    lBytesToWrite = 0;
    return true;
}

