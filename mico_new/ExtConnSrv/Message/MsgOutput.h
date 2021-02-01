#ifndef MSG_OUTPUT_H  
#define MSG_OUTPUT_H

#include <stdint.h>
#include <netinet/in.h>

#include "Message/MsgReader.h"
//#include "../Config/Config.h"
#include "config/shmconfig.h"

class MessageQueuePosix;

class MsgOutput
{
public:
    MsgOutput();
    //MsgOutput(char* addr, int sock);
    ~MsgOutput();
    
    void Init(MessageQueuePosix *q, int sock);
    int UdpSend();
    int Read();
private:
    //MsgReader m_msgReader;  // 用来从共享内存中读数据
    MessageQueuePosix *m_queue;
    int m_sock;
    sockaddr_in m_sockAddr;  
    
    char* m_msgBuf;
    uint16_t m_msgLen;   
};


#endif
