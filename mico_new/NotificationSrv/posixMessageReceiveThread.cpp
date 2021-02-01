#include <unistd.h>
#include "posixMessageReceiveThread.h"
#include "mainqueue.h"
#include "Message/Notification.h"
#include "msgqueue/ipcmsgqueue.h"
//#include "NotifReqReader.h"
//#include "NotifRequest.h"
#include "queuemessage.h"
#include "Message/Message.h"
#include "Message/MsgReader.h"

extern bool running;

PosixMessageReceiveThread::PosixMessageReceiveThread()
{
    m_thread = new Thread([this](){
        this->run();
    });
}

PosixMessageReceiveThread::~PosixMessageReceiveThread()
{
    delete m_thread;
}

void PosixMessageReceiveThread::run()
{
    //NotifReqReader reqReader;
    //NotifRequest *notifReq = new NotifRequest;
    MessageQueuePosix *p = ::msgqueueLogicalToNotifySrv();
    MsgReaderPosixMsgQueue r(p);
    while (running)
    {
        //
        // get message and post to main thread

        Message *msg;
        if((msg = r.read()) == 0)
        {
            usleep(10 * 1000); // 10ms
        }
        else
        {
            // post to main thread
            //
            NewNotifyRequestMessage *nrm = 
                                    new NewNotifyRequestMessage(msg);
            ::postMessage(nrm);
            //notifReq = new NotifRequest;
        }
    }

   // delete notifReq;
}
