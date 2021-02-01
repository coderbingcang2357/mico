#include <unistd.h>
#include "../Business/Business.h"
#include "../onlineinfo/icache.h"
#include "../imessagesender.h"
#include "../onlineinfo/userdata.h"
#include "../sessionkeygen.h"
#include "timer/TimeElapsed.h"
#include "Message/Message.h"
#include "../onlineinfo/mainthreadmsgqueue.h"
#include "../messageencrypt.h"
#include "../newmessagecommand.h"
#include "protocoldef/protocol.h"

#include "../usermessageprocess.h"
#include "../loginprocessor.h"
#include "../deviceregister.h"
#include "../devicelogin.h"
#include "../userpasswordrecover.h"
#include "../searchuser.h"
#include "../scenemessageprocess.h"
#include "../claimdevice.h"
#include "../usermodifymail.h"
#include "../feedback.h"
#include "../messagefromnotifysrv.h"
#include "../deletenotifymessage.h"
#include "../transferdevicecluster.h"
#include "../messagesender.h"
#include "testmessateprocessor.h"
#include "../getuserinfo.h"

void initMessageProcessor(InternalMessageProcessor *msgprocessor);

class TestCache : public ICache
{
public:
    TestCache()
    {
        ud = std::shared_ptr<UserData>(new UserData);
        ud->userid = 100039;
        std::vector<char> sesskey;
        genSessionKey(&sesskey);
        ud->setSessionKey(sesskey);
    }
    bool objectExist(uint64_t objid)
    {
        return true;
    }
    void insertObject(uint64_t objid, const shared_ptr<UserData> &data)
    {

    }

    void insertObject(uint64_t objid, const shared_ptr<DeviceData> &data)
    {
    }

    void removeObject(uint64_t objid) 
    {
    }

    shared_ptr<IClient> getData(uint64_t objid)
    {
        return ud;
    }

    std::shared_ptr<UserData> ud;
};

TestCache tc;

class TestSender : public IMessageSender
{
public:
    bool sendMessage(InternalMessage *msg)
    {
        //assert(msg->message()->k);
        if (msg->message()->CommandID() != CMD::CMD_PER__LOGIN1__REQ)
            ::decryptMsg(msg->message(), &tc);

        if (msg->message()->CommandID() == CMD::CMD_PER__LOGIN2__VERIFY)
            return true;

        return true;
        char *ans = msg->message()->Content();
        assert(uint8_t(*ans) == ANSC::Success);
        return true;
    }
    bool sendMessageToAnotherServer(InternalMessage *msg)
    {
        return true;
    }
};


void testmessgeProcessor(Tcpserver*t)
{
    TestSender ts;
    InternalMessageProcessor im(&tc, t, &ts);
    initMessageProcessor(&im);
    // post 10000 message to message queue
    for (int i=0; i < 10000; ++i)
    {
        Message *msg = new Message;
        msg->SetCommandID(CMD::CMD_PER__MODIFY_NICKNAME);
        msg->SetObjectID(tc.ud->userid);
        std::string str("测试");
        str.append(std::to_string(i));
        msg->appendContent(str);
        encryptMessage(msg, &tc);
        NewMessageCommand *nc = new NewMessageCommand(
            new InternalMessage(msg,
                    InternalMessage::FromExtserver,
                    InternalMessage::Unused)
            );
        MainQueue::postCommand(nc);
    }

    TimeElap elap;
    elap.start();
    int cnt = 0;
    while (1)
    {
        if (im.handleMessage() != 0)
        {
            //usleep(1000);
            break;
        }
        cnt++;
    }
    uint64_t r = elap.elaps();
    printf("timerused: msg: %d, s, %lu, us: %lu\n", cnt, r / (1000 * 1000), r % (1000 * 1000));
}

