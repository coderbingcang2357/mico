#ifndef CONDWRAP_H
#define CONDWRAP_H

class MutexWrap;
class ConditionWrap
{
public:
    ConditionWrap();
    ~ConditionWrap();

    void signal();
    void wait(MutexWrap *mutex);
    void signalall();
private:
    void *d;
};
#endif
