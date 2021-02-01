#ifndef MICOASSERT__H
#define MICOASSERT__H
#include <string>
static void mcassert_(bool b, int line, const char *info="")
{
    if (!b)
    {
        throw std::string(std::to_string(line) + std::string(" ") + std::string(__FILE__)+" falied: " + info);
    }
}

#define mcassert(b) mcassert_((b), __LINE__)
#define mcassert2(b, info) mcassert_((b), __LINE__, (info))
#endif
