#ifndef IMYSQLCONNPOOL__H
#define IMYSQLCONNPOOL__H
#include <memory>

class MysqlConnection;
class IMysqlConnPool
{
public:
    virtual ~IMysqlConnPool(){}
    virtual std::shared_ptr<MysqlConnection> get() = 0;
};
#endif
