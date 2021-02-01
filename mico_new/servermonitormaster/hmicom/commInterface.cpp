#include "commservice.h"
#include "commInterface.h"
#include "mpi300Service.h" //更新至HMI2018-04-26 这几行只是换了位置，其实没有改
#include "modbusService.h"
#include "ppimpiService.h"

#include "hostlinkService.h"
#include "finHostlinkService.h"
#include "miprotocol4Service.h"
#include "miprofxService.h"
#include "mewtocolService.h"
#include "dvpService.h"
#include "fatekService.h"
#include "cnetService.h"
#include "FPService.h"
#include "gateWayService.h"
#include "tcpModbusService.h"

#include <string>
#include "stdio.h"


void HMICommService::stopAllConn()
{
#if 1
    for ( int i = 0; i < m_commServiceList.count(); i++ )
    {
        CommService* service = m_commServiceList[i];    
		delete service;
        //service->deleteLater();
    }
#endif
    //for ( int j = 0; j < m_commSrvThreadList.count(); j++ )
    //{
    //    QThread* thread = m_commSrvThreadList[j];
    //    thread->exit();
    //    thread->wait();
    //    delete thread;
    //}
	//qDeleteAll(m_commServiceList);
    //m_commServiceList.clear();
    //m_commSrvThreadList.clear();
#if 1
	QHash<int, struct HmiVar*>::const_iterator it = m_hmiVarList.constBegin();
	while(it != m_hmiVarList.constEnd()){
		struct HmiVar *var = it.value();
		if(NULL != var){
			//DelHmiVar(var);
			delete var;
			var = NULL;
		}
		++it;
	}
    m_hmiVarList.clear();
#endif
}

void DelHmiVar(struct HmiVar *hmival){
	qDebug() << "DelHmiVar";
	if(hmival == NULL){
		return;
	}
#if 1
	//qDebug() << "del hmival varId:" << hmival->varId << "splitVars len:" << hmival->splitVars.length();
	if(!hmival->splitVars.isEmpty()){
		QList<struct HmiVar*>::iterator item = hmival->splitVars.begin();
		while(item != hmival->splitVars.end()){
			DelHmiVar(*item);
			delete *item;
			*item = NULL;
			qDebug() << "DelHmiVar" << item - hmival->splitVars.begin();
			++item;
		}
		hmival->splitVars.clear();
	}
#endif
}

//注册MODBUS连接
int HMICommService::registerConnect( struct ModbusConnectCfg& connectCfg )
{
    for ( int i = 0; i < m_commServiceList.count(); i++ )
    {
        CommService* service = m_commServiceList[i];
        if ( service->meetServiceCfg( SERVICE_TYPE_MODBUS, connectCfg.COM_interface ) )
        {
            (( ModbusService* )service )->addConnect( connectCfg );
            return 0;
        }
    }
    ModbusService* service = new ModbusService( connectCfg );
    QTimer::singleShot(1, service, SLOT(slot_run()));
    //QThread *runThread = new QThread;
    //QObject::connect(runThread, SIGNAL(started()),service,SLOT(slot_run()));
    //service->moveToThread(runThread);
    //runThread->start();
    //Q_ASSERT( service );
    m_commServiceList.append( service );
    //m_commSrvThreadList.append(runThread);
    return 0;
}

//注册PPI/MPI连接
int HMICommService::registerConnect( struct PpiMpiConnectCfg& connectCfg )
{
    for ( int i = 0; i < m_commServiceList.count(); i++ )
    {
        CommService* service = m_commServiceList[i];
        if ( service->meetServiceCfg( SERVICE_TYPE_PPIMPI, connectCfg.COM_interface ) )
        {
            (( PpiMpiService* )service )->addConnect( connectCfg );
            return 0;
        }
    }
    PpiMpiService* service = new PpiMpiService( connectCfg );
    QTimer::singleShot(1, service, SLOT(slot_run()));
    //QThread *runThread = new QThread;
    //QObject::connect(runThread, SIGNAL(started()),service,SLOT(slot_run()));
    //service->moveToThread(runThread);
    //runThread->start();
    //Q_ASSERT( service );
    m_commServiceList.append( service );
    //m_commSrvThreadList.append(runThread);
    return 0;
}

