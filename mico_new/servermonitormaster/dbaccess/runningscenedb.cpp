#include "runningscenedb.h"
#include "mysqlcli/mysqlconnection.h"
#include "mysqlcli/mysqlconnpool.h"
#include "util/base64.h"
#include "util/formatsql.h"
#include "util/logwriter.h"

RunningSceneDbaccess::RunningSceneDbaccess(IMysqlConnPool *mysqlpool)
    : m_mysqlpool(mysqlpool)
{
}

int RunningSceneDbaccess::insertRunningScene(const RunningSceneData &rs)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;
    std::string wechartphone = sql->RealEscapeString(rs.wechartphone);
    std::string msgphone = sql->RealEscapeString(rs.msgphone);
    std::string b64 = ::Base64Encode((uint8_t *)(rs.blobscenedata.c_str()), rs.blobscenedata.size());
    std::string query = FormatSql("insert into runningscene (sceneid, wecharten, msgen, wechartphone, msgphone, startdate, startuser, runnerserverid, scenedata) "
                                " values (%llu, %d, %d, \"%s\", \"%s\", now(), %llu, %d, \"%s\")",
                                rs.sceneid,
                                rs.wecharten,
                                rs.msgen,
                                wechartphone.c_str(),
                                msgphone.c_str(),
                                rs.startuserid,
                                rs.runningserverid,
                                b64.c_str());
    if (sql->Insert(query) == 0) // ok
        return 0;
    else
        return -1;
}

int RunningSceneDbaccess::removeRunningScene(uint64_t sceneid)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;
    std::string query = FormatSql("delete from runningscene where sceneid=%llu",
                                 sceneid);
    int r = sql->Delete(query);
    if (r != -1)
        return 0;
    else
        return -1;
}

int RunningSceneDbaccess::getRunningScene(uint64_t sceneid, RunningSceneData *rsd)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;
    std::string query = FormatSql( "select sceneid, wecharten, msgen, wechartphone, msgphone, startdate, startuser, runnerserverid, scenedata from runningscene where sceneid = %llu",
                                  sceneid);
    if (sql->Select(query) == 0)
    {
        auto res = sql->result();
        int c = res->rowCount();
        if (c == 0)
        {
            rsd->runningserverid = 0;
            return 0;
        }
        auto row = res->nextRow();
        rsd->sceneid = row->getUint64(0);
        rsd->wecharten = row->getUint32(1);
        rsd->msgen = row->getUint32(2);
        rsd->wechartphone = row->getString(3);
        rsd->msgphone = row->getString(4);
        rsd->startuserid = row->getUint64(6);
        rsd->runningserverid = row->getInt32(7);
        std::string s = row->getString(8);
        std::vector<uint8_t> d = Base64Decode(s);
        rsd->blobscenedata = std::string((char *)&d[0], d.size());
        return 0;
    }
    else
    {
        return -1;
    }
}

int RunningSceneDbaccess::getRunningSceneWithoutScenedata(uint64_t sceneid, RunningSceneData *rsd)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;
    std::string query = FormatSql( "select sceneid, wecharten, msgen, wechartphone, msgphone, startdate, startuser, runnerserverid from runningscene where sceneid = %llu",
                                  sceneid);
    if (sql->Select(query) == 0)
    {
        auto res = sql->result();
        int c = res->rowCount();
        if (c == 0)
            return -1;
        auto row = res->nextRow();
        rsd->sceneid = row->getUint64(0);
        rsd->wecharten = row->getUint32(1);
        rsd->msgen = row->getUint32(2);
        rsd->wechartphone = row->getString(3);
        rsd->msgphone = row->getString(4);
        rsd->startuserid = row->getUint64(6);
        rsd->runningserverid = row->getInt32(7);
        return 0;
    }
    else
    {
        return -1;
    }
}

