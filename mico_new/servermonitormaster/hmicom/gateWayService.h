
#ifndef GATEWAY_SERVICE_H
#define GATEWAY_SERVICE_H

#include <QTimer>
#include "commservice.h"
#include "commInterface.h"
#include "gateWay/udpL1L2.h"

struct GateWayConnect
{
    struct GateWayConnectCfg cfg;
    QList<struct HmiVar*> actVarsList; //该连接激活的变量
    QList<struct HmiVar*> onceRealVarsList; //一次实时更新变量
    QList<struct HmiVar*> waitUpdateVarsList; //该连接需要读更新的变量
    QList<struct HmiVar*> wrtieVarsList;//该变量需要写更新的变量
    QList<struct HmiVar*> waitWrtieVarsList; //该变量需要写更新的变量

    QList<struct HmiVar*> combinVarsList; //合并待请求的变量List
    struct HmiVar combinVar;  //合并待请求的变量合并后的变量
    struct HmiVar connectTestVar; //pConnect test var

    QList<struct HmiVar*> requestVarsList; //正在请求的变量List
    struct HmiVar requestVar; //正在请求变量合并后的变量
    int writingCnt;           //表示正在写的个数
    bool isLinking;           //MPI是否正在创建连接
    int beginTime;            //开始建立连接时间
    int connStatus;           //表示连接的状态
};

void DelGateWayConnect(struct GateWayConnect *conn);

class GateWayService : public CommService
{
    Q_OBJECT
public:
    GateWayService( struct GateWayConnectCfg& connectCfg );
    ~GateWayService();

    bool meetServiceCfg( int serviceType, int COM_interface );
    int getConnLinkStatus( int connectId );
    int startUpdateVar( struct HmiVar* pHmiVar );
    int stopUpdateVar( struct HmiVar* pHmiVar );
    int readVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen );
    int readDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen );
    int writeVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen );
    int writeDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen );
    int addConnect( struct GateWayConnectCfg& connectCfg );

    bool updateWriteReq( struct GateWayConnect* pConnect );
    bool updateReadReq( struct GateWayConnect* pConnect );
    void combineReadReq( struct GateWayConnect* pConnect );

    int sndWriteVarMsgToDriver( struct GateWayConnect* pConnect, struct HmiVar* pHmiVar );
    int PackAndPostL2MsgPpiMpiR();
    int sndReadVarMsgToDriver( struct GateWayConnect* pConnect, struct HmiVar* pHmiVar );

    bool processRspData( QHash<int , struct ComMsg> &msgComTmp );//( int mstime );
    //int processMulReadRsp(L7_MSG_STRU *pMsg, struct GateWayConnect* pConnect);
    bool trySndWriteReq( struct GateWayConnect* pConnect, QHash<int , struct ComMsg> &msgComTmp );
    bool trySndReadReq( struct GateWayConnect* pConnect, QHash<int , struct ComMsg> &msgComTmp );
    bool hasProcessingReq( struct GateWayConnect* pConnect );
    bool initCommDev();
    bool createMpiConnLink( struct GateWayConnect* pConnect );


    int getComMsg( QList<struct ComMsg> &msgList );
    int onceRealUpdateVar( HmiVar* );
    bool updateOnceRealReadReq( GateWayConnect* );
    int splitLongVar( struct HmiVar* pHmiVar );
    void readAllVar( int connectId, struct HmiVar* pHmiVar );

    int processMulReadRsp( struct UdpL7Msg *pMsg, struct GateWayConnect* pConnect );
    int processMulWriteRsp( struct UdpL7Msg *pMsg, struct GateWayConnect* pConnect );

signals:
    void startRuntime();
    void writeMsg( QString ipAddr, int Port, int connectID, const QByteArray &array );

public slots:
    void slot_readCOMData();
    void slot_runTimeout();
    void slot_run();
/*protected:
    virtual void run();
*/
private:
    enum { MAX_MPI_CONNECT_COUNT = 40 };
    QList<struct GateWayConnect*> m_connectList;
    QHash<int , struct ComMsg> m_comMsg;
    QHash<int , struct ComMsg> m_comMsgTmp;
    bool m_isMpi;
    int m_devUseCnt;
    int m_fd;
    int m_currTime;
    UDPL1L2 *ppil2;
    QTimer *m_runTime;
};

#endif   //GATEWAY_SERVICE_H
