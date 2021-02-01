#include <iostream>
#include <stdio.h>
#include <string.h>
#include "devicedata.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/writer.h"
#include "rapidjson/document.h"
#include "util/util.h"
#include "arpa/inet.h"
#include "util/logwriter.h"

using namespace rapidjson;

DeviceData::DeviceData(uint64_t devid)
    : //clusterID(0),
     m_devid(devid), 
    m_versionNumber(0),
    m_sessionKey(16, char(0)),
    m_newsessionKey(16, char(0)),
    m_lastSerialNumber(0)
{
    m_keyupdateStatus = Cache::KUS_Updated;

    memset(&m_address, 0, sizeof(m_address));
    m_logintime = time(0);
    m_lastKeyUpdateTime = time(0);
    m_lastHeartBeatTime = time(0);
}

DeviceData::~DeviceData()
{
}

uint64_t DeviceData::id()
{
    return this->deviceID();
}

int DeviceData::connectionServerId()
{
    return m_connectionServerId;
}

uint64_t DeviceData::connectionId()
{
    return m_connectionId;
}

std::string DeviceData::toStringJson()
{
    StringBuffer sb;
    Writer<StringBuffer> writer(sb);

    writer.StartObject();

    writer.String("devid");
    writer.Uint64(m_devid);

    // ip address:port
    writer.String("address");
    std::string straddr = m_address.toString();
    if (straddr.empty())
    {
        straddr = std::string("unknown");
    }
    writer.String(straddr.c_str(), straddr.size());

    // ip address bin
    writer.String("address_bin");
    std::string addrbin = ::byteArrayToHex((uint8_t *)(&m_address),
                                            sizeof(m_address));
    writer.String(addrbin.c_str(), addrbin.size());

    writer.String("logintime");
    writer.Uint64(m_logintime);
    // 
    writer.String("versionNumber");
    writer.Uint(m_versionNumber);

    writer.String("lastHeartBeatTime");
    writer.Uint64(m_lastHeartBeatTime);

    writer.String("sessionKey"); // 16 bytes
    std::string sesskey = ::byteArrayToHex((uint8_t *)(&m_sessionKey[0]),
                                        m_sessionKey.size());
    writer.String(sesskey.c_str(), sesskey.size());

    writer.String("newsessionKey");
    std::string newsesskey = ::byteArrayToHex((uint8_t *)(&m_newsessionKey[0]),
                                        m_newsessionKey.size());
    writer.String(newsesskey.c_str(), newsesskey.size());

    writer.String("lastSerialNumber");
    writer.Uint(m_lastSerialNumber);

    writer.String("lastKeyUpdateTime");
    writer.Uint64(m_lastKeyUpdateTime);

    writer.String("keyupdateStatus"); // updateing updated needupdate
    writer.Uint(m_keyupdateStatus);

    writer.String("macAddr"); // 6 bytes
    std::string macstr = ::byteArrayToHex((uint8_t *)(&m_macAddr[0]),
                                    m_macAddr.size());
    writer.String(macstr.c_str(), macstr.size());

    writer.String("name");          // 32 bytes???
    writer.String(m_name.c_str(), m_name.size());

    writer.String("loginserverid");
    writer.Uint(m_loginserverID);

    writer.String("connectionServerId");
    writer.Uint(m_connectionServerId);

    writer.String("connectionId");
    writer.Uint64(m_connectionId);

    writer.EndObject();

    return std::string(sb.GetString());
}

bool DeviceData::fromStringJson(const std::string &strjson)
{
    Document doc;
    doc.Parse(strjson.c_str());
    assert(doc.IsObject());

    if (doc.HasMember("devid"))
    {
        m_devid = doc["devid"].GetUint64();
    }
    else
    {
        assert(false);
    }

    if (doc.HasMember("address_bin"))
    {
        std::string tmp = doc["address_bin"].GetString();
        std::vector<char> out;
        bool r = ::hexToBytesArray(tmp, &out);
        assert(r); (void)r;
        //assert(sizeof(m_address) == out.size());
        memcpy(&m_address, &(out[0]), out.size());
    }
    else
    {
        assert(false);
    }

    if (doc.HasMember("versionNumber"))
    {
        m_versionNumber = doc["versionNumber"].GetUint();
    }
    else
    {
        assert(false);
    }

    if (doc.HasMember("logintime"))
    {
        m_logintime = doc["logintime"].GetUint64();
    }

    if (doc.HasMember("lastHeartBeatTime"))
    {
        m_lastHeartBeatTime = doc["lastHeartBeatTime"].GetUint64();
    }
    else
    {
        assert(false);
    }

    if (doc.HasMember("sessionKey"))
    {
        //std::string tmp = doc["sessionKey"].GetString();
        const char *p = doc["sessionKey"].GetString();
        int len = doc["sessionKey"].GetStringLength();
        std::string tmp(p, len);
        m_sessionKey.clear();
        int r = ::hexToBytesArray(tmp, &m_sessionKey);
        assert(r);(void)r;
        assert(m_sessionKey.size() == 16);
    }
    else
    {
        assert(false);
    }

    if (doc.HasMember("newsessionKey"))
    {
        std::string tmp = doc["newsessionKey"].GetString();
        m_newsessionKey.clear();
        int r = ::hexToBytesArray(tmp, &m_newsessionKey);
        assert(r);(void)r;
        assert(m_newsessionKey.size() == 16);
    }
    else
    {
        assert(false);
    }

    if (doc.HasMember("lastSerialNumber"))
    {
        m_lastSerialNumber = doc["lastSerialNumber"].GetUint();
    }
    else
    {
        assert(false);
    }

    if (doc.HasMember("lastKeyUpdateTime"))
    {
        m_lastKeyUpdateTime = doc["lastKeyUpdateTime"].GetUint64();
    }
    else
    {
        assert(false);
    }

    if (doc.HasMember("keyupdateStatus"))
    {
        m_keyupdateStatus = doc["keyupdateStatus"].GetUint();
    }
    else
    {
        assert(false);
    }

    if (doc.HasMember("macAddr"))
    {
        std::string tmp = doc["macAddr"].GetString();
        int r = ::hexToBytesArray(tmp, &m_macAddr);
        assert(r);(void)r;
        assert(m_macAddr.size() == 6);
    }
    else
    {
        assert(false);
    }

    if (doc.HasMember("name"))
    {
        const char *p = doc["name"].GetString();
        int len = doc["name"].GetStringLength();
        m_name = std::string(p, len);
    }
    else
    {
        assert(false);
    }

    if (doc.HasMember("loginserverid"))
    {
        m_loginserverID = doc["loginserverid"].GetInt();
    }

    if (doc.HasMember("connectionServerId"))
    {
        m_connectionServerId = doc["connectionServerId"].GetUint();
    }

    if (doc.HasMember("connectionId"))
    {
        m_connectionId = doc["connectionId"].GetUint64();
    }

    return true;
}

