
#ifndef MPI300_SERVICE_H
#define MPI300_SERVICE_H

#include <QTimer>
#include "commservice.h"
#include "commInterface.h"

#define PPI_ERR_NO_ERR			0x00	// 没错误
#define PPI_ERR_TIMEROUT		0x01	// 通讯超时

#define L7_OneLink_IsLinked_Ok  0x04
struct Mpi300Connect
{
    struct Mpi300ConnectCfg cfg;
    QList<struct HmiVar*> actVarsList; //该连接激活的变量
    QList<struct HmiVar*> onceRealVarsList; //一次实时更新变量
    QList<struct HmiVar*> waitUpdateVarsList; //该连接需要读更新的变量
    QList<struct HmiVar*> wrtieVarsList;//该变量需要写更新的变量
    QList<struct HmiVar*> waitWrtieVarsList; //该变量需要写更新的变量

    QList<struct HmiVar*> combinVarsList; //合并待请求的变量List
    struct HmiVar combinVar;  //合并待请求的变量合并后的变量
    struct HmiVar connectTestVar; //connect test var

    QList<struct HmiVar*> requestVarsList; //正在请求的变量List
    struct HmiVar requestVar; //正在请求变量合并后的变量
    int writingCnt;           //表示正在写的个数
    bool isLinking;           //MPI是否正在创建连接
    int beginTime;            //开始建立连接时间
    int connStatus;           //表示连接的状态
};

class RWSocket;
class Mpi300Service : public CommService
{
    Q_OBJECT
public:
    Mpi300Service( struct Mpi300ConnectCfg& connectCfg );
    ~Mpi300Service(); //lvyong

    bool meetServiceCfg( int serviceType, int COM_interface );
    int getConnLinkStatus( int connectId );
    int startUpdateVar( struct HmiVar* pHmiVar );
    int stopUpdateVar( struct HmiVar* pHmiVar );
    int readVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen );
    int readDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen );
    int writeVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen );
    int writeDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen );
    int addConnect( struct Mpi300ConnectCfg& connectCfg );

    bool updateWriteReq( struct Mpi300Connect* connect );
    bool updateReadReq( struct Mpi300Connect* connect );
    void combineReadReq( struct Mpi300Connect* connect );

    int sndWriteVarMsgToDriver( struct Mpi300Connect* connect, struct HmiVar* pHmiVar );
    int PackAndPostL2MsgPpiMpiR();
    int sndReadVarMsgToDriver( struct Mpi300Connect* connect, struct HmiVar* pHmiVar );

    bool processRspData( char *pMsg, QHash<int , struct ComMsg> &msgComTmp );//( int mstime );
    //int processMulReadRsp(L7_MSG_STRU *pMsg, struct Mpi300Connect* connect);
    bool trySndWriteReq( struct Mpi300Connect* connect, QHash<int , struct ComMsg> &msgComTmp );
    bool trySndReadReq( struct Mpi300Connect* connect, QHash<int , struct ComMsg> &msgComTmp );
    bool hasProcessingReq( struct Mpi300Connect* connect );
    bool initCommDev();
    bool createMpiConnLink( struct Mpi300Connect* connect );


    int getComMsg( QList<struct ComMsg> &msgList );
    int onceReadUpdateVar( HmiVar* );
    bool updateOnceReadReadReq( Mpi300Connect* );
    int splitLongVar( struct HmiVar* pHmiVar );
    void readAllVar( int connectId, struct HmiVar* pHmiVar );

    int processMulReadRsp( char *pMsg, struct Mpi300Connect* pConnect );
    int processMulWriteRsp( char *pMsg, struct Mpi300Connect* pConnect );
    void initPpiMpiDriver();
signals:
    void startRuntime();
public slots:
    void slot_readCOMData();
    void slot_runTimeout();
    void slot_run();

private:
    enum { MAX_MPI_CONNECT_COUNT = 40 };
    QList<struct Mpi300Connect*> m_connectList;
    QHash<int , struct ComMsg> m_comMsg;
    QHash<int , struct ComMsg> m_comMsgTmp;
    bool m_isMpi;
    int m_devUseCnt;
    int m_fd;
    int m_currTime;
    QTimer *m_runTime;
    RWSocket *m_rwSocket;
    char m_sn;//ppi serial number
    bool iniDriverFlg;
	bool m_isInitPPIMsgSend;
};

#endif   //PPIMPI_SERVICE_H
