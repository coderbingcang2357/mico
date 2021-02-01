#include <memory>
#include <map>
#include <set>
#include <algorithm>
#include "useroperator.h"
#include "mysqlcli/mysqlconnection.h"
#include "dbcolumn.h"
#include "util/util.h"
#include "util/logwriter.h"
#include "mysqlcli/mysqlconnpool.h"
#include "protocoldef/protocol.h"
#include "thread/mutexwrap.h"
#include "thread/mutexguard.h"
#include "cacheduseroperator.h"

UserOperator::UserOperator(IMysqlConnPool *mp)
    : m_mysqlpool(mp)
{

}

UserOperator::~UserOperator()
{
    //MysqlConnPool::release(m_sql);
}

int UserOperator::registerUser(const std::string &username, 
    const std::string &mail,
    const std::string &md5password,
    uint64_t *userid)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;

    char buf[1024];
    int len;
    len = sql->RealEscapeString(buf, (char *)username.c_str(), username.size());
    std::string usernameescape(buf, len);
    len = sql->RealEscapeString(buf, (char *)mail.c_str(), mail.size());
    std::string mailescape(buf, len);

    char insertStatment[1024];
    sprintf(insertStatment,
        "insert into T_User (account, mail, password, createDate)"
        "values ('%s', '%s', '%s', now())",
        usernameescape.c_str(),
        mailescape.c_str(),
        md5password.c_str());
    std::string statement(insertStatment);
    int r = sql->Insert(statement);
    if (r == 0)
    {
        // ok
        uint64_t uid = sql->getAutoIncID();
        if (uid <= 0)
            return -1;
        *userid = uid;
        return 0;
    }
    else
    {
        return -1;
    }
}

std::map<uint64_t, std::string> idpwd;
std::map<std::string, std::string> accountpwd;
std::map<std::string, uint64_t> idaccount;

// 0 success
int UserOperator::getUserPassword(uint64_t userID, std::string *password)
{
    //CachedUserOperator cuop;
    //return cuop.getUserPassword(userID, password);

    //auto it = idpwd.find(userID);
    //if (it != idpwd.end())
    //{
    //    *password = it->second;
    //    return 0;
    //}

    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;

    string statement = EMPTY_STRING
           + "SELECT " + TUSER__PWD
           + " FROM " + TUSER
           + " WHERE " + TUSER__ID + "=\'" + Uint2Str(userID) + "\';";

    int result = sql->Select(statement);
    if(result == 0)
    {
        MYSQL_ROW row = sql->FetchRow(sql->Res());
        *password = GetString(row[0]);

        //idpwd.insert(std::make_pair(userID, *password));
    }
    else
        *password = EMPTY_STRING;

    return result;
}

int UserOperator::getUserPassword(const std::string &usraccount, std::string *password)
{
    //CachedUserOperator cuop;
    //return cuop.getUserPassword(usraccount, password);
    //auto it = accountpwd.find(usraccount);
    //if (it != accountpwd.end())
    //{
    //    *password = it->second;
    //    return 0;
    //}
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;

    string statement = EMPTY_STRING
           + "SELECT " + TUSER__PWD
           + " FROM " + TUSER
           + " WHERE " + TUSER__ACCOUNT + "=\'" + usraccount + "\';";

    int result = sql->Select(statement);
    if(result == 0)
    {
        MYSQL_ROW row = sql->FetchRow(sql->Res());
        *password = GetString(row[0]);
        //accountpwd.insert(std::make_pair(usraccount, *password));
    }
    else
        *password = EMPTY_STRING;

    return result;
}

int UserOperator::getUserIDByAccount(const std::string &usraccount,
    uint64_t *userid)
{
    static MutexWrap m;
    MutexGuard g(&m);

    auto it = idaccount.find(usraccount);
    if (it != idaccount.end())
    {
        *userid = it->second;
        return 0;
    }
    //CachedUserOperator cuop;
    //if (cuop.getUserIDByAccount(usraccount, userid) == 0)
    //{
    //    idaccount.insert(std::make_pair(usraccount, *userid));
    //    return 0;
    //}
    //else
    //{
    //    return -1;
    //}

    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;

    string statement = EMPTY_STRING
           + "SELECT " + TUSER__ID
           + " FROM " + TUSER
           + " WHERE " + TUSER__ACCOUNT + "=\'" + usraccount + "\';";

    int result = sql->Select(statement);
    if(result == 0) {
        MYSQL_ROW row = sql->FetchRow(sql->Res());
        Str2Uint(row[0], *userid);
        idaccount.insert(std::make_pair(usraccount, *userid));
    }
    else
        *userid = 0;

    return result;
}

int UserOperator::getUserAccount(uint64_t userid, std::string *account)
{
    UserInfo *u = this->getUserInfo(userid);
    if (u != 0)
    {
        *account = u->account;
        delete u;
        return 0;
    }
    else
    {
        return -1;
    }
}

bool UserOperator::isUserExist(const std::string &usraccount)
{
    //CachedUserOperator cuop;
    //return cuop.isUserExist(usraccount);

    auto sql = m_mysqlpool->get();
    if(!sql)
        return false;

    string statement = EMPTY_STRING
        + "SELECT 1"
        + " FROM " + TUSER
        + " WHERE " + TUSER__ACCOUNT + "=\'" + usraccount + "\'" + " LIMIT 1;";

    int result = sql->Select(statement);
    return (result == 0) ? true : false;
}

std::map<uint64_t, UserInfo> ui;

UserInfo *UserOperator::getUserInfo(uint64_t userid)
{
    //CachedUserOperator cuop;
    //return cuop.getUserInfo(userid);
    //auto it = ui.find(userid);
    //if (it != ui.end())
    //{
    //    UserInfo *uit = new UserInfo;
    //    *uit = it->second;
    //    return uit;
    //}
    auto sql = m_mysqlpool->get();
    if(!sql)
        return 0;

    //string statement = EMPTY_STRING
    //       + "SELECT "
    //       + TUSER__ACCOUNT + ", "
    //       + TUSER__NICKNAME + ", "
    //       + TUSER__SIGNATURE + ", "
    //       + TUSER__MAIL + ", "
    //       + TUSER__PHONE + ", "
    //       + TUSER__HEAD
    //       + " FROM " + TUSER
    //       + " WHERE " + TUSER__ID + "=\'" + Uint2Str(userid) + "\';";

    //UserInfo *udt = new UserInfo;
    //udt->userid = 100039;
    //udt->account = "wqs1";
    //return udt;
    char buf[1024];
    snprintf(buf, sizeof(buf),
        "select account, password, nickName, signature, mail, phone, head from T_User where ID=%lu",
        userid);

    int result = sql->Select(std::string(buf));
    UserInfo *ud = 0;
    if(result == 0)
    {
        ud = new UserInfo;
        MYSQL_ROW row = sql->FetchRow(sql->Res());

        ud->userid = userid;
        ud->account = GetString(row[0]);
        ud->nickName = GetString(row[1]);
        ud->pwd = GetString(row[2]);
        ud->signature = GetString(row[3]);
        ud->mail = GetString(row[4]);
        ud->phone = GetString(row[5]);
        Str2Uint(row[6], ud->headNumber);
        //userInfo.id = userID;
        //userInfo.account = GetString(row[0]);
        //userInfo.nickName = GetString(row[1]);
        //userInfo.signature = GetString(row[2]);
        //Str2Uint(row[3], userInfo.head);
        //ui.insert(std::make_pair(userid, *ud));
    }

    return ud;
}

UserInfo *UserOperator::getUserInfo(const std::string &useraccount)
{
    //CachedUserOperator cuop;
    //return cuop.getUserInfo(useraccount);

    auto sql = m_mysqlpool->get();
    if(!sql)
        return 0;

    char buf[1024];
    int len = sql->RealEscapeString(buf,
                const_cast<char*>(useraccount.c_str()),
                useraccount.size());
    assert(uint64_t(len) < sizeof(buf));
    std::string useraccountAfterEscape(buf, len);
    //string statement = EMPTY_STRING
    //       + "SELECT "
    //       + TUSER__ID + ", "
    //       + TUSER__NICKNAME + ", "
    //       + TUSER__SIGNATURE + ", "
    //       + TUSER__HEAD + ", "
    //       + TUSER__PHONE + ", "
    //       + TUSER__MAIL
    //       + " FROM " + TUSER
    //       + " WHERE " + TUSER__ACCOUNT + "=\'" + useraccountAfterEscape + "\';";

    char bufsql[1024];
    snprintf(bufsql, sizeof(bufsql),
        "select ID, password, nickName, signature, head, phone, mail from T_User where account='%s'",
        useraccountAfterEscape.c_str());

    int result = sql->Select(std::string(bufsql));
    UserInfo *ud = 0;
    if(result == 0)
    {
        ud = new UserInfo;
        MYSQL_ROW row = sql->FetchRow(sql->Res());

        ud->userid = StrtoU64(row[0]);
        ud->account = useraccount;
        ud->pwd = GetString(row[1]);
        //ud->account = GetString(row[0]);
        ud->nickName = GetString(row[2]);
        ud->signature = GetString(row[3]);
        Str2Uint(row[4], ud->headNumber);
        ud->phone = GetString(row[5]);
        ud->mail = std::string(row[6]);

        //userInfo.id = userID;
        //userInfo.account = GetString(row[0]);
        //userInfo.nickName = GetString(row[1]);
        //userInfo.signature = GetString(row[2]);
        //Str2Uint(row[3], userInfo.head);
    }

    return ud;
}

// 0 success, otherwise failed
int UserOperator::modifyPassword(uint64_t userid, const std::string &newpassword)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;

    assert(newpassword.size() == 32);

    string statement = EMPTY_STRING
           + "UPDATE " + TUSER
           + " SET " + TUSER__PWD + "=" + "\'" + newpassword + "\'"
           + " WHERE " + TUSER__ID + "=" + "\'" + Uint2Str(userid) + "\';";

    // 0 success, otherwish failed
    return sql->Update(statement);
}

// 0 success, else failed
int UserOperator::modifyMail(uint64_t userid, const std::string &mail)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;

    char mailbuf[1024];
    int len = sql->RealEscapeString(mailbuf, (char *)mail.c_str(), mail.size());
    std::string mailstr(mailbuf, len);
    // is mail used??
    char sqlbuf[1024];
    snprintf(sqlbuf, sizeof(sqlbuf),
        "select 1 from T_User where mail='%s'",
        mailstr.c_str());
    int res = sql->Select(sqlbuf);
    if (res == 0)
    {
        return ANSC::EMAIL_USED;
    }
    else if (res < 0) // error
    {
        return res;
    }
    // else ok, mail is valid

    string statement = EMPTY_STRING
           + "UPDATE " + TUSER
           + " SET " + TUSER__MAIL + "=" + "\'" + mail + "\'"
           + " WHERE " + TUSER__ID + "=" + "\'" + Uint2Str(userid) + "\';";

    return sql->Update(statement);
}

// 0, success, others failed
int UserOperator::modifyNickName(uint64_t userid,
    const std::string &newnickname)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;

    char buf[1024];
    int len = sql->RealEscapeString(buf,
        (char *)newnickname.c_str(),
        newnickname.size());
    assert(uint64_t(len) < sizeof(buf));
    std::string nickname(buf, len);

    string statement = EMPTY_STRING
           + "UPDATE " + TUSER
           + " SET " + TUSER__NICKNAME + "=" + "\'" + nickname + "\'"
           + " WHERE " + TUSER__ID + "=" + "\'" + Uint2Str(userid) + "\';";

    // 0, success, others failed
    return sql->Update(statement);
}

