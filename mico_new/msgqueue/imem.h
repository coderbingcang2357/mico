#ifndef IMEM___H
#define IMEM___H

class IMem
{
public:
    virtual ~IMem(){}
    virtual char *address() = 0;
    virtual int length() = 0;
};
#endif
