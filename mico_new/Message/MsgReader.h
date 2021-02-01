#ifndef MSG_READER_H
#define MSG_READER_H

class MessageQueuePosix;

class Message;
//class MsgReader
//{
//public:
//    MsgReader();
//    MsgReader(char* addr);
//    ~MsgReader();
//
//    void SetReadAddr(char* addr);
//    int Read(Message* msg);
//private:
//    IpcMemory m_ipcMemory;
//};

class MsgReaderPosixMsgQueue
{
public:
    MsgReaderPosixMsgQueue(MessageQueuePosix *queue = 0);
    ~MsgReaderPosixMsgQueue();

    void setReadQueue(MessageQueuePosix *queue);
    Message *read();
private:
    MessageQueuePosix *m_queue;
};
#endif
