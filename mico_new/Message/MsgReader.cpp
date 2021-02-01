#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <assert.h>
#include <errno.h>

#include "Message/Message.h"
#include "Message/MsgReader.h"
//#include "../Util/Util.h"
#include "util/util.h"
#include "util/util.h"
//#include "config/shmconfig.h"
#include "msgqueue/posixmsgqueue.h"
#include "util/logwriter.h"
#include "protocoldef/protocol.h"

MsgReaderPosixMsgQueue::MsgReaderPosixMsgQueue(MessageQueuePosix *queue)
        : m_queue(queue)
{
}

MsgReaderPosixMsgQueue::~MsgReaderPosixMsgQueue()
{
}


void MsgReaderPosixMsgQueue::setReadQueue(MessageQueuePosix *queue)
{
    assert(queue != 0);
    m_queue = queue;
}

Message *MsgReaderPosixMsgQueue::read()
{
    char buf[MAX_MSG_SIZE];
    int len = sizeof(buf);
    int res;
    if ((res = m_queue->get(buf, &len)) == SuccessQueue)
    {
        sockaddrwrap sockAddr;
        memcpy(&sockAddr, buf, sizeof(sockAddr));
        uint16_t msgLen = len - sizeof(sockAddr);

        Message *msg = new Message;
        msg->setSockerAddress(sockAddr);
        if (msg->unpack(buf + sizeof(sockAddr), msgLen))
        {
            return msg;
        }
        else
        {
            delete msg;
            ::writelog("err meessage.");
            return 0;
        }
    }
    else if (res == EmptyQueue)
    {
        return 0;
    }
    else if (res == ErrorQueue)
    {
        ::writelog(InfoType,
                "msgqueue: %s read failed: %s",
                m_queue->name().c_str(),
                strerror(errno));
        assert(false);
        return 0;
    }
    else
    {
        assert(false);
        return 0;
    }
}
