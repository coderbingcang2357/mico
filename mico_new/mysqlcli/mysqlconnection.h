#ifndef MY_SQL__CONNECTION_H
#define MY_SQL__CONNECTION_H

#include <mysql/mysql.h>
#include <assert.h>
#include <string>
#include <memory>
#include "sqlconnpara.h"

class MyResult;
class MyRow;

class MysqlConnection
{
public:
    MysqlConnection();
    ~MysqlConnection();
public:
    bool connect(const SqlParam &para); //数据库连接
    int Select(const std::string &sql);      //数据库查询
    int Insert(const std::string &sql);      //数据库插入
    int Update(const std::string &sql);      //数据库更新
    int Delete(const std::string &sql);      //数据库删除
    uint64_t getAutoIncID();

    MYSQL_RES* Res() { return m_sqlRes; }
    std::shared_ptr<MyResult> result() { return std::make_shared<MyResult>(m_sqlRes);}
    int RealEscapeString(char* to, const char * from, unsigned int size);
    std::string RealEscapeString(const std::string &s);

    static MYSQL_ROW FetchRow(MYSQL_RES* res);
private:
    void FreeRes();
private:
    MYSQL *m_mysql;
    MYSQL_RES* m_sqlRes;
    SqlParam m_para;
};

class MyResult
{
public:
    MyResult(MYSQL_RES *res) : m_sqlRes(res) {}
    int rowCount() { return mysql_num_rows(m_sqlRes); }
    std::shared_ptr<MyRow> nextRow() {return std::make_shared<MyRow>(mysql_fetch_row(m_sqlRes)); }

public:
    MYSQL_RES* m_sqlRes;
};

class MyRow
{
public:
    MyRow(MYSQL_ROW row) : m_row(row) {}

    int32_t getInt32(int col);
    uint32_t getUint32(int col);

    int64_t getInt64(int col);
    uint64_t getUint64(int col);

    std::string getString(int col);

private:
    MYSQL_ROW m_row;
};
#endif


