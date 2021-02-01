#ifndef USERDEVICEPAIR__H_
#define USERDEVICEPAIR__H_
#include <stdint.h>
class UserDevPair
{
public:
    // 这个类是做为map的key使用
    // 为了实现UserDevPair(id1, id2)和UserDevPair(id2, id1),代表同一个key
    // 所以对id1,id2进行了大小处理
    UserDevPair(uint64_t id1, uint64_t id2)
    {
        if (id1 < id2)
        {
            m_minID = id1;
            m_maxID = id2;
        }
        else
        {
            m_minID = id2;
            m_maxID = id1;
        }
    }

    bool operator < (const UserDevPair& other) const
    {
        if(m_minID < other.m_minID)
        {
            return true;
        }
        else if(m_minID == other.m_minID)
        {
            return m_maxID < other.m_maxID;
        }
        else
        {
            return false;
        }
    }
    bool operator==(const UserDevPair& other) const
    {
        if(m_minID == other.m_minID && m_maxID == other.m_maxID)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

private:
    uint64_t m_minID;
    uint64_t m_maxID;
};
#endif

