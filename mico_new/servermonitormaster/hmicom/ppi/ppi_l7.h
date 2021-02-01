/******************************************************************************
** Copyright (c) 2003-2005 by SHENZHEN CO-TRUST AUTOMATION TECH.
** File name   : PPI_L7.H
** Description : td200 local configration functions
** Created by  : hpch
** Date        : 2006-03-11
** Version     : 1.0
** History     :
******************************************************************************/
#ifndef __PPI_L7_H_
#define __PPI_L7_H_

//#include "ppi_l2.h"
#include "ppi_l7wr.h"
//#include "../hs_timer/hs_timer.h"

#define MAX_PACK_LEN  16
#define MAX_FRAME_LEN 222				//每帧最多222个数据
#define MAX_FRAME_Write_LEN (MAX_FRAME_LEN - 22)		//每帧一次最多写比读少的个数

/*****************************************************************************/
/*                define CPU mode                                     */
/***************************************************************************/
#define CPU_MODE_RUN             	0x1        // CPU in run mode
#define CPU_MODE_STOP            	0x2        // CPU in stop mode
#define CPU_NOT_RESPOND          	0x3        // CPU not responding
#define CHECK_CPU_MODE_OFFSET    483
#define CHECK_FORCE_CFG_OFFSET   564

/****************************  层7的错误码  ***********************************
*******************************************************************************/

//内部使用的波特率
#define BAUD_RATE_38400   0x00
#define BAUD_RATE_19200   0x01
#define BAUD_RATE_9600    0x02
#define BAUD_RATE_4800    0x03
#define BAUD_RATE_2400    0x04
#define BAUD_RATE_1200    0x05
#define BAUD_RATE_115200  0x06
#define BAUD_RATE_57600   0x07
#define BAUD_RATE_187500  0x08             //187.5K的波特率只有PPI使用。

//网络读写的一些错误码
#define NETWR_NO_ERR        0x00
#define NETWR_TIMEROUT      0x01
#define NETWR_RCV_ERROR     0x02
#define NETWR_OFFLINE       0x03
#define NETWR_Q_OVERFLOW    0x04
#define NETWR_PROTO_ERR     0x05
#define NETWR_PARA_ERR      0x06
#define NETWR_NO_RESOURCE 0x07
#define NETWR_L7_ERR        0x08
#define NETWR_MSG_ERR       0x09

#define NETWR_PORT_ERR      0x80
#define NETWR_TBL_ERR       0x81

//Master 模式时的服务码
#define PMS_NETR                0x01
#define PMS_NETW               0x02
#define PMS_FORCE              0x03
#define PMS_UNFORCE          0x04
#define PMS_UNFORCEALL    0x05
#define PMS_WRITE_TOD     0x06
#define PMS_READ_TOD       0x07
#define PMS_START_PLC      0x08
#define PMS_STOP_PLC        0x09
#define PMS_PWD_CHECK     0x0A
#define PMS_PWD_RELEASE  0x0B

//PDU结构需要用到的定义
#define PPI_D_TYPE_BIT        0x03   //读写的数据类型是bit
#define PPI_D_TYPE_BYTE       0x04   //读写的数据类型是Byte、Word、Dword等
#define PROT_ID_S7_200        0x32    //S7 200 固定

#define ROSCTR_REQUEST        0x01    //请求用
#define ROSCTR_INFO_NO        0x02    //答复用，表示没有PARA和DATA块
#define ROSCTR_INFO_YES       0x03    //答复用，表示有PARA和DATA块其中1或者2块
#define ROSCTR_DEV_STATUS   0x07    //Virtual Device Status 请求和答复都用这个

//定义PPI协议的Service ID内容
#define PPI_SID_DEV_STATUS    0x00  //很多内容都用这个Service ID
#define PPI_SID_READ          	0x04  //PPI的读消息
#define PPI_SID_WRITE         	0x05  //PPI的写消息
#define PPI_SID_ASSN          	0xF0  //PPI的ASSOCIATION消息
#define PPI_SID_DL_START      	0x1A  //PPI的准备下载
#define PPI_SID_DL_DATA       	0x1B  //PPI的要求下载数据
#define PPI_SID_DL_END        	0x1C  //PPI的结束下载
#define PPI_SID_UL_START      	0x1D  //PPI的准备上载
#define PPI_SID_UL_DATA       	0x1E  //PPI的要求上载数据
#define PPI_SID_UL_END        	0x1F  //PPI的结束上载
#define PPI_SID_SERVICE_28    	0x28  //要求CPU运行等多种用途
#define PPI_SID_SERVICE_29    	0x29  //要求CPU停机等多种用途

#define PPI_CT_OMC_COMMAND    0xFA  //内部调试用途的service ID.

