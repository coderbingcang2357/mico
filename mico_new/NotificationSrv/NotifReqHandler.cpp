#include <time.h>
#include <stdint.h>
#include <assert.h>
#include <unordered_set>
#include "Message/Notification.h"
#include "NotifReqHandler.h"
#include "util/logwriter.h"
//#include "../NotifRequest.h"
//#include "../NotifResponse.h"
//#include "../NotifReqReader.h"
//#include "../NotifResWriter.h"
#include "Database/DBOperator.h"
#include "config/shmconfig.h"
#include "notifytimeout.h"
#include "notifyManager.h"
#include "Message/Message.h"
#include "notifyidgen.h"

NotifReqHandler::NotifReqHandler(NotifyManager *nm,
            NotifyTimeOut *to,
            IDBoperator *idb)
        : m_notifymanager(nm), m_notifytimeout(to), dbOperator(idb)
{
}

NotifReqHandler::~NotifReqHandler()
{
    ClearResponses();
}

void NotifReqHandler::ClearResponses()
{
    for(size_t i = 0; i != m_vNotifRes.size(); i++)
        delete m_vNotifRes.at(i);

    m_vNotifRes.clear();
}

std::vector<Notification*>& NotifReqHandler::Execute(Message *notifReq)
{
    ClearResponses();
    uint16_t cmdID = notifReq->commandID();
    Notification *notification = new Notification;
    if (!notification->fromByteArray(notifReq->content(), notifReq->contentLen()))
    {
        ::writelog("error notify.");
        return m_vNotifRes;
    }
    switch(cmdID)
    {
        //case CMD::CMD_SaveOfflineNotif:
            //::writelog("OnSaveOfflineNotif");
            //OnSaveOfflineNotif(notification);   // 处理“存储离线通知”业务, 要通知的对象不在线
            //break;

        case CMD::CMD_NEW_NOTIFY: //
            //::writelog("OnSaveOnlineNotif");
            OnSaveOnlineNotif(notification);    // 处理“存储在线通知”业务, 要通知的对象在线,不在线就存数据库，等上线再通知
            break;

        case CMD::CMD_NEW_SESSIONAL_NOTIFY:
            //::writelog("OnSaveSessionalOnlineNotif");
            // 处理“仅在当前会话有效的在线通知”业务,要通知的对象在线且通知仅在当前会话有效
            OnSaveSessionalOnlineNotif(notification); 
            break;

        case CMD::CMD_GET_NOTIFY_WHEN_LOGIN:
            ::writelog("OnGetNotifWhenLogin");
            OnGetNotifWhenLogin(notifReq->objectID());
            delete notification;
            break;

        case CMD::CMD_GET_NOTIFY_WHEN_BACK_ONLINE:
            ::writelog("OnGetNotifWhenBackOnline");
            OnGetNotifWhenBackOnline(notifReq->objectID());
            delete notification;
            break;

        case CMD::CMD_DELETE_NOTIFY:
            ::writelog("OnDeleteNotif");
            // 处理“删除通知”业务, 收到对方回复，表示通知已送达, 当客户端收到通
            // 知后会发此命令过来
            OnDeleteNotif(notifReq->objectID(), notification->serialNumber());
            delete notification;
            break;

        //case CMD::CMD_MoveNotifToDatabase:
        //    ::writelog("OnMoveNotifToDatabase");
        //    // 处理“转储通知”业务, 对象下线时, 将缓存中的通知转存到数据库
        //    OnDumpNotif(notifReq->ObjectID());
        //    break;

        default:
            ::writelog(InfoType, "unknow command: %u", cmdID);
            delete notification;
            break;
    }

    return m_vNotifRes;
}

void NotifReqHandler::OutputResponse(Notification* notifRes)
{
    uint8_t an = notifRes->answercode();
    (void)an;
    m_vNotifRes.push_back(notifRes);
}

// 发通知到客户端, 保存到时cache中, 然后发回客户端
void NotifReqHandler::OnSaveOnlineNotif(Notification *notification)
{
    // 插入到数据库中, 同时生成通知号(notifynumber), 这一步必须成功, 否则
    // 后面的操作没法进行
    uint64_t notifyid;
    bool res = dbOperator->SaveNotification(*notification, &notifyid);
    if (!res)
    {
        // error
        delete notification;
        return;
    }
    assert(notifyid != 0);
    // 通知号
    notification->setSerialNumber(notifyid);
    m_notifymanager->insertNotify(notification);
    m_notifytimeout->addTimeout(notifyid);
    ::writelog(InfoType, "new notify: %lu, %u", notification->GetObjectID(), notification->getNotifyType());

    // 发到 LogicalSrv, 然后LogicalSrv会转发到ExtSrv, 最后ExtSrv发到客户端
    OutputResponse(new Notification(*notification));   // 把notifRes放到列表m_vNotifRes中
}

