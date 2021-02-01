#ifndef RUNSCENEATSERVICEPROCESS_H
#define RUNSCENEATSERVICEPROCESS_H

#include "onlineinfo/icache.h"
#include "imessageprocessor.h"
#include "useroperator.h"
#include "timer/clocktimer.h"
#include "../servermonitormaster/serverscenerunnermasterclient.h"
#include "servermonitormaster/dbaccess/runningscenedb.h"

class Timer;
class ISceneDal;
class IDelSceneUploadLog;
class MicoServer;

class IsRunSceneAtServiceProcess : public IMessageProcessor
{
public:
    IsRunSceneAtServiceProcess(ICache *, SceneRunnerManager *s, RunningSceneDbaccess *rs);
    int processMesage(Message *, std::list<InternalMessage *> *r);

    static void test();

private:
    ICache *m_cache;
    SceneRunnerManager *srm;
    RunningSceneDbaccess *m_rsdbaccess;
};

class StartRunSceneAtServiceProcess : public IMessageProcessor
{
public:
    StartRunSceneAtServiceProcess(ICache *,SceneRunnerManager *s, RunningSceneDbaccess *rs, UserOperator *u);
	int processMesage(Message *, std::list<InternalMessage *> *r);
	void generateNotify(uint64_t sceneid,
			uint16_t notifytype,
			std::list<InternalMessage *> *r);

	static void test();
private:
    ICache *m_cache;
    SceneRunnerManager *srm;
    RunningSceneDbaccess *m_rsdbaccess;
	UserOperator *m_useroperator;
};

class StopRunSceneAtServiceProcess : public IMessageProcessor
{
public:
    StopRunSceneAtServiceProcess(ICache *,SceneRunnerManager *s, RunningSceneDbaccess *rs, UserOperator *u);
    int processMesage(Message *, std::list<InternalMessage *> *r);
	void generateNotify(uint64_t sceneid,
			uint16_t notifytype,
			std::list<InternalMessage *> *r);

    static void test();

private:
    ICache *m_cache;
    SceneRunnerManager *srm;
    RunningSceneDbaccess *m_rsdbaccess;
	UserOperator *m_useroperator;
};

#endif
