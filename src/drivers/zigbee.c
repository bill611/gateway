/*
 * =============================================================================
 *
 *       Filename:  UartZigbee.c
 *
 *    Description:  ��������Э��(���ͼ�����)
 *
 *        Version:  1.0
 *        Created:  2018-04-04 10:56:26
 *       Revision:  none
 *
 *         Author:  zzw
 *        Company:  Taichuan
 *
 * =============================================================================
 */
/* ---------------------------------------------------------------------------*
 *                      include head files
 *----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <pthread.h>

#include "uart.h"
#include "zigbee.h"
// #include "smart_home_pro.h"


/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/
/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/
static void uartFromZigbee(char * buff);
static void cmdParser(char *buf, int len);
/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
#define UART_LENGTH  30
#define ZIGBEE_TPYE_COORDINATER
/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static int send_cmd = 0;
static unsigned int work_tick_1s = 0;
static unsigned char zigbee_type = 0xff;		//�豸���� 0--Э����  1--·����  2--�ն˽�Ŀ
static char cmd_param[CMD_NUM];
static char get_ieee_flag = 0;			//IEEE��ַ��ȡ��ɱ�־
static short int zigbee_net_id = 0;		//PAN ID
static uint16_t zigbee_net_addr = 0xffff;	//�̵�ַ
static uint8_t zigbee_net_channel = 0xff;		//�ŵ�
static uint8_t zigbee_ieee_addr[8] = { 0 };	//IEEE��ַ

static int (*zigbeeDataRcv)(char* buf, int len) = NULL;
/*********************************************************************************************************
** Descriptions:      ������������ָ�������ֹͣ���ͣ�
** input parameters:   cmd Ҫ��������񣨶�Ӧ�ڵ�BITλ��
** output parameters:  -
** Returned value:     -
*********************************************************************************************************/
static void clearSendCmd(int cmd)
{
	send_cmd &= ~(1 << cmd);
}

/*********************************************************************************************************
** Descriptions:       ����������ͨ�����ڷ��ͳ�ȥ
** input parameters:   cmd�����֣�data����Ĳ����� len�����ĳ���
** output parameters:  -
** Returned value:     -
*********************************************************************************************************/
static void sendCmdPacket(int cmd, char* data, int len)
{
	char buf[10];

	buf[0] = 0xfc;
	buf[1] = cmd;
	buf[2] = len;
	memcpy(&buf[3], data, len);
	buf[3 + len] = 0xf5;
	if (uart)
		uart->send(uart,buf, 4 + len);
	return;
}

/*********************************************************************************************************
** Descriptions:      zigbee����ͣ���ѭ���ж�ʱ����
** input parameters:   cmd Ҫ��������񣨶�Ӧ�ڵ�BITλ��
** output parameters:  -
** Returned value:     -
*********************************************************************************************************/
static void sendCmdProcess(void)
{
	char data = 0;
	if (send_cmd)
	{
		if (send_cmd&(1 << GET_DEVICE_TYPE))
			sendCmdPacket(GET_DEVICE_TYPE_VALUE, (char*)(&data), 0);
		else if (send_cmd&(1 << SET_DEVICE_TYPE))
			sendCmdPacket(SET_DEVICE_TYPE_VALUE, (char*)(&cmd_param[SET_DEVICE_TYPE]) , 1);
		else if (send_cmd&(1 << GET_NET_ID))
			sendCmdPacket(GET_NET_ID_VALUE, (char*)(&data), 0);
		else if (send_cmd&(1 << SET_NET_ID))
			sendCmdPacket(SET_NET_ID_VALUE, (char*)(&cmd_param[SET_NET_ID]), 2);
		else if (send_cmd&(1 << GET_NODE_ADDR))
			sendCmdPacket(GET_NODE_ADDR_VALUE, (char*)(&data), 0);
		else if (send_cmd&(1 << GET_CHANNEL))
			sendCmdPacket(GET_CHANNEL_VALUE, (char*)(&data), 0);
		else if (send_cmd&(1 << GET_IEEE_ADDR))
			sendCmdPacket(GET_IEEE_ADDR_VALUE, (char*)(&data), 0);
#ifdef ZIGBEE_TPYE_COORDINATER
		else if (send_cmd&(1 << SET_CHANNEL))
			sendCmdPacket(GET_CHANNEL_VALUE, (uint8_t*)(&cmd_param[SET_CHANNEL]), 2);
#endif
		else if (send_cmd&(1 << RESUME_FACTORY_SETTINGS))
			sendCmdPacket(RESUME_FACTORY_SETTINGS_VALUE, (char*)(&data), 0);
#ifdef ZIGBEE_TPYE_COORDINATER
		else if (send_cmd&(1 << SET_NETIN_ENABLE))
			sendCmdPacket(SET_NETIN_ENABLE_VALUE, (char*)(&cmd_param[SET_NETIN_ENABLE]), 1);
		else if (send_cmd&(1 << SET_NET_SECRETKEY))
			sendCmdPacket(SET_NET_SECRETKEY_VALUE, (uint8_t*)(&data), 0);
		else if (send_cmd&(1 << RESUME_NODE_SETTING))
			sendCmdPacket(RESUME_NODE_SETTING_VALUE, (uint8_t*)(&cmd_param[RESUME_NODE_SETTING]), 2);
#endif
	}
}