// 0, success, otehrs failed
int UserOperator::modifySignature(
    uint64_t userid, const std::string &newsignature)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;

    char buf[1024];
    int len;
    len = sql->RealEscapeString(buf,
                        (char *)newsignature.c_str(),
                        newsignature.size());
    assert(uint64_t(len) < sizeof(buf));
    std::string signature(buf, len);
    string statement = EMPTY_STRING
           + "UPDATE " + TUSER
           + " SET " + TUSER__SIGNATURE + "=" + "\'" + signature + "\'"
           + " WHERE " + TUSER__ID + "=" + "\'" + Uint2Str(userid) + "\';";

    // 0, success, otehrs failed
    return sql->Update(statement);
}

int UserOperator::modifyUserHead(uint64_t userid, uint8_t header)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;

    std::string statement = EMPTY_STRING
           + "UPDATE " + TUSER
           + " SET " + TUSER__HEAD + "=" +  std::to_string(header).c_str()
           + " WHERE " + TUSER__ID + "=" + "\'" + Uint2Str(userid) + "\';";

    // 0, success, otehrs failed
    return sql->Update(statement);
}

// 0 success, 
int UserOperator::getMyDeviceList(uint64_t userid,
    std::vector<DeviceInfo> *out)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;

    static std::string stat_("select ID, type, macAddr, name, notename, description, clusterID "
                            "from T_Device join T_User_Device on ID=deviceID and userID=");

    std::string statement = stat_ + std::to_string(userid);
    //string statement = EMPTY_STRING
    //       + "SELECT "
    //       + TDEVICE__ID + ", "
    //       + TDEVICE__TYPE + ", "
    //       + TDEVICE__MAC_ADDR + ", "
    //       + TDEVICE__NAME + ", "
    //       + TDEVICE__NOTENAME + ", "
    //       + TDEVICE__DESCRIPTION + ", "
    //       + TDEVICE__CLUSTER_ID
    //       + " FROM " + TDEVICE
    //       + " WHERE " + TDEVICE__ID  + " IN"
    //       + "(SELECT " + TUSER_DEVICE__DEVICE_ID
    //       + " FROM " + TUSER_DEVICE
    //       + " WHERE " + TUSER_DEVICE__USER_ID + "=\'" + Uint2Str(userid) + "\');";

    int result = sql->Select(statement);
    if(result >= 0)
    {
        for(my_ulonglong i = 0; i != sql->Res()->row_count; i++)
        {
            MYSQL_ROW row = sql->FetchRow(sql->Res());

            DeviceInfo deviceInfo;
            Str2Uint(row[0], deviceInfo.id);
            Str2Uint(row[1], deviceInfo.type);
            uint64_t mac;
            Str2Uint(row[2], mac);
            char macchar[6];
            uint64ToMacAddr(mac, macchar);
            deviceInfo.macAddr = std::vector<char>(macchar, macchar + 6);
            deviceInfo.name = GetString(row[3]);
            deviceInfo.noteName = GetString(row[4]);
            deviceInfo.description = GetString(row[5]);

            Str2Uint(row[6], deviceInfo.clusterID);

            out->push_back(deviceInfo);
        }
        return 0;
    }
    else
    {
        return -1;
    }
}

int UserOperator::removeMyDevice(uint64_t userid, std::list<uint64_t> &deviceid)
{
    if (deviceid.size() == 0)
        return 0;

    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;
    std::string statement = EMPTY_STRING
        + "DELETE "
        + " FROM " + TUSER_DEVICE      // 用户 -- 设备 关系表
        + " WHERE " + TUSER_DEVICE__USER_ID + "=\'" + Uint2Str(userid) + "\'"
        + " AND " + TUSER_DEVICE__DEVICE_ID + " IN ("; // devviceid 在map中存在

    for(auto iter = deviceid.begin();
        iter != deviceid.end();
        ++iter)
    {
        statement.append(Uint2Str(*iter));
        statement.append(",");
    }

    statement[statement.size() - 1] = ')';

    return sql->Delete(statement);
}

int UserOperator::getDeviceName(uint64_t deviceID, std::string *deviceName)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;
    std::string statement = EMPTY_STRING
           + "SELECT " + TDEVICE__NAME
           + " FROM " + TDEVICE
           + " WHERE " + TDEVICE__ID + "=\'" + Uint2Str(deviceID) + "\';";

    int result = sql->Select(statement);
    if(result == 0) {
        MYSQL_ROW row = sql->FetchRow(sql->Res());
        *deviceName = GetString(row[0]);
    }
    else
        *deviceName = EMPTY_STRING;

    return result;

}

int UserOperator::modifyDeviceName(uint64_t devid, const std::string &name)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;

    char buf[1024];
    int len = sql->RealEscapeString(buf, (char *)name.c_str(), name.size());
    std::string deviceName(buf, len);
    string statement = EMPTY_STRING
           + "UPDATE " + TDEVICE
           + " SET " + TDEVICE__NAME + "=" + "\'" + deviceName + "\'"
           + " WHERE " + TDEVICE__ID + "=" + "\'" + Uint2Str(devid) + "\';";

    return sql->Update(statement);
}

// 0 success,
int UserOperator::modifyNoteName(uint64_t deviceID, const std::string &noteName)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;

    char buf[1024];
    int len = sql->RealEscapeString(buf,
        (char *)noteName.c_str(),
        noteName.size());
    assert(len < int(sizeof(buf)));

    std::string notename(buf, len);
    string statement = EMPTY_STRING
           + "UPDATE " + TDEVICE
           + " SET " + TDEVICE__NOTENAME + "=" + "\'" + notename + "\'"
           + " WHERE " + TDEVICE__ID + "=" + "\'" + Uint2Str(deviceID) + "\';";

    // 0 success
    return sql->Update(statement);
}

int UserOperator::getDeviceIDAndLoginKey(uint64_t firmClusterID,
        uint64_t macAddr,
        uint64_t *deviceID,
        std::string *strLoginKey)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;

    std::string statement = EMPTY_STRING
           + "SELECT "
           + TDEVICE__ID + ", "
           + TDEVICE__LOGIN_KEY
           + " FROM " + TDEVICE
           + " WHERE "
           + TDEVICE__FIRM_CLUSTER_ID + "=\'" + Uint2Str(firmClusterID) + "\' AND "
           + TDEVICE__MAC_ADDR + "=\'" + Uint2Str(macAddr) + "\' AND "
           + TDEVICE__NEW_PROTOCOL_FLAG + "=\'1\' AND "
           + TDEVICE__DELETE_FLAG + "=0;"; // 没有删除标志

    int result = sql->Select(statement);
    if(result == 0) {
        MYSQL_ROW row = sql->FetchRow(sql->Res());
        Str2Uint(row[0], *deviceID);
        std::string hexloginkey = GetString(row[1]);
        // 数据库里面存的是hex, 所以要把hex转成byte
        std::vector<char> byteloginkey;
        assert(hexloginkey.size() == 32);
        ::hexToBytesArray(hexloginkey, &byteloginkey);
        *strLoginKey = std::string(&(byteloginkey[0]), byteloginkey.size());
    }

    return result;
}

int UserOperator::setDeviceStatus(const std::list<uint64_t> &ids,
                                  uint8_t status)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;
    std::string strid;
    bool isfirst = true;
    for (auto it = ids.begin(); it != ids.end(); ++it)
    {
        if (!isfirst)
        {
            strid.append(",");
            isfirst = false;
        }
        strid.append(std::to_string(*it));
    }

    int size = snprintf(nullptr, 0,
                        "update T_Device set status = %u where ID in (%s)",
                        status, strid.c_str());
    std::vector<char> buf(size+1, char('\0'));
    snprintf(buf.data(), buf.size(),
            "update T_Device set status = %u where ID in (%s)",
            status, strid.c_str());
    return sql->Update(buf.data());
}

DeviceInfo *UserOperator::getDeviceInfo(uint64_t deviceid)
{
    DeviceInfo *d = 0;
    auto sql = m_mysqlpool->get();
    if(!sql)
        return 0;

    string statement = EMPTY_STRING
           + "SELECT "
           + TDEVICE__NAME + ", "
           + TDEVICE__MAC_ADDR + ", "
           + TDEVICE__CLUSTER_ID + ", "
           + TDEVICE__LOGIN_KEY + ", "
           + " status "
           + " FROM " + TDEVICE
           + " WHERE " + TDEVICE__ID + "=\'" + Uint2Str(deviceid) + "\';";

    int result = sql->Select(statement);
    if(result == 0)
    {
        d = new DeviceInfo;
        d->id = deviceid;
        MYSQL_ROW row = sql->FetchRow(sql->Res());

        d->name = GetString(row[0]);
        // mac, 数据库里面存的是mac转成整数后的值
        uint64_t macint;
        Str2Uint(row[1], macint);
        char macbuf[6];
        uint64ToMacAddr(macint, macbuf);
        d->macAddr = std::vector<char>(macbuf, macbuf + sizeof(macbuf));
        //
        uint64_t clusterID;
        Str2Uint(row[2], clusterID);
        d->clusterID = clusterID;

        // 数据库里面存的是hex, 所以要把hex转成byte
        std::vector<char> byteloginkey;
        ::hexToBytesArray(std::string(row[3]), &byteloginkey);
        Str2Uint(row[4], d->status);
        //d->loginkey = byteloginkey;
        d->loginkey.swap(byteloginkey);
    }

    return d;
}

int UserOperator::getUserIDInDeviceCluster(uint64_t clusterid,
    std::list<uint64_t> *idlist)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;

    string statement = EMPTY_STRING
           + "SELECT " + TUSER_DEVCL__USER_ID
           + " FROM " + TUSER_DEVCL
           + " WHERE " + TUSER_DEVCL__DEVCLUSTER_ID + "=" + "\'" + Uint2Str(clusterid) + "\';";

    int result = sql->Select(statement);
    if(result == 0)
    {
        uint64_t userMemberID = 0;
        for(my_ulonglong i = 0; i != sql->Res()->row_count; i++)
        {
            MYSQL_ROW row = sql->FetchRow(sql->Res());
            Str2Uint(row[0], userMemberID);
            idlist->push_back(userMemberID);
        }
    }

    return result;
}

//---------------
int UserOperator::getClusterAccount(uint64_t clusterID, std::string *account)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;

    string statement = EMPTY_STRING
           + "SELECT " + TDEVCL__ACCOUNT
           + " FROM " + TDEVCL
           + " WHERE " + TDEVCL__ID + "=\'" + Uint2Str(clusterID) + "\';";

    int result = sql->Select(statement);
    if(result == 0) {
        MYSQL_ROW row = sql->FetchRow(sql->Res());
        *account = GetString(row[0]);
    }
    else
        *account = EMPTY_STRING;

    return result;
}

int UserOperator::getSysAdminID(uint64_t clusterID, uint64_t *ownerid)
{
    *ownerid = getDeviceClusterOwner(clusterID);
    if (*ownerid != 0)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

int UserOperator::checkClusterAdmin( uint64_t userid,
    uint64_t clusterid,
    bool *isadmin)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;

    // role: 1 系统管理员(群所有者), 2 管理员, 3 操作员
    string statement = EMPTY_STRING
       + "SELECT 1"
       + " FROM " + TUSER_DEVCL
       + " WHERE " + TUSER_DEVCL__USER_ID + "=" + "\'" + Uint2Str(userid) + "\'"
       + " AND " + TUSER_DEVCL__DEVCLUSTER_ID + "=\'" + Uint2Str(clusterid) + "\'"
       + " AND " + TUSER_DEVCL__ROLE + "<\'3\';"; // role < 3

    int result = sql->Select(statement);
    if(result == 0)
        *isadmin = true;
    else
        *isadmin = false;

    return result >= 0 ? 0 : -1;
}


