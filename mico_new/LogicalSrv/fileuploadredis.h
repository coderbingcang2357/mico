#ifndef FILEUPLOADREDIS__H
#define FILEUPLOADREDIS__H

#include <string>
#include <stdint.h>
#include <unordered_map>
#include "redisconnectionpool/redisconnectionpool.h"
#include "thread/mutexwrap.h"
#include "hiredis/hiredis.h"

class FileUpAndDownloadRedis
{
private:
    FileUpAndDownloadRedis(){}

public:
    static FileUpAndDownloadRedis getInstance(const std::string &ip, uint16_t port, const std::string &password);

    bool insertUploadUrl(const std::string &uid,  uint64_t sceneid, const std::string &filename, uint32_t filesize);
    bool deleteUploadUrl(const std::string &uid);
    bool insertDownloadUrl(const std::string &uid, uint64_t sceneid, const std::string &filename);
    bool deleteDownloadUrl(const std::string &uid);

    bool postDelSceneMessage(const std::string &msg);

private:
    std::shared_ptr<redisContext> rediscontext;
};

#endif
