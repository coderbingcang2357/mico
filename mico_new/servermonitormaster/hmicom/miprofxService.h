
#ifndef MIPROFX_SERVICE_H
#define MIPROFX_SERVICE_H

#include "commservice.h"
#include "commInterface.h"
#include "rwsocket.h"

class QTimer;
class QThread;
struct MiprofxConnect
{
    struct MiProfxConnectCfg cfg;
    QList<struct HmiVar*> actVarsList; //该连接激活的变量
    QList<struct HmiVar*> onceRealVarsList; //一次实时更新变量
    QList<struct HmiVar*> waitUpdateVarsList;//该连接需要读更新的变量
    QList<struct HmiVar*> wrtieVarsList;//该变量需要写更新的变量
    QList<struct HmiVar*> waitWrtieVarsList;//该变量需要写更新的变量

    QList<struct HmiVar*> combinVarsList; //合并待请求的变量List
    struct HmiVar combinVar;  //合并待请求的变量合并后的变量
    struct HmiVar connectTestVar; //connect test var

    QList<struct HmiVar*> requestVarsList; //正在请求的变量List
    struct HmiVar requestVar; //正在请求变量合并后的变量
    int writingCnt;           //表示正在写的个数
    int connStatus;           //表示连接的状态
};

class MiProfxService : public CommService
{
    Q_OBJECT
public:
    MiProfxService( struct MiProfxConnectCfg& connectCfg );
    ~MiProfxService(); //lvyong

    bool meetServiceCfg( int serviceType, int COM_interface );
    int getConnLinkStatus( int connectId );
    int startUpdateVar( struct HmiVar* pHmiVar );
    int stopUpdateVar( struct HmiVar* pHmiVar );
    int readVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen );
    int readDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen );
    int writeVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen );
    int writeDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen );

    bool updateWriteReq( );
    bool updateReadReq( );
    void combineReadReq( );

    int sndWriteVarMsgToDriver( struct HmiVar* pHmiVar );
    int sndReadVarMsgToDriver( struct HmiVar* pHmiVar );

    bool processRspData( struct MiprofxMsgPar ,struct ComMsg comMsg );
    bool trySndWriteReq( );
    bool trySndReadReq( );
    bool hasProcessingReq( );
    bool initCommDev();

    int getComMsg( QList<struct ComMsg> &msgList );
    int onceRealUpdateVar( HmiVar* );
    bool updateOnceRealReadReq();
    int splitLongVar( struct HmiVar* pHmiVar );
/*
protected:
    virtual void run();*/
signals:
    void startRunTimer();
private slots:
        void slot_readData();
        void slot_runTimerTimeout();
        void slot_run();
private:
    struct MiprofxConnect m_connect;
    QHash<int , struct ComMsg> m_comMsg;
    int m_devUseCnt;
    int m_fd;
    int m_currTime;
    QTimer *m_runTimer;
    QHash<int , struct ComMsg> m_comMsgTmp;
    RWSocket *m_rwSocket;
};

#endif   //MIPROCOTOL4_SERVICE_H
