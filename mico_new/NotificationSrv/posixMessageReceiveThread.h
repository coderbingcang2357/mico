#ifndef POSIXMESSAGERECEIVETHREAD_H
#define POSIXMESSAGERECEIVETHREAD_H
#include "thread/threadwrap.h"
class PosixMessageReceiveThread
{
public:
    PosixMessageReceiveThread();
    ~PosixMessageReceiveThread();
private:
    void run();

    Thread *m_thread;
};

#endif
