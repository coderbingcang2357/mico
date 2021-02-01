#include <fcntl.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "udpserver.h"
#include "util/logwriter.h"
#include "Message/Message.h"
#include "config/configreader.h"

Udpserver::Udpserver()
{
    int sock = socket (PF_INET, SOCK_DGRAM, 0);
    assert (sock >= 0);

    // fcntl(sock, F_SETFL, fcntl(sock, F_GETFL) | O_NONBLOCK);

    int sendbuflen = 2 * 1024 * 1024;
    int r =::setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*)&sendbuflen, sizeof(sendbuflen));
    if (r != 0)
    {
        writelog("set buf error.");
        exit(3);
    }
    // recvfrom timeout
    struct timeval tv;
    tv.tv_sec = 0;  /* 30 Secs Timeout */
    tv.tv_usec = 100* 1000;  // Not init'ing this can cause strange errors
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval));

    Configure *conf = Configure::getConfig();
    //conf->extconn_srv_address;

    sockaddr_in localAddr;
    bzero (&localAddr, sizeof (localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_port = htons(conf->extconn_srv_port);
    inet_pton (AF_INET, "localhost", &localAddr.sin_addr);
    //inet_pton (AF_INET,
    //        conf->extconn_srv_address.c_str(),
    //        &localAddr.sin_addr);

    int ret = bind (sock, (sockaddr*) &localAddr, sizeof (localAddr));
    if (ret != 0)
    {
        ::writelog(InfoType, "bind port failed:%s", strerror(errno));
        abort();
    }
    assert (ret == 0);
    (void)ret;  // for unused warning

    m_fd = sock;
}

Message *Udpserver::recv()
{
    sockaddrwrap addr;
    socklen_t len = sizeof(addr);
    char buf[10*1024];
    int datalen = ::recvfrom(m_fd, buf, sizeof(buf), 0, (sockaddr *)&addr, &len);
    if (datalen > 0)
    {
        if (check(buf, datalen))
        {
            Message *msg = new Message;
            msg->setSockerAddress(addr);
            if (msg->fromByteArray(buf, datalen))
            {
                return msg;
            }
            else
            {
                delete msg;
                return 0;
            }
        }
    }
    else if (datalen == -1)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            //::writelog("recvfrom timeout");
        }
        else
        {
            ::writelog(InfoType, "recvfrom errro:%s", strerror(errno));
        }
    }
    else if (datalen == 0)
    {
    }

    return 0;
}

void Udpserver::send(Message *msg)
{
    std::vector<char> d;
    d.reserve(512);
    msg->pack(&d);
    sockaddrwrap &addr = msg->sockerAddress();
    ::sendto(m_fd, &d[0], d.size(), 0, (sockaddr*)&(addr),
                sizeof(addr));
    ::writelog(InfoType, "send: %s, cmd: 0x%04x, len: %d",
               addr.toString().c_str(),
                msg->commandID(), d.size());
}

bool Udpserver::check(char *msgBuf, uint32_t msgLen)
{
    if(msgLen > MAX_MSG_SIZE)
    {
        ::writelog("Received message size too large.");
        return false;
    }

    uint8_t msgPrefix = *msgBuf;
    uint8_t msgSuffix = *(msgBuf + msgLen - 1);
    if((msgPrefix == 0xF0 && msgSuffix == 0xF1)
        || (msgPrefix == 0xE0 && msgSuffix == 0xE1))
        return true;
    else
        return false;
}

