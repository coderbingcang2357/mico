#include "heartbeattimer.h"
#include "userdata.h"
#include <iostream>
#include "rapidjson/rapidjson.h"
#include "rapidjson/writer.h"
#include "rapidjson/document.h"
#include "util/util.h"
#include "arpa/inet.h"

using namespace rapidjson;

UserData::UserData() : lastSerialNumber(0),
    m_versionNumber(0),
    m_sessionKey(16, 0),
    m_logintime(time(0)),
    m_lastHeartBeatTime(time(0)),
    m_clientVersion(0)
{
}

UserData::~UserData()
{
}

uint64_t UserData::id()
{
    return userid;
}

std::vector<char> UserData::sessionKey()
{
    return m_sessionKey;
}

void UserData::setSessionKey(const std::vector<char> &sess)
{
    m_sessionKey = sess;
}

//std::string UserData::loginServerIpAndPort()
//{
//    return m_loginserveripport;
//}

time_t UserData::lastHeartBeatTime()
{
    return m_lastHeartBeatTime;
}

time_t UserData::loginTime()
{
    return m_logintime;
}

void UserData::setLoginTime(time_t t)
{
    m_logintime = t;
}

void UserData::setLastHeartBeatTime(time_t t)
{
    m_lastHeartBeatTime = t;
}

int UserData::connectionServerId()
{
    return m_connectionServerId;
}

uint64_t UserData::connectionId()
{
    return m_connectionId;
}

void UserData::setConnectionId(const uint64_t &connectionId)
{
    m_connectionId = connectionId;
}

void UserData::setConnectionServerId(int connectionServerId)
{
    m_connectionServerId = connectionServerId;
}

std::string UserData::toStringJson()
{
    std::string str;

    StringBuffer sb;
    Writer<StringBuffer> writer(sb);

    writer.StartObject();

    writer.String("userid");
    writer.Uint64(userid);

    writer.String("lastSerialNumber");
    writer.Int(this->lastSerialNumber);

    writer.String("account"); // name
    writer.String(account.c_str(), account.size());

//    writer.String("nickName");
//    writer.String(nickName.c_str(), nickName.size());

//    writer.String("signature");
//    writer.String(signature.c_str(), signature.size());

//    writer.String("headNumber");
//    writer.Int(headNumber);

//    writer.String("mail");
//    writer.String(mail.c_str(), mail.size());

//    writer.String("secureMail");
//    writer.String(secureMail.c_str(), secureMail.size());

//    writer.String("phone");
//    writer.String(phone.c_str(), phone.size());

//    writer.String("sex");
//    writer.Int(sex);

//    writer.String("birthday");
//    writer.Uint(birthday);

//    writer.String("region");
//    writer.Uint(region);

    writer.String("sessionKey");
    std::string sesstr(::byteArrayToHex((uint8_t *)&m_sessionKey[0], m_sessionKey.size()));
    writer.String(sesstr.c_str(), sesstr.size());

    writer.String("versionNumber");
    writer.Uint(m_versionNumber);

    // 字符串ip地址
    writer.String("sockaddr");
    std::string straddr = m_sockaddr.toString();
    if (straddr.empty())
    {
        straddr = std::string("unknown");
    }
    writer.String(straddr.c_str(), straddr.size());

    // 二进制ip地址
    writer.String("sockaddr_bin");
    std::string binaddr = ::byteArrayToHex((uint8_t *)&m_sockaddr, sizeof(m_sockaddr));
    writer.String(binaddr.c_str(), binaddr.size());

//    writer.String("onlineStatus");
//    writer.Uint(m_onlineStatus);

    writer.String("logintime");
    writer.Uint64(m_logintime);

    writer.String("lastHeartBeatTime");
    writer.String(timet2String(m_lastHeartBeatTime).c_str());

    writer.String("lastHeartBeatTime_bin");
    writer.Uint64(m_lastHeartBeatTime);


    writer.String("loginserverid");
    writer.Uint(m_loginserverid);

    writer.String("clientversion");
    writer.Uint(m_clientVersion);

    writer.String("connectionServerId");
    writer.Uint(m_connectionServerId);

    writer.String("connectionId");
    writer.Uint(m_connectionId);

    writer.EndObject();

    str = std::string(sb.GetString());

    return str;
}

