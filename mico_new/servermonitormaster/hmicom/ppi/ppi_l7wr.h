/******************************************************************************
** Copyright (c) 2003-2005 by SHENZHEN CO-TRUST AUTOMATION TECH.
** File name   : PPI_L7Wr.H
** Description : td200 local configration functions
** Created by  : hpch
** Date        : 2006-03-11
** Version     : 1.0
** History     :
******************************************************************************/
#include "../ct_def.h"
#ifndef _PPI_L7Wr_H_
#define _PPI_L7Wr_H_

//any pointer 的数据类型
#define PPI_AP_TYPE_BOOL      0x01
#define PPI_AP_TYPE_BYTE      0x02
#define PPI_AP_TYPE_WORD      0x04
#define PPI_AP_TYPE_DWORD     0x06
#define PPI_AP_TYPE_C_IEC     0x1E
#define PPI_AP_TYPE_T_IEC     0x1F
#define PPI_AP_TYPE_HC_IEC    0x20
#define PPI_AP_TYPE_C	      0x1C
#define PPI_AP_TYPE_T         0x1D

//any pointer 的内存类型
#define PPI_AP_AREA_SLOP_OVER 0x00
#define PPI_AP_AREA_SYS		  0x03
#define PPI_AP_AREA_S         0x04
#define PPI_AP_AREA_SM        0x05
#define PPI_AP_AREA_AI        0x06
#define PPI_AP_AREA_AQ        0x07
#define PPI_AP_AREA_C_IEC     0x1E
#define PPI_AP_AREA_T_IEC     0x1F
#define PPI_AP_AREA_HC_IEC    0x20
#define PPI_AP_AREA_I         0x81
#define PPI_AP_AREA_Q         0x82
#define PPI_AP_AREA_M         0x83
#define PPI_AP_AREA_V         0x84
#define PPI_AP_AREA_DB_H      0x85
#define PPI_AP_AREA_DB PPI_AP_AREA_V //300 DB区域
#define PPI_AP_AREA_T         0x1D   //300 定时器区域
#define PPI_AP_AREA_C         0x1C   //300 计数器区域
#define PPI_AP_AREA_P         0x80   //300 PI PQ 区域

//定义PPI读写操作的 Access Result
#define ACC_RSLT_NO_ERR           0xFF       //没有错误
#define ACC_RSLT_HARDWARE_FAULT   0x01       //硬件错误
#define ACC_RSLT_ILLEGAL_OBJECT   0x03       //非法的对象
#define ACC_RSLT_INVALID_ADDR     0x05       //无效的地址
#define ACC_RSLT_DATA_TYPE_ERR    0x06       //数据类型错误
//#define ACC_RSLT_LENGTH_ERR       0x0A       //长度错误
#define ACC_RSLT_OBJ_NOT_EXIST    0x0A       //对象不存在

//定义PPI读写操作的 Data Type
#define D_TYPE_ERROR              0x00       //有错误
#define D_TYPE_BIT                0x03       //是位访问
#define D_TYPE_BYTES              0x04       //byte，word，dword等其他非
#define D_TYPE_LONG				  0x09

//#define OS_MEM INT8U
#define L7_MSG_TYPE INT32U
#define MAX_PPI_PDU_LEN 240
#define MAX_LONG_BUF_LEN 260
#define MAX_SHORT_BUF_LEN 260
#define LongBufMem    260
#define L2_MSG_CREAT_LINK 0
#define ShortBufMem 260

/*enum
{
    PPI_NOERR,
    PPI_ERR_TIMEROUT,
    COM_OPENERR,
    COM_OPENSUCCESS,
    PPI_BUILD_SD2_ERR,
    PPI_CONNECT_OK
};*/
#pragma pack(push,1)
typedef struct
{
    INT8U  Syntax_ID;                       //固定是0x10
    INT8U  Type;                            //数据类型
    INT16U  NumElements;                     //多少个数据单元
    INT16U  Subarea;                         //0x0001: V; 0x0000: 其他
    INT8U   Area;                            //内存类型，参见定义
    INT8U  Offset[3];                      //偏移量：对于Bit,Byte,Word,Dword都是表示到位，其他表示序列号。
} ANY_POINTER;

typedef struct
{
    INT8U  VarSpec;                         //Variable Spec
    INT8U  AddrLen;                         //V_ADDR_LG
    ANY_POINTER AP;                        //any pointer
} VAR_ADDR_STRU;

typedef struct
{
    INT8U  AccRslt;                          //access result
    INT8U  DataType;                         //Data Type
    INT16U  Length;                           //
    INT8U  Var[2];                           //实际上可能不至2个，只是为了容易处理
} VAR_VALUE_STRU;


typedef struct
{
    INT8U  ProtoID;              	//协议标识，固定0x32
    INT8U  Rosctr;               	//远程操作控制，1：请求，2：应答（没有参数和数据区），3：
    INT16U  RedID;                	//冗余标识，固定0x0000
    INT16U  PduRef;               	//PDU参考，应答必须和请求一样。
    INT16U  ParLen;               	//参数区长度
    INT16U  DatLen;               	//数据区长度
} PPI_PDU_REQ_HEADER;

