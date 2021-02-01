#ifndef REGISTERDEVICE__H
#define REGISTERDEVICE__H

#include <stdint.h>
#include <string>
#include <memory>
#include "Message/itobytearray.h"
#include "imessageprocessor.h"

class Message;
class UserOperator;
class TransferDeviceToAnotherCluster;

class RegisterData
{
public:
    RegisterData();
    bool decode(const char *p, int len);

    uint64_t deviceID;
    uint8_t encryptionMethod;
    uint8_t keyGenerationMethod;
    uint64_t firmClusterID;
    uint64_t macAddr; // mac
    uint8_t type; //
    uint8_t transferMethod;
    uint8_t clusterAccountLen;
    uint8_t deviceNameLen;
    std::string clusterAccount; // 群帐号
    std::string deviceName; // 设备名
    std::string loginkey; // 登录密码的hex
    char registerKey[16];
};

class RegisterResult : public IToByteArray
{
public:
    RegisterResult()
        : answccode(0), deviceID(0), loginkey(16, 'a'),
          msgs(std::make_shared<std::list<InternalMessage*>>())
    {}

    void toByteArray(std::vector<char> *out);

    uint8_t answccode;
    uint64_t deviceID;
    std::string loginkey;
    std::shared_ptr<std::list<InternalMessage *>> msgs;
};

class DeviceRegister : public IMessageProcessor
{
public:
    DeviceRegister(UserOperator *uop);

    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    RegisterResult registerDevice(const RegisterData &rdata);
    UserOperator *m_deviceoperator;
    TransferDeviceToAnotherCluster *m_transfer;
};

#endif
