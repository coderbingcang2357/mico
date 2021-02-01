#ifndef SIGNALTHREAD__H
#define SIGNALTHREAD__H

class Thread;
// 处理SIGUSR1信号, 用来退出进程
// 这个类**一定要在任何线程创建之前调用**
class SignalThread
{
public:
    SignalThread();
    ~SignalThread();

private:
    Thread *m_t;
};
#endif
