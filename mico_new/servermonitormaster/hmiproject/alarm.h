#ifndef ALARM_H
#define ALARM_H
#include <QList>
#include <QObject>
#include "../../util/logwriter.h"
namespace HMI {

class Alarm
{
public:
    Alarm();
    static bool readAlarmList(char **buf, int *len, QList<Alarm> *alarm);

    //quint16 alarmid = 0;
    quint16 alarmid = 0;
    QString alarmText ;
    quint8 alarmType = 0;
    quint8 alarmMode = 0;
    quint32 alarmCycle = 0;
    quint32 trigVarId = 0;
    quint8 trigBit = 0;
    quint8 limitFlag = 0;
    quint8 trigMode = 0;
    qreal limitValue = 0;
};
}

#endif // ALARM_H
