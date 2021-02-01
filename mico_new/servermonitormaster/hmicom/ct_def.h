#ifndef _CT_DEF_H_
#define _CT_DEF_H_

typedef unsigned int    INT32U;
typedef unsigned short  INT16U;
typedef unsigned char   INT8U;

typedef signed long     INT32S; //确定没有问题？？？不是int而是long?
typedef signed short    INT16S;
typedef signed char     INT8S;

#define FALSE               0
#define TRUE                1
#define SUCCESS             0
#define FAILURE             -1
#define OS_NO_ERR           0
#define OS_ERR              1

#define MSG_WARN            2
#define MSG_INFO            3
#define MSG_ERR             4
#define PPI_HSA             126

#define OS_MEM              int
#define OS_EVENT            void*
typedef struct
{
    int OSNMsgs;
} OS_Q_DATA;

#define OS_CRITICAL_METHOD    3
#define OS_CPU_SR             unsigned long

extern void ct_assert( INT32U cond, char* file, int line );
#define CT_ASSERT(cond) ct_assert((INT32U)cond, __FILE__, __LINE__)

__inline INT32U conv_32bit( INT32U data )
{
    INT32U ret;

    ret = (( data << 24 ) & 0xFF000000 ) | (( data << 8 ) & 0xFF0000 ) | (( data >> 8 ) & 0xFF00 ) | (( data >> 24 ) & 0xFF );
    return ret;
}

__inline unsigned short conv_16bit( INT16U data )
{
    INT16U ret;

    ret = (( data << 8 ) & 0xFF00 ) | (( data >> 8 ) & 0xFF );
    return ret;
}

#endif /*_CT_DEF_H_*/