// 这里的处理是sessional通知不添加到数据库中, 只在内存中, 这样当此通知超时重发次
// 数超过预设值3次, 就会把sessional通知删除
void NotifReqHandler::OnSaveSessionalOnlineNotif(Notification *notification)
{
    // 通过插入数据库, 在数据库中生成通知号
    uint64_t notifyid = Notify::genID();
    assert(notifyid != 0);
    notification->setSerialNumber(notifyid);

    if (!m_notifymanager->insertNotify(notification))
    {
        ::writelog("find same notification id.");
        delete notification;
        return;
    }

    m_notifytimeout->addTimeout(notification->serialNumber());

    OutputResponse(new Notification(*notification));
}

//用户不在线时的通知消息, 直接写到数据库
void NotifReqHandler::OnSaveOfflineNotif(Notification *notification)
{
    uint64_t id;
    dbOperator->SaveNotification(*notification, &id);
	printf("genIDDDDDDD:%llu\n",id);
    delete notification;
}

// 用户上线, 取得用户的通知
void NotifReqHandler::OnGetNotifWhenLogin(uint64_t objectID)
{
	// 离线消息超过300，删除旧的，最多给客户端推送300条,多了客户端卡
	dbOperator->DelOfflineMsgIfMoreThan300(objectID, 300);
    std::vector<Notification *> notifyInDb; // 数据库中的通知

    std::list<Notification *> notifysInManager; // notify manager中的通知
    m_notifymanager->getByUserid(objectID, &notifysInManager);
    dbOperator->GetNotification(objectID, &notifyInDb);

    // 如果一个消息同时存在于数据库和notify manager中, 这里就会
    // 重复发两次, 所以第一次发时保所有的id保存下来, 在第二个循环里面判断是否
    // 是重复的
    //
    std::unordered_set<uint64_t> sendedID;

    for (auto it = notifysInManager.begin(); it != notifysInManager.end(); ++it)
    {
        auto notify = *it;
        // 保存id
        sendedID.insert(notify->serialNumber());

        // 去掉这个因为会导致通知发两次
        // OutputResponse(new Notification(*notify));
    }

    for (size_t i = 0; i != notifyInDb.size(); i++)
    {
        Notification *notif = notifyInDb[i];

        bool finded = sendedID.find(notif->serialNumber()) != sendedID.end();
        // 如果id可以找到表示此id所表示的通知在上面的循环已经发过一次了
        if (finded)
        {
			printf("findedddddd:%llu\n",notif->serialNumber());
            delete notif;
            continue;
        }

        OutputResponse(new Notification(*notif));
        // 从数据库读出来的消息还要添加到notifyManager和notifytimeout中
        // 因为要处理超时重发, map 的特性可以保证不会添加相同的, 所以不用处理重复
        if (!m_notifymanager->insertNotify(notif))
        {
            delete notif;
            continue;
        }
        m_notifytimeout->addTimeout(notif->serialNumber());
    }
}

// ??? 这个和上面那个函数不是一样的吗???是的, 所以...直接调用上面的函数了
void NotifReqHandler::OnGetNotifWhenBackOnline(uint64_t objectID)
{
    OnGetNotifWhenLogin(objectID);
}

void NotifReqHandler::OnDumpNotif(uint64_t objectID) //用户注销, 把通知写入数据库
{
    (void)objectID;
    // do nothing 因为修改后第一件事就是把通知写入数据库
}

void NotifReqHandler::OnDeleteNotif(uint64_t objectID, uint64_t notifNumber) //通知消息收到回复
{
    // notify会先保存到数据库, 再保存到notifymanager中, 所以要分别从这两个地方
    // 删除
    if (!m_notifymanager->deleteNotification(notifNumber))
    {
        ::writelog(InfoType, "!!delete notify failed: %lu", notifNumber);
    }
    else
    {
        ::writelog(InfoType, "delete notify success: %lu", notifNumber);
    }

    if (!m_notifytimeout->removeTimeout(notifNumber))
    {
        ::writelog(InfoType, "delete notify timeout failed: %lu", notifNumber);
    }
    else
    {
        ::writelog(InfoType, "!!delete notify timeout success: %lu", notifNumber);
    }

    // FIXME 如果是sessional通知是否会保存到数据库中的, 但是这里没有处理
    // 不过没关系, 只是浪费了一次处理时间, 最好还是区分开普通的通知的
    // sessional通知
    dbOperator->DeleteNotification(objectID, notifNumber);
}

