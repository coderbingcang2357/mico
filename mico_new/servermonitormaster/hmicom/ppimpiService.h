#ifndef PPIMPI_SERVICE_H
#define PPIMPI_SERVICE_H

#include <QTimer>
#include "commservice.h"
#include "commInterface.h"
//#include "ppi/ppil2prcss.h"

// 应答层7的错误状态
#define PPI_ERR_NO_ERR			0x00	// 没错误
#define PPI_ERR_TIMEROUT		0x01	// 通讯超时

#define L7_OneLink_IsLinked_Ok  0x04
/*
typedef enum
{
    L7_Net12_Close_Ok,
    L7_Net12_Start_Ok,
    L7_Net12_Restart_Ok,

    L7_Delete_OneLink_Ok,				//层二中删除一条连接的步骤完成
    L7_OneLink_IsLinked_Ok,			//一个被要求的连接已经连上(MPI或者PPI)
    L7_OneLinked_MPICutted,			//一个已经连上的MPI连接断开

    L7_MSG_DATA_IND,				// 来自对方的SD2请求，发给层7
    L7_MSG_DATA_CON,				// 层7的主站请求的应答
    L7_MSG_SET_CON,
    L7_MSG_UPDATE_CON,                  	// 层7的从站应答的响应
    L7_MSG_NULL,                         		// 无效的消息
    L7_ERR_PARA
} L7_MSG_TYPE;
*/
struct PpiMpiConnect
{
    struct PpiMpiConnectCfg cfg;
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
    //int pollCnt;              //定时器轮询
};

class RWSocket;

class PpiMpiService : public CommService
{
    Q_OBJECT
public:
    PpiMpiService( struct PpiMpiConnectCfg& connectCfg );
    ~PpiMpiService(); //lvyong

    bool meetServiceCfg( int serviceType, int COM_interface );
    int getConnLinkStatus( int connectId );
    int startUpdateVar( struct HmiVar* pHmiVar );
    int stopUpdateVar( struct HmiVar* pHmiVar );
    int readVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen );
    int readDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen );
    int writeVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen );
    int writeDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen );
    int addConnect( struct PpiMpiConnectCfg& connectCfg );

    bool updateWriteReq( struct PpiMpiConnect* pConnect );
    bool updateReadReq( struct PpiMpiConnect* pConnect );
    void combineReadReq( struct PpiMpiConnect* pConnect );

    int sndWriteVarMsgToDriver( struct PpiMpiConnect* pConnect, struct HmiVar* pHmiVar );
    int PackAndPostL2MsgPpiMpiR();
    int sndReadVarMsgToDriver( struct PpiMpiConnect* pConnect, struct HmiVar* pHmiVar );

    bool processRspData( char *pMsg, QHash<int , struct ComMsg> &msgComTmp );//( int mstime );
    //int processMulReadRsp(L7_MSG_STRU *pMsg, struct PpiMpiConnect* pConnect);
    bool trySndWriteReq( struct PpiMpiConnect* pConnect, QHash<int , struct ComMsg> &msgComTmp );
    bool trySndReadReq( struct PpiMpiConnect* pConnect, QHash<int , struct ComMsg> &msgComTmp );
    bool hasProcessingReq( struct PpiMpiConnect* pConnect );
    bool initCommDev();
    bool createMpiConnLink( struct PpiMpiConnect* pConnect );


    int getComMsg( QList<struct ComMsg> &msgList );
    int onceRealUpdateVar( HmiVar* );
    bool updateOnceRealReadReq( PpiMpiConnect* );
    int splitLongVar( struct HmiVar* pHmiVar );
    void readAllVar( int connectId, struct HmiVar* pHmiVar );

    int processMulReadRsp( char *pMsg, struct PpiMpiConnect* pConnect );
    int processMulWriteRsp( char *pMsg, struct PpiMpiConnect* pConnect );
    void initPpiMpiDriver();
signals:
    void startRuntime();
public slots:
    void slot_readCOMData();
    void slot_runTimeout();
    void slot_run();

private:
    enum { MAX_MPI_CONNECT_COUNT = 40 };
    QList<struct PpiMpiConnect*> m_connectList;
    QHash<int , struct ComMsg> m_comMsg;
    QHash<int , struct ComMsg> m_comMsgTmp;
    bool m_isMpi;
    int m_fd;
    int m_currTime;
    //PPIL2Prcss *ppil2;
    QTimer *m_runTime;
    RWSocket *m_rwSocket;
    char m_sn;//ppi serial number
    bool m_iniDriverFlg;
    bool m_isInitPPIMsgSend;
};

#endif   //PPIMPI_SERVICE_H