bool UserData::fromStringJson(const std::string &strjson)
{
    Document doc;
    doc.Parse(strjson.c_str());
    assert(doc.IsObject());

    if (doc.HasMember("userid"))
    {
        this->userid = doc["userid"].GetUint64();
    }
    else
    {
        assert(false);
    }

    if (doc.HasMember("lastSerialNumber"))
    {
        this->lastSerialNumber = doc["lastSerialNumber"].GetInt();
    }
    else
    {
        assert(false);
    }

    if (doc.HasMember("account"))
    {
        const Value &v = doc["account"];
        this->account = std::string(v.GetString(), v.GetStringLength());
    }
    else
    {
        assert(false);
    }

    if (doc.HasMember("sessionKey"))
    {
        const char *str = doc["sessionKey"].GetString();
        int len = doc["sessionKey"].GetStringLength();
        m_sessionKey.clear();
        bool r = ::hexToBytesArray(std::string(str, len), &m_sessionKey);
        assert(r);(void)r;
    }
    else
    {
        assert(false);
    }

    if (doc.HasMember("versionNumber"))
    {
        this->m_versionNumber = doc["versionNumber"].GetUint();
    }
    else
    {
        assert(false);
    }

    if (doc.HasMember("sockaddr_bin"))
    {
        const char *p = doc["sockaddr_bin"].GetString();
        int len = doc["sockaddr_bin"].GetStringLength();
        std::vector<char> addr;
        bool r = ::hexToBytesArray(std::string(p, len), &addr);
        assert(r);(void)r;
        memcpy(&this->m_sockaddr, &(addr[0]), addr.size());
    }
    else
    {
        
    }

//    if (doc.HasMember("onlineStatus"))
//    {
//        this->m_onlineStatus = doc["onlineStatus"].GetUint();
//    }
//    else
//    {
//        assert(false);
//    }
    if (doc.HasMember("logintime"))
    {
        this->m_logintime = doc["logintime"].GetUint64();
    }

    if (doc.HasMember("lastHeartBeatTime_bin"))
    {
        this->m_lastHeartBeatTime = doc["lastHeartBeatTime_bin"].GetUint64();
    }
    else
    {
        assert(false);
    }

    if (doc.HasMember("loginserverid"))
    {
        m_loginserverid = doc["loginserverid"].GetInt();
    }
    if (doc.HasMember("clientversion"))
    {
        m_clientVersion = doc["clientversion"].GetUint();
    }

    if (doc.HasMember("connectionServerId"))
    {
        m_connectionServerId = doc["connectionServerId"].GetInt();
    }

    if (doc.HasMember("connectionId"))
    {
        m_connectionId = doc["connectionId"].GetUint64();
    }

    return true;
}


bool UserData::toByteArray(char *buf, int *len)
{
    if (uint32_t(*len) < sizeof(UserDataByteArray))
        return false;

    UserDataByteArray &ba = *((UserDataByteArray *) buf);
    ba.classVersion = 1;
    ba.userid = this->userid;
    ba.lastSerialNumber = this->lastSerialNumber;
    strncopyn(ba.account, sizeof(ba.account),
                account.c_str(), account.length());

    memcpy(ba.sessionKey, &(m_sessionKey[0]), m_sessionKey.size());
    ba.versionNumber = m_versionNumber;
    ba.sockaddr = m_sockaddr;

    ba.logintime = m_logintime;
    ba.lastHeartBeatTime = m_lastHeartBeatTime;

    ba.loginserverid = m_loginserverid;
    ba.clientVersion = m_clientVersion;

    *len = sizeof(UserDataByteArray);

    return true;
}

bool UserData::fromByteArray(char *buf, int *len)
{
    if (uint32_t(*len) < sizeof(UserDataByteArray))
        return false;

    UserDataByteArray &ba = *((UserDataByteArray *) buf);
    if (ba.classVersion != 1)
        return false;

    this->userid = ba.userid;
    this->lastSerialNumber = ba.lastSerialNumber;
    this->account = ba.account;

//    this->nickName = ba.nickName;
//    this->signature = ba.signature;
//    this->headNumber = ba.headNumber;
//    this->mail = ba.mail;

//    this->secureMail = ba.secureMail;
//    this->phone = ba.phone;
//    this->sex = ba.sex;
//    this->birthday = ba.birthday;

//    this->region = ba.region;
    m_sessionKey.clear();
    this->m_sessionKey.insert(m_sessionKey.begin(), 
        ba.sessionKey,
        ba.sessionKey + sizeof(ba.sessionKey));
    this->m_versionNumber = ba.versionNumber;
    this->m_sockaddr = ba.sockaddr;

    //this->m_onlineStatus = ba.onlineStatus;
    this->m_logintime = ba.logintime;
    this->m_lastHeartBeatTime = ba.lastHeartBeatTime;
    //this->m_loginserveripport = ba.loginserverIpport;
    this->m_loginserverid = ba.loginserverid;
    this->m_clientVersion = ba.clientVersion;

    return true;
}