int UserOperator::checkClusterAccount(const std::string &clusterAccount,
    bool *isExisted)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;

    string statement = EMPTY_STRING
           + "SELECT 1"
           + " FROM " + TDEVCL
           + " WHERE " + TDEVCL__ACCOUNT + "=\'" + clusterAccount + "\'" + " LIMIT 1;";

    int result = sql->Select(statement);
    *isExisted = (result == 0) ? true : false;

    return result;
}

int UserOperator::insertCluster(uint64_t creatorID,
                    const std::string &clusterAccount,
                    const std::string &description,
                    uint64_t *newClusterID)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;
    // ??? 没有type参数???, 但是数据库中type写的是notnull, 难道会自动赋值???
    uint64_t clusterID;

    char clusteraccbuf[1024];
    int len = sql->RealEscapeString(clusteraccbuf,
        (char *)clusterAccount.c_str(),
        clusterAccount.size());
    assert(len > 0 && len < int(sizeof(clusteraccbuf)));
    std::string clusteracc(clusteraccbuf, len);

    char descbuf[2048];
    len = sql->RealEscapeString(descbuf,
        (char *)description.c_str(),
        description.size());
    assert(len < int(sizeof(descbuf)));
    std::string desc(descbuf, len);

    string statement = EMPTY_STRING
           + "INSERT INTO " + TDEVCL
           + "("
           //+ TDEVCL__ID + ", "
           + TDEVCL__ACCOUNT + ", "
           + TDEVCL__DESCRIPTION + ", "
           + TDEVCL__CREATOR_ID + ", "
           + TDEVCL__SYSADMIN_ID + ", "
           + TDEVCL__CREATE_DATE
           + ")"
           + " VALUES (\'"
           //+ Uint2Str(clusterID) + "\', \'"
           + clusteracc + "\', \'"
           + desc + "\', \'"
           + Uint2Str(creatorID) + "\', \'"
           + Uint2Str(creatorID) + "\', "
           + "NOW()"
           + ");";

    int result = sql->Insert(statement);
    if(result < 0)
    {
        return result;
    }
    // get id
    clusterID = sql->getAutoIncID();
    assert(clusterID > 0);

    // 把创建者加到群中, role的默认值是1, 表示群主, 所以这里不用写
    statement = EMPTY_STRING
              + "INSERT INTO " + TUSER_DEVCL
              + "("
              + TUSER_DEVCL__USER_ID + ", "
              + TUSER_DEVCL__DEVCLUSTER_ID + ")"
              + " VALUES (\'" + Uint2Str(creatorID) + "\', \'" + Uint2Str(clusterID) + "\');";

    *newClusterID = clusterID;

    int r = sql->Insert(statement);
    assert(r == 0);
    return r;
}

// 0 success else failed
int UserOperator::modifyClusterNoteName(uint64_t userid,
            uint64_t clusterID,
            const std::string &noteName)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;

    char notenamebuf[1024];
    int len = sql->RealEscapeString(notenamebuf, (char *)noteName.c_str(),
        noteName.size());
    assert(len < int(sizeof(notenamebuf)));
    std::string newNoteName(notenamebuf, len);

    char sqlbuf[1024];
    snprintf(sqlbuf, sizeof(sqlbuf), 
        "update T_User_DevCluster "
        "set notename='%s' "
        "where userID=%lu and deviceClusterID=%lu;",
        newNoteName.c_str(),
        userid,
        clusterID);
    return sql->Update(std::string(sqlbuf));
}

// 0 success else failed
int UserOperator::modifyClusterDescription(uint64_t userID,
    uint64_t clusterID,
    const std::string &newDescription)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;

    char descbuf[2048];
    int len = sql->RealEscapeString(descbuf, (char *)newDescription.c_str(),
        newDescription.size());
    assert(len < int(sizeof(descbuf)));
    std::string desc(descbuf, len);

    string statement = EMPTY_STRING
           + "UPDATE " + TDEVCL
           + " SET " + TDEVCL__DESCRIPTION + "=\'" + desc + "\'"
           + " WHERE " + TDEVCL__ID + "=\'" + Uint2Str(clusterID) + "\'"
           + " AND " + TDEVCL__SYSADMIN_ID + "=\'" + Uint2Str(userID) + "\';";

    // 0 success else failed
    return sql->Update(statement);
}

int UserOperator::getClusterListOfUser(uint64_t userid,
    std::vector<DevClusterInfo> *vecClusterInfo)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;
    // 取群id列表, 用户加入了哪些群
    string statement = EMPTY_STRING
           + "SELECT "
           + TUSER_DEVCL__DEVCLUSTER_ID + ", "
           + TUSER_DEVCL__ROLE + ", "
           + " notename "
           + " FROM " + TUSER_DEVCL
           + " WHERE " + TUSER_DEVCL__USER_ID + "=\'" + Uint2Str(userid) + "\';";

    int result = sql->Select(statement);
    if(result < 0)
    {
        return result;
    }

    std::map<uint64_t, std::pair<uint8_t, std::string> > mapDevClusterIDRole;

    std::string idlist("(");
    for(my_ulonglong i = 0; i != sql->Res()->row_count; i++)
    {
        uint64_t deviceClusterID = 0;
        uint8_t role = 0;
        MYSQL_ROW row = sql->FetchRow(sql->Res());
        Str2Uint(row[0], deviceClusterID);
        Str2Uint(row[1], role);
        std::string notename = GetString(row[2]);
        mapDevClusterIDRole.insert(std::make_pair(deviceClusterID,
            std::make_pair(role, notename)));
        idlist.append(std::to_string(deviceClusterID));
        idlist.append(",");
    }
    // 一个群没有
    if (mapDevClusterIDRole.size() == 0)
        return 0;
    vecClusterInfo->reserve(mapDevClusterIDRole.size());
    // .
    if (idlist.back() == ',')
    {
        idlist.pop_back();
    }
    idlist.append(")");

    // 取这些id对应的cluster的信息
    std::string getinfostatement("select ID, account, describ from T_DeviceCluster where ID in ");
    getinfostatement.append(idlist);
    result = sql->Select(getinfostatement);
    if (result == 0)
    {
        std::shared_ptr<MyResult> r = sql->result();
        uint32_t rowcount = r->rowCount();
        if (rowcount != mapDevClusterIDRole.size())
        {
            ::writelog(InfoType, "getclusterlist info count not eq id count: %lu", userid);
        }
        // 把取到的信息和mapDapClusterIDRolw合并, 保存到DevClusterInfo
        for (uint32_t i = 0; i < rowcount; ++i)
        {
            std::shared_ptr<MyRow> row = r->nextRow();
            uint64_t clusterid = row->getUint64(0);
            auto mapdevit = mapDevClusterIDRole.find(clusterid);
            if (mapdevit == mapDevClusterIDRole.end())
            {
                ::writelog(InfoType,
                        "getclusterlist id not in mapDevClusterIDRole:user:%lu,clusterid:%lu",
                        userid,
                        clusterid);
                continue;
            }
            DevClusterInfo clusterInfo;
            clusterInfo.role = mapdevit->second.first;
            clusterInfo.id = mapdevit->first;
            clusterInfo.account = row->getString(1);//GetString(row[0]);
            clusterInfo.noteName = mapdevit->second.second;//GetString(row[1]);
            clusterInfo.description = row->getString(2);//GetString(row[2]);
            vecClusterInfo->push_back(clusterInfo);
        }
    }
    else if (result == 1)
    {
        ::writelog(InfoType, "getclusterlist getinfo error:%lu", userid);
        return 0;
    }
    else
    {
        // sql exec error
        return -1;
    }
    return result;
}

int UserOperator::getClusterIDListOfUser(uint64_t userid, std::vector<uint64_t> *o)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;

    // 取群id列表, 用户加入了哪些群
    //string statement = EMPTY_STRING
    //       + "SELECT "
    //       + TUSER_DEVCL__DEVCLUSTER_ID
    //       + " FROM " + TUSER_DEVCL
    //       + " WHERE " + TUSER_DEVCL__USER_ID + "=\'" + Uint2Str(userid) + "\';";
    //char sqlbuf[1024];
    //snprintf(sqlbuf, sizeof(sqlbuf),
    //    "select deviceClusterID from T_User_DevCluster where userID=%lu",
    //    userid);
    std::string statement("select deviceClusterID from T_User_DevCluster where userID=");
    //std::string statement("select * from UserCluster where ID=");
    statement.append(std::to_string(userid));

    int result = sql->Select(statement);
    if(result < 0)
    {
        return result;
    }

    uint64_t deviceClusterID = 0;
    for(my_ulonglong i = 0; i != sql->Res()->row_count; i++)
    {
        MYSQL_ROW row = sql->FetchRow(sql->Res());
        Str2Uint(row[0], deviceClusterID);
        o->push_back(deviceClusterID);
    }

    return 0;
}

int UserOperator::getClusterInfo(uint64_t userid,
            uint64_t clusterID,
            DevClusterInfo *clusterInfo)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;

    string statement = EMPTY_STRING
           + "SELECT "
           + TDEVCL__ACCOUNT + ", "
           + TDEVCL__NOTENAME + ", "
           + TDEVCL__DESCRIPTION
           + " FROM " + TDEVCL
           + " WHERE " + TDEVCL__ID + "=\'" + Uint2Str(clusterID) + "\';";

    int result = sql->Select(statement);
    if(result != 0)
        return result;

    MYSQL_ROW row = sql->FetchRow(sql->Res());
    clusterInfo->id = clusterID;
    clusterInfo->account = GetString(row[0]);
    clusterInfo->noteName = GetString(row[1]);
    clusterInfo->description = GetString(row[2]);
    // 取userr的权限role...
    statement = EMPTY_STRING
           + "SELECT "
           + TUSER_DEVCL__ROLE + ", "
           + " notename "
           + " FROM " + TUSER_DEVCL
           + " WHERE " + TUSER_DEVCL__DEVCLUSTER_ID + "=\'" + Uint2Str(clusterID) + "\'"
           + " AND " + TUSER_DEVCL__USER_ID + "=\'" + Uint2Str(userid) + "\';";

    result = sql->Select(statement);
    if(result < 0)
        return result;

    if(sql->Res()->row_count != 0)
    {
        row = sql->FetchRow(sql->Res());
        Str2Uint(row[0], clusterInfo->role);
        clusterInfo->noteName = GetString(row[1]);
    }
    else
        clusterInfo->role = 0;

    return result;
}

int UserOperator::getDevMemberInCluster(
    uint64_t clusterID, std::vector<DeviceInfo> *vDeviceInfo)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;

    string statement = EMPTY_STRING
        + "SELECT "
        + TDEVICE__ID + ", "
        + TDEVICE__MAC_ADDR + ", "
        + TDEVICE__NAME + ", "
        + TDEVICE__NOTENAME + ", "
        + TDEVICE__DESCRIPTION + ", "
        + TDEVICE__TYPE
        + " FROM " + TDEVICE
        + " WHERE " + TDEVICE__CLUSTER_ID + "=" + Uint2Str(clusterID)
        + " and status=" + Uint2Str(DeviceStatu::Active);

    int result = sql->Select(statement);
    if(result == 0) {
        DeviceInfo deviceInfo;
        for(my_ulonglong i = 0; i != sql->Res()->row_count; i++) {
            MYSQL_ROW row = sql->FetchRow(sql->Res());

            Str2Uint(row[0], deviceInfo.id);
            uint64_t mac;
            Str2Uint(row[1], mac);
            char macbuf[6];
            uint64ToMacAddr(mac, macbuf);
            deviceInfo.macAddr = std::vector<char>(macbuf, macbuf + 6);
            deviceInfo.name = GetString(row[2]);
            deviceInfo.noteName = GetString(row[3]);
            deviceInfo.description = GetString(row[4]);
            Str2Uint(row[5], deviceInfo.type);

            vDeviceInfo->push_back(deviceInfo);
        }
    }

    return result;
}

