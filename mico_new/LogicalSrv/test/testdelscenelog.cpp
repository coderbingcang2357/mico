#include <assert.h>
#include <set>
#include <vector>
#include "./testdelscenelog.h"
#include "../dbaccess/scene/delsceneuploadlog.h"
#include "mysqlcli/mysqlconnection.h"
#include "util/util.h"
#include "util/logwriter.h"
#include "mysqlcli/imysqlconnpool.h"
#include "config/configreader.h"
#include "util/micoassert.h"
#include "../dbaccess/scene/idelscenefromfileserver.h"

#undef assert
#define assert mcassert

class TestPool : public IMysqlConnPool
{
public:
    TestPool()
    {
        MysqlConnection *c = new MysqlConnection;
        Configure *cfg = Configure::getConfig();
        SqlParam *p = new SqlParam;
        p->host = cfg->databasesrv_sql_host;
        p->port = cfg->databasesrv_sql_port;
        p->user = cfg->databasesrv_sql_user;
        p->passwd = cfg->databasesrv_sql_pwd;
        p->db = cfg->databasesrv_sql_db;
        if (c->connect(p))
        {
            m_conn = std::shared_ptr<MysqlConnection>(c);
        }
        else
        {
            mcassert(false, __LINE__, "conn db failed");
        }
    }

    std::shared_ptr<MysqlConnection> get()
    {
        return m_conn;
    }

private:
    std::shared_ptr<MysqlConnection> m_conn;
};

class DelSceneFromfileServerFake : public IDelScenefromFileServer
{
public:
    int postDeleteMsg(uint64_t sceneid, uint64_t userid)
    {
        cnt++;
    }

    int cnt = 0;
};


TestDelSceneUploadLog::TestDelSceneUploadLog()
{
    m_connpool = std::make_shared<TestPool>();
}

void TestDelSceneUploadLog::test()
{
    ::writelog("\n");
    ::writelog("TestDelSceneUploadLog\n");
    auto sql = m_connpool->get();
    if(!sql)
        assert(false, __LINE__);

    MysqlConnection *conn = sql.get();
    try 
    {
        int res = conn->Update("begin");
        assert(res >= 0, __LINE__);
        DelSceneFromfileServerFake delscenef;
        DelSceneUploadLog delscene(m_connpool.get(), &delscenef);
        delscene.today = "'2017-06-02 17:33:18'";

        std::set<uint64_t> idsinlog;
        delscene.readIdInLog(conn, &idsinlog);
        assert(idsinlog.size() == 4, __LINE__);

        uint64_t min = *idsinlog.begin();
        uint64_t max = *(--idsinlog.end());
        assert(min == 288230376151811759ll, __LINE__);
        assert(max == 288230376151811939ll, __LINE__);

        std::set<uint64_t> idinusescene;
        delscene.readIdsInTUserScene(conn, min, max, &idinusescene);
        assert(idinusescene.size() == 2, __LINE__);

        int r = delscene.delSceneUploadLog();
        assert(r == 0, __LINE__);

        idinusescene.clear();
        delscene.readIdsInTUserScene(conn, min, max, &idinusescene);
        assert(idinusescene.size() == 2, __LINE__);
        // UploadScneeLog中超时的记录被删除, 只第剩下6个
        res = conn->Select("select * from UploadSceneLog");
        int rc = conn->result()->rowCount();
        assert(rc == 6, __LINE__);
        // 执行之后场景: 288230376151811759 288230376151811939要被删除
        // 所以是查不到的
        res = conn->Select("select * from T_Scene where id in (288230376151811759,288230376151811939)");
        rc = conn->result()->rowCount();
        assert(rc == 0, __LINE__);

        assert(delscenef.cnt == 2, __LINE__);

    }
    catch (const std::string &i)
    {
        ::writelog(InfoType, "failed: %s", i.c_str());
    }
    int res = conn->Update("rollback");
    assert(res >= 0, __LINE__);
    ::writelog("TestDelSceneUploadLog end\n");
}

