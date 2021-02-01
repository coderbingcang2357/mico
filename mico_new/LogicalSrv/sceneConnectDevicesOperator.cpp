#include <string>
#include <algorithm>
#include <functional>
#include "protocoldef/protocol.h"
#include "mysqlcli/mysqlconnection.h"
#include "mysqlcli/mysqlconnpool.h"
#include "util/logwriter.h"
#include "sceneConnectDevicesOperator.h"
#include "useroperator.h"

SceneConnectDevicesOperator::SceneConnectDevicesOperator(UserOperator *uop)
    : m_useroperator(uop)
{

}

int SceneConnectDevicesOperator::setSceneConnectDevices(
    uint64_t userid,
    uint64_t sceneid,
    const std::map<uint16_t, std::vector<uint64_t> > & conns)
    //const std::vector< std::pair<uint8_t, uint64_t> > & conns)
{
    // check is user the owner of scene ?
    uint64_t ownerid;
    m_useroperator->getSceneOwner(sceneid, &ownerid);
    if (ownerid != userid)
        return ANSC::AUTH_ERROR;

    std::map<uint16_t, std::vector<uint64_t> > prevconns;
    int res;
    if ((res = getSceneConnectDevices(sceneid, &ownerid, &prevconns)) != 0)
        return res;

    // find removed connection and new connection
    std::vector<uint16_t> connshoulddel;
    std::map<uint16_t, std::vector<uint64_t> > deviceshoulddel;
    std::vector< std::pair<uint16_t, uint64_t> > deviceshouldinsert;
    //std::vector<>
    SceneOperatorAnalysis::analysis(prevconns, conns,
        &connshoulddel,
        &deviceshoulddel,
        &deviceshouldinsert);

    res = deleteConnector(sceneid, connshoulddel);
    if (res != 0)
    {
        return res;
    }

    res = deleteDeviceOfscene(sceneid, deviceshoulddel);

    if (res != 0)
    {
        return res;
    }
		
    res = insertNewDevice(sceneid, deviceshouldinsert);

    return res;
}

int SceneConnectDevicesOperator::getSceneConnectDevices(
    uint64_t sceneid,
    uint64_t *ownerid,
    std::map<uint16_t, std::vector<uint64_t> > *conns)
{
    std::unique_ptr<MysqlConnection, std::function<void(MysqlConnection *)> >
        sql(MysqlConnPool::getConnection(), [](MysqlConnection *p) {
                MysqlConnPool::release(p);
            });
    if(!sql)
    {
        return ANSC::FAILED;//return -1; ANSC::AuthErr
    }

    char sqlbuf[1024];
    snprintf(sqlbuf, sizeof(sqlbuf),
        "select connectorid, deviceid from SceneConnector where sceneid=%lu",
        sceneid);

    int res = sql->Select(sqlbuf);
    if (res == -1)
        return ANSC::FAILED;

    std::shared_ptr<MyResult> result = sql->result();
    for (int i = 0; i < result->rowCount(); ++i)
    {
         std::shared_ptr<MyRow> row = result->nextRow();
         (*conns)[uint16_t(row->getUint32(0))].push_back(row->getUint64(1));
         //conns->push_back(std::make_pair(uint8_t(row->getUint32(0)),
         //               row->getUint64(1)));
    }

    *ownerid = 0;
    return 0;
}

