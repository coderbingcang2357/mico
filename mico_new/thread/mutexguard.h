#ifndef MUTEX_GUARD__H
#define MUTEX_GUARD__H

class MutexWrap;
class MutexGuard
{
public:
    MutexGuard(MutexWrap *);
    ~MutexGuard();

private:
    MutexWrap *m_mutex;
};
#endif
