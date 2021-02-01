#ifndef _TCP_MBUS_H_
#define _TCP_MBUS_H_

/**************��վ������**********************/
#define M_NO_ERROR        0   //No error
#define M_PARITY_ERROR    1   //У����� 
#define M_BAUD_ERROR      2   //�����ʴ���

#define M_TIMEOUT_ERROR   3   //��ʱ���� //������HMI2018-04-26
#define M_REQUEST_ERROR   4   //������� //������HMI2018-04-26
#define M_ACTIVE_ERROR    5   //Modbusû��ʹ��
#define M_BUSY_ERROR      6   //ͨѶæ
#define M_RESPONSE_ERROR  7   //Ӧ�����
#define M_CRC_ERROR       8   //CRC����
#define M_HW_ERROR        9   //Ӳ������
/**************************************************/


typedef struct tagmbus_tcp_req_struct
{
	unsigned char First;
	unsigned char IP[20];     /* VAR_INPUT */
	unsigned short Port;      /* VAR_INPUT */
	unsigned short Unit;      /* VAR_INPUT */
	unsigned char RW;         /* VAR_INPUT */
	unsigned int Addr;        /* VAR_INPUT */
	unsigned short Count;     /* VAR_INPUT */
	unsigned char Data[256];  /* VAR_INPUT */
	unsigned char Done;       /* VAR_OUTPUT */
	unsigned char Error;      /* VAR_OUTPUT */
}mbus_tcp_req_struct;
//������HMI2018-04-26
void mbus_tcp_req__main(mbus_tcp_req_struct *p);
void  stopModbusTCP(void);
////////END
#endif
