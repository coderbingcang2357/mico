#ifndef _MDTYPEDEF_H
#define _MDTYPEDEF_H

#ifndef INT32U
//#define INT32U     unsigned int
typedef unsigned int INT32U;
#endif
#ifndef INT16U
//#define INT16U     unsigned short
typedef unsigned short INT16U;
#endif
#ifndef INT8U
//#define INT8U      unsigned char
typedef unsigned char INT8U      ;
#endif
#ifndef INT32S
//#define INT32S     signed long
typedef signed long INT32S     ;
#endif
#ifndef INT16S
//#define INT16S     signed short
typedef signed short INT16S     ;
#endif
#ifndef INT8S
//#define INT8S      signed char
typedef signed char INT8S      ;
#endif

//#define false           -1
///#define true             0
#ifndef SUCCESS
#define SUCCESS          0
#endif

#ifndef FAILURE
#define FAILURE         -1
#endif
//#define TRUE            true
//#define FALSE           false

#ifndef NULL
#define NULL   ((void*)0)
#endif

#endif //_MDTYPEDEF_H