/* ---------------------------------------------------------------------------*/
/**
 * @brief callbackProcess ���յ�Uart������Ϣ
 *
 * @param This
 */
/* ---------------------------------------------------------------------------*/
static void callbackProcess(void)
{
	int len = 0;
	char buff[256] = {0};
	if (uart) {
		len = uart->recvBuffer(uart,buff,sizeof(buff));
	}
	if (len <= 0)
		return;
#if 1
	printf("[callbackProcess] rx_len:%d :", len);
	int i = 0;
	for (; i < len; i++)
	{
		printf("%02x ", buff[i]);
	}
	printf("\n");
#endif
	if (len < 4)
		return;

	if ((buff[len - 1] != 0xf5))
		return;

	if (buff[0] == 0xfa)	//͸�����ݴ���
	{
		if (zigbeeDataRcv)
			zigbeeDataRcv(&buff[4], len);
	}
	else if (buff[0] == 0xfc)	//�������ݴ���
	{
		cmdParser(buff, len);
	} else {
		printf("zigbee cmd err\n");
	}

}


/*********************************************************************************************************
** Descriptions:       ������ص����Ͷ����У�������send_cmd��ʾ��ÿһBIT��ʾһ������
** input parameters:   cmd Ҫ���͵����񣨶�Ӧ�ڵ�BITλ��  param �������
** output parameters:  -
** Returned value:     -
*********************************************************************************************************/
static void setSendCmd(cmd_in_bit_TypeDef cmd, unsigned param)
{
	send_cmd |= (1 << cmd);
	cmd_param[cmd] = param;
}

