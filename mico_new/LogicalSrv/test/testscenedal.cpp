#include <assert.h>
#include "util/logwriter.h"
#include "testscenedal.h"
#include "../dbaccess/scene/scenedal.h"
#include "mysqlcli/imysqlconnpool.h"
#include "mysqlcli/mysqlconnection.h"

class MysqlConnpoolFake : public IMysqlConnPool
{
public:
    std::shared_ptr<MysqlConnection> get()
    {
        return std::make_shared<MysqlConnection>();
    }
};

void TestSceneDal::testSceneDal()
{
    ::writelog(InfoType, "\n\n--TestSceneDal");
    MysqlConnpoolFake pf;
    SceneDal sd(&pf);
    assert(sd.getMaxShareTimes() == 32);
    assert(sd.getSharedTimes(288230376151811753ll) == 2);
    ::writelog("ok");
}

