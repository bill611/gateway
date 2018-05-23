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
	sprintf(id,"%02X%02X%02X%02X%02X%02X%02X%02X",
			cmdBuf->param[0],
			cmdBuf->param[1],
			cmdBuf->param[2],
			cmdBuf->param[3],
			cmdBuf->param[4],
			cmdBuf->param[5],
			cmdBuf->param[6],
			cmdBuf->param[7]);
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

static char * smarthomeGetId(SMART_HOME_PRO *cmdBuf)
{
	static char id[32];
	sqlGetDeviceId(cmdBuf->addr,id);
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
	SMART_HOME_PRO *packet = (SMART_HOME_PRO *)buf;
    if (smarthomeCmdFilter(packet) == 0)
        return;
	switch (packet->cmd)
	{
		case NetIn_Report:
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
				gwRegisterSubDevice(id,packet->device_type);
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
			printf("on\n");
			gwReportPowerOn(smarthomeGetId(packet),packet->param);
			break;
			
		case Device_Off_Res:	//�豸��
			printf("off\n");
			gwReportPowerOff(smarthomeGetId(packet));
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

void smarthomeLightCmdCtrOpen(uint16_t addr,uint16_t channel_num,uint16_t channel)
{
	uint8_t param[2] = {0xff,0};
	smarthomeSendDataPacket(addr,Device_On,channel_num,channel,param,sizeof(param));
}

void smarthomeLightCmdCtrClose(uint16_t addr,uint16_t channel_num,uint16_t channel)
{
	smarthomeSendDataPacket(addr,Device_Off,channel_num,channel,NULL,0);
}

void smarthomeFreshAirCmdCtrOpen(uint16_t addr,uint8_t value)
{
	uint8_t param[2] = {0};
	param[0] = value;
	printf("value :%d,test:%d\n", param[0],value);
	smarthomeSendDataPacket(addr,Device_On,1,0,param,sizeof(param));
}

void smarthomeFreshAirCmdCtrClose(uint16_t addr)
{
	smarthomeSendDataPacket(addr,Device_Off,1,0,NULL,0);
}
