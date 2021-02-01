#ifndef MESSAGERECEIVER__H
#define MESSAGERECEIVER__H

class Thread;
class MessageReceiver
{
public:
    MessageReceiver();
    ~MessageReceiver();
    int create();

private:
    void run();
    void readMessage();

    Thread *m_thread;
    bool m_isrun;
    friend class MsgRecvProc;
};
#endif