//
#define FORCE					0x00
#define UNFORCE					0x01
#define UNFORCEALL				0x02

/***************************  主站模式时错误定义 **************************/
#define MERR_NO_ERR            		0x00
#define MERR_FATAL_NOT_CLEAR 	0x01
#define MERR_SWITCH_INF_STOP   	0x02
#define MERR_BUFF_OVERFLOW     	0x03
#define MERR_START_PLC_ERR     	0x04
#define MERR_FORCE_ERR         		0x05
#define MERR_PWD_REQUIRE       	0x06

/*extern OS_MEM *L7_Hook_inMem;
extern OS_EVENT *L7_Hook_in;
extern OS_EVENT *L7_Hook_out;*/

extern OS_MEM *L7_Hook_inMem;
//extern OS_EVENT *L7_Hook_in;
//extern OS_EVENT *L7_Hook_out;
extern OS_MEM *L7_Hook_outMem;

/*************************** 整个网络层的状态 **************************/
typedef enum
{
    Net_NUsed_State,
    Net_Initing_State,
    Net_Close_State,
    Net_Closeing_State,
    Net_Starting_State,
    Net_Restart_State,
    Net_Restarting_State,
    Net_Norm_State,
    Net_Norming_State
} Net_State_CSRN;
#pragma pack(push, 1)
/***************************  NetW 读多个变量时的数据结构 ************************/
typedef struct
{
    INT8U D_type;             //D_TYPE_BIT,D_TYPE_BYTES
    INT8U* pTbl;
    INT8U  area;
    INT16U offset;
    INT16U len;			//如果是Bit，这里存放Bit offset.
} NETW_TBL_STRU;

/***************************  NetR 读多个变量时的数据结构 ************************/
typedef struct
{
    INT8U* pTbl;
    INT8U  area;
    INT16U offset;
    INT16U len;
    INT8U err;
} NETR_TBL_STRU;

/***************************  MASTER 模式的返回数据结构 **************************/
typedef struct
{
    INT8U Status;
    INT8U err;
} L7_MASTER_RSP;

/************************ 作为主站时，FORCE  请求的数据  结构**********************/
typedef struct
{
    INT8U Rev1;			//不祥，固定 0xff.
    INT8U  Rev2;			//不祥，固定 0x09.
    INT16U  Len;	         	//后面数据的长度
    INT8U  Cmd;             	//00 是force, 01 是 unforce, 02 是 unforce all.
    INT8U  Num;             	//anypointer 变量的数量，固定未1
    ANY_POINTER VarAddr;	//AnyPointer
    INT8U Val[4];           	//存放值
} FORCE_REQ_DATA_STRU; //force, unforce, unforce all 都用同样的结构


/************************  TOD 返回的DATA 结构**********************/
typedef struct
{
    INT8U  AccRslt;              //
    INT8U  DType;              //
    INT16U  Length;
    INT16U  ClkStatus;
//	RTC_BCD_STRU Rtc;
} TOD_RSP_DATA_STRU;

/************************  Virtual Device Status 返回的PARA 结构**********************/
typedef struct
{
    INT8U  ServiceID;              //
    INT8U  NoVar;              //
    INT8U  VarSpc;
    INT8U  VAddrLg;
    INT8U  SynID;
    INT8U  Class;
    INT8U  ID1;
    INT8U  ID2;
    INT8U  SegID;
    INT8U  MoreSeg;
    INT8U  ErrCls;
    INT8U  ErrCod;
} VDS_RSP_PARA_STRU;

/************************  Virtual Device Status 返回的DATA 结构**********************/
typedef struct
{
    INT8U  AccRslt;              //
    INT8U  DType;              //
    INT16U  Length;
    INT16U  NumEvent;
} VDS_RSP_DATA_STRU;

/***************************  表示不同的BLOCK头部 **************************/
typedef enum
{
    BH_PROGRAM = 0,                 //
    BH_DATA,
    BH_SYSTEM,
    BH_SYSTEM_P,
    BH_NULL
} BLOCK_HEADER;

/***************************  SERVICE 0x28 的不同子任务 **************************/
typedef enum
{
    SID_28_CPU_RUN  = 0,        	//让CPU运行
    SID_28_SYS_BLK_INSE,        	//把SystemBlock编译，并插入EPROM
    SID_28_DAT_BLK_INSE,        	//把DataBlock编译，并插入EPROM
    SID_28_PROG_BLK_INSE,       	//把ProgramBlock编译，并插入EPROM
    SID_28_SYS_BLK_DELE,        	//删除SystemBlock内存拷贝
    SID_28_DAT_BLK_DELE,        	//删除SystemBlock内存拷贝
    SID_28_PROG_BLK_DELE,       	//删除SystemBlock内存拷贝
    SID_28_DELE_MEM_CARD,       	//删除
    SID_28_NULL                 		//无效的ID
} SID_28_SUB_SERVICE;