void testGetUserInfo(Tcpserver*t)
{
    // TestCache tc;
    TestSender ts;
    InternalMessageProcessor im(&tc, t, &ts);
    initMessageProcessor(&im);
    // post 10000 message to message queue
    for (int i=0; i < 10000; ++i)
    {
        Message *msg = new Message;
        msg->SetCommandID(CMD::CMD_PER__GET_USER_INFO);
        msg->SetObjectID(tc.ud->userid);
        //std::string str("测试");
        //str.append(std::to_string(i));
        msg->appendContent(tc.ud->userid);
        encryptMessage(msg, &tc);
        NewMessageCommand *nc = new NewMessageCommand(
            new InternalMessage(msg,
                    InternalMessage::FromExtserver,
                    InternalMessage::Unused)
            );
        MainQueue::postCommand(nc);
    }

    TimeElap elap;
    elap.start();
    int cnt = 0;
    while (1)
    {
        if (im.handleMessage() != 0)
        {
            //usleep(1000);
            break;
        }
        cnt++;
    }
    uint64_t r = elap.elaps();
    printf("get userinfo timerused: msg: %d, s, %lu, us: %lu\n", cnt, r / (1000 * 1000), r % (1000 * 1000));
}

void testLoginReq(Tcpserver*t)
{
    // TestCache tc;
    TestSender ts;
    InternalMessageProcessor im(&tc, t, &ts);
    initMessageProcessor(&im);
    // post 10000 message to message queue
    for (int i=0; i < 10000; ++i)
    {
        Message *msg = new Message;
        msg->SetCommandID(CMD::CMD_PER__LOGIN1__REQ);
        msg->SetObjectID(0);
        std::string str("wqs");
        str.append(std::to_string(i));
        msg->appendContent(str);
        // encryptMessage(msg, &tc);
        NewMessageCommand *nc = new NewMessageCommand(
            new InternalMessage(msg,
                    InternalMessage::FromExtserver,
                    InternalMessage::Unused)
            );
        MainQueue::postCommand(nc);
    }

    TimeElap elap;
    elap.start();
    int cnt = 0;
    while (1)
    {
        if (im.handleMessage() != 0)
        {
            //usleep(1000);
            break;
        }
        cnt++;
    }
    uint64_t r = elap.elaps();
    printf("get loginreq timerused: msg: %d, s, %lu, us: %lu\n", cnt, r / (1000 * 1000), r % (1000 * 1000));
}

void testLoginVerfy(Tcpserver*t)
{
    // TestCache tc;
    TestSender ts;
    InternalMessageProcessor im(&tc, t, &ts);
    initMessageProcessor(&im);
    // post 10000 message to message queue
    for (int i=0; i < 10000; ++i)
    {
        Message *msg = new Message;
        msg->SetCommandID(CMD::CMD_PER__LOGIN2__VERIFY);
        msg->SetObjectID(tc.ud->userid + i);
        std::string str("wqs");
        str.append(std::to_string(i+1));
        msg->appendContent(str);
        // encryptMessage(msg, &tc);
        NewMessageCommand *nc = new NewMessageCommand(
            new InternalMessage(msg,
                    InternalMessage::FromExtserver,
                    InternalMessage::Unused)
            );
        MainQueue::postCommand(nc);
    }

    TimeElap elap;
    elap.start();
    int cnt = 0;
    while (1)
    {
        if (im.handleMessage() != 0)
        {
            //usleep(1000);
            break;
        }
        cnt++;
    }
    uint64_t r = elap.elaps();
    printf("get loginverfy timerused: msg: %d, s, %lu, us: %lu\n", cnt, r / (1000 * 1000), r % (1000 * 1000));

}

