#ifndef IDELSCENEFROMFILESERVER__H
#define IDELSCENEFROMFILESERVER__H
#include <stdint.h>
class IDelScenefromFileServer
{
public:
    virtual ~IDelScenefromFileServer() {}
    virtual int postDeleteMsg(uint64_t sceneid, uint64_t userid) = 0;
};
#endif
