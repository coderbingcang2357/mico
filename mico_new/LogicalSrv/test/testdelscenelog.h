#ifndef TESTDELSCENELOG__H
#define TESTDELSCENELOG__H
#include <memory>
class IMysqlConnPool;
class TestDelSceneUploadLog
{
public:
    TestDelSceneUploadLog();
    void test();

private:
    std::shared_ptr<IMysqlConnPool> m_connpool;
};

#endif
