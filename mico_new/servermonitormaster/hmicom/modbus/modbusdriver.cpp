
#include <QTimer>
#include "modbusdriver.h"


Modbus::Modbus()
{
    m_mbusCtrlBlock = new struct MbusCtrlBlock;
    m_mbusCtrlBlock->curDriverToPlatMsgPoint = 0;
    m_mbusCtrlBlock->curPlatToDriverMsgPoint = 0;
    m_mbusCtrlBlock->driverToPlatMsgNum = 0;
    m_mbusCtrlBlock->platToDriverMsgNum = 0;
    m_mbusCtrlBlock->status = MODBUS_STA_OK;
    m_synTimer = new QTimer;
    connect( m_synTimer, SIGNAL( timeout() ), this, SLOT( slot_synTimerTimeout() ) );
    m_serialPort = new QextSerialPort( QString( "COM1" ), QextSerialPort::EventDriven );
    connect( m_serialPort, SIGNAL( readyRead() ), this, SLOT( slot_onReceiveFrame() ) );
}

Modbus::~Modbus()
{
    delete m_synTimer;
    if ( m_serialPort->isOpen() )
    {
        m_serialPort->close();
        delete m_serialPort;
        m_serialPort = 0;
    }

}

bool Modbus::openCom( const QString &portNumber, long lBaudRate, int dataBit,
               int stopBit, int parity )
{
    m_serialPort->setPortName(portNumber);
    m_serialPort->setTimeout(4000);
    m_serialPort->setFlowControl(FLOW_OFF);
    switch ( dataBit )
    {
    case 7:
        m_serialPort->setDataBits(DATA_7);
        break;
    case 8:
    default:
        m_serialPort->setDataBits(DATA_8);
        break;
    }

    if ( stopBit == 2 )
    {
        m_serialPort->setStopBits(STOP_2);
    }
    else
    {
        m_serialPort->setStopBits(STOP_1);
    }

    switch ( parity )
    {
    case 1: //奇校验
        m_serialPort->setParity(PAR_ODD);
        break;
    case 2: //偶校验
        m_serialPort->setParity(PAR_EVEN);
        break;
    case 0:
    default:
        m_serialPort->setParity(PAR_NONE);
        break;
    }

    switch ( lBaudRate )       // 波特率
    {
    case 115200:
        m_serialPort->setBaudRate(BAUD115200);
        break;
    case 57600:
        m_serialPort->setBaudRate(BAUD57600);
        break;
    case 38400:
        m_serialPort->setBaudRate(BAUD38400);
        break;
    case 19200:
        m_serialPort->setBaudRate(BAUD19200);
        break;
    case 9600:
        m_serialPort->setBaudRate(BAUD9600);
        break;
    case 4800:
        m_serialPort->setBaudRate(BAUD4800);
        break;
    case 2400:
        m_serialPort->setBaudRate(BAUD2400);
    case 1200:
        m_serialPort->setBaudRate(BAUD1200);
        break;
    default:
        m_serialPort->setBaudRate(BAUD115200);
        break;
    }

    if ( !m_serialPort->isOpen() )
    {
        bool ok = m_serialPort->open(QIODevice::ReadWrite);
        if ( !ok )
        {
            return false;
        }
    }
    qDebug()<<"open com successful";
    m_synTimer->start(100);
    return true;
}

qint64 Modbus::sendToUart()
{
    //qDebug()<<"snd "<<(QByteArray *)(*m_mbusCtrlBlock->packData);
    return m_serialPort->write( (char*)m_mbusCtrlBlock->packData, m_mbusCtrlBlock->packLen );
}

void Modbus::slot_onReceiveFrame()
{
    if ( m_mbusCtrlBlock->status == MODBUS_STA_RCVING )
    {
        m_recvBlock += m_serialPort->readAll();
        //qDebug()<<"rcv "<<m_recvBlock.toHex();
        m_synTimer->stop();
        prcRecvData();
    }
    else
    {
        m_serialPort->readAll();
    }
}