int UserOperator::searchCluster(
    const std::string &clusterAccount, DevClusterInfo* clusterInfo)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;
   //string statement = EMPTY_STRING
   //        + "SELECT "
   //        + TDEVCL__ID + ", "
   //        + TDEVCL__ACCOUNT + ", "
   //        + TDEVCL__NOTENAME + ", "
   //        + TDEVCL__DESCRIPTION
   //        + " FROM " + TDEVCL
   //        + " WHERE " + TDEVCL__ACCOUNT + "=\'" + clusterAccount + "\';";

    // ..
    if (clusterAccount.size() > 255 || clusterAccount.size() <= 0)
    {
        return -1;
    }

    char buf[1024];
    int len = sql->RealEscapeString(buf, (char *)clusterAccount.c_str(),
        clusterAccount.size());
    assert(len > 0);
    std::string searchAccount(buf, len);

    char statement[1024];
    snprintf(statement, sizeof(statement),
        "select ID, account, notename, describ "
        "from T_DeviceCluster where account = '%s';",
        searchAccount.c_str());

    int result = sql->Select(statement);
    if(result == 0)
    {
        MYSQL_ROW row = sql->FetchRow(sql->Res());

        clusterInfo->role = 0;
        Str2Uint(row[0], clusterInfo->id);
        clusterInfo->account = GetString(row[1]);
        clusterInfo->noteName = GetString(row[2]);
        clusterInfo->description = GetString(row[3]);
        writelog(InfoType, "search : %s, %s, %s", clusterInfo->account.c_str(), clusterInfo->noteName.c_str(), clusterInfo->description.c_str());
    }

    return result;
}

// 取设备群的所有者, 对应就是数据库中的sysAdminID字段
uint64_t UserOperator::getDeviceClusterOwner(uint64_t deviceclusterid)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
    {
        ::writelog("getDeviceClusterOwner get .");
        return 0;
    }

    char statement[1024];
    snprintf(statement, sizeof(statement),
        "select sysAdminID from T_DeviceCluster where ID=%lu",
        deviceclusterid);
    if (sql->Select(std::string(statement)) != 0)
    {
        ::writelog("getDeviceClusterOwner select failed..");
        return 0;
    }
    MYSQL_RES *res = sql->Res();
    MYSQL_ROW row = sql->FetchRow(res);
    uint64_t ownerid;
    Str2Uint(row[0], ownerid);

    return ownerid;
}

int UserOperator::modifyClusterOwner(uint64_t clusterid,
    uint64_t newowner,
    uint64_t oldowner)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;
    if (sql->Update("begin") == -1)
    {
        ::writelog(InfoType,
                   "modifyClusterOwner begin failed.%lu, %lu",
                   oldowner, clusterid);
        return -1;
    }

    char statement[1024];
    int res = 0;
     // 把原来的设为管理员
    snprintf(statement, sizeof(statement),
        "update T_User_DevCluster set role=%u where userid=%lu and deviceClusterID=%lu",
        ClusterAuth::MANAGER,
        oldowner,
        clusterid);
    res = sql->Update(statement);
    if (res != 0)
    {
        ::writelog(InfoType,
                "modifyClusterOwner.failed.1: %lu .%lu", oldowner, clusterid);
        sql->Update("rollback");
        return -1;
    }

    // 把新的设为所有者
    snprintf(statement, sizeof(statement),
        "update T_User_DevCluster set role=%u where userid=%lu and deviceClusterID=%lu",
        ClusterAuth::OWNER,
        newowner,
        clusterid);
    res = sql->Update(statement);
    if (res != 0)
    {
        ::writelog(InfoType,
                "modifyClusterOwner.failed.2: %lu .%lu", newowner, clusterid);
        sql->Update("rollback");
        return -1;
    }

    // 还要修改群信息里面的管理员
    snprintf(statement, sizeof(statement),
        "update T_DeviceCluster set sysAdminID=%lu where ID=%lu",
        newowner,
        clusterid);

    // 0 success, else failed
    res = sql->Update(statement);
    if (res != 0)
    {
        ::writelog(InfoType,
                "modifyClusterOwner.failed.3: %lu .%lu", newowner, clusterid);
        sql->Update("rollback");
        return -1;
    }
    // commit
    res = sql->Update("commit");
    if (res == -1)
    {
        ::writelog(InfoType,
                   "modifyClusterOwner.commit failed.4: %lu .%lu",
                   oldowner, clusterid);
        sql->Update("rollback");
        return -1;
    }
    else
    {
        return 0;
    }
}

// 设备群中是否有设备
bool UserOperator::deviceClusterHasDevice(uint64_t deviceclusterid)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;

    char statement[1024];
    snprintf(statement, sizeof(statement),
        "select 1 from T_Device where clusterID=%lu and status=%u limit 1",
        deviceclusterid,
        DeviceStatu::Active);
    if (sql->Select(statement) == 0)
    {
        // 0 表示返回的结果[不]为空, 就是有设备在群里面
        return true;
    }
    else
    {
        return false;
    }
}

// 删除一个设备群
int UserOperator::deleteDeviceCluster(uint64_t deviceclusterid)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;

    if (sql->Update("begin") == -1)
    {
        ::writelog("del cluster begin failed.");
        return -1;
    }

    char statement[1024];
    snprintf(statement, sizeof(statement),
        "delete from T_DeviceCluster where ID=%lu",
        deviceclusterid);
    sql->Delete(statement);

    char statement2[1024];
    snprintf(statement2, sizeof(statement2),
        "delete from T_User_DevCluster where deviceClusterID=%lu",
        deviceclusterid);
    sql->Delete(statement2);

    if (sql->Update("commit") == -1)
    {
        ::writelog("del cluster commit failed.");
        sql->Update("rollback");
        return -1;
    }
    else
    {
        return 0;
    }
}

// 用户退出群, 从群里面删除用户
int UserOperator::deleteUserFromDeviceCluster(uint64_t userid,
    uint64_t devclusterid)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;

    char statement[1024];
    snprintf(statement, sizeof(statement),
        "delete from T_User_DevCluster where userID=%lu and deviceClusterID=%lu",
        userid,
        devclusterid);

    if (sql->Delete(statement) == -1)
        return -1;

    // delete device 
    snprintf(statement, sizeof(statement),
        "delete from T_User_Device where userID=%lu and deviceID in (select id from T_Device where clusterID = %lu)",
        userid,
        devclusterid);

    if (sql->Delete(statement) == -1)
        return -1;
    else
        return 0;
}

// 取得群里面某一个设备的所有使用者
int UserOperator::getOperatorListOfDevMember(uint64_t clusterID,
    uint64_t deviceMemberID,
    std::vector<DeviceAutho> *vDeviceAutho)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;
    // 语句1:
    // select   distinct T_User_Device.userID, T_User_Device.authority, T_User.account
    // from     T_User_Device
    // join     T_User on T_User_Device.userID = T_User.ID
    // where    T_User_Device.userID in
    //      (select T_User_Device.userID
    //      from T_User_Device
    //      where T_User_Device.deviceID='72057594037927940');
    // 实现方法2:
    // select   T_User.ID, T_User.account, T_User_Device.authority
    // from     T_User
    // join     T_User_Device on T_User.ID=T_User_Device.userID
    // where    T_User_Device.deviceID=72057594037927940;
    //
    // 没有判断设备的便用者是否在群里面, 这个语句是上面的实现方法1
    //std::string statement = EMPTY_STRING
    //       + "SELECT DISTINCT "
    //       + TUSER_DEVICE + "." + TUSER_DEVICE__USER_ID + ", "
    //       + TUSER_DEVICE + "." + TUSER_DEVICE__AUTHORITY + ", "
    //       + TUSER + "." + TUSER__ACCOUNT
    //       + " FROM " + TUSER_DEVICE + " JOIN " + TUSER
    //       + " ON " + TUSER_DEVICE + "." + TUSER_DEVICE__USER_ID + "=" + TUSER + "." + TUSER__ID
    //       + " WHERE " + TUSER_DEVICE + "." + TUSER_DEVICE__USER_ID + " IN "
    //       + "(SELECT " + TUSER_DEVICE__USER_ID
    //       + " FROM " + TUSER_DEVICE
    //       + " WHERE " + TUSER_DEVICE__DEVICE_ID + "=\'" + Uint2Str(deviceMemberID) + "\');";
    char statement[1024];
    snprintf(statement, sizeof(statement),
                    " select   T_User.ID, T_User_Device.authority, T_User.account, T_User.signature, T_User.head "
                    " from     T_User "
                    " join     T_User_Device on T_User.ID=T_User_Device.userID "
                    " where    T_User_Device.deviceID=%lu",
                    deviceMemberID);


    int result = sql->Select(std::string(statement));

    if(result == 0)
    {
        for(my_ulonglong i = 0; i != sql->Res()->row_count; i++)
        {
            MYSQL_ROW row = sql->FetchRow(sql->Res());

            DeviceAutho deviceAuto;
            Str2Uint(row[0], deviceAuto.id);
            Str2Uint(row[1], deviceAuto.authority);
            deviceAuto.name = GetString(row[2]);
            deviceAuto.signature = GetString(row[3]);
            Str2Uint(row[4], deviceAuto.header);

            vDeviceAutho->push_back(deviceAuto);
        }
    }

    return result;
}

// 取群里面某一个人可以用的设备
int UserOperator::getDeviceListOfUserMember(uint64_t clusterID,
    uint64_t userMemberID,
    std::vector<DeviceAutho> *vDeviceAutho)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;

    //string statement = EMPTY_STRING
    //       + "SELECT DISTINCT "
    //       + TUSER_DEVICE + "." + TUSER_DEVICE__DEVICE_ID + ", "
    //       + TUSER_DEVICE + "." + TUSER_DEVICE__AUTHORITY + ", "
    //       + TDEVICE + "." + TDEVICE__NAME
    //       + " FROM " + TUSER_DEVICE + " JOIN " + TDEVICE
    //       + " ON " + TUSER_DEVICE + "." + TUSER_DEVICE__DEVICE_ID + "=" + TDEVICE + "." + TDEVICE__ID
    //       + " WHERE " + TDEVICE__CLUSTER_ID + "=\'" + Uint2Str(clusterID) + "\'"
    //       + " AND " + TUSER_DEVICE + "." + TUSER_DEVICE__DEVICE_ID + " IN "
    //       + "(SELECT " + TUSER_DEVICE__DEVICE_ID
    //       + " FROM " + TUSER_DEVICE
    //       + " WHERE " + TUSER_DEVICE__USER_ID + "=\'" + Uint2Str(userMemberID) + "\');";
    char statement[1024];
    snprintf(statement, sizeof(statement),
            " select  T_Device.ID, T_User_Device.authority, T_Device.name, T_Device.type, T_Device.macAddr "
            " from    T_Device "
            " join    T_User_Device on T_Device.id=T_User_Device.deviceID "
            " where   T_User_Device.userID=%lu and T_Device.clusterID=%lu",
            userMemberID,
            clusterID);

    int result = sql->Select(std::string(statement));
    if(result == 0)
    {
        for(my_ulonglong i = 0; i != sql->Res()->row_count; i++) {
            MYSQL_ROW row = sql->FetchRow(sql->Res());

            DeviceAutho deviceAuto;
            Str2Uint(row[0], deviceAuto.id);
            Str2Uint(row[1], deviceAuto.authority);
            deviceAuto.name = GetString(row[2]);
            Str2Uint(row[3], deviceAuto.type);
            uint64_t mac;
            Str2Uint(row[4], mac);
            char macbuf[6];
            uint64ToMacAddr(mac, macbuf);
            deviceAuto.macAddr = std::vector<char>(macbuf, macbuf + 6);

            vDeviceAutho->push_back(deviceAuto);
        }
    }

    return result;
}

