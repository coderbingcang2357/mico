#ifndef PROJECTRUNNER_H
#define PROJECTRUNNER_H
#include <memory>
#include <QObject>
#include <QMap>
#include <QTimer>
#include "hmiproject.h"
#include "tmpstruct.h"
#include "rabbitmqservice.h"

class ProjectRunner : public QObject
{
    Q_OBJECT
public:
    ProjectRunner(HMI::HmiProject* _prj);

    bool init();
    bool start();
private slots:
    void doalarm();

private:
    int readValue(int id, char *buf, int sizeofbuf);
    int getComMsg(QList<ComMsg> &c);
    void readExtTag( int id );
    TagDataType tagDataType(quint32 id);
    int read_plc_var(int id, void *buf, int len);
    void updateTagValue( int id, const QVariant &newValue, bool isCon );
    bool setHMITagValue( HMI::Variable *v, const QVariant &value, bool isUpdateFromPLC );
    void alarmDisposal( HMI::Variable *v, const QVariant &oldVarValue );
    QVariant tagValue( int tagId );
	// 打包通知客户端的报警消息
	void alarmNotifyPublish(quint64 id,quint16 alarmId,QString alarmText,quint8 alarmEventType);
	void alarmPublish(quint64 id,quint16 alarmId,QString alarmText, quint8 alarmEventType,int alarmtype);

    HMI::HmiProject* prj;
    QMap<int, HMI::Variable> idvar;
    QMap<int, QList<std::shared_ptr<HMI::Alarm>>> idalarm;
    QMap<int, QList<std::shared_ptr<HMI::Alarm>>> varidAnalogAlarm; // 模拟量报警处理
    QMap<int, QList<std::shared_ptr<HMI::Alarm>>> variddiscreteAlarm; // 离散量报警处理
    QMap<int, HMI::Connection> idconn;
    QTimer *m_alarmtimer = nullptr;
};

#endif // PROJECTRUNNER_H
