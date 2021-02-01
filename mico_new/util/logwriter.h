#ifndef LOGWRITER__H
#define LOGWRITER__H

enum LogType { InfoType = 1,
            WarningType = 2,
            ErrorType = 3,
            };

extern const char *LogPathDatabase;
extern const char *LogPathInternal;
extern const char *LogPathLogical;
extern const char *LogPathExternal;
extern const char *LogPathNotify;
extern const char *LogPathRelay;
extern const char *scenerunner;
extern const char *mointormaster;

void logSetPrefix(const char *prefix);
void loginit(const char *logfilepath);
void writelog(const char *log, LogType evel = InfoType);
void writelog(LogType level, const char *format, ...);

#endif
