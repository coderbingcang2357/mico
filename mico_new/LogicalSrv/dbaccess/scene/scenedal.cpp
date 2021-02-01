#include <memory>
#include <functional>
#include "util/util.h"
#include "util/logwriter.h"
#include "mysqlcli/mysqlconnpool.h"
#include "mysqlcli/mysqlconnection.h"
#include "mysqlcli/imysqlconnpool.h"
#include "scenedal.h"

SceneDal::SceneDal(IMysqlConnPool *p) : m_connpool(p)
{
}

// 可以被共离的最大次次, 文档上写的32, 所以暂时直接写32了
// 以后可改成从数据库中取, 这样方便控制
int SceneDal::getMaxShareTimes()
{
    return 32;
}

// 取一个场景被共享了多少次
int SceneDal::getSharedTimes(uint64_t sceneid)
{
    std::unique_ptr<MysqlConnection, std::function<void(MysqlConnection *)> >
        sql(MysqlConnPool::getConnection(), [](MysqlConnection *p) {
                MysqlConnPool::release(p);
            });
    if(!sql)
        return -1;

    int times;
    std::string query("select count(*) from T_User_Scene where sceneID=");
    query.append(std::to_string(sceneid));
    int r = sql->Select(query);
    if (r == 0)
    {
        MYSQL_ROW row = sql->FetchRow(sql->Res());
        if (row[0] != 0)
        {
            times = atoi(row[0]);
        }
        else
        {
            times = -1;
        }
    }
    else
    {
        times = -1;
    }
    return times;
}

int SceneDal::getSceneServerid(uint64_t sceneid, uint32_t *serverid)
{
    auto sql = m_connpool->get();
    if (!sql)
    {
        ::writelog(ErrorType, "getscene serverid get sql conn failed.");
        return -1;
    }
    char buf[1024];
    snprintf(buf, sizeof(buf),
        "select fileserverid from T_Scene where ID=%lu",
        sceneid);
    if (sql->Select(buf) == 0)
    {
        MYSQL_RES *res = sql->Res();
        assert(res->row_count == 1);
        MYSQL_ROW row = sql->FetchRow(res);
        Str2Uint(row[0], *serverid);
        return 0;
    }
    else
    {
        return -1;
    }
}
