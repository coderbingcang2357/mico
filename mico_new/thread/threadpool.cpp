#include <stdio.h>
#include <assert.h>
#include "threadpool.h"

ThreadPool::ThreadPool(int maxq) : m_isrun(1), m_maxQueue(maxq)
{
}

ThreadPool::~ThreadPool()
{
    m_isrun = 0;
    m_notEmpty.signalall();
    m_notFull.signalall();

    std::vector<Thread *>::iterator it = m_thread.begin();
    for (; it != m_thread.end(); ++it)
        delete (*it);
    // clear remain messge or execute it ?
    printf("remain: %lu\n", m_worker.size());
    std::deque<Proc *>::iterator itwk = m_worker.begin();
    for (; itwk != m_worker.end(); ++itwk)
        delete (*itwk);
    //printf("quit.\n");
}

void ThreadPool::thredProc(ThreadPool *threadpool)
{
    while (threadpool->m_isrun)
    {
        threadpool->m_mutex.lock();
        while (threadpool->m_worker.size() == 0 && threadpool->m_isrun)
        {
            //printf("wait a worker.\n");
            threadpool->m_notEmpty.wait(&(threadpool->m_mutex));
        }

        if (!threadpool->m_isrun)
        {
            threadpool->m_mutex.release();
            break;
        }

        assert (threadpool->m_worker.size() != 0);

        Proc *proc = threadpool->m_worker.back();
        threadpool->m_worker.pop_back();
        threadpool->m_mutex.release();
        threadpool->m_notFull.signal();
        proc->run();
        delete proc;
    }
}

void ThreadPool::init(int threadcount)
{
    for (int i = 0; i < threadcount; i++)
    {
        Thread *th = new Thread([this](){
            ThreadPool::thredProc(this);
        });
        m_thread.push_back(th);
    }
}

void ThreadPool::addWorker(Proc *proc)
{
    m_mutex.lock();
    //if (m_worker.size() >= static_cast<unsigned long>(m_maxQueue))
    if (m_worker.size() >= static_cast<std::deque<Proc*>::size_type>(m_maxQueue))
    {
        //printf("full noow.\n");
        m_notFull.wait(&m_mutex);
    }

    if (!m_isrun)
        return;

    // add proc to queue
    m_worker.push_front(proc);
    m_mutex.release();
    m_notEmpty.signal();
}
