#include <assert.h>
#include <memory>
#include "deviceregister.h"
#include "util/util.h"
#include "Crypt/MD5.h"
#include "Crypt/tea.h"
#include "protocoldef/protocol.h"
#include "util/logwriter.h"
#include "useroperator.h"
#include "sessionkeygen.h"
#include "Message/Message.h"
#include "transferdevicetoanothercluster.h"
#include "dbaccess/cluster/clusterdal.h"

RegisterData::RegisterData()
    : deviceID(0), loginkey(32, char(0))
{
}

bool RegisterData::decode(const char *data, int len)
{
    const char* pos = data;

    encryptionMethod = ReadUint8(pos);
    keyGenerationMethod = ReadUint8(pos += sizeof(encryptionMethod));
    firmClusterID = ReadUint64(pos += sizeof(keyGenerationMethod));
    pos += sizeof(firmClusterID);

#ifndef TEST
    uint16_t ciperLen = len - (pos - data);
    MD5 md5;
    char md5ClusterID[16] = {0};
    md5.input((const char*) &firmClusterID, sizeof(firmClusterID));
    md5.output(md5ClusterID);
    // 解密用的密钥, md5群号与factor运算
    const char* const registerKeyFactor = "FEDCBA9876543210"; // ??? 这个factor做什么的?
    //char registerKey[16] = {0};
    for(size_t i = 0; i != 16; i++)
        registerKey[i] = md5ClusterID[i] ^ registerKeyFactor[i];

    char plainBuf[MAX_MSG_SIZE];
    if (tea_dec(const_cast<char *>(pos),
                ciperLen,
                plainBuf,
                registerKey, 16) < 0)
    {
        //
        ::writelog("register decrypt failed.");
        return false;
    }

    pos = plainBuf;
#endif

    macAddr = ReadMacAddr(pos); // mac
    type = ReadUint8(pos += 6); //
    transferMethod = ReadUint8(pos += sizeof(type));
    clusterAccountLen = ReadUint8(pos += sizeof(transferMethod));
    deviceNameLen = ReadUint8(pos += sizeof(clusterAccountLen));
    clusterAccount = std::string(pos += sizeof(deviceNameLen), clusterAccountLen); // 群帐号
    deviceName = std::string(pos += clusterAccountLen, deviceNameLen); // 设备名

    return true;
}


//----------------------------------------------------------------------------
//
DeviceRegister::DeviceRegister(UserOperator *uop)
    : m_deviceoperator(uop)
{
      m_transfer= new TransferDeviceToAnotherCluster(m_deviceoperator);
}

int DeviceRegister::processMesage(Message *reqmsg, std::list<InternalMessage *> *r)
{
    if (reqmsg->commandID() == CMD::DCMD__REGISTER)
    {
        char *p = reqmsg->content();
        int len = reqmsg->contentLen();
        RegisterData registerdata;
        RegisterResult regresult;
        bool res = registerdata.decode(p, len);
        if (res)
        {
            regresult = this->registerDevice(registerdata);
        }
        else
        {
            regresult.answccode = ANSC::FAILED;
            regresult.deviceID = 0;
            ::writelog(InfoType, "Register device decode failed");
            // failed, errro package
        }
        // output regresult
        Message *msg = new Message;
        msg->CopyHeader(reqmsg);
        msg->setObjectID(0);
        msg->setDestID(0);
        std::vector<char> enckey(registerdata.registerKey,
                    registerdata.registerKey + sizeof(registerdata.registerKey));
        EncryptByteArray encb(&regresult, &enckey);
        msg->appendContent(&encb);
        InternalMessage *imsg = new InternalMessage(msg);
        imsg->setEncrypt(false);
        //OutputMsg(imsg, ToOutside);
        r->push_back(imsg);
        r->splice(r->end(), *(regresult.msgs.get()));
    }
    else
    {
        assert(false);
    }
    return r->size();
}

