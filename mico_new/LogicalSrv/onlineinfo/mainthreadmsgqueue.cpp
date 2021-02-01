#include <atomic>
#include "mainthreadmsgqueue.h"
//#include "msgqueue/msgqueue.h"

namespace
{
BlockMessageQueue<ICommand *> mainqueue;
} //

namespace MainQueue
{

static std::atomic<bool> _isrun(true);

void postCommand(ICommand *ic)
{
    mainqueue.put(ic);
}

// BlockIfNoCommand
ICommand *getCommand()
{
    return mainqueue.get();
}

bool hasCommand()
{
    return mainqueue.hasMessage();
}

bool isrun()
{
    return _isrun;
}

void exit()
{
    _isrun = false;
    mainqueue.quit();
}

} //

