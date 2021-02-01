#include "datadecode.h"
#include <string>

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

bool Util2::readString_end0(char **buf, int *len, QString *v)
{
#if 1
	int strlen = 0;
	char *pbuf = *buf;
	int i;
	for (i = 0; i < *len; i++, ++pbuf)
	{
		if (*pbuf == '\0')
		{ 
			break;
		}  
		strlen++;
	}
	// err
	if (i >= *len)
	{
		return 0;
	}
	// add \0
	strlen++;

	std::string tmp;
	tmp.append(*buf,strlen);
	*v = QString::fromLocal8Bit(tmp.c_str());

    //*v = QString::fromUtf8(*buf, strlen);
    (*buf) += strlen;
    *len -= strlen;
    return true;
#endif
}

bool Util2::readDouble(char **buf, int *len, qreal *v)
{
	if( *len < 8) {
		return false;
	}
	*v = qreal(* (qreal *) *buf);
	*buf += 8;
	*len -= 8;
    return true;
}
