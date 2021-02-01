#include "scenerunner.h"
#include "util/logwriter.h"

SceneRun *SceneRun::runScene(RunningSceneData d)
{
    //QThread *th = new QThread();
    SceneRun *sr = new SceneRun(d);
	sr->m_th = new QThread();
    //connect(th, &QThread::started, [sr](){
    connect(sr->m_th, &QThread::started, [sr](){
        // create project and load data
        // project  p;p
        HMI::HmiProject *prj = new HMI::HmiProject;
        sr->m_prj = prj;
        const char *p = sr->m_data.blobscenedata.c_str();
        int len = sr->m_data.blobscenedata.size();
        if (!prj->fromByte(const_cast<char*>(p), len))
        {
            ::writelog(InfoType, "parse scene data error.");
        }
        prj->sceneid = sr->m_data.sceneid;
		prj->phone = QString::fromStdString(sr->m_data.wechartphone);
        prj->run();
        // p.load(sr);
        // p.start(); // craete comm ....connect signals ...
        //
    });
    connect(sr->m_th, &QThread::finished, [sr](){
    //connect(th, &QThread::finished, [sr](){
		::writelog(InfoType,"delete hmiproject");
        delete sr->m_prj;
    });

    //QThread *t = QThread::create([&sr](){
    //    QEventLoop e;
    //    sr->m_el =&e;
    //    sr->init();
    //    e.exec();
    //    delete sr;
    //});
    //t->start();
    //sr->moveToThread(t);
    //sr->m_th = t;

	sr->m_th->start();
    //th->start();
    //sr->m_th = th;
    return sr;
}

SceneRun::SceneRun(RunningSceneData d)
    : m_data(d)
{

}

SceneRun::~SceneRun()
{
    ::writelog("delscenr run");
    this->quit();
    delete m_th;
    ::writelog("after delscenr run");
}

void SceneRun::quit()
{
	::writelog("SceneRun quit");
    m_th->quit();
    m_th->wait(1000);
}
