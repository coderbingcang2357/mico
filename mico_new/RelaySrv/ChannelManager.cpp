#include <assert.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <list>
#include <sys/time.h>
#include "ChannelManager.h"
#include "protocoldef/protocol.h"
#include "util/logwriter.h"
#include "mainqueue.h"
#include "relayinternalmessage.h"
#include "sessionRandomNumber.h"
#include "mainqueue.h"

static const int64_t CHANNEL_TIMEOUT = 120;

ChannelManager::ChannelManager()
{

}

ChannelManager::~ChannelManager()
{

}

void ChannelManager::insert(const std::shared_ptr<ChannelInfo> &ch)
{
    ::writelog("inser channel....");
    UserDevPair userdev(ch->userID, ch->deviceID);
    auto already = this->findChannel(userdev);
    if (already)
        return;
    ::writelog(InfoType, "inser channel....%lu, %lu, %d, %d", ch->userID, ch->deviceID,
        ch->userfd, ch->devfd);
    m_channels.insert(std::make_pair(userdev, ch));
}

void ChannelManager::openChannel(const std::shared_ptr<ChannelInfo> &ch)
{
    UserDevPair userdev(ch->userID, ch->deviceID);
    auto already = this->findChannel(userdev);
    if (already)
    {
        sockaddrwrap empty;
        memset(&empty, 0, sizeof(empty));
        this->m_rndChannels.erase(already->userSessionRandomNumber);
        this->m_rndChannels.erase(already->devSessionRandomNumber);
        already->userSessionRandomNumber = ch->userSessionRandomNumber;
        already->devSessionRandomNumber = ch->devSessionRandomNumber;
        already->userSockAddr = empty;
        already->deviceSockAddr = empty;
        already->channelid = ch->channelid;
        // insert Randomnumber To Map, so we can find channel by rand number
        m_rndChannels.insert(std::make_pair(ch->userSessionRandomNumber, already));
        m_rndChannels.insert(std::make_pair(ch->devSessionRandomNumber, already));
        assert(ch->userfd == already->userfd);
        assert(ch->devfd == already->devfd);
    }
    else
    {
        m_channels.insert(std::make_pair(userdev, ch));
        // insert Randomnumber To Map, so we can find channel by rand number
        m_rndChannels.insert(std::make_pair(ch->userSessionRandomNumber, ch));
        m_rndChannels.insert(std::make_pair(ch->devSessionRandomNumber, ch));
    }
    ::writelog(InfoType, "open channel: %lu, %lu, userfd: %d, devfd: %d",
                ch->userID, ch->deviceID,
                ch->userfd, ch->devfd);
}

void ChannelManager::closeChannel(const UserDevPair &ud)
{
    auto ch = this->findChannel(ud);
    if (!ch)
        return;
    if (ch->userSessionRandomNumber != 0)
    {
        m_rndChannels.erase(ch->userSessionRandomNumber);
    }
    if (ch->devSessionRandomNumber != 0)
    {
        m_rndChannels.erase(ch->devSessionRandomNumber);
    }
    m_channels.erase(ud);
    MainQueue::postMsg(new MessageReleasePort(ch->userID, ch->deviceID, ch->channelid));
    // remove channel from database
    MainQueue::postMsg(new MessageRemoveChannelFromDatabase(ch->userID, ch->deviceID));
    ::writelog(InfoType, "close channel: %lu, %lu, chid:%lu", ch->userID, ch->deviceID, ch->channelid);
}

std::shared_ptr<ChannelInfo> ChannelManager::findChannel(const UserDevPair &ud)
{
    auto it = m_channels.find(ud);
    if (it != m_channels.end())
    {
        return it->second;
    }
    else
    {
        return std::shared_ptr<ChannelInfo>(0);
    }
}

bool ChannelManager::updateAddress(uint64_t randomnumber, const sockaddrwrap &addr)
{
    auto it = m_rndChannels.find(randomnumber);
    if(it == m_rndChannels.end())
        return false;
    auto ch = it->second;
    if (ch->userSessionRandomNumber == randomnumber)
    {
        ::writelog(InfoType,
            "uid: %lu, did: %lu, modify user address to: %s", ch->userID, ch->deviceID, addr.toString().c_str());
        m_rndChannels.erase(randomnumber);
        ch->userSessionRandomNumber = 0;
        ch->userSockAddr = addr;
        //return true;
    }
    else if (ch->devSessionRandomNumber == randomnumber)
    {
        ::writelog(InfoType, "uid: %lu, did: %lu, modify dev address to: %s",
                    ch->userID,
                    ch->deviceID,
                   addr.toString().c_str());
        m_rndChannels.erase(randomnumber);
        ch->devSessionRandomNumber = 0;
        ch->deviceSockAddr = addr;
        //return true;
    }
    else
    {
        return false;
    }
    if (ch->deviceSockAddr.port() != 0 && ch->userSockAddr.port() != 0)
    {
        // write channel to database
        MessageInsertChannelToDatabase *msg = new MessageInsertChannelToDatabase;
        msg->userid() = ch->userID;
        msg->deviceid() = ch->deviceID;
        msg->userport() = ch->userport;
        msg->deviceport() = ch->deviceport;
        msg->userfd() = ch->userfd;
        msg->devicefd() = ch->devfd;
        msg->useraddress() = ch->userSockAddr;
        msg->deviceaddress() = ch->deviceSockAddr;
        MainQueue::postMsg(msg);
    }
    return true;
}

// 轮询所有的端口检查是否超时, 如果超时可把通道关闭
void ChannelManager::poll()
{
    std::list<UserDevPair> channelWillClose;
    timeval cur;
    gettimeofday(&cur, 0);
    for (auto &v : m_channels)
    {
        timeval diff;
        timersub(&cur, &(v.second->lastCommuniteTime), &diff);
        if (diff.tv_sec >= CHANNEL_TIMEOUT)
        {
            // ok timeout
            channelWillClose.push_back(UserDevPair(v.second->userID,
                                                v.second->deviceID));
        }
    }
    // close channel in list
    for (auto &v : channelWillClose)
    {
        this->closeChannel(v);
    }
}
