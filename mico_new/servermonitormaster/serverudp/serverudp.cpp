#include "serverudp.h"
#include "RelaySrv/sessionRandomNumber.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rabbitmqservice.h"
#include "LogicalSrv/onlineinfo/devicedata.h"
#include "LogicalSrv/onlineinfo/rediscache.h"
#include "Message/Message.h"
#include "util/logwriter.h"
#include "util/util.h"
#include "Crypt/MD5.h"
#include "protocoldef/protocol.h"
//#include <unistd.h>

ServerUdp::ServerUdp(uint64_t deviceid, uint64_t uerid, std::string extserverip, QObject *p)
{
    m_udp = new QUdpSocket(this);
    connect(m_udp, &QUdpSocket::readyRead,
                  this, &ServerUdp::readPendingDatagrams);
    this->deviceid = deviceid;
    this->userid = uerid;
    this->extserverip = extserverip;
	//::writelog(InfoType,"deviceid:%llu",deviceid);
    m_heartbeattimer = new QTimer(this);
    m_heartbeattimer->setInterval(16 * 1000);
    connect(m_heartbeattimer, &QTimer::timeout,
            this, &ServerUdp::sendHeartBeatSlot);
    m_heartbeattimeout = new QTimer(this);
    m_heartbeattimeout->setInterval(30 * 1000);
    connect(m_heartbeattimeout, &QTimer::timeout,
            [this](){
        m_heartbeattimeout->stop();
        this->state = StateBegin;
    });
    m_polltimer = new QTimer(this);
    m_polltimer->setInterval(5 * 1000);
    connect(m_polltimer, &QTimer::timeout,
            this, &ServerUdp::poll);
}

void ServerUdp::readPendingDatagrams()
{
    while (m_udp->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_udp->receiveDatagram();
        processTheDatagram(datagram);
    }
}

void ServerUdp::sendHeartBeatSlot()
{
    //
    if (state != StateOK)
        return;
    Message respmsg;
    respmsg.setPrefix(CD_MSG_PREFIX);
    respmsg.setSuffix(CD_MSG_SUFFIX);
    respmsg.setCommandID(CMD::DCMD__OLD_FUNCTION);
    respmsg.setSerialNumber(sn++);
    //respmsg.setVersionNumber(msg->VersionNumber());
    respmsg.setDestID(this->deviceid);
    respmsg.setObjectID(this->userid);
    char data[2];
    data[0] = uint8_t(0xff); // 前缀
    data[1] = uint8_t(0xc1); // 心跳码
    //uint8_t buf[8];
    //data[2] = buf[0]; // 这两个字节不知有啥用...
    //data[3] = buf[1];
    respmsg.appendContent(data, sizeof(data));
    respmsg.appendContent(this->deviceid);
    //respmsg.appendContent(data, sizeof(data));
    respmsg.Encrypt(&m_key[0], 16);
    std::vector<char> dbyte;
    respmsg.toByteArray(&dbyte);
    m_udp->writeDatagram(dbyte.data(), dbyte.size(), m_deviceaddr, m_devport);
}

void ServerUdp::poll()
{
    //
    if (state == StateBegin
        || state == StateRandomNumberSend)
    {
        this->openChannel();
    }
}

void ServerUdp::getkey(const std::vector<char> &sessionkey)
{
    m_key = std::vector<char>(16, char(0));
    char md5UserID[16] = {0};
    MD5 md5;
    md5.input((const char*) &this->userid, sizeof(userid));
    md5.output(md5UserID);
    for(size_t i = 0; i < 16; i++)
    {
        m_key[i] = sessionkey[i] ^ md5UserID[i];
    }
}

bool ServerUdp::bind()
{
    //
    ::writelog(InfoType, "bind");
    m_udp->bind();
    m_polltimer->start();
    return true;
}

quint64 ServerUdp::writedata(const QByteArray &d)
{
    if (state != StateOK)
        return 0;
    Message respmsg;

    respmsg.setPrefix(CD_MSG_PREFIX);
    respmsg.setSuffix(CD_MSG_SUFFIX);
    respmsg.setCommandID(CMD::DCMD__OLD_FUNCTION);
    respmsg.setSerialNumber(sn++);
    //respmsg.setVersionNumber(msg->VersionNumber());
    respmsg.setDestID(this->deviceid);
    respmsg.setObjectID(this->userid);
	//char data[2];
    //data[0] = uint8_t(0xff); // 前缀
    //data[1] = uint8_t(0xc1); // 心跳码
    //uint8_t buf[8];
    //data[2] = buf[0]; // 这两个字节不知有啥用...
    //data[3] = buf[1];
    respmsg.appendContent(d.data(), d.size());
    //respmsg.appendContent(this->deviceid);
    //respmsg.appendContent(data, sizeof(data));
    respmsg.Encrypt(&m_key[0], 16);
    std::vector<char> dbyte;
	//::writelog(InfoType,"dbyte len:%d",dbyte.size());
	//for(int i = 0; i < dbyte.size();++i){
		//printf("%02x ",(uchar)dbyte[i]);
	//}
	//printf("\n");
    respmsg.toByteArray(&dbyte);
	return m_udp->writeDatagram(dbyte.data(), dbyte.size(), m_deviceaddr, m_devport);
    //return m_udp->writeDatagram(d, m_deviceaddr, m_devport);
}

