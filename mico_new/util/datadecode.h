#ifndef DATDECODE__H_
#define DATDECODE__H_
#include <QObject>
typedef quint8 INT8U;
typedef quint16 INT16U;
typedef quint32 INT32U;
namespace Util2 {
bool readUint8(char **buf, int *len, quint8 *v);
bool readUint16(char **buf, int *len, quint16 *v);
bool readUint32(char **buf, int *len, quint32 *v);
bool readUint64(char **buf, int *len, quint64 *v);
bool readString(char **buf, int *len, QString *v);
}
#endif
