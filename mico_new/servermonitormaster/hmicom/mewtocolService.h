
#ifndef MEWTOCOL_SERVICE_H
#define MEWTOCOL_SERVICE_H

#include "commservice.h"
#include "commInterface.h"
#include "modbus/modbusdriver.h"
#include "rwsocket.h"

class QTimer;
class QThread;
struct MewtocolConnect
{
    struct MewtocolConnectCfg cfg;
    QList<struct HmiVar*> actVarsList; //该连接激活的变量
    QList<struct HmiVar*> onceRealVarsList;
    QList<struct HmiVar*> waitUpdateVarsList;//该连接需要读更新的变量
    QList<struct HmiVar*> writeVarsList;
    QList<struct HmiVar*> waitWriteVarsList;//该变量需要写更新的变量

    QList<struct HmiVar*> combinVarsList; //合并待请求的变量List
    struct HmiVar combinVar;  //合并待请求的变量合并后的变量
    struct HmiVar connectTestVar; //connect test var

    QList<struct HmiVar*> requestVarsList; //正在请求的变量List
    struct HmiVar requestVar; //正在请求变量合并后的变量
    int writingCnt;           //表示正在写的个数
    int connStatus;           //表示连接的状态
};

class MewtocolService : public CommService
{
    Q_OBJECT
public:
    MewtocolService( struct MewtocolConnectCfg& connectCfg );
    ~MewtocolService(); //lvyong

    bool meetServiceCfg( int serviceType, int COM_interface );
    int getConnLinkStatus( int connectId );
    int startUpdateVar( struct HmiVar* pHmiVar );
    int stopUpdateVar( struct HmiVar* pHmiVar );
    int readVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen );
    int readDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen );
    int writeVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen );
    int writeDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen );
    int addConnect( struct MewtocolConnectCfg& connectCfg );

    bool updateWriteReq( struct MewtocolConnect* connect );
    bool updateReadReq( struct MewtocolConnect* connect );
    void combineReadReq( struct MewtocolConnect* connect );

    int sndWriteVarMsgToDriver( struct MewtocolConnect* connect, struct HmiVar* pHmiVar );
    int sndReadVarMsgToDriver( struct MewtocolConnect* connect, struct HmiVar* pHmiVar );

    bool processRspData( struct MewtocolMsgPar msgPar ,struct ComMsg comMsg );
    bool trySndWriteReq( struct MewtocolConnect* connect );
    bool trySndReadReq( struct MewtocolConnect* connect );
    bool hasProcessingReq( struct MewtocolConnect* connect );
    bool initCommDev();

    int getComMsg( QList<struct ComMsg> &msgList );
    int onceRealUpdateVar( HmiVar* );
    bool updateOnceRealReadReq( MewtocolConnect* );
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
    QList<struct MewtocolConnect*> m_connectList;
    QHash<int , struct ComMsg> m_comMsg;
    enum { MAXDEVCNT = 16  };
    int m_devUseCnt;
    int m_fd;
    int m_currTime;
    QTimer *m_runTimer;
    QHash<int , struct ComMsg> m_comMsgTmp;
    RWSocket *m_rwSocket;
};

#endif   //HOSTLINK_SERVICE_H
