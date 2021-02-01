#include <linux/init.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/irq.h>
#include <linux/utsrelease.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/spinlock.h>
#include <linux/serial_core.h>
#include <linux/miscdevice.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/serial_core.h>
#include <asm/io.h>        //__raw_writel
#include <asm/semaphore.h>
#include <asm/segment.h>
#include <asm/uaccess.h> /*for copy_to_user */
#include <linux/clk.h>
/*   RT5350		*/
#ifndef CONFIG_RALINK_RT5350
#define CONFIG_RALINK_RT5350
#endif
#include <asm/rt2880/rt_mmap.h>
#include <asm/rt2880/surfboardint.h>
#include "serial_rt2880.h"
#include "ralink_gpio.h"
/*		end RT5350*/

#include "hs_timer.h"
#include "freePort_driver.h"

#define PHYS_TO_K1(physaddr) KSEG1ADDR(physaddr) /*物理地址转虚拟地址*/

#define RT5350_UART_BASE 		(void*)KSEG1ADDR(RALINK_UART_BASE)		/*UART FULL*/
#define RT5350_UART_LITE_BASE (void*)KSEG1ADDR(RALINK_UART_LITE_BASE)/*UART LITE*/

#define pio_outw(address, value)	*((volatile uint32_t *)(address)) = cpu_to_le32(value)/*GPIO 输出*/
#define pio_inw(address)			le32_to_cpu(*(volatile u32 *)(address))					/*GPIO 读取*/
#define CT_GPIO(x) (1 << x)

#define CNET_RX_TRG 8
#define CNET_TX_TRG 1

//#define DEBUG
#ifdef DEBUG
#define DBG printk("<%s>\n",__func__)
#else
#define DBG do{}while(0)
#endif
static int hmi_FP_major;

unsigned long pclk_sys;

struct hmi_FP_dev
{
	struct cdev cdev;
};

struct hmi_FP_dev dev;

#define FP_DEVICE_NAME	 "freeport_dev"

#define RS485_SEND      0x01			// 把总线置于发送状态(在发送状态，依然可以接收)
#define RS485_IDLE      0xff			// 485的中断还是全处于关闭状态

//多协议平台最大写缓冲区
#define MAX_MULPLAT_TO_DRIVER_MSG_NUM 1
//多协议平台最大读缓冲区
#define MAX_DRIVER_TO_MULPLAT_MSG_NUM 1


static DECLARE_WAIT_QUEUE_HEAD( serial_port_wait );  /* 串口接收数据时上层用户的等待队列 */
wait_queue_head_t read_watque;

struct FpCtrlBlock
{

	INT8U platToDriverMsgNum;             //当前写消息数量
	//INT8U curPlatToDriverMsgPoint;      //当前写消息处理位置
	struct FpMsgPar platToDriverMsg/*[MAX_MULPLAT_TO_DRIVER_MSG_NUM]*/;

	INT8U driverToPlatMsgNum;            //当前读消息数量
	//INT8U curDriverToPlatMsgPoint;    //当前读消息处理位置
	struct FpMsgPar driverToPlatMsg/*[MAX_DRIVER_TO_MULPLAT_MSG_NUM]*/;

	INT8U port;
	int  baudRate;
	INT8U dataBits;
	INT8U parity;
	INT8U stopBits;

	TIMERID synTimer;
	INT8U retryCnt;
	INT8U packLen;
	INT8U maxToRcv;
	INT8U packData[sizeof( struct FpDataPacket )];
	INT8U sndedLen;

	INT8U rcvData[sizeof( struct FpDataPacket )];
	INT8U rcvedLen;

	int status;  //IDLE, CAN_SND, SNDING, SYNING, RCVING,
};

INT16U packNewRequest( INT8U slave, INT8U rw, INT8U area, INT16U addr, INT16U count,
                                     INT8U* dadaPtr, struct FpDataPacket* packet );
INT8U checkGetResponse( INT8U* dadaPtr, struct FpDataPacket* packetReq,
                        struct FpDataPacket* packetRsp, INT8U rcvedLen );

#define FP_STA_IDLE       0 //空闲状态
#define FP_STA_SYN_TO_SND 1 //准备发送同步中
#define FP_STA_SNDING     2 //正在发送
#define FP_STA_SYN_TO_RCV 3 //发送完成，切换到接收同步中
#define FP_STA_RCVING     4 //接收中 
#define FP_STA_OK         5 //完成，得到结果

struct FpCtrlBlock fpCtrlBlock;  //需要初始化


TIMERID reset485Timer;
TIMERID checkComAlive;
int ComAlivFlg = 0;
/*---------------------------------------*--------------------------------------*/
/**************************  下面的函数跟处理器相关的  **************************/
/*---------------------------------------*--------------------------------------*/

void set_CT_GPIO_output_mode(int num)
{
	unsigned long data;
#if 0	
	data = pio_inw(RALINK_REG_GPIOMODE);
	//data |= RALINK_GPIOMODE_JTAG/*(1<<6)*/;
	//data &= ~RALINK_GPIOMODE_UARTF;
	//printk("gpio data %08X\n",data);
	data |= RALINK_GPIOMODE_JTAG/*(1<<6)*/;
	data &= ~RALINK_GPIOMODE_UARTF;
	//data |= 0x14;
	pio_outw(RALINK_REG_GPIOMODE, data);
#endif	
	
	
	data = pio_inw(RALINK_REG_PIODIR);
	data |= CT_GPIO(num);
	pio_outw(RALINK_REG_PIODIR,data);
}
void set_CT_GPIO_data(int num, int val)
{
	unsigned long data;
	data = pio_inw(RALINK_REG_PIODATA);
	if( val >= 1 )
	{
		data |= CT_GPIO(num);
	}
	else
	{
		data &= ~CT_GPIO(num);
	}
	pio_outw(RALINK_REG_PIODATA,data); 
}

