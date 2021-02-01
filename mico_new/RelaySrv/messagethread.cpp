#include <sys/select.h>
#include "messagethread.h"
#include "mainqueue.h"
#include "msgqueue/ipcmsgqueue.h"
#include "msgqueue/posixmsgqueue.h"
#include "Message/Message.h"
#include "Message/MsgReader.h"
#include "relayinternalmessage.h"
#include "util/logwriter.h"

MessageThread::MessageThread() :
    m_thread(0),
    m_isrun(true)
{
}

MessageThread::~MessageThread()
{
    this->quit();
    delete m_thread;
}

bool MessageThread::create()
{
    m_thread = new Thread([this](){
        this->run();
    });
    return true;
}

void MessageThread::run()
{
    ::writelog("start wait msg");
    MessageQueuePosix *logicToRelay = ::msgqueueLogicalToRelaySrv();
    fd_set readset;
    int maxfd = logicToRelay->fd();
    int fd = logicToRelay->fd();
    while (m_isrun)
    {
        FD_ZERO(&readset);
        FD_SET(fd, &readset);
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
                //assert(false);
                ::writelog(InfoType, "select error: %s", strerror(errno));
                exit(-1);
            }
        }
        else if (ret == 0)
        {
            // timeout
        }
        else
        {
            MsgReaderPosixMsgQueue msgReader(logicToRelay);
            while (m_isrun)
            {
                Message *msg  = msgReader.read();
                if (msg != 0)
                {
                    MessageFromOtherServer *newmsg = new MessageFromOtherServer(msg);
                    MainQueue::postMsg(newmsg);
                }
                else
                {
                    break;
                }
            }
        }
    }
}

