#include <fcntl.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include "udpserver.h"
#include "util/logwriter.h"
#include "util/util.h"
#include "Message/Message.h"
#include "config/configreader.h"
#include "Message/tcpservermessage.h"
#include "connectionservermessage.h"
#include "utilsock.h"
#include "serverinfo/serverid.h"
#include "util/sock2string.h"

Udpserver::Udpserver(ServerQueue *masterqueue) : m_masterqueue(masterqueue)
{
    int sock = socket (PF_INET, SOCK_DGRAM, 0);
    assert (sock >= 0);

    // fcntl(sock, F_SETFL, fcntl(sock, F_GETFL) | O_NONBLOCK);

    int recvbuf = 2 * 1024 * 1024;
    int r =::setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*)&recvbuf, sizeof(recvbuf));
    if (r != 0)
    {
        writelog("set buf error.");
        exit(3);
    }

    int sendbuff = 2 * 1024;
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sendbuff, sizeof(sendbuff));

    socklen_t optlen;
    optlen = sizeof(sendbuff);
    getsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sendbuff, &optlen);
    ::writelog(InfoType, "SNDBUF: %d", sendbuff);

    // recvfrom timeout
    struct timeval tv;
    tv.tv_sec = 0;  /* 30 Secs Timeout */
    tv.tv_usec = 10* 1000;  // Not init'ing this can cause strange errors
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval));

    Configure *conf = Configure::getConfig();
    //conf->extconn_srv_address;

    sockaddr_in6 localAddr;
    bzero (&localAddr, sizeof(localAddr));
    localAddr.sin6_family = AF_INET;
    localAddr.sin6_port = htons(conf->extconn_srv_port);
    //inet_pton (AF_INET, "localhost", &localAddr.sin_addr);
    //inet_pton (AF_INET,
    //        conf->extconn_srv_address.c_str(),
    //        &localAddr.sin_addr);

    int ret = bind(sock, (sockaddr*) &localAddr, sizeof (localAddr));
    if (ret != 0)
    {
        ::writelog(InfoType, "bind port failed:%s", strerror(errno));
        abort();
    }
    assert (ret == 0);
    (void)ret;  // for unused warning

    m_fd = sock;
}

Udpserver::~Udpserver()
{
    close(m_fd);
}

void Udpserver::run()
{
    while (m_isrun)
    {
        this->recv();
        this->processQueue();
    }
}

void Udpserver::quit()
{
    m_isrun = false;
}

void Udpserver::recv()
{
    Configure *config = Configure::getConfig();

    sockaddrwrap addr;
    socklen_t len = sizeof(addr);
    char buf[10*1024];
    int datalen = ::recvfrom(m_fd, buf, sizeof(buf), 0, (sockaddr *)&addr, &len);
    if (datalen > 0)
    {
        if (check(buf, datalen))
        {
            Message msg;// = Message;
            msg.setSockerAddress(addr);
            if (msg.fromByteArray(buf, datalen))
            {
                TcpServerMessage tsm;
                tsm.clientaddr = (sockaddr *)&addr;
                tsm.addrlen = len;
                tsm.cmd = TcpServerMessage::NewMessage;
                tsm.connid = 0;
                tsm.connServerId = config->serverid;
                tsm.clientmessage = buf;
                tsm.clientmessageLength = datalen;
                std::vector<char> td = tsm.toByteArray();
                //
                UdpSendToServerMessage *udpmsg = new UdpSendToServerMessage;
                udpmsg->data = td;
                udpmsg->ipport = Utilsock::sockaddr2string((sockaddr *)&addr, len);

                if (isServerID(msg.destID()))
                {
                    udpmsg->logicalserverid = msg.destID();
                }
                m_masterqueue->addMessage(udpmsg);
                //::writelog(InfoType, "get udp data from %s, size: %d",
                           //addr.toString().c_str(), datalen);
            }
            else
            {
                // ...
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
}

void Udpserver::send(char *d, int dlen, sockaddr *addr, int addrlen)
{
    uint16_t cmd = ::ReadUint16(d + 3);
    int  r = ::sendto(m_fd, d, dlen, 0, addr, addrlen);
#if 0
    ::writelog(InfoType, "send: %s, cmd: 0x%04x, len: %d",
               ::Utilsock::sockaddr2string(addr, addrlen).c_str(),
               cmd,
               dlen);
#endif
    if (r == -1)
    {
        ::writelog(InfoType, "udp send error : %s", strerror(errno));
    }
}

void Udpserver::processQueue()
{
    std::list<IConnectionServerMessage*> msgs;
    m_queue.getMessage(&msgs);
    for (IConnectionServerMessage *msg : msgs)
    {
        switch (msg->type())
        {
        case UdpSendToClientMessageType:
        {
            UdpSendToClientMessage *udpmsg = (UdpSendToClientMessage *)msg;
            this->send(udpmsg->data().data(), udpmsg->data().size(),
                       udpmsg->clientAddr().getAddress(), udpmsg->clientAddr().getAddressLen());
        }
        break;

        default:
            assert(false);
            break;
        }
        delete msg;
    }
}

void Udpserver::addMessage(IConnectionServerMessage *msg)
{
    m_queue.addMessage(msg);
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

