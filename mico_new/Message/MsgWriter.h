#ifndef MSG_WRITER_H
#define MSG_WRITER_H

class Message;
class MessageQueuePosix;

//class MsgWriter
//{
//public:
//    MsgWriter();
//    MsgWriter(char* addr);
//    ~MsgWriter();
//
//    void SetWriteAddr(char* addr);
//    int Write(Message* msg);
//private:   
//    IpcMemory m_ipcMemory;
//};

class MsgWriterPosixMsgQueue
{
public:
    MsgWriterPosixMsgQueue(MessageQueuePosix *q = 0);
    ~MsgWriterPosixMsgQueue();

    void setQueue(MessageQueuePosix *q) { m_queue = q;}
    int Write(Message* msg);
private:   
    MessageQueuePosix *m_queue;
};
#endif
