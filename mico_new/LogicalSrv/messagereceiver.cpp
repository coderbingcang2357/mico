#include <sys/select.h>
#include <assert.h>
#include "messagereceiver.h"
#include "thread/threadwrap.h"
#include "onlineinfo/mainthreadmsgqueue.h"
#include "Message/MsgReader.h"
#include "msgqueue/ipcmsgqueue.h"
#include "msgqueue/posixmsgqueue.h"
#include "Message/Message.h"
#include "newmessagecommand.h"

class MsgRecvProc : public Proc
{
public:
    MsgRecvProc(MessageReceiver *m) : t(m){}
    void run()
    {
        t->run();
    }

private:
    MessageReceiver *t;
};

MessageReceiver::MessageReceiver() : m_isrun(true)
{
}

MessageReceiver::~MessageReceiver()
{
    m_isrun = false;
}

int MessageReceiver::create()
{
    MsgRecvProc *proc = new MsgRecvProc(this);
    // thread will take ownership of proc
    m_thread = new Thread(proc);
    return 0;
}

void MessageReceiver::run()
{
    // read message from ipc
    MessageQueuePosix *notifyToLogical = msgqueueNotifySrvToLogical();
    MessageQueuePosix *relayToLogical = msgqueueRelaySrvToLogical();

    int notifyToLogicalFD = notifyToLogical->fd();
    int relayToLogicalFD = relayToLogical->fd();

    fd_set readset;
    int maxfd =  std::max(notifyToLogicalFD, relayToLogicalFD);
    while (m_isrun)
    {
        FD_ZERO(&readset);
        FD_SET(notifyToLogicalFD, &readset);
        FD_SET(relayToLogicalFD, &readset);
        timeval timeout = {0, 100 * 1000};// 100 ms
        int ret = select(maxfd + 1, &readset, 0, 0, &timeout);
        if(ret == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            else
            {
                assert(false);
                exit(-1);
            }
        }
        else if (ret == 0)
        {
            // timeout
        }
        else
        {
            readMessage();
        }
    }
}

void MessageReceiver::readMessage()
{
    MsgReaderPosixMsgQueue notifyReader(::msgqueueNotifySrvToLogical());
    MsgReaderPosixMsgQueue relayReader(::msgqueueRelaySrvToLogical());

    std::list<ICommand *> msglist;

    //while (m_isrun)
    {
        //bool busy = false;
        Message *msg;// = new Message;

        if((msg = notifyReader.read()) != 0)
        {
            //busy = true;
            InternalMessage *imsg
                = new InternalMessage(msg,
                    InternalMessage::FromNotifyServer,
                    InternalMessage::Unused);
            //msglist.push_back(new NewMessageCommand(imsg));
            MainQueue::postCommand(new NewMessageCommand(imsg));
        }

        if((msg = relayReader.read()) != 0) // 从replay的共享内存中读数据
        {
            //busy = true;
            InternalMessage *imsg
                = new InternalMessage(msg,
                    InternalMessage::FromRelayServer,
                    InternalMessage::Unused);
            //msglist.push_back(new NewMessageCommand(imsg));

            MainQueue::postCommand(new NewMessageCommand(imsg));
        }

        // 如果读到了消息则一直读, 直到队列里面没有消息时才返回
        // 每100个消息发一次到主线程
        //if (busy)
        //{
        //    busy = false;
        //    if (msglist.size() >= 100)
        //    {
        //        getMainThreadMsgQueue()->put(&msglist);
        //        msglist.clear();
        //    }
        //}
        //else
        //{
        //    if (msglist.size() > 0)
        //    {
        //        getMainThreadMsgQueue()->put(&msglist);
        //        msglist.clear();
        //        return;
        //    }
        //}
    }
    //if (!m_isrun)
    //{
        // msglist里面可能还有消息没有发出去, 但是m_isrun为false时说明是进程主动
        // 退出了.
    //}
}

