/*
 * =============================================================================
 *
 *       Filename:  smart_home_pro.c
 *
 *    Description:  智能家居相关协议
 *
 *        Version:  v1.0
 *        Created:  2018-05-04 14:10:09
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
/* ---------------------------------------------------------------------------*
 *                      include head files
 *----------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>

#include "sqlHandle.h"
#include "smart_home_pro.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
//因为要共用原来RF868智能家居的APP，而ZIGBEE是没有单元码，只有唯一地址，为实现兼容，
// (uint32_t)单元码 = (高 uint16_t)唯一地址 + (低 uint16_t) 通道数量 + 当前通道

// sendBuf_TypeDef zb_sendQueue[SEND_QUEUE_NUM];
// sendBuf_TypeDef zbSend;
// extern ipAddr_TypeDef phoneIP;

/* ---------------------------------------------------------------------------*/
/**
 * @brief smarthomeSendDataPacket 单控命令封包
 *
 * @param d_addr 目的地址
 * @param cmd 命令
 * @param ch_num 通道号
 * @param ch 通道
 * @param param 参数
 * @param param_len 参数长度
 */
/* ---------------------------------------------------------------------------*/
static void smarthomeSendDataPacket(uint16_t d_addr,
		uint8_t cmd,
	   	uint8_t ch_num,
		uint8_t ch,
	   	uint8_t* param,
	   	uint8_t param_len)
{
	char buf[128] = {0};
	SMART_HOME_PRO *packet;
	packet = (SMART_HOME_PRO *)buf;
	packet->addr = 0;  //协调器地址为0
	packet->device_type = DEVICE_TYPE_ZK;	//协调器的设备类型为0
	packet->channel_num = ch_num;	//通道数据
	packet->current_channel = ch;	//当前通道
	packet->cmd = cmd;
	
	if (param)
		memcpy(packet->param, param, param_len);
	zigbeeSendData(d_addr,buf,param_len + 6);
}

/*********************************************************************************************************
** Descriptions:       群控命令封包
** input parameters:   id 被控设备的ID   param 控制的参数
** output parameters:   buf 封好的数据包   len 数据包的长度
** Returned value:      无
*********************************************************************************************************/
void smarthomeMultiPacket(uint8_t *buf, uint8_t *len,uint8_t devNum)
{
#if 0
	MULTI_CTRL_TypeDef *multiPacket;
	multiPacket = (MULTI_CTRL_TypeDef *)&buf[3];
	
	memset(multiPacket,0,sizeof(multiPacket)*MULTI_PACKET_DEV_MAX);
	
	memset(buf, 0xff, 3);	//协议类型，群控协议固定为3个0XFF
	
	for (uint32_t i=0; i<devNum; i++)
	{
		
		EEPROM_GetDeviceInfo(multi[i].id, &sDev);
		multiPacket[i].zAddr = (uint16_t)(sDev.devUnit>>16);		//设备的ZIGBEE地址
		multiPacket[i].channel = (uint8_t)(sDev.devUnit&0xff);	//当前通道
		if (multi[i].param == 0)
		{
			multiPacket[i].cmd = Device_Off;
		}
		else
		{
			multiPacket[i].cmd = Device_On;
			multiPacket[i].param = multi[i].param;
		}
	}
	
	*len = 3+devNum*6;
#endif	
}



//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&以下是接收处理&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&

/*********************************************************************************************************
** Descriptions:      收到想要的数据后，终止发送，并且回复给APP
** input parameters:   zAddr 命令的源地址   ch 设备通道  cmd功能指令  id为收到的设备的ID号
** output parameters:   无
** Returned value:      无
*********************************************************************************************************/
void smarthomeReceiveNeeded(uint16_t zAddr, uint8_t ch, uint8_t cmd, uint8_t id)
{
#if 0
	SMART_HOME_PRO *packet;
	int32_t param[3];

	
	packet = (SMART_HOME_PRO *)zbSend.cmdData;
	
	//正在发送中的数据是否为单控指令，只有单控指令才会等待回复
	if (zbSend.dstAddr == 0xffff) //正在发送的指令为群控，不需回复
	{
		return;
	}
	
	//判断收到的指令是不是正在等待回复的指令
	if( (zAddr == packet->zAddr) && (ch == packet->current_channel) && (cmd == packet->cmd+1) )
	{
		if (zbSend.dAddr.port) //端口号为零表示不需要发送到网络
		{
			param[0] = 0; //控制成功
			param[1] = id;//设备的ID号
			CtrlDeviceRep(param,zbSend.TAG,&zbSend.dAddr,&zbSend.tAddr);
		}
		
		memset( &zbSend,0,sizeof(zbSend) );	//终止发送此命令
	}
	else
	{	//发送到最后一次控制的手机端，不过手机有可能收不到
		param[0] = 0; //控制成功
		param[1] = id;//设备的ID号
		CtrlDeviceRep(param,zbSend.TAG,&phoneIP,&zbSend.tAddr);
	}
#endif
}