/*********************************************************************************************************
** Descriptions:	   ��ȡģ���IEEE��ַ
** input parameters:    ��
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
static void getIEEE(void)
{
	if (!get_ieee_flag)
		setSendCmd(GET_IEEE_ADDR, 0);
}
/*********************************************************************************************************
** Descriptions:       ����zigbeeģ�鷵�ص�����
** input parameters:
** output parameters:  -
** Returned value:     -
*********************************************************************************************************/
static void cmdParser(char *buf, int len)
{
	int i;
	printf("cmdParser buf[%d]:", len);
	for (i=0; i<len; i++) {
		printf("%02x ", buf[i]);
	}
	printf("\n");
	switch (buf[1])
	{
		case 0x12://�豸���ͷ���
			if (send_cmd&(1 << GET_DEVICE_TYPE))
				clearSendCmd(GET_DEVICE_TYPE);
			else if (send_cmd&(1 << SET_DEVICE_TYPE))
				clearSendCmd(SET_DEVICE_TYPE);

			zigbee_type = buf[3];
			break;
		case 0x22://�����
			if (send_cmd&(1 << GET_NET_ID))
				clearSendCmd(GET_NET_ID);
			else if (send_cmd&(1 << SET_NET_ID))
				clearSendCmd(SET_NET_ID);

#ifdef MCU_BIFG_MODE
			zigbee_net_id = ((uint16_t)buf[3]) | ((uint16_t)buf[4] << 8);
#else
			zigbee_net_id = *((short int*)(&buf[3]));
#endif
			break;
		case 0x32://�ڵ��ַ����
			if (send_cmd&(1 << GET_NODE_ADDR))
				clearSendCmd(GET_NODE_ADDR);

#ifdef MCU_BIFG_MODE
			zigbee_net_addr = ((uint16_t)buf[3]) | ((uint16_t)buf[4] << 8);
#else
			zigbee_net_addr = *((short int*)(&buf[3]));
#endif
			break;
		case 0x42://ͨѶ�ŵ�����
			if (send_cmd&(1 << GET_CHANNEL))
				clearSendCmd(GET_CHANNEL);
			else if (send_cmd&(1 << SET_CHANNEL))
				clearSendCmd(SET_CHANNEL);

			zigbee_net_channel = buf[3];
			break;
		case 0x62://��ȡIEEE����
			if (send_cmd&(1 << GET_IEEE_ADDR))
				clearSendCmd(GET_IEEE_ADDR);
			memcpy(zigbee_ieee_addr, &buf[3], sizeof(zigbee_ieee_addr));
			get_ieee_flag = 1;
			break;
		case 0xff://�ɹ�����
			if (send_cmd&(1 << RESUME_FACTORY_SETTINGS))	//�ָ���������
			{
				clearSendCmd(RESUME_FACTORY_SETTINGS);
				zigbee_type = 0xff;
				zigbeeClearNetAddr();
			}
			else if (send_cmd&(1 << SET_NETIN_ENABLE))	//������������
				clearSendCmd(SET_NETIN_ENABLE);
			else if (send_cmd&(1 << SET_NET_SECRETKEY))
				clearSendCmd(SET_NET_SECRETKEY);
			else if (send_cmd&(1 << RESUME_NODE_SETTING))
				clearSendCmd(RESUME_NODE_SETTING);
			break;
		case 0x00://��λ״̬��
			zigbee_type = buf[3];
#ifdef MCU_BIFG_MODE
			zigbee_net_id = ((uint16_t)buf[4]) | ((uint16_t)buf[5] << 8);
			zigbee_net_addr = ((uint16_t)buf[6]) | ((uint16_t)buf[7] << 8);
#else
			zigbee_net_id = *((short int*)(&buf[4]));
			zigbee_net_addr = *((short int*)(&buf[6]));
#endif
			zigbee_net_channel = buf[8];
			break;
		default:
			break;
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief zigbeeSendData ����zigbee͸������ͨ�����ڷ���ȥ��
 *
 * @param dst_addr Ŀ���ַ
 * @param data ���͵�����
 * @param len ���ݳ���
 *
 * @returns 
 */
/* ---------------------------------------------------------------------------*/
unsigned char zigbeeSendData(uint16_t dst_addr, uint8_t* data, uint16_t len)
{
	unsigned char tx_buf[80];

	tx_buf[0] = 0xfa;	//��ʼ��
	*(unsigned int *)&tx_buf[1] = dst_addr;	//�����ݰ���Ŀ���ַ
	tx_buf[3] = len;
	memcpy(&tx_buf[4], data, len);
	tx_buf[len + 4] = 0xf5;
	if (uart)
		uart->send(uart,tx_buf, 5 + len);
	return 1;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief zigbeeSendDataMulti Ⱥ��zigbee͸������ͨ�����ڷ���ȥ����
 *
 * @param data ����
 * @param len ����
 *
 * @returns 
 */
/* ---------------------------------------------------------------------------*/
unsigned char zigbeeSendDataMulti(uint8_t* data, uint16_t len)
{
	
	unsigned char tx_buf[80];

	tx_buf[0] = 0xfa;	//��ʼ��
	*(unsigned int *)&tx_buf[1] = 0xffff;	//Ⱥ�ص�ַΪ0xffff
	tx_buf[3] = len;
	memcpy(&tx_buf[4], data, len);
	tx_buf[len + 4] = 0xf5;
	if (uart)
		uart->send(uart,tx_buf, 5 + len);
	return 1;
}

uint16_t zigbeeGetNetAddr(void)
{
	return zigbee_net_addr;
}

void zigbeeClearNetAddr(void)
{
	zigbee_net_addr = 0xffff;
}

uint8_t *zigbeeGetIEEEAddr(void)
{
	return zigbee_ieee_addr;
}
int zigbeeIsReady(void)
{
	if (get_ieee_flag
			&& zigbee_net_addr > 0x1
			&& zigbee_net_addr < 0xffff)	 {

		return 1;
	} else {
		return 0;
	}
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief routerSetting ��ģ�����ó�·������Э����
 */
/* ---------------------------------------------------------------------------*/
static void routerSetting(void)
{
	if (zigbee_type != ZIGBEE_DEVICE_TYPE)
		setSendCmd(SET_DEVICE_TYPE, ZIGBEE_DEVICE_TYPE);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief getNetNodeAddr ��ȡģ��ĵ�ǰ�̵�ַ
 */
/* ---------------------------------------------------------------------------*/
static void getNetNodeAddr(void)
{
	if((zigbee_net_addr < 0x1)||(zigbee_net_addr > 0xFFF7))
		setSendCmd(GET_NODE_ADDR, 0);
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief getNetChannel ��ȡģ�鵱ǰ�ŵ�
 */
/* ---------------------------------------------------------------------------*/
void getNetChannel(void)
{
	if(zigbee_net_channel==0xff)
		setSendCmd(GET_CHANNEL, 0);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief zigbeeModuleTask zigbee��ʱ��ѯ
 *
 * @param arg
 *
 * @returns 
 */
/* ---------------------------------------------------------------------------*/
static void *zigbeeModuleTask(void *arg)
{
	while (1) {
		if (!(send_cmd&(1 << RESUME_FACTORY_SETTINGS))) {
			routerSetting();	//ģ������Ϊ·����
			if(ZIGBEE_DEVICE_TYPE == Router) // ·�������豸
				getNetNodeAddr();
			else if (ZIGBEE_DEVICE_TYPE == Coordinator) // Э��������
				getNetChannel();
			getIEEE();		//��ȡIEEE
		}
		sendCmdProcess();
		sleep(1);
	}
	pthread_exit(NULL);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief zigbeeNetIn �����豸����(Э����)
 */
/* ---------------------------------------------------------------------------*/
void zigbeeNetIn(void)
{
	setSendCmd(GET_CHANNEL, 0xfe);
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief zigbeeOutNet �豸����(·����)
 */
/* ---------------------------------------------------------------------------*/
void zigbeeOutNet(void)
{
	setSendCmd(RESUME_FACTORY_SETTINGS, 0);
	zigbeeClearNetAddr();//�����ݲ��ڴ���ֱ���ٴ�����
}

void zigbeeSetDataRecvFunc(int (*func)(char*,int))
{
	zigbeeDataRcv = func;
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief zigbeeInit ������zigbeeͨ��ģ���ʼ��
 *
 *
 * @returns ����ָ��
 */
/* ---------------------------------------------------------------------------*/
int zigbeeInit(void)
{
	int ret = -1;
	gpioInit();
	if(uartInit(callbackProcess) == 0) {
		printf("Err:uart init fail!\n");
		goto end;
	}

	pthread_t task;
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&task, &attr, zigbeeModuleTask, NULL);
	ret = 0;
end:
	return ret;
}
