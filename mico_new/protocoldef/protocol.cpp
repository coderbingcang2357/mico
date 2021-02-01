#include "protocol.h"

IdType GetIDType (uint64_t id)
{
    switch(uint8_t(id >> (64 - 8)) & 0x0F) {
        case 0x00:
            return IT_User;
        case 0x01:
            return IT_Device;
        case 0x02:
            return IT_DClus;
        case 0x03:
            return IT_UClus;
        default:
            return IT_Unknown;
    }
}

uint64_t Mac2DevID(uint64_t macAddr)
{
    return (macAddr | 0x0101000000000000);
}
