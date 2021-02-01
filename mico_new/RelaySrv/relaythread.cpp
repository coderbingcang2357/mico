#include <sys/epoll.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <memory>

#include "thread/threadwrap.h"
#include "relaythread.h"
#include "mainqueue.h"
#include "relayinternalmessage.h"
#include "util/logwriter.h"
#include "sessionRandomNumber.h"
#include "protocoldef/protocol.h"
#include "ChannelManager.h"
#include "util/util.h"

namespace
{
static const uint8_t WakeUpCode = 0;
static const uint8_t ExitCode = 1;
static const uint32_t CHANNEL_TIMEOUT = 60; // 60s
}

static const int MAX_SEND_PACKETS = 500;

class RelayProc : public Proc
{
public:
    RelayProc(RelayThread *r) : rt(r)
    {
    }

    void run()
    {
        rt->run();
    }

    RelayThread *rt;
};

RelayThread::RelayThread() : m_thread(0),
    m_isrun(true), m_epollfd(-1)//, m_relayport(relayport)
{
    m_channels = new ChannelManager;
    gettimeofday(&polltimer, 0);
}

RelayThread::~RelayThread()
{
    this->quit();
    delete m_thread;
    for (auto &v : m_command)
    {
        delete v;
    }
    close(m_epollfd);
    //close(m_relayfd);
    close(wakeupfd[0]);
    close(wakeupfd[1]);
    delete m_channels;
}

bool RelayThread::create()
{
    // create epollfd
    m_epollfd = epoll_create(10);
    if (m_epollfd == -1)
    {
        perror("epoll create failed, ");
        ::exit(1);
    }
    ::writelog(InfoType, "eppo: %d", m_epollfd);
    createWakeupfd();
    // create relay socket
    //this->createRelaySocket();

    // thread will take ownership of p
    RelayProc *p = new RelayProc(this);
    m_thread = new Thread(p);

    return true;
}

void RelayThread::wakeup()
{
    char d[1] = {WakeUpCode};
    int len = write(wakeupfd[1], d, sizeof(d));
    if (len == -1)
    {
        perror("wakeup write failed");
    }
    else
    {
        assert(len == sizeof(d));
    }
    return;
}

void RelayThread::createWakeupfd()
{
    int res = pipe(wakeupfd);
    if (res == -1)
    {
        perror("pipe error.\n");
        ::exit(1);
    }
    // add wakeup fd to epoll
    struct epoll_event event;
    event.data.fd = wakeupfd[0]; // 0 is read
    event.events = EPOLLIN;
    if (epoll_ctl(m_epollfd, EPOLL_CTL_ADD, wakeupfd[0], &event) == 0)
    {
        // return;
    }
    else
    {
        perror("epoll_ctl add wakeup fd failed:");
        ::exit(1);
    }
}

