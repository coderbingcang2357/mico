#ifndef SERVER_H__H
#define SERVER_H__H
#include <map>
#include <memory>
#include <vector>

class Connection;

class Server
{
static const int WakeUpFdCount = 10;
public:
    Server(int port_, int tc);
    ~Server();
    void run();

private:
    void run(int i);
    void createsockfd();
    void createepollfd();
    void createWakeupfd();

    void insertConnection(Connection *con);
    void processReadEvent(int threadid, int fd);
    void processData(const std::shared_ptr<Connection> &conn);
    void processPack(const std::shared_ptr<Connection> &conn, const std::vector<char> &pack);


    int threadcount;
    bool isrun;
    int port;
    int listenfd;
    int epollfd[WakeUpFdCount];
    int wakeupfd[WakeUpFdCount][2];

    std::map<int, std::shared_ptr<Connection> > connections[WakeUpFdCount];

    friend class ThreadProc;
};


#endif

