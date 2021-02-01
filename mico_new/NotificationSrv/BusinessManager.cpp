#include "BusinessManager.h"
#include "util/logwriter.h"
//#include "../NotifRequest.h"
//#include "../NotifResponse.h"
//#include "../NotifReqReader.h"
//#include "../NotifResWriter.h"
#include "NotifReqHandler.h"
#include "Database/DBOperator.h"
#include "msgqueue/ipcmsgqueue.h"
#include "Message/MsgWriter.h"
#include "Message/Message.h"

BusinessManager::BusinessManager()
    : m_resWriter(::msgqueueNotifySrvToLogical())
{
    m_dboperator = new DBOperator;
    m_reqhandler = new NotifReqHandler(&m_notifymanager,
                                       &m_notifytimeout,
                                       m_dboperator);
}

BusinessManager::~BusinessManager()
{
    delete m_dboperator;
    delete m_reqhandler;
}

int BusinessManager::HandleIpcRequest(Message *notifReq) // 处理从 LogicalSrv 通过共享内存发过来的数据
{
    std::vector<Notification *>& vNotifRes = m_reqhandler->Execute(notifReq); // 处理请求
    if(!vNotifRes.empty())
    {
        for(size_t i = 0; i != vNotifRes.size(); i++)
        {

            //::writelog(InfoType, "send notify: %lu", vNotifRes[i]->serialNumber());
            Message msg;
            msg.setCommandID(CMD::CMD_SEND_NOTIF);
            msg.appendContent(vNotifRes[i]);
            this->writeMessage(msg);
        }
    }
    m_reqhandler->ClearResponses();

    // 测试代码, 测试和logicalsrv的进程通信
    //NotifResponse notifRes;
    //char test[] = "::This is a nofity test";
    //notifRes.SetMsg(test, sizeof(test));
    //resWriter.Write(notifRes, ::msgqueueNotifySrvToLogical());

    return 0;
}

void BusinessManager::notifyTimeout(uint64_t notifyid, int times)
{
    // resend or remove from NotifyManager 
    Notification *notf = m_notifymanager.getByNotifyID(notifyid);
    //assert(notf);
    if (!notf)
    {
        // error
        ::writelog(InfoType, "notify missed: %lu", notifyid);
        return;
    }
    if (times < 3) // resend
    {
        // 
        //::writelog(InfoType, "resend notify: %lu", notifyid);
        //Message *msg = new Message;
        Message msg;
        msg.setCommandID(CMD::CMD_SEND_NOTIF);
        msg.appendContent(notf);
        this->writeMessage(msg);
    }
    else
    {
        // remove from notifyManager and remove timeout
        m_notifymanager.deleteNotification(notifyid);
        m_notifytimeout.removeTimeout(notifyid);
    }
}

void BusinessManager::writeMessage(const Message &msg)
{
    m_resWriter.Write(const_cast<Message*>(&msg));
}

