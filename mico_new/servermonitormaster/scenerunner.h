#pragma once
#include <QObject>
#include <QThread>
#include <QEventLoop>
#include "dbaccess/runningscenedata.h"
#include "hmiproject/hmiproject.h"

class SceneRun : public QObject
{
    Q_OBJECT

public:
    static SceneRun *runScene(RunningSceneData d);
public:
    SceneRun(RunningSceneData d);
    ~SceneRun();
    //void init();

private:
    void quit();

    QThread *m_th = nullptr;
    RunningSceneData m_data;
    HMI::HmiProject *m_prj;
};
