#include <unistd.h>
#include <netinet/in.h>
#include <assert.h>
#include <strings.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <errno.h>
#include <string.h>
#include "logserver.h"
#include "logread.h"

const uint16_t AddLog = 1; // uint16_t + nbyte ==> 0x1+string
const uint16_t GetLastLog = 2; // uint16_t + uint16_t ==> 0x2+size
extern bool mainfile_isrun;

Logserver::Logserver(int port, char *logfilepath)
    : m_port(port), m_logfilepath(logfilepath), m_logreader(0)
{
}

Logserver::~Logserver()
{
    if (m_listensock >= 0)
        close(m_listensock);
    if (m_logreader)
        delete m_logreader;
}

bool Logserver::create()
{
    int sock = socket (PF_INET, SOCK_DGRAM, 0);
    assert(sock >= 0);

    // fcntl(sock, F_SETFL, fcntl(sock, F_GETFL) | O_NONBLOCK);

    int sendbuflen = 2 * 1024 * 1024;
    int r =::setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*)&sendbuflen, sizeof(sendbuflen));
    if (r != 0)
    {
        printf("set buf error.");
        exit(3);
    }

    // Configure *conf = Configure::getConfig();
    //conf->extconn_srv_address;

    sockaddr_in localAddr;
    bzero (&localAddr, sizeof (localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_port = htons(m_port);
    inet_pton (AF_INET, "localhost", &localAddr.sin_addr);
    //inet_pton (AF_INET,
    //        conf->extconn_srv_address.c_str(),
    //        &localAddr.sin_addr);

    int ret = bind (sock, (sockaddr*) &localAddr, sizeof (localAddr));
    assert (ret >= 0);
    (void)ret;  // for unused warning

    m_listensock = sock;

    m_logreader = new Logreader(m_logfilepath);
    if (!m_logreader->init())
        return false;
    else
        return true;
}

void Logserver::run()
{

    fd_set readfds;
    int maxFdp = m_listensock + 1;
    while(mainfile_isrun)
    {
        //1. 处理客户输入的消息
        FD_ZERO(&readfds);
        FD_SET(m_listensock, &readfds);

        timeval timeout = {0, 100 * 1000};// 100 ms

        int ret = select(maxFdp, &readfds, NULL, NULL, &timeout);
        if(ret == -1)
        {
            if (errno == EINTR)
            {
                printf("eintr user exit.\n");
                continue;
            }
            else
            {
                assert(false);
                printf("select errro %s\n", strerror(errno));
                exit(-1);
            }
        }
        else if (ret == 0)
        {
            // timeout
        }
        else
        {
            if(FD_ISSET(m_listensock, &readfds))
            {
                sockaddr_in addr;
                socklen_t addrlen = sizeof(addr);
                char buf[1024];
                int recvlen = recvfrom(m_listensock,
                    buf, 
                    sizeof(buf),
                    0,
                    (struct sockaddr *)&addr,
                    &addrlen); // flags
                if (recvlen == -1)
                {
                    printf("errno recvfrom: %s\n", strerror(errno));
                }
                else if (recvlen > 2) // at least 3 byte
                {
                    processCommand(addr, buf, recvlen);
                }
                else
                {
                    printf("error package.\n");
                }
            }
        }
    }
}

void Logserver::processCommand(const sockaddr_in &sockaddr, char *buf, int len)
{
    uint16_t command =  *((uint16_t *)buf);
    command = ntohs(command);
    switch (command)
    {
    case AddLog:
    {
        buf[len] = 0;
        addLog(sockaddr, buf + 2);
    }
    break;

    case GetLastLog:
    {
        int16_t size = *((int16_t *)(buf+2));
        size = ntohs(size);
        getLastLog(sockaddr, size);
    }
    break;

    default:
        printf("unknown command: %d\n", command);
    break;
    }
}

void Logserver::addLog(const sockaddr_in &sockaddr, char *l)
{
    printf("new log from %s:%u : %s\n",
                        inet_ntoa(sockaddr.sin_addr),
                        ntohs(sockaddr.sin_port),
                        l);
}

void Logserver::getLastLog(const sockaddr_in &sockaddr, int size)
{
    if (size > 1000)
        size = 1000;
    if (size < 0)
        size = 1;

    std::string d;
    m_logreader->getLast(size, &d);
    sendto(m_listensock, d.data(), d.size(),
            0,
            (struct sockaddr *)&sockaddr, sizeof(sockaddr));
}