/******************************************************************************
** 函 数 名： uart_init
** 说    明:  初始化串口，设定波特率，奇偶校验位，数据位，停止位
** 输    入： 使用的串口端口号，数据位， 停止位， 奇偶校验位
** 返    回： void
******************************************************************************/
void uart_init( int Port, int baud, INT8U dataBits, INT8U parity , INT8U stopBits )
{
	INT8U i;
	INT32U ticks;
	INT32U baudCfg = 0;
	INT32U comCfg = 0;
	//int div;

	switch ( dataBits )
	{
	case 7:
		comCfg |= 0x2;
		break;
	case 8:
	default:
		comCfg |= 0x3;
		break;
	}
	if ( stopBits == 2 )
	{
		comCfg |= 0x4;
	}
	switch ( parity )
	{
	case 1: //奇校验
		comCfg |= 0x08;
		break;
	case 2: //偶校验
		comCfg |= 0x18;
		break;
	case 0:
	default:
		comCfg |= 0x00;
		break;
	}

	baudCfg = pclk_sys / ( 16 * baud ) /*- 1*/;   // 4舍5入

	if ( Port == 1 )
	{
		__raw_writel( comCfg, RT5350_UART_BASE + UART_LCR );//Normal,Even parity,1 stop,8 bit
		__raw_writel( 0x03,  RT5350_UART_BASE + UART_IER ); //rx=edge,tx=level,enable timeout int.,disable rx error int.,normal,interrupt or polling
		__raw_writel( 0x87,  RT5350_UART_BASE + UART_FCR ); //TX 8byte triger, Rx 8 bytes triger,clear TX/RX FIFO buf. FIFO enable
		__raw_writel( baudCfg, RT5350_UART_BASE  + UART_DLL );		
	}
	else if ( Port == 2 )
	{
		__raw_writel( comCfg, RT5350_UART_LITE_BASE + UART_LCR );//Normal,Even parity,1 stop,8 bit
		__raw_writel( 0x03,  RT5350_UART_LITE_BASE + UART_IER ); //rx=edge,tx=level,enable timeout int.,disable rx error int.,normal,interrupt or polling
		__raw_writel( 0x87, RT5350_UART_LITE_BASE + UART_FCR ); //TX 8byte triger, Rx 8 bytes triger,clear TX/RX FIFO buf. FIFO enable
		__raw_writel( baudCfg, RT5350_UART_LITE_BASE + UART_DLL );
		set_CT_GPIO_output_mode(21);
	}
	else
	{
		printk( "port is not exist \n" );
	}
	for ( i = 0; i < 100; i++ ); //初始化后的延迟
	{}
	//set_CT_GPIO_output_mode(21);
	set_CT_GPIO_output_mode(9);
	set_CT_GPIO_data(9, 1);
	ticks = 12 * 1000000 / fpCtrlBlock.baudRate;  //12位同步时间
	SetTicks_HS_timer( fpCtrlBlock.synTimer, ticks );
	Start_HS_timer( fpCtrlBlock.synTimer );
	//set_CT_GPIO_output_mode(11);
	//set_CT_GPIO_output_mode(14);
}
void reset485TimerHook(INT32U Port)
{
	printk("reset 485 chipt\n");
	set_CT_GPIO_data(9, 1);
	ComAlivFlg = 0;
}

void checkComAliveHook(INT32U Port)
{
	printk("ready reset 485 chipt\n");					
	set_CT_GPIO_data(9, 0);
	if ( Start_HS_timer( reset485Timer ) == FAILURE )
		CT_ASSERT( 0 );		
}

/******************************************************************************
** 函 数 名： ClearUartRcvFIFO
** 说    明:  清空串口发送缓冲区
** 输    入： 串口端口号
** 返    回： void
******************************************************************************/
void ClearUartRcvFIFO( INT8U Port )
{
	INT8U ch;
	INT32U tmp;
	INT32U rcvFlg = 0;
DBG;
	switch ( Port )
	{
	case 0:		
		break;
	case 1:
		rcvFlg = __raw_readl( RT5350_UART_BASE + UART_LSR ) & 0x01;
		tmp = __raw_readl( RT5350_UART_BASE + UART_FCR );
		__raw_writel( tmp | 0x02, RT5350_UART_BASE + UART_FCR );//clear RX FIFO buf.
		ch = __raw_readb( RT5350_UART_BASE + UART_RX );
		break;
	case 2:
		rcvFlg = __raw_readl( RT5350_UART_LITE_BASE + UART_LSR ) & 0x01;
		tmp = __raw_readl( RT5350_UART_LITE_BASE + UART_FCR );
		__raw_writel( tmp | 0x02, RT5350_UART_LITE_BASE + UART_FCR );//clear RX FIFO buf.
		ch = __raw_readb( RT5350_UART_LITE_BASE + UART_RX );
		break;
	default:
		CT_ASSERT( 0 );
		break;
	}
	if( rcvFlg == 0 )
	{
		if(ComAlivFlg == 0)
		{
			ComAlivFlg = 1;				
			
			if ( Start_HS_timer( checkComAlive ) == FAILURE )
				CT_ASSERT( 0 );
		}
	}
	else
	{
		if( ComAlivFlg == 1 )
		{
			ComAlivFlg = 0;
			Stop_HS_timer( checkComAlive ); 
		}
	}
}
/******************************************************************************
** 函 数 名： sndDataToUart
** 说    明:  串口发送中断产生后发送数据到串口发送缓冲区
** 输    入： void
** 返    回： 发送的字节数
******************************************************************************/
INT8U sndDataToUart( void )
{

	int i;
	INT8U FreeBuf, Left2Send = 0;
DBG ;
	if ( fpCtrlBlock.port == 1 )
	{
		if ( fpCtrlBlock.sndedLen < fpCtrlBlock.packLen )
		{
			FreeBuf = 16 - CNET_TX_TRG ;
			Left2Send = fpCtrlBlock.packLen - fpCtrlBlock.sndedLen;
			if ( Left2Send > FreeBuf )  //如果需要发送的字节比缓冲少，只发送缓冲的可用数量
				Left2Send = FreeBuf;
			for ( i = 0; i < Left2Send; i++ )
				__raw_writeb( fpCtrlBlock.packData[fpCtrlBlock.sndedLen + i], RT5350_UART_BASE + UART_TX );

			fpCtrlBlock.sndedLen += Left2Send;
			CT_ASSERT( Left2Send > 0 );
		}
	}
	else
	{
		if ( fpCtrlBlock.sndedLen < fpCtrlBlock.packLen )
		{
			FreeBuf = 16 - CNET_TX_TRG;
			Left2Send = fpCtrlBlock.packLen - fpCtrlBlock.sndedLen;
			if ( Left2Send > FreeBuf )  //如果需要发送的字节比缓冲少，只发送缓冲的可用数量
				Left2Send = FreeBuf;
			for ( i = 0; i < Left2Send; i++ )
				__raw_writeb( fpCtrlBlock.packData[fpCtrlBlock.sndedLen + i], RT5350_UART_LITE_BASE + UART_TX );

			fpCtrlBlock.sndedLen += Left2Send;
			CT_ASSERT( Left2Send > 0 );
		}
	}

	return Left2Send;
}

