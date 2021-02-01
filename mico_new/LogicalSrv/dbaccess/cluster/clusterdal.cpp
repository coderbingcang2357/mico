#include <functional>
#include "util/logwriter.h"
#include "./clusterdal.h"
#include "mysqlcli/mysqlconnpool.h"
#include "mysqlcli/mysqlconnection.h"
#include "../../dbcolumn.h"
#include "util/util.h"

ClusterDal::ClusterDal() : m_maxuser(500), m_maxDevice(500)
{
}

int ClusterDal::getDeviceClusterIDByAccount(const std::string &clusteracc,
        uint64_t *clusterid)
{
    std::unique_ptr<MysqlConnection, std::function<void(MysqlConnection *)> >
        sql(MysqlConnPool::getConnection(),
        [](MysqlConnection *p){
            MysqlConnPool::release(p);
        });
    if(!sql)
        return -1;

    // clusteracc要进行转义, 不然不安全
    char accountbuf[1024];
    int len = sql->RealEscapeString(accountbuf, (char *)clusteracc.c_str(), clusteracc.size());
    // 根据群名(account)找群id, FIX ME
    string statement = EMPTY_STRING
           + "SELECT " + TDEVCL__ID
           + " FROM " + TDEVCL
           + " WHERE "
           + TDEVCL__ACCOUNT + "=\'" + std::string(accountbuf, len) + "\';";

    int result = sql->Select(statement);
    if(result < 0)
        return result;

    uint64_t deviceClusterID = 0;
    if(result == 0) {
        MYSQL_ROW row = sql->FetchRow(sql->Res());
        Str2Uint(row[0], deviceClusterID);
        *clusterid = deviceClusterID;
        return 0;
    }
    else
    {
        return -1;
    }
}


int ClusterDal::insertDevice(
            const std::string &name,
            uint64_t firmClusterID,
            uint64_t macAddr,
            uint8_t encryptionMethod,
            uint8_t keyGenerationMethod,
            const std::string &strLoginKey,
            const std::string &deviceClusterAccount,
            uint8_t type,
            uint8_t transferMethod,
            uint64_t previousID,
            uint64_t *deviceid)
{
    std::unique_ptr<MysqlConnection, std::function<void(MysqlConnection *)> >
        sql(MysqlConnPool::getConnection(),
        [](MysqlConnection *p){
            MysqlConnPool::release(p);
        });
    if(!sql)
        return -1;

    uint64_t clusterid;
    int result = this->getDeviceClusterIDByAccount(deviceClusterAccount,
                            &clusterid);
    if(result < 0)
    {
        // 如果群名不存在, 则设为0?????失败返回
        return result;
    }
    
    // 检查是否超过限制
    // 1表示是否可以再添加一个设备
    if (!this->canAddDevice(clusterid, 1))
    {
        ::writelog(InfoType,
            "add device to cluster, max limit: %s",
            deviceClusterAccount.c_str());
        return -1;
    }

    assert(strLoginKey.size() == 16);
    // byte转成hex
    std::string hexloginkey = ::byteArrayToHex(
                        (const uint8_t *) strLoginKey.c_str(),
                        strLoginKey.length());

    std::string statement = EMPTY_STRING
              + "INSERT INTO " + TDEVICE
              + "("
              // + TDEVICE__ID + ", "
              + TDEVICE__NAME + ", "
              + TDEVICE__FIRM_CLUSTER_ID + ", "
              + TDEVICE__MAC_ADDR + ", "
              + TDEVICE__CLUSTER_ID + ", "
              + TDEVICE__TYPE + ", "
              + TDEVICE__TRANSFER_METHOD + ", "
              + TDEVICE__NEW_PROTOCOL_FLAG + ", "
              + TDEVICE__ENCRYPTION_METHOD+ ", "
              + TDEVICE__KEY_GENERATION_METHOD+ ", "
              + TDEVICE__LOGIN_KEY + ", "
              + TDEVICE__CREATE_DATE + ", "
              + TDEVICE__PREVIOUS_ID
              + ")"
              + " VALUES ("
              //+ "\'" + Uint2Str(deviceID) + "\', "
              + "\'" + name + "\', "
              + "\'" + Uint2Str(firmClusterID) + "\', "
              + "\'" + Uint2Str(macAddr) + "\', "
              + "\'" + Uint2Str(clusterid) + "\', "
              + "\'" + Uint2Str(type) + "\', "
              + "\'" + Uint2Str(transferMethod) + "\', "
              + "\'" + "1" + "\', "
              + "\'" + Uint2Str(encryptionMethod) + "\', "
              + "\'" + Uint2Str(keyGenerationMethod) + "\', "
              + "\'" + hexloginkey + "\', "
              + "NOW(), "
              + Uint2Str(previousID) // previous ID
              + ");";

    int res = sql->Insert(statement);
    if (res < 0)
    {
        return res;
    }
    // 取得新插入的ID
    *deviceid = sql->getAutoIncID();

    // 前一个ID不为0, 则把前一个ID标记为删除
    if (previousID != 0)
    {
        char sqlstatement[255];
        sprintf(sqlstatement, "update `%s` set `%s` = %d where `%s`=%lu",
                TDEVICE, 
                TDEVICE__DELETE_FLAG,
                1, // 1表示删除
                TDEVICE__ID,
                previousID);

        res = sql->Update(std::string(sqlstatement));
        if (res < 0)
        {
            char log[255];
            sprintf(log, "set delete flag failed: %lu", previousID);
            ::writelog(log);
        }
    }
    return res;
}

int ClusterDal::maxUserCount()
{
    return m_maxuser;
}

int ClusterDal::maxDeviceCount()
{
    return m_maxDevice;
}

int ClusterDal::userCount(uint64_t cluid)
{
    std::unique_ptr<MysqlConnection, std::function<void(MysqlConnection *)> >
        sql(MysqlConnPool::getConnection(), [](MysqlConnection *p) {
                MysqlConnPool::release(p);
                });
    if(!sql)
        return -1;
    std::string query("select count(*) from T_User_DevCluster where deviceClusterID=");
    query.append(std::to_string(cluid));
    int r = sql->Select(query);
    if (r == 0)
    {
        MYSQL_ROW row = sql->FetchRow(sql->Res());
        return atoi(row[0]);
    }
    else
    {
        return -1;
    }
}

int ClusterDal::deviceCount(uint64_t cluid)
{
    std::unique_ptr<MysqlConnection, std::function<void(MysqlConnection *)> >
        sql(MysqlConnPool::getConnection(), [](MysqlConnection *p) {
                MysqlConnPool::release(p);
                });
    if(!sql)
        return -1;
    std::string query("select count(*) from T_Device where clusterid=");
    query.append(std::to_string(cluid));
    int r = sql->Select(query);
    if (r == 0)
    {
        MYSQL_ROW row = sql->FetchRow(sql->Res());
        return atoi(row[0]);
    }
    else
    {
        return -1;
    }
}

bool ClusterDal::canAddUser(uint64_t cluid)
{
    int usercnt = this->userCount(cluid);
    int maxuser = this->maxUserCount();
    return (usercnt >= 0 && maxuser >= 0 && usercnt < maxuser);
}

bool ClusterDal::canAddDevice(uint64_t cluid, int count)
{
    int devcnt = this->deviceCount(cluid);
    int maxdev = this->maxDeviceCount();
    return devcnt >= 0 && maxdev >= 0 && (devcnt + count) <= maxdev;
}

