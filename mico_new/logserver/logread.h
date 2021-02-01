#ifndef LOGREAD__H
#define LOGREAD__H

#include <string>
#include <stdint.h>

class Logreader 
{
public:
    Logreader(const char *path);
    ~Logreader();
    bool init();
    int32_t getLast(int size, std::string *d);

private:
    int getFirstNewlinepos(char *buf, int buflen);
    int getlines(char *buff, int buflen, int maxlines, std::string *d);
    void removelinesFromString(int howmany, std::string *d);

    const char *m_path;
    int m_fd;
};

#endif
