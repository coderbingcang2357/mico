#include <iostream>
#include <assert.h>
#include <stdio.h>
#include "../shmmsgqueue.h"
#include "../imem.h"
#include "thread/threadwrap.h"
#include "timer/TimeElapsed.h"

class TestMem : public IMem
{
public:
    TestMem()
    {
        len = 6000 * 1 + sizeof(MsgQueue) + 1;
        d = new char[len];
    }

    ~TestMem()
    {
        delete [] d;
        len = 0;
    }
    char *address()
    {
        return d;
    }

    int length()
    {
        return len;
    }

    char *d;
    int len;
};

void test1()
{
    TestMem mem;
    ShmMessageQueue queue(&mem);
    queue.init();
    //assert(queue.freeSize() == 600);
    for (int i = 0; i < 100; i++)
    {
        bool b = queue.put(&i, sizeof(i));
        assert(b == true);
    }

    //int emp;
    //bool b = queue.put(&emp, sizeof(emp));
    //assert(b == false);
    assert(queue.messageSize() == 100);

    for (int i = 0; i < 100; i++)
    {
        int l ;
        int len = queue.get(&l, sizeof(l));
        //std::cout << "value: " << l << std::endl;
        assert(len == 4);
        assert(l == i);
    }
    assert(queue.messageSize() == 0);
}

void test2()
{
    TestMem mem;
    ShmMessageQueue queue(&mem);
    queue.init();
    //assert(queue.freeSize() == 600);
    for (int i = 0; i < 75; i++)
    {
        if (i % 2 == 0)
        {
            bool b = queue.put(&i, sizeof(i));
            assert(b == true);
        }
        else
        {
            char c = (char)i;
            bool b = queue.put(&c, sizeof(c));
            assert(b == true);
        }
    }
    for (int i = 0; i < 75; i++)
    {
        int l ;
        int len = queue.get(&l, sizeof(l));
        //std::cout << "value: " << l << std::endl;
        //assert(len == 4);
        //assert(l == i);
    }
    assert(queue.messageSize() == 0);

    for (int i = 0; i < 60; i++)
    {
        bool b = queue.put(&i, sizeof(i));
        assert(b == true);
    }
    assert(queue.messageSize() == 60);
    for (int i = 0; i < 60; i++)
    {
        int l ;
        int len = queue.get(&l, sizeof(l));
        //std::cout << "value: " << l << std::endl;
        assert(len == 4);
        assert(l == i);
    }
    assert(queue.messageSize() == 0);
}

void testProduceConsumer();

int main()
{
    //test1();
    //test2();
    testProduceConsumer();
    return 0;
}

int Size = 300 * 1000;

class Producer : public Proc
{
public:
    Producer(ShmMessageQueue *queue) : m_q(queue){}

    void run()
    {
        for (int i = 0; i < Size; i++)
        {
            m_q->put(&i, sizeof(i));
        }
    }

    ShmMessageQueue *m_q;
};

class Consumer : public Proc
{
public:
    Consumer(ShmMessageQueue *queue) : m_q(queue) {}

    void run() 
    {
        int i;
        int v;
        for (i = 0; i < Size; i++)
        {
            m_q->get(&v, sizeof(v));
            assert(v == i);
        }
        printf("last value of v: %d.\n", v);
    }
    ShmMessageQueue *m_q;
};

void testProduceConsumer()
{
    TestMem mem;

    ShmMessageQueue queue(&mem);
    queue.init();
    Producer p(&queue);
    Consumer c(&queue);
    TimeElap elap;
    elap.start();
    {
    Thread pth(&p);
    Thread cth(&c);
    }
    printf("time used: %ld us\n", elap.elaps());
}
