
#ifndef MIPROCOTOL4_SERVICE_H
#define MIPROCOTOL4_SERVICE_H

#include "commservice.h"
#include "commInterface.h"
#include "rwsocket.h"

class QTimer;
class QThread;
struct Miprocotol4Connect
{
    struct MiProtocol4ConnectCfg cfg;
    QList<struct HmiVar*> actVarsList; //该连接激活的变量
    QList<struct HmiVar*> onceRealVarsList; //一次实时更新变量
    QList<struct HmiVar*> waitUpdateVarsList;//该连接需要读更新的变量
    QList<struct HmiVar*> wrtieVarsList;//该变量需要写更新的变量
    QList<struct HmiVar*> waitWrtieVarsList;//该变量等待写更新

    QList<struct HmiVar*> combinVarsList; //合并待请求的变量List
    struct HmiVar combinVar;  //合并待请求的变量合并后的变量
    struct HmiVar connectTestVar; //connect test var

    QList<struct HmiVar*> requestVarsList; //正在请求的变量List
    struct HmiVar requestVar; //正在请求变量合并后的变量
    int writingCnt;           //表示正在写的个数
    int connStatus;           //表示连接的状态

    int maxBR;            //位读最大长度
    int maxBW;            //位写最大长度
    int maxWR;            //字读最大长度
    int maxWW;            //字写最大长度
};

class MiProtocol4Service : public CommService
{
    Q_OBJECT
public:
    MiProtocol4Service( struct MiProtocol4ConnectCfg& connectCfg );
    ~MiProtocol4Service(); //lvyong

    bool meetServiceCfg( int serviceType, int COM_interface );
    int getConnLinkStatus( int connectId );
    int splitLongVar( struct HmiVar* pHmiVar );
    //int splitLongVar( struct HmiVar* pHmiVar);
    int startUpdateVar( struct HmiVar* pHmiVar );
    int stopUpdateVar( struct HmiVar* pHmiVar );
    int readVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen );
    int readDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen );
    int writeVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen );
    int writeDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen );
    int addConnect( struct MiProtocol4ConnectCfg& connectCfg );

    bool updateWriteReq( struct Miprocotol4Connect* connect );
    bool updateReadReq( struct Miprocotol4Connect* connect );
    void combineReadReq( struct Miprocotol4Connect* connect );

    int sndWriteVarMsgToDriver( struct Miprocotol4Connect* connect, struct HmiVar* pHmiVar );
    int sndReadVarMsgToDriver( struct Miprocotol4Connect* connect, struct HmiVar* pHmiVar );

    bool processRspData( struct MiProtocol4MsgPar msgPar ,struct ComMsg comMsg  );
    bool trySndWriteReq( struct Miprocotol4Connect* connect );
    bool trySndReadReq( struct Miprocotol4Connect* connect );
    bool hasProcessingReq( struct Miprocotol4Connect* connect );
    bool initCommDev();

    int getComMsg( QList<struct ComMsg> &msgList );
    int onceRealUpdateVar( HmiVar* );
    bool updateOnceRealReadReq( Miprocotol4Connect* );
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
    QList<struct Miprocotol4Connect*> m_connectList;
    QHash<int , struct ComMsg> m_comMsg;
    int m_devUseCnt;
    enum { MAXDEVCNT = 32  };
    int m_fd;
    int m_currTime;
    QTimer *m_runTimer;
    QHash<int , struct ComMsg> m_comMsgTmp;
    RWSocket *m_rwSocket;
};

#endif   //MIPROCOTOL4_SERVICE_H