int SceneConnectDevicesOperator::findInvalidDevice(
    uint64_t userid)
{
    std::unique_ptr<MysqlConnection, std::function<void(MysqlConnection *)> >
        sql(MysqlConnPool::getConnection(), [](MysqlConnection *p) {
                MysqlConnPool::release(p);
            });
    if(!sql)
    {
        return ANSC::FAILED;//return -1; ANSC::AuthErr
    }

    // 取用户所拥用的场景 
    char sqlbuf[1024];
    snprintf(sqlbuf, sizeof(sqlbuf), 
        "select ID from T_Scene where ownerID = %lu",
        userid);

    int res = sql->Select(sqlbuf);
    if (res != 0 || res < 0)
    {
        // 如果用户一个场景都没有则直接返回
        return 0;
    }
    std::shared_ptr<MyResult> rows = sql->result();
    std::vector<uint64_t> sceneidlist;
    for (int i = 0; i < rows->rowCount(); ++i)
    {
        std::shared_ptr<MyRow> row = rows->nextRow();
        sceneidlist.push_back(row->getUint64(0));
    }


    // 取用户的设备
    std::vector<DeviceInfo> devlist;
    if (m_useroperator->getMyDeviceList(userid, &devlist) != 0)
    {
        ::writelog(InfoType, "find invalid device get mydevice failed:%lu", userid);
        return 0;
    }

    std::string sceneidstr;
    bool isfirst = true;
    for (auto sid : sceneidlist)
    {
        if (isfirst)
        {
            sceneidstr.append(std::to_string(sid));
            isfirst = false;
        }
        else
        {
            sceneidstr.append(", ");
            sceneidstr.append(std::to_string(sid));
        }
    }

    std::string devidstr;
    isfirst = true;
    for (auto devid : devlist)
    {
        if (isfirst)
        {
            devidstr.append(std::to_string(devid.id));
            isfirst = false;
        }
        else
        {
            devidstr.append(", ");
            devidstr.append(std::to_string(devid.id));
        }
    }

    std::string deletesql;
    if (devlist.size() == 0)
    {
        // 如果用户一个设备都没有了, 那么它所有的连接都不能用了
        int size = snprintf(nullptr, 0,
            "delete from SceneConnector where sceneid in (%s)",
            sceneidstr.c_str());
        std::vector<char> sqltmp(size + 1, 0); // + 1 for the last '\0'
        snprintf(&sqltmp[0], sqltmp.size(),
            "delete from SceneConnector where sceneid in (%s)",
            sceneidstr.c_str());
        deletesql = std::string(&sqltmp[0], sqltmp.size());
    }
    else
    {
        // delete from SceneConnector where sceneid in (2) and deviceid not in (device);
        int size = snprintf(nullptr, 0,
            "delete from SceneConnector where sceneid in (%s) and deviceid not in (%s)",
            sceneidstr.c_str(),
            devidstr.c_str());
        std::vector<char> sqltmp(size + 1, 0); // + 1 for the last '\0'
        snprintf(&sqltmp[0], sqltmp.size(),
            "delete from SceneConnector where sceneid in (%s) and deviceid not in (%s)",
            sceneidstr.c_str(),
            devidstr.c_str());
        deletesql = std::string(&sqltmp[0], sqltmp.size());
    }

    sql->Delete(deletesql);

    return 0;
}

int SceneConnectDevicesOperator::removeConnectorOfDevice(const std::vector<uint64_t> &devids)
{
    std::unique_ptr<MysqlConnection, std::function<void(MysqlConnection *)> >
        sqlconn(MysqlConnPool::getConnection(), [](MysqlConnection *p) {
                MysqlConnPool::release(p);
            });
    if(!sqlconn)
    {
        return ANSC::FAILED;//return -1; ANSC::AuthErr
    }

    std::string devid;
    bool isfirst = true;

    for (auto did : devids)
    {
        if (isfirst)
        {
            devid.append(std::to_string(did));
            isfirst = false;
        }
        else
        {
            devid.append(", ");
            devid.append(std::to_string(did));
        }
    }

    std::string sql("delete from SceneConnector where deviceid in (");
    sql.append(devid);
    sql.append(")");

    //char sqlbuf[10240];
    //int size = snprintf(nullptr, 0,
    //    "delete from SceneConnector where deviceid in (%s)", 
    //    devid.c_str());

    sqlconn->Delete(sql);

    return 0;
}