/******************************************************************************
** 函  数  名 ： md_port_tx_irq
** 说      明 :  串口发送中断请求处理函数
** 输      入 ： irq  中断请求标识符
**               dev_id  设备标识符
**               reg  注册列表指针
** 返      回 ： NONE
******************************************************************************/
static void md_port_tx_irq( void )
{
	INT32U ticks;

	if ( fpCtrlBlock.status == FP_STA_SNDING )
	{
		if ( fpCtrlBlock.sndedLen < fpCtrlBlock.packLen )
			sndDataToUart();       //继续发送
		else
		{                          //发送完成,同步到接收
			fpCtrlBlock.rcvedLen = 0;
			fpCtrlBlock.status = FP_STA_SYN_TO_RCV;

			ticks = /*(CNET_TX_TRG+1) * */12 * 1000000 / fpCtrlBlock.baudRate;  //12位同步时间
			//if (fpCtrlBlock.baudRate == 1200)		//参考了PLC自由口
			//	ticks -= (300 + 280);
			//else if (fpCtrlBlock.baudRate == 2400)
			//	ticks -= (0 + 300);
			//else if (fpCtrlBlock.baudRate == 4800)
			//	ticks -= (100 + 50);
			//else if (fpCtrlBlock.baudRate == 9600)
			//	ticks -= (75);

			SetTicks_HS_timer( fpCtrlBlock.synTimer, ticks );
			Start_HS_timer( fpCtrlBlock.synTimer );
		}
	}
	return ;
}

/******************************************************************************
** 函  数  名 ： md_port_rcv_irq
** 说      明 :  串口接收中断请求处理函数
** 输      入 ： irq  中断请求标识符
**               dev_id  设备标识符
**               reg  注册列表指针
** 返      回 ： NONE
******************************************************************************/
static void md_port_rcv_irq( void )
{
	unsigned long cpu_sr;
	int i;
	int ret;

	const volatile void __iomem* pURX;
	const volatile void __iomem* pLSR;
	INT8U err;
	INT32U ticks;
	//INT8U msgTail;
	struct FpMsgPar *msg;
	//printk("port_rcv_irq \n");
	if (  fpCtrlBlock.port == 1 )
	{
		pURX = RT5350_UART_BASE + UART_RX;
		pLSR = RT5350_UART_BASE + UART_LSR;
	}
	else
	{
		pURX = RT5350_UART_LITE_BASE + UART_RX;
		pLSR = RT5350_UART_LITE_BASE + UART_LSR;
	}
	
	if ( fpCtrlBlock.status == FP_STA_RCVING )
	{		
		
		for( i=0; i < CNET_RX_TRG - 1; i++ )
		{			
			if( (__raw_readl( pLSR ) & 0x01) && 
					fpCtrlBlock.rcvedLen < 255 )
			{		
				fpCtrlBlock.rcvData[fpCtrlBlock.rcvedLen++] = __raw_readl( pURX );
			}
			else
			{
				if( fpCtrlBlock.rcvedLen >= 255  )
				{
					/*接收字节太多 只能读到最后一个字节里去*/
					fpCtrlBlock.rcvData[fpCtrlBlock.rcvedLen] = __raw_readl( pURX );
				}
				break;
			}
		}

		if( (__raw_readl( pLSR ) & 0x01) == 0 )
		{
			Stop_HS_timer( fpCtrlBlock.synTimer ); //关闭超时定时器, 解析FP应答包
			msg = &fpCtrlBlock.platToDriverMsg /*+ fpCtrlBlock.curPlatToDriverMsgPoint*/;

			//err = checkGetResponse( msg->data, ( struct FpDataPacket* ) & ( fpCtrlBlock.packData ),\
			                        ( struct FpDataPacket* ) & ( fpCtrlBlock.rcvData ), fpCtrlBlock.rcvedLen );
			err = FP_NO_ERR;
			/*if ( err == FP_TIMEOUT && fpCtrlBlock.retryCnt < M_FP_RETRIES )
			{
				fpCtrlBlock.retryCnt++;
				fpCtrlBlock.status = FP_STA_SYN_TO_SND;

				ticks = 4 * 12 * 1000000 / fpCtrlBlock.baudRate;
				if ( ticks < 50000 ) //最小50ms
					ticks = 50000; //50ms后再重发
				SetTicks_HS_timer( fpCtrlBlock.synTimer, ticks );
				Start_HS_timer( fpCtrlBlock.synTimer );

				return ;
			}*/

			local_irq_save( cpu_sr );
			msg->err = err;
			//msgTail = fpCtrlBlock.curDriverToPlatMsgPoint + fpCtrlBlock.driverToPlatMsgNum; //copy to read cache
			//msgTail %= MAX_DRIVER_TO_MULPLAT_MSG_NUM;
			memcpy( &( fpCtrlBlock.driverToPlatMsg/*[msgTail]*/ ), msg , sizeof( struct FpMsgPar ) );
			/*fpCtrlBlock.curPlatToDriverMsgPoint++;
			fpCtrlBlock.curPlatToDriverMsgPoint %= MAX_MULPLAT_TO_DRIVER_MSG_NUM;
			if ( fpCtrlBlock.platToDriverMsgNum > 0 )
			{
				fpCtrlBlock.platToDriverMsgNum--;
			}
			*/
			fpCtrlBlock.driverToPlatMsgNum++;
			fpCtrlBlock.status = FP_STA_OK;
			local_irq_restore( cpu_sr );

			//同步1ms秒马上发送下一条请求
			SetTicks_HS_timer( fpCtrlBlock.synTimer, 1000 ); //3
			Start_HS_timer( fpCtrlBlock.synTimer );
		}
	}
	else
	{
		ret = __raw_readl( pLSR ) ;
		ClearUartRcvFIFO(fpCtrlBlock.port );		
	}
	return ;
}

