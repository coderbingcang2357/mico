#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <assert.h>
#include <string.h>
#include "Message.h"
#include "MsgWriter.h"
//#include "../Util/Util.h"
#include "util/util.h"
#include "config/shmconfig.h"
#include "msgqueue/ipcmsgqueue.h"
#include "util/logwriter.h"
#include "msgqueue/posixmsgqueue.h"
#include "protocoldef/protocol.h"

//MsgWriter::MsgWriter()
//{
//    
//}
//
//MsgWriter::MsgWriter(char* addr)
//{
//    m_ipcMemory.Init(addr, IpcMemory::IT_Msg);
//}
//
//MsgWriter::~MsgWriter()
//{
//    
//}
//
//void MsgWriter::SetWriteAddr(char* addr)
//{
//    m_ipcMemory.Init(addr, IpcMemory::IT_Msg);
//}
//
//#include "../Config/Debug.h"
//int MsgWriter::Write(Message* msg)
//{
//    if(msg == NULL)
//        return -1;
//
//    if(m_ipcMemory.Full())
//        return -1;
//
//    int msgLen = msg->Len(); 
//    sockaddr_in sockAddr = msg->SockAddr();
//    uint16_t len = msgLen + sizeof(sockAddr);
//    char* buf = new char[len];
//    memcpy(buf, &sockAddr, sizeof(sockAddr));
//    memcpy(buf + sizeof(sockAddr), msg->Buf(), msgLen);
//
//    int ret = m_ipcMemory.PushIpcPacket(buf, len);    
//    delete [] buf;
//    return ret;
//}


MsgWriterPosixMsgQueue::MsgWriterPosixMsgQueue(MessageQueuePosix *q)
        : m_queue(q)
{
}

MsgWriterPosixMsgQueue::~MsgWriterPosixMsgQueue()
{
}

int MsgWriterPosixMsgQueue::Write(Message* msg)
{
    if(msg == NULL)
        return -1;

    int res;

    std::vector<char> msgbuf;
    msg->pack(&msgbuf);
    //int msgLen = msg->Len();
    sockaddrwrap &sockAddr = msg->sockerAddress();
    uint16_t len = msgbuf.size() + sizeof(sockAddr);
    char buf[len];

    memcpy(buf, &sockAddr, sizeof(sockAddr));
    //memcpy(buf + sizeof(sockAddr), msg->Buf(), msgLen);
    memcpy(buf + sizeof(sockAddr), &msgbuf[0], msgbuf.size());

    uint32_t count = 1;
again:
    if ((res = m_queue->put(buf, len)) == SuccessQueue)
    {
        return 0;
    }
    else if (res == FullQueue)
    {
        if (count > MAX_RETRY_WHEN_MSGQUEUE_FULL)
        {
            ::writelog(InfoType, "Queue full try 10 times failed. %s", m_queue->name().c_str());
            //assert(false);
            return -1;
        }
        if (count++ == 1)
            ::writelog(InfoType, "Queue full, %s", m_queue->name().c_str());
        usleep(1000); // 1 ms
        goto again;
    }
    else
    {
        // error
        ::writelog(InfoType, "Queue put error, %s, error: %s, len: %d",
                   m_queue->name().c_str(),
                   strerror(errno), len);
        //assert(false);
        return -1;
    }
}
