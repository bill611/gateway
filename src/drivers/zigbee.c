/*
 * =============================================================================
 *
 *       Filename:  UartZigbee.c
 *
 *    Description:  解析串口协议(发送及接收)
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
static unsigned char zigbee_type = 0xff;		//设备类型 0--协调器  1--路由器  2--终端节目
static char cmd_param[CMD_NUM];
static char get_ieee_flag = 0;			//IEEE地址获取完成标志
static short int zigbee_net_id = 0;		//PAN ID
static uint16_t zigbee_net_addr = 0xffff;	//短地址
static uint8_t zigbee_net_channel = 0xff;		//信道
static uint8_t zigbee_ieee_addr[8] = { 0 };	//IEEE地址

static int (*zigbeeDataRcv)(char* buf, int len) = NULL;
/*********************************************************************************************************
** Descriptions:      清除命令队列中指定的命令（停止发送）
** input parameters:   cmd 要清除的命今（对应在的BIT位）
** output parameters:  -
** Returned value:     -
*********************************************************************************************************/
static void clearSendCmd(int cmd)
{
	send_cmd &= ~(1 << cmd);
}

/*********************************************************************************************************
** Descriptions:       命令封包，并通过串口发送出去
** input parameters:   cmd命令字，data命令的参数， len参数的长度
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
** Descriptions:      zigbee命令发送，主循环中定时调用
** input parameters:   cmd 要清除的命今（对应在的BIT位）
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
 * @brief callbackProcess 接收到Uart来的消息
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

	if (buff[0] == 0xfa)	//透传数据处理
	{
		if (zigbeeDataRcv)
			zigbeeDataRcv(&buff[4], len);
	}
	else if (buff[0] == 0xfc)	//命令数据处理
	{
		cmdParser(buff, len);
	} else {
		printf("zigbee cmd err\n");
	}

}


/*********************************************************************************************************
** Descriptions:       命令加载到发送队列中，队列用send_cmd表示，每一BIT表示一种命令
** input parameters:   cmd 要发送的命今（对应在的BIT位）  param 命令参数
** output parameters:  -
** Returned value:     -
*********************************************************************************************************/
static void setSendCmd(cmd_in_bit_TypeDef cmd, unsigned param)
{
	send_cmd |= (1 << cmd);
	cmd_param[cmd] = param;
}

/*********************************************************************************************************
** Descriptions:	   获取模块的IEEE地址
** input parameters:    无
** output parameters:   无
** Returned value:      无
*********************************************************************************************************/
static void getIEEE(void)
{
	if (!get_ieee_flag)
		setSendCmd(GET_IEEE_ADDR, 0);
}
/*********************************************************************************************************
** Descriptions:       解析zigbee模块返回的命令
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
		case 0x12://设备类型返回
			if (send_cmd&(1 << GET_DEVICE_TYPE))
				clearSendCmd(GET_DEVICE_TYPE);
			else if (send_cmd&(1 << SET_DEVICE_TYPE))
				clearSendCmd(SET_DEVICE_TYPE);

			zigbee_type = buf[3];
			break;
		case 0x22://网络号
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
		case 0x32://节点地址返回
			if (send_cmd&(1 << GET_NODE_ADDR))
				clearSendCmd(GET_NODE_ADDR);

#ifdef MCU_BIFG_MODE
			zigbee_net_addr = ((uint16_t)buf[3]) | ((uint16_t)buf[4] << 8);
#else
			zigbee_net_addr = *((short int*)(&buf[3]));
#endif
			break;
		case 0x42://通讯信道返回
			if (send_cmd&(1 << GET_CHANNEL))
				clearSendCmd(GET_CHANNEL);
			else if (send_cmd&(1 << SET_CHANNEL))
				clearSendCmd(SET_CHANNEL);

			zigbee_net_channel = buf[3];
			break;
		case 0x62://获取IEEE返回
			if (send_cmd&(1 << GET_IEEE_ADDR))
				clearSendCmd(GET_IEEE_ADDR);
			memcpy(zigbee_ieee_addr, &buf[3], sizeof(zigbee_ieee_addr));
			get_ieee_flag = 1;
			break;
		case 0xff://成功返回
			if (send_cmd&(1 << RESUME_FACTORY_SETTINGS))	//恢复出厂设置
			{
				clearSendCmd(RESUME_FACTORY_SETTINGS);
				zigbee_type = 0xff;
				zigbeeClearNetAddr();
			}
			else if (send_cmd&(1 << SET_NETIN_ENABLE))	//设置入网允许
				clearSendCmd(SET_NETIN_ENABLE);
			else if (send_cmd&(1 << SET_NET_SECRETKEY))
				clearSendCmd(SET_NET_SECRETKEY);
			else if (send_cmd&(1 << RESUME_NODE_SETTING))
				clearSendCmd(RESUME_NODE_SETTING);
			break;
		case 0x00://复位状态包
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
 * @brief zigbeeSendData 单控zigbee透传数据通过串口发送去出
 *
 * @param dst_addr 目标地址
 * @param data 发送的数据
 * @param len 数据长度
 *
 * @returns 
 */
