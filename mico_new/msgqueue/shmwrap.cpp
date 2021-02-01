#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include "util/logwriter.h"
#include "shmwrap.h"

ShmWrap::ShmWrap() : m_d(0), m_len(0)
{

}

ShmWrap::~ShmWrap()
{
}

char *ShmWrap::address()
{
    assert(m_d != 0);
    return m_d;
}

int ShmWrap::length()
{
    return m_len;
}

bool ShmWrap::create(const char *shmname, int size)
{
    assert(size != 0);

    int shmFd = shm_open(shmname, O_CREAT|O_RDWR, 0777);
    assert(shmFd != -1);
    if (shmFd == -1)
    {
        ::writelog(InfoType, "Shm %s create failed: %s", shmname, strerror(errno));
        return false;
    }

    int ret = ftruncate(shmFd, size);
    assert(ret != -1);(void)ret;
    if (ret == -1)
    {
        ::writelog(InfoType, "Shm %s ftruncate failed: %s", shmname, strerror(errno));
        shm_unlink(shmname);
        close(shmFd);
        return false;
    }

    char* shmAddr = (char*) mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, shmFd, 0);
    assert(shmAddr != MAP_FAILED);
    if (shmAddr == MAP_FAILED)
    {
        ::writelog(InfoType, "Shm %s fmap failed: %s", shmname, strerror(errno));
        shm_unlink(shmname);
        close(shmFd);
        return false;
    }

    close(shmFd);
    m_name = shmname;
    m_d = shmAddr;
    m_len = size;
    return true;
}

void ShmWrap::release()
{
    shm_unlink(m_name.c_str());
    munmap(m_d, m_len);
    m_d = 0;
}
