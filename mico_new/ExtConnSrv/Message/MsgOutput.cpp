#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <cassert>
#include <sys/mman.h>
#include <arpa/inet.h>
#include "MsgOutput.h"
#include "protocoldef/protocol.h"
#include "msgqueue/posixmsgqueue.h"
#include "util/logwriter.h"

MsgOutput::MsgOutput()
{
    m_msgBuf = NULL;
}


//MsgOutput::MsgOutput(char* addr, int sock)
//    : m_msgReader(addr)
//{
//    m_sock = sock;
//    m_msgBuf = new char[MAX_MSG_SIZE];
//}

MsgOutput::~MsgOutput()
{
    delete [] m_msgBuf;
}

void MsgOutput::Init(MessageQueuePosix *q, int sock)
{
    m_queue = q;
    m_sock = sock;
    //m_msgReader.Init(addr);
    if(m_msgBuf != NULL)
        delete [] m_msgBuf;
    m_msgBuf = new char[MAX_MSG_SIZE];
}

//UDP方式发送消息给客户端
int MsgOutput::UdpSend()
{
    int ret = sendto (m_sock, m_msgBuf, m_msgLen, 0, (sockaddr*) &m_sockAddr, sizeof (m_sockAddr));
    uint8_t msgPrefix = 0;
    memcpy(&msgPrefix, m_msgBuf, sizeof(msgPrefix));
    uint16_t cmdID = 0;
    memcpy(&cmdID, m_msgBuf + 3, sizeof(cmdID));

    ::writelog(InfoType, "send: %s:%d, cmd: 0x%04x, len: %d",
                inet_ntoa(m_sockAddr.sin_addr),
                ntohs(m_sockAddr.sin_port),
                cmdID, m_msgLen);
    uint8_t msgSuffix = *(m_msgBuf + m_msgLen - 1);
    if((msgPrefix == 0xF0 && msgSuffix == 0xF1)
        || (msgPrefix == 0xE0 && msgSuffix == 0xE1))
    {
        ;
    }
    else
    {
        printf("error message send to client\n");
    }

    return ret;
}

//从共享内存中读取数据
int MsgOutput::Read()
{
    // buf的大小必需要比创建队列设置的消息大小大
    char buf[MAX_MSG_SIZE];
    int len = sizeof(buf);
    int res;
    if ((res = m_queue->get(buf, &len)) == SuccessQueue)
    {
        m_msgLen = len - sizeof(m_sockAddr);
        memcpy(&m_sockAddr, buf, sizeof(m_sockAddr));
        memcpy(m_msgBuf, buf + sizeof(m_sockAddr), m_msgLen);

        return 0;
    }
    else if (res == EmptyQueue)
    {
        return -1;
    }
    else if (res == ErrorQueue)
    {
        ::writelog(InfoType, "msgqueue read error, %s", strerror(errno));
        return -1;
    }
    else
    {
        assert(false);
        return -1;
    }
}

