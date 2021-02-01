/**********************************************************************
Copyright (c) 2008-2010 by SHENZHEN CO-TRUST AUTOMATION TECH.
** 文 件 名:  dvpService.cpp
** 说    明:  台达DVP协议多协议平台头文件，定义一个DVP协议的类
** 版    本:  【可选】
** 创建日期:  2012.06.05
** 创 建 人:  陶健军
** 修改信息： 【可选】
**         修改人    修改日期       修改内容**
*********************************************************************/
#ifndef DVP_SERVICE_H
#define DVP_SERVICE_H

#include "commservice.h"
#include "commInterface.h"
#include "rwsocket.h"

class QTimer;
class QThread;
struct DvpConnect
{
    struct DvpConnectCfg cfg;
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
};
/**************************************************************
*** 类    名:  DvpService
** 功    能:  实现DVP协议
** 接口说明:  实现后台的数据读写，链接管理
** 修改信息： 【若修改过则必需注明】**
 **************************************************************/
class DvpService : public CommService
{
    Q_OBJECT
public:
    DvpService( struct DvpConnectCfg& connectCfg );
    ~DvpService();

    bool meetServiceCfg( int serviceType, int com_interface );
    int getConnLinkStatus( int connectId );
    int splitLongVar( struct HmiVar* pHmiVar );
    //int splitLongVar( struct HmiVar* pHmiVar);
    int startUpdateVar( struct HmiVar* pHmiVar );
    int stopUpdateVar( struct HmiVar* pHmiVar );
    int readVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen );
    int readDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen );
    int writeVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen );
    int writeDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen );
    int addConnect( struct DvpConnectCfg& connectCfg );

    bool updateWriteReq( struct DvpConnect* connect );
    bool updateReadReq( struct DvpConnect* connect );
    void combineReadReq( struct DvpConnect* connect );

    int sndWriteVarMsgToDriver( struct DvpConnect* connect, struct HmiVar* pHmiVar );
    int sndReadVarMsgToDriver( struct DvpConnect* connect, struct HmiVar* pHmiVar );

    bool processRspData( struct DvpMsgPar msgPar ,struct ComMsg comMsg  );
    bool trySndWriteReq( struct DvpConnect* connect );
    bool trySndReadReq( struct DvpConnect* connect );
    bool hasProcessingReq( struct DvpConnect* connect );
    bool initCommDev();

    int getComMsg( QList<struct ComMsg> &msgList );
    int onceRealUpdateVar( HmiVar* );
    bool updateOnceRealReadReq( DvpConnect* );
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
    QList<struct DvpConnect*> m_connectList;
    QHash<int , struct ComMsg> m_comMsg;
    int m_devUseCnt;
    enum { MAXDEVCNT = 32  };
    int m_fd;
    int m_currTime;
    QTimer *m_runTimer;
    QHash<int , struct ComMsg> m_comMsgTmp;
    RWSocket *m_rwSocket;
};

#endif   //DVP_SERVICE_H
