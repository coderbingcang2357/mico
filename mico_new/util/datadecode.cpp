#include "datadecode.h"

bool Util2::readUint8(char **buf, int *len, quint8 *v)
{
    if (*len < 1)
        return false;
    *v = quint8(**buf);
    (*buf)++;
    (*len)--;
    return true;
}

bool Util2::readUint16(char **buf, int *len, quint16 *v)
{
    if ((*len) < 2)
        return false;
    *v = quint16(*((quint16*)(*buf)));
    (*buf) += 2;
    (*len) -= 2;
    return true;
}

bool Util2::readUint32(char **buf, int *len, quint32 *v)
{
    if ((*len) < 4)
        return false;
    *v = quint32(*((quint32*)(*buf)));
    (*buf) += 4;
    (*len) -= 4;
    return true;

}

bool Util2::readUint64(char **buf, int *len, quint64 *v)
{
    if ((*len) < 8)
        return false;
    *v = quint64(*((quint64*)(*buf)));
    (*buf) += 8;
    (*len) -= 8;
    return true;

}

bool Util2::readString(char **buf, int *len, QString *v)
{
    quint32 strlen = 0;
    if (!readUint32(buf, len, &strlen))
    {
        return false;
    }
    if (quint32(*len) < strlen)
        return false;
    *v = QString::fromUtf8(*buf, int(strlen));
    (*buf) += strlen;
    *len -= strlen;
    return true;
}
