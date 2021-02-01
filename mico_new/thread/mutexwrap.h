#ifndef MUTEXWRAP_H
#define MUTEXWRAP_H

//struct pthread_mutex_t;
#include <pthread.h>

class MutexWrap
{
public:
    MutexWrap();
    ~MutexWrap();
    bool lock();
    bool trylock();
    bool release();
private:
    pthread_mutex_t *mutex();

    void *d;
    friend class ConditionWrap;
};
#endif

