#include <vector>
#include <assert.h>
#include "util/util.h"
#include "dbproxy/command.h"
#include "util/tcpdatapack.h"
#include "util/logwriter.h"
#include "protocoldef/protocol.h"
#include "cacheddbuseroperator.h"
#include "dbproxyclient.h"
#include "user/user.h"

CachedDbUserOperator::CachedDbUserOperator(DbproxyClient *client)
    : m_client(client)
{
}

int CachedDbUserOperator::getUserInfo(uint64_t userid, User *u, uint16_t *answercode)
{
    std::vector<char> data, packdata;
    ::WriteUint32(&data, GetUserInfo);
    ::WriteUint64(&data, uint64_t(userid));
    TcpDataPack::pack(&data[0], data.size(), &packdata);

    std::vector<char> unpackdata;

    assert(packdata.size() > 0);

    int res = m_client->sendCommandAndWaitReplay(
        &packdata[0], packdata.size(), &unpackdata);

    if (res != DbSuccess)
    {
        printf("send command ret error %d\n", res);
        return res;
    }

    // TODO FIXME 判断unpackdata长度是否足够
    uint16_t readanswercode = ::ReadUint16(&unpackdata[0]);

    *answercode = readanswercode;

    bool r = u->fromByteArray(&unpackdata[0] + sizeof(uint16_t), unpackdata.size() - sizeof(uint16_t));
    if (r)
    {
        return DbSuccess;
    }
    else
    {
        printf("from byte error %d\n", r);
        return DbError;
    }
}

int CachedDbUserOperator::getUserInfo(const std::string &account, User *u, uint16_t *answercode)
{
    std::vector<char> data, packdata;
    ::WriteUint32(&data, GetUserInfoByAccount);
    ::WriteString(&data, account);
    TcpDataPack::pack(&data[0], data.size(), &packdata);

    std::vector<char> unpackdata;

    int res = m_client->sendCommandAndWaitReplay(
        &packdata[0], packdata.size(), &unpackdata);

    if (res != DbSuccess)
        return res;

    // TODO FIXME 判断unpackdata长度是否足够
    uint16_t readanswercode = ::ReadUint16(&unpackdata[0]);

    *answercode = readanswercode;

    bool r = u->fromByteArray(&unpackdata[0] + sizeof(uint16_t), unpackdata.size() - sizeof(uint16_t));
    if (r)
    {
        return DbSuccess;
    }
    else
    {
        return DbError;
    }

}

int CachedDbUserOperator::modifyPassword(const std::string &account, const std::string &newpassword, uint16_t *answercode)
{
    assert(false);
    return 0;
}