/*********************************************************************************************************
** Descriptions:       在ZIGBEE列表中删除设备
** input parameters:  devUnit设备的单元码
** output parameters:   无
** Returned value:      1成功
*********************************************************************************************************/
uint32_t smarthomeDelDev(uint32_t devUnit)
{
#if 0
	uint16_t zAddr;
	uint8_t ch;
	zbDev_TypeDef addrBuf[MAX_REGIST_DEVICE];
	
	EEPROM_GetZbAddr((uint8_t *)addrBuf);	//读出所有设备的地址及通道信息
	
	zAddr = (uint16_t)(devUnit>>16); //从单元码中提取地址
	ch = (uint8_t)(devUnit&0xff);	//提取出当前通道
	
	for (int i=0; i<MAX_REGIST_DEVICE; i++)
	{
		if( (zAddr == addrBuf[i].zAddr) && (ch == addrBuf[i].channel) )
		{
			addrBuf[i].zAddr = 0;
			addrBuf[i].channel = 0;
			addrBuf[i].id = 0;
		}
	}
	EEPROM_WriteZbAddr((uint8_t *)addrBuf);
#endif
	return 1;
}

/*********************************************************************************************************
** Descriptions:       添加新设备
** input parameters:  
** output parameters:   无
** Returned value:      1成功
*********************************************************************************************************/
uint32_t smarthomeAddNewDev(SMART_HOME_PRO *cmdBuf)
{
#if 0
	char *str;
	char num[6];
	char name[16];
	
	uint32_t devSum;
	uint8_t i,j;
	znBoxDevInfo_TypeDef dev;
	zbDev_TypeDef addrBuf[MAX_REGIST_DEVICE];
	
	EEPROM_GetZbAddr((uint8_t *)addrBuf);	//读出所有设备的地址及通道信息
	EEPROM_GetDeviceSum(&devSum);
	
	memset(&dev,0,sizeof(dev));
	
	//判断该设备是否已存在
/*	for (i=2; i<MAX_REGIST_DEVICE; i++)
	{
		if (addrBuf[i].zAddr == cmdBuf->zAddr)
		{
			return 0;	//已存在不重新添加
		}
	}*/
	
	for (j=0; j<cmdBuf->channel_num; j++)		//如果一个设备包含有多个通道的话，每个通道进行添加
	{
		if(devSum >= MAX_REGIST_DEVICE)
		{
			return 0;
		}

		for (i=2; i<MAX_REGIST_DEVICE; i++)	//id 1被本身插座占用
		{
			if( (addrBuf[i].zAddr == 0xffff) || (addrBuf[i].zAddr == 0)  //搜索列表中空的位置
			  || ((addrBuf[i].zAddr == cmdBuf->zAddr) && (addrBuf[i].channel == j+1)) )	//列表中已用相同的设备也要覆盖，用于退网重新加网
			{
				//把新加的设备保存到列表中（此列表用来实现ZIGBEE地址和ID的转换）
				addrBuf[i].zAddr = cmdBuf->zAddr; 
			//	dev.deviceID = i; oujie modify
				//(uint32_t)单元码 = (高 uint16_t)唯一地址 + (低 uint16_t) 通道数量 + 当前通道
				dev.devUnit = ((uint32_t)cmdBuf->zAddr)<<16 | ((uint32_t)cmdBuf->channel_num)<<8 | ((uint32_t)j+1);
				dev.devType = cmdBuf->device_type;
				dev.status = 0;
				dev.roomID = 1;

				switch (cmdBuf->device_type)
				{
				case DEVICE_TYPE_DK:				// 灯控开关
#ifdef FORGIGN
					str = "Light";
#else
					str = "灯光";
#endif
					break;
				case DEVICE_TYPE_TG:				// 调光开关
#ifdef FORGIGN
					str = "A Light";
#else
					str = "调光";
#endif
					break;
				case DEVICE_TYPE_KT:				// 空调开关
#ifdef FORGIGN
					str = "AC";
#else
					str = "空调";
#endif
					break;
				case DEVICE_TYPE_CZ:						// 插座
#ifdef FORGIGN
					str = "Socket";
#else
					str = "插座";
#endif
					break;
				case DEVICE_TYPE_CL:						// 窗帘
#ifdef FORGIGN
					str = "Curtain";
#else
					str = "窗帘";
#endif
					break;

				case DEVICE_TYPE_JJ:						// 紧急按钮
#ifdef FORGIGN
					str = "SOS.Button";
#else
					str = "紧急按钮";
#endif
					break;
				case DEVICE_TYPE_HW:						// 红外报警
#ifdef FORGIGN
					str = "PIR";
#else
					str = "红外";
#endif
					break;
				case DEVICE_TYPE_YW:						// 烟雾报警
#ifdef FORGIGN
					str = "Smoke sensor";
#else
					str = "烟雾";
#endif
					break;
				case DEVICE_TYPE_WS:						// 瓦斯报警
#ifdef FORGIGN
					str = "Smoke sensor";
#else
					str = "瓦斯";
#endif
					break;
				case DEVICE_TYPE_MC:						// 门磁报警
#ifdef FORGIGN
					str = "Door sensor";
#else
					str = "门磁";
#endif
					break;

				case DEVICE_TYPE_CC:						// 窗磁报警
#ifdef FORGIGN
					str = "W sensor";
#else
					str = "窗磁";
#endif
					break;
				case DEVICE_TYPE_TY:						// 通用报警
#ifdef FORGIGN
					str = "G";
#else
					str = "探头";
#endif
					break;
				case DEVICE_TYPE_DS:						// 电视控制器
#ifdef FORGIGN
					str = "TV";
#else
					str = "电视";
#endif
					break;
				case DEVICE_TYPE_WXHW:					// 无线红外
#ifdef FORGIGN
					str = "PIR";
#else
					str = "无线红外";
#endif
					break;
				case DEVICE_TYPE_QJ:						// 情景控制器
#ifdef FORGIGN
					str = "Scene";
#else
					str = "情景控制器";
#endif
					break;

				case DEVICE_TYPE_DHX:					// 单火线灯控
#ifdef FORGIGN
					str = "S Light";
#else
					str = "单火线灯控";
#endif
					break;
				case DEVICE_TYPE_LED:						// LED控制器
					str = "LED";
					break;
				case DEVICE_TYPE_ZYKT:					// 中央空调
#ifdef FORGIGN
					str = "C AC";
#else
					str = "中央空调";
#endif

				case DEVICE_TYPE_SGBJQ:					// 声光报警器
#ifdef FORGIGN
					str = "Alertor";
#else
					str = "声光报警器";
#endif
					break;
				}

				memset(name,0,sizeof(name));
				strcpy(name,str);
				sprintf(num,(char *)"%d",i);	//
				strcat(name,num);	//函数在string.h
				memcpy(dev.devName,name,sizeof(dev.devName));
				dev.X = 0;
				dev.Y = 0;
//				EEPROM_WriteDeviceInfo(i,&dev);// oujie modify
				
				if ( !((addrBuf[i].zAddr == cmdBuf->zAddr) && (addrBuf[i].channel == j+1)) )
				{
					dev.deviceID = EEPROM_GetEmptyDeviceId();
					EEPROM_WriteDeviceInfo(dev.deviceID,&dev);// oujie modify	
					devSum++;
					EEPROM_WriteDeviceSum(devSum);			
					addrBuf[i].id = dev.deviceID;					
				}
				addrBuf[i].channel = j+1;
				break;
				
			}
		}
	}
	EEPROM_WriteZbAddr((uint8_t *)addrBuf);
#endif
	return 1;
}

