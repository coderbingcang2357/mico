#ifndef DBPROXY__CLIENT__H
#define DBPROXY__CLIENT__H
#include <string>
#include <vector>

static const int DbError = -1;
static const int DbSuccess = 0;
static const int DbConnectFailed = 1;
static const int DbTimeOut = 2;
static const int DbConnectionClosed = 3;

class User;
class DbproxyClient
{
public:
    DbproxyClient(const std::string &serveraddr, int port);
    ~DbproxyClient();
    int connect();
    int sendCommandAndWaitReplay(char *buf, int buflen, std::vector<char> *pack);
    int ping();

private:
    int readsocket(std::vector<char> *unpackdata);
    int writesocket(char *buf, int buflen);

    std::string m_serveraddr;
    int m_port;
    int m_sockfd;
};


#endif
