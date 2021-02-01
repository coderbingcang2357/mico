#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <algorithm>
#include "logread.h"

class ReverseFileBuffer
{
public:
    ReverseFileBuffer(int fd) : m_fd(fd), buffedcharsize(0)
    {
        fileend = lseek(m_fd, 0, SEEK_END);
        curpos = fileend;
        if (fileend == -1)
        {
            assert(false);
            printf("lseek error 1:%s\n", strerror(errno));
        }
    }

    bool getChar(char *c)
    {
        if (buffedcharsize == 0)
        {
            if (!readMore())
                return false;
            if (buffedcharsize == 0)
                return false;
        }
        buffedcharsize--;
        *c = buff[buffedcharsize];

        return true;
    }

    bool readMore()
    {
        if (curpos == 0)
            return false;

        int datatoread;
        if (fileend > int(sizeof(buff)))
            datatoread = sizeof(buff);
        else
            datatoread = fileend;
        curpos = lseek(m_fd, fileend - datatoread, SEEK_SET);
        if (curpos == -1)
        {
            printf("lseek error 2: %s\n", strerror(errno));
            return false;
        }
        ssize_t cnt = read(m_fd, buff, sizeof(buff));
        if (cnt == -1)
        {
            printf("read error 3: %s\n", strerror(errno));
            return false;
        }
        buffedcharsize = cnt;
        fileend -= cnt;
        return true;
    }
private:
    int m_fd;
    int fileend;
    char buff[50*1024]; // 50k buff
    int buffedcharsize;
    int curpos;
};

Logreader::Logreader(const char *path) : m_path(path), m_fd(-1)
{
}

Logreader::~Logreader()
{
    if (m_fd >= 0)
    {
        close(m_fd);
    }
}

bool Logreader::init()
{
    m_fd = open(m_path, O_RDONLY);
    // 
    if (m_fd == -1)
    {
        printf("open log file failed: %s\n", strerror(errno));
        return false;
    }
    else
        return true;
}

int32_t Logreader::getLast(int size, std::string *d)
{
    if (m_fd == -1)
        return 0;
    //off_t offsetend = lseek(m_fd, 0, SEEK_END);
    ReverseFileBuffer fb(m_fd);
    // off_t filesize = offsetend;
    //char buff[50 * 1024]; // 50k
    //size_t datatoread = 0;
    while (size > 0)
    {
        char c;
        bool haschar = fb.getChar(&c);
        if (!haschar)
            break;
        d->push_back(c);
        if (c == '\n')
        {
            size--;
        }
    }
    std::reverse(d->begin(), d->end());
    return d->size();
}

int Logreader::getFirstNewlinepos(char *buf, int buflen)
{
    int i = 0;
    for (; i < buflen; ++i)
    {
        if (buf[i] == '\n')
            break;
    }
    if (i == buflen)
        return -1;
    return i;
}

int Logreader::getlines(char *buff,
        int buflen,
        int maxlines,
        std::string *d)
{
    int linecnt = 0;
    int i = 0;
    for (; i <buflen; ++i)
    {
        if (buff[i] == '\n') // 
            linecnt++;
        d->push_back(buff[i]);
        //if (linecnt == maxlines) // ok all 
        //    break;
    }

    return linecnt;
}

void Logreader::removelinesFromString(int howmany, std::string *d)
{
    const char *p = d->data();
    unsigned int i = 0;
    for (; i < d->size(); ++i) 
    {
        if (p[i] == '\n')
            howmany--;
        if (howmany <= 0)
        {
            // skip last '\n'
            ++i;
            break;
        }
    }
    // this is remove all
    if (i == d->size())
    {
        d->clear();
    }
    else
    {
        std::string s(p+i, d->size() - i);
        d->swap(s);
    }
}