int UserOperator::getUserMemberInfoList(uint64_t clusterID,
    std::vector<UserMemberInfo> *vUserInfo)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;

    std::string statement("SELECT T_User_DevCluster.userID, "
                                 "T_User_DevCluster.role, "
                                 "T_User.account, "
                                 "T_User.nickName, "
                                 "T_User.signature, "
                                 "T_User.head "
                            "FROM T_User_DevCluster "
                            "JOIN T_User "
                            "ON T_User_DevCluster.userID=T_User.ID "
                            "WHERE deviceClusterID=");
    statement.append(std::to_string(clusterID));
    //string statement = EMPTY_STRING
    //       + "SELECT DISTINCT "
    //       + TUSER_DEVCL + "." + TUSER_DEVCL__USER_ID + ", "
    //       + TUSER_DEVCL + "." + TUSER_DEVCL__ROLE + ", "
    //       + TUSER + "." + TUSER__ACCOUNT + ", "
    //       + TUSER + "." + TUSER__NICKNAME + ", "
    //       + TUSER + "." + TUSER__SIGNATURE + ", "
    //       + TUSER + "." + TUSER__HEAD
    //       + " FROM " + TUSER_DEVCL + " JOIN " + TUSER
    //       + " ON " + TUSER_DEVCL + "." + TUSER_DEVCL__USER_ID + "=" + TUSER + "." + TUSER__ID
    //       + " WHERE (" + TUSER_DEVCL + "." + TUSER_DEVCL__USER_ID + ", " + TUSER_DEVCL + "." + TUSER_DEVCL__ROLE + ") IN "
    //       + "(SELECT " + TUSER_DEVCL__USER_ID + ", " + TUSER_DEVCL__ROLE
    //       + " FROM " + TUSER_DEVCL
    //       + " WHERE " + TUSER_DEVCL__DEVCLUSTER_ID + "=\'" + Uint2Str(clusterID) + "\');";

    int result = sql->Select(statement);
    if(result == 0) {
        UserMemberInfo userInfo;
        for(my_ulonglong i = 0; i != sql->Res()->row_count; i++) {
            MYSQL_ROW row = sql->FetchRow(sql->Res());

            Str2Uint(row[0], userInfo.id);
            Str2Uint(row[1], userInfo.role);
            userInfo.account = GetString(row[2]);
            userInfo.nickName = GetString(row[3]);
            userInfo.signature = GetString(row[4]);
            Str2Uint(row[5], userInfo.head);

            vUserInfo->push_back(userInfo);
        }
    }

    return result;
}

int UserOperator::claimDevice(uint64_t userid, uint64_t deviceid)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;

    // 验证用户和设备应该同群
    std::shared_ptr<DeviceInfo> di(this->getDeviceInfo(deviceid));
    if (!di || di->status != DeviceStatu::Active)
        return -1;
    std::vector<uint64_t> clusterids;
    this->getClusterIDListOfUser(userid, &clusterids);
    if (std::find(clusterids.begin(), clusterids.end(), di->clusterID) == clusterids.end())
    {
        return -1;
    }

    // 插入认领信息
    std::string statement = EMPTY_STRING
        + "INSERT INTO " + TUSER_DEVICE
        + "("
        + TUSER_DEVICE__USER_ID + ", "
        + TUSER_DEVICE__DEVICE_ID + ", "
        + TUSER_DEVICE__AUTHORITY + ", "
        + TUSER_DEVICE__AUTHORIZER_ID + ")"
        + " VALUES "
        + "(\'" + Uint2Str(userid)
        + "\', \'"  + Uint2Str(deviceid)
        + "\', \'" + Uint2Str(uint8_t(2))             // 2表示读写权限
        + "\', \'" + Uint2Str(uint64_t(0)) +"\'),";   // 这一项是授权者id, 但是认领没有授权者,所以就写0表示认领

    statement[statement.size() - 1] = ';';

    return sql->Insert(statement);
}

// 修改设备群号
int UserOperator::modifyDevicesOwnership(const std::vector<uint64_t> &vDeviceID,
        uint64_t srcClusterID,
        uint64_t destClusterID)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;
    std::string deviceIDList = EMPTY_STRING;
    for(size_t i= 0; i< vDeviceID.size(); ++i)
    {
        deviceIDList += Uint2Str(vDeviceID[i]);
        if (i != vDeviceID.size() - 1)
            deviceIDList += ",";
    }
    // deviceIDList.erase(vDeviceID.size() - 1);

    string statement = EMPTY_STRING
           + "DELETE"
           + " FROM " + TUSER_DEVICE
           + " WHERE " + TUSER_DEVICE__DEVICE_ID
           + " IN ("
           + deviceIDList
           + ");";

    int result = sql->Delete(statement);
    if(result < 0)
        return result;

    statement = EMPTY_STRING
        + "UPDATE " + TDEVICE
        + " SET " + TDEVICE__CLUSTER_ID + "=" + "\'" + Uint2Str(destClusterID) + "\'"
        + " WHERE " + TDEVICE__CLUSTER_ID + "=" + "\'" + Uint2Str(srcClusterID) + "\'"
        + " AND " + TDEVICE__ID
        + " IN ("
        + deviceIDList
        + ");";

    return sql->Update(statement);
}

int UserOperator::asignDevicesToUserMember(
    uint64_t adminID,
    uint64_t clusterID,
    uint64_t userMemberID,
    const std::map<uint64_t, uint8_t> &mapDeviceIDAuthority)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return ANSC::FAILED;

    // 验证autherizerID是否有权限把设备给别人用
    string statement = EMPTY_STRING
           + "SELECT 1"
           + " FROM " + TUSER_DEVCL
           + " WHERE " + TUSER_DEVCL__DEVCLUSTER_ID + "=\'" + Uint2Str(clusterID) + "\'"
           + " AND " + TUSER_DEVCL__USER_ID + "=\'" + Uint2Str(adminID) + "\'"
           + " AND " + TUSER_DEVCL__ROLE + "<=" + "\'2\';"; 

    int result = sql->Select(statement);
    if(result != 0)
        return ANSC::AUTH_ERROR;

    // 验证设备是否在群中, 人是否在群中
    std::list<uint64_t> devids;
    devids.resize(mapDeviceIDAuthority.size(), uint64_t(0));
    std::transform(mapDeviceIDAuthority.begin(), mapDeviceIDAuthority.end(),
        devids.begin(), [](const std::pair<uint64_t, uint8_t> &v){
            return v.first;
            });
    if (!isAllDeviceInCluster(clusterID, devids))
        return ANSC::DEVICE_ID_ERROR;
    std::list<uint64_t> userids {userMemberID};
    if (!isUserInCluster(clusterID, userids))
        return ANSC::USER_ID_OR_DEVICE_ID_ERROR;
    //把用户插入到时 用户-设备 关联表中
    statement = EMPTY_STRING
        + "INSERT INTO " + TUSER_DEVICE
        + "("
        + TUSER_DEVICE__USER_ID + ", "
        + TUSER_DEVICE__DEVICE_ID + ", "
        + TUSER_DEVICE__AUTHORITY + ", "
        + TUSER_DEVICE__AUTHORIZER_ID
        + ")"
        + " VALUES ";

    for(auto iter = mapDeviceIDAuthority.begin(); iter != mapDeviceIDAuthority.end(); ++iter)
        statement = statement
            + "(\'" + Uint2Str(userMemberID)
            + "\', \'" + Uint2Str(iter->first)
            + "\', \'" + Uint2Str(iter->second)
            + "\', \'" + Uint2Str(adminID) +"\'),";

    statement[statement.size() - 1] = ';';

    if (sql->Insert(statement) != 0)
    {
        return ANSC::FAILED;
    }
    else
    {
        return 0;
    }
}

int UserOperator::removeDevicesOfUserMember(uint64_t adminID,
                uint64_t clusterID,
                uint64_t userMemberID,
                const std::map<uint64_t, uint8_t> &mapDeviceIDAuthority)
{
    (void)adminID;
    (void)clusterID;
    // 验证 authorizerID 是不是群管理员
    //string statement = EMPTY_STRING
    //       + "SELECT 1"
    //       + " FROM " + TDEVCL
    //       + " WHERE " + TDEVCL__ID + "=\'" + Uint2Str(clusterID) + "\'" // 群id
    //       + " AND " + TDEVCL__SYSADMIN_ID + "=\'" + Uint2Str(adminID) + "\';"; // 管理员id

    //int result = sql->Select(statement);
    //if(result != 0)
    //{
    //    ::writelog(InfoType, "remove dev of mem no auth.%s", statement.c_str());
    //    return -1;
    //}

    //todo FIXME 验证设备是否在群里面

    // 从 用户 -- 设备 关系表中删除 map 中的device
    std::list<uint64_t> devices;
    for (auto it = mapDeviceIDAuthority.begin();
         it != mapDeviceIDAuthority.end(); ++it)
    {
        devices.push_back(it->first);
    }
    return this->removeMyDevice(userMemberID, devices);
}

int UserOperator::assignDeviceToManyUser(uint64_t authorizerID,
                uint64_t clusterID,
                uint64_t deviceID,
                const std::map<uint64_t, uint8_t> &mapUserIDAuthority)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return ANSC::FAILED;
    // 验证 authorizerID 权限
    string statement = EMPTY_STRING
           + "SELECT 1"
           + " FROM " + TUSER_DEVCL
           + " WHERE " + TUSER_DEVCL__DEVCLUSTER_ID + "=\'" + Uint2Str(clusterID) + "\'" // 是群里面的user
           + " AND " + TUSER_DEVCL__USER_ID + "=\'" + Uint2Str(authorizerID) + "\'"
           + " AND " + TUSER_DEVCL__ROLE + "<=" + "\'2\';"; // 有权限

    int result = sql->Select(statement);
    if (result != 0)
        return ANSC::AUTH_ERROR;
    // 检查用户是否在群里面, 设备是否在群里面
    std::list<uint64_t> devids{deviceID};
    if (!isAllDeviceInCluster(clusterID, devids))
        return ANSC::DEVICE_ID_ERROR;
    std::list<uint64_t> userids;
    userids.resize(mapUserIDAuthority.size(), uint64_t(0));
    std::transform(mapUserIDAuthority.begin(), mapUserIDAuthority.end(),
        userids.begin(), [](const std::pair<uint64_t, uint8_t> &u){
            return u.first;
            });
    if (!isUserInCluster(clusterID, userids))
        return ANSC::USER_ID_OR_DEVICE_ID_ERROR;
    // 给设备指定操作员, 就是在 user - device 表中添加一项
    statement = EMPTY_STRING
        + "INSERT INTO " + TUSER_DEVICE
        + "("
        + TUSER_DEVICE__USER_ID + ", "
        + TUSER_DEVICE__DEVICE_ID + ", "
        + TUSER_DEVICE__AUTHORITY + ", "
        + TUSER_DEVICE__AUTHORIZER_ID + ")"
        + " VALUES ";

    for(auto iter = mapUserIDAuthority.begin(); iter != mapUserIDAuthority.end(); ++iter)
        statement = statement
            + "(\'" + Uint2Str(iter->first)
            + "\', \'"  + Uint2Str(deviceID)
            + "\', \'" + Uint2Str(iter->second)
            + "\', \'" + Uint2Str(authorizerID) +"\'),";

    statement[statement.size() - 1] = ';';

    if (sql->Insert(statement) != 0)
    {
        return ANSC::FAILED;
    }
    else
    {
        return 0;
    }
}

