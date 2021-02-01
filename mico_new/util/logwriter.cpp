#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <string.h>
#include <time.h>
#include "logwriter.h"

static std::string logPrefix;
static std::string logFilePath;
static bool isInit = false;

const char *LogPathDatabase = "/var/log/mico/common/common.log";
const char *LogPathInternal = "/var/log/mico/common/common.log";
const char *LogPathLogical = "/var/log/mico/common/common.log";
const char *LogPathExternal = "/var/log/mico/common/common.log";
const char *LogPathNotify = "/var/log/mico/common/common.log";
const char *LogPathRelay = "/var/log/mico/common/common.log";
const char *scenerunner = "/var/log/mico/scenerunner/scenerunner.log";
const char *mointormaster = "/var/log/mico/mointormaster/mointormaster.log";

//const char *LogPathDatabase = "/var/log/mico/databaselog";
//const char *LogPathInternal = "/var/log/mico/internallog";
//const char *LogPathLogical = "/var/log/mico/logicallog";
//const char *LogPathExternal = "/var/log/mico/externallog";
//const char *LogPathNotify = "/var/log/mico/notifylog";
//const char *LogPathRelay = "/var/log/mico/relaylog";

void gettimestr(char *buf, int sizeofbuf)
{
    time_t t = time(0);
    struct tm local_time;
    localtime_r(&t, &local_time);
    strftime(buf, sizeofbuf, "%F %T", &local_time);
}

const char *level2String(LogType level)
{
    switch (level)
    {
    case InfoType:
        return "Info    ";
    case WarningType:
        return "Warning ";
    case ErrorType:
        return "Error   ";
    default:
        return "        ";
    }
}

void writelog(const char *log, LogType level)
{
    (void)level;
    char strtime[1024];
    gettimestr(strtime, sizeof(strtime));
    printf("[%s][%s] %s : %s\n", level2String(level),
                                strtime,
                                logPrefix.c_str(),
                                log);
}

void writelog(LogType level, const char *format, ...)
{
    va_list ap;
    char log[1024];
    va_start(ap, format);
    vsnprintf(log, sizeof(log), format, ap);
    va_end(ap);

    char strtime[1024];
    gettimestr(strtime, sizeof(strtime));
    printf("[%s][%s] %s : %s\n", level2String(level),
                                strtime,
                                logPrefix.c_str(),
                                log);
}

void logSetPrefix(const char *prefix)
{
    logPrefix = std::string(prefix);
}

void loginit(const char *logfilepath)
{
    if (isInit)
        return;

    logFilePath = std::string(logfilepath);

    int fd = open(logfilepath,
                    O_RDWR | O_APPEND | O_CREAT,
                    0666);
    if (fd == -1)
    {
        syslog(LOG_ERR, "Loginit failed: %s", strerror(errno));
        printf("Loginit failed: %s\n", strerror(errno));
        exit(1);
    }
    // redirect stdout and stderr to our own fileno
    close(STDOUT_FILENO); // 1
    close(STDERR_FILENO); // 2
    int fdout = dup2(fd, 1);
    int fderr = dup2(fd, 2); // out
    if (fdout != 1 || fderr != 2)
    {
        syslog(LOG_ERR, "Log init error, unexpected fileno: %d, %d\n",
            fdout, fderr);
        close(fd);
        close(fdout);
        close(fderr);
        exit(1);
    }
    // set stdout and stderr to linebuf otherwish it will be full buffer
    setvbuf(stdout, 0, _IOLBF, 0);
    setvbuf(stderr, 0, _IOLBF, 0);

    close(fd);

    isInit = true;
}