void Modbus::prcRecvData()
{
    struct MbusMsgPar *msg;
    INT8U err;
    INT8U msgTail;
    //正确接收 或者 收到 FC大于0x80的包才进行处理 否则认为没有接收完成。
    //因为串口缓冲区大小为64，当包大小达到64的时候会触发一次readReady()信号

    if ( (m_recvBlock.size() >= m_mbusCtrlBlock->maxToRcv || (unsigned char)m_recvBlock.data()[1] > 0x80) && m_recvBlock.size() <=250 )
    {
        //qDebug()<<"rcv "<<m_recvBlock.toHex();
        m_mbusCtrlBlock->rcvedLen = m_recvBlock.size();
        for ( int i = 0; i < m_mbusCtrlBlock->rcvedLen; i++ )
        {
            m_mbusCtrlBlock->rcvData[i] = m_recvBlock.data()[i];
        }
    }
    else
    {
        return;
    }
    m_recvBlock.clear();
    msg = m_mbusCtrlBlock->platToDriverMsg + m_mbusCtrlBlock->curPlatToDriverMsgPoint;

    err = checkGetResponse( msg->data, ( struct ModbusDataPacket* ) & ( m_mbusCtrlBlock->packData ),
        ( struct ModbusDataPacket* ) & ( m_mbusCtrlBlock->rcvData ), m_mbusCtrlBlock->rcvedLen );
    if ( err == MODBUS_TIMEOUT && m_mbusCtrlBlock->retryCnt < M_MODBUS_RETRIES )
    {
        m_mbusCtrlBlock->retryCnt++;
        sendToUart();
        m_synTimer->start(500);
    }
    else
    {
        msg->err = err;
        msgTail = m_mbusCtrlBlock->curDriverToPlatMsgPoint + m_mbusCtrlBlock->driverToPlatMsgNum; //copy to read cache
        msgTail %= MAX_DRIVER_TO_MULPLAT_MSG_NUM;
        memcpy( &( m_mbusCtrlBlock->driverToPlatMsg[msgTail] ), msg , sizeof( struct MbusMsgPar ) );
        m_mbusCtrlBlock->curPlatToDriverMsgPoint++;
        m_mbusCtrlBlock->curPlatToDriverMsgPoint %= MAX_MULPLAT_TO_DRIVER_MSG_NUM;
        m_mbusCtrlBlock->platToDriverMsgNum--;
        m_mbusCtrlBlock->driverToPlatMsgNum++;
        emit readDataReady();
        m_mbusCtrlBlock->status = MODBUS_STA_OK;
        m_synTimer->start(10);
    }
}

void Modbus::slot_synTimerTimeout( )
{
    m_synTimer->stop();
    INT8U msgTail;
    struct MbusMsgPar *msg;
    struct NewPackLenPar packPar;

    if ( m_mbusCtrlBlock->status == MODBUS_STA_OK )
    {
        if ( m_mbusCtrlBlock->platToDriverMsgNum > 0 )
        {
            msg = m_mbusCtrlBlock->platToDriverMsg + m_mbusCtrlBlock->curPlatToDriverMsgPoint;

            packPar = packNewRequest( msg->slave, msg->rw,
                                      msg->area, msg->addr,
                                      msg->count, msg->data,
                                      ( struct ModbusDataPacket* )( m_mbusCtrlBlock->packData ) );
            m_mbusCtrlBlock->packLen = packPar.packLen;
            m_mbusCtrlBlock->maxToRcv = packPar.maxToRcv;
            if ( m_mbusCtrlBlock->packLen > 0 && m_mbusCtrlBlock->maxToRcv > 0 )
            {
                m_mbusCtrlBlock->retryCnt = 0;
                m_mbusCtrlBlock->rcvedLen = 0;
                m_recvBlock.clear();
                sendToUart();
                m_mbusCtrlBlock->status = MODBUS_STA_RCVING;
                m_synTimer->start(500);
            }
            else
            {
                //将消息拷贝到将要获得数据的缓冲区

                msgTail = m_mbusCtrlBlock->curDriverToPlatMsgPoint + m_mbusCtrlBlock->driverToPlatMsgNum;
                msgTail %= MAX_DRIVER_TO_MULPLAT_MSG_NUM;
                msg->err = MODBUS_PARM_ERR;
                memcpy( &( m_mbusCtrlBlock->driverToPlatMsg[msgTail] ), msg , sizeof( struct MbusMsgPar ) );
                m_mbusCtrlBlock->curPlatToDriverMsgPoint++;
                m_mbusCtrlBlock->curPlatToDriverMsgPoint %= MAX_MULPLAT_TO_DRIVER_MSG_NUM;
                m_mbusCtrlBlock->platToDriverMsgNum--;
                //有一条错误消息返回给所协议平台
                m_mbusCtrlBlock->driverToPlatMsgNum++;
                emit readDataReady();
                m_mbusCtrlBlock->status = MODBUS_STA_OK;

                m_synTimer->start(1);
            }
        }
        else
        {
            //没有请求发送，等待30ms再检查是否有请求发送
            m_mbusCtrlBlock->status = MODBUS_STA_OK;
            m_synTimer->start(30);
        }
    }
    else if ( m_mbusCtrlBlock->status == MODBUS_STA_RCVING )
    {                               //500ms超时了,  超时后处理
        if ( m_mbusCtrlBlock->retryCnt < M_MODBUS_RETRIES )
        {
            m_mbusCtrlBlock->retryCnt++;
            sendToUart();
            m_synTimer->start(500);
        }
        else
        {
            //有一条超时消息返回给所协议平台
            msg = m_mbusCtrlBlock->platToDriverMsg + m_mbusCtrlBlock->curPlatToDriverMsgPoint;
            msg->err = MODBUS_TIMEOUT;
            msgTail = m_mbusCtrlBlock->curDriverToPlatMsgPoint + m_mbusCtrlBlock->driverToPlatMsgNum; //copy to read cache
            msgTail %= MAX_DRIVER_TO_MULPLAT_MSG_NUM;
            memcpy( &( m_mbusCtrlBlock->driverToPlatMsg[msgTail] ), msg , sizeof( struct MbusMsgPar ) );
            m_mbusCtrlBlock->curPlatToDriverMsgPoint++;
            m_mbusCtrlBlock->curPlatToDriverMsgPoint %= MAX_MULPLAT_TO_DRIVER_MSG_NUM;
            m_mbusCtrlBlock->platToDriverMsgNum--;
            m_mbusCtrlBlock->driverToPlatMsgNum++;
            m_mbusCtrlBlock->status = MODBUS_STA_OK;
            emit readDataReady();

            m_synTimer->start(1);
        }
    }
}

