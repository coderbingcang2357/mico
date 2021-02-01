#include "util/logwriter.h"
#include "config/configreader.h"
#include "../../fileuploadredis.h"
#include "serverinfo/serverinfo.h"
#include "./delscenefromfileserver.h"
#include "mysqlcli/imysqlconnpool.h"
#include "iscenedal.h"

DelSceneFromFileServer::DelSceneFromFileServer(ISceneDal *sd)
    : m_scenedal(sd)
{

}

int DelSceneFromFileServer::postDeleteMsg(uint64_t sceneid, uint64_t userid)
{
    uint32_t serverid;
    if (m_scenedal->getSceneServerid(sceneid, &serverid) != 0)
    {
        ::writelog("delete scene failed: get scene serverid failed.");
        return -1;
    }
    // post a message to file server
    Configure *cfg = Configure::getConfig();
    std::shared_ptr<ServerInfo> server = ServerInfo::getFileServerInfo(serverid);
    if (!server)
    {
        ::writelog(InfoType, "delscene get serverinfo failed of id: %d", serverid);
        return -1;
    }
    std::string ip = server->ip();
    FileUpAndDownloadRedis fr = FileUpAndDownloadRedis::getInstance(ip,
            cfg->file_redis_port,
            cfg->redis_pwd);
    std::string delscenemsg(std::to_string(sceneid));
    delscenemsg.append(":");
    delscenemsg.append(std::to_string(userid));
    fr.postDelSceneMessage(delscenemsg);
    return 0;
}