/******************************************************************************
** 函  数  名 ： md_port_err_irq
** 说      明 :  串口接收中断请求处理函数
** 输      入 ： irq  中断请求标识符
**               dev_id  设备标识符
**               reg  注册列表指针
** 返      回 ： NONE
******************************************************************************/
static void md_port_err_irq( void )
{
	printk("md_port_err_irq\n");
	return ;
}
/******************************************************************************
** 函  数  名 ： md_uart_irq
** 说      明 :  串口接断请求处理函数
** 输      入 ： irq  中断请求标识符
**               dev_id  设备标识符
**               reg  注册列表指针
** 返      回 ： NONE
******************************************************************************/
static irqreturn_t md_uart_irq( int irq, void *dev_id )
{
	int ret = __raw_readl( RT5350_UART_BASE+ UART_IIR ) ;
	
DBG;
	//printk("ret %d\n", ret);
	if( ret & UART_IIR_RDI )
	{
		md_port_rcv_irq();
	}
	
	if( ret & UART_IIR_THRI )
	{
		md_port_tx_irq();
	}
	
	if( ret == 3 )
	{
		md_port_err_irq();
	}	
	
	return IRQ_HANDLED;
}

/******************************************************************************
** 函  数  名 ： md_uart_irq
** 说      明 :  串口接断请求处理函数
** 输      入 ： irq  中断请求标识符
**               dev_id  设备标识符
**               reg  注册列表指针
** 返      回 ： NONE
******************************************************************************/
static irqreturn_t md_uart_lite_irq( int irq, void *dev_id )
{
	int ret = __raw_readl( RT5350_UART_LITE_BASE+ UART_IIR );
DBG;
	if( ret & UART_IIR_RDI )
	{
		md_port_rcv_irq();
	}
	
	if( ret & UART_IIR_THRI )
	{
		md_port_tx_irq();
	}
	
	if( ret == 3 )
	{
		md_port_err_irq();
	}	
	return IRQ_HANDLED;
}

/******************************************************************************
** 函数名: RS485_Switch
** 说明  : RS485接口不能同时接收，要发送的时候，必须
**        把nRTS 信号设为1。接收时设为0。(设计的约定)
** 输入  : Port : 哪个端口
          Mode : 发送还是接收
** 输出  : nRTS
** 返回  : NA
** 其它  : nRTS在RS485总线中没有使用，所以用来控制接收和发送
******************************************************************************/
void RS485_Switch( INT8U Port, INT8U Mode )
{
DBG ;
//set_CT_GPIO_data(11, Mode&0x01);
//set_CT_GPIO_data(14, Mode&0x01);
//printk("mode %02X\n",Mode&0x01);
	switch ( Port )
	{
	case 0:		
		break;
	case 1:
		if ( Mode == RS485_SEND )
		{
			__raw_writel( 0x00, RT5350_UART_BASE + UART_MCR );
			__raw_writel( 0x02, RT5350_UART_BASE + UART_IER );
		}
		else
		{
			__raw_writel( 0x02, RT5350_UART_BASE + UART_MCR );
			__raw_writel( 0x01, RT5350_UART_BASE + UART_IER );			
		}
		break;
	case 2:
		if ( Mode == RS485_SEND )
		{	
			//set_CT_GPIO_data(21, 1);
			__raw_writel( 0x00, RT5350_UART_LITE_BASE + UART_MCR );
			__raw_writel( 0x02, RT5350_UART_LITE_BASE + UART_IER );
		}
		else
		{
			//set_CT_GPIO_data(21, 0);
			__raw_writel( 0x02, RT5350_UART_LITE_BASE + UART_MCR );
			__raw_writel( 0x01, RT5350_UART_LITE_BASE + UART_IER );			
		}
		break;
	default:
		CT_ASSERT( 0 );
		break;
	}
}


