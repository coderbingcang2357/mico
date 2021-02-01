#ifndef MAIN_QUEUE__H
#define MAIN_QUEUE__H
#include "queuemessage.h"
void postMessage(QueueMessage *msg);
QueueMessage *getQueueMessage();
#endif
