#include "runsceneatserverprocess.h"
#include "Message/Message.h"
#include "useroperator.h"
#include "util/util.h"
#include "util/logwriter.h"
#include "messageencrypt.h"
#include "newmessagecommand.h"
#include "onlineinfo/mainthreadmsgqueue.h"
#include "Message/Notification.h"

void IsRunSceneAtServiceProcess::test()
{
    InternalMessage *fmsg = new InternalMessage(new Message);
    fmsg->message()->setConnectionId(0);
    fmsg->message()->setConnectionServerid(0);

    //fmsg->message()->setSockerAddress(msg->clientaddr, msg->addrlen);
    fmsg->message()->setObjectID(1111);
    fmsg->message()->setCommandID(CMD::CMD_HMI_IS_SCENE_RUN_AT_SERVER);
    fmsg->message()->appendContent(uint64_t(72057594037945455));
    NewMessageCommand *ic = new NewMessageCommand(fmsg);
    ::MainQueue::postCommand(ic);
}

IsRunSceneAtServiceProcess::IsRunSceneAtServiceProcess(ICache *c, SceneRunnerManager *s, RunningSceneDbaccess *rs)
    : m_cache(c), srm(s), m_rsdbaccess(rs)
{
}

int IsRunSceneAtServiceProcess::processMesage(Message *reqmsg, std::list<InternalMessage *> *r)
{
    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
    msg->setDestID(reqmsg->objectID());
    InternalMessage *imsg = new InternalMessage(msg);
    r->push_back(imsg);

    if(::decryptMsg(reqmsg, m_cache) != 0)
    {
        // 解密失败
        msg->appendContent(&ANSC::MESSAGE_ERROR, (Message::SIZE_ANSWER_CODE));
		::writelog(InfoType, "decryptMsg(reqmsg, m_cache) failed");
        return r->size();
    }
    char *c = reqmsg->content();
    uint32_t len = reqmsg->contentLen();
    if (len < 8)
    {
        // message error
        ::writelog(InfoType, "remove my device error len");
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }
    uint64_t sceneid = 0;
    sceneid = ::ReadUint64(c);
    int s = 0;
    //int s = srm->isRunning(sceneid);
    RunningSceneData rd;
	::writelog(InfoType, "get run scene msg %llu", rd.runningserverid);
    m_rsdbaccess->getRunningScene(sceneid, &rd);
    if (rd.valid() && rd.runningserverid != 0)
    {
        //
        s = 1;
    }
    else
    {
        s = 0;
    }
    msg->appendContent(ANSC::SUCCESS);
    msg->appendContent(sceneid);
    msg->appendContent(uint8_t(s));
    //LSH_2020-10-12 add alarm info
    msg->appendContent(uint8_t(rd.wecharten));
    msg->appendContent(uint8_t(rd.msgen));
	// 让客户端记住用户上次输入的手机号
	if(rd.wechartphone == ""){
		std::string userphone ; 
		m_rsdbaccess->getUserPhone(sceneid, &userphone);
		::writelog(InfoType,"userphone:%s",userphone.c_str());
		msg->appendContent(userphone);
	}
	else{
		msg->appendContent(rd.wechartphone);
	}
    msg->appendContent(rd.msgphone);
    msg->appendContent(rd.blobscenedata);

    return r->size();
}


//-----------start
StartRunSceneAtServiceProcess::StartRunSceneAtServiceProcess(ICache *c, SceneRunnerManager *s, RunningSceneDbaccess *rs, 
		UserOperator   *u) : m_cache(c), srm(s), m_rsdbaccess(rs), m_useroperator(u) 
{

}

void StartRunSceneAtServiceProcess::test()
{
    InternalMessage *fmsg = new InternalMessage(new Message);
    fmsg->message()->setConnectionId(0);
    fmsg->message()->setConnectionServerid(0);

    //fmsg->message()->setSockerAddress(msg->clientaddr, msg->addrlen);
    fmsg->message()->setObjectID(1111);
    fmsg->message()->setCommandID(CMD::CMD_HMI_START_SCENE_RUN_AT_SERVER);
    fmsg->message()->appendContent(uint64_t(72057594037945455));
    //fmsg->message()->appendContent(uint64_t(72057594037947034));
    fmsg->message()->appendContent("18676784444");
    fmsg->message()->appendContent("data");
    NewMessageCommand *ic = new NewMessageCommand(fmsg);
    ::MainQueue::postCommand(ic);
}


