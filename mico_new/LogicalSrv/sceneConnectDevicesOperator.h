#ifndef SCENECONNECTDEVICESOPERATOR__H
#define SCENECONNECTDEVICESOPERATOR__H

#include "isceneConnectDevicesOperator.h"

class UserOperator;
class SceneConnectDevicesOperator : public ISceneConnectDevicesOperator
{
public:
    SceneConnectDevicesOperator(UserOperator *uop);
    //int setSceneConnectDevices(uint64_t, uint64_t, const std::vector< std::pair<uint8_t, uint64_t> > &) override;
    int setSceneConnectDevices(uint64_t, uint64_t, const std::map<uint16_t, std::vector<uint64_t> > &) override;
    int getSceneConnectDevices(uint64_t sceneid, uint64_t *ownerid, std::map<uint16_t, std::vector<uint64_t> > *) override;
    int findInvalidDevice(uint64_t userid) override;
    int removeConnectorOfDevice(const std::vector<uint64_t> &devids) override;

private:
    int deleteConnector(uint64_t sceneid, const std::vector<uint16_t> &connshoulddel);
    int deleteDeviceOfscene(uint64_t sceneid, const std::map<uint16_t, std::vector<uint64_t> > &deviceshoulddel);
    int insertNewDevice(uint64_t sceneid, const std::vector< std::pair<uint16_t, uint64_t> > &deviceshouldinsert);

    UserOperator *m_useroperator;
};

class SceneOperatorAnalysis
{
public:
    static void analysis(const std::map<uint16_t, std::vector<uint64_t> > &prev,
            const std::map<uint16_t, std::vector<uint64_t> > &news,
            std::vector<uint16_t> *connshoulddel,
            std::map<uint16_t, std::vector<uint64_t> > *deviceshoulddel,
            std::vector< std::pair<uint16_t, uint64_t> > *deviceshouldinsert);

    static void test();
};
#endif
