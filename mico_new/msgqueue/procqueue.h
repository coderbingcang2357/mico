#ifndef MAIL_QUEUE__H_
#define MAIL_QUEUE__H_
#include <functional>
class Proc;

class ProcQueue
{
public:
    ProcQueue();
    ~ProcQueue();
    void post(Proc *p);
    void post(const std::function<void()> &f);
    Proc *get();
    void quit();
    // bool isRun();
private:
    void *m_queue;
};
#endif
