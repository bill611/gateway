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

#include "zigbee.h"
#include "gateway.h"
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
#define VAILDTIME 	(200)				//200ms
typedef struct _PacketsID {
	uint16_t addr;  //�̵�ַ
	uint16_t current_channel; // ��ǰ����ͨ��
	uint8_t cmd; // ��ǰ����ͨ��
	uint64_t dwTick;		//ʱ��
}PacketsID;

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
//��ΪҪ����ԭ��RF868���ܼҾӵ�APP����ZIGBEE��û�е�Ԫ�룬ֻ��Ψһ��ַ��Ϊʵ�ּ��ݣ�
// (uint32_t)��Ԫ�� = (�� uint16_t)Ψһ��ַ + (�� uint16_t) ͨ������ + ��ǰͨ��

// sendBuf_TypeDef zb_sendQueue[SEND_QUEUE_NUM];
// sendBuf_TypeDef zbSend;
// extern ipAddr_TypeDef phoneIP;

static PacketsID packets_id[10];
static int packet_pos;
/* ---------------------------------------------------------------------------*/
/**
 * @brief smarthomeSendDataPacket ����������
 *
 * @param d_addr Ŀ�ĵ�ַ
 * @param cmd ����
 * @param device_type �豸����
 * @param ch_num ͨ����
 * @param ch ͨ��
 * @param param ����
 * @param param_len ��������
 */