//注册三菱 COMPUTER CONTROL LINK FORMAT 4连接
int HMICommService::registerConnect( struct MiProtocol4ConnectCfg& connectCfg )
{
    for ( int i = 0; i < m_commServiceList.count(); i++ )
    {
        CommService* service = m_commServiceList[i];
        if ( service->meetServiceCfg( SERVICE_TYPE_MIPROTOCOL4, connectCfg.COM_interface ) )
        {
            (( MiProtocol4Service* )service )->addConnect( connectCfg );
            return 0;
        }
    }
    MiProtocol4Service* service = new MiProtocol4Service( connectCfg );
    QTimer::singleShot(1, service, SLOT(slot_run()));
    //QThread *runThread = new QThread;
    //QObject::connect(runThread, SIGNAL(started()),service,SLOT(slot_run()));
    //service->moveToThread(runThread);
    //runThread->start();
    //Q_ASSERT( service );
    m_commServiceList.append( service );
    //m_commSrvThreadList.append(runThread);
    return 0;
}

//注册台达 DVP 协议
int HMICommService::registerConnect( struct DvpConnectCfg& connectCfg )
{
    for ( int i = 0; i < m_commServiceList.count(); i++ )
    {
        CommService* service = m_commServiceList[i];
        if ( service->meetServiceCfg( SERVICE_TYPE_DVP, connectCfg.COM_interface ) )
        {
            (( DvpService* )service )->addConnect( connectCfg );
            return 0;
        }
    }
    DvpService* service = new DvpService( connectCfg );
    QTimer::singleShot(1, service, SLOT(slot_run()));
    //QThread *runThread = new QThread;
    //QObject::connect(runThread, SIGNAL(started()),service,SLOT(slot_run()));
    //service->moveToThread(runThread);
    //runThread->start();
    //Q_ASSERT( service );
    m_commServiceList.append( service );
    //m_commSrvThreadList.append(runThread);
    return 0;
}

//注册永宏 FATEK 协议
int HMICommService::registerConnect( struct FatekConnectCfg& connectCfg )
{
    for ( int i = 0; i < m_commServiceList.count(); i++ )
    {
        CommService* service = m_commServiceList[i];
        if ( service->meetServiceCfg( SERVICE_TYPE_FATEK, connectCfg.COM_interface ) )
        {
            (( FatekService* )service )->addConnect( connectCfg );
            return 0;
        }
    }
    FatekService* service = new FatekService( connectCfg );
    QTimer::singleShot(1, service, SLOT(slot_run()));
    //QThread *runThread = new QThread;
    //QObject::connect(runThread, SIGNAL(started()),service,SLOT(slot_run()));
    //service->moveToThread(runThread);
    //runThread->start();
    //Q_ASSERT( service );
    m_commServiceList.append( service );
    //m_commSrvThreadList.append(runThread);
    return 0;
}


//注册HOSTLINK连接
int HMICommService::registerConnect( struct HostlinkConnectCfg& connectCfg )
{
    for ( int i = 0; i < m_commServiceList.count(); i++ )
    {
        CommService* service = m_commServiceList[i];
        if ( service->meetServiceCfg( SERVICE_TYPE_HOSTLINK, connectCfg.COM_interface ) )
        {
            (( HostlinkService* )service )->addConnect( connectCfg );
            return 0;
        }
    }
    HostlinkService* service = new HostlinkService( connectCfg );
    QTimer::singleShot(1, service, SLOT(slot_run()));
    //QThread *runThread = new QThread;
    //QObject::connect(runThread, SIGNAL(started()),service,SLOT(slot_run()));
    //service->moveToThread(runThread);
    //runThread->start();
    ////qDebug()<<"regist Hostlink Service! ";
    //Q_ASSERT( service );
    m_commServiceList.append( service );
    //m_commSrvThreadList.append(runThread);
    return 0;
}

