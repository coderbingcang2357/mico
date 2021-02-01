#ifndef IPC_MSG_QUEUE_H
#define IPC_MSG_QUEUE_H

class MessageQueuePosix;

static const int Ext_Logical = 1;

static const int Internal_Logical = 2;

static const int Notify_Logical = 4;

static const int Relay_Logical = 8;

static const int ALL = Ext_Logical
                        | Internal_Logical
                        | Notify_Logical
                        | Relay_Logical;

int createMsgQueue(int which = ALL);
MessageQueuePosix *msgqueueExtserverToLogical();
MessageQueuePosix *msgqueueLogicalToExtserver();

MessageQueuePosix *msgqueueInternalSrvToLogical();
MessageQueuePosix *msgqueueLogicalToInternalSrv();

MessageQueuePosix *msgqueueNotifySrvToLogical();
MessageQueuePosix *msgqueueLogicalToNotifySrv();

MessageQueuePosix *msgqueueRelaySrvToLogical();
MessageQueuePosix *msgqueueLogicalToRelaySrv();
#endif
