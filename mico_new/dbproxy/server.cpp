#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include "server.h"
#include "thread/threadwrap.h"
#include "util/tcpdatapack.h"
#include "util/util.h"
#include "connection.h"

class ThreadProc : public Proc
{
public:
    ThreadProc(Server *s, int ti) : server(s),threadid(ti) {}
    void run()
    {
        server->run(threadid);
    }

    Server *server;
    int threadid;
};

Server::Server(int port_, int tc) 
    : threadcount(tc),isrun(true), port(port_), listenfd(-1)
{
    createWakeupfd();
    createsockfd();
    createepollfd();
}

Server::~Server()
{
    close(listenfd);
    for (int i = 0; i < threadcount; ++i)
    {
        close(epollfd[i]);
    }
}

void Server::createsockfd()
{
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);
    if (listenfd < 0)
    {
        printf("socket failed. %s", strerror(errno));
        exit(0);
    }

    // reuse addr
    int reusaddr = 1;
    int res = setsockopt(listenfd, 
                 SOL_SOCKET, SO_REUSEADDR,
                 &reusaddr, sizeof(reusaddr));
    assert(res >= 0);(void)res;

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(0);
    if (bind(listenfd, (sockaddr *)&addr, sizeof(addr)))
    {
        printf("socket bind failed. %s", strerror(errno));
        exit(0);
    }

    if (listen(listenfd, 20) < 0)
    {
        printf("socket listen failed. %s", strerror(errno));
        exit(0);
    }

}

void Server::createepollfd()
{
    for (int i = 0; i < threadcount; ++i)
    {
        //
        epollfd[i] = epoll_create(10);
        if (epollfd[i] == -1)
        {
            perror("epoll create failed, ");
            ::exit(1);
        }
        // add wakeup fd to epoll
        struct epoll_event event;
        event.data.fd = wakeupfd[i][0]; // 0 is read
        event.events = EPOLLIN;
        if (epoll_ctl(epollfd[i], EPOLL_CTL_ADD, wakeupfd[i][0], &event) == 0)
        {
            // return;
        }
        else
        {
            perror("epoll_ctl failed:");
            ::exit(1);
        }
    }

    // add listen fd to epollfd[0]
    struct epoll_event eventlisten;
    eventlisten.data.fd = listenfd;
    eventlisten.events = EPOLLIN;
    if (epoll_ctl(epollfd[0], EPOLL_CTL_ADD, listenfd, &eventlisten) == 0)
    {
        // return;
    }
    else
    {
        perror("epoll_ctl listen fd failed:");
        ::exit(1);
    }
}

void Server::createWakeupfd()
{
    for (int i = 0; i < WakeUpFdCount; ++i)
    {
        int res = pipe(wakeupfd[i]);
        if (res == -1)
        {
            perror("pipe error.\n");
            ::exit(1);
        }
    }
}

void Server::run()
{
    // create thread
    Thread *thread[100];
    for (int i = 0; i < threadcount; ++i)
    {
        // thread whill take ownership of threadproc
        thread[i] = new Thread(new ThreadProc(this, i));
    }

    while (1)
    {
        sleep(1);
    }
    for (int i = 0; i < threadcount; ++i)
    {
        delete thread[i];
    }
}

void Server::run(int threadid)
{
    if (!isrun)
        return;

    char pipebuf[1000];
    int datalen = 0;
    int maxevent = 1000;
    struct epoll_event events[maxevent];
    int ms = 3000;
    for (;isrun;)
    {
        //::writelog(InfoType, "epoll wait start: %d", epollfd );
        int nfds = epoll_wait(epollfd[threadid], events, maxevent, ms);
        if (nfds == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            else
            {
                ::printf("epoll wait failed: %s, %d", strerror(errno), errno);
                ::exit(1);
            }
        }

        for (int i = 0; i < nfds; i++)
        {
            int fd = events[i].data.fd;
            if (fd == wakeupfd[threadid][0])
            {
                if (!isrun)
                {
                    // TODO fixme 是否要清理资源
                    return;
                }

                ssize_t readlen = ::read(wakeupfd[threadid][0], pipebuf+ datalen, sizeof(pipebuf) - datalen);
                assert(readlen >= 0);
                datalen += readlen;
                if (readlen >= 4)
                {
                    char *p2buf = pipebuf;
                    while (datalen >= 4)
                    {
                        int clientfd = *((int *)p2buf);
                        printf("new connection: threadid %d, %d\n", threadid, clientfd);
                        struct epoll_event newevent;
                        newevent.data.fd = clientfd;
                        newevent.events = EPOLLIN;
                        if (epoll_ctl(epollfd[threadid], EPOLL_CTL_ADD, clientfd, &newevent) == 0)
                        {
                            // insertConnection(con);
                            connections[threadid][clientfd] = std::make_shared<Connection>(clientfd);
                        }
                        else
                        {
                            perror("new fd add to epoll failed2:");
                            close(clientfd);
                        }

                        if (datalen-4 < 0)
                        {
                            assert(false);
                        }

                        datalen -= 4;
                        p2buf+=4;
                    }
                    if (datalen != 0)
                    {
                        ::memmove(pipebuf, p2buf, datalen);
                    }
                }
                else
                {
                    assert(false);
                }
                continue;
            }
            else if (fd == listenfd)
            {
                static int whichthread = 1;
                sockaddr_in clientaddr;
                socklen_t addrlen = sizeof(clientaddr);
                int clientfd = ::accept(listenfd, (sockaddr *)&clientaddr, &addrlen);
                assert(clientfd >= 0);
                int wl = write(wakeupfd[whichthread][1], (char *)&clientfd, sizeof(clientfd));
                (void)wl;
                if (++whichthread >= threadcount)
                {
                    whichthread = 1;
                }
                // printf("new client:%d", clientfd);
                //struct epoll_event newevent;
                //newevent.data.fd = clientfd;
                //newevent.events = EPOLLIN;
                //if (epoll_ctl(epollfd, EPOLL_CTL_ADD, clientfd, &newevent) == 0)
                //{
                //    // insertConnection(con);
                //    connections[threadid][clientfd] = std::make_shared<Connection>(clientfd);
                //}
                //else
                //{
                //    perror("new fd add to epoll failed2:");
                //    close(fd);
                //}
            }
            else
            {
                processReadEvent(threadid, fd);
            }
        }
    }

}

void Server::insertConnection(Connection *con)
{
}

void Server::processReadEvent(int threadid, int fd)
{
    // printf("read: %d\n", fd);
    auto it = connections[threadid].find(fd);
    if (it != connections[threadid].end())
    {
        std::shared_ptr<Connection> conn = it->second;
        int readlen = conn->read();
        if (readlen == 0)
        {
            // cose conection
            connections[threadid].erase(it);
            int res = epoll_ctl(epollfd[threadid], EPOLL_CTL_DEL, fd, 0);
            if (res < 0)
            {
                perror("remove client failed:");
            }
        }
        else if (readlen < 0)
        {
            perror("read error.");
        }
        else
        {
            // ok
        }
    }
    else
    {
        assert(false);
    }
}

