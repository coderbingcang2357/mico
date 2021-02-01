#ifndef TCPMESSAGEPROCESSOR__H
#define TCPMESSAGEPROCESSOR__H
#include <vector>
class TcpServerMessage;
class Tcpmessageprocessor
{
public:
    virtual ~Tcpmessageprocessor(){}
    virtual void processMessage(TcpServerMessage *msg) = 0;
};
#endif
