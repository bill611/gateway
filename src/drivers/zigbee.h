/*
 * =====================================================================================
 *
 *       Filename:  zigbee.h
 *
 *    Description:  zigbee模块处理
 *
 *        Version:  1.0
 *        Created:  2018-04-04
 *       Revision:  none
 *
 *         Author:  zzw
 *        Company:  Taichuan
 *
 * =====================================================================================
 */
#ifndef _ZIGBEE_H
#define _ZIGBEE_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */
#define ZIGBEE_UART_MAX	70		//模块一帧数据最大长度
#define BUFSIZE          1024
#define ZIGBEE_BROAD_CAST_ADDR      0xFFFF

	enum { 
		Coordinator,
	   	Router 
	};

// #define ZIGBEE_DEVICE_TYPE  Router
#define ZIGBEE_DEVICE_TYPE  Coordinator

	typedef enum
	{
		GET_DEVICE_TYPE,
		SET_DEVICE_TYPE,
		GET_NET_ID,
		SET_NET_ID,
		GET_NODE_ADDR,
		GET_CHANNEL,
		SET_CHANNEL,
		GET_IEEE_ADDR,
		RESUME_FACTORY_SETTINGS,
		SET_NETIN_ENABLE,
		SET_NET_SECRETKEY,
		RESUME_NODE_SETTING,
		CMD_NUM
	}cmd_in_bit_TypeDef;	//用于标志指令，第一BIT表示一个命令，节省内存空间

	enum
	{
		GET_DEVICE_TYPE_VALUE = 0x10,
		SET_DEVICE_TYPE_VALUE = 0x11,
		GET_NET_ID_VALUE = 0x20,
		SET_NET_ID_VALUE = 0x21,
		GET_NODE_ADDR_VALUE = 0x30,
		GET_CHANNEL_VALUE = 0x40,
		SET_CHANNEL_VALUE = 0x41,
		GET_IEEE_ADDR_VALUE = 0x60,
		RESUME_FACTORY_SETTINGS_VALUE = 0xF0,
		SET_NETIN_ENABLE_VALUE = 0xF1,
		SET_NET_SECRETKEY_VALUE = 0xF2,
		RESUME_NODE_SETTING_VALUE = 0xF3,
	};//模块的命令




	int zigbeeInit(void);
	int zigbeeSetVentiMode(int index);
#define CLEAR_ZIGBEE_NET_ADDR()			(zigbee_net_addr = 0xffff)

extern uint16_t zigbeeGetNetAddr(void);
extern void zigbeeClearNetAddr(void);
extern int zigbeeIsReady(void);
extern uint8_t *zigbeeGetIEEEAddr(void);

extern unsigned char zigbeeSendData(uint16_t dst_addr, uint8_t* data, uint16_t len);
extern void zigbeeSetDataRecvFunc(void (*func)(uint8_t*,uint8_t));
extern void zigbeeNetIn(uint8_t time);
extern int zigbeeGetNetInStatus();
extern void zigbeeFactorySet(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif

