#include "alarm.h"
#include "util2/datadecode.h"
#include <QDebug>

HMI::Alarm::Alarm()
{

}

#if 1
bool HMI::Alarm::readAlarmList(char **buf, int *len, QList<HMI::Alarm> *alarm)
{
    quint16 alarmCount = 0;
    if (!Util2::readUint16(buf, len, &alarmCount))
    {
        return false;
    }
    for (int i = 0; i < (int)alarmCount; ++i)
    {
        Alarm al;
        if (!Util2::readUint16(buf, len, &al.alarmid)
        //if (!Util2::readUint16(buf, len, &al.alarmid)
            ||!Util2::readString_end0(buf, len, &al.alarmText)
            ||!Util2::readUint8(buf, len, &al.alarmType)
            ||!Util2::readUint8(buf, len, &al.alarmMode)
            ||!Util2::readUint32(buf, len, &al.alarmCycle)
            ||!Util2::readUint32(buf, len, &al.trigVarId)
            ||!Util2::readUint8(buf, len, &al.trigBit)
            ||!Util2::readUint8(buf, len, &al.limitFlag)
            ||!Util2::readUint8(buf, len, &al.trigMode)
            ||!Util2::readDouble(buf, len, &al.limitValue)
		   )
		   {
			   //printf("false11111111111 %s\n",al.alarmText.toStdString().c_str());
			   ::writelog(InfoType, "readAlarmList failed!");
			   return false;
		   }
		   //qDebug()<<"alarmtext:"<<al.alarmText;
		   //printf("true11111111111 %s\n",al.alarmText.toStdString().c_str());
		   alarm->push_back(al);
	}

	return true;
}
#endif

