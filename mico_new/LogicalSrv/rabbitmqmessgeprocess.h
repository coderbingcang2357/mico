#pragma once
#include "imessageprocessor.h"
#include "mysqlcli/mysqlconnection.h"
#include "mysqlcli/mysqlconnpool.h"
#include "servermonitormaster/dbaccess/runningscenedb.h"
#include <string>
#include <vector>

class RabbitmqMessageProcess : public IMessageProcessor
{
public:
    RabbitmqMessageProcess();
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
};

#if 1
class RabbitmqMessageAlarmNotifyProcess : public IMessageProcessor
{
public:
    RabbitmqMessageAlarmNotifyProcess(IMysqlConnPool *pool);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
	IMysqlConnPool *m_mysqlpool;
};
#endif