std::pair<bool, int> RelayThread::createRelaySocket(uint16_t port)
{
    int relayfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (relayfd == -1)
    {
        perror("create relayfd failed.\n");
        return std::make_pair(false, -1);
    }
    // bind to port
    sockaddr_in localAddr;
    bzero (&localAddr, sizeof (localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_port = htons(port);
    inet_pton (AF_INET, "localhost", &localAddr.sin_addr);

    int ret = bind(relayfd, (sockaddr*) &localAddr, sizeof (localAddr));
    if (ret == -1)
    {
        ::writelog(ErrorType, "bind port failed:%u", port);
        close(relayfd);
        return std::make_pair(false, -1);
    }

    // make noblock
    fcntl(relayfd, F_SETFL, fcntl(relayfd, F_GETFL) | O_NONBLOCK);
    // set recvbuf to 4M
    int sendbuflen = 4 * 1024 * 1024;
    int r =::setsockopt(relayfd, SOL_SOCKET, SO_RCVBUF, (char*)&sendbuflen, sizeof(sendbuflen));
    if (r != 0)
    {
        writelog(ErrorType, "set buf error.");
        close(relayfd);
        return std::make_pair(false, -1);
    }
    // add to epoll
    struct epoll_event event;
    event.data.fd = relayfd;
    event.events = EPOLLIN;
    if (epoll_ctl(m_epollfd, EPOLL_CTL_ADD, relayfd, &event) == 0)
    {
        return std::make_pair(true, relayfd);
    }
    else
    {
        perror("epoll_ctl add relayfd failed:");
        close(relayfd);
        return std::make_pair(false, -1);
    }
}

void RelayThread::run()
{
    if (!m_isrun)
        return;

    int maxevent = 1000;
    struct epoll_event events[maxevent];
    int ms = 30;
    for (;m_isrun;)
    {
        //::writelog(InfoType, "epoll wait start: %d", epollfd );
        int nfds = epoll_wait(m_epollfd, events, maxevent, ms);
        if (nfds == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            else
            {
                ::writelog(ErrorType, "epoll wait failed: %s, %d", strerror(errno), errno);
                ::exit(1);
            }
        }

        for (auto it = m_writelist.begin(); it != m_writelist.end();)
        {
            auto &v = *it;
            int r = sendto(v->sendfd,
                           &(v->data[0]),
                           v->data.size(),
                           0,
                            (sockaddr *)&(v->dest),
                            sizeof(v->dest));
            if (r < 0)
            {
                if (errno != EAGAIN && errno != EWOULDBLOCK)
                {
                    //
                    ::writelog(ErrorType,
                            "forward send to failed.1: %s",
                            strerror(errno));
                }
                break;
            }
            // send success remove from list
            else
            {
                it = m_writelist.erase(it);
            }
        }

        for (int i = 0; i < nfds; i++)
        {
            int fd = events[i].data.fd;
            if (fd == wakeupfd[0])
            {
                if (!m_isrun)
                {
                    // TODO fixme 是否要清理资源
                    return;
                }

                char buf[1000];
                ssize_t readlen = ::read(wakeupfd[0], buf, sizeof(buf));
                (void)readlen;
                // user call exit-
                processQueueMessage();

                continue;
            }
            else //if (fd == m_relayfd)
            {
                readData(fd);
            }
        }

        // 处理定时器
        processTimer();
    }
}

void RelayThread::processQueueMessage()
{
    if (m_command.size() <= 0)
        return;

    std::list<RelayThreadCommand *> cmd;
    {
        mutex.lock();
        m_command.swap(cmd);
        mutex.release();
    }
    for (auto it = cmd.begin(); it != cmd.end(); ++it)
    {
        RelayThreadCommand *cmd = *it;
        switch (cmd->commandid)
        {
        case RelayThreadCommand::AddChannel:
        {
            m_channels->openChannel(cmd->channel);
        }
        break;

        case RelayThreadCommand::InsertChannel:
        {
            m_channels->insert(cmd->channel);
        }
        break;

        case RelayThreadCommand::CloseChannel:
        {
            m_channels->closeChannel(UserDevPair(cmd->userid, cmd->devid));
        }
        break;

        default:
            assert(false);
            break;
        }

        delete cmd;
    }
}

void RelayThread::processTimer()
{
    // 每30秒执行一次
    timeval cur, diff;
    gettimeofday(&cur, 0);
    timersub(&cur, &polltimer, &diff);
    if (diff.tv_sec >= 30)
    {
        m_channels->poll();
        polltimer = cur;
    }
}

void RelayThread::readData(int sockfd)
{
    // 转发包计数, 一次循环最多处理MAX_SEND_PACKETS个包
    int count = 0;
    for (;;)
    {
        if (count >= MAX_SEND_PACKETS)
        {
            return;
        }

        char buf[1024];
        sockaddrwrap sa;
        uint sockAddrLen = sizeof(sockaddr);
        ssize_t ret = recvfrom(sockfd,
                            buf,
                            sizeof(buf),
                            0,
                            (sockaddr*) &sa,
                            &sockAddrLen);
        if(ret < 0)
            return;

        // is the change client address message ??
        // this message is always 10 bytes
        if (ret == 10)
        {
            if (uint8_t(buf[0]) == 0xff && uint8_t(buf[1]) == 0xc0)
            {
                uint64_t randomnumber = getSessionRandomNumber(((char *)buf) + 2, 8);
                bool r = m_channels->updateAddress(randomnumber, sa);
                if (!r)
                {
                    ::writelog(WarningType, "relay unknow randomnumber.%lu",
                        randomnumber);
                }
            }
            else
            {
                ::writelog(WarningType, "unknow message.");
            }
            return;
        }
        if (ret <= PACK_HEADER_SIZE_V2)
        {
            //::writelog(WarningType, "error message size: %d, from %s",
            //            ret,
            //           sa.toString().c_str());
            //return;
        }

        // 解析用户id和设备id
        uint64_t srcid = ::ReadUint64(buf + 7); // 包头第7个字节开始是源id
        uint64_t destid = ::ReadUint64(buf + 7 + 8); // 包头第7+8个字节开始是目标id

        // 查找转发通道
        auto ch = m_channels->findChannel(UserDevPair(srcid, destid));
        if (!ch)
        {
            ::writelog(WarningType, "channel not find: src: %lu, dest: %lu, size:%d, from: %s",
                        srcid, destid, ret,
                       sa.toString().c_str());
            return;
        }
        // 判断目的ip
        sockaddrwrap dest;
        sockaddrwrap srcaddr;

        int sendfd = -1;
        // to user
        if (ch->userID == destid)
        {
            srcaddr = ch->deviceSockAddr;
            dest = ch->userSockAddr;
            sendfd = ch->userfd;
            //assert(sockfd == ch->devfd); // 表示从dev收到数据, 要发给user
            if (sockfd != ch->devfd)
            {
                ::writelog(WarningType,
                        "sock fd no same 1: %d, %d",
                        sockfd, ch->devfd);
            }
        }
        // to device
        else if (ch->deviceID == destid)
        {
            srcaddr = ch->userSockAddr;
            dest = ch->deviceSockAddr;
            sendfd = ch->devfd;
            //assert(sockfd == ch->userfd); // 表示从user收到数据, 要发给dev
            if (sockfd != ch->userfd)
            {
                ::writelog(WarningType,
                        "sock fd no same",
                        sockfd, ch->userfd);
            }
        }
        else
        {
            ::writelog(WarningType, "wtf???");
            assert(false);
            return;
        }
        // 判断源ip是否合法
        if (!(srcaddr == sa))
        {
            //
            ::writelog(WarningType, "message from unknown address, require: %s, Error:%s",
                       srcaddr.toString().c_str(),
                       sa.toString().c_str());
            return;
        }
        // update lasttime
        timeval cur;
        gettimeofday(&cur, 0);
        ch->lastCommuniteTime = cur;

        // 转发包计数
        count++;

        int r = sendto(sendfd,
                    buf,
                    ret,
                    0,
                    (sockaddr *) &(dest), dest.getAddressLen());
        if (r == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // save to send queue
                DataToWrite *dtw = new DataToWrite;
                dtw->sendfd = sendfd;
                dtw->dest = dest;
                dtw->data = std::vector<char>(buf, buf + ret);
                m_writelist.push_back(dtw);

                ::writelog(WarningType, "forward send to eagain");
            }
            else
            {
                //
                ::writelog(ErrorType,
                        "forward send to failed: %s, %s, send fd: %d, addrlen: %d",
                        strerror(errno),
                        dest.toString().c_str(),
                           sendfd,
                           dest.getAddressLen());
            }
            break;
        }

        //printf("Data from : %s:%u", inet_ntoa(sa.sin_addr), ntohs(sa.sin_port));
        //for (uint16_t i = 0; i < ret; ++i)
        //{
        //    printf("%x ", uint8_t(buf[i]));
        //}
        //printf("\n");
    }
}

void RelayThread::addCommand(RelayThreadCommand *c)
{
    mutex.lock();
    m_command.push_back(c);
    mutex.release();
    wakeup();
}

//uint16_t RelayThread::getRelayPort()
//{
//    //return //m_relayport;
//}

