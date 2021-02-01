#include <unistd.h>
#include <iostream>
#include <string>
#include <stdint.h>
#include <stdlib.h>
#include "tcpserver.h"
#include "tcpmessageprocessor.h"
#include "thread/threadwrap.h"

class Tr : public Proc
{
public:
    Tr(Tcpserver *t, std::string a)  : tcp(t), addr(a)
    {
        for (int i =0; i < 100; ++i)
        {
            str.append("T");
        }
    }

    void run()
    {
        usleep(10000);
        for (;;)
        {
            tcp->send(addr, str.c_str(), str.size() + 1);
            //usleep(10);
        }
    }

    std::string str;
    Tcpserver *tcp;
    std::string addr;
};

class Newmessage : public Tcpmessageprocessor
{
public:
    void processMessage(const std::vector<char> &d)
    {
        printf("readdata: %s\n", &d[0]);
    }
};

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        return 0;
    }
    uint16_t port = atoi(argv[1]);
    std::string ip;// = argv[1];
    Tcpserver server(ip, port, new Newmessage);
    server.create();
    std::string str;
    std::getline(std::cin, str);

    Thread *t[10];
    for (int i = 0; i < 2; ++i)
    {
        t[i] = new Thread(new Tr(&server, argv[2]));
    }
    while (1)
    {
        //printf("send data\n");
        //server.send(argv[2], str.c_str(), str.size() + 1);
        usleep(100000);
    }
    return 0;
}
