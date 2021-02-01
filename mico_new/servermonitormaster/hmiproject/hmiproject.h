#ifndef HMIPROJECT_H
#define HMIPROJECT_H
#include <QObject>
#include "alarm.h"
#include "connection.h"
#include "variable.h"
#include "serverudp/serverudp.h"
#include "hmicom/commInterface.h"
#include "../../util/logwriter.h"

class ProjectRunner;
namespace HMI {

class HmiProject
{
public:
    HmiProject();
	~HmiProject();

    bool fromByte(char *buf, int len);
    void run();
//private:
    quint64 sceneid = 0;
    QString phone;
    QList<Alarm> alarm;
    QList<Connection> connection;
    QList<Variable> var;
	//std::shared_ptr<ServerUdp> m_udp;
	ServerUdp *m_udp;
    HMICommService m_hmiservice;
    std::shared_ptr<ProjectRunner> m_alamrunner;
};

}
#endif // HMIPROJECT_H
