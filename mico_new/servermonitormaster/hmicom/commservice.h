#include <QtGui>

#ifndef COM_SERVICE_H
#define COM_SERVICE_H

//#include "datatypedef.h"
#include "ct_def.h"
#include "commInterface.h"

#define SERVICE_TYPE_MODBUS         1
#define SERVICE_TYPE_PPIMPI         2
#define SERVICE_TYPE_HOSTLINK       3
#define SERVICE_TYPE_MIPROTOCOL4    4
#define SERVICE_TYPE_MIPROFX        5
#define SERVICE_TYPE_FINHOSTLINK    6
#define SERVICE_TYPE_MEWTOCOL       7
#define SERVICE_TYPE_DVP            8
#define SERVICE_TYPE_FATEK          9
#define SERVICE_TYPE_CNET           10
#define SERVICE_TYPE_MPI300         11
#define SERVICE_TYPE_FP             12
#define SERVICE_TYPE_TCPMBUS        13
#define SERVICE_TYPE_GATEWAY        14

class CommService : public QObject
{
    Q_OBJECT
public:
    CommService();
    //基类的析构函数必须设为虚函数，否则使用基类指针析构派生类时不会调用派生类的析构函数，现像就是线程一直没退出
    virtual ~CommService();

    virtual bool meetServiceCfg( int serviceType, int COM_interface ) = 0;
    virtual int getComMsg( QList<struct ComMsg> &msgList ) = 0;
    virtual int getConnLinkStatus( int connectId ) = 0;
    virtual int startUpdateVar( struct HmiVar* pHmiVar ) = 0;
    virtual int stopUpdateVar( struct HmiVar* pHmiVar ) = 0;
    virtual int readDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen ) = 0;
    virtual int writeDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen ) = 0;
    virtual int splitLongVar( struct HmiVar* pHmiVar ) = 0;

protected:
    int getCurrTime();
    int calIntervalTime( int lastTime, int currTime );
    int getDataTypeLen( INT8U type );

protected:
    QMutex m_mutexRw;
    QMutex m_mutexWReq;
    QMutex m_mutexUpdateVar;
    QMutex m_mutexOnceReal;
    QMutex m_mutexGetMsg;
    QMutex m_mutexThreadRW;
    QTime  m_time;
    int m_timeTicks;
};

#endif //COM_SERVICE_H
