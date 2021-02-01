#ifndef CONNECTION_H
#define CONNECTION_H
#include <QList>
#include <QObject>
#include "util2/datadecode.h"
#include "hmicom/commInterface.h"
#include <memory>

namespace HMI {


class Connection
{
public:
    Connection();
    ~Connection();
    static bool readConnList(char **buf, int *len, QList<Connection> *connection);
    static ProtocolPars *getPar(quint8 dt);
	static void SetConnid(Connection &cn, std::shared_ptr<ProtocolPars> pp);
    quint32 connid = 0;
    quint64 deviceId = 0;
    quint8 driverType = 0;
    //ProtocolPars *protocolPars = nullptr;
    std::shared_ptr<ProtocolPars> protocolPars;// = nullptr;
};

}
#endif // CONNECTION_H
