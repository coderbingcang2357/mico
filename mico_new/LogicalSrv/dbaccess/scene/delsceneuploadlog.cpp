#include <map>
#include <set>
#include <assert.h>
#include "delsceneuploadlog.h"
#include "mysqlcli/mysqlconnection.h"
#include "util/util.h"
#include "util/logwriter.h"
#include "mysqlcli/imysqlconnpool.h"
#include "idelscenefromfileserver.h"

DelSceneUploadLog::DelSceneUploadLog(IMysqlConnPool *myconnpool, IDelScenefromFileServer *delfile)
    : m_connpool(myconnpool), m_delscenefromfileserver(delfile), today("now()")
{
}

int DelSceneUploadLog::delSceneUploadLog()
{
    auto sql = m_connpool->get();
    if(!sql)
        return -1;
    ::writelog(InfoType, "start delscene log");
    // read scene log
    std::set<uint64_t> idsInLog;
    // read T_User_Scene where id in log
    readIdInLog(sql.get(), &idsInLog);
    if (idsInLog.size() == 0)
    {
        ::writelog(InfoType, "end delscene log");
        return 0;
    }

    uint64_t min = *idsInLog.begin();
    uint64_t max = *(--idsInLog.end());
    std::set<uint64_t> idinusescene;
    readIdsInTUserScene(sql.get(), min, max, &idinusescene);
    // if id not in set then delete from log and delete from T_Scene

    std::vector<uint64_t> idshoulddel(idsInLog.size());
    if (idinusescene.size() == 0)
    {
        std::copy(idsInLog.begin(), idsInLog.end(), idshoulddel.begin());
    }
    else
    {
        for (auto it = idsInLog.begin(); it != idsInLog.end(); ++it)
        {
            uint64_t id = *it;
            if (idinusescene.find(id) == idinusescene.end())
            {
                idshoulddel.push_back(id);
            }
        }
    }

    // delete from T_Scene
    if (idshoulddel.size() == 0)
    {
        ::writelog(InfoType, "end delscene log");
        return 0;
    }

    for (uint32_t i = 0; i < idshoulddel.size(); )
    {
        int r = deletefromTscene(sql.get(), &idshoulddel, i, 200);
        if (r == -1)
        {
            ::writelog(ErrorType, "delete from scene error.");
            ::writelog(InfoType, "end delscene log");
            return -1;
        }
        else
        {
            i += r;
        }
    }

    // delete from upload log
    // delete from UploadSceneLog where sceneid in (12,34)
    std::string delogquery("delete from UploadSceneLog where sceneid in ");
    std::string ids("(");
    uint32_t cnt = 0;
    for (auto pos = idsInLog.begin();pos != idsInLog.end();)
    {
        ids.append(std::to_string(*pos));
        ids.append(",");
        ++pos;
        cnt++;
        if (cnt >= 200 || pos == idsInLog.end())
        {
            cnt = 0;
            auto dot = ids.find_last_of(',');
            ids[dot] = ')';
            int res = sql->Delete(delogquery + ids);
            if (res == -1)
            {
                ::writelog(InfoType, "delscene log failed.3");
                ::writelog(InfoType, "end delscene log");
                return -1;
            }
        }
    }

    ::writelog(InfoType, "end delscene log");
    return 0;
}

void DelSceneUploadLog::readIdInLog(MysqlConnection *conn, std::set<uint64_t> *ids)
{
    const char *query="select sceneid from UploadSceneLog where uploadtime < Date_sub(%s, interval 1 day)";
    char querybuf[1024];
    snprintf(querybuf, sizeof(querybuf), 
        query,
        today.c_str());
    int res = conn->Select(querybuf);
    if (res >= 0)
    {
        std::shared_ptr<MyResult> result = conn->result();
        int rowcnt = result->rowCount();
        for (int i = 0; i < rowcnt; ++i)
        {
            std::shared_ptr<MyRow> arow = result->nextRow();
            uint64_t id = arow->getUint64(0);
            assert(id > 0);
            ids->insert(id);
        }
    }
}

void DelSceneUploadLog::readIdsInTUserScene(MysqlConnection *conn,
                        uint64_t min,
                        uint64_t max,
                        std::set<uint64_t> *idinusescene)
{
    char query[1024];
    snprintf(query, sizeof(query),
        "select sceneID from T_User_Scene where sceneID>=%lu && sceneID<=%lu",
        min, max);
    int res = conn->Select(query);
    if (res < 0)
    {
        return;
    }
    std::shared_ptr<MyResult> result = conn->result();
    int rowcnt = result->rowCount();
    for (int i = 0; i < rowcnt; ++i)
    {
        std::shared_ptr<MyRow> row = result->nextRow();
        uint64_t id = row->getUint64(0);
        assert(id>0);
        idinusescene->insert(id);
    }
}

int DelSceneUploadLog::deletefromTscene(MysqlConnection *conn,
                    std::vector<uint64_t> *idshoulddel,
                    uint32_t pos,
                    uint32_t max)
{
    const char *delquery_="delete from T_Scene where id in ";
    std::string delquery(delquery_);
    std::string ids("(");
    uint32_t i;
    for (i = pos; i < max && i < idshoulddel->size(); ++i)
    {
        // post a message to file server's redis
        //
        m_delscenefromfileserver->postDeleteMsg(idshoulddel->at(i), 0);

        ids.append(std::to_string(idshoulddel->at(i)));
        ids.append(",");
    }
    if (i == pos)
    {
        return 0;
    }

    auto it = ids.find_last_of(',');
    ids[it] = ')';
    delquery.append(ids);
    int res = conn->Delete(delquery);
    if (res < 0)
    {
        return -1;
    }
    else
    {
        return i - pos;
    }
}

