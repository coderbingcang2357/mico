#ifndef MAIN_THREAD_MSG_QUEUE_H
#define MAIN_THREAD_MSG_QUEUE_H

#include "msgqueue/msgqueue.h"
#include "icommand.h"

namespace MainQueue
{
void postCommand(ICommand *ic);
// BlockIfNoCommand
ICommand *getCommand();
bool hasCommand();
bool isrun();
void exit();
}

#endif