/* ---------------------------------------------------------------------------*/
static void smarthomeSendDataPacket(
		uint16_t d_addr,
		uint8_t cmd,
		uint8_t device_type,
	   	uint8_t ch_num,
		uint8_t ch,
	   	uint8_t* param,
	   	uint8_t param_len)
{
	uint8_t buf[128] = {0};
	SMART_HOME_PRO *packet;
	packet = (SMART_HOME_PRO *)buf;
	packet->addr = 0;  //Э������ַΪ0
	packet->device_type = device_type;	//Э�������豸����Ϊ0
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
static char* smarthomeAddNewDev(SMART_HOME_PRO *cmdBuf,char *id)
{
	printf("id:%s,type:%d,addr:%x\n", id,cmdBuf->device_type,cmdBuf->addr);
	sqlInsertDevice(id,
			cmdBuf->device_type,
			cmdBuf->addr,
			cmdBuf->channel_num);
	return id;
}

static int smarthomeCmdFilter(SMART_HOME_PRO *data)
{
	int i;
	uint64_t dwTick;
	if (data == NULL) {
		return 0;
	}
    //�жϰ��Ƿ��ط�
    dwTick = GetMs();
    for(i=0;i<10;i++) {
        if (data->addr == packets_id[i].addr
                 && data->current_channel == packets_id[i].current_channel
				 && data->cmd == packets_id[i].cmd
				 && (getDiffSysTick(dwTick,packets_id[i].dwTick) < VAILDTIME)){
            // saveLog("Packet ID %d is already receive,diff:%d!\n",
                    // packets_id[i].id,getDiffSysTick(dwTick,packets_id[i].dwTick));
            return 0;
        }
    }

	//����ID
    packets_id[packet_pos].addr = data->addr;
	packets_id[packet_pos].cmd = data->cmd;
	packets_id[packet_pos].current_channel = data->current_channel;
	packets_id[packet_pos++].dwTick = dwTick;
	packet_pos %= 10;
    return 1;
}

static void smarthomeGetId(SMART_HOME_PRO *cmdBuf,char *id)
{
	if (!id)
		return;
	sqlGetDeviceId(cmdBuf->addr,id);
}
/*********************************************************************************************************
** Descriptions:       Э�����
** input parameters:   id Ϊ�豸�����кţ� param ���������dAddr  tAddr TAG Ϊ�������ʱ�ã�
** output parameters:   ��
** Returned value:      ��
*********************************************************************************************************/
static void smarthomeRecieve(uint8_t *buf, uint8_t len)
{
	SMART_HOME_PRO *packet = (SMART_HOME_PRO *)buf;
    if (smarthomeCmdFilter(packet) == 0)
        return;
	switch (packet->cmd)
	{
		case NetIn_Report:
			{
				char id[32] = {0};
				sprintf(id,"%02X%02X%02X%02X%02X%02X%02X%02X",
						packet->param[0],
						packet->param[1],
						packet->param[2],
						packet->param[3],
						packet->param[4],
						packet->param[5],
						packet->param[6],
						packet->param[7]);
				smarthomeSendDataPacket(
						packet->addr,
						NetIn_Report_Res,
						packet->device_type,
						packet->channel_num,
						packet->current_channel,
						NULL,0);
				smarthomeSendDataPacket(
						packet->addr,
						Demand_Sw_Status,
						packet->device_type,
						packet->channel_num,
						packet->current_channel,
						NULL,0);
				int ret = gwRegisterSubDevice(id,packet->device_type,packet->addr,packet->channel_num);
				if (ret == 0) 
					smarthomeAddNewDev(packet,id);
				// ���ݰ���APP�趨������������ֹ����
				zigbeeNetIn(0);
			}
			break;
		case NetOut_Report_Res:	//���豸Ҫ����
			printf("out\n");
			break;	
			
		case Demand_Sw_Status_Res:	//����״̬����
			printf("status\n");
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
			{
				printf("on:%x\n",packet->addr);
				char id[32];
				smarthomeGetId(packet,id);
				gwReportPowerOn(id,packet->param);
			} break;
			
		case Device_Off_Res:	//�豸��
			printf("off:%x\n",packet->addr);
			{
				char id[32];
				smarthomeGetId(packet,id);
				gwReportPowerOff(id);
			} break;
		
		case Demand_Device_Alarm_Type_Res:		//��ѯ��Ԫ�ľ���״̬����
			{
				printf("alarm_status:%x,%d\n",packet->addr,packet->param[0]);
				char id[32];
				smarthomeGetId(packet,id);
				gwReportAlarmStatus(id,packet->param);
			} break;
			
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

void smarthomeAllDeviceCmdGetSwichStatus(DeviceStr *dev,uint16_t channel)
{
	smarthomeSendDataPacket(
			dev->addr,
			Demand_Sw_Status,
			dev->type_para->device_type,
			dev->channel,
			channel, NULL, 0);
}

void smarthomeLightCmdCtrOpen(DeviceStr *dev,uint16_t channel)
{
	uint8_t param[2] = {0xff,0};
	printf("[%s]type:%d\n",__FUNCTION__, dev->type_para->device_type);
	smarthomeSendDataPacket(
			dev->addr,
			Device_On,
			dev->type_para->device_type,
			dev->channel,
			channel,param,sizeof(param));
}

void smarthomeLightCmdCtrClose(DeviceStr *dev,uint16_t channel)
{
	printf("[%s]type:%d\n",__FUNCTION__, dev->type_para->device_type);
	smarthomeSendDataPacket(
			dev->addr,
			Device_Off,
			dev->type_para->device_type,
			dev->channel,
			channel, NULL, 0);
}

void smarthomeFreshAirCmdCtrOpen(DeviceStr *dev,uint8_t value)
{
	uint8_t param[2] = {0};
	param[0] = value;
	printf("[%s]type:%d,value:%d\n",
			__FUNCTION__, 
			dev->type_para->device_type,
			param[0]);
	smarthomeSendDataPacket(
			dev->addr,
			Device_On,
			dev->type_para->device_type,
			dev->channel,
			0,param,sizeof(param));
}

void smarthomeFreshAirCmdCtrClose(DeviceStr *dev)
{
	printf("[%s]type:%d\n",__FUNCTION__, dev->type_para->device_type);
	smarthomeSendDataPacket(
			dev->addr,
			Device_Off,
			dev->type_para->device_type,
			dev->channel,
			0,NULL,0);
}
