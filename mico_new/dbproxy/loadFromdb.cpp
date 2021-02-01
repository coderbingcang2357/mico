#include "mysqlcli/mysqlconnpool.h"
#include "mysqlcli/mysqlconnection.h"
#include "loadFromdb.h"
#include "user/user.h"
#include "users.h"
#include "config/configreader.h"

void loadUserFromdb()
{
    MysqlConnection *conn = MysqlConnPool::getConnection();
    int res = conn->Select("select ID, account, password, mail, nickName, phone, signature, head from T_User");
    if (res == -1)
    {
        printf("select db failed.\n");
        assert(false);
        return ;
    }
    std::shared_ptr<MyResult> result = conn->result();
    int rowcount = result->rowCount();
    for (int i = 0; i < rowcount; ++i)
    {
        std::shared_ptr<User> usr = std::make_shared<User>();
        //UserInfo info;
        std::shared_ptr<MyRow> row = result->nextRow();
        assert(row);

        usr->info.userid = row->getUint64(0);
        usr->info.account = row->getString(1);
        usr->info.pwd = row->getString(2);
        usr->info.mail = row->getString(3);
        usr->info.nickName = row->getString(4);
        usr->info.phone = row->getString(5);
        usr->info.signature = row->getString(6);
        usr->info.headNumber = row->getInt32(7);

        Users::instance()->insert(usr);
    }

    // ID             | bigint(20)
    // account        | varchar(32) 
    // password       | char(33)    
    // mail           | varchar(32) 
    // phone          | varchar(24) 
    // secureMail     | varchar(32) 
    // nickName       | varchar(32) 
    // signature      | varchar(96) 
    // head           | tinyint(4)  
    // sex            | tinyint(4)  
    // birthday       | date        
    // region         | int(11)     
    // lastLoginTime  | datetime    
    // activateFlag   | tinyint(4)  
    // activationCode | int(11)     
    // createDate     | datetime    
    // deleteFlag     | tinyint(4)  
    // deleteDate     | datetime    
    // for (int i = 0; i < 10000; ++i)
    // {
    //     UserInfo info;
    //     info.userid = i;
    //     std::string data;
    //     data.append(std::string("abc")).append(std::to_string(i));
    //     info.account = data;
    //     info.pwd = data;
    //     info.mail = data;
    //     info.nickName = data;
    //     info.phone = data;
    //     info.signature = data;
    //     info.headNumber = 0;
    //     Users::instance()->insert(std::make_shared<User>(info));
    // }
}