/******************************************************************************
** 函  数  名 ： modbus_serial_port_write
** 说      明 :  往串口中写入数据
** 输      入 ： filp  文件描述符
**               buffer  存储写入串口数据的用户空间的数组
**               count  写入串口的字节数
**               ppos  文件当前读取位指示符
** 返      回 ： 实际写入串口的字节数
******************************************************************************/
int Modbus::write(  const char *buffer, size_t count )
{
    INT8U msgTail;
    Q_ASSERT( count == sizeof( struct MbusMsgPar ) );

    if ( m_mbusCtrlBlock->platToDriverMsgNum >=  MAX_MULPLAT_TO_DRIVER_MSG_NUM || m_mbusCtrlBlock->driverToPlatMsgNum >= MAX_DRIVER_TO_MULPLAT_MSG_NUM ) //没有缓冲区了
    {
        return -1;
    }
    else
    {

        msgTail = m_mbusCtrlBlock->curPlatToDriverMsgPoint + m_mbusCtrlBlock->platToDriverMsgNum;
        msgTail %= MAX_MULPLAT_TO_DRIVER_MSG_NUM;
        memcpy( (char *)&m_mbusCtrlBlock->platToDriverMsg[msgTail], buffer, count );
        m_mbusCtrlBlock->platToDriverMsgNum++;
        return count;
    }
}

/******************************************************************************
** 函  数  名 ： modbus_serial_port_read
** 说      明 :  从串口中读取数据
** 输      入 ： filp  文件描述符
**               buffer  存储读取的数据的用户空间的数组
**               count  上层用户要读取的字节数
**               ppos  文件当前读取位指示符
** 返      回 ： 实际读取的字节数
******************************************************************************/
int Modbus::read( char * buffer, size_t count)
{
    struct MbusMsgPar *msg;
    Q_ASSERT( count == sizeof( struct MbusMsgPar ) );

    if ( m_mbusCtrlBlock->driverToPlatMsgNum > 0 )
    {
        msg = m_mbusCtrlBlock->driverToPlatMsg + m_mbusCtrlBlock->curDriverToPlatMsgPoint;
        memcpy( buffer, (char *)msg, count );
        m_mbusCtrlBlock->driverToPlatMsgNum--;
        m_mbusCtrlBlock->curDriverToPlatMsgPoint++;
        m_mbusCtrlBlock->curDriverToPlatMsgPoint %= MAX_DRIVER_TO_MULPLAT_MSG_NUM;
        return count;
    }
    return -1;
}