/* ---------------------------------------------------------------------------*/
unsigned char zigbeeSendData(uint16_t dst_addr, uint8_t* data, uint16_t len)
{
	unsigned char tx_buf[80];

	tx_buf[0] = 0xfa;	//起始符
	*(unsigned int *)&tx_buf[1] = dst_addr;	//本数据包的目标地址
	tx_buf[3] = len;
	memcpy(&tx_buf[4], data, len);
	tx_buf[len + 4] = 0xf5;
	if (uart)
		uart->send(uart,tx_buf, 5 + len);
	return 1;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief zigbeeSendDataMulti 群控zigbee透传数据通过串口发送去出，
 *
 * @param data 数据
 * @param len 长度
 *
 * @returns 
 */
/* ---------------------------------------------------------------------------*/
unsigned char zigbeeSendDataMulti(uint8_t* data, uint16_t len)
{
	
	unsigned char tx_buf[80];

	tx_buf[0] = 0xfa;	//起始符
	*(unsigned int *)&tx_buf[1] = 0xffff;	//群控地址为0xffff
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
 * @brief routerSetting 把模块配置成路由器或协调器
 */
/* ---------------------------------------------------------------------------*/
static void routerSetting(void)
{
	if (zigbee_type != ZIGBEE_DEVICE_TYPE)
		setSendCmd(SET_DEVICE_TYPE, ZIGBEE_DEVICE_TYPE);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief getNetNodeAddr 获取模块的当前短地址
 */
/* ---------------------------------------------------------------------------*/
static void getNetNodeAddr(void)
{
	if((zigbee_net_addr < 0x1)||(zigbee_net_addr > 0xFFF7))
		setSendCmd(GET_NODE_ADDR, 0);
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief getNetChannel 获取模块当前信道
 */
/* ---------------------------------------------------------------------------*/
void getNetChannel(void)
{
	if(zigbee_net_channel==0xff)
		setSendCmd(GET_CHANNEL, 0);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief zigbeeModuleTask zigbee定时查询
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
			routerSetting();	//模块设置为路由器
			if(ZIGBEE_DEVICE_TYPE == Router) // 路由器子设备
				getNetNodeAddr();
			else if (ZIGBEE_DEVICE_TYPE == Coordinator) // 协调器网关
				getNetChannel();
			getIEEE();		//获取IEEE
		}
		sendCmdProcess();
		sleep(1);
	}
	pthread_exit(NULL);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief zigbeeNetIn 允许设备入网(协调器)
 */
/* ---------------------------------------------------------------------------*/
void zigbeeNetIn(void)
{
	setSendCmd(GET_CHANNEL, 0xfe);
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief zigbeeOutNet 设备退网(路由器)
 */
/* ---------------------------------------------------------------------------*/
void zigbeeOutNet(void)
{
	setSendCmd(RESUME_FACTORY_SETTINGS, 0);
	zigbeeClearNetAddr();//收数据不在处理，直到再次入网
}

void zigbeeSetDataRecvFunc(int (*func)(char*,int))
{
	zigbeeDataRcv = func;
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief zigbeeInit 串口与zigbee通信模块初始化
 *
 *
 * @returns 对象指针
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
