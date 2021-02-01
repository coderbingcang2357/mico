#include "rabbitmqmessgeprocess.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/writer.h"
#include "rapidjson/document.h"
#include "util/util.h"
#include "Message/Message.h"
#include "util/logwriter.h"
#include <stdio.h>
#include "Message/Notification.h"
#include "useroperator.h"

using namespace rapidjson;

RabbitmqMessageProcess::RabbitmqMessageProcess()
{
}

int RabbitmqMessageProcess::processMesage(Message *msg,
                                          std::list<InternalMessage *> *r)
{
    //
    Document doc;
    doc.Parse(msg->content());
    assert(doc.IsObject());
    if (!doc.IsObject())
        return 0;

    uint64_t randomnumber = 0;
    uint64_t deviceid = 0;
    uint64_t userid = 0;
    std::string exip;
    std::string deviceip;
    uint16_t port = 0;

    ::writelog(InfoType, "rabbmtmq: %s", msg->content());
    bool valid = doc.HasMember("rd")
            && doc.HasMember("did")
            && doc.HasMember("uid")
            && doc.HasMember("eip")
            && doc.HasMember("port")
            && doc.HasMember("deviceip");
    if (!valid)
        return 0;

    randomnumber = doc["rd"].GetUint64();
    deviceid = doc["did"].GetUint64();
    userid = doc["uid"].GetUint64();
    exip = doc["eip"].GetString();
    deviceip = doc["deviceip"].GetString();
    port = doc["port"].GetUint();

    //::writelog(InfoType, "mq portttttttttttttttttttttttttt : %d", (int)port);
    //::writelog(InfoType, "deviceip : %s", deviceip.c_str());

    //shared_ptr<DeviceData> deviceData
    //    = static_pointer_cast<DeviceData>(m_cache->getData(rd.devicdid));
    //if (!deviceData)
    //{
    //    deviceData = static_pointer_cast<DeviceData>(RedisCache::instance()->getData(rd.devicdid));
    //}

    sockaddrwrap ip;
    ip.fromString(exip);
    uint8_t  prefix = 0xFF;
    uint8_t  opcode = 0xB0;

    Message *deviceMsg = Message::createDeviceMessage();
    sockaddrwrap deviceSockAddr;
    deviceSockAddr.fromString(deviceip);
    deviceMsg->setSockerAddress(deviceSockAddr);
    deviceMsg->setCommandID(CMD::DCMD__OLD_FUNCTION);
    deviceMsg->setObjectID(0);
    deviceMsg->setDestID(deviceid);
    deviceMsg->appendContent(&prefix, sizeof(prefix));
    deviceMsg->appendContent(&opcode, sizeof(opcode));
    deviceMsg->appendContent(ip.ipv4());
    deviceMsg->appendContent(htons(port)); // 要进行大小尾转换
    //deviceMsg->appendContent(userid);
    deviceMsg->appendContent(randomnumber);

	r->push_back(new InternalMessage(deviceMsg));

    return r->size();
}

#if 1
RabbitmqMessageAlarmNotifyProcess::RabbitmqMessageAlarmNotifyProcess(IMysqlConnPool *pool):m_mysqlpool(pool)
{
}

int RabbitmqMessageAlarmNotifyProcess::processMesage(Message *msg,
                                          std::list<InternalMessage *> *r)
{
    Document doc;
    doc.Parse(msg->content());
    assert(doc.IsObject());
    if (!doc.IsObject())
        return 0;

	::writelog(InfoType, "rabbmtmq_notify: %s", msg->content());
    bool valid = doc.HasMember("id")
            && doc.HasMember("alarmId")
            && doc.HasMember("alarmText")
            && doc.HasMember("alarmTime")
            && doc.HasMember("alarmEventType");
    if (!valid)
        return 0;

	uint64_t sceneid;
	uint16_t alarmid;
	std::string alarmText;
	uint64_t alarmTime;
	uint8_t alarmEventType;

    sceneid = doc["id"].GetUint64();
    alarmid = (uint16_t)doc["alarmId"].GetUint();
    alarmText = doc["alarmText"].GetString();
    alarmTime = doc["alarmTime"].GetUint64();
    alarmEventType = (uint8_t)doc["alarmEventType"].GetUint();

	std::vector<std::pair<uint64_t,uint8_t>> useridInfo;
	RunningSceneDbaccess rundb(m_mysqlpool);
	if(0 != rundb.GetAllUserIDOfScene(std::to_string(sceneid),useridInfo)){
		::writelog(InfoType,"GetAllUserIDOfScene failed");
		return -1;
	}
#if 0
	::writelog(InfoType,"sceneid:%lu",sceneid);
	::writelog(InfoType,"alarmid:%lu",alarmid);
	::writelog(InfoType,"sceneid:%s",alarmText.c_str());
	::writelog(InfoType,"sceneid:%lu",alarmTime);
	::writelog(InfoType,"sceneid:%lu",alarmEventType);
#endif
	

    std::vector<char> notifybuf;
	uint64_t userid;
	uint8_t auth;
	UserOperator useroperator(m_mysqlpool);
	
	if(useridInfo.size() == 0){
		::writelog(InfoType,"no useridInfo");
		return -1;
	}
	::writelog(InfoType,"useridInfo:%d",useridInfo.size());
	for(int i = 0; i < useridInfo.size();++i){
		// 场景所有者ID和account
		std::string account;
		userid = useridInfo[i].first;
		//auth = useridInfo[i].second;
		if (useroperator.getUserAccount(userid, &account) != 0)
		{
			::writelog(InfoType, "share scene gen notify: get user account failed: %lu", userid);
			return -1;
		}
		// ID
		//::WriteUint64(&notifybuf, userid);
		// 帐号
		//char *p = (char *)account.c_str();
		//notifybuf.insert(notifybuf.end(), p, p + account.size() + 1); // 加1是因以字符串要以0结尾

		// 场景ID和场景名
		::WriteUint64(&notifybuf, sceneid);
		::WriteUint16(&notifybuf, alarmid);
		::WriteString(&notifybuf, alarmText);
		::WriteUint64(&notifybuf, alarmTime);
		::WriteUint8(&notifybuf, alarmEventType);
		// 场景名
		std::string scenename;
		if (useroperator.getSceneName(sceneid, &scenename) != 0)
		{
			::writelog(InfoType, "Share scene get scene name failed: %lu", sceneid);
			return -1;
		}
		::WriteString(&notifybuf, scenename);
		//::WriteUint8(&notifybuf, auth);

		Notification notify;
		
		notify.Init(userid,
				ANSC::SUCCESS,
				CMD::CMD_NEW_RABBITMQ_MESSAGE_ALARM_NOTIFY,
				0,
				notifybuf.size(),
				&notifybuf[0]);

		// 生成通知发到通知处理进程
		Message *msg = new Message;
		msg->setCommandID(CMD::CMD_NEW_NOTIFY);
		msg->setObjectID(userid); // 通知接收者的ID
		msg->appendContent(&notify);
		InternalMessage *imsg = new InternalMessage(msg);
		imsg->setEncrypt(false);
		imsg->setType(InternalMessage::ToNotify); // 表示这个消息是要发到通知进程的
		r->push_back(imsg);
	}

	return r->size();
}
#endif