typedef struct
{
    INT8U  VarSpec;               //固定0x12
    INT8U  AddrLen;               //固定0x0A（就是AnyPointer的长度）
    ANY_POINTER VarAddr;          //AnyPointer
} PPI_NETR_REQ_VAR_STRU;

typedef struct
{
    INT8U  ServiceID;             //服务码
    INT8U  VarNum;                //变量的个数（最多8个）
    PPI_NETR_REQ_VAR_STRU Vars[4]; //变量的内容，可能超过4个，这里只是方便处理定义4个。
} PPI_PDU_REQ_PARA_MULT_READ;         //可以同时读取多个变量的NETR。

typedef struct
{
    PPI_PDU_REQ_HEADER Header;     //NETR(支持多个变量) 请求消息的结构
    PPI_PDU_REQ_PARA_MULT_READ  Para;
} PPI_PDU_REQ_MULT_READ;



typedef struct
{
    INT8U  Reserved;               	//固定0x00
    INT8U  DataType;               	//数据的类型
    INT16U Length;			//数据长度
    INT8U Dat[4];				//数据内容
} PPI_NETW_REQ_DAT_STRU;

typedef struct
{
    INT8U  ProtoID;              	//同上
    INT8U  Rosctr;               	//同上
    INT16U  RedID;                	//同上
    INT16U  PduRef;               	//同上
    INT16U  ParLen;               	//同上
    INT16U  DatLen;               	//同上
    INT8U  ErrCls;               	//错误类别
    INT8U  ErrCod;               	//错误码
} PPI_PDU_RSP_HEADER;

typedef struct
{
    INT8U  ServiceID;             //参考上面
    INT8U  VarNum;                //参考上面
} PPI_PDU_RSP_PARA_READ;
/*
typedef struct
{
    OS_MEM *MsgMem;                      	// 消息包使用的缓冲类型
    L7_MSG_TYPE MsgType;                 	// 消息的类型
    INT8U *pTbl;                         		// NetR/NetW 使用的Table指针，层2不需要处理，只回填
    INT8U Port;                          		// 消息使用的端口
    INT8U Service;                       		// 消息的层7服务类型，层2不需要处理，只回填
    INT8U Err;                           		// 应答消息是否有错误
    INT8U PduLen;                        		// PDU 的长度
    INT8U pPdu[240];	     	// PDU 的内容
    INT8U SSAP;                          		//MPI从站使用，层2给层7的参数，层7需要回填该参数
    INT8U Saddr;						//为一个主站可以连接多个从站而加的
} L7_MSG_STRU;
*/

#pragma pack(pop)
#if 1
//#pragma pack(push,1)
typedef struct
{
    OS_MEM MsgMem;                      	// 消息包使用的缓冲类型
    L7_MSG_TYPE MsgType;                 	// 消息的类型
    INT32U pTbl;                         		// NetR/NetW 使用的Table指针，层2不需要处理，只回填
    INT8U Port;                          		// 消息使用的端口
    INT8U Service;                       		// 消息的层7服务类型，层2不需要处理，只回填
    INT8U Err;                           		// 应答消息是否有错误
    INT8U PduLen;                        		// PDU 的长度
    INT8U pPdu[MAX_PPI_PDU_LEN];	     	// PDU 的内容
    INT8U SSAP;                          		//MPI从站使用，层2给层7的参数，层7需要回填该参数
    INT8U Saddr;						//为一个主站可以连接多个从站而加的
} L7_MSG_STRU;

typedef struct {
    INT8U IfMaster;			// 是否把层2设置为主站模式
    INT8U GapFactor;		// GAP管理的间隔(多少次令牌持有)
    INT8U RetryCount;		// 消息的最大重发次数
    INT8U Hsa;				// 最高站地址
    INT8U Addr;				// 本站的地址
    INT32U BaudRate;        // 0表示不改变波特率，其他表示设定的波特率
    INT8U DA;               //CPU地址，TD200用到，每次层7发送控制命令时填该参数用于建立MPI连接
    INT8U SetProtol;		// 设置协议类型
}L2_FMA_PARA_STRU;

typedef struct
{
    OS_MEM MsgMem;			// 消息使用的缓冲类型，长消息还是短消息
    INT8U Port;					// 操作的端口
    INT8U Saddr;					// 此消息对应的从站地址,为一个主站可以连接多个从站而加
    INT8U Frome;					// 用此标志这条消息是否来自层7，来自层7时其值为0xAA表示是系统消息
    // 0xA1表示来自层1  ;0xA2表示来自层2  ;0xAB表示来自层层二时钟中断；0xAA表示来自层7
    /*L2_MSG_TYPE*/ INT32U MsgType;		// 消息的类型
    INT8U SSAP;    				//MPI从站使用，由层7回填
    union
    {
        /*L1_DATA_IND_STRU*/
        INT8U L1DataInd;		// 当层1收到总线消息时，使用该结构送给层2
        /*L1_SEND_CON_STRU*/
        INT8U L1SendCon;	// 层1把消息发出去后，使用该结构通知层2
        /*L7_DATA_REQ_STRU*/
        INT8U L7Data;		// 层7应答层2的SD2请求时，使用该结构
        /*L2_FMA_PARA_STRU*/
        //INT8U L2FmaPara;		// 层7的PPI管理消息，例如复位PPI，改变波特率等
        L2_FMA_PARA_STRU L2FmaPara;
        INT8U Data[2];					// 便于数据处理，无意义
    } unit;
} L2_MSG_STRU;  //这个结构体单纯为了能编译通过  ppi用不到