/***************************  SERVICE 0x00 的不同子任务 **************************/
typedef enum
{
    SID_00_PLC_EVENT  = 0,        // PLC的事件表
    SID_00_READ_RTC,
    SID_00_READ_RTC_NEW,
    SID_00_SET_RTC_NEW,
    SID_00_STOP_OUTPUT,
    SID_00_RESET_SCAN_TIME,
    SID_00_READ_BLOCK_HEADER,
    SID_00_SET_PROG_MINITOR,
    SID_00_PROG_STATUS,
    //SID_00_READ_PROG_HEADER,
    //SID_00_READ_DATA_HEADER,
    //SID_00_READ_SYS_HEADER,
    //SID_00_READ_SYS_HEADER_P,
    SID_00_SET_OUTPUT_TABLE,
    SID_00_USE_OUTPUT_TABLE,
    SID_00_USE_LAST_STATUS,
    SID_00_READ_BLK_COPY,         //how many copy each block
    SID_00_READ_BLK_INFO,
    SID_00_PWD_CHECK,
    SID_00_PWD_EXPIRE,
    SID_00_SET_SCAN_CYCLE,    	//指定系统扫描的次数
    SID_00_FORCE_CMD,    		//指定系统扫描的次数

    SID_00_UNKNOWN_1,
    SID_00_UNKNOWN_2,
//	SID_00_UNKNOWN_3,
//	SID_00_UNKNOWN_4,
//	SID_00_UNKNOWN_5,
//	SID_00_UNKNOWN_6,
    SID_00_NULL                 		//无效的ID
} SID_00_SUB_SERVICE;


/***************************  NETW/NETR的控制块 **************************/
typedef struct
{
    INT8U  Status;         		//请求的状态。
    INT8U  WR;             		//NETR 还是 NETW。
    INT16U  PduRef;         		//PDU 参考，区别不同的L7请求。
    INT8U  DA;             			//目的地址
    INT8U *pData;          		//指向TABEL的指针(STL执行器要填写)
    ANY_POINTER AP;       		//目的地址的AnyPointer指针(STL执行器要填写)
} NET_WR_REQ_CTRL;

/*******************************  SD2-REQ 的头部 *****************************/
/*typedef struct
{
    INT8U  ProtoID;              	//协议标识，固定0x32
    INT8U  Rosctr;               	//远程操作控制，1：请求，2：应答（没有参数和数据区），3：
    INT16U  RedID;                	//冗余标识，固定0x0000
    INT16U  PduRef;               	//PDU参考，应答必须和请求一样。
    INT16U  ParLen;               	//参数区长度
    INT16U  DatLen;               	//数据区长度
} PPI_PDU_REQ_HEADER;*/

/*typedef struct
{
    INT8U  ProtoID;              	//同上
    INT8U  Rosctr;               	//同上
    INT16U  RedID;                	//同上
    INT16U  PduRef;               	//同上
    INT16U  ParLen;               	//同上
    INT16U  DatLen;               	//同上
    INT8U  ErrCls;               	//错误类别
    INT8U  ErrCod;               	//错误码
} PPI_PDU_RSP_HEADER;*/

typedef struct
{
    INT8U  ServiceID;             	//服务码
    INT8U  VarNum;                	//变量的个数（在我们的系统，这里固定1）
    INT8U  VarSpec;               	//固定0x12
    INT8U  AddrLen;               	//固定0x0A（就是AnyPointer的长度）
    ANY_POINTER VarAddr;		//AnyPointer
} PPI_PDU_REQ_PARA_READ;	//NETW/NETR都只用到一个变量地址。

/*typedef struct
{
    INT8U  VarSpec;               //固定0x12
    INT8U  AddrLen;               //固定0x0A（就是AnyPointer的长度）
    ANY_POINTER VarAddr;          //AnyPointer
} PPI_NETR_REQ_VAR_STRU;*/

/*typedef struct
{
    INT8U  Reserved;               	//固定0x00
    INT8U  DataType;               	//数据的类型
    INT16U Length;			//数据长度
    INT8U Dat[4];				//数据内容
} PPI_NETW_REQ_DAT_STRU;*/


/*typedef struct
{
    INT8U  ServiceID;             //服务码
    INT8U  VarNum;                //变量的个数（最多8个）
    PPI_NETR_REQ_VAR_STRU Vars[4]; //变量的内容，可能超过4个，这里只是方便处理定义4个。
} PPI_PDU_REQ_PARA_MULT_READ;  */       //可以同时读取多个变量的NETR。

/*typedef struct
{
    INT8U  ServiceID;             //参考上面
    INT8U  VarNum;                //参考上面
} PPI_PDU_RSP_PARA_READ;*/