/*********************************************************************************************************
** Descriptions:       协议解析
** input parameters:   id 为设备的序列号， param 命令参数，dAddr  tAddr TAG 为网络控制时用，
** output parameters:   无
** Returned value:      无
*********************************************************************************************************/
static void smarthomeRecieve(uint8_t *buf, uint8_t len)
{
	zbDev_TypeDef addrBuf[MAX_REGIST_DEVICE];
	SMART_HOME_PRO *packet;
	packet = (SMART_HOME_PRO *)buf;
	uint8_t id[8],idCnt = 0;
	// znBoxDevInfo_TypeDef dev;
	uint8_t i,j;
	static uint8_t id_bak = 0;
	static uint8_t cmd_bak = Cmd_Null;
	
	EEPROM_GetZbAddr((uint8_t *)addrBuf);	//读出所有设备的地址及通道信息
	//先根据短地址和通道找出对应的ID
	memset(id,0,sizeof(id));

#if 1
	if( ((packet->current_channel == 0xff) || (packet->current_channel == 0)) && (packet->cmd != NetIn_Report) )	//此指令为所有通道操作
	{
		for (j=0; j<packet->channel_num; j++)
		{
			for (i=2; i<MAX_REGIST_DEVICE; i++)	//id 1被本身插座占用
			{
				if( (addrBuf[i].zAddr == packet->addr) && (addrBuf[i].channel == packet->param[j*2+1]) )
				{
					id[idCnt] = addrBuf[i].id;
					idCnt++;
					break;
				}
			}
		}
	}
	else
	{
		for (i=2; i<MAX_REGIST_DEVICE; i++)	//id 1被本身插座占用
		{
			if( (addrBuf[i].zAddr == packet->addr) && (addrBuf[i].channel == packet->current_channel) )
			{
				id[0] = addrBuf[i].id;
				break;
			}
		}		
		if (i >= MAX_REGIST_DEVICE)	//设备列表中没有找到此设备
		{
			if(packet->cmd == NetIn_Report)	//此设备要入网？
			{
				smarthomeAddNewDev(packet);
				memset(&sendBuf,0,sizeof(sendBuf));
				memset(&dev,0,sizeof(dev));
				sendBuf.dstAddr = packet->addr;
				sendBuf.cnt = 1;
				dev.devUnit = 0x3ff;
				smarthomSinglePacket( &sendBuf, &dev,NetIn_Report_Res, 0);
				smarthomLoadCmdToQueue(&sendBuf);	
				sendBuf.cnt = 1;
				dev.devUnit = 0x300;
				smarthomSinglePacket( &sendBuf, &dev,Demand_Sw_Status, 0);
				smarthomLoadCmdToQueue(&sendBuf);	
				ZnBoxDevAddOk(&phoneIP);
			}
			return;	//找不到对应的设备
		}
	}
#endif	
	if((cmd_bak == packet->cmd)&&(id_bak == id[0])&&(packet->cmd != Demand_Sw_Status_Res))
		return;
	else
	{
		cmd_bak = packet->cmd;
		id_bak = id[0];
	}
	EEPROM_GetDeviceInfo(id[0], &dev);
	switch (packet->cmd)
	{
		case NetOut_Report_Res:	//有设备要退网
			break;	
			
		case Demand_Sw_Status_Res:	//开关状态返回
			for (i=0; i<idCnt; i++)
			{
				if(packet->param[i*2] > 100)
					packet->param[i*2] = 100;
				dev.status = packet->param[i*2];
				EEPROM_WriteDeviceInfo(id[i],&dev);	//更新EEPROM中的状态
				EEPROM_GetDeviceInfo(id[i+1], &dev);	//读出下一个待处理设备信息
			}
			break;
		
		case Device_On_Res:		//设备开
			if(packet->param[0] > 100)
				packet->param[0] = 100;
			dev.status = packet->param[0];
			EEPROM_WriteDeviceInfo(id[0],&dev);	//更新EEPROM中的状态
			
			smarthomeReceiveNeeded(packet->addr,packet->current_channel,Device_On_Res,id[0]);	//
			
			Linkage(id[0],1);	//1 《MINI网关通信协议》5 设备联动
			break;
			
		case Device_Off_Res:	//设备关
			dev.status = 0;
			EEPROM_WriteDeviceInfo(id[0],&dev);	//更新EEPROM中的状态
			
			smarthomeReceiveNeeded(packet->addr,packet->current_channel,Device_Off_Res,id[0]);	//
			
			Linkage(id[0],0);	//1 《MINI网关通信协议》5 设备联动
			break;
		
		case Device_Scene:		//情景控制
			SceneStart(packet->param[0],1);
			break;
			
	default:
		break;
	}
}

static void *smarthomeThread(void *arg)
{

	while (1) {
		// smarthomeNetInReport();
		// smarthomOutHandle();
		// smarthomeCheckTemprature();
		usleep(10000);
	}
	pthread_exit(NULL);
}

void smarthomeInit(void)                                        
{                                                               
    zigbeeSetDataRecvFunc(smarthomeRecieve);                    
                                                                
    pthread_t task;                                             
    pthread_attr_t attr;                                        
                                                                
    pthread_attr_init(&attr);                                   
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&task, &attr, smarthomeThread, NULL);        
}                                                               