/******************************************************************************
** 函 数 名： synTimerHook
** 说    明:  定时器的回调函数，定时器超时后根据当前的状态进入相应的数据处理
** 输    入： 串口端口号
** 返    回： void
******************************************************************************/
void synTimerHook( INT32U parm )
{
	unsigned long cpu_sr;
	INT32U ticks;
	INT8U msgTail;
	struct FpMsgPar *msg;
	//struct NewPackLenPar packPar;
	INT16U len
	INT16U i;
/*
	fpCtrlBlock.port = 3;
	fpCtrlBlock.baudRate = 9600;
	fpCtrlBlock.parity = 0;
	fpCtrlBlock.dataBits = 8;
	fpCtrlBlock.stopBits = 1;
*/
	//printk("synTimerHook %d port %d baud %d par %d data %d stop %d\n",fpCtrlBlock.status,
//	fpCtrlBlock.port,fpCtrlBlock.baudRate ,fpCtrlBlock.parity ,fpCtrlBlock.dataBits,fpCtrlBlock.stopBits);
	if ( fpCtrlBlock.status == FP_STA_OK )
	{
		if ( fpCtrlBlock.platToDriverMsgNum > 0 )
		{
			msg = &(fpCtrlBlock.platToDriverMsg)/* + fpCtrlBlock.curPlatToDriverMsgPoint*/;

			fpCtrlBlock.packLen = msg->count;
			/*	packNewRequest( msg->slave, msg->rw,
			                          msg->area, msg->addr,
			                          msg->count, msg->data,
			                          ( struct FpDataPacket* )( fpCtrlBlock.packData ) );*/
			memcpy(fpCtrlBlock.packData, msg->data, msg->count);
			fpCtrlBlock.maxToRcv = 256;
			
			/*for(i=0; i<packPar.packLen;i++)
				printk("%02X ",fpCtrlBlock.packData[i]);
				printk("\n");*/
			
			if ( fpCtrlBlock.packLen > 0 && fpCtrlBlock.maxToRcv > 0 )
			{
				fpCtrlBlock.retryCnt = 0;
				fpCtrlBlock.status = FP_STA_SYN_TO_SND;

				ticks = 3 * 11 * 1000000 / fpCtrlBlock.baudRate;
				if ( ticks < 1000 ) //最小1ms
					ticks = 1000;
				SetTicks_HS_timer( fpCtrlBlock.synTimer, ticks ); //3个字节同步时间
				Start_HS_timer( fpCtrlBlock.synTimer );
			}
			else
			{
				//将消息拷贝到将要获得数据的缓冲区
				/*local_irq_save( cpu_sr );
				msgTail = fpCtrlBlock.curDriverToPlatMsgPoint + fpCtrlBlock.driverToPlatMsgNum;
				msgTail %= MAX_DRIVER_TO_MULPLAT_MSG_NUM;
				msg->err = FP_PARM_ERR;
				memcpy( &( fpCtrlBlock.driverToPlatMsg[msgTail] ), msg , sizeof( struct FpMsgPar ) );
				fpCtrlBlock.curPlatToDriverMsgPoint++;
				fpCtrlBlock.curPlatToDriverMsgPoint %= MAX_MULPLAT_TO_DRIVER_MSG_NUM;
				if ( fpCtrlBlock.platToDriverMsgNum > 0 )
				{
					fpCtrlBlock.platToDriverMsgNum--;
				}
				//有一条错误消息返回给所协议平台
				fpCtrlBlock.driverToPlatMsgNum++;
				fpCtrlBlock.status = FP_STA_OK;
				local_irq_restore( cpu_sr );
				*/
				//同步1ms秒马上发送下一条请求
				printk("fpCtrlBlock.packLen %d fpCtrlBlock.maxToRcv %d",fpCtrlBlock.packLen,fpCtrlBlock.maxToRcv);
				SetTicks_HS_timer( fpCtrlBlock.synTimer, 1000 ); //3
				Start_HS_timer( fpCtrlBlock.synTimer );
			}
		}
		else
		{
			//没有请求发送，等待50ms再检查是否有请求发送
			fpCtrlBlock.status = FP_STA_OK;
			SetTicks_HS_timer( fpCtrlBlock.synTimer, 30000 ); //3
			Start_HS_timer( fpCtrlBlock.synTimer );
		}
	}

	if ( fpCtrlBlock.status == FP_STA_SYN_TO_SND ) //发送前的同步4个字节
	{
		fpCtrlBlock.status = FP_STA_SNDING;
		CT_ASSERT( fpCtrlBlock.packLen > 0 );  //没有数据根本不要启动发送
		fpCtrlBlock.sndedLen = 0;	//初始化为０
		fpCtrlBlock.rcvedLen = 0; //
		//清缓冲区,打开发送中断,关闭接收中断
		RS485_Switch( fpCtrlBlock.port, RS485_SEND );
		sndDataToUart();               //开始发送
	}
	else if ( fpCtrlBlock.status == FP_STA_SYN_TO_RCV ) //发送完成到接收的同步
	{
		if ( fpCtrlBlock.port == 1 )
		{
			while ( 1 )
			{
				INT32U status = __raw_readl( RT5350_UART_BASE + UART_LSR );
				if ( status & 0x60 )
				{
					break;
				}
				else
				{
					ticks = 12 * 1000000 / fpCtrlBlock.baudRate;  //12位同步时间
					SetTicks_HS_timer( fpCtrlBlock.synTimer, ticks );
					Start_HS_timer( fpCtrlBlock.synTimer );
					return ;
				}
				//
				//CT_ASSERT(0);
				//已经同步过,不应该走到这里,这样写是为了防止同步时间不够
			}
		}
		else
		{
			while ( 1 )
			{
				INT32U status = __raw_readl( RT5350_UART_LITE_BASE + UART_LSR );
				if ( status & 0x60 )
				{
					break;
				}
				else
				{
					ticks = 12 * 1000000 / fpCtrlBlock.baudRate;  //12位同步时间
					SetTicks_HS_timer( fpCtrlBlock.synTimer, ticks );
					Start_HS_timer( fpCtrlBlock.synTimer );
					return ;
				}
				//CT_ASSERT(0);
				//已经同步过,不应该走到这里,这样写是为了防止同步时间不够
			}
		}

		fpCtrlBlock.status = FP_STA_RCVING;
		fpCtrlBlock.rcvedLen = 0; //已经接收长度０
		ClearUartRcvFIFO( fpCtrlBlock.port );      //清缓冲区,打开接收中断,关闭发送中断
		RS485_Switch( fpCtrlBlock.port, RS485_RECEIVE );

		//启动接收超时定时器, 超时时间 = 要接收字节需要时间	+ 100ms+
		ticks = ( 12 * 1000000 / fpCtrlBlock.baudRate ) * fpCtrlBlock.maxToRcv;
		if ( fpCtrlBlock.baudRate == 1200 )
			ticks += 320000;
		else if ( fpCtrlBlock.baudRate == 2400 )
			ticks += 250000;
		else if ( fpCtrlBlock.baudRate == 4800 )
			ticks += 180000;
		else if ( fpCtrlBlock.baudRate == 9600 )
			ticks += 130000;
		else if ( fpCtrlBlock.baudRate == 19200 )
			ticks += 110000;
		else
			ticks += 100000;

		SetTicks_HS_timer( fpCtrlBlock.synTimer, ticks );
		Start_HS_timer( fpCtrlBlock.synTimer );
	}
	else if ( fpCtrlBlock.status == FP_STA_RCVING )
	{                               //500ms超时了,  超时后处理
		fpCtrlBlock.status = FP_STA_OK;		
		ticks = 1000;
		SetTicks_HS_timer( fpCtrlBlock.synTimer, ticks ); //3个字节同步时间
		Start_HS_timer( fpCtrlBlock.synTimer );
		/*if ( fpCtrlBlock.retryCnt < M_FP_RETRIES )
		{
			fpCtrlBlock.retryCnt++;
			fpCtrlBlock.status = FP_STA_SYN_TO_SND;
			//printk("time out retry = %d\n",fpCtrlBlock.retryCnt);
			ticks = 4 * 12 * 1000000 / fpCtrlBlock.baudRate;
			if ( ticks < 2000 ) //最小2ms
				ticks = 2000;
			SetTicks_HS_timer( fpCtrlBlock.synTimer, ticks ); //3个字节同步时间
			Start_HS_timer( fpCtrlBlock.synTimer );
		}
		else
		{
			//有一条超时消息返回给所协议平台
			local_irq_save( cpu_sr );
			msg = fpCtrlBlock.platToDriverMsg + fpCtrlBlock.curPlatToDriverMsgPoint;
			msg->err = FP_TIMEOUT;
			msgTail = fpCtrlBlock.curDriverToPlatMsgPoint + fpCtrlBlock.driverToPlatMsgNum; //copy to read cache
			msgTail %= MAX_DRIVER_TO_MULPLAT_MSG_NUM;
			memcpy( &( fpCtrlBlock.driverToPlatMsg[msgTail] ), msg , sizeof( struct FpMsgPar ) );
			fpCtrlBlock.curPlatToDriverMsgPoint++;
			fpCtrlBlock.curPlatToDriverMsgPoint %= MAX_MULPLAT_TO_DRIVER_MSG_NUM;
			if ( fpCtrlBlock.platToDriverMsgNum > 0 )
			{
				fpCtrlBlock.platToDriverMsgNum--;
			}
			fpCtrlBlock.driverToPlatMsgNum++;
			fpCtrlBlock.status = FP_STA_OK;
			local_irq_restore( cpu_sr );

			SetTicks_HS_timer( fpCtrlBlock.synTimer, 1000 ); //3
			Start_HS_timer( fpCtrlBlock.synTimer );
		}*/
	}
}