typedef struct
{
    INT8U  Result;  			//参考上面
    INT8U  DataType;			//参考上面
    INT16U  Len; 				//位访问：1，其他：一共多少位。
    INT8U  Value[4];			//实际可能不止2个，这里定义一个数组，方便处理。
} PPI_PDU_RSP_DATA_READ;

typedef struct
{
    PPI_PDU_REQ_HEADER     Header;     //NETR 请求消息的结构
    PPI_PDU_REQ_PARA_READ  Para;
} PPI_PDU_REQ_READ;

/*typedef struct
{
    PPI_PDU_REQ_HEADER     Header;     //NETR(支持多个变量) 请求消息的结构
    PPI_PDU_REQ_PARA_MULT_READ  Para;
} PPI_PDU_REQ_MULT_READ;*/

typedef struct               //NETR 应答消息的结构
{
    PPI_PDU_RSP_HEADER     Header;
    PPI_PDU_RSP_PARA_READ  Para;
    PPI_PDU_RSP_DATA_READ  Data;
} PPI_PDU_RSP_READ;

typedef struct               //NETW 请求消息的结构
{
    PPI_PDU_REQ_HEADER     Header;
    PPI_PDU_REQ_PARA_READ  Para;	   	//和NETR的结构是一样的
    PPI_PDU_RSP_DATA_READ  Data;       	//和NETR应答的结构是一样的
} PPI_PDU_REQ_WRITE;

typedef struct               //NETW 应答消息的结构
{
    PPI_PDU_RSP_HEADER     Header;
    PPI_PDU_RSP_PARA_READ  Para;	   //和NETR的结构是一样的
    INT8U                   Data[2];    //存放每个AnyPointer变量的结果，实际上只有一个。
} PPI_PDU_RSP_WRITE;

typedef struct               //password check struct
{
    PPI_PDU_REQ_HEADER     Header;
    INT8U  Para[8];
    INT8U  Data[12];
} PPI_PDU_REQ_PWD_CHECK;

typedef struct               //password check struct
{
    PPI_PDU_REQ_HEADER     Header;
    INT8U  Para[8];
    INT8U  Data[4];
} PPI_PDU_REQ_PWD_RELEASE;

typedef struct
{
    INT16U Clock_Status;
    INT8U  Year;
    INT8U  Month;
    INT8U  Day;
    INT8U  Hour;
    INT8U  Minute;
    INT8U  Second;
    INT16U MS_Weekday;					//前24位是毫秒，后4位表示星期
} RTC_TIME;

typedef struct
{
    INT16U UNKNOW;
    INT16U Clock_Status;
    INT8U  Year;
    INT8U  Month;
    INT8U  Day;
    INT8U  Hour;
    INT8U  Minute;
    INT8U  Second;
    INT16U MS_Weekday;					//前24位是毫秒，后4位表示星期
    INT16U U;
    INT16U N;
    INT16U k;
    INT16U n;
    INT16U o;
    INT16U w;
} RTC_TIME_LIKE_MCOWIN;

typedef struct
{
    INT8U ServiceID;
    INT8U VarNum;
    INT8U VarSpec;
    INT8U VAddrLen;
    INT8U Syn_ID;
    INT8U Class;
    INT8U ID1;
    INT8U ID2;
} PPI_PDU_RTC_REQ_PARA;

typedef struct
{
    INT8U ServiceID;
    INT8U VarNum;
    INT8U VarSpec;
    INT8U VAddrLen;
    INT8U Syn_ID;
    INT8U Class;
    INT8U ID1;
    INT8U ID2;
    INT16U UNKNOW;
} PPI_PDU_RTC_REQ_PARA_LIKE_MCOWIN;


typedef struct
{
    INT8U ServiceID;
    INT8U VarNum;
    INT8U VarSpec;
    INT8U VAddrLen;
    INT8U Syn_ID;
    INT8U Class;
    INT8U ID1;
    INT8U ID2;
    INT8U Seg_ID;
    INT8U More_Seg;
    INT8U Err_Cls;
    INT8U Err_Cod;
} PPI_PDU_RTC_RSP_PARA;

typedef struct
{
    INT8U  Acc_Rslt;
    INT8U  D_Type;
    INT16U Length;
} PPI_PDU_RTC_DATA;

typedef struct
{
    PPI_PDU_REQ_HEADER    Header;
    PPI_PDU_RTC_REQ_PARA  Para;
    PPI_PDU_RTC_DATA  Data;
} PPI_PDU_RTC_REQ_READ;

typedef struct
{
    PPI_PDU_REQ_HEADER    Header;
    PPI_PDU_RTC_REQ_PARA  Para;
    PPI_PDU_RTC_DATA  Data;
    RTC_TIME			  RTC_Time;
} PPI_PDU_RTC_REQ_WRITE;

