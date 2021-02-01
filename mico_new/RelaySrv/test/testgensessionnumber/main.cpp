#include "../../sessionRandomNumber.h"
#include <set>
#include <stdio.h>

int main()
{
    std::set<uint64_t> rnd;
    int repeat = 0;
    ::initialRandomnumberGenerator(2);
    printf("repeat: %d\n", repeat);
    for (int i = 0; i < 10; ++i)
    {
        printf("%lx\n", ::genSessionRandomNumber());
    }
    for (int i = 0; i < 100 * 10000; ++i)
    {
        uint64_t v = ::genSessionRandomNumber();
        auto it = rnd.find(v);
        if (it == rnd.end())
        {
            rnd.insert(v);
        }
        else
        {
            repeat++;
        }
    }
    return 0;
}
