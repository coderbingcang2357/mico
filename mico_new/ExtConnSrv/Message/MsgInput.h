#ifndef MSG_INPUT_H
#define MSG_INPUT_H

#include <stdint.h>
#include <netinet/in.h>

#include "Message/MsgWriter.h"
//#include "../Config/Config.h"
#include "config/shmconfig.h"
static const int MAX_RECVIVE_MESSAGES = 512;
class MessageQueuePosix;
class MsgInput
{
public:
    MsgInput();
    //MsgInput(char* addr, int sock);
    ~MsgInput();

    void Init(MessageQueuePosix *q, int sock);
    int  UdpRecv();
private:
    void Write(char *msgBuf, int msgLen, const sockaddr_in &sockAddr);
    bool Check(char *msgBuf, int msgLen);

    //MsgWriter m_msgWriter;  // 用来把消息写入共享内存
    MessageQueuePosix *m_queue;

    int m_sock;
    struct mmsghdr hdr[MAX_RECVIVE_MESSAGES];
    struct iovec bufvec[MAX_RECVIVE_MESSAGES];
    sockaddr_in srcaddr[MAX_RECVIVE_MESSAGES];
    char control[MAX_RECVIVE_MESSAGES][20];
    char buf[MAX_RECVIVE_MESSAGES][1024 * 2]; // 512 * 2k?
    // init hdr
    //sockaddr_in m_sockAddr;

    //char* m_msgBuf;
    //uint16_t m_msgLen;
};

#endif
