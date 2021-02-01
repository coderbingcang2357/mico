#ifndef GETMYHMISCENELIST__H
#define GETMYHMISCENELIST__H

#include "onlineinfo/icache.h"
#include "imessageprocessor.h"
#include "useroperator.h"
#include "timer/clocktimer.h"
#include "servermonitormaster/dbaccess/runningscenedata.h"
#include "servermonitormaster/dbaccess/runningscenedb.h"
class Timer;
class ISceneDal;
class IDelSceneUploadLog;
class MicoServer;

class GetMyHmiSenceListProcessor : public IMessageProcessor
{
public:
    GetMyHmiSenceListProcessor(ICache *, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    ICache *m_cache;
    UserOperator *m_useroperator;
};

class ModifySceneInfo : public IMessageProcessor
{
public:
    ModifySceneInfo(ICache *c, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    ICache *m_cache;
    UserOperator *m_useroperator;
};

class ModifyNotename : public IMessageProcessor
{
public:
    ModifyNotename(ICache *c, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    ICache *m_cache;
    UserOperator *m_useroperator;
};

class GetHmiSceneInfo : public IMessageProcessor
{
public:
    GetHmiSceneInfo(ICache *, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    ICache *m_cache;
    UserOperator *m_useroperator;
};

class GetHmiSenceShareUsers : public IMessageProcessor
{
public:
    GetHmiSenceShareUsers(ICache *, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    ICache *m_cache;
    UserOperator *m_useroperator;
};

// 共享场景
class ShareScene : public IMessageProcessor
{
public:
    ShareScene(ICache *, ISceneDal *sd, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

    static void generateNotify(uint64_t userid,
        uint64_t targetuserid,
        uint64_t sceneid,
        uint8_t auth,
        uint16_t notifytype,
        UserOperator *useroperator,
        std::list<InternalMessage *> *r);

private:
    ISceneDal *m_scenedal;
    ICache *m_cache;
    UserOperator *m_useroperator;
};

// 取消共享
class DonotShareScene : public IMessageProcessor
{
public:
    DonotShareScene(ICache *, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    //void generateCancelShareNotify(uint64_t userid,
    //        uint64_t targetuser,
    //        uint64_t sceneid,
    //        std::list<InternalMessage *> *r);

    ICache *m_cache;
    UserOperator *m_useroperator;
};

// 修改共享权限
class SetShareSceneAuth : public IMessageProcessor
{
public:
    SetShareSceneAuth(ICache *c, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    //void genSceneAuthChangeNotify(uint64_t userid, uint64_t targetuser,
    //    uint64_t sceneid, uint8_t auth, std::list<InternalMessage *> *r);

    ICache *m_cache;
    UserOperator *m_useroperator;
};

// 删除场景
class DeleteScene : public IMessageProcessor
{
public:
    DeleteScene(ICache *, UserOperator *uop,RunningSceneDbaccess*);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    void genSceneDelNotify(uint64_t userid,
        uint64_t sceneid,
        const std::string &scenename,
        std::vector<SharedSceneUserInfo> *,
        std::list<InternalMessage *> *r);

    ICache *m_cache;
    UserOperator *m_useroperator;
    RunningSceneDbaccess *m_rsdbaccess;
};

class HmiPenetrate : public IMessageProcessor
{
public:
    HmiPenetrate(ICache *c, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    ICache *m_cache;
    UserOperator *m_useroperator;
};

class HmiRelay : public IMessageProcessor
{
public:
    HmiRelay(ICache *c, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    ICache *m_cache;
    UserOperator *m_useroperator;
};

class HmiOpenRelayResult : public IMessageProcessor
{
public:
    HmiOpenRelayResult(ICache *c, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    ICache *m_cache;
    UserOperator *m_useroperator;
};

class UploadReqData
{
public:
    uint64_t userid = 0;
    uint64_t sceneid = 0;
    uint32_t allsize = 0;
    uint32_t usedsize = 0;
    uint32_t uploadfilesize = 0;
    std::string scenename;
    std::string strfilenames;
    std::vector<std::pair<uint32_t, std::string> > files;
    bool shouldConfirm = false;
    Message *msg = 0;
};
class UploadScene : public IMessageProcessor
{
public:
    UploadScene(MicoServer *ms, ICache *c, IDelSceneUploadLog *idel, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    int reqUpload(Message *, std::list<InternalMessage *> *r);
    int uploadConfirm(Message *, std::list<InternalMessage *> *r);

    void uploadNew(const UploadReqData &d);
    void replaceUpload(const UploadReqData &d);

    void setUploadUrlToMsg(uint64_t userid,
                uint64_t sceneid,
                uint32_t serverid, 
                const std::vector<std::pair<uint32_t, std::string> > &files,
                Message *msg);

    MicoServer  *m_server;
    ICache *m_cache;
    UserOperator *m_useroperator;
    IDelSceneUploadLog *m_delscenelog;
    std::shared_ptr<Timer> m_delscenepollTimer;
    ClockTimer m_clocktimer;
};

class DownloadScene : public IMessageProcessor
{
public:
    DownloadScene(ICache *c, UserOperator *uop);
    int processMesage(Message *reqmsg, std::list<InternalMessage *> *r);

private:
    ICache *m_cache;
    UserOperator *m_useroperator;
};

class TransferScene : public IMessageProcessor
{
public:
    TransferScene(ICache *c, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    void genSceneTransferNotify(uint64_t userid,
            uint64_t targetuserid,
            uint64_t sceneid,
            std::list<InternalMessage *> *r);

    ICache *m_cache;
    UserOperator *m_useroperator;
};

class GetClientDeviceSessionkey : public IMessageProcessor
{
public:
    GetClientDeviceSessionkey(ICache *c, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    ICache *m_cache;
    UserOperator *m_useroperator;
};

class CloseRelay : public IMessageProcessor
{
public:
    CloseRelay(ICache *c);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    ICache *m_cache;
};

class SceneFileChanged : public IMessageProcessor
{
public:
    SceneFileChanged(ICache *c, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    void genNotifySceneFileChanged(uint64_t ownerid,
            uint64_t sceneid,
            const std::string &scenename,
            std::list<InternalMessage *> *r);

    ICache *m_cache;
    UserOperator *m_useroperator;
};
#endif