int SceneConnectDevicesOperator::deleteConnector(
    uint64_t sceneid,
    const std::vector<uint16_t> &connshoulddel)
{
    if (connshoulddel.size() == 0)
        return 0;

    std::unique_ptr<MysqlConnection, std::function<void(MysqlConnection *)> >
        sqlconn(MysqlConnPool::getConnection(), [](MysqlConnection *p) {
                MysqlConnPool::release(p);
            });
    if(!sqlconn)
    {
        return ANSC::FAILED;//return -1; ANSC::AuthErr
    }

    std::string connectorids;
    for (uint32_t i = 0; i < connshoulddel.size(); ++i)
    {
        connectorids.append(std::to_string(connshoulddel[i]));
        if (i != connshoulddel.size() - 1)
            connectorids.append(",");
    }

    char sqlbuf[10240];
    snprintf(sqlbuf, sizeof(sqlbuf),
        "delete from SceneConnector where sceneid=%lu and connectorid in (%s)",
        sceneid, connectorids.c_str());

    if (sqlconn->Delete(sqlbuf) == 0)
        return 0;
    else
        return ANSC::FAILED;
}

int SceneConnectDevicesOperator::deleteDeviceOfscene(
    uint64_t sceneid,
    const std::map<uint16_t, std::vector<uint64_t> > &deviceshoulddel)
{
    if (deviceshoulddel.size() == 0)
        return 0;

    std::unique_ptr<MysqlConnection, std::function<void(MysqlConnection *)> >
        sqlconn(MysqlConnPool::getConnection(), [](MysqlConnection *p) {
                MysqlConnPool::release(p);
            });
    if(!sqlconn)
    {
        return ANSC::FAILED;//return -1; ANSC::AuthErr
    }

    std::string devtodel; // (connectorid = 2 and deviceid=3) or (connectorid=3 and deviceid=4)

    bool isfirst = true;
    for (auto connid : deviceshoulddel)
    {
        char tmpbuf[1024];
        for (auto devid: connid.second)
        {
            snprintf(tmpbuf, sizeof(tmpbuf), 
                " (connectorid=%d and deviceid=%lu) ",
                connid.first,
                devid);

            if (isfirst)
            {
                devtodel.append(tmpbuf);
                isfirst = false;
            }
            else
            {
                devtodel.append("or");
                devtodel.append(tmpbuf);
            }
        }
    }

    char sqlbuf[10240];
    snprintf(sqlbuf, sizeof(sqlbuf),
        "delete from SceneConnector where sceneid=%lu and (%s)",
        sceneid,
        devtodel.c_str());

    if (sqlconn->Delete(sqlbuf) == 0)
        return 0;
    else
        return ANSC::FAILED;
}

int SceneConnectDevicesOperator::insertNewDevice(
    uint64_t sceneid,
    const std::vector< std::pair<uint16_t, uint64_t> > &deviceshouldinsert)
{
    if (deviceshouldinsert.size() == 0)
        return 0;

    std::unique_ptr<MysqlConnection, std::function<void(MysqlConnection *)> >
        sqlconn(MysqlConnPool::getConnection(), [](MysqlConnection *p) {
                MysqlConnPool::release(p);
            });
    if(!sqlconn)
    {
        return ANSC::FAILED;//return -1; ANSC::AuthErr
    }

    std::string items;

    bool isfirst = true;
    char itembuf[1024];
    for (auto item : deviceshouldinsert)
    {
        snprintf(itembuf, sizeof(itembuf),
            " (%lu, %d, %lu) ",
            sceneid,
            item.first,
            item.second);

        if (isfirst)
        {
            items.append(itembuf);
            isfirst = false;
        }
        else
        {
            items.append(", ");
            items.append(itembuf);
        }
    }

    char sqlbuf[10240];
    snprintf(sqlbuf, sizeof(sqlbuf),
        "insert into SceneConnector (sceneid, connectorid, deviceid) values %s",
        items.c_str());

    if (sqlconn->Insert(sqlbuf) == 0)
        return 0;
    else
        return ANSC::FAILED;
}

