#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <cassert>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

#include "MsgInput.h"
#include "protocoldef/protocol.h"
#include "util/logwriter.h"
#include "msgqueue/ipcmsgqueue.h"
#include "msgqueue/posixmsgqueue.h"
#include "unistd.h"
#include "protocoldef/protocol.h"

MsgInput::MsgInput()
{
    //m_msgBuf = NULL;
}

//MsgInput::MsgInput(char* addr, int sock)
//: m_msgWriter(addr)
//, m_sock(sock)
//{
//    m_msgBuf = new char[MAX_MSG_SIZE];
//}

MsgInput::~MsgInput()
{
    //delete [] m_msgBuf;
}

void MsgInput::Init(MessageQueuePosix *q, int sock)
{
    m_queue = q;

    m_sock = sock;
    //m_msgWriter.Init(addr);
    //if(m_msgBuf != NULL)
    //    delete m_msgBuf;
    //m_msgBuf = new char[MAX_MSG_SIZE];
    socklen_t sockAddrLen = sizeof (sockaddr_in);
    for (int i = 0; i < MAX_RECVIVE_MESSAGES; i++)
    {
        hdr[i].msg_len = 0;
        bufvec[i].iov_base = buf[i];
        bufvec[i].iov_len = 1024 * 2;
        hdr[i].msg_hdr.msg_iov = &bufvec[i];
        hdr[i].msg_hdr.msg_iovlen = 1;
        hdr[i].msg_hdr.msg_name = srcaddr + i;
        hdr[i].msg_hdr.msg_namelen = sockAddrLen;
        hdr[i].msg_hdr.msg_control = control[i];
        hdr[i].msg_hdr.msg_controllen = 20;
        hdr[i].msg_hdr.msg_flags = 0;
    }
}

//UDP方式接收客户端消息
int MsgInput::UdpRecv()
{
    int cnt = recvmmsg(m_sock, hdr, MAX_RECVIVE_MESSAGES, MSG_DONTWAIT, 0);
    assert(cnt > 0);
    ::writelog(InfoType, "recv cnt: %d", cnt);
    int msglen;
    uint16_t cmdID = 0;
    for (int i = 0; i < cnt; i++)
    {
        msglen = hdr[i].msg_len;
        hdr[i].msg_len = 0;
        hdr[i].msg_hdr.msg_flags = 0;
        char *rbuf = buf[i];
        if (Check(rbuf, msglen))
        {
            memcpy(&cmdID, rbuf + 3, sizeof(cmdID));
            writelog(InfoType, "recv: %s:%d, cmd: 0x%04x, len: %d",
                    inet_ntoa(srcaddr[i].sin_addr),
                    ntohs(srcaddr[i].sin_port),
                    cmdID,
                    msglen);
            //static int all = 0;
            //static int drop = 0;
            //all++;
            //if ((rand() % 1000) > 700)
            //{
            //    ::writelog("drop a package.!!!!!!");
            //    //drop++;
            //}
            //else
            //{
            Write(rbuf, msglen, srcaddr[i]);
            //}
            //::writelog(InfoType, "all: %d, drop: %d\n", all, drop);
        }
        else
        {
            ::writelog(InfoType, "error package.");
        }
    }

    return cnt;
}

//校验消息, 检查数据包的包头少包尾
bool MsgInput::Check(char *msgBuf, int msgLen)
{
    if(msgLen + sizeof(sockaddr_in) > MAX_MSG_SIZE)
    {
        ::writelog("Received message size too large.");
        return false;
    }

    uint8_t msgPrefix = *msgBuf;
    uint8_t msgSuffix = *(msgBuf + msgLen - 1);
    if((msgPrefix == 0xF0 && msgSuffix == 0xF1)
        || (msgPrefix == 0xE0 && msgSuffix == 0xE1))
        return true;
    else
        return false;
}

//将数据写入共享内存
void MsgInput::Write(char *msgBuf, int msgLen, const sockaddr_in &sockAddr)
{
    uint16_t len = msgLen + sizeof(sockAddr);
    char buf[len];
    memcpy(buf, &sockAddr, sizeof(sockAddr));
    memcpy(buf + sizeof(sockAddr), msgBuf, msgLen);

    int ret;
    uint32_t resendcount = 1;
again:
    if ((ret = m_queue->put(buf, len)) == SuccessQueue)
    {
    }
    else if (ret == FullQueue)
    {
        if (resendcount >= MAX_RETRY_WHEN_MSGQUEUE_FULL)
        {
            ::writelog(InfoType,
                    "Message queue full try %ds failed.",
                    resendcount);
            //assert(false);
            return;
        }

        if (resendcount++ == 1)
            ::writelog(InfoType, "Message queue full: %s", m_queue->name().c_str());

        usleep(1*1000);
        goto again;
    }
    else if (ret == ErrorQueue)
    {
        ::writelog(InfoType,
            "Message queue write failed: %s", strerror(errno));
    }
    else
    {
        assert(false);
    }
}
