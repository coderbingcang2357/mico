#include "mutexwrap.h"
#include "mutexguard.h"

MutexGuard::MutexGuard(MutexWrap *mutex) : m_mutex(mutex)
{
    m_mutex->lock();
}

MutexGuard::~MutexGuard()
{
    m_mutex->release();
}
