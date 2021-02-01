#pragma once

#include <vector>
#include <list>
#include <string>
#include <map>
#include <memory>
#include "runningscenedata.h"
class IMysqlConnPool;


class RunningSceneDbaccess
{
public:
    RunningSceneDbaccess(IMysqlConnPool *mysqlpool);
    int insertRunningScene(const RunningSceneData &rs);
    int removeRunningScene(uint64_t sceneid);
    int getRunningScene(uint64_t sceneid, RunningSceneData *rsd);
    int getRunningSceneWithoutScenedata(uint64_t sceneid, RunningSceneData *rsd);
    int getRunningSceneOfServerid(int serverid, std::list<RunningSceneData> *sl);
    int setServeridOfRunningScene(uint64_t sceneid, int serverid);
	int GetUserIdBySceneId(uint64_t sceneid, uint64_t &userid);
	int getUserPhone(uint64_t sceneid, std::string *phone);
	int updatePhone(uint64_t sceneid, const std::string &phone);
	int GetAllUserIDOfScene(const std::string &sceneid,std::vector<std::pair<uint64_t,uint8_t>> &useridInfo);

private:
    IMysqlConnPool *m_mysqlpool;
};
