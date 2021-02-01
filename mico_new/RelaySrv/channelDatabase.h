#ifndef CHANNELDATABASE__H_
#define CHANNELDATABASE__H_

#include <stdint.h>
#include <netinet/in.h>
#include <list>
#include <memory>
#include "util/sock2string.h"

class IMysqlConnPool;
class ChannelInfo;
class ChannelDatabase
{
public:
    ChannelDatabase(IMysqlConnPool *p);
    int removeChannel(uint64_t userid, uint64_t deviceid);
    //  serverid      | int(10) unsigned    
    //  userid        | bigint(20) unsigned 
    //  deviceid      | bigint(20) unsigned 
    //  userport      | int(10) unsigned    
    //  deviceport    | int(10) unsigned    
    //  userfd        | int(11)             
    //  devicefd      | int(11)             
    //  useraddress   | varchar(100)        
    //  deviceaddress
    int insertChannel(uint64_t userid, uint64_t deviceid,
        uint16_t userport, uint16_t deviceport,
        int userfd, int devicefd,
        const sockaddrwrap &useraddr, const sockaddrwrap &deviceaddr);

    std::list<std::shared_ptr<ChannelInfo>> readChannel();
    void deleteAll();

private:
    IMysqlConnPool *mysqlpool;
};
#endif