//注册FINHOSTLINK连接
int HMICommService::registerConnect( struct FinHostlinkConnectCfg& connectCfg )
{
    for ( int i = 0; i < m_commServiceList.count(); i++ )
    {
        CommService* service = m_commServiceList[i];
        if ( service->meetServiceCfg( SERVICE_TYPE_FINHOSTLINK, connectCfg.COM_interface ) )
        {
            (( FinHostlinkService* )service )->addConnect( connectCfg );
            return 0;
        }
    }
    FinHostlinkService* service = new FinHostlinkService( connectCfg );
    QTimer::singleShot(1, service, SLOT(slot_run()));
    //QThread *runThread = new QThread;
    //QObject::connect(runThread, SIGNAL(started()),service,SLOT(slot_run()));
    //service->moveToThread(runThread);
    //runThread->start();
    //Q_ASSERT( service );
    m_commServiceList.append( service );
    //m_commSrvThreadList.append(runThread);
    return 0;
}



//注册三菱 编程口协议
int HMICommService::registerConnect( struct MiProfxConnectCfg& connectCfg )
{
    for ( int i = 0; i < m_commServiceList.count(); i++ )
    {
        CommService* service = m_commServiceList[i];
        if ( service->meetServiceCfg( SERVICE_TYPE_MIPROFX, connectCfg.COM_interface ) )
        {
            return -1;
        }
    }
    MiProfxService* service = new MiProfxService( connectCfg );
    QTimer::singleShot(1, service, SLOT(slot_run()));
    //QThread *runThread = new QThread;
    //QObject::connect(runThread, SIGNAL(started()),service,SLOT(slot_run()));
    //service->moveToThread(runThread);
    //runThread->start();
    //Q_ASSERT( service );
    m_commServiceList.append( service );
    //m_commSrvThreadList.append(runThread);
    return 0;
}

//注册松下 Mewtocol协议
int HMICommService::registerConnect( struct MewtocolConnectCfg& connectCfg )
{
    for ( int i = 0; i < m_commServiceList.count(); i++ )
    {
        CommService* service = m_commServiceList[i];
        if ( service->meetServiceCfg( SERVICE_TYPE_MEWTOCOL, connectCfg.COM_interface ) )
        {
            (( MewtocolService* )service )->addConnect( connectCfg );
            return 0;
        }
    }
    MewtocolService* service = new MewtocolService( connectCfg );
    QTimer::singleShot(1, service, SLOT(slot_run()));
    //QThread *runThread = new QThread;
    //QObject::connect(runThread, SIGNAL(started()),service,SLOT(slot_run()));
    //service->moveToThread(runThread);
    //runThread->start();
    //Q_ASSERT( service );
    m_commServiceList.append( service );
    //m_commSrvThreadList.append(runThread);
    return 0;
}


