#ifndef USERMESSAGEPROCESS__H
#define USERMESSAGEPROCESS__H

#include <vector>
#include <list>
#include <stdint.h>
#include <string>
#include <map>
#include "imessageprocessor.h"

class UserOperator;
class ICache;
class Notification;
class TransferDeviceToAnotherCluster;
class MicoServer;
class DeviceAutho;

class UserRegister : public IMessageProcessor
{
public:
    UserRegister(ICache *c, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    ICache *m_cache;
    UserOperator *m_useroperator;
};

class UserLogoutProcessor : public IMessageProcessor
{
public:
    UserLogoutProcessor(MicoServer *, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    MicoServer *m_server;
    UserOperator *m_useroperator;
};

class UserOffline : public IMessageProcessor
{
public:
    UserOffline(ICache *c, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    ICache *m_cache;
    UserOperator *m_useroperator;
};
class UserModifyPassword : public IMessageProcessor
{
public:
    UserModifyPassword(ICache *c, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);
private:
    UserOperator *m_useroperator;
    ICache *m_cache;
};

class ModifyNickName : public IMessageProcessor
{
public:
    ModifyNickName(ICache *c, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    UserOperator *m_useroperator;
    ICache *m_cache;
};

class ModifySignature : public IMessageProcessor
{
public:
    ModifySignature(ICache *c, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    UserOperator *m_useroperator;
    ICache *m_cache;
};

class GetMyDeviceList : public IMessageProcessor
{
public:
    GetMyDeviceList(ICache *c, UserOperator *uop);
    // InternalMessage *processMesage(Message *);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    Message *getMyDeviceList_V180(Message *);

    UserOperator *m_useroperator;
    ICache *m_cache;
};

class ModifyDeviceNoteName : public IMessageProcessor
{
public:
    ModifyDeviceNoteName(ICache *c, UserOperator *uop);
    // InternalMessage *processMesage(Message *);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    UserOperator *m_useroperator;
    ICache *m_cache;
};

// 用户心跳处理
class UserHeartBeat : public IMessageProcessor
{
public:
    UserHeartBeat(MicoServer *ms, ICache *c, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    MicoServer *m_ms;
    ICache *m_cache;
    UserOperator *m_useroperator;
};

class GetClientIPAndPort : public IMessageProcessor
{
public:
    GetClientIPAndPort(ICache *c, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    ICache *m_cache;
    UserOperator *m_useroperator;
};

// -创建设备群-
class CreateDeviceCluster : public IMessageProcessor
{
public:
    CreateDeviceCluster(ICache *c, UserOperator *uop);
    // InternalMessage *processMesage(Message *);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:

    UserOperator *m_useroperator;
    ICache *m_cache;
};

// 删除设备群
class DeleteDeviceCluster : public IMessageProcessor
{
public:
    DeleteDeviceCluster(ICache *c, UserOperator *uop);
    // InternalMessage *processMesage(Message *);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    void genClusterDelNotify(uint64_t userid,
            uint64_t deviceclusterid,
            const std::string &clusteraccount,
            std::list<uint64_t> usersincluster,
            std::list<InternalMessage *> *r);

    UserOperator *m_useroperator;
    ICache *m_cache;
};

class UserExitDeviceCluster : public IMessageProcessor
{
public:
    UserExitDeviceCluster(ICache *c, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    void genNotifyUserExitCluster(uint64_t userid,
            uint64_t clusterid,
            std::list<InternalMessage *> *r);

    ICache *m_cache;
    UserOperator *m_useroperator;
};

// 修改设备群notename
class DeviceClusterModifyNoteName : public IMessageProcessor
{
public:
    DeviceClusterModifyNoteName(ICache *c, UserOperator *uop);
    // InternalMessage *processMesage(Message *);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    UserOperator *m_useroperator;
    ICache *m_cache;
};

class DeviceClusterModifyDescription : public IMessageProcessor
{
public:
    DeviceClusterModifyDescription(ICache *c, UserOperator *uop);
    // InternalMessage *processMesage(Message *);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    void genClusterDescChangedNotify(
            uint64_t clusterID,
            const std::string &description,
            std::list<InternalMessage *> *r);

    UserOperator *m_useroperator;
    ICache *m_cache;
};

//--
class GetClusterListOfUser : public IMessageProcessor
{
public:
    GetClusterListOfUser(ICache *cache, UserOperator *uop);
    // InternalMessage *processMesage(Message *);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    Message *getClusterList_V180(Message *reqmsg);

