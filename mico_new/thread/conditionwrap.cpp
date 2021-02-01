#include <pthread.h>
#include <assert.h>
#include "conditionwrap.h"
#include "mutexwrap.h"

#define D() ((CondWrapData *)d)

class CondWrapData
{
public:
    pthread_cond_t cond;
};

ConditionWrap::ConditionWrap() : d(new CondWrapData)
{
    pthread_cond_init(&(D()->cond), NULL);
}

ConditionWrap::~ConditionWrap()
{
    pthread_cond_destroy(&(D()->cond));
    delete D();
    d = 0;
}

void ConditionWrap::signal()
{
    pthread_cond_signal(&(D()->cond));
}

void ConditionWrap::wait(MutexWrap *mutex)
{
    int res = pthread_cond_wait(&(D()->cond), mutex->mutex());
    assert(res == 0);(void)res;
}

void ConditionWrap::signalall()
{
    pthread_cond_broadcast(&(D()->cond));
}
