#include "mutexwrap.h"
#include <assert.h>

#define D() ((MutexWrapData *)d)
class MutexWrapData
{
public:
    pthread_mutex_t mutex;
};

MutexWrap::MutexWrap() : d(new MutexWrapData)
{
    if (pthread_mutex_init(&(D()->mutex), NULL) != 0)
        assert(false);
}

MutexWrap::~MutexWrap()
{
    pthread_mutex_destroy(&(D()->mutex));
    delete D();
    d = 0;
}

bool MutexWrap::lock()
{
    if (pthread_mutex_lock(&(D()->mutex)) == 0)
        return true;
    else
    {
        assert(false);
        return false;
    }
}

bool MutexWrap::trylock()
{
    if (pthread_mutex_trylock(&(D()->mutex)) == 0)
        return true;
    else
    {
        assert(false);
        return false;
    }
}

bool MutexWrap::release()
{
    if (pthread_mutex_unlock(&(D()->mutex)) == 0)
        return true;
    else
    {
        assert(false);
        return false;
    }
}

pthread_mutex_t *MutexWrap::mutex()
{
    return &(D()->mutex);
}
