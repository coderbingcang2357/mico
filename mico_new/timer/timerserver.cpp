#include <sys/select.h>
#include <sys/time.h>
#include <assert.h>
#include <list>
#include <stdio.h>
#include "timerserver.h"
#include "thread/threadwrap.h"

class TimerInfo
{
public:
    static const uint32_t Every = 1;
    static const uint32_t Once = 0;

    TimerInfo(const std::shared_ptr<Proc> &p, uint32_t ms, uint32_t isevery)
        : _proc(p), _ms(ms), _isevery(isevery)
    {
    }

    //friend bool operator < (const TimerInfo &l, const TimerInfo &r)
    static bool Compare (const TimerInfo *l, const TimerInfo *r)
    {
        if (l->tv.tv_sec < r->tv.tv_sec)
            return true;
        else if (l->tv.tv_sec == r->tv.tv_sec)
            if (l->tv.tv_usec < r->tv.tv_usec)
                return true;
            else if (l->tv.tv_usec == r->tv.tv_usec)
                return l < r;
            else
                return false;
        else
            return false;
    }

    std::shared_ptr<Proc> _proc;
    timeval     tv;
    uint32_t _ms;
    uint32_t _isevery;
};

class TimerThread : public Proc
{
public:
    TimerThread(TimerServer *t) :  _quit(0), timer(t)
    {
    }

    void quit()
    {
        _quit = 1;
    }
    void run()
    {
        struct timeval tv;
        while (!_quit)
        {
            timer->timerMutex.lock();
            while (!_quit && timer->_timer.size() == 0)
            {
                timer->timerCondition.wait(&(timer->timerMutex));
            }
            timer->timerMutex.release();

            //printf("wake up.\n");
            tv.tv_sec = 0;
            tv.tv_usec = 200 * 1000; // 200 ms
            // select
            select(0, NULL, NULL, NULL, &tv);

            if (_quit)
            {
                break;
            }
            // 检查是否有要删除的定时器, 如果有就删除它们
            timer->doTimerRemove();

            timeval curtv;
            gettimeofday(&curtv, NULL);
            timer->processTimerout(curtv);
        }
    }

private:
    int _quit;
    TimerServer *timer;
};

TimerServer::TimerServer() : _timer(TimerInfo::Compare)
{
    assert(_timer.key_comp() != NULL);
    // create a thread
    threadproc = new TimerThread(this);
    // thread whill take ownership of threadproc
    thread = new Thread(threadproc);
}

TimerServer::~TimerServer()
{
    threadproc->quit();
    timerCondition.signal();
    delete thread;
    // should i lock???
    timerMutex.lock();
    std::set<TimerInfo *>::iterator it = _timer.begin();
    for (; it != _timer.end(); ++it)
        delete *it;
    timerMutex.release();
}

uint64_t TimerServer::runEvery(const std::shared_ptr<Proc> &p, uint32_t ms)
{
    TimerInfo *ti = new TimerInfo(p, ms, TimerInfo::Every);
    gettimeofday(&(ti->tv), NULL);
    ti->tv.tv_usec += ms * 1000;
    ti->tv.tv_sec += ti->tv.tv_usec / (1000 * 1000);
    ti->tv.tv_usec = ti->tv.tv_usec % (1000 * 1000);

    //printf("sec: %ld, usec: %ld\n", ti->tv.tv_sec, ti->tv.tv_usec);

    timerMutex.lock();
    std::pair<std::set<TimerInfo *>::iterator,bool> insres = _timer.insert(ti);
    //std::set<TimerInfo *>::iterator insres = _timer.insert(ti);
    //_timerInfoIterator.insert(std::make_pair(ti, insres.first));
    timerMutex.release();
    assert(insres.second == true);(void)insres;

    timerCondition.signal();
    return reinterpret_cast<uint64_t>(ti);
}

uint64_t TimerServer::runOnce(const std::shared_ptr<Proc> &p, uint32_t ms)
{
    TimerInfo *ti = new TimerInfo(p, ms, TimerInfo::Once);
    gettimeofday(&(ti->tv), NULL);
    ti->tv.tv_usec += ms * 1000;
    ti->tv.tv_sec += ti->tv.tv_usec / (1000 * 1000);
    ti->tv.tv_usec = ti->tv.tv_usec % (1000 * 1000);

    timerMutex.lock();
    std::pair<std::set<TimerInfo *>::iterator,bool> insres = _timer.insert(ti);
    //_timerInfoIterator.insert(std::make_pair(ti, insres.first));
    timerMutex.release();
    assert(insres.second == true);(void)insres;

    timerCondition.signal();
    return reinterpret_cast<uint64_t>(ti);
}

// 把id代表的定时器加到删除队列, 延时删除
void TimerServer::removeTimer(uint64_t id)
{
    assert(id != 0);
    TimerInfo *ti = reinterpret_cast<TimerInfo *>(id);
    // 删除先添加到一个列表里面, 在select下一次超时时删除它们
    removedMutex.lock();
    //_removed.push_back(ti);
    _removed.insert(ti);
    removedMutex.release();
}

void TimerServer::processTimerout(const struct timeval &tv)
{
    TimerInfo ti(NULL, 0, TimerInfo::Every);
    ti.tv = tv;
    //printf("sec: %ld, usec: %ld\n", tv.tv_sec, tv.tv_usec);

    timerMutex.lock();

    std::set<TimerInfo *>::iterator lower = _timer.lower_bound(&ti);
    // no timeout
    if (lower == _timer.begin())
    {
        timerMutex.release();
        return;
    }
    std::set<TimerInfo *>::iterator begin = _timer.begin();
    std::list<TimerInfo *> timeout(std::distance(begin, lower));// TODO fixme 这里distance是线性复杂度
    copy(begin, lower, timeout.begin());
    _timer.erase(begin, lower);

    timerMutex.release();

    std::list<TimerInfo *>::iterator timeoutiter = timeout.begin();
    for (; timeoutiter != timeout.end(); ++timeoutiter)
    {
        // run every re install timer
        if ((*timeoutiter)->_isevery == TimerInfo::Every)
        {
            TimerInfo *newti = *timeoutiter;
            newti->tv = tv;
            newti->tv.tv_usec += newti->_ms * 1000;
            newti->tv.tv_sec += newti->tv.tv_usec / (1000 * 1000);
            newti->tv.tv_usec = newti->tv.tv_usec % (1000 * 1000);
            // ..
            timerMutex.lock();
            _timer.insert(newti);
            timerMutex.release();
            // or post to queue ??
            if ((*timeoutiter)->_proc)
                (*timeoutiter)->_proc->run();
        }
        else
        {
            if ((*timeoutiter)->_proc)
                (*timeoutiter)->_proc->run();
            //delete *timeoutiter;
            this->removeTimer(uint64_t(*timeoutiter));
        }
    }
}

// 检查是否有要删除的定时器, 如果有就删除它们
// 此函数会在select超时时每次都调用一次
void TimerServer::doTimerRemove()
{
    removedMutex.lock();
    if (_removed.size() > 0)
    {
        timerMutex.lock();
        auto it = _removed.begin();
        auto end = _removed.end();
        for (; it != end; ++it)
        {
            _timer.erase(*it);
            delete *it;
        }
        _removed.clear();
        timerMutex.release();
    }
    removedMutex.release();
}

TimerServer *TimerServer::server()
{
    // TODO Fix me, 多线程不安全
    static TimerServer  *timerserver = 0;
    if (timerserver == NULL)
            timerserver = new TimerServer;

    return timerserver;
}