typedef struct
{
    PPI_PDU_REQ_HEADER    				Header;
    PPI_PDU_RTC_REQ_PARA_LIKE_MCOWIN  	Para;
    PPI_PDU_RTC_DATA  					Data;
    RTC_TIME_LIKE_MCOWIN			 	RTC_Time;
} PPI_PDU_RTC_REQ_WRITE_LIKE_MCOWIN;

typedef struct
{
    INT8U  Acc_Rslt;
    INT8U  D_Type;
    INT16U Length;
    INT8U  Err;
} PPI_PDU_FORCE_RSP_DATA;

typedef struct
{
    INT8U ServiceID;
    INT8U VarNum;
    INT8U VarSpec;
    INT8U VAddrLen;
    INT8U Syn_ID;
    INT8U Class;
    INT8U ID1;
    INT8U ID2;
    INT32U UnKnow;
} PPI_PDU_FORCEIQ_PARA;

typedef struct
{
    INT8U  Acc_Rslt;
    INT8U  D_Type;
    INT16U Length;
    INT8U  ConBit;
    INT8U VarNum;
    ANY_POINTER VarAddr;
    INT16U Value;
} PPI_PDU_FORCEIQ_DATA;

typedef struct
{
    INT8U  Acc_Rslt;
    INT8U  D_Type;
    INT16U Length;
    INT8U  ConBit;
    INT8U VarNum;
    ANY_POINTER VarAddr;
} PPI_PDU_UNFORCEIQ_DATA;

typedef struct
{
    INT8U  Acc_Rslt;
    INT8U  D_Type;
    INT16U Length;
    INT8U  ConBit;
    INT8U VarNum;
} PPI_PDU_UNFORCEIQALL_DATA;

typedef struct
{
    PPI_PDU_REQ_HEADER    		Header;
    PPI_PDU_FORCEIQ_PARA		Para;
    PPI_PDU_FORCEIQ_DATA		Data;
} PPI_PDU_FORCEIQ;

typedef struct
{
    PPI_PDU_REQ_HEADER    		Header;
    PPI_PDU_FORCEIQ_PARA		Para;
    PPI_PDU_UNFORCEIQ_DATA		Data;
} PPI_PDU_UNFORCEIQ;

typedef struct
{
    PPI_PDU_REQ_HEADER    		Header;
    PPI_PDU_FORCEIQ_PARA		Para;
    PPI_PDU_UNFORCEIQALL_DATA	Data;
} PPI_PDU_UNFORCEIQALL;

#pragma pack(pop)
//**********************MSG_STRUCT******************************
typedef struct
{
    INT8U  tdAddr;   				// 0~126
    INT32U baudRate;  			// 187k, 19.2k, 9.6k
    INT8U  hsa;       				// 15,31,63,126
    INT8U  gap;                     		// 1~25
    INT8U  NewAddr;				// 用于更改已经建立连接的CPU地址
} L7_LOCAL_PAR_MSG_STRU;

typedef struct
{
    INT8U * pTbl;
    INT8U area;
    INT16U offset;
    INT16U len;
} L7_NetR_MSG_STRU;

typedef struct
{
    INT8U* pTbl;
    INT8U area;
    INT16U offset;
    INT16U len;
} L7_NetW_MSG_STRU;

typedef struct
{
    INT8U value;
    INT8U area;
    INT16U offset;
    INT8U bitoffset;
} L7_NetWBit_MSG_STRU;

typedef struct
{
    RTC_TIME* time;
} L7_ReadRTC_MSG_STRU;

typedef struct
{
    RTC_TIME* time;
} L7_WriteRTC_MSG_STRU;

typedef struct
{
    INT8U blkNum;
    NETR_TBL_STRU *pNetRTbl;
    NETW_TBL_STRU *pNetWTbl;
} L7_MulNetR_MSG_STRU;

typedef struct
{
    INT8U blkNum;
    NETW_TBL_STRU *pNetWTbl;
} L7_MulNetW_MSG_STRU;

typedef struct
{
    INT8U area;
    INT8U byteoffset;
    INT8U bitOffset;
    INT8U value;
} L7_ForceIQ_MSG_STRU;

typedef struct
{
    INT8U area;
    INT8U byteoffset;
    INT8U bitOffset;
} L7_UnForceIQ_MSG_STRU;

typedef struct
{
    INT8U area;
} L7_UnForceIQALL_MSG_STRU;

typedef struct
{
    INT8U forceCfg[64];
    INT8U area;
} L7_GetForceCfg_MSG_STRU;

typedef struct
{
    INT8U* Pwd;
} L7_PwdCheck_MSG_STRU;

typedef struct
{
    INT8U area;
} L7_PwdRelease_MSG_STRU;

