#ifndef VARIABLE_H
#define VARIABLE_H
#include <QList>
#include <QVariant>
#include "util2/datadecode.h"
#include "../../util/logwriter.h"

namespace HMI {

class Variable
{
public:
    Variable();
    static bool readVar(char **buf, int *len, QList<Variable> *varlist);

    uint16_t varId;				//变量的唯一标识码
    uint16_t connectId;          //对应那个连接
    int cycleTime;          //变量的刷新周期，单位毫秒 (最大周期值24小时)
    int area;               //变量所在的区域
    int subArea;            //子区域 300 协议 DB 用到
    int addOffset;          //数据在PLC中的地址偏移量
    int bitOffset;          //如果需要位偏移量，那么这个位变量的偏移量，否则写-1
    int  len;               //变量的数据长度，字节个数, 位变量为位的个数
    INT8U dataType;         //变量的类型


    bool isConnected = false;
    QVariant value;
    bool isOnline = false;
#if 0
    int varId;		//变量的唯一标识码
    int connectId;          //对应那个连接
    int cycleTime;          //变量的刷新周期，单位毫秒 (最大周期值24小时)
    int area;               //变量所在的区域
    int subArea;            //子区域 300 协议 DB 用到
    quint64 addOffset;          //数据在PLC中的地址偏移量
    int bitOffset;          //如果需要位偏移量，那么这个位变量的偏移量，否则写-1
    int  len;               //变量的数据长度，字节个数, 位变量为位的个数
    INT8U dataType;         //变量的类型


    bool isConnected = false;
    QVariant value;
    bool isOnline = false;
#endif
};

}
#endif // VARIABLE_H
