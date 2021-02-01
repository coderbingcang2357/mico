#include "connection.h"
#include "util/logwriter.h"

HMI::Connection::Connection()
{

}

HMI::Connection::~Connection()
{
    ::writelog(InfoType, "delconn");
    //if (protocolPars)
    //{
    //    delete protocolPars;
    //    protocolPars = nullptr;
    //}
}

bool HMI::Connection::readConnList(char **buf, int *len, QList<HMI::Connection> *connection)
{
    quint16 size = 0;
    if (!Util2::readUint16(buf, len , &size))
        return false;

    for (int i = 0;i < size; ++i)
    {
        Connection cn;
        if (!Util2::readUint64(buf, len, &cn.deviceId)
        || !Util2::readUint8(buf, len, &cn.driverType))
        {
            return false;
        }
        std::shared_ptr<ProtocolPars> pp = std::shared_ptr<ProtocolPars>(getPar(cn.driverType));
        if (!pp)
            return false;
		int type = cn.driverType;
		::writelog(InfoType,"cn.deviceId:%llu",cn.deviceId);
		::writelog(InfoType,"cn.driverType:%d",cn.driverType);
		// 离线报警暂只支持gateway ppimpi modbus这3中协议
		//if(type != 0 ){
		if(type != 0 && type != 1 && type != 2 && type != 4){
			::writelog(InfoType, "protocol error!");
			return false;
		}

        if (!pp->fromData(buf, len))
        {
            return false;
		}
		//::writelog(InfoType,"connid:%d",(std::dynamic_pointer_cast<GateWayConnectCfg>(pp))->id);
		//cn.protocolPars = pp;
		SetConnid(cn,pp);
		//cn.connid = i;
#if 0
		if(type == 0){
			cn.protocolPars = pp;
			//SetConnid(cn,pp);
			cn.connid = i;
			//SetConnid(cn,pp);

			//cn.connid = tmp->id;
		}
		else{
			SetConnid(cn,pp);
			//cn.protocolPars = pp;
			//cn.connid = i;
		}
#endif
		//cn.connid = (std::dynamic_pointer_cast<GateWayConnectCfg>(pp))->id;
		::writelog(InfoType,"connid11111:%d\n",cn.connid);
        connection->push_back(cn);
    }
    return true;
}

void HMI::Connection::SetConnid(Connection &cn, std::shared_ptr<ProtocolPars> pp){
	cn.protocolPars = pp;
	switch(cn.driverType){
		case 0:
			{
				//GateWayConnectCfg *tmp = dynamic_cast<GateWayConnectCfg *>(pp.get());
				//cn.connid = tmp->id;
			}
			cn.connid = (dynamic_cast<GateWayConnectCfg *>(pp.get()))->id;
			break;
		case 1:
			cn.connid = (dynamic_cast<PpiMpiConnectCfg *>(pp.get()))->id;
			break;
		case 2:
			cn.connid = (dynamic_cast<PpiMpiConnectCfg *>(pp.get()))->id;
			break;
		case 4:
			cn.connid = (dynamic_cast<ModbusConnectCfg *>(pp.get()))->id;
			break;
		default:
			printf("暂不支持除udpppi,modbus,mpippi协议外的其他协议aaaaaa\n");
			//Q_ASSERT(0);
	}
}

ProtocolPars *HMI::Connection::getPar(quint8 dt)
{
    switch (dt) {
    case 0:
        return new GateWayConnectCfg();
    case 1:
        return new PpiMpiConnectCfg();
    case 2:
        return new PpiMpiConnectCfg();
    case 3:
        return new Mpi300ConnectCfg();
    case 4:
        return new ModbusConnectCfg();
    case 5:
        return new TcpModbusConnectCfg();
    case 6:
        return new FinHostlinkConnectCfg();
    case 7:
        return new MiProtocol4ConnectCfg();
    case 8:
        return new MiProfxConnectCfg();
    case 9:
        return new FinHostlinkConnectCfg();
    case 10:
        return new MewtocolConnectCfg();
    case 11:
        return new DvpConnectCfg();
    case 12:
        return new FatekConnectCfg();
    case 13:
        return new CnetConnectCfg();
    case 14:
        return new fpConnectCfg();
    default:
        return nullptr;
    }
    return nullptr;
}
