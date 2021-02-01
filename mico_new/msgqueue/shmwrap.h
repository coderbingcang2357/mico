#ifndef SHMWRAP__H
#define SHMWRAP__H

#include <string>
#include "imem.h"

class ShmWrap : public IMem
{
public:
    ShmWrap();
    ~ShmWrap();
    char *address();
    int length();

    bool create(const char *shmname, int size);
    void release();
private:
    char *m_d;
    int m_len;
    std::string m_name;
};
#endif
