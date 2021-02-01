#ifndef BUSINESS_MANAGER_H
#define BUSINESS_MANAGER_H

#include "../Message/MsgInput.h"
#include "../Message/MsgOutput.h"

class BusinessManager
{
public:
    BusinessManager();
    ~BusinessManager();
    
    int Init();
    int HandleMsgInput();
    int HandleMsgOutput();
private:
    MsgInput m_msgInput;    // 从客户端收到消息
    MsgOutput m_msgOutput;  // 发消息到客户端
};

#endif