//层7发给层2的消息结构
typedef struct
{
    OS_MEM MsgMem;				// 消息包使用的缓冲类型
    INT32U pTbl;            				// NetR/NetW 使用的Table指针，层2不需要处理，只回填
    INT8U Port;              				// 消息使用的端口
    //	INT8U Saddr;						//此消息对应的从站地址
    //	INT8U Frome;						// 用此标志这条消息是否来自层7
    INT8U Da;                				// 消息的目的站地址
    INT8U Service;       				// 消息的层7服务类型，层2不需要处理，只回填
    INT8U PduLen;	   				    // PDU 的长度
    INT8U pPdu[MAX_PPI_PDU_LEN];		// PDU 的内容
} L2_MASTER_MSG_STRU;
//#pragma pack(pop)
#endif
#if 0
//#pragma pack(pop)
typedef struct
{
    OS_MEM MsgMem;                      	// 消息包使用的缓冲类型
    L7_MSG_TYPE MsgType;                 	// 消息的类型
    INT32U pTbl;  // NetR/NetW 使用的Table指针，层2不需要处理，只回填
    INT8U Port;                          		// 消息使用的端口
    INT8U Service; // 消息的层7服务类型，层2不需要处理，只回填
    INT8U Err;                           		// 应答消息是否有错误
    INT8U PduLen;                        		// PDU 的长度
    INT8U pPdu[MAX_PPI_PDU_LEN];	     	// PDU 的内容
    INT8U SSAP; //MPI从站使用，层2给层7的参数，层7需要回填该参数
    INT8U Saddr;			//为一个主站可以连接多个从站而加的
} L7_MSG_STRU;

typedef struct {
    INT8U IfMaster;			// 是否把层2设置为主站模式
    INT8U GapFactor;		// GAP管理的间隔(多少次令牌持有)
    INT8U RetryCount;		// 消息的最大重发次数
    INT8U Hsa;				// 最高站地址
    INT8U Addr;				// 本站的地址
   // INT8U tmp[3];			//填充用，无实际意义
    INT32U BaudRate;        // 0表示不改变波特率，其他表示设定的波特率
    INT8U DA;               //送控制命令时填该参数用于建立MPI连接
    INT8U SetProtol;		// 设置协议类型
}L2_FMA_PARA_STRU;

typedef struct
{
    OS_MEM MsgMem;// 消息使用的缓冲类型，长消息还是短消息
    INT8U Port;					// 操作的端口
    INT8U Saddr;	// 此消息对应的从站地址,为一个主站可以连接多个从站而加
    INT8U Frome;	// 用此标志这条消息是否来自层7，来自层7时其值为0xAA表示是系统消息
    // 0xA1表示来自层1  ;0xA2表示来自层2  ;0xAB表示来自层层二时钟中断；0xAA表示来自层7
    INT8U tmp	;		//填充用，无实际意义
    INT32U MsgType;		// 消息的类型
    INT8U SSAP;    				//MPI从站使用，由层7回填
    INT8U tmpa[3]	;		//填充用，无实际意义
    union
    {
        /*L1_DATA_IND_STRU*/
        INT8U L1DataInd;	// 当层1收到总线消息时，使用该结构送给层2
        /*L1_SEND_CON_STRU*/
        INT8U L1SendCon;	// 层1把消息发出去后，使用该结构通知层2
        /*L7_DATA_REQ_STRU*/
        INT8U L7Data;		// 层7应答层2的SD2请求时，使用该结构

        L2_FMA_PARA_STRU L2FmaPara;
        INT8U Data[2];		// 便于数据处理，无意义
    } unit;
} L2_MSG_STRU;  //这个结构体单纯为了能编译通过  ppi用不到

//层7发给层2的消息结构
typedef struct
{
    OS_MEM MsgMem;				// 消息包使用的缓冲类型
    INT32U pTbl;		// NetR/NetW 使用的Table指针，层2不需要处理，只回填
    INT8U Port;              				// 消息使用的端口

    INT8U Da;                				// 消息的目的站地址
    INT8U Service;       				// 消息的层7服务类型，层2不需要处理，只回填
    INT8U PduLen;	   				    // PDU 的长度
    INT8U pPdu[MAX_PPI_PDU_LEN];		// PDU 的内容
} L2_MASTER_MSG_STRU;
#pragma pack(pop)
#endif
#endif