RegisterResult DeviceRegister::registerDevice(const RegisterData &registerData)
{
    RegisterResult regresult;
    ClusterDal cludal;
    uint64_t deviceid;
    std::string loginkey;
     uint64_t previousID = registerData.deviceID;
    // 通过 厂商id和mac addr查找deviceid 和 loginkey
    int result = m_deviceoperator->getDeviceIDAndLoginKey(
                            registerData.firmClusterID,
                            registerData.macAddr,
                            &deviceid,
                            &loginkey);
    // 设备在数据库中不存在
    if (result > 0)
    {
        // 生成一个loginKey
        char buffkey[16];
        genLoginkey(buffkey, 16);
        loginkey = std::string(buffkey, 16);

        //MD5 md5;
        //char szLoginKey[16 * 2 + 1] = {0}; 
        //// byte转成hex
        //md5.bytesToString((const unsigned char*) loginKey, szLoginKey);
        //strloginkey = std::string(szLoginKey);

        //
        if(cludal.insertDevice(
                    registerData.deviceName,
                    registerData.firmClusterID,
                    registerData.macAddr,
                    registerData.encryptionMethod,
                    registerData.keyGenerationMethod,
                    //registerData.loginkey,
                    loginkey,
                    registerData.clusterAccount,
                    registerData.type,
                    registerData.transferMethod,
                    previousID,
                    &deviceid) < 0)
        {
            regresult.answccode = ANSC::FAILED;
            char log[255];
            sprintf(log, "register device failed: %lx, previousid: %lu",
                    registerData.macAddr, previousID);
            ::writelog(log);
            regresult.deviceID = 0;
        }
        else
        {
            assert(deviceid > 0);
            regresult.deviceID = deviceid;
            regresult.answccode = ANSC::SUCCESS;
            regresult.loginkey = loginkey;
        }
        return regresult;
    }
    // 已经存在, 直接取其id返回
    else if (result == 0)// error
    {
        uint64_t destclusterid;
        int result = cludal.getDeviceClusterIDByAccount(
                                registerData.clusterAccount,
                                &destclusterid);
        if (result != 0)
        {
            // error 群号不存在
            regresult.answccode = ANSC::FAILED;
            regresult.deviceID = 0;
            return regresult;
        }
        // else
        uint32_t res = m_transfer->deviceClusterChanged(deviceid,
                                         destclusterid,
                                         registerData.clusterAccount,
                                         regresult.msgs.get());
        // ok
        if(uint8_t(res) == ANSC::SUCCESS)
        {
            regresult.answccode = ANSC::SUCCESS;
            regresult.deviceID = deviceid;
            regresult.loginkey = loginkey;
            return regresult;
        }
        // failed
        else
        {
            regresult.answccode = ANSC::FAILED;
            regresult.deviceID = 0;
            return regresult;
        }
    }
    else // (result < 0)
    {
        regresult.deviceID = 0;
        regresult.answccode = ANSC::FAILED;
        regresult.loginkey = loginkey;
        return regresult;
    }

//end:
//    char loginKey[16];
//    MD5 md5;
//    md5.stringTobytes(registerData.loginkey.c_str(),
//            (unsigned char*) loginKey);
//    // 发回deviceid, loginkey
//    Message* responseMsg = new Message(Message::MT_WithDevice);
//    responseMsg->CopyHeader(m_requestMsg);
//    responseMsg->AppendContent(&registerResult, Message::SIZE_ANSWER_CODE); // 1字节
//    responseMsg->AppendContent(&registerData.deviceID, sizeof(registerData.deviceID)); // 把deviceid发回去
//    responseMsg->AppendContent(loginKey, 16);
//
//#ifndef TEST
//    responseMsg->Encrypt(registerData.registerKey); // ...这个通过群号生成的, 设备可以通过群号生成此key
//#endif
//
//    responseMsg->SetPrefix(DEVICE_MSG_PREFIX);
//    responseMsg->SetSuffix(DEVICE_MSG_SUFFIX);
//    OutputMsg(responseMsg);
}


void RegisterResult::toByteArray(std::vector<char> *out)
{
    // answc code 1字节, 设备id, 8字节, loginkey 16字节
    out->push_back(this->answccode);
    if (this->answccode != ANSC::SUCCESS)
        return;
    char *ptoid = (char *)&deviceID;
    out->insert(out->end(), ptoid, ptoid + sizeof(deviceID));
    assert(this->loginkey.size() == 16);
    const char *p = this->loginkey.c_str();
    out->insert(out->end(), p, p + this->loginkey.size());
}
