#include "ipcmsgqueue.h"
#include "posixmsgqueue.h"
#include "protocoldef/protocol.h"

static MessageQueuePosix *extserverToLogical = 0;
static MessageQueuePosix *logicalToExtserver = 0;

static MessageQueuePosix *internalSrvToLogical = 0;
static MessageQueuePosix *logicalToInternalSrv = 0;

static MessageQueuePosix *notifySrvToLogical = 0;
static MessageQueuePosix *logicalToNotifySrv = 0;

static MessageQueuePosix *relaySrvToLogical = 0;
static MessageQueuePosix *logicalToRelaySrv = 0;

int createMsgQueue(int which)
{
    if (which & Ext_Logical)
    {
        extserverToLogical = new MessageQueuePosix("/extServerToLogical",
                                    MAX_MSG_IN_MSG_QUEUE, MAX_MSG_SIZE);
        if (!extserverToLogical->create())
            return -1;

        logicalToExtserver = new MessageQueuePosix("/logicalToExtServer",
                                    MAX_MSG_IN_MSG_QUEUE, MAX_MSG_SIZE);
        if (!logicalToExtserver->create())
            return -1;
    }

    if (which & Internal_Logical)
    {
        internalSrvToLogical = new MessageQueuePosix("/internalSrv_to_logical", 
                                MAX_MSG_IN_MSG_QUEUE, MAX_MSG_SIZE);
        if (!internalSrvToLogical->create())
            return -1;

        logicalToInternalSrv = new MessageQueuePosix("/logical_to_internalSrv",
                                MAX_MSG_IN_MSG_QUEUE, MAX_MSG_SIZE);
        if (!logicalToInternalSrv->create())
            return -1;
    }

    if (which & Notify_Logical)
    {
        notifySrvToLogical = new MessageQueuePosix("/notify_to_logical",
                            MAX_MSG_IN_MSG_QUEUE, MAX_MSG_SIZE);
        if (!notifySrvToLogical->create())
            return -1;

        logicalToNotifySrv = new MessageQueuePosix("/logical_to_notify",
                            MAX_MSG_IN_MSG_QUEUE, MAX_MSG_SIZE);
        if (!logicalToNotifySrv->create())
            return -1;
    }

    if (which & Relay_Logical )
    {
        relaySrvToLogical = new MessageQueuePosix("/relay_to_logical",
                            MAX_MSG_IN_MSG_QUEUE, MAX_MSG_SIZE);
        if (!relaySrvToLogical->create())
            return -1;

        logicalToRelaySrv = new MessageQueuePosix("/logical_to_relay",
                            MAX_MSG_IN_MSG_QUEUE, MAX_MSG_SIZE);
        if (!logicalToRelaySrv->create())
            return -1;
    }
    return 0;
}

MessageQueuePosix *msgqueueExtserverToLogical()
{
    return extserverToLogical;
}

MessageQueuePosix *msgqueueLogicalToExtserver()
{
    return logicalToExtserver;
}

MessageQueuePosix *msgqueueInternalSrvToLogical()
{
    return internalSrvToLogical;
}

MessageQueuePosix *msgqueueLogicalToInternalSrv()
{
    return logicalToInternalSrv;
}

MessageQueuePosix *msgqueueNotifySrvToLogical()
{
    return notifySrvToLogical;
}

MessageQueuePosix *msgqueueLogicalToNotifySrv()
{
    return logicalToNotifySrv;
}

MessageQueuePosix *msgqueueRelaySrvToLogical()
{
    return relaySrvToLogical;
}

MessageQueuePosix *msgqueueLogicalToRelaySrv()
{
    return logicalToRelaySrv;
}
