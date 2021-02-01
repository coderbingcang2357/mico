#include "hmiproject.h"
#include "util2/datadecode.h"
#include "serverudp/isock.h"
#include "hmicom/commInterface.h"
#include "projectrunner.h"
#include "util/logwriter.h"
#include <config/configreader.h>
#include <thread>

HMI::HmiProject::HmiProject()
{

}

HMI::HmiProject::~HmiProject(){
	::writelog(InfoType, "stopAllconn");
	m_hmiservice.stopAllConn();
	ISock::del();
}
bool HMI::HmiProject::fromByte(char *buf, int len)
{
    if (//!Util2::readUint64(&buf, &len, &sceneid)
        //|| !Util2::readString(&buf, &len, &phone)
           !Alarm::readAlarmList(&buf, &len, &alarm)
        || !Connection::readConnList(&buf, &len, &connection)
        || !Variable::readVar(&buf, &len, &var))
    {

        return false;
    }
    else
    {
        return true;
    }
}

void HMI::HmiProject::run()
{
    ::writelog(InfoType, "runscene sceneid: %lu", sceneid);
#if 0
	if(sceneid != 288230376151930161){
		return ;
	}
#endif
    //
    for (auto &conn : connection)
    {
		//std::shared_ptr<ServerUdp> m_udp = std::make_shared<ServerUdp>(conn.deviceId, 100000, Configure::getConfig()->localip);
		m_udp = new ServerUdp(conn.deviceId, 100000, Configure::getConfig()->localip);
        m_udp->bind();

        // save m_udp to global, we will get it on rwsocket and udpsocket
        ISock::set(conn.connid, m_udp);
		::writelog(InfoType,"conn.connid:%d",conn.connid);
		::writelog(InfoType,"m_udp:%p",m_udp);
        conn.protocolPars->registerConfig(&m_hmiservice);
    }

    for (auto &v : var)
    {
        HmiVar hv;
        hv.varId = v.varId;		             //变量的唯一标识码
        hv.connectId = v.connectId;          //对应那个连接
		::writelog(InfoType,"hv.varId:%d",hv.varId);
		::writelog(InfoType,"hv.connectId:%d",hv.connectId);
        hv.cycleTime = v.cycleTime;          //变量的刷新周期，单位毫秒 (最大周期值24小时)
        hv.area = v.area;               	//变量所在的区域
        hv.subArea = v.subArea;            	//子区域 300 协议 DB 用到
        hv.addOffset = v.addOffset;          //数据在PLC中的地址偏移量
        hv.bitOffset = v.bitOffset;          //如果需要位偏移量，那么这个位变量的偏移量，否则写-1
        hv.len = v.len;               		//变量的数据长度，字节个数, 位变量为位的个数
        //hv.dataType = 0x02;         		//变量的类型
        hv.dataType = v.dataType;         //变量的类型
		//::writelog(InfoType,"hv.dataType:%02x",hv.dataType);
        m_hmiservice.registerVar(hv);
        m_hmiservice.startUpdateVar(hv.varId);
    }
    m_alamrunner = std::make_shared<ProjectRunner>(this);
    m_alamrunner->init();
    m_alamrunner->start();
}
