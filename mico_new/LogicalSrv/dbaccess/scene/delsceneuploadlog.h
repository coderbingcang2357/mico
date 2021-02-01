#ifndef DELSCENEUPLOADLOG__H
#define DELSCENEUPLOADLOG__H
#include <set>
#include <vector>
#include <stdint.h>
#include <string>
#include "idelsceneuploadlog.h"
class MysqlConnection;
class IMysqlConnPool;
class  IDelScenefromFileServer;
class DelSceneUploadLog : public IDelSceneUploadLog
{
public:
    DelSceneUploadLog(IMysqlConnPool *myconnpool, IDelScenefromFileServer *delfile);
    // 0 success else failed
    int delSceneUploadLog() override;
private:
    void readIdInLog(MysqlConnection *conn, std::set<uint64_t> *res);
    void readIdsInTUserScene(MysqlConnection *conn,
                            uint64_t min,
                            uint64_t max,
                            std::set<uint64_t> *idinusescene);
    int deletefromTscene(MysqlConnection *conn,
                        std::vector<uint64_t> *idshoulddel,
                        uint32_t pos,
                        uint32_t max);

    IMysqlConnPool *m_connpool;
    IDelScenefromFileServer *m_delscenefromfileserver;
    std::string today;
    friend class TestDelSceneUploadLog;
};
#endif

