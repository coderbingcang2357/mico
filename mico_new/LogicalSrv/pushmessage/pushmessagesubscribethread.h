#include "thread/threadwrap.h"

#ifndef PUSHMESSAGESUBSCRIBETHREAD__H___
#define PUSHMESSAGESUBSCRIBETHREAD__H___

class PushMessageSubscribeThread
{
public:
    PushMessageSubscribeThread();
    ~PushMessageSubscribeThread();

private:
    bool m_isrun;
    Thread *m_th;
};
#endif
