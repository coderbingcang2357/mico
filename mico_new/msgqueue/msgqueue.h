#ifndef MSGQUEUE_H
#define MSGQUEUE_H
#include <list>
#include <atomic>
#include "thread/mutexwrap.h"
#include "thread/mutexguard.h"
#include "thread/conditionwrap.h"

template<typename T>
class BlockMessageQueue
{
public:
    BlockMessageQueue() : m_queuesize(0) {
        m_isrun = true;
    }
    ~BlockMessageQueue(){}
    void put(T t)
    {
        {
            MutexGuard guart(&m_queueMutex);
            m_queue.push_back(t);
            m_queuesize++;
        }
        m_notEmpty.signal();
    }

//    void put(std::list<T> *t)
//    {
//        if (t->size() <= 0)
//            return;

//        {
//            MutexGuard guart(&m_queueMutex);
//            m_queuesize += t->size();
//            m_queue.splice(m_queue.end(), *t);
//        }
//        m_notEmpty.signal();
//    }

    T get()
    {
        m_queueMutex.lock();
        while (m_isrun && m_queuesize == 0)
        {
            m_notEmpty.wait(&m_queueMutex);
        }

        if (!m_isrun)
        {
            m_queueMutex.release();
            return 0;
        }

        T t = m_queue.front();
        m_queue.pop_front();
        m_queuesize--;
        m_queueMutex.release();
        return t;
    }

    bool hasMessage()
    {
        MutexGuard guart(&m_queueMutex);
        return m_queuesize != 0;
    }

    void quit()
    {
        this->m_isrun = false;
        m_notEmpty.signalall();
    }

private:
    int m_queuesize;// c++ 03 m_queue.size()是线性复杂志度, 所以添加了这个成员
    std::list<T> m_queue; 
    MutexWrap m_queueMutex;
    ConditionWrap m_notEmpty;
    std::atomic<bool> m_isrun;
};
#endif 