typedef struct
{
    INT8U CheckAddr;			//作为请求时候，这个值表示要检查的连接。作为回复时:1表示连接了，0表示还没有连接上。
} L7_CheckLink_MSG_STRU;

/**************************  NetParaStru  ***********************************/
/*typedef struct
{
    INT32U baudRate;
    INT8U localaddr;
    INT8U Hsa;
    INT8U gap;
}Net_Para_Stru;
*/
//*********************L7_Hook_in*******************
typedef enum
{
    L7_MSG_NULL_REQ = 0,			//无请求
    L7_MSG_CREAT_LINK,				//创建一个连接
    L7_MSG_LINKED,					//被请求的连接已经连上
    L7_MSG_DELET_ONELINK,			//删除一个连接
    L7_MSG_LINKED_CHECK,
    L7_LOCAL_PAR_INIT,

    L7_Net_Close,
    L7_Net_Start,
    L7_Net_Restart,

    L7_SET_SetCommBaudRate,
    L7_SET_SetTd200Addr,
    L7_SET_SetCpuAddr,
    L7_SET_SetHighestStaAddr,
    L7_SET_SetGapFactor,

    L7_CheckCheckCpuMode,

    L7_NetR,
    L7_NetW,
    L7_NetWBite,

    L7_ReadRTC,
    L7_WriteRTC,
    L7_MulNetR,
    L7_MulNetW,
    L7_ForceIQ,
    L7_UnForceIQ,
    L7_UnForceIQALL,
    L7_GetForceCfg,
    L7_PwdCheck,
    L7_PwdRelease
} PPI_L7_Hook_in_type;


typedef struct
{
    OS_MEM *MsgMem;          // 消息包使用的缓冲类型
    PPI_L7_Hook_in_type    	MSG_Type;
    INT8U Saddr;						//为一个主站可以连接多	个从站而加的
    union
    {
        L7_LOCAL_PAR_MSG_STRU LOCAL_PAR_MSG_HI_MSG;
        L7_CheckLink_MSG_STRU CheckLinkAddr;
        L7_NetR_MSG_STRU NetR_MSG_HI_MSG;
        L7_NetW_MSG_STRU NetW_MSG_HI_MSG;
        L7_NetWBit_MSG_STRU NetWBit_MSG_HI_MSG;
        L7_ReadRTC_MSG_STRU ReadRTC_MSG_HI_MSG;
        L7_WriteRTC_MSG_STRU WriteRTC_MSG_HI_MSG;
        L7_MulNetR_MSG_STRU MulNetR_MSG_HI_MSG;
        L7_MulNetW_MSG_STRU MulNetW_MSG_HI_MSG;
        L7_ForceIQ_MSG_STRU ForceIQ_MSG_HI_MSG;
        L7_UnForceIQ_MSG_STRU UnForceIQ_MSG_HI_MSG;
        L7_UnForceIQALL_MSG_STRU UnForceIQALL_MSG_HI_MSG;
        L7_GetForceCfg_MSG_STRU GetForceCfg_MSG_HI_MSG;
        L7_PwdCheck_MSG_STRU PwdCheck_MSG_HI_MSG;
        L7_PwdRelease_MSG_STRU PwdRelease_MSG_HI_MSG;
    } enum_L7_Hook_in_MSG;
} PPI_L7_Hook_in_MSGstru;

/*
//层7发给外界应用的消息结构
typedef struct {
    OS_MEM *MsgMem;            			// 消息包使用的缓冲类型
    PPI_L7_Hook_in_type MsgType;		// 消息的类型
    INT8U *pTbl;            				// NetR/NetW 使用的Table指针，层2不需要处理，只回填
    INT8U Port;                          		// 消息使用的端口
    INT8U Service;                       		// 消息的层7服务类型，层2不需要处理，只回填
    INT8S Err;                           		// 应答消息是否有错误
    INT8U PduLen;                        		// PDU 的长度
    INT8U pPdu[MAX_PPI_PDU_LEN];	     	// PDU 的内容
    INT8U SSAP;                          		//MPI从站使用，层2给层7的参数，层7需要回填该参数
    INT8U Saddr;						//为一个主站可以连接多个从站而加的
    NETR_TBL_STRU *MulpTbl;
    INT8U MulblkNum;
    union{
        L7_LOCAL_PAR_MSG_STRU LOCAL_PAR_MSG_HI_MSG;
        L7_CheckLink_MSG_STRU CheckLinkAddr;
        L7_NetR_MSG_STRU NetR_MSG_HI_MSG;
        L7_NetW_MSG_STRU NetW_MSG_HI_MSG;
        L7_NetWBit_MSG_STRU NetWBit_MSG_HI_MSG;
        L7_ReadRTC_MSG_STRU ReadRTC_MSG_HI_MSG;
        L7_WriteRTC_MSG_STRU WriteRTC_MSG_HI_MSG;
        L7_MulNetR_MSG_STRU MulNetR_MSG_HI_MSG;
        L7_MulNetW_MSG_STRU MulNetW_MSG_HI_MSG;
        L7_ForceIQ_MSG_STRU ForceIQ_MSG_HI_MSG;
        L7_UnForceIQ_MSG_STRU UnForceIQ_MSG_HI_MSG;
        L7_UnForceIQALL_MSG_STRU UnForceIQALL_MSG_HI_MSG;
        L7_GetForceCfg_MSG_STRU GetForceCfg_MSG_HI_MSG;
        L7_PwdCheck_MSG_STRU PwdCheck_MSG_HI_MSG;
        L7_PwdRelease_MSG_STRU PwdRelease_MSG_HI_MSG;
    } enum_L7_Hook_in_MSG;
}PPI_L7_Hook_out_MSGstru;*/

