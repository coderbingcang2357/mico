#include "util/logwriter.h"
#include "mysqlconnection.h"
#include <iostream>
#include <vector>
//#include "../Config/Debug.h"
//#include <mysql/errmsg.h>
MysqlConnection::MysqlConnection() : m_mysql(0), m_sqlRes(0)
{
}


MysqlConnection::~MysqlConnection()
{
    FreeRes();
    mysql_close(m_mysql);
    delete m_mysql;
}

//数据库连接函数
bool MysqlConnection::connect(const SqlParam &parameter)
{
    m_para = parameter;

    if (m_mysql == 0)
    {
        m_mysql = new MYSQL;
        mysql_init(m_mysql);
    }

    bool reconnect = 1;
    mysql_options(m_mysql, MYSQL_OPT_RECONNECT, &reconnect);
    mysql_options(m_mysql, MYSQL_SET_CHARSET_NAME, "UTF8");

    MYSQL* ret = mysql_real_connect(
        m_mysql,
        m_para.host.c_str(),
        m_para.user.c_str(),
        m_para.passwd.c_str(),
        m_para.db.c_str(),
        m_para.port,
        NULL,
        CLIENT_REMEMBER_OPTIONS);

    if(ret == NULL)
    {
        ::writelog(ErrorType,
            "sql connection failed: %d, %s,"
            "host: %s, port: %d, user: %s, db: %s\n",
            mysql_errno(m_mysql),
            mysql_error(m_mysql),
            m_para.host.c_str(),
            m_para.port,
            m_para.user.c_str(),
            m_para.db.c_str());
        return false;
    }
    // mysql_set_character_set(m_mysql, "UTF8");

    return true;
}

//插入数据到数据库
int MysqlConnection::Insert(const std::string &statement)
{
    FreeRes();
    int ret = mysql_real_query(m_mysql, statement.c_str(), statement.length());
    if(ret != 0) {
        //std::cout << "sql insert failed" << std::endl;
        ::writelog(ErrorType, "error %s : %s", statement.c_str(), mysql_error(m_mysql));
        std::cout << statement;
        ::writelog(ErrorType, "error %s", mysql_error(m_mysql));
        return -1;
    }
    
    if(mysql_field_count(m_mysql) == 0) {
        if(mysql_affected_rows(m_mysql) == 0) {
            //std::cout << "sql insert success, but affect no rows" << std::endl;
            return 1;
        }
    }
    return 0;
}

//从数据库中查找数据
int MysqlConnection::Select(const std::string &statement)
{
    FreeRes();
    //std::cout << statement << std::endl;
    int ret = mysql_real_query(m_mysql, statement.c_str(), statement.length());
    if(ret != 0) {
        //std::cout << "sql select failed" << std::endl;
        ::writelog(ErrorType, "error %s : %s", statement.c_str(), mysql_error(m_mysql));
        return -1;
    }
    m_sqlRes = mysql_store_result(m_mysql);
    if(m_sqlRes == NULL) {
        //std::cout << "sql select failed" << std::endl;
        FreeRes();
        ::writelog(ErrorType, "error %s : %s", statement.c_str(), mysql_error(m_mysql));
        return -1;
    }
    if(m_sqlRes->row_count == 0) {
        //std::cout << "sql select success, but return no res" << std::endl;
        return 1;        
    }

    return 0;
}

//更新数据库中内容
int MysqlConnection::Update(const std::string &statement)
{
    FreeRes();
    //std::cout << statement << std::endl;
    int ret = mysql_real_query(m_mysql, statement.c_str(), statement.length());
    if(ret != 0) {
        //std::cout << "sql update failed" << std::endl;
        ::writelog(ErrorType, "error %s : %s", statement.c_str(), mysql_error(m_mysql));
        return -1;
    }
    
    if(mysql_field_count(m_mysql) == 0) {
        if(mysql_affected_rows(m_mysql) == 0) {
            //std::cout << "sql update success, but affect no rows" << std::endl;
            return 1;
        }
    }
    return 0;
}

//删除数据库数据
int MysqlConnection::Delete(const std::string &statement)
{
    FreeRes();
    //std::cout << statement << std::endl;
    int ret = mysql_real_query(m_mysql, statement.c_str(), statement.length());
    if(ret != 0) {
        //std::cout << "sql delete failed" << std::endl;
        //std::cout << "return " << ret << std::endl;
        //std::cout << "error no: " << mysql_errno(m_mysql) << std::endl;
        //std::cout << "error msg: " << mysql_error(m_mysql) << std::endl;
        ::writelog(ErrorType, "error %s : %s", statement.c_str(), mysql_error(m_mysql));
        return -1;
    }
    
    if(mysql_field_count(m_mysql) == 0) {
        if(mysql_affected_rows(m_mysql) == 0) {
            //std::cout << "sql delete success, but affect no rows" << std::endl;
            return 1;
        }
    }
    return 0;    
}

int MysqlConnection::RealEscapeString(char* to, const char * from, unsigned int size)
{
    return mysql_real_escape_string(m_mysql, to, from, size);
}

std::string MysqlConnection::RealEscapeString(const std::string &s)
{
    std::vector<char> c(s.size() *2 +1, char(0));
    unsigned long len = mysql_real_escape_string(m_mysql, &(c[0]), s.c_str(), s.size());
    if (len == (unsigned long)-1 )
        return std::string("");
    else
        return std::string(&c[0], len);
}

//清除查找的缓存数据
void MysqlConnection::FreeRes()
{
    if(m_sqlRes != NULL) {
        mysql_free_result(m_sqlRes);
        m_sqlRes = NULL;
    }
}

MYSQL_ROW MysqlConnection::FetchRow(MYSQL_RES* res)
{
    return(mysql_fetch_row(res));
}

uint64_t MysqlConnection::getAutoIncID()
{
    return mysql_insert_id(m_mysql);
}


int32_t MyRow::getInt32(int col)
{
    if (m_row[col] == 0)
        return 0;

    return atoi(m_row[col]);
}

uint32_t MyRow::getUint32(int col)
{
    if (m_row[col] == 0)
        return 0;

    return uint32_t(atoll(m_row[col]));
}

int64_t MyRow::getInt64(int col)
{
    if (m_row[col] == 0)
        return 0;

    return atoll(m_row[col]);
}

uint64_t MyRow::getUint64(int col)
{
    if (m_row[col] == 0)
        return 0;

    uint64_t value;
    char *end = 0;
    value = strtoull(m_row[col], &end, 10);
    return value;
}

std::string MyRow::getString(int col)
{
    if (m_row[col] == 0)
        return std::string();

    return std::string(m_row[col]);
}
