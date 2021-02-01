#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include "thread/threadwrap.h"
#include "timer/TimeElapsed.h"
#include "../users.h"
#include "user/user.h"
#include "../command.h"
#include "util/util.h"
#include "util/tcpdatapack.h"
#include "dbproxyclient/dbproxyclient.h"
#include "dbproxyclient/cacheddbuseroperator.h"
#include "dbproxyclient/dbproxyclientpool.h"
#include "protocoldef/protocol.h"

static const int MaxThread = 10000;
int threadcount = 4;
int maxmessage = 10;
char *srvaddr;
void run(int id);
void run2(int id);
void parsearg(int argc, char **argv);

DbproxyClientPool *pool;
class TestProc : public Proc
{
public:
    TestProc (int id_) : id(id_) {}
    void run()
    {
        ::run2(id);
    }

    int id;
};

enum State
{
Init,
count,
addr,
thread
};

void parsearg(int argc, char **argv)
{
    enum State state = Init;
    for (int i = 1; i < argc; ++i)
    {
        switch (state)
        {
        case Init:
        {
            if (strcmp(argv[i], "count") == 0)
            {
                state = count;
            }
            else if (strcmp(argv[i], "addr") == 0)
            {
                state = addr;
            }
            else if (strcmp(argv[i], "thread") == 0)
            {
                state = thread;
            }
            else if (strcmp(argv[i], "-h") == 0)
            {
                printf("%s count 10000 addr 127.0.0.1 thread 20\n", argv[0]);
                exit(0);
            }
            else
            {
                printf("error arg:%s\n", argv[i]);
            }
        }
        break;

        case count:
        {
            maxmessage = atoi(argv[i]);
            state = Init;
        }
        break;

        case addr:
        {
            srvaddr = argv[i];
            state = Init;
        }
        break;

        case thread:
        {
            threadcount = atoi(argv[i]);
            if (threadcount > 1000)
                threadcount = 1000;
            state = Init;
        }
        break;
        };
    }

    if (state != Init)
    {
        printf("arg error.\n");
    }
}

int main(int argc, char **argv)
{
    const char *sd = "127.0.0.1";
    srvaddr = (char *)sd;
    parsearg(argc, argv);

    pool = new DbproxyClientPool(std::string(srvaddr), 8989);

    Thread *thread[MaxThread];
    TimeElap elap;
    elap.start();
    for (int i=0; i < threadcount; ++i)
    {
        thread[i] = new Thread(new TestProc(i));
    }
    for (int i = 0; i < threadcount; ++i)
    {
        delete thread[i];
    }
    long t  = elap.elaps();
    printf("%d, timeused: %lds, %ldms, %ldus\n", maxmessage, t / (1000 * 1000), t/1000, t % (1000 * 1000));
    assert(pool->poolsize() == threadcount);

    return 0;
}

void run(int id)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    assert(fd >= 0);
    sockaddr_in addr;
    addr.sin_addr.s_addr = inet_addr(srvaddr);
    addr.sin_port = htons(8989);
    addr.sin_family = AF_INET;
    socklen_t len = sizeof(addr);
    // printf("connect:\n");
    if (connect(fd, (sockaddr *)&addr, len) != 0)
    {
        perror("conenct error:");
        exit(0);
    }
    printf("connect: %d\n", id);
    int msgs = maxmessage / threadcount;

    std::vector<char> data, packdata;
    ::WriteUint32(&data, GetUserInfo);
    ::WriteUint64(&data, uint64_t(100));
    TcpDataPack::pack(&data[0], data.size(), &packdata);
    while (msgs--)
    {
        // printf("send:\n");
        int len = write(fd, &packdata[0], packdata.size());
        (void)len;

        char buf[1000];
        int l = read(fd, buf, sizeof(buf));
        std::vector<char> unpackdata;
        // printf("read:%d\n", l);
        int pl = TcpDataPack::unpack(buf, l, &unpackdata);
        assert(pl > 0);
        UserInfo i;
        User u(i);
        bool r = u.fromByteArray(&unpackdata[0], unpackdata.size());
        assert(r);
    }
    close(fd);
}

void run2(int id)
{
    printf("connect: %d\n", id);
    // DbproxyClient client(srvaddr, 8989);
    //int res = client.connect();
    // if (res != DbSuccess)
    // {
    //     return;
    // }
    //CachedDbUserOperator cuserop(&client);
    std::shared_ptr<DbproxyClient> client = pool->get();
    if (!client)
    {
        return;
    }
    CachedDbUserOperator cuserop(client.get());
    int msgs = maxmessage / threadcount;

    while (msgs--)
    {
        User u;
        uint16_t answercode = ANSC::Success;
        int res = cuserop.getUserInfo(100039, &u, &answercode);
        //if (res != DbSuccess)
        //{
        //    printf("error.");
        //    while (1)
        //    {
        //        sleep(12);
        //    }
        //}
        assert(res == DbSuccess);
    }
}