void SceneOperatorAnalysis::analysis(
        const std::map<uint16_t, std::vector<uint64_t> > &prev,
        const std::map<uint16_t, std::vector<uint64_t> > &news,
        std::vector<uint16_t> *connshoulddel,
        std::map<uint16_t, std::vector<uint64_t> > *deviceshoulddel,
        std::vector< std::pair<uint16_t, uint64_t> > *deviceshouldinsert)
{
    //getConnShouldDel(prev, news, connshoulddel);
    for (auto it = prev.begin(); it != prev.end(); ++it)
    {
        // conn can not find in news
        uint16_t connectorid = it->first;
        if (news.find(connectorid) == news.end())
        {
            connshoulddel->push_back(it->first);
        }
        else
        {
            const std::vector<uint64_t> &newdev = news.at(connectorid);
            const std::vector<uint64_t> &prevdev = prev.at(connectorid);
            // find device should del
            // 在以前的列表里面有, 在新的列表里面没有则是要删除的
            for (auto v : prevdev)
            {
                if (std::find(newdev.begin(), newdev.end(), v) == newdev.end())
                {
                    // can not find in newdevs, it's should be del
                    (*deviceshoulddel)[connectorid].push_back(v);
                }
            }
            // 在 新的列表时面 有在 以前的列表里面没有 则是要添加的
            for (auto v : newdev)
            {
                // can not find in prevdevs, They should be added
                if (std::find(prevdev.begin(), prevdev.end(), v) == prevdev.end())
                {
                    deviceshouldinsert->push_back(std::make_pair(connectorid, v));
                }
            }
        }
    }
    // find other device should insert, that is the new connector
    for (auto con : news)
    {
        // the connector not in prev is the new connector
        uint16_t connectorid = con.first;
        if (prev.find(connectorid) == prev.end())
        {
            for (auto v : con.second)
            {
                deviceshouldinsert->push_back(std::make_pair(connectorid, v));
            }
        }
    }
}

void SceneOperatorAnalysis::test()
{
    std::map<uint16_t, std::vector<uint64_t> > prev;
    std::map<uint16_t, std::vector<uint64_t> > news;
    std::vector<uint16_t> connshoulddel;
    std::map<uint16_t, std::vector<uint64_t> > deviceshoulddel;
    std::vector< std::pair<uint16_t, uint64_t> > deviceshouldinsert;

//.
    prev[1].push_back(23);
    prev[1].push_back(24);
    prev[2].push_back(25);
    prev[2].push_back(26);
    prev[3].push_back(27);
    prev[3].push_back(28);

// new
    news[1].push_back(23);
                            // 24 should del
    news[1].push_back(34); // new

    // connector 2 should del

    news[3].push_back(27); // 
                            // 28 should del

    news[4].push_back(27); // new
    news[4].push_back(28); // new

    SceneOperatorAnalysis::analysis(prev, news,
        &connshoulddel,
        &deviceshoulddel,
        &deviceshouldinsert);

    assert(connshoulddel.size() == 1 && connshoulddel[0] == 2);

    //del
    auto it1 = deviceshoulddel.find(1);
    auto it3 = deviceshoulddel.find(3);
    assert(it1 != deviceshoulddel.end());
    assert(it3 != deviceshoulddel.end());
    assert(it1->second.size() == 1);
    assert(it1->second.at(0) == 24);

    assert(it3->second.size() == 1);
    assert(it3->second.at(0) == 28);

    // insert
    auto it4 =  std::find_if(deviceshouldinsert.begin(),
                            deviceshouldinsert.end(),
                            [](const std::pair<uint16_t, uint64_t> &l) {
        return l.first == 1 && l.second == 34;
    });
    assert(it4 != deviceshouldinsert.end());

    auto it5 =  std::find_if(deviceshouldinsert.begin(),
                            deviceshouldinsert.end(),
                            [](const std::pair<uint16_t, uint64_t> &l) {
        return l.first == 4 && l.second == 27;
    });
    assert(it5 != deviceshouldinsert.end());

    auto it6 =  std::find_if(deviceshouldinsert.begin(),
                            deviceshouldinsert.end(),
                            [](const std::pair<uint16_t, uint64_t> &l) {
        return l.first == 4 && l.second == 28;
    });
    assert(it6 != deviceshouldinsert.end());

    ::writelog("testok.");
}