    UserOperator *m_useroperator;
    ICache *m_cache;
};

class GetDeviceClusterInfo : public IMessageProcessor
{
public:
    GetDeviceClusterInfo(ICache *c, UserOperator *uop);
    // InternalMessage *processMesage(Message *);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    UserOperator *m_useroperator;
    ICache *m_cache;
};

class GetDeviceInCluster : public IMessageProcessor
{
public:
    GetDeviceInCluster(ICache *c, UserOperator *uop);
    // InternalMessage *processMesage(Message *);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    Message *getDeviceInCluster_V180(Message *reqmsg);

    UserOperator *m_useroperator;
    ICache *m_cache;
};

class GetOnlineDeviceOfCluster : public IMessageProcessor
{
public:
    GetOnlineDeviceOfCluster(ICache *c, UserOperator *uop);
    // InternalMessage *processMesage(Message *);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    UserOperator *m_useroperator;
    ICache *m_cache;
};

// 取得群里面某一个设备的所有使用者
class GetOperatorListOfDeviceInCluster : public IMessageProcessor
{
public:
    GetOperatorListOfDeviceInCluster(ICache *c, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    void writeListToMessageV200(Message *destmsg, uint32_t startpos, std::vector<DeviceAutho> *vDeviceAutho, uint16_t *cnt);
    void writeListToMessageV201(Message *destmsg, uint32_t startpos, std::vector<DeviceAutho> *vDeviceAutho, uint16_t *cnt, uint32_t maxmsgsize);

    UserOperator *m_useroperator;
    ICache *m_cache;
};

class GetDeviceListOfUserInDeviceCluster : public IMessageProcessor
{
public:
    GetDeviceListOfUserInDeviceCluster(ICache *c, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    ICache *m_cache;
    UserOperator *m_useroperator;
};

// 取设备群中的设备列表
class GetUerMembersInDeviceCluster : public IMessageProcessor
{
public:
    GetUerMembersInDeviceCluster(ICache *c, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    ICache *m_cache;
    UserOperator *m_useroperator;
};

// ..
class GetUserMemberOnlineStatusInDeviceCluster : public IMessageProcessor
{
public:
    GetUserMemberOnlineStatusInDeviceCluster(ICache *c, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    ICache *m_cache;
    UserOperator *m_useroperator;
};

class TransferDeiceToAnotherClusterProcessor : public IMessageProcessor
{
public:
    TransferDeiceToAnotherClusterProcessor(ICache *c, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    int deviceClusterChanged(Message *reqmsg, std::list<InternalMessage *> *r);
    int transferreq(Message *, std::list<InternalMessage *> *r);
    int transferApprover(Message *, std::list<InternalMessage *> *r);

    bool getClusterIDAndAccount(char *buf, int buflen, std::pair<uint64_t, std::string> *out);

    ICache *m_cache;
    UserOperator *m_useroperator;
    TransferDeviceToAnotherCluster *m_transfer;
};

class AssignDevicetToUser : public IMessageProcessor
{
public:
    AssignDevicetToUser(ICache *, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    void genNotifyUserGetSomeDevice(uint64_t userMemberID,
        uint64_t adminid,
        uint64_t clusterID,
        const std::map<uint64_t, uint8_t> &mapDeviceIDAuthority,
        std::list<InternalMessage *> *rr);

    ICache *m_cache;
    UserOperator *m_useroperator;
};

class RemoveDeviceOfUserMember : public IMessageProcessor
{
public:
    RemoveDeviceOfUserMember(ICache *c, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    void genNotifyUserLostDevice(uint64_t usermemberID,
        uint64_t adminID,
        uint64_t clusterID,
        const std::map<uint64_t, uint8_t> &mapDeviceIDAuthority,
        std::list<InternalMessage *> *r);

    ICache *m_cache;
    UserOperator *m_useroperator;
};

class GiveDeviceToManyUser : public IMessageProcessor
{
public:
    GiveDeviceToManyUser(ICache *c, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);
private:
    void genNotifyUserGetDevice(
        uint64_t adminID,
        uint64_t clusterID,
        uint64_t deviceID,
        const std::map<uint64_t, uint8_t> &mapUserIDAuthority,
        std::list<InternalMessage *> *r);

