#include <iostream>

#include "sharedptr.h"

class Test
{
public:
    Test(int i) : m_i(i)
    {
        std::cout << "new Test " << i << std::endl;
    }

        ~Test()
        {
            std::cout << "descturct  Test " << m_i << std::endl;
        }

        void p()
        {
            std::cout << "the value is: " << m_i << std::endl;
        }
        int m_i;
};

void test()
{
    shared_ptr<Test> p(new Test(0));
    p->p();
    (*p).p();
    for (int i = 0; i < 1; i++)
        p = shared_ptr<Test>(new Test(i + 1));
    p->p();
    (*p).p();
}
class TestBase
{
public:
    TestBase(int i) : m_i(i)
    {
    }
    virtual ~TestBase() { std::cout << "desc base.\n"; }

    virtual void p()
    {
        std::cout << "base "<< m_i << std::endl;
    }
    int m_i;
};

class TestDev : public TestBase
{
public:
    TestDev(int i) : TestBase(i){}
    ~TestDev() { std::cout << "desc child.\n";}

    void p()
    {
        std::cout << "child " << m_i << std::endl;
    }
};

void test2()
{
    TestBase *b = new TestDev(0);
    shared_ptr<TestBase> ptrb(b);
    shared_ptr<TestBase> pcp(ptrb);
    shared_ptr<TestDev> pdev(dynamic_pointer_cast<TestDev, TestBase>(ptrb));

    ptrb->p();
    pdev->p();
    pcp->p();
}

int main()
{
    //test();
    std::cout << "===================================\n";
    test2();
    return 0;
}