const INT16U MCrcTable[] =
{
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241, 0xC601, 0x06C0, 0x0780, 0xC741,
    0x0500, 0xC5C1, 0xC481, 0x0440, 0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841, 0xD801, 0x18C0, 0x1980, 0xD941,
    0x1B00, 0xDBC1, 0xDA81, 0x1A40, 0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641, 0xD201, 0x12C0, 0x1380, 0xD341,
    0x1100, 0xD1C1, 0xD081, 0x1040, 0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441, 0x3C00, 0xFCC1, 0xFD81, 0x3D40,
    0xFF01, 0x3FC0, 0x3E80, 0xFE41, 0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41, 0xEE01, 0x2EC0, 0x2F80, 0xEF41,
    0x2D00, 0xEDC1, 0xEC81, 0x2C40, 0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041, 0xA001, 0x60C0, 0x6180, 0xA141,
    0x6300, 0xA3C1, 0xA281, 0x6240, 0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41, 0xAA01, 0x6AC0, 0x6B80, 0xAB41,
    0x6900, 0xA9C1, 0xA881, 0x6840, 0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40, 0xB401, 0x74C0, 0x7580, 0xB541,
    0x7700, 0xB7C1, 0xB681, 0x7640, 0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241, 0x9601, 0x56C0, 0x5780, 0x9741,
    0x5500, 0x95C1, 0x9481, 0x5440, 0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841, 0x8801, 0x48C0, 0x4980, 0x8941,
    0x4B00, 0x8BC1, 0x8A81, 0x4A40, 0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641, 0x8201, 0x42C0, 0x4380, 0x8341,
    0x4100, 0x81C1, 0x8081, 0x4040
};

INT16U calCrc( INT8U* pMbufr, int len )
{
    INT16U count, crc, data;

    for ( crc = 0xFFFF, count = len; count > 0; count-- )
    {
        data = *pMbufr++;
        data = ( data ^ crc ) & 0xFF;
        crc = ( crc >> 8 ) ^ MCrcTable[data];
    }
    crc = (( crc & 0xFF ) << 8 ) | ( crc >> 8 );

    //pMbufr[0] = (crc>>8) & 0xFF;
    //pMbufr[1] = crc & 0xFF;

    return crc;
}

const INT8U MBitMaskTable[] = {0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF};