    ICache *m_cache;
    UserOperator *m_useroperator;
};

class RemoveDeviceUsers : public IMessageProcessor
{
public:
    RemoveDeviceUsers(ICache *c, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);
private:
    void genNotifyUserLostDevice(
        uint64_t adminID,
        uint64_t clusterID,
        uint64_t deviceID,
        const std::map<uint64_t, uint8_t> &mapUserIDAuthority,
        std::list<InternalMessage *> *r);

    ICache *m_cache;
    UserOperator *m_useroperator;

};

class InviteUserToDeviceCluster : public IMessageProcessor
{
public:
    InviteUserToDeviceCluster(ICache *c, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);
private:
    int inviteReq(Message *reqmsg, std::list<InternalMessage *> *r);
    int inviteApproval(Message *reqmsg, std::list<InternalMessage *> *r);

    void genNotifyToTargetUser(uint64_t, // 邀请者ID
            const std::string &account,// 邀请者帐号
            uint64_t clusterID, // 群ID
            const std::string &clusterAccount, // 群帐号
            uint64_t inviteeID, // 被邀请者ID
            std::list<InternalMessage *> *r);

    void genNotifyAdminUserAccept(uint8_t, // 是否同意
            uint64_t adminID,
            uint64_t clusterID,
            const std::string &clusterAccount,
            uint64_t inviteeID, // 被邀请者ID
            const std::string &inviteeaccount, // 被邀请者帐号
            std::list<InternalMessage *> *r);

    void genNotifyNewUserAddedToCluster(uint64_t clusterID,
            const std::string &clusterAccount,
            uint64_t inviteeID, // 加入者ID
            const std::string &inviteeaccount, // 加入者帐号
            std::list<InternalMessage *> *r);

    ICache *m_cache;
    UserOperator *m_useroperator;
};

class RemoveUserFromDeviceCluster : public IMessageProcessor
{
public:
    RemoveUserFromDeviceCluster(ICache *c, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);
private:
    // 通知被删除的用户
    void genNotifyUserBeRemoved(uint64_t clusterID,
            const std::string &clusterAccount,
            uint64_t adminid,
            const std::string &adminaccount,
            const std::vector<uint64_t> &vUserMemberID,
            std::list<InternalMessage *> *r);

    // 通知群里面的用户哪些人被删除了
    void genNotifySomeUserRemovedFromCluster(uint64_t clusterID,
            const std::string &clusterAccount,
            const std::vector<uint64_t> &vUserMemberID,
            std::list<InternalMessage *> *r);


    ICache *m_cache;
    UserOperator *m_useroperator;
};

class ModifyUserMemberRole : public IMessageProcessor
{
public:
    ModifyUserMemberRole(ICache *c, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    void genNotifyRoleChanged(uint64_t userid,
            uint64_t clusterID,
            uint8_t newRole,
            std::list<InternalMessage *> *r);

    ICache *m_cache;
    UserOperator *m_useroperator;
};

class GetDeviceInfoInCluster : public IMessageProcessor
{
public:
    GetDeviceInfoInCluster(ICache *c, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    ICache *m_cache;
    UserOperator *m_useroperator;
};

class UserApplyToJoinCluster : public IMessageProcessor
{
public:
    UserApplyToJoinCluster(ICache *c, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    int  req(Message *reqmsg,  std::list<InternalMessage *> *r);
    int  approval(Message *reqmsg,  std::list<InternalMessage *> *r);

    void genNotifyClusterOwner(uint64_t sysAdminID,
            uint64_t clusterID,
            const std::string &clusterAccount,
            uint64_t userid,
            const std::string &useraccount,
            std::list<InternalMessage *> *r);

    void genNotifyToUserResult(uint64_t userid,
            uint64_t clusterid,
            const std::string &clusteraccount,
            uint8_t result,
            std::list<InternalMessage *> *r);

    void genNotifyToUserInCluster(uint64_t clusterID,
            const std::string &clusteraccount,
            uint64_t applicantID,
            std::list<InternalMessage *> *r);

    ICache *m_cache;
    UserOperator *m_useroperator;
};

class SearchDeviceCluster : public IMessageProcessor
{
public:
    SearchDeviceCluster(ICache *, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);
private:
    ICache *m_cache;
    UserOperator *m_useroperator;
};

#endif