//注册LS masterK Cnet 协议
int HMICommService::registerConnect( struct CnetConnectCfg& connectCfg )
{
    for ( int i = 0; i < m_commServiceList.count(); i++ )
    {
        CommService* service = m_commServiceList[i];
        if ( service->meetServiceCfg( SERVICE_TYPE_CNET, connectCfg.COM_interface ) )
        {
            (( CnetService* )service )->addConnect( connectCfg );
            return 0;
        }
    }
    CnetService* service = new CnetService( connectCfg );
    QTimer::singleShot(1, service, SLOT(slot_run()));
    //QThread *runThread = new QThread;
    //QObject::connect(runThread, SIGNAL(started()),service,SLOT(slot_run()));
    //service->moveToThread(runThread);
    //runThread->start();
    //Q_ASSERT( service );
    m_commServiceList.append( service );
    //m_commSrvThreadList.append(runThread);
    return 0;
}
//注册 MPI300 协议
int HMICommService::registerConnect( struct Mpi300ConnectCfg& connectCfg )
{
    for ( int i = 0; i < m_commServiceList.count(); i++ )
    {
        CommService* service = m_commServiceList[i];
        if ( service->meetServiceCfg( SERVICE_TYPE_MPI300, connectCfg.COM_interface ) )
        {
            (( Mpi300Service* )service )->addConnect( connectCfg );
            return 0;
        }
    }
    Mpi300Service* service = new Mpi300Service( connectCfg );
    QTimer::singleShot(1, service, SLOT(slot_run()));
    //QThread *runThread = new QThread;
    //QObject::connect(runThread, SIGNAL(started()),service,SLOT(slot_run()));
    //service->moveToThread(runThread);
    //runThread->start();
    //Q_ASSERT( service );
    //m_commServiceList.append( service );
    //m_commSrvThreadList.append(runThread);
    return 0;
}
//注册 freePort 协议
int HMICommService::registerConnect( struct fpConnectCfg& connectCfg )
{
    for ( int i = 0; i < m_commServiceList.count(); i++ )
    {
        CommService* service = m_commServiceList[i];
        if ( service->meetServiceCfg( SERVICE_TYPE_FP, connectCfg.COM_interface ) )
        {
            (( fpService* )service )->addConnect( connectCfg );
            return 0;
        }
    }
    fpService* service = new fpService( connectCfg );
    QTimer::singleShot(1, service, SLOT(slot_run()));
    //QThread *runThread = new QThread;
    //QObject::connect(runThread, SIGNAL(started()),service,SLOT(slot_run()));
    //service->moveToThread(runThread);
    //runThread->start();
    //Q_ASSERT( service );
    m_commServiceList.append( service );
    //m_commSrvThreadList.append(runThread);
    return 0;
}

//注册 TCP MODBUS 协议
int HMICommService::registerConnect( struct TcpModbusConnectCfg& connectCfg )
{
    for ( int i = 0; i < m_commServiceList.count(); i++ )
    {
        CommService* service = m_commServiceList[i];
        if ( service->meetServiceCfg( SERVICE_TYPE_TCPMBUS, 0 ) )
        {
            (( TcpModbusService* )service )->addConnect( connectCfg );
            return 0;
        }
    }
    TcpModbusService* service = new TcpModbusService( connectCfg );
    QTimer::singleShot(1, service, SLOT(slot_run()));
    //QThread *runThread = new QThread;
    //QObject::connect(runThread, SIGNAL(started()),service,SLOT(slot_run()));
    //service->moveToThread(runThread);
    //runThread->start();
    //Q_ASSERT( service );
    m_commServiceList.append( service );
    //m_commSrvThreadList.append(runThread);
    return 0;
}

//注册合信网关连接
int HMICommService::registerConnect( struct GateWayConnectCfg& connectCfg )//注册合信网关连接
{
    for ( int i = 0; i < m_commServiceList.count(); i++ )
    {
        CommService* service = m_commServiceList[i];
        if ( service->meetServiceCfg( SERVICE_TYPE_GATEWAY, 0 ) )
        {
            (( GateWayService* )service )->addConnect( connectCfg );
            return 0;
        }
    }
    GateWayService* service = new GateWayService( connectCfg );   
    //QThread *runThread = new QThread;
    QTimer::singleShot(1, service, SLOT(slot_run()));
    //QObject::connect(runThread, SIGNAL(started()),service,SLOT(slot_run()));
    //service->moveToThread(runThread);
    //runThread->start();
    Q_ASSERT( service );
    m_commServiceList.append( service );
    //m_commSrvThreadList.append(runThread);
    return 0;
}