/******************************************************************************
** 函  数  名 ： FP_serial_port_write
** 说      明 :  往串口中写入数据
** 输      入 ： filp  文件描述符
**               buffer  存储写入串口数据的用户空间的数组
**               count  写入串口的字节数
**               ppos  文件当前读取位指示符
** 返      回 ： 实际写入串口的字节数
******************************************************************************/
static int FP_serial_port_write( struct file *filp, const char *buffer, size_t count, loff_t *ppos )
{
	unsigned long cpu_sr;
	INT8U msgTail;
	int ret;
	int i;
	CT_ASSERT( count == sizeof( struct FpMsgPar ) );
	//printk("sfd %d slave %d ?? %d rw %d\n",buffer[0],buffer[1],buffer[2],buffer[3]);
	//printk("W:%d-%d\n",fpCtrlBlock.platToDriverMsgNum,fpCtrlBlock.driverToPlatMsgNum);
	if ( fpCtrlBlock.platToDriverMsgNum > 0 || fpCtrlBlock.driverToPlatMsgNum > 0 ) //没有缓冲区了
	{
		return -1;
	}
	else
	{
		local_irq_save( cpu_sr );
		//msgTail = fpCtrlBlock.curPlatToDriverMsgPoint + fpCtrlBlock.platToDriverMsgNum;
		//msgTail %= MAX_MULPLAT_TO_DRIVER_MSG_NUM;
		ret = copy_from_user( &fpCtrlBlock.platToDriverMsg, buffer, count );
		fpCtrlBlock.platToDriverMsgNum = 1;
		local_irq_restore( cpu_sr );

		if ( fpCtrlBlock.status != FP_STA_OK )
		{
			return count;
		}
		else
		{
			Stop_HS_timer( fpCtrlBlock.synTimer );
			SetTicks_HS_timer( fpCtrlBlock.synTimer, 1000 ); //3
			Start_HS_timer( fpCtrlBlock.synTimer );
			return count;
		}
	}
}

/******************************************************************************
** 函  数  名 ： FP_serial_port_read
** 说      明 :  从串口中读取数据
** 输      入 ： filp  文件描述符
**               buffer  存储读取的数据的用户空间的数组
**               count  上层用户要读取的字节数
**               ppos  文件当前读取位指示符
** 返      回 ： 实际读取的字节数
******************************************************************************/
static int FP_serial_port_read( struct file * filp, char * buffer, size_t count, loff_t *ppos )
{
	unsigned long  cpu_sr;
	struct FpMsgPar *msg;
	int ret;
	CT_ASSERT( count == sizeof( struct FpMsgPar ) );
	//if( count != sizeof( struct FpMsgPar ) )
	//{
	//	printk("count %d sizeof( struct FpMsgPar ) %d\n",count,sizeof( struct FpMsgPar ));
	//}
	//printk("R:%d-%d\n",fpCtrlBlock.platToDriverMsgNum,fpCtrlBlock.driverToPlatMsgNum);
	if ( fpCtrlBlock.driverToPlatMsgNum > 0 )
	{
		local_irq_save( cpu_sr );
		msg = &fpCtrlBlock.driverToPlatMsg ;
		ret = copy_to_user( buffer, msg, sizeof( struct FpMsgPar ) );
		fpCtrlBlock.driverToPlatMsgNum = 0;
		//fpCtrlBlock.curDriverToPlatMsgPoint++;
		//fpCtrlBlock.curDriverToPlatMsgPoint %= MAX_DRIVER_TO_MULPLAT_MSG_NUM;
		local_irq_restore( cpu_sr );
		return count;
	}

	return -1;
}

