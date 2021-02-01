#include <stdio.h>
#include <stdint.h>
#include <vector>
#include "./tea.h"
#include "MD5.h"

int main()
{
    std::vector<char> d, out;
    for (int i = 0; i < 50 * 1024; ++i)
    {
        d.push_back(i*i);
        out.push_back(i*8);
    }
    out.reserve(50 * 1024 + 1024);
    int r = ::tea_enc(&d[0], d.size(), out.data(), "abc", 3);
    printf("sizeof: %d, %d\n", r, out.size());
    return 0;
}
