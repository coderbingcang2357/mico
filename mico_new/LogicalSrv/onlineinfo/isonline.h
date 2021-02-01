#ifndef ISONLINE__H
#define ISONLINE__H

#include <stdint.h>
class ICache;

class IsOnline
{
public:
    IsOnline(ICache *, ICache *);
    bool isOnline(uint64_t id);

private:
    ICache  *m_c1;
    ICache  *m_c2;
};
#endif
