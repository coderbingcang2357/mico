#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <iostream>
#include <signal.h>
#include <string.h>
#include "logread.h"
#include "logserver.h"

void test();
void testlast1();
void testlast0();
void testlast1000();
void testsmallfile();

bool mainfile_isrun = true;
static void sigusr1(int )
{
    mainfile_isrun = false;
    //printf("siguser1\n");
}

int main(int argc, char **argv) 
{
    if (argc < 2)
    {
        printf("error argument\nusage: %s /path/to/log/file [-daemon]\nSIG_USR1 to quit.\n", argv[0]);
        return 1;
    }
    // daemon
    bool isdaemon = false;
    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "-daemon") == 0)
        {
            isdaemon = true;
        }
    }
    if (isdaemon)
    {
        if (daemon(1, 0) == -1)
        {
            printf("daemon failed.%s\n", strerror(errno));
        }
    }

    if (signal(SIGUSR1, sigusr1) == SIG_ERR)
    {
        printf("signal failed.\n");
        return 3;
    }
    // test();
    Logserver server(8883, argv[1]);
    if (!server.create())
    {
        printf("create server failed.\n");
        return 2;
    }

    server.run();
    return 0;
}

void test()
{
    testlast0();
    testlast1();
    testlast1000();
    testsmallfile();
}

void testlast0()
{
    Logreader r("test.txt");
    assert(r.init());
    std::string d;
    int cnt = r.getLast(0, &d);
    std::cout << "readsize: " << cnt << std::endl << d << std::endl;

}

void testlast1()
{
    Logreader r("test.txt");
    assert(r.init());
    std::string d;
    int cnt = r.getLast(1, &d);
    std::cout << "readsize: " << cnt << std::endl << d << std::endl;

}

void testlast1000()
{
    Logreader r("test.txt");
    assert(r.init());
    std::string d;
    int cnt = r.getLast(1000, &d);
    std::cout << "readsize: " << cnt << std::endl << d << std::endl;
}

void testsmallfile()
{
    Logreader r("smalltest.txt");
    assert(r.init());
    std::string d;
    int cnt = r.getLast(100, &d);
    std::cout << "readsize: " << cnt << std::endl << d << std::endl;
}