struct NewPackLenPar Modbus::packNewRequest( INT8U slave, INT8U rw, INT8U area, INT16U addr, INT16U count,
    INT8U* dataPtr, struct ModbusDataPacket* packet )
{
    struct NewPackLenPar packPar;
    INT8U byteCnt;
    INT16U crc;

    packPar.packLen = 0;
    packPar.maxToRcv = 0;

    Q_ASSERT( packet && packet );
    if ( slave > 247 || count == 0 || count > 8*M_MODBUS_MAX || dataPtr == NULL || packet == NULL )
        return packPar;

    packet->slave = slave;
    packet->addr = (( addr << 8 ) & 0xFF00 ) | (( addr >> 8 ) & 0x00FF ); //转换
    if ( area == MODBUX_AREA_0X )
    {
        if ( rw == M_READ )
        {
            packet->FC = 1;
            packet->data[0] = ( count >> 8 ) & 0xFF;
            packet->data[1] = count & 0xFF;

            packPar.packLen = 6;
            packPar.maxToRcv = (( count + 7 ) / 8 ) + 5;
        }
        else if ( rw == M_WRITE )
        {
            if ( count == 1 )
            {
                packet->FC = 5;
                if ( *dataPtr & 0x1 )
                    packet->data[0] = 0xFF;
                else
                    packet->data[0] = 0x00;
                packet->data[1] = 0x00;

                packPar.packLen = 6;
                packPar.maxToRcv = 3 + 5;
            }
            else if ( count > 1 )
            {
                packet->FC = 15;
                packet->data[0] = ( count >> 8 ) & 0xFF;
                packet->data[1] = count & 0xFF;

                byteCnt = ( count + 7 ) / 8;    // byte len, count >= 0
                if ( byteCnt > 0 && byteCnt <= M_MODBUS_MAX )
                {	                            // byteCnt <= 240
                    packet->data[2] = byteCnt;
                    memcpy(( void* )&( packet->data[3] ), ( void* )dataPtr, ( int )byteCnt );
                    if ( count & 0x7 )          // bit left
                        packet->data[2 + byteCnt] &= MBitMaskTable[count&0x7];

                    packPar.packLen = byteCnt + 7;
                    packPar.maxToRcv = 3 + 5;
                }
                else
                    return packPar;
            }
            else
                return packPar;
        }
        else
            return packPar;
    }
    else if ( area == MODBUX_AREA_1X )
    {
        if ( rw == M_READ )
        {
            packet->FC = 2;
            packet->data[0] = ( count >> 8 ) & 0xFF;
            packet->data[1] = count & 0xFF;

            packPar.packLen = 6;
            packPar.maxToRcv = (( count + 7 ) / 8 ) + 5;
        }
        else
            return packPar;
    }
    else if ( area == MODBUX_AREA_3X )
    {
        if ( rw == M_READ )
        {
            packet->FC = 4;
            packet->data[0] = ( count >> 8 ) & 0xFF;
            packet->data[1] = count & 0xFF;

            packPar.packLen = 6;
            packPar.maxToRcv = ( count * 2 ) + 5;
        }
        else
            return packPar;
    }
    else if ( area == MODBUX_AREA_4X )
    {
        if ( rw == M_READ )
        {
            packet->FC = 3;
            packet->data[0] = ( count >> 8 ) & 0xFF;
            packet->data[1] = count & 0xFF;

            packPar.packLen = 6;
            packPar.maxToRcv = ( count * 2 ) + 5;
        }
        else if ( rw == M_WRITE )
        {
            if ( count == 1 )
            {
                packet->FC = 6;
                memcpy( packet->data, dataPtr, 2 );

                packPar.packLen = 6;
                packPar.maxToRcv = 3 + 5;
            }
            else if ( count > 1 )
            {
                packet->FC = 16;
                byteCnt = count * 2;            // byte len, Count >= 0
                if ( byteCnt <= M_MODBUS_MAX )
                {
                    packet->data[0] = ( count >> 8 ) & 0xFF;
                    packet->data[1] = count & 0xFF;
                    packet->data[2] = byteCnt;
                    memcpy( &( packet->data[3] ), dataPtr, byteCnt );

                    packPar.packLen = byteCnt + 7;
                    packPar.maxToRcv = 3 + 5;
                }
                else
                    return packPar;
            }
            else
                return packPar;
        }
        else
            return packPar;
    }
    else
        return packPar;

    crc = calCrc(( INT8U* )packet, packPar.packLen );	   // 计算 crc
    (( INT8U* )packet )[packPar.packLen++] = ( crc >> 8 ) & 0xFF;
    (( INT8U* )packet )[packPar.packLen++] = crc & 0xFF;

    return packPar;
}

INT8U Modbus::checkGetResponse( INT8U* dadaPtr, struct ModbusDataPacket* packetReq,
                        struct ModbusDataPacket* packetRsp, INT8U rcvedLen )
{
    INT8U len;
    Q_ASSERT( dadaPtr && packetReq && packetRsp );

    if ( packetRsp->FC > 0x80 )
    {
        return MODBUS_PARM_ERR;
    }
    else if ( packetReq->FC != packetRsp->FC )
    {
        qDebug()<<"Here err 2"<<packetRsp->slave<< packetRsp->FC<< rcvedLen ;
        return MODBUS_TIMEOUT;
    }
    else if ( 0 != calCrc(( INT8U* )packetRsp, rcvedLen ) )
    {
        qDebug()<<"Here err 3, %02x, %02x, %d\n"<< packetRsp->slave<< packetRsp->FC<< rcvedLen;
        if ( rcvedLen < 1 || 0 != calCrc(( INT8U* )packetRsp, ( rcvedLen - 1 ) ) )
        {
            return MODBUS_TIMEOUT;
        }
        else
        {
            if ( packetReq->FC <= 4 )
            {
                len = *(( INT8U* )( packetRsp ) + 2 );
                memcpy( dadaPtr, (( INT8U* )( packetRsp ) + 3 ), len );
            }
            return MODBUS_NO_ERR;
        }
    }
    else
    {
        if ( packetReq->FC <= 4 )
        {
            len = *(( INT8U* )( packetRsp ) + 2 );
            memcpy( dadaPtr, (( INT8U* )( packetRsp ) + 3 ), len );
        }
        return MODBUS_NO_ERR;
    }
}
