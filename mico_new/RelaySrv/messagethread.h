#ifndef MESSAGETHREAD__H
#define MESSAGETHREAD__H
#include "thread/threadwrap.h"
class MessageThread
{
public:
    MessageThread();
    ~MessageThread();
    bool create();
    void quit() {m_isrun = false;}

private:
    void run();

    Thread *m_thread;
    bool m_isrun;
};

#endif
