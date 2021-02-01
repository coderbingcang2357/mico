#include <unistd.h>
#include <sys/socket.h>
#include <assert.h>
#include "util/tcpdatapack.h"
#include "connection.h"
#include "command.h"
#include "util/util.h"
#include "commandprocessor.h"

Connection::Connection(int fd_) : fd(fd_), datalen(0)
{
}

Connection::~Connection()
{
    close(fd);
}

//std::pair<char *, int> Connection::getAvaiableBuf()
//{
//    return std::make_pair(data + datalen, MAX_BUF - datalen);
//}

// void Connection::updateDataSize(int incsize)
// {
//     datalen += incsize;
// }

std::pair<char *, int> Connection::getData()
{
    return std::make_pair(data, datalen);
}

int Connection::read()
{
    for (;;)
    {
        assert(sizeof(data) - datalen > 0);
        int readlen = ::read(fd, data + datalen, sizeof(data) - datalen);
        if (readlen < 0)
        {
            if (errno == EINTR)
                continue;
            else
                return readlen;

        }
        else if (readlen == 0)
        {
            // assert(false);
            return 0;
        }
        else
        {
            datalen += readlen;
            processData();
            return readlen;
        }
    }

}

void Connection::processData()
{
    // unpack
    char *p = this->data;
    std::vector<char> pack;
    int l = TcpDataPack::unpack(p, datalen, &pack);
    if (l != 0)
    {
        // get userinfo
        processPack(pack);
        datalen = 0;
        // assert(conn->getData().second == 0);
    }
    else
    {
        assert(false);
    }
}

void Connection::processPack(const std::vector<char> &pack)
{
    char *p = (char *)&pack[0];
    int len = pack.size();
    uint32_t commandid = ReadUint32(p);
    p += sizeof(uint32_t);
    len -= sizeof(uint32_t);

    switch (commandid)
    {
    case GetUserInfo:
    {
        getUserInfoByID(shared_from_this(), p, len);
    }
    break;

    case GetUserInfoByAccount:
    {
        getUserInfoByAccount(shared_from_this(), p, len);
    }
    break;
    default:
        assert(false);
    }

}

void Connection::write(char buf[], int buflen)
{
    int wl = ::write(fd, buf, buflen);
    assert(wl == buflen);
    (void)wl;
}