bool DeviceData::toByteArray(char *buf, int *len)
{
    if (uint32_t(*len) < sizeof(DeviceDataByteArray))
        return false;

    DeviceDataByteArray &d = *((DeviceDataByteArray *)buf);

    d.classVersion = 1;

    d.devid = this->m_devid;
    d.address = this->m_address;
    d.versionNumber = this->m_versionNumber;
    d.logintime = this->m_logintime;
    d.lastHeartBeatTime = this->m_lastHeartBeatTime;

    memcpy(d.sessionKey, &(m_sessionKey[0]), m_sessionKey.size());
    memcpy(d.newsessionKey, &(m_newsessionKey[0]), m_newsessionKey.size());
    d.lastSerialNumber = this->m_lastSerialNumber;
    d.lastKeyUpdateTime = this->m_lastKeyUpdateTime;

    d.keyupdateStatus = this->m_keyupdateStatus;

    std::string hexmac = byteArrayToHex((uint8_t *)&m_macAddr[0], m_macAddr.size());
    assert(hexmac.size() == 6 * 2); // mac地址是6字节, 转成hex后是12字节
    memcpy(d.macAddrhex, hexmac.c_str(), 12);
    strncopyn(d.name, sizeof(d.name), m_name.c_str(), m_name.size());
    d.loginserverID = this->m_loginserverID;
    *len = sizeof(DeviceDataByteArray);
    return true;
}

bool DeviceData::fromByteArray(char *buf, int *len)
{
    if (uint32_t(*len) < sizeof(DeviceDataByteArray)){
		::writelog("DeviceData len less than DeviceDataByteArray!");
        return false;
	}

    DeviceDataByteArray &d = *((DeviceDataByteArray *)buf);
    if (d.classVersion != 1){
		::writelog("DeviceData classVersion not equal 1!");
        return false;
	}

    this->m_devid = d.devid;
    this->m_address = d.address;
    this->m_versionNumber = d.versionNumber;
    this->m_logintime = d.logintime;
    this->m_lastHeartBeatTime = d.lastHeartBeatTime;

    this->m_sessionKey.clear();
    this->m_sessionKey.insert(m_sessionKey.begin(),
                d.sessionKey, d.sessionKey + sizeof(d.sessionKey));
    this->m_newsessionKey.clear();
    this->m_newsessionKey.insert(m_newsessionKey.begin(),
                d.newsessionKey,
                d.newsessionKey + sizeof(d.newsessionKey));
    this->m_lastSerialNumber = d.lastSerialNumber;
    this->m_lastKeyUpdateTime = d.lastKeyUpdateTime;

    this->m_keyupdateStatus = d.keyupdateStatus;

    this->m_macAddr.clear();
    hexToBytesArray(std::string(d.macAddrhex, sizeof(d.macAddrhex)), &m_macAddr);
//    m_macAddr.insert(m_macAddr.begin(),
//            d.macAddr,
//            d.macAddr + sizeof(d.macAddr));
    this->m_name = d.name;
    //this->m_noteName = d.noteName;

    //this->m_iconNumber = d.iconNumber;
    //this->m_loginserveripport = d.loginserverIpport;
    m_loginserverID = d.loginserverID;

    *len = sizeof(DeviceDataByteArray);
    return true;
}

void DeviceData::setConnectionId(const uint64_t &connectionId)
{
    m_connectionId = connectionId;
}

void DeviceData::setConnectionServerId(int connectionServerId)
{
    m_connectionServerId = connectionServerId;
}