typedef struct
{
    PPI_L7_Hook_in_type MsgType;		// 消息的类型
    INT8U *pTbl;            				// NetR/NetW 使用的Table指针，层2不需要处理，只回填
    INT8U Port;                          		// 消息使用的端口
    INT8S Err;                           		// 应答消息是否有错误
    INT8U PduLen;                        		// PDU 的长度
    INT8U Saddr;						//为一个主站可以连接多个从站而加的
    union
    {
        L7_LOCAL_PAR_MSG_STRU LOCAL_PAR_MSG_HI_MSG;
        L7_CheckLink_MSG_STRU CheckLinkAddr;
        L7_NetR_MSG_STRU NetR_MSG_HI_MSG;
        L7_NetW_MSG_STRU NetW_MSG_HI_MSG;
        L7_NetWBit_MSG_STRU NetWBit_MSG_HI_MSG;
        L7_ReadRTC_MSG_STRU ReadRTC_MSG_HI_MSG;
        L7_WriteRTC_MSG_STRU WriteRTC_MSG_HI_MSG;
        L7_MulNetR_MSG_STRU MulNetR_MSG_HI_MSG;
        L7_MulNetW_MSG_STRU MulNetW_MSG_HI_MSG;
        L7_ForceIQ_MSG_STRU ForceIQ_MSG_HI_MSG;
        L7_UnForceIQ_MSG_STRU UnForceIQ_MSG_HI_MSG;
        L7_UnForceIQALL_MSG_STRU UnForceIQALL_MSG_HI_MSG;
        L7_GetForceCfg_MSG_STRU GetForceCfg_MSG_HI_MSG;
        L7_PwdCheck_MSG_STRU PwdCheck_MSG_HI_MSG;
        L7_PwdRelease_MSG_STRU PwdRelease_MSG_HI_MSG;
    } enum_L7_Hook_in_MSG;
} GET_NET_RSP_STRU;

typedef struct
{
    INT8U LinkOn;						//连接是否被建立(为1表示建立)
    INT8S REQNum;					//请求计数
    INT8S REQBeforNum;				//请求消息排序计数
    PPI_L7_Hook_in_type MSG_type;		//消息类型
    INT8U NET_station;					//网络繁忙状态，如果此数超过二，那么就要挂起层7一个系统时钟周期。

    L7_MulNetR_MSG_STRU MulNetR_state;//记录多变量读的信息，以便消息返回时进行确认
    L7_MulNetW_MSG_STRU MulNetW_state;

    INT16U ProcessNumByte;			//
    L7_NetR_MSG_STRU NetR_BeyongData;	//用于支持超长数据包的拆分
    L7_NetW_MSG_STRU NetW_BeyongData;	//用于支持超长数据包
} L7_MSG_Control_stru;

//int InitPPIPara(INT32U baudRate, INT8U addr, INT8U Hsa, INT8U gap,PPI_L7_Hook_in_type type);
int	Creatlink( INT8U cpuaddr );
int	Deletlink( INT8U cpuaddr );
int SetCommBaudRate( INT32U baudRate );// 设置波特率
int SetTd200Addr( INT8U addr );        		// 设置本地TD200地址
int SetCpuAddr( INT8U addr );          		// 设置对话CPU地址
int SetHighestStaAddr( INT8U addr );   	// 设置最高站地址
int SetGapFactor( INT8U gap );         		// 设置GAP FACROR

INT8U CheckCpuMode( void );                  	// 察看CPU运行状态, RUN ,STOP,或者通讯失败

//Len可以超过层2包的长度，所以需要打包，分多次发送（由PPI来管理）
//返回值：SUCCESS, FAILURE（层2和层7都可能导致失败，如果多包有一包）
//int NetR(INT8U *pTbl, INT8U area, INT16U offset, INT16U len ,INT8U Saddr);
//void PackAndPostL2MsgW( INT8U* pTbl,INT8U area,INT16U offset,INT16U Len,INT8U Saddr);
//返回值：SUCCESS, FAILURE
int ReadRTC( RTC_TIME* time , INT8U Saddr );		//读 RTC
void WriteRTC( RTC_TIME* time , INT8U Saddr );		//写 TRC