/******************************************************************************
** 函  数  名 ： FP_serial_port_select
** 说      明 :  检测能否从串口中读取数据
** 输      入 ： filp  文件描述符
**               wait  等待列表指针
**               ppos  文件当前读取位指示符
** 返      回 ： 串口在指定的等待的时间间隔内有数据可读立即返回1
**               串口在指定的等待的时间间隔超时后有无数据可读返回0
******************************************************************************/
static unsigned int FP_serial_port_select( struct file *filp, struct poll_table_struct *wait )
{
	/* 串口还没有接收数据完毕，把上层用户的操作加入等待的队列中，串口接收数据完毕时唤醒这个队列 */
	//poll_wait(filp, &serial_port_wait, wait);
	return 0;
}




/******************************************************************************
** 函  数  名 ： request_irqs
** 说      明 :  申请这个模块需要的中断
** 输      入 ： NONE
** 返      回 ： 申请成功返SUCCESS
**               申请不成功返回FAILURE
******************************************************************************/
static int request_irqs( void )
{
	int ret;

	if ( fpCtrlBlock.port == 1 )
	{
		ret = request_irq( SURFBOARDINT_UART, md_uart_irq, IRQF_DISABLED, "mbus_dev_uart_full", NULL );

		if ( ret )
		{
			printk("request_irqs 1 err ret = %d\n",ret);
			return FAILURE;
		}
	}
	else
	{
		ret = request_irq( SURFBOARDINT_UART1, md_uart_lite_irq, IRQF_DISABLED, "mbus_dev_uart_lite", NULL );
		if ( ret )
		{
			printk("request_irqs 2 err ret = %d\n",ret);
			return FAILURE;
		}
	}

	return SUCCESS;
}

/******************************************************************************
** 函  数  名 ： free_irqs
** 说      明 :  释放这个模块已申请的中断
** 输      入 ： NONE
** 返      回 ： NONE
******************************************************************************/
static void free_irqs( void )
{
	if ( fpCtrlBlock.port == 1 )
	{
		free_irq( SURFBOARDINT_UART,  NULL );   /* 释放串口发送中断 */
	}
	else
	{
		free_irq( SURFBOARDINT_UART1,  NULL );   /* 释放串口发送中断 */
	}
}

/******************************************************************************
** 函  数  名 ： FP_serial_port_ioctl
** 说      明 :  设置串口
** 输      入 ： innode  文件信息节点指针
**               cmd  设置属性指示符
**               arg  设置的参数
** 返      回 ： 串口设置成功返回1
**               串口设置失败返回负数
******************************************************************************/

static int FP_serial_port_ioctl( struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg )
{
	unsigned long  cpu_sr;

	local_irq_save( cpu_sr );
	fpCtrlBlock.port = ( arg >> 24 ) & 0xFF;;
	fpCtrlBlock.baudRate = cmd;
	fpCtrlBlock.dataBits = ( arg >> 16 ) & 0xFF;
	fpCtrlBlock.parity = ( arg >> 8 ) & 0xFF;
	fpCtrlBlock.stopBits = arg & 0xFF;
	local_irq_restore( cpu_sr );

/*for test only*/
fpCtrlBlock.port = 1;
/*end for test only*/
	printk( "FP com Par = baud %d, dataBit %d, parity %d, stopBit %d, interface %d\n", fpCtrlBlock.baudRate, fpCtrlBlock.dataBits,
	        fpCtrlBlock.parity, fpCtrlBlock.stopBits,  fpCtrlBlock.port );

	if ( SUCCESS != request_irqs() )
	{
		return -EBUSY;
	}

	fpCtrlBlock.synTimer = Register_HS_timer( 1000, synTimerHook, 0, ( INT32U )( &fpCtrlBlock ) );

	reset485Timer = Register_HS_timer( 2000*1000, reset485TimerHook, 0, ( INT32U )( &fpCtrlBlock ) );
	checkComAlive = Register_HS_timer( 3000*1000, checkComAliveHook, 0, ( INT32U )( &fpCtrlBlock ) );
	
	ComAlivFlg = 0;
	init_waitqueue_head( &read_watque );

	uart_init( fpCtrlBlock.port, fpCtrlBlock.baudRate, fpCtrlBlock.dataBits,
	           fpCtrlBlock.parity, fpCtrlBlock.stopBits );
	//printk("uiok\n");
	
	
	
	
	return 0;
}

/******************************************************************************
** 函  数  名 ： FP_serial_port_open
** 说      明 :  打开FP串口
** 输      入 ： 记录设备模块被使用的次数
** 返      回 ： 串口设置成功返回1
**               串口设置失败返回负数
******************************************************************************/
static int FP_serial_port_open( struct inode* inode, struct file* file )
{
	unsigned long  cpu_sr;

	if ( !try_module_get( THIS_MODULE ) )
	{
		printk( "FP_serial_port_open err \n" );
		return -EBUSY;
	}
	local_irq_save( cpu_sr );
	//msg = NULL;
	fpCtrlBlock.status = FP_NO_ERR;
	fpCtrlBlock.port = 3;
	fpCtrlBlock.baudRate = 9600;
	fpCtrlBlock.parity = 0;
	fpCtrlBlock.dataBits = 8;
	fpCtrlBlock.stopBits = 1;

	fpCtrlBlock.curDriverToPlatMsgPoint = 0;
	fpCtrlBlock.curPlatToDriverMsgPoint = 0;
	fpCtrlBlock.driverToPlatMsgNum = 0;
	fpCtrlBlock.platToDriverMsgNum = 0;

	fpCtrlBlock.status = FP_STA_OK; //重新进入空闲状态
	local_irq_restore( cpu_sr );
/*
	fpCtrlBlock.synTimer = Register_HS_timer( 1000, synTimerHook, 0, ( INT32U )( &fpCtrlBlock ) );

	reset485Timer = Register_HS_timer( 2000*1000, reset485TimerHook, 0, ( INT32U )( &fpCtrlBlock ) );
	checkComAlive = Register_HS_timer( 3000*1000, checkComAliveHook, 0, ( INT32U )( &fpCtrlBlock ) );
	
	ComAlivFlg = 0;
	init_waitqueue_head( &read_watque );
*/
	printk( "FP port open\n" );
	return 0;
}