int UserOperator::removeDeviceUsers( uint64_t authorizerID,
                uint64_t clusterID,
                uint64_t deviceID,
                const std::map<uint64_t, uint8_t> &mapUserIDAuthority)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;
    // authorizerID 必需是这个设备的 管理员
    char sqlbuf[1024];
    snprintf(sqlbuf, sizeof(sqlbuf),
        "select 1 from T_User_DevCluster where userid=%lu and deviceclusterid=%lu and (role=%d or role=%d)",
        authorizerID,
        clusterID,
        ClusterAuth::OWNER,
        ClusterAuth::MANAGER);

    int result = sql->Select(sqlbuf);
    if(result != 0)
        return ANSC::AUTH_ERROR;

    // 从 用户 -- 设备 表中删除
    std::string statement = EMPTY_STRING
        + "DELETE "
        + " FROM " + TUSER_DEVICE
        + " WHERE " + TUSER_DEVICE__DEVICE_ID + "=\'" + Uint2Str(deviceID) + "\'"
        + " AND " + TUSER_DEVICE__USER_ID + " IN (";

    for(auto iter = mapUserIDAuthority.begin();
        iter != mapUserIDAuthority.end();
        ++iter)
    {
        statement = statement + "\'" + Uint2Str(iter->first) + "\',";
    }

    statement[statement.size() - 1] = ')';
    statement = statement + ';';

    return sql->Delete(statement);
}

int UserOperator::insertUserMember(uint64_t userid, uint64_t clusterID)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;

    string statement = EMPTY_STRING
           + "INSERT INTO " + TUSER_DEVCL
           + "("
           + TUSER_DEVCL__USER_ID + ", "
           + TUSER_DEVCL__DEVCLUSTER_ID + ", "
           + TUSER_DEVCL__ROLE + ")"
           + " VALUES (\'" + Uint2Str(userid) + "\', \'" + Uint2Str(clusterID) + "\', \'" + "3" + "\');";

    return sql->Insert(statement);
}

// 把用户从群里面删除
int UserOperator::removeUserMembers(uint64_t adminID,
    std::vector<uint64_t> &vUserMemberID,
    uint64_t clusterID)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;

    std::string statement = EMPTY_STRING
        + "SELECT 1"
        + " FROM " + TUSER_DEVCL
        + " WHERE " + TUSER_DEVCL__USER_ID + "=\'" + Uint2Str(adminID) + "\'"
        + " AND " + TUSER_DEVCL__DEVCLUSTER_ID + "=\'" + Uint2Str(clusterID) + "\'"
        + " AND " + TUSER_DEVCL__ROLE + "<\'3\';";

    int result = sql->Select(statement);
    if(result != 0)
        return -1;

    statement = EMPTY_STRING
        + "DELETE"
        + " FROM " + TUSER_DEVCL
        + " WHERE " + TUSER_DEVCL__DEVCLUSTER_ID + "=\'" + Uint2Str(clusterID) + "\'";

    statement = statement + " AND " + TUSER_DEVCL__USER_ID + " IN(";

    for(size_t i = 0; i != vUserMemberID.size(); i++)
    {
        statement = statement + "\'" + Uint2Str(vUserMemberID.at(i)) + "\',";
    }
    statement[statement.size() - 1] = ')';
    statement.push_back(';');

    if (sql->Delete(statement) == -1)
        return -1;

    std::string userids;
    for(size_t i = 0; i != vUserMemberID.size(); i++)
    {
        userids += std::to_string(vUserMemberID[i]);
        if (i != vUserMemberID.size() - 1)
            userids+=",";
    }

    char statbuf[1024];
    snprintf(statbuf, sizeof(statbuf),
        "delete from T_User_Device where userID in (%s) and deviceID in (select id from T_Device where clusterID = %lu)",
        userids.c_str(),
        clusterID);

    if (sql->Delete(statbuf) == -1)
        return -1;
    else
        return 0;
}

int UserOperator::modifyUserMemberRole(uint64_t adminid,
                        uint64_t userMemberID,
                        uint8_t newRole,
                        uint64_t clusterID)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;

    std::string statement = EMPTY_STRING
           + "SELECT 1"
           + " FROM " + TDEVCL
           + " WHERE " + TDEVCL__ID + "=\'" + Uint2Str(clusterID) + "\'"
           + " AND " + TDEVCL__SYSADMIN_ID + "=\'" + Uint2Str(adminid) + "\';";

    int result = sql->Select(statement);
    if(result != 0)
        return -1;

    statement = EMPTY_STRING
        + "UPDATE " + TUSER_DEVCL
        + " SET " + TUSER_DEVCL__ROLE + "=" + "\'" + Uint2Str(newRole) + "\'"
        + " WHERE " + TUSER_DEVCL__USER_ID + "=" + "\'" + Uint2Str(userMemberID) + "\'"
        + " AND " + TUSER_DEVCL__DEVCLUSTER_ID + "=" + "\'" + Uint2Str(clusterID) + "\';";

    return sql->Update(statement);

}

// 判断一些设备是否在一个群里面, 任何一个在群里面都返回true
bool UserOperator::isDeviceInCluster(const std::vector<uint64_t> &idlist, uint64_t clusterid)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return true;

    if (idlist.size() == 0)
    {
        return false;
    }

    std::string idlistbuf;
    for (uint32_t i = 0; i < idlist.size(); ++i)
    {
        idlistbuf.append(std::to_string(idlist[i]));
        if (i != (idlist.size() - 1))
        {
            idlistbuf.append(", ");
        }
    }

    char sqlbuf[1024];

    snprintf(sqlbuf, sizeof(sqlbuf),
        "select 1 from T_Device where id in (%s) and clusterid=%lu;",
        idlistbuf.c_str(),
        clusterid);

    int r = sql->Select(sqlbuf);
    // 出错了, 就认为都在群里面
    if (r == -1)
        return true;

    // 返回0表示有结果, 说明其中有设备在群里面
    return r == 0;
}

bool UserOperator::isAllDeviceInCluster(uint64_t clusterID,
                        std::list<uint64_t> devids,
                        std::shared_ptr<MysqlConnection> conn)
{
    if (!conn)
        conn = m_mysqlpool->get();
    if (!conn)
        return false;
    std::vector<DeviceInfo> devs;
    this->getDevMemberInCluster(clusterID, &devs);
    std::set<uint64_t> devidindex;
    std::for_each(devs.begin(), devs.end(), [&devidindex](const DeviceInfo &di){
        devidindex.insert(di.id);
        });
    return std::all_of(devids.begin(), devids.end(), [&devidindex](uint64_t id){
        return devidindex.find(id) != devidindex.end();
        });
}

bool UserOperator::isUserInCluster(uint64_t clusterID,
                    std::list<uint64_t> userids,
                    std::shared_ptr<MysqlConnection> conn)
{
    if (!conn)
        conn = m_mysqlpool->get();
    if (!conn)
        return false;
    std::list<uint64_t> clusterusers;
    this->getUserIDInDeviceCluster(clusterID, &clusterusers);
    std::set<uint64_t> useridx;
    std::for_each(clusterusers.begin(), clusterusers.end(), [&useridx](uint64_t id){
        useridx.insert(id);
        });
    return std::all_of(userids.begin(), userids.end(), [&useridx](uint64_t uid){
        return useridx.find(uid) != useridx.end();
        });
}

int UserOperator::deleteDeviceFromCluster(uint64_t clustid, const std::list<uint64_t> &ids)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;
    if (sql->Update("begin") == -1)
    {
        return -1;
    }
    // set delete flag
    std::string strid;
    bool isfirst = true;
    for (auto it = ids.begin(); it != ids.end(); ++it)
    {
        if (!isfirst)
        {
            strid.append(",");
        }
        isfirst = false;
        strid.append(std::to_string(*it));
    }

    int size = snprintf(nullptr, 0,
                        "update T_Device set status = %u where ID in (%s)",
                        DeviceStatu::Deleted, strid.c_str());
    std::vector<char> buf(size+1, char('\0'));
    snprintf(buf.data(), buf.size(),
            "update T_Device set status = %u where ID in (%s)",
            DeviceStatu::Deleted, strid.c_str());
    if (sql->Update(buf.data()) == -1)
    {
        sql->Update("rollback");
        return -1;
    }
    // 删除所有的授权
    std::string delallauth("delete from T_User_Device where deviceID in ");
    delallauth.append("(");
    delallauth.append(strid);
    delallauth.append(")");
    if (sql->Delete(delallauth) == -1)
    {
        sql->Update("rollback");
        return -1;
    }

    // 删除所有的连接
    std::string delconn("delete from SceneConnector where deviceid in (");
    delconn.append(strid);
    delconn.append(")");
    if (sql->Delete(delconn) == -1)
    {
        sql->Update("rollback");
        return -1;
    }

    if (sql->Update("commit") == -1)
    {
        sql->Update("rollback");
        return -1;
    }
    return 0;

}

// ---------------------------------------

int UserOperator::getMySceneList(uint64_t userid, std::vector<SceneInfo> *r)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;

    //string statement = EMPTY_STRING
    //       + "SELECT DISTINCT "
    //       + TUSER_SCENE + "." + TUSER_SCENE__SCENE_ID + ", "
    //       + TUSER_SCENE + "." + TUSER_SCENE__AUTHORITY + ", "
    //       + TSCENE + "." + TSCENE__NAME
    //       + " FROM " + TUSER_SCENE + " JOIN " + TSCENE
    //       + " ON " + TUSER_SCENE + "." + TUSER_SCENE__SCENE_ID + "=" + TSCENE + "." + TSCENE__ID
    //       + " WHERE " + TUSER_SCENE__USER_ID + "=\'" + Uint2Str(userid) + "\'"
    //       + " AND " + TUSER_SCENE + "." + TUSER_SCENE__SCENE_ID + " IN "
    //       + "(SELECT " + TUSER_SCENE__SCENE_ID
    //       + " FROM " + TUSER_SCENE
    //       + " WHERE " + TUSER_SCENE__USER_ID + "=\'" + Uint2Str(userid) + "\');";

    char statement[1024];
    snprintf(statement, sizeof(statement),
        "select sceneID, authority, notename from T_User_Scene where userID=%lu;",
        userid);
    int result = sql->Select(statement);
    if(result == 0) {
        for(my_ulonglong i = 0; i != sql->Res()->row_count; i++) {
            MYSQL_ROW row = sql->FetchRow(sql->Res());

            SceneInfo sceneInfo;
            Str2Uint(row[0], sceneInfo.id);
            Str2Uint(row[1], sceneInfo.authority);
            sceneInfo.notename = GetString(row[2]);

            r->push_back(sceneInfo);
        }
    }

    return result;
}

