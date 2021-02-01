#pragma once
#include <string>
//数据库链接参数配置结构体
struct SqlParam
{
    std::string host;    //数据库IP地址
    std::string user;    //用户名称
    std::string passwd;  //数据库密码
    std::string db;      //数据库名称
    unsigned int port = 3306;   //端口号

    bool isValid() {
        return !host.empty() && !user.empty();
    }
};
