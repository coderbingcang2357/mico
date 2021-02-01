#ifndef MAINQUEUE__H
#define MAINQUEUE__H

class RelayInterMessage;
namespace MainQueue
{
    RelayInterMessage *getMsg();
    void quit();
    void postMsg(RelayInterMessage *msg);
    bool hasMsg();
}
#endif
