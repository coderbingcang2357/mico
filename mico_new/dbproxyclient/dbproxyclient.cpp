#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <errno.h>
#include "dbproxyclient.h"
#include "user/user.h"
#include "dbproxy/command.h"
#include "util/tcpdatapack.h"
#include "util/util.h"
#include "util/logwriter.h"

DbproxyClient::DbproxyClient(const std::string &serveraddr, int port)
    : m_serveraddr(serveraddr), m_port(port), m_sockfd(-1)
{
}

DbproxyClient::~DbproxyClient()
{
    if (m_sockfd >= 0)
        close(m_sockfd);
}

int DbproxyClient::connect()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    assert(fd >= 0);
    sockaddr_in addr;
    addr.sin_addr.s_addr = inet_addr(m_serveraddr.c_str());
    addr.sin_port = htons(8989);
    addr.sin_family = AF_INET;
    socklen_t len = sizeof(addr);
    // printf("connect:\n");
    if (::connect(fd, (sockaddr *)&addr, len) != 0)
    {
        ::writelog(InfoType,"dbproxy connect failed: %s:%d:%s",
            m_serveraddr.c_str(), m_port, strerror(errno));
        return DbConnectFailed;
    }
    m_sockfd = fd;
    return DbSuccess;
}

int DbproxyClient::sendCommandAndWaitReplay(
    char *buf, int buflen, std::vector<char> *pack)
{
    int res;
    if ((res = writesocket(buf, buflen)) != DbSuccess)
    {
        printf("write socket error.%d\n", res);
        return res;
    }

    // 
    return readsocket(pack);
}

int DbproxyClient::ping()
{
    return DbSuccess;
}

int DbproxyClient::readsocket(std::vector<char> *unpackdata)
{
    char buf[1000];
    int datalen = 0;

    timeval curtime;
    gettimeofday(&curtime, 0);
    for (;;)
    {
        timeval aftertime, elaptime;
        gettimeofday(&aftertime, 0);

        timersub(&aftertime, &curtime, &elaptime);

        // 500 ms 超时
        if (elaptime.tv_sec > 0 || elaptime.tv_usec >= 1000 * 500)
        {
            ::writelog(InfoType, "dbproxy timeout.%ld, %ld\n", elaptime.tv_sec, elaptime.tv_usec);
            return DbTimeOut;
        }

        int readlen = read(m_sockfd, buf + datalen, sizeof(buf) - datalen);

        if (readlen < 0)
        {
            if (errno == EINTR)
                continue;
            else
            {
                perror("read soket error.");
                return DbError;
            }
        }

        // connection closed
        if (readlen == 0)
        {
            return DbConnectionClosed;
        }

        datalen += readlen;

        unpackdata->clear();
        int pl = TcpDataPack::unpack(buf, datalen, unpackdata);

        if (pl > 0)
        {
            // timeval aftertime, elaptime;
            // gettimeofday(&aftertime, 0);

            // timersub(&curtime, &aftertime, &elaptime);
            // printf();
            return DbSuccess;
        }
    }
}

int DbproxyClient::writesocket(char *buf, int buflen)
{
    for (;;)
    {
        int r = ::write(m_sockfd, buf, buflen);
        if (r < 0)
        {
            if (errno == EINTR)
                continue;
            else if (errno == EPIPE)
                return DbConnectionClosed;
            else
                return DbError;
        }
        else
        {
            assert(r == buflen);
            return DbSuccess;
        }
    }
}