int HMICommService::registerVar( struct HmiVar& hmiVar )  //超长变量要拆分,暂时没有处理,第一次激活变量时由其所在连接进行拆分
{
    Q_ASSERT( hmiVar.varId >= 0 && hmiVar.varId < 65535 );
    if ( !m_hmiVarList.contains( hmiVar.varId ) ) //检查是否重复ID
    {
        struct HmiVar* pHmiVar = new HmiVar;
        Q_ASSERT( pHmiVar );
        pHmiVar->varId = hmiVar.varId;
        pHmiVar->connectId = hmiVar.connectId;
        pHmiVar->cycleTime = hmiVar.cycleTime;
        pHmiVar->area = hmiVar.area;
        pHmiVar->subArea = hmiVar.subArea;
        pHmiVar->addOffset = hmiVar.addOffset;
        pHmiVar->bitOffset = hmiVar.bitOffset;
        pHmiVar->len = hmiVar.len;
        pHmiVar->dataType = hmiVar.dataType;

        pHmiVar->err = VAR_NO_OPERATION; //没有数据，初始状态为没有初始化
        pHmiVar->wStatus = VAR_NO_ERR;
        pHmiVar->writeCnt = 0;

        m_hmiVarList.insert( hmiVar.varId, pHmiVar );

        for ( int i = 0; i < m_commServiceList.count(); i++ )
        {
            CommService* service = m_commServiceList[i];
            if ( 0 == service->splitLongVar( pHmiVar ) )
                return 0;
        }
        //Q_ASSERT( false );
    }
    else
        Q_ASSERT( false );

    return 0;
}

int HMICommService::startUpdateVar( int varId )
{
    struct HmiVar* pHmiVar = m_hmiVarList.value( varId );
    if ( pHmiVar != NULL )
    {
        for ( int i = 0; i < m_commServiceList.count(); i++ )
        {
            CommService* service = m_commServiceList[i];
            if ( 0 == service->startUpdateVar( pHmiVar ) )
                return 0;
        }
        //Q_ASSERT( false );                    //没有这个连接
        return 0;
    }
    else
    {
        Q_ASSERT( false );
        return -1;
    }
}

int HMICommService::stopUpdateVar( int varId )
{
    struct HmiVar* pHmiVar = m_hmiVarList.value( varId );
    if ( pHmiVar != NULL )
    {
        pHmiVar->lastUpdateTime = -1;
        for ( int i = 0; i < m_commServiceList.count(); i++ )
        {
            CommService* service = m_commServiceList[i];
            if ( 0 == service->stopUpdateVar( pHmiVar ) )
                return 0;
        }
        Q_ASSERT( false );                    //没有这个连接
        return 0;
    }
    else
    {
        Q_ASSERT( false );
        return -1;
    }
}


int HMICommService::onceRealUpdateVar( int varId )
{
    struct HmiVar* pHmiVar = m_hmiVarList.value( varId );
    if ( pHmiVar != NULL )
    {
        for ( int i = 0; i < m_commServiceList.count(); i++ )
        {
            CommService* service = m_commServiceList[i];
            if ( 0 == service->startUpdateVar( pHmiVar ) )
                return 0;
        }
        Q_ASSERT( false );                    //没有这个连接
        return 0;
    }
    else
    {
        Q_ASSERT( false );
        return -1;
    }
}

int HMICommService::readDevVar( int varId, uchar* pBuf, int bufLen )
{
    struct HmiVar* pHmiVar = m_hmiVarList.value( varId );
    if ( pHmiVar != NULL )
    {
        //if ( pHmiVar->wStatus == VAR_NO_OPERATION )
        //return VAR_NO_OPERATION;
        for ( int i = 0; i < m_commServiceList.count(); i++ )
        {
            CommService* service = m_commServiceList[i];
            int ret = service->readDevVar( pHmiVar, pBuf, bufLen );
            if ( ret >= 0 )
                return ret;
        }
        Q_ASSERT( false );                    //没有这个连接
        return 0;
    }
    else
    {
        Q_ASSERT( false );
        return -1;
    }
}