/******************************************************************************
** 函  数  名 ： FP_serial_port_release
** 说      明 :  释放FP串口
** 输      入 ： 记录设备模块被释放的次数
** 返      回 ： 串口设置成功返回1
**               串口设置失败返回负数
******************************************************************************/
static int FP_serial_port_release( struct inode* inode, struct file* file )
{
	//unsigned long  cpu_sr;
	free_irqs();
	module_put( THIS_MODULE );

	/*
	local_irq_save( cpu_sr );
	if ( msg )
	{
		struct FpMsgPar* msg = msg;
		msg = NULL;
		local_irq_restore( cpu_sr );

		kfree( msg );
	}
	else
		local_irq_restore( cpu_sr );
	*/

	fpCtrlBlock.curDriverToPlatMsgPoint = 0;
	fpCtrlBlock.curPlatToDriverMsgPoint = 0;
	fpCtrlBlock.driverToPlatMsgNum = 0;
	fpCtrlBlock.platToDriverMsgNum = 0;

	fpCtrlBlock.status = FP_STA_OK; //重新进入空闲状态
	UnRegister_HS_timer( fpCtrlBlock.synTimer );
	UnRegister_HS_timer( reset485Timer );
	UnRegister_HS_timer( checkComAlive );

	printk( "FP port release\n" );
	return 0;
}

/*------------------------------*------------------------------------*/
/*   用已定义的对串口的操作函数填充对串口设备文件操作的相关数据结构  */
/*------------------------------*------------------------------------*/
static struct file_operations hmi_FP_fops =
{
	.owner = THIS_MODULE,
	.open = FP_serial_port_open,
	.release = FP_serial_port_release,
	.read = FP_serial_port_read,
	.write = FP_serial_port_write,
	.poll = FP_serial_port_select,
	.ioctl = FP_serial_port_ioctl,
};

void hmi_FP_setup_cdev( dev_t devno )
{
	int err;

	cdev_init( &dev.cdev, &hmi_FP_fops );
	dev.cdev.owner = THIS_MODULE;
	dev.cdev.ops = &hmi_FP_fops;
	err = cdev_add( &dev.cdev, devno, 1 );
	if ( err )
		printk( KERN_NOTICE "Error %d adding hmi_FP ", err );
}

struct class *FP_class;
struct device* FP_device;

/******************************************************************************
** 函  数  名 ： hmi_FP_init
** 说      明 :  初始化串口驱动，在串口驱动模块加载到LINUX内核中时这个函数被调用
** 输      入 ： NONE
** 返      回 ： 串口驱动模块加载成功返回0
**               串口驱动模块加载失败返回负数
******************************************************************************/
static int __init hmi_FP_init( void )
{
	int ret;
	dev_t devno;

	pclk_sys = 40000000;
	printk( "clock pclk  : %ld \n", pclk_sys );
	printk( "the FP module is insmod.\n" );

	ret = alloc_chrdev_region( &devno, 0, 1, FP_DEVICE_NAME ); //申请设备号
	if ( ret < 0 )
		return ret;

	hmi_FP_major = MAJOR( devno );
	hmi_FP_setup_cdev( devno );

	FP_class = class_create( THIS_MODULE, FP_DEVICE_NAME );
	if ( IS_ERR( FP_class ) )
	{
		cdev_del( &dev.cdev );	//删除cdev结构体和cdev设备
		unregister_chrdev_region( MKDEV( hmi_FP_major, 0 ), 1 );//释放设备号
		printk( "Err: failed in creating class FP.\n" );
		return -1;
	}

	FP_device = device_create( FP_class, NULL, devno, FP_DEVICE_NAME );
	if ( IS_ERR( FP_device ) )
	{
		cdev_del( &dev.cdev );	//删除cdev结构体和cdev设备
		unregister_chrdev_region( MKDEV( hmi_FP_major, 0 ), 1 );//释放设备号
		printk( KERN_ERR "FP_add: device_create failed\n" );
		return -1;
	}
	
	set_CT_GPIO_output_mode(9);
	set_CT_GPIO_data(9, 1);
	RS485_Switch( 1, RS485_RECEIVE );
	return 0;
}

/******************************************************************************
** 函  数  名 ： hmi_FP_exit
** 说      明 :  卸载串口驱动，在串口驱动从LINUX内核中卸载时这个函数被调用
** 输      入 ： NONE
** 返      回 ： NONE
******************************************************************************/
static void __exit hmi_FP_exit( void )
{

	printk( "the module is removed.\n" );
	device_destroy( FP_class, MKDEV( hmi_FP_major, 0 ) );
	class_destroy( FP_class );
	cdev_del( &dev.cdev );	//删除cdev结构体和cdev设备
	unregister_chrdev_region( MKDEV( hmi_FP_major, 0 ), 1 );//释放设备号
}

/* 定义驱动加载和卸载调用的函数对应的宏 */
module_init( hmi_FP_init );            /* 串口驱动模块加载调用的函数对应的宏 */
module_exit( hmi_FP_exit );            /* 串口驱动模块卸载调用的函数对应的宏 */
MODULE_LICENSE( "GPL" );                   /* 设置模块的证书，在GPL下可以发布此代码 */

