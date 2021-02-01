#include "isonline.h"
#include "icache.h"

IsOnline::IsOnline(ICache *c1, ICache *c2) : m_c1(c1), m_c2(c2)
{

}

bool IsOnline::isOnline(uint64_t id)
{
    if (m_c1->objectExist(id))
        return true;

    if (m_c2->objectExist(id))
        return true;

    return false;
}