int StartRunSceneAtServiceProcess::processMesage(Message *reqmsg, std::list<InternalMessage *> *r)
{
    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
    msg->setDestID(reqmsg->objectID());
    InternalMessage *imsg = new InternalMessage(msg);
    r->push_back(imsg);

    if(::decryptMsg(reqmsg, m_cache) != 0)
    {
        // 解密失败
        msg->appendContent(&ANSC::MESSAGE_ERROR, (Message::SIZE_ANSWER_CODE));
		::writelog(InfoType, "StartRunSceneAtServiceProcess decryptMsg(reqmsg, m_cache) failed");
        return r->size();
    }
    uint64_t sceneid = 0;
    uint8_t wecharten = 0;
    uint8_t msgen = 0;
    std::string wechartphone;
    std::string msgphone;
    char *c = reqmsg->content();
    uint32_t len = reqmsg->contentLen();
    if (len < 8)
    {
        // message error
        ::writelog(InfoType, "start scene msg error 1");
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }
    sceneid = ::ReadUint64(c);
    c += 8;
    len -= 8;
    wecharten = ::ReadUint8(c);
    c += 1;
    len -= 1;
    msgen = ::ReadUint8(c);
    c += 1;
    len -= 1;
    int wephonelen = readString(c, len, &wechartphone);
    if (wephonelen < 0)
    {
        // message error
        ::writelog(InfoType, "start runscene msg err 2");
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }
    c += wephonelen;
    len -= wephonelen;

    int msgphonelen = readString(c, len, &msgphone);
    if (msgphonelen < 0)
    {
        // message error
        ::writelog(InfoType, "start runscene msg err 2");
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }
    c += msgphonelen;
    len -= msgphonelen;

    // save to db
    RunningSceneData rsd;
    rsd.sceneid = sceneid;
    rsd.wecharten = wecharten;
    rsd.msgen = msgen;
    rsd.wechartphone = wechartphone;
    rsd.msgphone = msgphone;
    rsd.startuserid = reqmsg->objectID();
    rsd.blobscenedata = std::string(c, len);
	if (m_rsdbaccess->insertRunningScene(rsd) != 0) {
		//
		::writelog(InfoType, "start runscene insert to db failed %llu.", sceneid);
		msg->appendContent(ANSC::FAILED);
		return r->size();
	}
	// 用户注册手机号
	m_rsdbaccess->updatePhone(sceneid, wechartphone);

    int s = srm->startRunning(sceneid);
	if(s == -1){
        ::writelog(InfoType, "start runscene failed rpc return failed %llu", sceneid);
		if (m_rsdbaccess->removeRunningScene(sceneid) != 0)
		{
			::writelog(InfoType, "remove from db failed %llu", sceneid);
		}
		return r->size();
	}
    msg->appendContent(ANSC::SUCCESS);
#if 0
	uint64_t userid,targetuser;
	if(0 != m_rsdbaccess->GetUserIdBySceneId(sceneid,userid)){
		::writelog(InfoType,"GetUserIdBySceneId failed");
		return r->size();
	}
	targetuser = userid;
	//::writelog(InfoType,"222222222222333444:%llu",userid);
	// generate notify ...

	generateNotify(userid,
			targetuser,
#endif
	generateNotify(sceneid,
			CMD::CMD_HMI_START_SCENE_RUN_AT_SERVER,
			r);
	return r->size();
}

void StartRunSceneAtServiceProcess::generateNotify(uint64_t sceneid,
		uint16_t notifytype,
		std::list<InternalMessage *> *r){
		std::vector<char> notifybuf;
	// 场景所有者ID和account
#if 0
	std::string account;
	if (useroperator->getUserAccount(userid, &account) != 0)
	{   
		::writelog(InfoType, "share scene gen notify: getuser account failed: %lu", userid);
		return;
	}
#endif
	// ID
	//::WriteUint64(&notifybuf, userid);
	// 帐号
	//char *p = (char *)account.c_str();
	//notifybuf.insert(notifybuf.end(), p, p + account.size() + 1); // 加1是因以字符串要以0结尾

	std::vector<std::pair<uint64_t,uint8_t>> useridInfo;
	if(0 != m_rsdbaccess->GetAllUserIDOfScene(std::to_string(sceneid),useridInfo)){
		::writelog(InfoType,"GetAllUserIDOfScene failed");
		return;
	}

	if(useridInfo.size() == 0){
		::writelog(InfoType,"no useridInfo");
		return;
	}
	::writelog(InfoType,"useridInfo:%d",useridInfo.size());
	for(int i = 0; i < useridInfo.size();++i){
		std::string account;
		uint64_t userid = useridInfo[i].first;
		//auth = useridInfo[i].second;
		if (m_useroperator->getUserAccount(userid, &account) != 0)
		{
			::writelog(InfoType, "share scene gen notify: get user account failed: %lu", userid);
			return;
		}

		// 场景ID和场景名
		::WriteUint64(&notifybuf, sceneid);
		// 场景名
		std::string scenename;
		if (m_useroperator->getSceneName(sceneid, &scenename) != 0)
		{
			::writelog(InfoType, "Share scene get scene name failed: %lu", sceneid);
			return;
		}
		::WriteString(&notifybuf, scenename);
		//::WriteUint8(&notifybuf, auth);

		Notification notify;
		notify.Init(userid,
				ANSC::SUCCESS,
				notifytype,
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
}

// stop
StopRunSceneAtServiceProcess::StopRunSceneAtServiceProcess(ICache *c,SceneRunnerManager *s, RunningSceneDbaccess *rs,
		UserOperator *u) : m_cache(c), srm(s), m_rsdbaccess(rs), m_useroperator(u)
{
}

void StopRunSceneAtServiceProcess::test()
{
    InternalMessage *fmsg = new InternalMessage(new Message);
    fmsg->message()->setConnectionId(0);
    fmsg->message()->setConnectionServerid(0);

    //fmsg->message()->setSockerAddress(msg->clientaddr, msg->addrlen);
    fmsg->message()->setObjectID(1111);
    fmsg->message()->setCommandID(CMD::CMD_HMI_STOP_SCENE_RUN_AT_SERVER);
    fmsg->message()->appendContent(uint64_t(72057594037945455));
    //fmsg->message()->appendContent(uint64_t(72057594037947034));
    //fmsg->message()->appendContent("18676784444");
    //fmsg->message()->appendContent("data");
    NewMessageCommand *ic = new NewMessageCommand(fmsg);
    ::MainQueue::postCommand(ic);

}

int StopRunSceneAtServiceProcess::processMesage(Message *reqmsg, std::list<InternalMessage *> *r)
{
    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
    msg->setDestID(reqmsg->objectID());
    InternalMessage *imsg = new InternalMessage(msg);
    r->push_back(imsg);

    if(::decryptMsg(reqmsg, m_cache) != 0)
    {
        // 解密失败
        msg->appendContent(&ANSC::MESSAGE_ERROR, (Message::SIZE_ANSWER_CODE));
        return r->size();
    }
    char *c = reqmsg->content();
    uint32_t len = reqmsg->contentLen();
    if (len < 8)
    {
        // message error
        ::writelog(InfoType, "stop runscene error len");
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }
    uint64_t sceneid = 0;
    sceneid = ::ReadUint64(c);
    int s = srm->stopRunning(sceneid);
    if (s != 0)
    {
        ::writelog(InfoType, "stop runscene failed rpc return failed %llu", sceneid);
        msg->appendContent(ANSC::FAILED);
        return r->size();
    }
    if (m_rsdbaccess->removeRunningScene(sceneid) != 0)
    {
        ::writelog(InfoType, "stop scene failed remove from db %llu", sceneid);
        msg->appendContent(ANSC::FAILED);
        return r->size();
    }

    msg->appendContent(ANSC::SUCCESS);
#if 0
	uint64_t userid,targetuser;
	if(0 != m_rsdbaccess->GetUserIdBySceneId(sceneid,userid)){
		::writelog(InfoType,"GetUserIdBySceneId failed");
		return r->size();
	}

	targetuser = userid;
	// generate notify ...
	generateNotify(userid,
			targetuser,
#endif
	generateNotify(sceneid,
			CMD::CMD_HMI_STOP_SCENE_RUN_AT_SERVER,
			r);

    return r->size();
}

void StopRunSceneAtServiceProcess::generateNotify(uint64_t sceneid,
		uint16_t notifytype,
		std::list<InternalMessage *> *r){
		std::vector<char> notifybuf;
	// 场景所有者ID和account
#if 0
	std::string account;
	if (useroperator->getUserAccount(userid, &account) != 0)
	{   
		::writelog(InfoType, "share scene gen notify: getuser account failed: %lu", userid);
		return;
	}
#endif
	// ID
	//::WriteUint64(&notifybuf, userid);
	// 帐号
	//char *p = (char *)account.c_str();
	//notifybuf.insert(notifybuf.end(), p, p + account.size() + 1); // 加1是因以字符串要以0结尾

	// 场景ID和场景名
	
	std::vector<std::pair<uint64_t,uint8_t>> useridInfo;
	if(0 != m_rsdbaccess->GetAllUserIDOfScene(std::to_string(sceneid),useridInfo)){
		::writelog(InfoType,"GetAllUserIDOfScene failed");
		return ;
	}

	if(useridInfo.size() == 0){
		::writelog(InfoType,"no useridInfo");
		return ;
	}
	::writelog(InfoType,"useridInfo:%d",useridInfo.size());
	for(int i = 0; i < useridInfo.size();++i){
		std::string account;
		uint64_t userid = useridInfo[i].first;
		//auth = useridInfo[i].second;
		if (m_useroperator->getUserAccount(userid, &account) != 0)
		{
			::writelog(InfoType, "share scene gen notify: get user account failed: %lu", userid);
			return ;
		}


		::WriteUint64(&notifybuf, sceneid);
		// 场景名
		std::string scenename;
		if (m_useroperator->getSceneName(sceneid, &scenename) != 0)
		{
			::writelog(InfoType, "Share scene get scene name failed: %lu", sceneid);
			return;
		}
		::WriteString(&notifybuf, scenename);
		//::WriteUint8(&notifybuf, auth);

		Notification notify;
		notify.Init(userid,
				ANSC::SUCCESS,
				notifytype,
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
}