void ServerUdp::disconnect()
{

}

void ServerUdp::readdata()
{

}

void ServerUdp::processTheDatagram(const QNetworkDatagram &dg)
{
    //
    QByteArray d = dg.data();
    const char *buf = d.data();
    if (d.size() == 10)
    {
        if (state != StateRandomNumberSend)
            return;

        if (uint8_t(buf[0]) == 0xff && uint8_t(buf[1]) == 0xc0)
        {
            uint64_t randomnumber = getSessionRandomNumber(((char *)buf) + 2, 8);
            if (randomnumber == this->randomnumber)
            {
                // ok
                state = StateOK;
                m_deviceaddr = dg.senderAddress();
                m_devport = dg.senderPort();
                startSendHeartBeat();
                ::writelog(WarningType, "ok rddddddddddmessage.");
            }
            ::writelog(WarningType, "rddddddddddmessage.");
        }
        else
        {
            ::writelog(WarningType, "unknow message.");
        }
        return;
    }

    //if (d.size() <= PACK_HEADER_SIZE_V2)
    //{
    //    //::writelog(WarningType, "error message size: %d, from %s",
    //    //            ret,
    //    //           sa.toString().c_str());
    //    //return;
    //}

    // 解析用户id和设备id
    //uint64_t srcid = ::ReadUint64(buf + 7); // 包头第7个字节开始是源id
    //uint64_t destid = ::ReadUint64(buf + 7 + 8); // 包头第7+8个字节开始是目标id
    Message m;
    if (!m.fromByteArray(d.data(), d.size()))
    {
        ::writelog(InfoType, "error msggggggggggg");
    }
    if (m.prefix() == CD_MSG_PREFIX)
    {
        int len = m.Decrypt(&m_key[0]);
        if (len < 0)
        {
            ::writelog(InfoType, "decrypt msg failed");
            return;
        }
    }
    else
    {
        ::writelog(InfoType, "error msg prefix, %x", m.prefix());
    }
    char *p = m.content();
    if (m.contentLen() == 0)
        return;

    uint8_t prefix = ::ReadUint8(p);
    uint8_t opcode = ::ReadUint8(p + 1);
    if (prefix == 0xff)
    {
        switch (opcode)
        {
        //case 0xb0:
        //    ::writelog(InfoType, "open channel:%llu",
        //               this->deviceid);
        //    this->openChannel(msg, dev);
        //    break;

        case 0xc1:
            //::writelog(InfoType, "client heart beat");
            // restart timer
            m_heartbeattimeout->start(30 * 1000);
            break;

        default:
            ::writelog(InfoType,
                       "communction unknow opcode:%u",
                       opcode);
            break;
        }
    }
    // 通信数据，06 gateway协议读数据,01,02,03,04其他协议打开设备，读设备，写设备等
    else if (prefix == 0x06 || prefix == 0x01 || prefix == 0x02 || prefix == 0x03 || prefix == 0x04)
    {
        //::writelog(InfoType, "communction111,prefix:%02x",prefix);
        emit readyRead(QByteArray(p, m.contentLen()));
        //this->communicationData(msg, dev, CDSessionKey);
    }
    else
    {
        ::writelog(InfoType, "communction error prefix:%02x", prefix);
    }
}

void ServerUdp::openChannel()
{
    state = StateBegin;

    // send random number
    // sendRandomNumber();
    uint64_t rd = ::genSessionRandomNumber();
    this->randomnumber = rd;
    rapidjson::Document v;
    std::shared_ptr<DeviceData> deviceData = static_pointer_cast<DeviceData>(RedisCache::instance()->getData(deviceid));
    if (!deviceData)
    {
		// retry 目的为减少日志打印次数，由5秒每次减少到5*40秒每次
		if(retry >= 40){
			retry = 0;
		}
		if(retry == 0){
			::writelog(InfoType, "error datttttttttttttttttttttttttttt, %llu", this->deviceid);
		}
		retry++;
		// 设备断线会保持重连150秒，超过150秒仍然连不上,redis会从在线设备表中删除该设备
		//QThread::sleep(150);
		//deviceData = static_pointer_cast<DeviceData>(RedisCache::instance()->getData(deviceid));
        return;
    }
	retry = 0;
    std::string addr = deviceData->sockAddr().toString();
    this->getkey(deviceData->sessionKey());
    v.SetObject();
    v.AddMember("rd", rd, v.GetAllocator());
    v.AddMember("did", this->deviceid, v.GetAllocator());
    v.AddMember("uid", this->userid, v.GetAllocator());
    v.AddMember("eip", rapidjson::StringRef(this->extserverip.c_str()),  v.GetAllocator());
    v.AddMember("port", this->m_udp->localPort(), v.GetAllocator());
    v.AddMember("deviceip", rapidjson::StringRef(addr.c_str()), v.GetAllocator());
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    v.Accept(writer);
    const char* output = buffer.GetString();
    ::writelog(InfoType, "openchannel: %s", output);
    RabbitMqService::get()->publish(output);

    state = StateRandomNumberSend;
    //::writelog(InfoType, "senr nummmmmmmmmmmmmmmm");
}

void ServerUdp::startSendHeartBeat()
{
    m_heartbeattimer->start();
    m_heartbeattimeout->start();
}
