#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "onlineinfo/mainthreadmsgqueue.h"
#include "util/logwriter.h"
#include "signalthread.h"
#include "thread/threadwrap.h"

namespace
{
class SigThread : public Proc
{
public:
    // 等待信号, 收到SIGUSR1后让主线程队列退出, 这样就可以退出进程一了.
    void run()
    {
        sigset_t waitset;
        sigemptyset(&waitset);
        sigaddset(&waitset, SIGUSR1);
        int sig;
        ::writelog("wait signal.");
        int r = sigwait(&waitset, &sig);
        if (r != 0)
        {
            ::writelog(InfoType, "error num: %d, %s", r, strerror(r));
        }
        assert(sig == SIGUSR1);
        ::writelog("sigusr1 exit.");
        MainQueue::exit();
    }
};
}

SignalThread::SignalThread()
{
    sigset_t set, oldset;
    sigemptyset(&set);
    sigemptyset(&oldset);
    sigaddset(&set, SIGUSR1);
    if (sigprocmask(SIG_BLOCK, &set, &oldset) != 0)
    {
        ::writelog("sigprocmask error.");
        exit(0);
    }

    // thread whill take ownership of SigThread
    m_t = new Thread(new SigThread);
}

SignalThread::~SignalThread()
{
    delete m_t;
}

