#ifndef __RELAYTHREAD__H
#define __RELAYTHREAD__H

#include <netinet/in.h>
#include <sys/time.h>
#include <map>
#include <list>
#include <set>
#include <memory>
#include <functional>
#include <vector>
#include "thread/mutexwrap.h"
#include "util/sock2string.h"
class ChannelManager;

class RelayTimer
{
public:
    static bool Comp(const std::shared_ptr<RelayTimer> &l, const std::shared_ptr<RelayTimer> &r);
    //int fd1;
    //int fd2;
    std::function<void()> callback;
    timeval time;
};

class Thread;
class ChannelInfo;
class RelayThreadCommand
{
public:
    enum
    {
        Nothing,
        AddChannel,
        CloseChannel,
        InsertChannel
    };
    int commandid;
    int fd1;
    sockaddr_in addr1;
    uint16_t port1;

    int fd2;
    sockaddr_in addr2;
    uint16_t port2;

    uint64_t userid;
    uint64_t devid;

    uint64_t userSessionRandomNumber;
    uint64_t devSessionRandomNumber;
    std::shared_ptr<ChannelInfo> channel;
};

class RelayThread
{
    class DataToWrite
    {
    public:
        int sendfd;
        sockaddrwrap dest;
        std::vector<char> data;
    };
public:
    RelayThread();
    ~RelayThread();
    bool create();
    void quit() { m_isrun = false; }
    void addCommand(RelayThreadCommand *c);
    std::pair<bool, int> createRelaySocket(uint16_t port);
    //uint16_t getRelayPort();

private:
    void run();
    void wakeup();
    void createWakeupfd();
    void processQueueMessage();
    void processTimer();
    // void processTimeOut();
    void readData(int sockfd);


    Thread *m_thread;
    bool m_isrun;
    int m_epollfd;
    int wakeupfd[2];
    //int m_relayfd = -1; // 转发的socket
    std::list<int> m_relayfd; // 转发的socket
    //uint16_t m_relayport = 0; // 转发的端口
    MutexWrap mutex;
    std::list<RelayThreadCommand *> m_command;

    //std::set<std::shared_ptr<RelayTimer>, Comp> m_timer;
    //timeval lastprocesstimer;
    ChannelManager *m_channels = nullptr;
    timeval polltimer;
    friend class RelayProc;
    std::list<DataToWrite*> m_writelist;
};

#endif

