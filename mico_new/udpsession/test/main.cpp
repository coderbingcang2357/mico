#include "../messagerepeatchecker.h"
//#include "../../util/micoassert.h"
#include <assert.h>
#include <netinet/in.h>
#include "../udpsession.h"
#include <iostream>

class TestUdpsession
{
public:
    void test();
};
class TestMessageRepeatCheaker
{
public:
    void test();
};

void test();
int main()
{
    TestUdpsession t;
    t.test();
    TestMessageRepeatCheaker tc;
    tc.test();
    printf("\nTest Ok.\n");
    return 0;
}

void TestMessageRepeatCheaker::test()
{
    MessageRepeatCheaker::UdpconnKey k, k2;
    k.iplow = 2;
    k.port = 3;
    k.clientid = 4;

    k2.iplow = 2;
    k2.port = 3;
    k2.clientid = 4;

    assert(!(k < k2) && !(k2 < k));

    MessageRepeatCheaker checker;
    sockaddr_in addr;
    addr.sin_addr.s_addr = 2;
    addr.sin_port = 3;
    uint64_t client  = 4;
    // will crash
    //checker.isMessageRepeat((sockaddr *)&addr, client, 3);
    checker.createSession((sockaddr *)&addr, client);
    checker.createSession((sockaddr *)&addr, client + 1);
    bool r = checker.isMessageRepeat((sockaddr *)&addr, client, 0);
    assert(!r);
    r = checker.isMessageRepeat((sockaddr *)&addr, client, 0);
    assert(r);
    for (int i = 1; i < 100; ++i)
    {
        bool r = checker.isMessageRepeat((sockaddr *)&addr, client, i);
        assert(!r);
    }
    r = checker.isMessageRepeat((sockaddr *)&addr, client, 99);
    assert(r);
    r = checker.isMessageRepeat((sockaddr *)&addr, client, 0);
    assert(r);
    r = checker.isMessageRepeat((sockaddr *)&addr, client, 1);
    assert(r);


    // insert 100 pop 0
    r = checker.isMessageRepeat((sockaddr *)&addr, client, 100);
    assert(!r);
    // insert 0 pop 1
    r = checker.isMessageRepeat((sockaddr *)&addr, client, 0);
    assert(!r);
    // insert 1 pop 2
    r = checker.isMessageRepeat((sockaddr *)&addr, client, 1);
    assert(!r);

    checker.removeSession((sockaddr *)&addr, client);
    assert(checker.m_udpconnections.size() == 1);
    checker.removeSession((sockaddr *)&addr, client + 1);
    assert(checker.m_udpconnections.size() == 0);
}

void TestUdpsession::test()
{
    sockaddr_in addr;
    addr.sin_addr.s_addr = 1;
    addr.sin_port = 0;
    Udpsession us(12, addr);
    bool r = us.isIDExist(0);
    assert(!r);

    for (uint32_t i = 0; i < 100; ++i)
    {
        us.insertID(i);
    }
    assert(us.idlist.size() == 100);
    assert(us.idset.size() == 100);
    assert(us.isIDExist(0));
    assert(us.isIDExist(1));
    assert(us.isIDExist(99));
    assert(!us.isIDExist(100));

    // insert 1 but 1 is already exist so nothing changed
    us.insertID(1);
    assert(us.idlist.size() == 100);
    assert(us.idset.size() == 100);
    assert(us.isIDExist(0));
    assert(us.isIDExist(1));

    // insert 100 and pop 0
    us.insertID(100);
    assert(us.idlist.size() == 100);
    assert(us.idset.size() == 100);
    assert(!us.isIDExist(0));
    assert(us.isIDExist(1));
}

