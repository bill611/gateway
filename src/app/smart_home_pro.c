/*
 * =============================================================================
 *
 *       Filename:  smart_home_pro.c
 *
 *    Description:  ���ܼҾ����Э��
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

#include "sql_handle.h"
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
//��ΪҪ����ԭ��RF868���ܼҾӵ�APP����ZIGBEE��û�е�Ԫ�룬ֻ��Ψһ��ַ��Ϊʵ�ּ��ݣ�
// (uint32_t)��Ԫ�� = (�� uint16_t)Ψһ��ַ + (�� uint16_t) ͨ������ + ��ǰͨ��

// sendBuf_TypeDef zb_sendQueue[SEND_QUEUE_NUM];
// sendBuf_TypeDef zbSend;
// extern ipAddr_TypeDef phoneIP;

/* ---------------------------------------------------------------------------*/
/**
 * @brief smarthomeSendDataPacket ����������
 *
 * @param d_addr Ŀ�ĵ�ַ
 * @param cmd ����
 * @param ch_num ͨ����
 * @param ch ͨ��
 * @param param ����
 * @param param_len ��������
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
	packet->addr = 0;  //Э������ַΪ0
	packet->device_type = DEVICE_TYPE_ZK;	//Э�������豸����Ϊ0
	packet->channel_num = ch_num;	//ͨ������
	packet->current_channel = ch;	//��ǰͨ��
	packet->cmd = cmd;
	
	if (param)
		memcpy(packet->param, param, param_len);
	zigbeeSendData(d_addr,buf,param_len + 6);
}

/*********************************************************************************************************
** Descriptions:       Ⱥ��������
** input parameters:   id �����豸��ID   param ���ƵĲ���
** output parameters:   buf ��õ����ݰ�   len ���ݰ��ĳ���
** Returned value:      ��
*********************************************************************************************************/
void smarthomeMultiPacket(uint8_t *buf, uint8_t *len,uint8_t devNum)
{
#if 0
	MULTI_CTRL_TypeDef *multiPacket;
	multiPacket = (MULTI_CTRL_TypeDef *)&buf[3];
	
	memset(multiPacket,0,sizeof(multiPacket)*MULTI_PACKET_DEV_MAX);
	
	memset(buf, 0xff, 3);	//Э�����ͣ�Ⱥ��Э��̶�Ϊ3��0XFF
	
	for (uint32_t i=0; i<devNum; i++)
	{
		
		EEPROM_GetDeviceInfo(multi[i].id, &sDev);
		multiPacket[i].addr = (uint16_t)(sDev.devUnit>>16);		//�豸��ZIGBEE��ַ
		multiPacket[i].channel = (uint8_t)(sDev.devUnit&0xff);	//��ǰͨ��
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



//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&�����ǽ��մ���&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&

/*********************************************************************************************************
** Descriptions:      �յ���Ҫ�����ݺ���ֹ���ͣ����һظ���APP
** input parameters:   addr �����Դ��ַ   ch �豸ͨ��  cmd����ָ��  idΪ�յ����豸��ID��
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
void smarthomeReceiveNeeded(uint16_t addr, uint8_t ch, uint8_t cmd, uint8_t id)
{
#if 0
	SMART_HOME_PRO *packet;
	int32_t param[3];

	
	packet = (SMART_HOME_PRO *)zbSend.cmdData;
	
	//���ڷ����е������Ƿ�Ϊ����ָ�ֻ�е���ָ��Ż�ȴ��ظ�
	if (zbSend.dstAddr == 0xffff) //���ڷ��͵�ָ��ΪȺ�أ�����ظ�
	{
		return;
	}
	
	//�ж��յ���ָ���ǲ������ڵȴ��ظ���ָ��
	if( (addr == packet->addr) && (ch == packet->current_channel) && (cmd == packet->cmd+1) )
	{
		if (zbSend.dAddr.port) //�˿ں�Ϊ���ʾ����Ҫ���͵�����
		{
			param[0] = 0; //���Ƴɹ�
			param[1] = id;//�豸��ID��
			CtrlDeviceRep(param,zbSend.TAG,&zbSend.dAddr,&zbSend.tAddr);
		}
		
		memset( &zbSend,0,sizeof(zbSend) );	//��ֹ���ʹ�����
	}
	else
	{	//���͵����һ�ο��Ƶ��ֻ��ˣ������ֻ��п����ղ���
		param[0] = 0; //���Ƴɹ�
		param[1] = id;//�豸��ID��
		CtrlDeviceRep(param,zbSend.TAG,&phoneIP,&zbSend.tAddr);
	}
#endif
}

/*********************************************************************************************************
** Descriptions:       ��ZIGBEE�б���ɾ���豸
** input parameters:  devUnit�豸�ĵ�Ԫ��
** output parameters:   ��
** Returned value:      1�ɹ�
*********************************************************************************************************/
uint32_t smarthomeDelDev(uint32_t devUnit)
{
#if 0
	uint16_t addr;
	uint8_t ch;
	zbDev_TypeDef addrBuf[MAX_REGIST_DEVICE];
	
	EEPROM_GetZbAddr((uint8_t *)addrBuf);	//���������豸�ĵ�ַ��ͨ����Ϣ
	
	addr = (uint16_t)(devUnit>>16); //�ӵ�Ԫ������ȡ��ַ
	ch = (uint8_t)(devUnit&0xff);	//��ȡ����ǰͨ��
	
	for (int i=0; i<MAX_REGIST_DEVICE; i++)
	{
		if( (addr == addrBuf[i].addr) && (ch == addrBuf[i].channel) )
		{
			addrBuf[i].addr = 0;
			addrBuf[i].channel = 0;
			addrBuf[i].id = 0;
		}
	}
	EEPROM_WriteZbAddr((uint8_t *)addrBuf);
#endif
	return 1;
}

/*********************************************************************************************************
** Descriptions:       ������豸
** input parameters:  
** output parameters:   ��
** Returned value:      1�ɹ�
*********************************************************************************************************/
static char* smarthomeAddNewDev(SMART_HOME_PRO *cmdBuf)
{
	static char id[32];
	sprintf(id,"%08x",cmdBuf->addr);
	sqlInsertDevice(id,
			cmdBuf->device_type,
			cmdBuf->addr,
			cmdBuf->channel_num);
	return id;
}

/*********************************************************************************************************
** Descriptions:       Э�����
** input parameters:   id Ϊ�豸�����кţ� param ���������dAddr  tAddr TAG Ϊ�������ʱ�ã�
** output parameters:   ��
** Returned value:      ��
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
	
	// EEPROM_GetZbAddr((uint8_t *)addrBuf);	//���������豸�ĵ�ַ��ͨ����Ϣ
	//�ȸ��ݶ̵�ַ��ͨ���ҳ���Ӧ��ID
	memset(id,0,sizeof(id));

#if 1
	if( ((packet->current_channel == 0xff) || (packet->current_channel == 0)) 
			&& (packet->cmd != NetIn_Report) )	//��ָ��Ϊ����ͨ������
	{
		for (j=0; j<packet->channel_num; j++)
		{
			for (i=2; i<MAX_REGIST_DEVICE; i++)	//id 1���������ռ��
			{
				if( (addrBuf[i].addr == packet->addr) && (addrBuf[i].channel == packet->param[j*2+1]) )
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
		for (i=2; i<MAX_REGIST_DEVICE; i++)	//id 1���������ռ��
		{
			if( (addrBuf[i].addr == packet->addr) && (addrBuf[i].channel == packet->current_channel) )
			{
				id[0] = addrBuf[i].id;
				break;
			}
		}		
		if (i >= MAX_REGIST_DEVICE)	//�豸�б���û���ҵ����豸
		{
			if(packet->cmd == NetIn_Report)	//���豸Ҫ������
			{
				char *id = smarthomeAddNewDev(packet);
				smarthomeSendDataPacket(
						packet->addr,
						NetIn_Report_Res,
						packet->channel_num,
						packet->current_channel,
						NULL,0);
				smarthomeSendDataPacket(
						packet->addr,
						Demand_Sw_Status,
						packet->channel_num,
						packet->current_channel,
						NULL,0);
				registerSubDevice(id,packet->device_type);
			}
			return;	//�Ҳ�����Ӧ���豸
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
	// EEPROM_GetDeviceInfo(id[0], &dev);
	switch (packet->cmd)
	{
		case NetOut_Report_Res:	//���豸Ҫ����
			break;	
			
		case Demand_Sw_Status_Res:	//����״̬����
			// for (i=0; i<idCnt; i++)
			// {
				// if(packet->param[i*2] > 100)
					// packet->param[i*2] = 100;
				// dev.status = packet->param[i*2];
				// EEPROM_WriteDeviceInfo(id[i],&dev);	//����EEPROM�е�״̬
				// EEPROM_GetDeviceInfo(id[i+1], &dev);	//������һ���������豸��Ϣ
			// }
			break;
		
		case Device_On_Res:		//�豸��
			// if(packet->param[0] > 100)
				// packet->param[0] = 100;
			// dev.status = packet->param[0];
			// EEPROM_WriteDeviceInfo(id[0],&dev);	//����EEPROM�е�״̬
			
			// smarthomeReceiveNeeded(packet->addr,packet->current_channel,Device_On_Res,id[0]);	//
			
			// Linkage(id[0],1);	//1 ��MINI����ͨ��Э�顷5 �豸����
			break;
			
		case Device_Off_Res:	//�豸��
			// dev.status = 0;
			// EEPROM_WriteDeviceInfo(id[0],&dev);	//����EEPROM�е�״̬
			
			// smarthomeReceiveNeeded(packet->addr,packet->current_channel,Device_Off_Res,id[0]);	//
			
			// Linkage(id[0],0);	//1 ��MINI����ͨ��Э�顷5 �豸����
			break;
		
		case Device_Scene:		//�龰����
			// SceneStart(packet->param[0],1);
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
	sqlInit();
    zigbeeSetDataRecvFunc(smarthomeRecieve);                    
                                                                
    pthread_t task;                                             
    pthread_attr_t attr;                                        
                                                                
    pthread_attr_init(&attr);                                   
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&task, &attr, smarthomeThread, NULL);        
}                                                               



