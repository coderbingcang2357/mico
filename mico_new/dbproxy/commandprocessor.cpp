#include <assert.h>
#include "connection.h"
#include "commandprocessor.h"
#include "util/tcpdatapack.h"
#include "util/util.h"
#include "user/user.h"
#include "users.h"
#include "protocoldef/protocol.h"

void getUserInfoByID(const std::shared_ptr<Connection> &conn, const char *data, int len)
{
    (void)len;

    char *p = (char *)data;
    //printf("get command.\n");
    uint64_t userid = ReadUint64(p);
    p += sizeof(uint64_t);
    std::shared_ptr<User> u = Users::instance()->getByID(userid);
    std::vector<char> d, packdata;
    uint16_t answercode = ANSC::SUCCESS;
    if (u)
    {
        u->toByteArray(&d);
    }
    else
    {
        answercode = ANSC::ACCOUNT_NOT_EXIST;
    }

    char *ptoanswer = (char *)&answercode;
    d.insert(d.begin(), ptoanswer, ptoanswer + sizeof(uint16_t));

    TcpDataPack::pack(&d[0], d.size(), &packdata);
    conn->write(&packdata[0], packdata.size());
}

void getUserInfoByAccount(const std::shared_ptr<Connection> &conn, const char *data, int len)
{
    (void)len;

    char *p = (char *)data;
    //printf("get command.\n");
    std::string account;
    int readlen = readString(p, len, &account);
    assert(readlen >= 1);(void)readlen;

    std::shared_ptr<User> u = Users::instance()->getByAccount(account);

    std::vector<char> d, packdata;
    uint16_t answercode = ANSC::SUCCESS;
    if (u)
    {
        u->toByteArray(&d);
    }
    else
    {
        answercode = ANSC::ACCOUNT_NOT_EXIST;
    }

    char *ptoanswer = (char *)&answercode;
    d.insert(d.begin(), ptoanswer, ptoanswer + sizeof(uint16_t));

    TcpDataPack::pack(&d[0], d.size(), &packdata);
    conn->write(&packdata[0], packdata.size());
}