int RunningSceneDbaccess::getRunningSceneOfServerid(int serverid, std::list<RunningSceneData> *sl)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;
    std::string query = FormatSql(
"select sceneid, wecharten, msgen, wechartphone, msgphone, startdate, startuser, runnerserverid, scenedata from runningscene where runnerserverid = %d",
        serverid);
    if (sql->Select(query) == 0)
    {
        auto res = sql->result();
        int c = res->rowCount();
        for (; c > 0; c--)
        {
            RunningSceneData rsd;
            auto row = res->nextRow();
            rsd.sceneid = row->getUint64(0);
            rsd.wecharten = row->getUint32(1);
            rsd.msgen = row->getUint32(2);
            rsd.wechartphone = row->getString(3);
            rsd.msgphone = row->getString(4);
            rsd.startuserid = row->getUint64(6);
            rsd.runningserverid = row->getInt32(7);
            std::string s = row->getString(8);
            std::vector<uint8_t> d = Base64Decode(s);
            rsd.blobscenedata = std::string((char *)&d[0], d.size());
            sl->push_back(rsd);
        }
        return 0;
    }
    else
    {
        return -1;
    }

}

int RunningSceneDbaccess::setServeridOfRunningScene(uint64_t sceneid, int serverid)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;
    std::string query = FormatSql("update runningscene set runnerserverid=%d where sceneid=%llu",
            serverid, sceneid);
    int r = sql->Update(query);
    if (r == 0 || r == 1)
        return 0;
    else
        return -1;
}

int RunningSceneDbaccess::GetUserIdBySceneId(uint64_t sceneid, uint64_t &userid){
	auto sql = m_mysqlpool->get();
	if(!sql)
		return -1;
	std::string query = FormatSql("select ownerID from T_Scene where ID = %llu",sceneid);
    if (sql->Select(query) == 0)
    {
        auto res = sql->result();
        int c = res->rowCount();
        if (c == 0)
            return -1;
        auto row = res->nextRow();
        userid = row->getUint64(0);
	}
	else{
		return -1;
	}
	return 0;
}

int RunningSceneDbaccess::getUserPhone(uint64_t sceneid, std::string *phone){
	uint64_t userid;
	GetUserIdBySceneId(sceneid, userid);
	auto sql = m_mysqlpool->get();
	if(!sql)
		return -1;
	std::string query = FormatSql("select phone from T_User where ID = %llu",userid);
    if (sql->Select(query) == 0)
    {
        auto res = sql->result();
        int c = res->rowCount();
        if (c == 0)
            return -1;
        auto row = res->nextRow();
        *phone = row->getString(0);
	}
	else{
		return -1;
	}
	return 0;
}
int RunningSceneDbaccess::updatePhone(uint64_t sceneid, const std::string &phone){
	uint64_t userid;
	GetUserIdBySceneId(sceneid, userid);
	auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;
    std::string phone_ = sql->RealEscapeString(phone);
    std::string query = FormatSql("update T_User set phone=\"%s\" where ID = %llu", phone_.c_str(), userid);
    int r = sql->Update(query);
    if (r == 0 || r == 1)
        return 0;
    else
        return -1;
}

int RunningSceneDbaccess::GetAllUserIDOfScene(const std::string &sceneid,std::vector<std::pair<uint64_t,uint8_t>> &useridInfo){
	auto sql = m_mysqlpool->get();
	if(!sql){
		::writelog(InfoType,"mysql connect error!");
		return -1;
	}
	std::string query = "select userID,authority from T_User_Scene where sceneID = "; 
	query += sceneid;
	int ret = sql->Select(query);
	if(!ret){
		auto res = sql->result();
		int c = res->rowCount();
		auto row = res->nextRow();
		for(; c>0;c--){
			uint64_t userid = row->getInt64(0);
			uint8_t authority = (uint8_t)row->getInt32(1);
			useridInfo.push_back({userid,authority});
			//::writelog(InfoType,"kkkkkkkkk:%llu",userid);
            row = res->nextRow();
		}
		return 0;
	}
	else{
		::writelog(InfoType, "msyql query error! sceneID:%llu",sceneid);
		return -1;
	}
}
