#include "variable.h"
#include "util2/datadecode.h"

HMI::Variable::Variable()
{

}

bool HMI::Variable::readVar(char **buf, int *len, QList<HMI::Variable> *varlist)
{
    quint16 size = 0;
    if (!Util2::readUint16(buf, len, &size))
    {
        return false;
    }

    for (int i = 0; i < size; ++i)
    {
        Variable var;
#if 0
        if (
            !Util2::readUint32(buf, len, (quint32*)&var.varId)
                || !Util2::readUint32(buf, len, (quint32*)&var.connectId)
                || !Util2::readUint32(buf, len, (quint32*)&var.cycleTime)
                || !Util2::readUint32(buf, len, (quint32*)&var.area)
                || !Util2::readUint32(buf, len, (quint32*)&var.subArea)
                || !Util2::readUint64(buf, len, (quint64*)&var.addOffset)
                || !Util2::readUint32(buf, len, (quint32*)&var.bitOffset)
                || !Util2::readUint32(buf, len, (quint32*)&var.len)
                || !Util2::readUint8(buf, len, (quint8*)&var.dataType)
            )
#endif
		if (
            !Util2::readUint16(buf, len, (quint16*)&var.varId)
                || !Util2::readUint16(buf, len, (quint16*)&var.connectId)
                || !Util2::readUint32(buf, len, (quint32*)&var.cycleTime)
                || !Util2::readUint32(buf, len, (quint32*)&var.area)
                || !Util2::readUint32(buf, len, (quint32*)&var.subArea)
                || !Util2::readUint32(buf, len, (quint32*)&var.addOffset)
                || !Util2::readUint32(buf, len, (quint32*)&var.bitOffset)
                || !Util2::readUint32(buf, len, (quint32*)&var.len)
                || !Util2::readUint8(buf, len, (quint8*)&var.dataType)
            )
        {
            return false;
        }
		//printf("varId:%d\n", var.varId);
        varlist->push_back(var);
    }
    return true;
}
