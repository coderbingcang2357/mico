#pragma once
#include <QObject>

class ISock : public QObject
{
    Q_OBJECT
public:
    static ISock *get(int connid);
    static void set(int connid, ISock *);
    static void del();
    ISock();
    virtual ~ISock();
    virtual quint64 writedata(const QByteArray &d) = 0;

signals:
    void readyRead(const QByteArray &d);
};
