#ifndef THREAD_POOL__H
#define THREAD_POOL__H
#include <deque>
#include <vector>
#include "threadwrap.h"
#include "mutexwrap.h"
#include "conditionwrap.h"

class ThreadPool
{
public:
    ThreadPool(int maxq = 100000);
    ~ThreadPool();

    void init(int threadcount);
    void addWorker(Proc *proc);

    static void thredProc(ThreadPool *tp);
private:
    std::deque<Proc *> m_worker;
    std::vector<Thread *> m_thread;
    MutexWrap m_mutex;
    ConditionWrap m_notEmpty;
    ConditionWrap m_notFull;
    int m_isrun;
    int m_maxQueue;
};

#endif
