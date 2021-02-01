#include <assert.h>
#include "sceneconnectordevices.h"
#include "messageencrypt.h"
#include "Message/Message.h"
#include "util/util.h"
#include "util/logwriter.h"
#include "isceneConnectDevicesOperator.h"
#include "protocoldef/protocolversion.h"

SceneConnectorDeviceMessageProcessor::SceneConnectorDeviceMessageProcessor(
    ICache *c, ISceneConnectDevicesOperator *isc, RunningSceneDbaccess *dba)
    : m_cache(c), m_sceneoperator(isc) ,m_rsdbaccess(dba) 
{
}

SceneConnectorDeviceMessageProcessor::~SceneConnectorDeviceMessageProcessor()
{
}

int SceneConnectorDeviceMessageProcessor::processMesage(
    Message *msg,
    std::list<InternalMessage *> *r)
{
    if (decryptMsg(msg, m_cache) != 0)
    {
        ::writelog("set scene connector descypt failed.");
        Message *resmsg = Message::createClientMessage();
        resmsg->CopyHeader(msg);
        resmsg->setObjectID(0);
        resmsg->setDestID(msg->objectID());
        resmsg->appendContent(ANSC::MESSAGE_ERROR);
        InternalMessage *imsg = new InternalMessage(resmsg);
        imsg->setEncrypt(true);
        r->push_back(imsg);
        return r->size();
    }
    switch (msg->commandID())
    {
    case CMD::CMD_SET_SCENE_CONNECTOR_DEVICES:
        return setConnectDevices(msg, r);
        break;

    case CMD::CMD_GET_SCENE_CONNECTOR_DEVICES:
        return getConnectDevices(msg, r);
        break;

    default:
        assert(false);
        return 0;
    }
}

int SceneConnectorDeviceMessageProcessor::setConnectDevices(
    Message *msg,
    std::list<InternalMessage *> *r)
{
    Message *resmsg = Message::createClientMessage();
    resmsg->CopyHeader(msg);
    resmsg->setObjectID(0); // server id
    resmsg->setDestID(msg->objectID());
    InternalMessage *imsg = new InternalMessage(resmsg);
    imsg->setEncrypt(true);
    r->push_back(imsg);

    char *p = msg->content();
    uint32_t plen = msg->contentLen();
    uint64_t userid = msg->objectID();
    uint64_t sceneid = 0;
    uint16_t connectorcount = 0;

    if (plen < sizeof(sceneid) + sizeof(connectorcount))
    {
        resmsg->appendContent(ANSC::MESSAGE_ERROR);
        ::writelog("set scene connector msg error.");
        return r->size();
    }

    sceneid = ReadUint64(p);
    p += sizeof(sceneid);
    plen -= sizeof(sceneid);
	
#if 1
	// can't change device when scene with alarm
	RunningSceneData rd;
	::writelog(InfoType, "get run scene msg222 %llu", rd.runningserverid);
    //RunningSceneDbaccess m_rsdbaccess(m_useroperator->getmysqlconnpool());
	m_rsdbaccess->getRunningScene(sceneid, &rd);
    if (rd.valid() && rd.runningserverid != 0)
    {
        ::writelog("change device when scene with alarm error222");
        resmsg->appendContent(ANSC::CHANGE_DEVICE_SCENE_WITH_ALARM);
        //msg->appendContent(&ANSC::CHANGE_DEVICE_SCENE_WITH_ALARM, Message::SIZE_ANSWER_CODE);
        return r->size();
    }
#endif

    connectorcount = ::ReadUint16(p);
    p += sizeof(connectorcount);
    plen -= sizeof(connectorcount);

    uint16_t protoversion = ::getProtoVersion(msg->versionNumber());
    int sizeofid = 1;
    if (protoversion >= 0x201)
    {
        sizeofid = 2;
    }

    if (plen != (connectorcount * (sizeofid + sizeof(uint64_t))))
    {
        ::writelog("set scene connector msg length error.");
        resmsg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    std::map<uint16_t, std::vector<uint64_t> > connector;
    for (uint16_t i = 0; i < connectorcount; ++i)
    {
        uint16_t connectid;
        if (protoversion >= 0x201)
            connectid = ReadUint16(p);
        else
            connectid = ReadUint8(p);
        p += sizeofid;
        uint64_t deviceid = ReadUint64(p);
        p += sizeof(deviceid);
        connector[connectid].push_back(deviceid);
    }
	
	int res = m_sceneoperator->setSceneConnectDevices(userid, sceneid, connector);
    if (res != 0)
    {
        resmsg->appendContent(uint8_t(res));
        ::writelog("call set scene connect devices failed.");
    }
    else
    {
        resmsg->appendContent(ANSC::SUCCESS);
    }
    return r->size();
}

int SceneConnectorDeviceMessageProcessor::getConnectDevices(
    Message *msg,
    std::list<InternalMessage *> *r)
{
    Message *resmsg = Message::createClientMessage();
    resmsg->CopyHeader(msg);
    resmsg->setObjectID(0); // should set to serverid
    resmsg->setDestID(msg->objectID());

    InternalMessage *imsg = new InternalMessage(resmsg);
    imsg->setEncrypt(true);
    r->push_back(imsg);

    char *p = msg->content();
    uint32_t plen = msg->contentLen();

    // uint64_t userid = msg->ObjectID();
    uint64_t sceneid = 0;

    if (plen != sizeof(sceneid))
    {
        ::writelog("get scene connect device msg size error.");
        resmsg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }
    sceneid = ::ReadUint64(p);

    uint64_t sceneownerid;
    std::map<uint16_t,  std::vector<uint64_t> > connect;
    int res = m_sceneoperator->getSceneConnectDevices(sceneid, &sceneownerid, &connect);
    if (res != 0)
    {
        resmsg->appendContent(uint8_t(res));
        return r->size();
    }

    resmsg->appendContent(ANSC::SUCCESS);
    resmsg->appendContent(sceneownerid);
    resmsg->appendContent(sceneid);

    // 把map转换成pair好方便取总数
    std::list< std::pair<uint16_t, uint64_t> > conndevpair;
    for (auto conn : connect)
    {
        auto connid = conn.first;

        for (auto id : conn.second)
        {
            conndevpair.push_back(std::make_pair(connid, id));
        }
    }

    // 取总数
    resmsg->appendContent(uint16_t(conndevpair.size()));

    uint16_t protoversion = ::getProtoVersion(msg->versionNumber());
    for (auto p : conndevpair)
    {
        if (protoversion >= 0x201)
        {
            resmsg->appendContent(p.first);
        }
        else
        {
            resmsg->appendContent(uint8_t(p.first));
        }
        resmsg->appendContent(p.second);
    }

    return r->size();
}