int UserOperator::modifySceneInfo(uint64_t userid, uint64_t sceneid,
    const std::string &name,
    const std::string &version,
    const std::string &desc)
{
    // 场景version会在场景上传时自动增加, 所以忽略
    (void)version;
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;

    (void)userid;
    (void)name;

    // TODO FIXME 验证长度
    char buf[1024];
    int len;

    //len = sql->RealEscapeString(buf, (char *)version.c_str(), version.size());
    // assert(len > 0);
    //std::string verescape(buf, len);

    len = sql->RealEscapeString(buf, (char *)desc.c_str(), desc.size());
    // assert(len > 0);
    std::string descescape(buf, len);

    char statementbuf[4096];
    sprintf(statementbuf,
        "update T_Scene set description='%s' where ID=%lu;",
        descescape.c_str(),
        sceneid);

   return sql->Update(std::string(statementbuf));
}

// modify notename
int UserOperator::modifySceneNoteName(uint64_t userid,
        uint64_t sceneid,
        const std::string &notename)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;

    char buf[1024];
    int len = sql->RealEscapeString(buf, (char*)notename.c_str(), notename.size());
    // assert(len > 0);
    std::string notenameescape(buf, len);
    if (len > 255)
    {
        return false;
    }
    // TODO FIXME 验证权限????, 修改备注名不需要权限
    char statementbuf[1024];
    snprintf(statementbuf, sizeof(statementbuf),
        "update T_User_Scene set notename='%s' where userID=%lu and sceneID=%lu;",
        notenameescape.c_str(),
        userid,
        sceneid);

    return sql->Update(std::string(statementbuf));
}

int UserOperator::getSceneInfo(uint64_t userid, uint64_t sceneid, SceneInfo *si)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;

    // .TODO FIXME 能否用一条sql语句完成呢? 这里用了三次查询
    char statement[1024];
    // 取得权限notename
    snprintf(statement, sizeof(statement), 
        "select authority, notename from T_User_Scene where userID=%lu and sceneID=%lu;",
        userid,
        sceneid);
    if (sql->Select(std::string(statement)) != 0)
    {
        return -1;
    }
    si->id = sceneid;
    // ok find the scene
    MYSQL_RES *res = sql->Res();
    assert(res->row_count == 1);
    MYSQL_ROW row = sql->FetchRow(sql->Res());

    Str2Uint(row[0], si->authority);
    si->notename = GetString(row[1]);

    // 取得场景信息
    snprintf(statement, sizeof(statement), 
        "select creatorID, ownerID, name, description, version from T_Scene where ID=%lu;",
        sceneid);
    if (sql->Select(std::string(statement)) != 0)
    {
        return -1;
    }

    res = sql->Res();
    assert(res->row_count == 1);
    row = sql->FetchRow(res);
    Str2Uint(row[0], si->creatorid);
    Str2Uint(row[1], si->ownerid);
    si->name = GetString(row[2]);
    si->description = GetString(row[3]);
    si->version = GetString(row[4]);

    // 最后取得用户名
    this->getUserAccount(si->creatorid, &(si->creatorusername));
    this->getUserAccount(si->ownerid, &(si->ownerusername));

    return 0;
}

int UserOperator::getSceneName(uint64_t sceneid, std::string *name)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;

    char statement[1024];
    snprintf(statement, sizeof(statement),
        "select name from T_Scene where ID=%lu",
        sceneid);
    if (sql->Select(statement) != 0)
    {
        return -1;
    }
    MYSQL_RES *res = sql->Res();
    MYSQL_ROW row = sql->FetchRow(res);
    *name = GetString(row[0]);

    return 0;
}

int UserOperator::getSceneSharedUses(uint64_t userid,
        uint64_t sceneid,
        std::vector<SharedSceneUserInfo> *l)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
        return -1;

    // TODO FIXME 验证权限, userid应该是scene的上传者, 这个在这个函数之前己经做过了.

    char statement[1024];
    MYSQL_RES *res;
    MYSQL_ROW row;
    snprintf(statement, sizeof(statement),
        " select T_User.ID, T_User.account, T_User.head, T_User_Scene.authority, T_User.signature "
        " from T_User_Scene inner join T_User on T_User.ID=T_User_Scene.userID "
        " where T_User_Scene.sceneID=%lu;",
        sceneid);
    if (sql->Select(std::string(statement)) != 0)
    {
        return -1;
    }

    res = sql->Res();
    for (uint64_t i = 0; i < res->row_count; ++i)
    {
        SharedSceneUserInfo ssu;
        row = sql->FetchRow(res);
        Str2Uint(row[0], ssu.userid);
        ssu.username = GetString(row[1]);
        Str2Uint(row[2], ssu.header);
        Str2Uint(row[3], ssu.authority);
        ssu.signature = GetString(row[4]);
        // if (ssu.authority != 1) // 0表示场景所有者
        {
            l->push_back(ssu);
        }
    }

    return 0;
}

// 检查userid对sceneid是否有share auth : userid是场景的ownerid, creatorid
bool UserOperator::hasShareAuth(uint64_t userid, uint64_t sceneid)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
    {
        ::writelog("sql get connection failed.");
        return false;
    }

    uint64_t ownerid;
    this->getSceneOwner(sceneid, &ownerid);
    //char statement[1024];
    //snprintf(statement, sizeof(statement),
    //    "select creatorID, ownerID from T_Scene where ID=%lu;",
    //    sceneid);
    //if (sql->Select(std::string(statement)) != 0)
    //{
    //    return false;
    //}
    //MYSQL_RES *res = sql->Res();
    //assert(res->row_count == 1);
    //MYSQL_ROW row = sql->FetchRow(res);
    //uint64_t ownerid;
    //Str2Uint(row[1], ownerid);

    return ownerid == userid;
}

// 共享场景给users
int UserOperator::shareSceneToUser(uint64_t userid, uint64_t sceneid,
    const std::vector<std::pair<uint64_t, uint8_t> > &targetusers)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
    {
        ::writelog("sql get connection failed.");
        return -1;
    }
    ::writelog("share scene to user.", InfoType);

    if (targetusers.size() == 0)
    {
        return 0;
    }

    std::string scenename;
    if (this->getSceneName(sceneid, &scenename) != 0)
    {
        return -1;
    }

    (void)userid;

    char statement[1024];
    std::string sqlstatement(" insert into T_User_Scene (userID, sceneID, authority, notename) values ");
    for (uint32_t it = 0; it < targetusers.size(); ++it)
    {
        // userid: it->first;
        // auth : it->second
        snprintf(statement, sizeof(statement),
            "(%lu, %lu, %d, '%s')",
            targetusers[it].first, // userid
            sceneid,   //
            targetusers[it].second,// auth
            scenename.c_str());

        sqlstatement.append(statement);
        if (it != targetusers.size() - 1)
        {
            sqlstatement.push_back(',');
        }
    }

    ::writelog(sqlstatement.c_str(), InfoType);

    if (sql->Insert(sqlstatement) != 0)
    {
        return -1;
    }

    return 0;
}

// 取消共享场景
int UserOperator::cancelShareSceneToUser(uint64_t userid, uint64_t sceneid,
    const std::vector<std::pair<uint64_t, uint8_t> > &targetusers)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
    {
        ::writelog("sql get connection failed.");
        return -1;
    }
    if (targetusers.size() == 0)
    {
        return 0;
    }
    if (targetusers.size() > 200)
    {
        return -1;
    }

    char bufstatement[1024];
    snprintf(bufstatement, sizeof(bufstatement),
        "delete from T_User_Scene where sceneID=%lu and userID in ",
        sceneid);
    //(22, 33);
    std::string sqlstatement(bufstatement);
    sqlstatement.push_back('(');
    for (uint32_t it = 0; it < targetusers.size(); ++it)
    {
        // usrid: it->first
        char intbuf[100];
        snprintf(intbuf, sizeof(intbuf),
            "%lu",
            targetusers[it].first);
        sqlstatement.append(intbuf);
        if (it != targetusers.size() - 1)
        {
            sqlstatement.push_back(',');
        }
    }
    sqlstatement.push_back(')');

    if (sql->Delete(sqlstatement) != 0)
    {
        return -1;
    }

    return 0;
}

// 设置共享场景的权限
int UserOperator::setShareSceneAuth(uint64_t userid, uint64_t sceneid,
    uint8_t auth,
    const std::vector<uint64_t> &targetusers)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
    {
        ::writelog("sql get connection failed.");
        return -1;
    }
    if (targetusers.size() == 0)
    {
        return 0;
    }

    if (targetusers.size() > 200)
    {
        return -1;
    }

    char bufstatement[1024];
    snprintf(bufstatement, sizeof(bufstatement),
        "update T_User_Scene set authority=%d where sceneID=%lu and userID in ",
        auth,
        sceneid);
    std::string sqlstatement(bufstatement);
    sqlstatement.push_back('(');
    for (uint32_t it = 0; it < targetusers.size(); ++it)
    {
        // usrid: it->first
        char intbuf[100];
        snprintf(intbuf, sizeof(intbuf),
            "%lu",
            targetusers[it]);
        sqlstatement.append(intbuf);
        if (it != targetusers.size() - 1)
        {
            sqlstatement.push_back(',');
        }
    }
    sqlstatement.push_back(')');

    if (sql->Delete(sqlstatement) != 0)
    {
        return -1;
    }

    return 0;
}

int UserOperator::deleteScene(uint64_t sceneid)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
    {
        ::writelog("sql get connection failed.");
        return -1;
    }

    char sqlbuf[1024];
    snprintf(sqlbuf, sizeof(sqlbuf),
        "delete from T_Scene where ID=%lu",
        sceneid);
    int r = sql->Delete(std::string(sqlbuf));
    assert(r >= 0);
    if (r == -1)
        return -1;

    snprintf(sqlbuf, sizeof(sqlbuf),
        "delete from T_User_Scene where sceneID=%lu",
        sceneid);
    r = sql->Delete(std::string(sqlbuf));
    assert(r >= 0);

    return 0;
}

int UserOperator::freeDisk(uint64_t userid, uint32_t scenesize)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
    {
        ::writelog("sql get connection failed.");
        return -1;
    }

    char sqlbuf[1024];
    snprintf(sqlbuf, sizeof(sqlbuf),
        "update UserDiskSpace set used=used-%u where userid=%lu",
        scenesize, userid);

    return sql->Update(std::string(sqlbuf));
}

int UserOperator::allocDisk(uint64_t targetuserid, uint32_t scenesize)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
    {
        ::writelog("sql get connection failed.");
        return -1;
    }

    char buf[1024];
    snprintf(buf, sizeof(buf),
        "update UserDiskSpace set used=(used + %u) where userid=%lu",
        scenesize,
        targetuserid);

    if (sql->Update(buf) == -1)
        return -1;
    else
        return 0;
}

int UserOperator::incSceneVersion(uint64_t sceneid)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
    {
        ::writelog("sql get connection failed.");
        return -1;
    }
    char buf[1024];
    snprintf(buf, sizeof(buf),
        "update T_Scene set version = version+1 where ID=%lu",
        sceneid);

    if (sql->Update(buf) == -1)
        return -1;
    else
        return 0;
}

int UserOperator::isUserHasAccessRightOfScene(uint64_t sceneid, uint64_t userid,
    bool *hasauth)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
    {
        ::writelog("sql get connection failed.");
        return -1;
    }

    std::string statement = EMPTY_STRING
           + "SELECT 1"
           + " FROM " + TUSER_SCENE
           + " WHERE " + TUSER_SCENE__USER_ID + "=" + "\'" + Uint2Str(userid) + "\'"
           + " AND " + TUSER_SCENE__SCENE_ID + "=" + "\'" + Uint2Str(sceneid) + "\';";

    int result = sql->Select(statement);
    if(result == 0)
        *hasauth = true;
    else
        *hasauth = false;

    return result;
}

