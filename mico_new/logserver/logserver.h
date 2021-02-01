#ifndef LOGSERVER__H
#define LOGSERVER__H
#include <stdint.h>
#include <string>
#include <netinet/in.h>

class Logreader;
class Logserver
{
public:
    Logserver(int port, char *logfilepath);
    ~Logserver();
    bool create();
    void run();

private:
    void processCommand(const sockaddr_in &sockaddr, char *buf, int len);

    void addLog(const sockaddr_in &sockaddr, char *log);
    void getLastLog(const sockaddr_in &sockaddr, int size);


    uint16_t m_port;
    int m_listensock;
    char *m_logfilepath;
    Logreader *m_logreader;
};

#endif
