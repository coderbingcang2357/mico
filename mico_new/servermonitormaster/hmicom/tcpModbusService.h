
#ifndef TCP_MODBUS_SERVICE_H
#define TCP_MODBUS_SERVICE_H

#include "commservice.h"
#include "commInterface.h"
#include "./tcpModbus/tcpmodbus.h"

#include <QTimer>
#include <QHash>


//extern "C" void mbus_tcp_req__main(mbus_tcp_req_struct *p);

struct TcpModbusConnect
{
    struct TcpModbusConnectCfg cfg;
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
    uchar buff[270];
    MBUSMSG msgData;
//	int processTime;
//	bool processTimeFlg;
};

class QTimer;
#include <QHash>
class TcpModbusService : public CommService
{
    Q_OBJECT
public:
    TcpModbusService( struct TcpModbusConnectCfg& connectCfg );
    ~TcpModbusService();

    bool meetServiceCfg( int serviceType, int com_interface );
    int getConnLinkStatus( int connectId );
    int splitLongVar( struct HmiVar* pHmiVar );
    //int splitLongVar( struct HmiVar* pHmiVar);
    int startUpdateVar( struct HmiVar* pHmiVar );
    //int startUpdateVar( struct HmiVar* pHmiVar );
    int stopUpdateVar( struct HmiVar* pHmiVar );
    int readVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen );
    int readDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen );
    int writeVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen );
    int writeDevVar( struct HmiVar* pHmiVar, uchar* pBuf, int bufLen );
    int addConnect( struct TcpModbusConnectCfg& connectCfg );

    bool updateWriteReq( struct TcpModbusConnect* connect );
    bool updateReadReq( struct TcpModbusConnect* connect );
    void combineReadReq( struct TcpModbusConnect* connect );

    int sndWriteVarMsgToDriver( struct TcpModbusConnect* connect, struct HmiVar* pHmiVar );
    int sndReadVarMsgToDriver( struct TcpModbusConnect* connect, struct HmiVar* pHmiVar );

    bool processRspData( struct TcpModbusConnect* connect, QHash<int , struct ComMsg> &msgComTmp );
    bool trySndWriteReq( struct TcpModbusConnect* connect );
    bool trySndReadReq( struct TcpModbusConnect* connect );
    bool hasProcessingReq( struct TcpModbusConnect* connect );
    bool initCommDev();

    int getComMsg( QList<struct ComMsg> &msgList );
    int onceRealUpdateVar( HmiVar* );
    bool updateOnceRealReadReq( TcpModbusConnect* );
public slots:
    void slot_run();
    void slot_timerHook();
/*
protected:
    virtual void run();
*/
private:
    QList<struct TcpModbusConnect*> m_connectList;
    QHash<int , struct ComMsg> m_comMsg;
    int m_devUseCnt;
    enum { MAXDEVCNT = 35  };
    int m_fd;
    int m_currTime;
    tcpModbus *m_tcpModbus;
    QTimer *m_runTimer;
    QHash<int , struct ComMsg> m_comMsgTmp;
};

#endif   //MODBUS_SERVICE_H