int UserOperator::setSceneOwnerID(uint64_t sceneid, uint64_t newowner)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
    {
        ::writelog("sql get connection failed.");
        return -1;
    }
    char sqlbuf[1024];
    snprintf(sqlbuf, sizeof(sqlbuf),
        "update T_Scene set ownerID=%lu where ID=%lu",
        newowner,
        sceneid);

    if (sql->Update(sqlbuf) != -1)
        return 0;
    else
        return -1;
}

int UserOperator::getSceneOwner(uint64_t sceneid, uint64_t *owner)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
    {
        ::writelog("sql get connection failed.");
        return -1;
    }
    char statement[1024];
    snprintf(statement, sizeof(statement),
        "select creatorID, ownerID from T_Scene where ID=%lu;",
        sceneid);
    if (sql->Select(std::string(statement)) != 0)
    {
        return -1;
    }
    MYSQL_RES *res = sql->Res();
    assert(res->row_count == 1);
    MYSQL_ROW row = sql->FetchRow(res);
    uint64_t ownerid;
    Str2Uint(row[1], ownerid);

    *owner = ownerid;
    return 0;
}

int UserOperator::getUserDiskSpace(uint64_t userid,
    uint32_t *allsize,
    uint32_t *usedsize)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
    {
        ::writelog("sql get connection failed.");
        return -1;
    }
    char buf[1024];
    snprintf(buf, sizeof(buf),
        "select diskspace, used from UserDiskSpace where userid=%lu",
        userid);
    // 返回了一个空的结果, 
    int r = sql->Select(buf);
    if (r == -1)
    {
        return -1;
    }
    else if (r == 1)
    {
        char bufinsert[1024];
        snprintf(bufinsert, sizeof(bufinsert),
            "insert into UserDiskSpace (userid) values (%lu)",
            userid);
        sql->Insert(bufinsert);

        // 再取一次
        if (sql->Select(buf) != 0)
            return -1;
    }
    // ok
    MYSQL_RES *res = sql->Res();
    assert(res->row_count == 1);
    MYSQL_ROW row = sql->FetchRow(res);
    Str2Uint(row[0], *allsize);
    Str2Uint(row[1], *usedsize);

    return 0;
}

int UserOperator::getSceneSize(uint64_t sceneid, uint32_t *cursize)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
    {
        ::writelog("sql get connection failed.");
        return -1;
    }
    char buf[1024];
    snprintf(buf, sizeof(buf),
        "select scenesize from T_Scene where ID=%lu",
        sceneid);
    if (sql->Select(buf) == 0)
    {
        MYSQL_RES *res = sql->Res();
        assert(res->row_count == 1);
        MYSQL_ROW row = sql->FetchRow(res);
        Str2Uint(row[0], *cursize);
        return 0;
    }
    else
    {
        return -1;
    }
}

// 取场景上传的服务器ID
int UserOperator::getSceneServerid(uint64_t sceneid, uint32_t *serverid)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
    {
        ::writelog("sql get connection failed.");
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

int UserOperator::getSceneFileList(uint64_t sceneid, std::string *files)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
    {
        ::writelog("sql get connection failed.");
        return -1;
    }
    char buf[1024];
    snprintf(buf, sizeof(buf),
        "select filelist from T_Scene where ID=%lu",
        sceneid);
    if (sql->Select(buf) == 0)
    {
        // ok
        MYSQL_RES *res = sql->Res();
        assert(res->row_count == 1);
        MYSQL_ROW row = sql->FetchRow(res);
        *files = GetString(row[0]);
        return 0;
    }
    else
    {
        return -1;
    }
}

// 更新场景的大小
int UserOperator::updateSceneInfo(uint64_t userid,
    const std::string &scenename,
    uint64_t sceneid,
    const std::string &files,
    uint32_t oldscenesize,
    uint32_t newscenesize)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
    {
        ::writelog("sql get connection failed.");
        return -1;
    }
    int res = sql->Update("begin");
    if (res < 0)
    {
        ::writelog(InfoType, "updateSceneInfo begin failed.");
        return -1;
    }
    char buf[1024];
    snprintf(buf, sizeof(buf),
        "update UserDiskSpace set used=(used + (%u - %u)) where userid=%lu",
        newscenesize,
        oldscenesize,
        sceneid);
    if (sql->Update(buf) == -1)
    {
        ::writelog("updateSceneInfo failed.1");
        sql->Update("rollback");
        return -1;
    }

    char filesbuf[1024];
    sql->RealEscapeString(filesbuf, (char *)files.c_str(), files.size());
    char namebuf[1024];
    sql->RealEscapeString(namebuf, (char *)scenename.c_str(), scenename.size());

    snprintf(buf, sizeof(buf),
        "update T_Scene set scenesize=%u, filelist='%s', name='%s', uploadDate=now() where ID=%lu",
        newscenesize,
        filesbuf,
        namebuf,
        sceneid);
    if (sql->Update(buf) == -1)
    {
        ::writelog("updateSceneInfo failed.2");
        sql->Update("rollback");
        return -1;
    }

    res = sql->Update("commit");
    if (res < 0)
    {
        ::writelog("updatesceneinfo commit failed.");
        sql->Update("rollback");
        return -1;
    }

    return 0;
}

int UserOperator::insertScene(uint64_t userid,
    const std::string &scenename,
    uint32_t allfilesize,
    const std::string &files,
    uint32_t serverid,
    bool shouldAddToMySceneList,
    uint64_t *sceneid)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
    {
        ::writelog("sql get connection failed.");
        return -1;
    }
    char buf[1024];

    if (*sceneid != 0)
    {
        assert(false);
        return -1;
    }
    else
    {
        int res = sql->Update("begin");
        if (res < 0)
        {
            ::writelog("insert scene begin failed.");
            sql->Update("rollback");
            return -1;
        }
        // 插入新的.
        // 更新空间使用信息
        // 需要确认的上传不更新UserDiskSpace
        if (shouldAddToMySceneList)
        {
            snprintf(buf, sizeof(buf),
                "update UserDiskSpace set used=(used + %u) where userid=%lu",
                allfilesize,
                userid);
            if (sql->Update(buf) == -1)
            {
                ::writelog("update UserDiskSpace failed.");
                sql->Update("rollback");
                return -1;
            }
        }

        char namebuf[1024];
        sql->RealEscapeString(namebuf, (char *)scenename.c_str(), scenename.size());
        char filesbuf[1024];
        sql->RealEscapeString(filesbuf, (char *)files.c_str(), files.size());
        // .
        snprintf(buf, sizeof(buf),
            "insert into T_Scene (name, creatorID, ownerID, createDate, scenesize, filelist, fileserverid, uploadDate) "
            "values ('%s', %lu, %lu, now(), %u, '%s', %u, now());",
            namebuf,
            userid,
            userid,
            allfilesize,
            filesbuf,
            serverid);
        if (sql->Insert(buf) == 0)
        {
            *sceneid = sql->getAutoIncID();

            // .没错, 共享结自己
            //std::vector<std::pair<uint64_t, uint8_t>> target;
            //target.push_back(std::make_pair(userid, SceneAuth::ReadWrie));
            if (shouldAddToMySceneList)
            {
                ::writelog("upload share touser");
                //res = this->shareSceneToUser(userid, *sceneid, target);
                char sqlstatement[1024];
                snprintf(sqlstatement,
                        sizeof(sqlstatement),
                        "insert into T_User_Scene (userID, sceneID, authority, notename) "
                        "values (%lu, %lu, %d, '%s')",
                        userid, *sceneid, SceneAuth::READ_WRITE, namebuf);
                res = sql->Insert(sqlstatement);
                if (res < 0)
                {
                    ::writelog("upload share to user failed");
                }
            }
            else
            {
                // 添加到上传记录表中
                char scenelogbuf[1024];
                snprintf(scenelogbuf, sizeof(scenelogbuf),
                    "insert into UploadSceneLog (sceneid, uploadtime) values "
                    "(%lu, now())",
                    *sceneid);
                res = sql->Insert(scenelogbuf);
                if (res < 0)
                {
                    ::writelog("add to scene log error.");
                }
                else
                {
                    ::writelog("add to scene log ok.");
                }
            }

            int r = sql->Update("commit");
            if (r < 0)
            {
                ::writelog("failed commit");
                sql->Update("rollback");
                return -1;
            }
            else
            {
                return 0;
            }
        }
        else
        {
            ::writelog(InfoType, "insert scene failed.%s", scenename.c_str());
            sql->Update("rollback");
            return -1;
        }
    }
}

int UserOperator::removeUploadLog(uint64_t sceneid)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
    {
        ::writelog("sql get connection failed.");
        return -1;
    }
    char query[1024];
    snprintf(query, sizeof(query),
            "delete from UploadSceneLog where sceneid=%lu",
            sceneid);
    if (sql->Update(query) >= 0)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

int UserOperator::saveFeedBack(uint64_t userid, const std::string &text)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
    {
        ::writelog("sql get connection failed.");
        return -1;
    }
    if (text.size() >= 9 * 1024)
    {
        ::writelog("feedback too large.");
        return -1;
    }

    char textbuf[10240];

    int len = sql->RealEscapeString(textbuf, (char *)text.c_str(), text.size());
    char sqlbuf[10240];
    snprintf(sqlbuf, sizeof(sqlbuf),
        "insert into T_Feedback (userID, feedback, submitTime) values (%lu, '%s', now())",
        userid,
        std::string(textbuf, len).c_str());

    return sql->Insert(std::string(sqlbuf));
}

// 更新在线时长和最后登录时间
int UserOperator::updateUserLastLoginTime(uint64_t userid, time_t lastlogintime)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
    {
        ::writelog("sql get connection failed.");
        return -1;
    }
    assert(lastlogintime > 0);
    std::string t = ::timet2String(lastlogintime);
    if (t.size() == 0)
    {
        ::writelog(InfoType, "update user login time timet2string error: %s",
            strerror(errno));
        return -1;
    }
    char query[200];
    snprintf(query, sizeof(query),
        "update T_User set lastLoginTime = '%s' where ID=%lu",
        t.c_str(), userid);
    if (sql->Update(query) < 0)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

int UserOperator::updateUserOnlineTime(uint64_t userid, int onlinetime)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
    {
        ::writelog("sql get connection failed.");
        return -1;
    }
    assert(onlinetime >= 0);
    if (onlinetime <= 0)
        return 0;
    char query[200];
    snprintf(query, sizeof(query), 
        "update T_User set onlinetime=onlinetime+%d where ID=%lu",
        onlinetime, userid);
    if (sql->Update(query) < 0)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

int UserOperator::updateDeviceLastLoginTime(uint64_t deviceid, time_t lastlogintime)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
    {
        ::writelog("sql get connection failed.");
        return -1;
    }
    assert(lastlogintime > 0);
    std::string t = ::timet2String(lastlogintime);
    if (t.size() == 0)
    {
        ::writelog(InfoType, "update device login time timet2string error: %s",
            strerror(errno));
        return -1;
    }
    char query[200];
    snprintf(query, sizeof(query),
        "update T_Device set lastOnlineTime ='%s' where ID=%lu",
        t.c_str(), deviceid);
    if (sql->Update(query) < 0)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

int UserOperator::updateDeviceOnlineTime(uint64_t deviceid, int onlinetime)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
    {
        ::writelog("sql get connection failed.");
        return -1;
    }
    assert(onlinetime > 0);
    if (onlinetime <= 0)
        return 0;
    char query[200];
    snprintf(query, sizeof(query), 
        "update T_Device set onlinetime=onlinetime+%d where ID=%lu",
        onlinetime, deviceid);
    if (sql->Update(query) < 0)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