int HMICommService::writeDevVar( int varId, uchar* pBuf, int bufLen )
{
    struct HmiVar* pHmiVar = m_hmiVarList.value( varId );
    if ( pHmiVar != NULL )
    {
        for ( int i = 0; i < m_commServiceList.count(); i++ )
        {
            CommService* service = m_commServiceList[i];
            int ret = service->writeDevVar( pHmiVar, pBuf, bufLen );
            if ( ret >= 0 )
                return ret;
        }
        Q_ASSERT( false );                    //没有这个连接
        return 0;
    }
    else
    {
        Q_ASSERT( false );
        return -1;
    }
}

int HMICommService::getWriteStatus( int varId )
{
    struct HmiVar* pHmiVar = m_hmiVarList.value( varId );
    Q_ASSERT( pHmiVar != NULL );
    if ( pHmiVar != NULL )
    {
        if ( pHmiVar->splitVars.count() == 0 )
            return pHmiVar->wStatus;
        else
        {
            for ( int j = 0; j < pHmiVar->splitVars.count(); j++ )
            {
                struct HmiVar* pSplitHhiVar = pHmiVar->splitVars[j];
                if ( pSplitHhiVar->wStatus != VAR_NO_ERR )
                    return pSplitHhiVar->wStatus;
            }
            return VAR_NO_ERR;
        }
    }
    else
    {
        Q_ASSERT( false );
        return -1;
    }
}

int HMICommService::getComMsg( QList<struct ComMsg> &msgList ) //获取连接状态
{
    int ret = 0;
    for ( int i = 0; i < m_commServiceList.count(); i++ )
    {
        CommService* service = m_commServiceList[i];
        ret += service->getComMsg( msgList );
    }

    return ret;
}

int HMICommService::getConnLinkStatus( int connectId ) //获取连接状态
{
    for ( int i = 0; i < m_commServiceList.count(); i++ )
    {
        CommService* service = m_commServiceList[i];
        int ret = service->getConnLinkStatus( connectId );
        if ( ret >= 0 )
            return ret;
    }
    Q_ASSERT( false );                    //没有这个连接
    return CONNECT_NO_OPERATION;
}

void ModbusConnectCfg::registerConfig(HMICommService *hc)
{
    hc->registerConnect(*this);
}

void PpiMpiConnectCfg::registerConfig(HMICommService *hc)
{
    hc->registerConnect(*this);
}

void HostlinkConnectCfg::registerConfig(HMICommService *hc)
{
    hc->registerConnect(*this);
}

void FinHostlinkConnectCfg::registerConfig(HMICommService *hc)
{
    hc->registerConnect(*this);
}

void MiProtocol4ConnectCfg::registerConfig(HMICommService *hc)
{
    hc->registerConnect(*this);
}

void MiProfxConnectCfg::registerConfig(HMICommService *hc)
{
    hc->registerConnect(*this);
}

void MewtocolConnectCfg::registerConfig(HMICommService *hc)
{
    hc->registerConnect(*this);
}

void DvpConnectCfg::registerConfig(HMICommService *hc)
{
    hc->registerConnect(*this);
}

void FatekConnectCfg::registerConfig(HMICommService *hc)
{
    hc->registerConnect(*this);
}

void CnetConnectCfg::registerConfig(HMICommService *hc)
{
    hc->registerConnect(*this);
}

void Mpi300ConnectCfg::registerConfig(HMICommService *hc)
{
    hc->registerConnect(*this);
}

void fpConnectCfg::registerConfig(HMICommService *hc)
{
    hc->registerConnect(*this);
}

void TcpModbusConnectCfg::registerConfig(HMICommService *hc)
{
    hc->registerConnect(*this);
}

void GateWayConnectCfg::registerConfig(HMICommService *hc)
{
    hc->registerConnect(*this);
}