void initMessageProcessor(InternalMessageProcessor *msgprocessor)
{
    ICache *cache = msgprocessor->cache();
    // 用户
    // 修改昵称
    msgprocessor->appendMessageProcessor(CMD::CMD_PER__MODIFY_NICKNAME,
        std::shared_ptr<IMessageProcessor>(new ModifyNickName(cache)));
    // 修改签名
    msgprocessor->appendMessageProcessor(CMD::CMD_PER__MODIFY_SIGNATURE,
        std::shared_ptr<IMessageProcessor>(new ModifySignature(cache)));
    // 取得用户加入了哪些设备群
    msgprocessor->appendMessageProcessor(CMD::CMD_DCL__GET_CLUSTER_LIST,
        std::shared_ptr<IMessageProcessor>(new GetClusterListOfUser(cache)));

    // login
    std::shared_ptr<IMessageProcessor> loginproc(new LoginProcessor(cache));
    msgprocessor->appendMessageProcessor(CMD::CMD_PER__LOGIN1__REQ, loginproc);
    msgprocessor->appendMessageProcessor(CMD::CMD_PER__LOGIN2__VERIFY, loginproc);

    // 注销, login out CMD_PER__LOGOUT
    msgprocessor->appendMessageProcessor(CMD::CMD_PER__LOGOUT,
        std::shared_ptr<UserLoginoutProcessor>(new UserLoginoutProcessor((cache))));

    // 用户下线
    msgprocessor->appendMessageProcessor(CMD::INCMD_USER_OFFLINE,
        std::shared_ptr<UserOffline>(new UserOffline((cache))));


    // user register
    msgprocessor->appendMessageProcessor(CMD::CMD_PER__REGISTER1__SET_MAILBOX,
        std::shared_ptr<IMessageProcessor>(new UserRegister(cache)));

    // user reset password
    std::shared_ptr<IMessageProcessor> resetpwdproc(new UserPasswordRecover);
    msgprocessor->appendMessageProcessor(CMD::CMD_PER__RECOVER_PWD1__REQ,
        resetpwdproc);
    msgprocessor->appendMessageProcessor(CMD::CMD_PER__RECOVER_PWD2__VERIFY,
        resetpwdproc);
    msgprocessor->appendMessageProcessor(CMD::CMD_PER__RECOVER_PWD3__RESET,
        resetpwdproc);

    // user modify password CMD_PER__MODIFY_PASSWORD
    msgprocessor->appendMessageProcessor(CMD::CMD_PER__MODIFY_PASSWORD,
        std::shared_ptr<UserModifyPassword>(new UserModifyPassword(cache)));

    // user modify mail
    std::shared_ptr<UserModifyMail> usermodifymail(new UserModifyMail(cache));
    msgprocessor->appendMessageProcessor(CMD::CMD_PER__MODIFY_MAILBOX1__REQ,
        usermodifymail);
    msgprocessor->appendMessageProcessor(CMD::CMD_PER__MODIFY_MAILBOX2__VERIFY,
        usermodifymail);

    // search user
    std::shared_ptr<IMessageProcessor> searchuserproc(new SearchUser(cache));
    msgprocessor->appendMessageProcessor(
        CMD::CMD_FRI__ADD_FRIEND__SEARCH_USER,
        searchuserproc);

    msgprocessor->appendMessageProcessor(
        CMD::CMD_PER__GET_USER_INFO,
        std::shared_ptr<GetUserInfo>(new GetUserInfo(cache)));

    // 用户心跳包
    msgprocessor->appendMessageProcessor(
        CMD::CMD_GEN__HEART_BEAT,
        std::shared_ptr<IMessageProcessor>(new UserHeartBeat(cache)));

    // 设备 -------------------------------------------------------------------
    // 取得我的设备
    msgprocessor->appendMessageProcessor(CMD::CMD_DEV__GET_MY_DEVICE_LIST,
        std::shared_ptr<IMessageProcessor>(new GetMyDeviceList(cache)));
    // 修改设备notename
    msgprocessor->appendMessageProcessor(CMD::CMD_DEV__MODIFY_DEVICE_NOTENAME,
        std::shared_ptr<IMessageProcessor>(new ModifyDeviceNoteName(cache)));
    // device register
    msgprocessor->appendMessageProcessor(CMD::DCMD__REGISTER,
        std::shared_ptr<IMessageProcessor>(new DeviceRegister));

    // device login
    msgprocessor->appendMessageProcessor(CMD::DCMD__LOGIN1,
        std::shared_ptr<IMessageProcessor>(new DeviceLoginProcessor(cache)));
    // device offline
    msgprocessor->appendMessageProcessor(CMD::INCMD_DEVICE_OFFLINE,
        std::shared_ptr<IMessageProcessor>(new DeviceOffline(cache)));

    // 设备心跳包
    std::shared_ptr<IMessageProcessor> devicehb(new DeviceHeartBeat(cache));
    msgprocessor->appendMessageProcessor(CMD::DCMD__SHORT_HEART_BEAT,
        devicehb);
    msgprocessor->appendMessageProcessor(CMD::DCMD__LONG_HEART_BEAT,
        devicehb);

    // 取设备在线...状态
    msgprocessor->appendMessageProcessor(CMD::CMD_DEV__GET_MY_DEVICE_ONLINE_LIST,
        std::shared_ptr<IMessageProcessor>(new GetMyDeviceOnlineStatus(cache)));


    // 设备群------------------------------------------------------------------
    // 创建设备群
    msgprocessor->appendMessageProcessor(CMD::CMD_DCL__CREATE,
        std::shared_ptr<IMessageProcessor>(new CreateDeviceCluster(cache)));

    // 搜索设备群
    msgprocessor->appendMessageProcessor(CMD::CMD_DCL__JOIN__SEARCH_CLUSTER,
        std::shared_ptr<IMessageProcessor>(new SearchDeviceCluster(cache)));

    // 解散群/删除群, CMD::CMD_DCL__DELETE__REQ
    msgprocessor->appendMessageProcessor(CMD::CMD_DCL__DELETE__REQ,
       std::shared_ptr<IMessageProcessor>(new DeleteDeviceCluster(cache)));

    // 退出群CMD::CMD_DCL__EXIT:
    msgprocessor->appendMessageProcessor(CMD::CMD_DCL__EXIT,
       std::shared_ptr<IMessageProcessor>(new UserExitDeviceCluster(cache)));
    //
    // 修改设备群 notename
    msgprocessor->appendMessageProcessor(CMD::CMD_DCL__MODIFY_NOTENAME,
        std::shared_ptr<IMessageProcessor>(new DeviceClusterModifyNoteName(cache)));
    // 修改设备群描述
    msgprocessor->appendMessageProcessor(CMD::CMD_DCL__MODIFY_DISCRIPTION,
        std::shared_ptr<IMessageProcessor>(new DeviceClusterModifyDescription(cache)));
    // 取得一个设备群的信息
    msgprocessor->appendMessageProcessor(CMD::CMD_DCL__GET_CLUSTER_INFO,
        std::shared_ptr<IMessageProcessor>(new GetDeviceClusterInfo(cache)));
    // 取得设备群中的设备
    msgprocessor->appendMessageProcessor(CMD::CMD_DCL__GET_DEVICE_MEMBER_LIST,
        std::shared_ptr<IMessageProcessor>(new GetDeviceInCluster(cache)));

    //msgprocessor->appendMessageProcessor(
    //    CMD::CMD_DCL__GET_TRANSER_DEVICE_LIST,
    //    std::shared_ptr<IMessageProcessor>(new (cache))));

    // 取得设备群中在线的设备
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__GET_DEVICE_MEMBER_ONLINE_LIST,
        std::shared_ptr<IMessageProcessor>(new GetOnlineDeviceOfCluster(cache)));

    // 取群里面某一个设备的使用者CMD::CMD_DCL__GET_OPERATOR_LIST_OF_DEV_MEMBER:
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__GET_OPERATOR_LIST_OF_DEV_MEMBER,
        std::shared_ptr<IMessageProcessor>(new GetOperatorListOfDeviceInCluster(cache)));
    // 取群里面某一个人可以使用哪些设备case CMD::CMD_DCL__GET_DEVICE_LIST_OF_USER_MEMBER:
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__GET_DEVICE_LIST_OF_USER_MEMBER,
        std::shared_ptr<IMessageProcessor>(new GetDeviceListOfUserInDeviceCluster(cache)));
    // 取得设备群中的成员 case CMD::CMD_DCL__GET_USER_MEMBER_LIST:
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__GET_USER_MEMBER_LIST,
        std::shared_ptr<IMessageProcessor>(new GetUerMembersInDeviceCluster(cache)));
    // CMD_DCL__GET_USER_MEMBER_ONLINE_LIST     
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__GET_USER_MEMBER_ONLINE_LIST,
        std::shared_ptr<IMessageProcessor>(new GetUserMemberOnlineStatusInDeviceCluster(cache)));

    // 设备认领CMD:::
    std::shared_ptr<IMessageProcessor> claimprocessor(new ClaimDeviceProcessor(cache));
    msgprocessor->appendMessageProcessor(
        CMD::CCMD__DEVICE_CLAIM,
        claimprocessor);
    msgprocessor->appendMessageProcessor(
        CMD::INTERNAL_CLAIM_TIMER_RESEND,
        claimprocessor);

    // 移交设备CMD::CMD_DCL__TRANSFER_DEVICES__REQ:CMD::CMD_DCL__TRANSFER_DEVICES__APPROVAL:
    std::shared_ptr<IMessageProcessor> transferdev(new TransferDeiceToAnotherCluster(cache));
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__TRANSFER_DEVICES__REQ,
        transferdev);
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__TRANSFER_DEVICES__APPROVAL,
        transferdev);
    // 设备授权CMD::CMD_DCL__ASSIGN_DEVICES_TO_USER_MEMBER:
    // 把一些设备给某一个人用
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__ASSIGN_DEVICES_TO_USER_MEMBER,
        std::shared_ptr<IMessageProcessor>(new AssignDevicetToUser(cache)));
    // 取消设备授权CMD::CMD_DCL__REMOVE_DEVICES_OF_USER_MEMBER:
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__REMOVE_DEVICES_OF_USER_MEMBER,
        std::shared_ptr<IMessageProcessor>(new RemoveDeviceOfUserMember(cache)));
    // CMD_DCL__ASSIGN_OPERATORS_TO_DEV_MEMBER
    // 把一个设备给一些人用
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__ASSIGN_OPERATORS_TO_DEV_MEMBER,
        std::shared_ptr<IMessageProcessor>(new GiveDeviceToManyUser(cache)));
    // CMD_DCL__REMOVE_OPERATORS_OF_DEV_MEMBER
    // 取消授权, 取消一个设备的一些使用者
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__REMOVE_OPERATORS_OF_DEV_MEMBER,
        std::shared_ptr<IMessageProcessor>(new RemoveDeviceUsers(cache)));

    // 邀请加入群CMD::CMD_DCL__INVITE_TO_JOIN__REQ:CMD::CMD_DCL__INVITE_TO_JOIN__APPROVAL:
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__INVITE_TO_JOIN__REQ,
        std::shared_ptr<IMessageProcessor>(new InviteUserToDeviceCluster(cache)));
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__INVITE_TO_JOIN__APPROVAL,
        std::shared_ptr<IMessageProcessor>(new InviteUserToDeviceCluster(cache)));

    // 移出群成员CMD::CMD_DCL__REMOVE_USER_MEMBERS:
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__REMOVE_USER_MEMBERS,
        std::shared_ptr<IMessageProcessor>(new RemoveUserFromDeviceCluster(cache)));
    // 设为管理员CMD::CMD_DCL__MODIFY_USER_MEMBER_ROLE:
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__MODIFY_USER_MEMBER_ROLE,
        std::shared_ptr<IMessageProcessor>(new ModifyUserMemberRole(cache)));
    // 移交群
    std::shared_ptr<IMessageProcessor> transferDevCluster(new TransferDeviceCluster(cache));
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__TRANSFER_DEVCLUSTER_REQ,
        transferDevCluster);
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__TRANSFER_DEVCLUSTER_VERIRY,
        transferDevCluster);

    // 取得设备信息CMD::CMD_DCL__GET_DEVICE_INFO:
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__GET_DEVICE_INFO,
        std::shared_ptr<IMessageProcessor>(new GetDeviceInfoInCluster(cache)));
    // 申请加入群CMD::CMD_DCL__JOIN__REQ:CMD::CMD_DCL__JOIN__APPROVAL:
    std::shared_ptr<IMessageProcessor> userjoincluster(new UserAskFroJoinCluster(cache));
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__JOIN__REQ,
        userjoincluster);
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__JOIN__APPROVAL,
        userjoincluster);

    // 场景相关----------------------------------------------------------------
    // get my scene list
    msgprocessor->appendMessageProcessor(CMD::CMD_HMI__GET_MY_SCENE_LIST,
        std::shared_ptr<IMessageProcessor>(new GetMyHmiSenceListProcessor(cache)));
    // modify scene info
    msgprocessor->appendMessageProcessor(CMD::CMD_HMI__MODIFY_SCENE_INFO,
        std::shared_ptr<IMessageProcessor>(new ModifySceneInfo(cache)));
    // modify scene notename
    msgprocessor->appendMessageProcessor(CMD::CMD_HMI__MODIFY_NOTENAME,
        std::shared_ptr<IMessageProcessor>(new ModifyNotename(cache)));
    // 取得场景信息
    msgprocessor->appendMessageProcessor(CMD::CMD_HMI__GET_SCENE_INFO,
        std::shared_ptr<IMessageProcessor>(new GetHmiSceneInfo(cache)));
    // 取得场景共享给了哪些人
    msgprocessor->appendMessageProcessor(CMD::CMD_HMI__GET_SCENE_USER_LIST,
        std::shared_ptr<IMessageProcessor>(new GetHmiSenceShareUsers(cache)));
    // 共享场景
    msgprocessor->appendMessageProcessor(CMD::CMD_HMI__SHARE_SCENE,
        std::shared_ptr<IMessageProcessor>(new ShareScene(cache)));
    // 取消共享
    msgprocessor->appendMessageProcessor(CMD::CMD_HMI__CANCEL_SHARE_SCENE,
        std::shared_ptr<IMessageProcessor>(new DonotShareScene(cache)));
    // 修改共享权限
    msgprocessor->appendMessageProcessor(CMD::CMD_HMI__SET_SHARE_SCENE_AUTH,
        std::shared_ptr<IMessageProcessor>(new SetShareSceneAuth(cache)));
    // 删除场景 *** 未完成
    msgprocessor->appendMessageProcessor(CMD::CMD_HMI__DELETE_SCENE,
        std::shared_ptr<IMessageProcessor>(new DeleteScene(cache)));

    // 移交场景CMD_HMI__TRANSFER_SYSADMIN
    msgprocessor->appendMessageProcessor(CMD::CMD_HMI__TRANSFER_SYSADMIN,
        std::shared_ptr<IMessageProcessor>(new TransferScene(cache)));

    // 穿透CMD::CMD_HMI__PENETRATE__REQ
    msgprocessor->appendMessageProcessor(CMD::CMD_HMI__PENETRATE__REQ,
        std::shared_ptr<IMessageProcessor>(new HmiPenetrate(cache)));
    // CMD_HMI__OPEN_RELAY_CHANNELS
    msgprocessor->appendMessageProcessor(CMD::CMD_HMI__OPEN_RELAY_CHANNELS,
        std::shared_ptr<IMessageProcessor>(new HmiRelay(cache)));
    // CMD_HMI__GET_NEW_CDSESSION_KEY
    msgprocessor->appendMessageProcessor(CMD::CMD_HMI__GET_NEW_CDSESSION_KEY,
        std::shared_ptr<IMessageProcessor>(new GetClientDeviceSessionkey(cache)));
    // 取客户端外网IP/port CMD_HMI__GET_EXTRANET_IP_PORT
    msgprocessor->appendMessageProcessor(CMD::CMD_HMI__GET_EXTRANET_IP_PORT,
        std::shared_ptr<IMessageProcessor>(new GetClientIPAndPort(cache)));
    //
    // 上传场景
    msgprocessor->appendMessageProcessor(CMD::CMD_HMI__UPLOAD_SCENE,
        std::shared_ptr<IMessageProcessor>(new UploadScene(cache)));
    // 下载场景
    msgprocessor->appendMessageProcessor(CMD::CMD_HMI__DOWNLOAD_SCENE,
        std::shared_ptr<IMessageProcessor>(new DownloadScene(cache)));
    //
    //
    // 从relayserver过来的打开转发通道处理结果的消息
    msgprocessor->appendMessageProcessor(CMD::CMD_HMI__OPEN_RELAY_CHANNELS_RESULT,
        std::shared_ptr<IMessageProcessor>(new HmiOpenRelayResult(cache)));
    // CMD_HMI__CLOSE_RELAY_CHANNELS
    msgprocessor->appendMessageProcessor(CMD::CMD_HMI__CLOSE_RELAY_CHANNELS,
        std::shared_ptr<IMessageProcessor>(new CloseRelay(cache)));

    // CMD_GEN__FEEDBACK
    msgprocessor->appendMessageProcessor(CMD::CMD_GEN__FEEDBACK,
        std::shared_ptr<IMessageProcessor>(new SaveFeedBack(cache)));

    // 通知处理
    msgprocessor->appendMessageProcessor(CMD::CMD_SendNotif,
        std::shared_ptr<IMessageProcessor>(new SendNotify()));

    // 客户端收到通知后的回复处理
    std::shared_ptr<IMessageProcessor> delnotify(new DeleteNotify(cache));
    msgprocessor->appendMessageProcessor(CMD::CMD_GEN__NOTIFY,
        delnotify);
    msgprocessor->appendMessageProcessor(CMD::CMD_NOTIFY_RESP,
        delnotify);
}
