#include  "channelDatabase.h"
#include "mysqlcli/imysqlconnpool.h"
#include "mysqlcli/mysqlconnection.h"
#include "config/configreader.h"
#include "util/sock2string.h"
#include "util/logwriter.h"
#include "ChannelManager.h"

ChannelDatabase::ChannelDatabase(IMysqlConnPool *p) : mysqlpool(p)
{

}

int ChannelDatabase::removeChannel(uint64_t userid, uint64_t deviceid)
{
    auto conn = mysqlpool->get();
    if (!conn)
        return -1;
    int serverid = Configure::getConfig()->serverid;
    char query[1024];
    snprintf(query, sizeof(query),
        "delete from RelayChannels where serverid=%d and userid=%lu and deviceid=%lu",
        serverid, userid, deviceid);
    if (conn->Delete(query) == -1)
    {
        ::writelog(ErrorType, "remove channel failed: %s", query);
        return -1;
    }
    else
    {
        return 0;
    }
}

//  serverid      | int(10) unsigned    
//  userid        | bigint(20) unsigned 
//  deviceid      | bigint(20) unsigned 
//  userport      | int(10) unsigned    
//  deviceport    | int(10) unsigned    
//  userfd        | int(11)             
//  devicefd      | int(11)             
//  useraddress   | varchar(100)        
//  deviceaddress
int ChannelDatabase::insertChannel(uint64_t userid, uint64_t deviceid,
    uint16_t userport, uint16_t deviceport,
    int userfd, int devicefd,
    const sockaddrwrap &useraddr, const sockaddrwrap &deviceaddr)
{
    auto conn = mysqlpool->get();
    if (!conn)
        return -1;
    int serverid = Configure::getConfig()->serverid;
    std::string useraddrstr = useraddr.toString();
    std::string deviceaddrstr = deviceaddr.toString();

    char query[1024];
    snprintf(query, sizeof(query),
        "replace into RelayChannels(serverid, userid, deviceid, userport, deviceport,userfd,devicefd, useraddress, deviceaddress, createdate)"
        " values (%d, %lu, %lu," // serverid uid devid
        " %u, %u," // userport deviceport
        "%d, %d,"  // userfd, devicefd
        "'%s', '%s', now())", // useraddress, device address, craete date
        serverid,
        userid, deviceid,
        userport, deviceport,
        userfd, devicefd,
        useraddrstr.c_str(), deviceaddrstr.c_str());
    if (conn->Insert(query) < 0)
    {
        ::writelog(ErrorType, "insert channel failed: %s", query);
        return -1;
    }
    else
    {
        return 0;
    }
}

std::list<std::shared_ptr<ChannelInfo>> ChannelDatabase::readChannel()
{
    std::list<std::shared_ptr<ChannelInfo>> channels;
    auto conn = mysqlpool->get();
    if (!conn)
        return channels;
    int serverid = Configure::getConfig()->serverid;
    char query[1024];
    snprintf(query, sizeof(query),
        "select userid, deviceid, userport, deviceport, userfd, devicefd, useraddress, deviceaddress from RelayChannels where serverid=%d",
        serverid);
    if (conn->Select(query) == -1)
        return channels;
    auto result = conn->result();
    int rowcount = result->rowCount();
    for (int i = 0; i < rowcount; ++i)
    {
        std::shared_ptr<ChannelInfo> ch = std::make_shared<ChannelInfo>();
        auto row = result->nextRow();
        ch->userID = row->getUint64(0);
        ch->deviceID = row->getUint64(1);
        ch->userport = row->getInt32(2);
        ch->deviceport = row->getInt32(3);
        ch->userfd = row->getInt32(4);
        ch->devfd = row->getInt32(5);
        ch->userSockAddr.fromString(row->getString(6));
        ch->deviceSockAddr.fromString(row->getString(7));
        channels.push_back(ch);
    }

    return channels;
}

void ChannelDatabase::deleteAll()
{
    auto conn = mysqlpool->get();
    if (!conn)
        return;
    int serverid = Configure::getConfig()->serverid;
    char query[1024];
    snprintf(query, sizeof(query),
        "delete from RelayChannels where serverid=%d", 
        serverid);
    conn->Delete(query);
}