//area只允许 I/Q，byteoffset：0－15，bitOffset：0－7，value：0/1
//返回值：SUCCESS表示操作成功，FAILURE：表示操作失败
int ForceIQ( INT8U area, INT8U byteoffset, INT8U bitOffset, INT8U value );
int UnForceIQ( INT8U area, INT8U byteoffset, INT8U bitOffset );
int	UnForceIQALL( void );
//参数指针指向的内容必须长于15字节
//Icfg: 1表示force，0表示no force
//Ival：1表示force on，0表示force off 。（如果Icfg对应的位不是1，这里的值没有意义）
//Qcfg: 1表示force，0表示no force
//Qval：1表示force on，0表示force off 。（如果Qcfg对应的位不是1，这里的值没有意义）
//返回值：SUCCESS表示取回了配置，FAILURE：表示没有取回数据
//int GetForceCfg(INT8U *cfg, INT8U *val, INT8U area, INT8U byteoffset);
int GetForceCfg( INT8U forceCfg[64], INT8U area );

int PwdCheck( INT8U* Pwd );
int PwdRelease( void );


////////////////////////////////////////////////////////////////////////////////////
//下面的函数是网络层的对外接口
////////////////////////////////////////////////////////////////////////////////////

//初始化网络参数，网络只有被初始化之后才能接受其它的命令
extern void PPINetParameterInit( INT32U baudRate, INT8U addr, INT8U Hsa, INT8U gap );

//创建一个指定CPU地址的网络连接
//extern void PPINetCreatLink(INT8U Saddr);

// 删除一个指定CPU地址的网络连接
//extern void PPINetDeleteOneLink(INT8U Saddr);

//功能同PPINetCreatLink
//extern void PPINetNetStart(INT32U baudRate, INT8U addr, INT8U Hsa, INT8U gap);

//关闭整个网络层，对所有发往网络层的请求都返回网络已关闭(除非启动网络命令和初始化网络)
//extern void PPINetNetClose(void);

// 以原来的网络参数重新启动整个网络
//extern void PPINetNetRestart(void);

/*
        暂时的实现方法是
1.先用关闭网络命令（L7_Net_Close）关闭网络，
然后通过L7_LOCAL_PAR_INIT初始化网络。这种方法
可以修改现有网络的参数。（所以原来修改网
络参数的命令就少了三个：修改波特率、修改最
高站地址和修改本站地址）
*/
//extern void PPINetSetCommBaudRate(void);	//此函数暂以其它方式实现
//extern void PPINetSetLocalAddr(void);		//此函数暂以其它方式实现
//extern void PPINetSetHighestStaAddr(void);	//此函数暂以其它方式实现

//把已经连接上的线路切断，连接到另外的CPU上去
//extern void PPINetChangeLinkCPUAddr(INT8U OriginalityAddr ,INT8U NewAddr);

//设置进行GAP管理的地址间隔数
//extern void PPINetSETGapFactor(INT8U gap);

//检查连接是否已经建立
extern INT8U PPINetLinkCheck( INT8U CheckAddr );
extern INT8U PPINetSent( INT8U SentAddr );

//================================================================================
//=			上面属于网络的控制功能												=
//=			下面属于网络的读写功能												=
//================================================================================

extern void PPINetCheckCpuMode( void );
extern int PPINetNetRead( INT8U *pTbl, INT8U area, INT16U offset, INT16U len , INT8U Saddr );
extern int PPINetNetWrite( INT8U *pTbl, INT8U area, INT16U offset, INT16U len , INT8U Saddr );
extern int PPINetNetWriteBite( INT8U value, INT8U area, INT16U offset, INT8U bitoffset, INT8U Saddr );
extern int PPINetReadRTC( RTC_TIME * ptime, INT8U Saddr );
extern int PPINetWriteRTC( RTC_TIME * ptime, INT8U Saddr );
extern int PPINetMulNetRead( INT8U blkNum, NETR_TBL_STRU *pNetRTbl, INT8U Saddr );
extern int PPINetMulNetWrite( INT8U blkNum, NETW_TBL_STRU *pNetWTbl, INT8U Saddr );
extern void PPINetForceIQ( void );
extern void PPINetUnForceIQ( void );
extern void PPINetUnForceIQALL( void );
extern void PPINetGetForceCfg( void );
extern void PPINetPwdCheck( void );
extern void PPINetPwdRelease( void );

//////////////////////////////////////////////////////////////////////////////////////////////////


#endif
