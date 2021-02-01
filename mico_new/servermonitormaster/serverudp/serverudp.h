#pragma once
#include <QObject>
#include <QByteArray>
#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QTimer>
#include <QThread>
#include "isock.h"

class ServerUdp : public ISock
{
    Q_OBJECT
    const static int StateBegin = 0;
    const static int StateRandomNumberSend = 1;
    //const static int StateBegin = 2;
    const static int StateOK = 3;
public:
    ServerUdp(uint64_t deviceid,
              uint64_t uerid,
              std::string extserverip,
              QObject *p = nullptr);
    bool bind();

    //int writeDatagram(const QByteArray &ba);
    virtual quint64 writedata(const QByteArray &d);
    void disconnect();

private slots:
    void readdata();
    void readPendingDatagrams();
    void sendHeartBeatSlot();
    void poll();

private:
    void openChannel();
    void getkey(const std::vector<char> &sessionkey);
    void processTheDatagram(const QNetworkDatagram &d);
    void startSendHeartBeat();

    int state = StateBegin;
    uint64_t deviceid;
    uint64_t userid;
    uint64_t randomnumber;
    std::string extserverip;
    std::vector<char>  m_key;
    QHostAddress m_deviceaddr;
    uint16_t  m_devport = 0;

    int sn = 1;
	int retry = 0;
    QUdpSocket *m_udp;
    QTimer *m_heartbeattimer;
    QTimer *m_heartbeattimeout;
    QTimer *m_polltimer;

    friend void test();
};
