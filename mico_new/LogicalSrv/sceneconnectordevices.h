#ifndef SCENECONNECTORDEVICES_H
#define SCENECONNECTORDEVICES_H
#include "imessageprocessor.h"
#include "servermonitormaster/dbaccess/runningscenedata.h"
#include "servermonitormaster/dbaccess/runningscenedb.h"

class Message;
class InternalMessage;
class ICache;
class ISceneConnectDevicesOperator;

class SceneConnectorDeviceMessageProcessor : public IMessageProcessor
{
public:
    SceneConnectorDeviceMessageProcessor(ICache *, ISceneConnectDevicesOperator * ,RunningSceneDbaccess *);
    ~SceneConnectorDeviceMessageProcessor();
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    int setConnectDevices(Message *, std::list<InternalMessage *> *r);
    int getConnectDevices(Message *, std::list<InternalMessage *> *r);

    ICache *m_cache;
    ISceneConnectDevicesOperator *m_sceneoperator;
    RunningSceneDbaccess *m_rsdbaccess;
};
#endif
