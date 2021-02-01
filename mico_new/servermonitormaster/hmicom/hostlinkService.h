
#ifndef HOSTLINK_SERVICE_H
#define HOSTLINK_SERVICE_H

#include "commservice.h"
#include "commInterface.h"
#include "rwsocket.h"

class QTimer;
class QThread;
struct HostlinkConnect
{
    struct HostlinkConnectCfg cfg;
    QList<struct HmiVar*> onceRealVarsList;
    QList<struct HmiVar*> actVarsList; //该连接激活的变量
    QList<struct HmiVar*> waitUpdateVarsList;//该连接需要读更新的变量
    QList<struct HmiVar*> waitWrtieVarsList;//该变量需要写更新的变量
    QList<struct HmiVar*> wrtieVarsList;

    QList<struct HmiVar*> combinVarsList; //合并待请求的变量List
    struct HmiVar combinVar;  //合并待请求的变量合并后的变量
    struct HmiVar connectTestVar;
    QList<struct HmiVar*> requestVarsList; //正在请求的变量List
    struct HmiVar requestVar; //正在请求变量合并后的变量
    int writingCnt;           //表示正在写的个数
    int connStatus;           //表示连接的状态
};

class HostlinkService : public CommService
{
    Q_OBJECT
public:
    HostlinkService( struct HostlinkConnectCfg& connectCfg );
    ~HostlinkService(); //lvyong

    bool meetServiceCfg( int serviceType, int com_interface );
    int getConnLinkStatus( int connectId );
    int startUpdateVar( struct HmiVar* pHmiVar );
    int stopUpdateVar( struct HmiVar* pHmiVar );
    int readVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen );
    int readDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen );
    int writeVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen );
    int writeDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen );
    int addConnect( struct HostlinkConnectCfg& connectCfg );

    bool updateWriteReq( struct HostlinkConnect* connect );
    bool updateReadReq( struct HostlinkConnect* connect );
    void combineReadReq( struct HostlinkConnect* connect );

    int sndWriteVarMsgToDriver( struct HostlinkConnect* connect, struct HmiVar* pHmiVar );
    int sndReadVarMsgToDriver( struct HostlinkConnect* connect, struct HmiVar* pHmiVar );

    bool processRspData( struct HlinkMsgPar msgPar ,struct ComMsg comMsg );
    bool trySndWriteReq( struct HostlinkConnect* connect );
    bool trySndReadReq( struct HostlinkConnect* connect );
    bool hasProcessingReq( struct HostlinkConnect* connect );
    bool initCommDev();

    int getComMsg( QList<struct ComMsg> &msgList );
    int onceRealUpdateVar( HmiVar* );
    int splitLongVar( struct HmiVar* pHmiVar );
    bool updateOnceRealReadReq( HostlinkConnect* );
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
    QList<struct HostlinkConnect*> m_connectList;
    QHash<int , struct ComMsg> m_comMsg;
    int m_devUseCnt;
    enum { MAXDEVCNT = 32  };
    int m_fd;
    int m_currTime;
    QTimer *m_runTimer;
    QHash<int , struct ComMsg> m_comMsgTmp;
    RWSocket *m_rwSocket;
};

#endif   //HOSTLINK_SERVICE_H
