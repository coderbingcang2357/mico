#ifndef MODBUS_DRIVER_H
#define MODBUS_DRIVER_H

#include "../ct_def.h"

#define M_MODBUS_RETRIES      2     // Number of retries requested
#define M_MODBUS_MAX          240   // Buffer bytes available for data writes

#define M_READ   0      //MODBUS 读
#define M_WRITE  1      //MODBUS 写

#define MODBUX_AREA_0X   0
#define MODBUX_AREA_1X   1
#define MODBUX_AREA_2X   2
#define MODBUX_AREA_3X   3
#define MODBUX_AREA_4X   4

#define MODBUS_NO_ERR    0
#define MODBUS_TIMEOUT   1
#define MODBUS_PARM_ERR  2
//更新至HMI2018-04-26
#define COM_OPEN_SUCCESS 3
#define COM_OPEN_ERR     4
////////END

//多协议平台最大写缓冲区
#define MAX_MULPLAT_TO_DRIVER_MSG_NUM 32
//多协议平台最大读缓冲区
#define MAX_DRIVER_TO_MULPLAT_MSG_NUM 32

#define MODBUS_STA_IDLE       0 //空闲状态
#define MODBUS_STA_SYN_TO_SND 1 //准备发送同步中
#define MODBUS_STA_SNDING     2 //正在发送
#define MODBUS_STA_SYN_TO_RCV 3 //发送完成，切换到接收同步中
#define MODBUS_STA_RCVING     4 //接收中 
#define MODBUS_STA_OK         5 //完成，得到结果
//
#pragma pack(push,1)
struct ModbusDataPacket
{
        INT8U  slave;  //1
        INT8U  FC;//2
        INT16U addr;   //3
        INT8U  data[256];//5
};
#pragma pack(pop)

struct NewPackLenPar
{
    INT8U packLen;
    INT8U maxToRcv;
};

// MbusMsg 函数传递参数结构体定义
struct MbusMsgPar
{
    INT8U sfd;      //socket fd
    INT16U serialNum;
    INT8U  slave;   //Slave address (0 - 247)
    INT8U  rw;      //Read = 0, Write = 1
    INT8U area;     //0x, 1x, 2x, 3x, 4x
    INT16U addr;    //Modbus addr (ie 00001)
    INT16U count;   //Number of elements (1- 120 words or 1 to 1920 bits)
    INT8U data[256];//Pointer to data (ie &VB100)
    INT8U err;      //Error code (0 = no error)
};

struct MbusCtrlBlock
{

    INT8U platToDriverMsgNum;         //当前写消息数量
    INT8U curPlatToDriverMsgPoint;    //当前写消息处理位置
    struct MbusMsgPar platToDriverMsg[MAX_MULPLAT_TO_DRIVER_MSG_NUM];

    INT8U driverToPlatMsgNum;         //当前读消息数量
    INT8U curDriverToPlatMsgPoint;    //当前读消息处理位置
    struct MbusMsgPar driverToPlatMsg[MAX_DRIVER_TO_MULPLAT_MSG_NUM];

    INT8U retryCnt;
    INT8U packLen;
    INT8U maxToRcv;
    INT8U packData[sizeof( struct ModbusDataPacket )];

    INT8U rcvData[sizeof( struct ModbusDataPacket )];
    INT8U rcvedLen;

    int status;  //IDLE, CAN_SND, SNDING, SYNING, RCVING,
};
/*
class Modbus : public QObject
{
    Q_OBJECT
public:
    Modbus();
    ~Modbus();
    int write( const char *buffer, size_t count );
    int read(  char *buffer, size_t count );
    bool openCom( const QString &portNumber, long lBaudRate, int dataBit, 
                  int stopBit, int parity );

protected:
signals:
    void readDataReady();

private slots:
    void slot_onReceiveFrame();
    void slot_synTimerTimeout();

private:
    struct NewPackLenPar packNewRequest( INT8U slave, INT8U rw, INT8U area, INT16U addr, INT16U count,
                                         INT8U* dadaPtr, struct ModbusDataPacket* packet );
    INT8U checkGetResponse( INT8U* dadaPtr, struct ModbusDataPacket* packetReq,
    struct ModbusDataPacket* packetRsp, INT8U rcvedLen );
    qint64 sendToUart();
    void prcRecvData();

    QTimer *m_synTimer;
    MbusCtrlBlock *m_mbusCtrlBlock;
    QextSerialPort *m_serialPort;
    QByteArray m_recvBlock;
};
*/
#endif //MODBUS_DRIVER_H
