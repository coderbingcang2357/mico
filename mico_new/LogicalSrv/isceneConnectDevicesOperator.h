#ifndef ISCENECONNECTDEVICESOPERATOR_H
#define ISCENECONNECTDEVICESOPERATOR_H

#include <stdint.h>
#include <vector>
#include <map>

class ISceneConnectDevicesOperator
{
public:
    virtual ~ISceneConnectDevicesOperator(){}
    virtual int setSceneConnectDevices(uint64_t, uint64_t, const std::map<uint16_t, std::vector<uint64_t> > &) = 0;
    virtual int getSceneConnectDevices(uint64_t sceneid, uint64_t *ownerid, std::map<uint16_t, std::vector<uint64_t> > *) = 0;
    // the map is: std::map<sceneid, vector< pair<connectorid, deviceid>>>
    virtual int findInvalidDevice(uint64_t userid) = 0;
    virtual int removeConnectorOfDevice(const std::vector<uint64_t> &devids) = 0;
};
#endif
