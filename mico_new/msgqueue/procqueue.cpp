#include "procqueue.h"
#include "msgqueue/msgqueue.h"
#include "thread/threadwrap.h"

#define q() ((BlockMessageQueue<Proc *> *)m_queue)

class ProcFun : public Proc
{
public:
    ProcFun(const std::function<void()> &p) : m_fun(p){}
    void run()
    {
        m_fun();
    }

private:
    const std::function<void()> m_fun;
};

ProcQueue::ProcQueue()
{
    m_queue = new BlockMessageQueue<Proc *>();
}

ProcQueue::~ProcQueue()
{
    delete q();
}

void ProcQueue::post(Proc *p)
{
    q()->put(p);
}
void ProcQueue::post(const std::function<void()> &f)
{
    q()->put(new ProcFun(f));
}

Proc *ProcQueue::get()
{
    return q()->get();
}
void ProcQueue::quit()
{
    q()->quit();
}

