#ifndef __SMART_HOME_PRO_H__
#define __SMART_HOME_PRO_H__
#include "zigbee.h"
#include "device_protocol.h"

#define MAX_REGIST_DEVICE 70  // ���֧�ֵ��豸

#define CONTROL_MACHINE_ARRD		0
#define DEVICE_HW_VER						10		//Ӳ���汾
#define DEVICE_SW_VER						10		//����汾					


enum 
{
	SHP_ST_IDIE,//������
	SHP_ST_WAIT,//�ȴ��ظ�
};

typedef enum
{
	DEVICE_TYPE_ZK = 			0,		// �����豸
	DEVICE_TYPE_DK = 			6,		// �ƿؿ���
	DEVICE_TYPE_TG,				// 7	���⿪��
	DEVICE_TYPE_KT,				// 8	�յ�����
	DEVICE_TYPE_CZ,				// 9	����
	DEVICE_TYPE_CL,				// 10	����
	DEVICE_TYPE_JJ,				// 11	������ť
	DEVICE_TYPE_HW,				// 12	���ⱨ��
	DEVICE_TYPE_YW,				// 13	������
	DEVICE_TYPE_WS,				// 14	��˹����
	DEVICE_TYPE_MC,				// 15	�Ŵű���
	DEVICE_TYPE_CC,				// 16	���ű���
	DEVICE_TYPE_TY,				// 17	ͨ�ñ���
	DEVICE_TYPE_DS,				// 18	���ӿ�����
	DEVICE_TYPE_WXHW,			// 19	���ߺ���
	DEVICE_TYPE_QJ,				// 20	�龰������
	DEVICE_TYPE_DHX,			// 21	�����ߵƿ�
	DEVICE_TYPE_LED,			// 22	LED������
	DEVICE_TYPE_ZYKT,			// 23	����յ�
	DEVICE_TYPE_BJYY,			// 24	��������
	DEVICE_TYPE_XFXT,			// 25	�·�ϵͳ
	DEVICE_TYPE_FS,		  		// 26	����
	DEVICE_TYPE_SGBJQ,		 	// 27	���ⱨ����
	DEVICE_TYPE_JLCZ10 = 47,			// 47	10A��������	
	DEVICE_TYPE_JLCZ16 = 48,			// 48	16A��������	
	DEVICE_TYPE_JD = 102,		// 102	���ѱ�����
}TC_Device_Type;

typedef enum
{
	NetIn_Report					= 0xf0,			//�����ϱ�
	NetIn_Report_Res				= 0xf1,			//�����ϱ�����
	NetOut_Report					= 0xf2,			//����֪ͨ
	NetOut_Report_Res				= 0xf3,			//����֪ͨ����
	NetIn_Apply						= 0xf4,			//��������
	Report_Status					= 0xf5,			//�ϱ�״̬
	Device_Syn						= 0xfa,			//�豸ͬ��
	Device_Syn_Res					= 0xfb,			//�豸ͬ������
	Demand_Sw_Status				= 0,			//��ѯ����״̬
	Demand_Sw_Status_Res			= 1,			//��ѯ����״̬����
	Demand_Device_Alarm_Type		= 0x02,			//��ѯ��Ԫ�ľ���״̬
	Demand_Device_Alarm_Type_Res	= 0x03,			//��ѯ��Ԫ�ľ���״̬����
	Demand_Device_Type				= 0x04,			//��ѯ�豸����
	Demand_Device_Type_Res			= 0x05,			//��ѯ�豸���ͷ���
	Demand_Device_Ver				= 0x06,			//��ѯ�豸�̼��汾��
	Demand_Device_Ver_Res			= 0x07,			//��ѯ�豸�̼��汾�ŷ���
	Demand_Time						= 0x08,			//��ѯʱ��
	Demand_Time_Res					= 0x09,			//��ѯʱ�䷵��
	Device_Ele_Quantity				= 0x58,			//�豸ÿ��30�����ϱ��õ���
	Device_Ele_Quantity_Res			= 0x59,			//�豸ÿ��30�����ϱ��õ�������
	Device_Ele_Power				= 0x5a,			//�豸ÿ��30�����ϱ���ǰ���ʣ������쳣ʱ�����ϱ���ǰ���ʣ������쳣��Χֵ��
   													//	1��10A����1800w��  2��16A����3000w��
	Device_Ele_Power_Res			= 0x5b,			//�����ϱ����ʷ���
	Device_On						= 0x90,			//������Ԫ
	Device_On_Res					= 0x91,			//������Ԫ����
	Device_Off						= 0x92,			//�رյ�Ԫ
	Device_Off_Res					= 0x93,			//�رյ�Ԫ����
	
	Device_Scene		 			= 0xb0,
	Device_Scene_Res 				= 0xb1,
	Cmd_Null						= 0xff,			//������
}TC_CMD;

enum {
	TC_ALARM_ACTION = 1,   // ��Ӧ����
	TC_ALARM_LOWPOWER,   // �͵�ѹ����
	TC_ALARM_TAMPER,   // ���𱨾�
	TC_ALARM_OPEN_WINDOW,   // ����/������
	TC_ALARM_CLOSE_WINDOW,   // ����/������
	TC_ALARM_ERR,   // ���ϱ�������е�֣�
	TC_ALARM_OPEN_DOOR,   // �Ƿ����ű���
	TC_ALARM_CLOSE_DOOR,   // ��δ�ر���
}TC_ALARM_STATUS;

#pragma pack(1)
typedef struct 
{
	uint16_t addr;
	uint8_t channel;
	uint8_t id;
}zbDev_TypeDef;	//zigbee�ܱ��豸������Ϣ

typedef struct
{
	uint16_t addr;
	uint8_t  device_type;
	uint8_t  channel_num;	 //ͨ������
	uint8_t  current_channel;//	��ǰͨ��
	uint8_t  cmd;
	uint8_t  param[8];
}SMART_HOME_PRO;	//����Э��

typedef struct 
{
	uint16_t addr;		//�ܱ��豸��ַ
	uint8_t channel;	//��ǰͨ��
	uint8_t cmd;		//����ָ��
	uint16_t param;		//����
}MULTI_CTRL_TypeDef;	//Ⱥ������Э�鵥���豸�ṹ
#pragma pack()

#define MULTI_PACKET_DEV_MAX	((ZIGBEE_UART_MAX-8)/(sizeof(MULTI_CTRL_TypeDef)))	//Ⱥ��ÿ�������豸����

typedef struct 
{
	uint8_t type[3];		//Э������
	MULTI_CTRL_TypeDef dev[MULTI_PACKET_DEV_MAX];
}multiPacket_TypeDef;	//Ⱥ������Э��������ݽṹ



#define SEND_QUEUE_NUM	8

void SHP_Timer(void);

extern void smarthomeAllDeviceCmdGetSwichStatus(DeviceStr *dev,uint16_t channel);
extern void smarthomeLightCmdCtrOpen(DeviceStr *dev,uint16_t channel);
extern void smarthomeLightCmdCtrClose(DeviceStr *dev,uint16_t channel);
extern void smarthomeFreshAirCmdCtrOpen(DeviceStr *dev,uint8_t value);
extern void smarthomeFreshAirCmdCtrClose(DeviceStr *dev);
extern void smarthomeAlarmWhistleCmdCtrOpen(DeviceStr *dev);
extern void smarthomeAlarmWhistleCmdCtrClose(DeviceStr *dev);
extern void smarthomeCurtainCmdCtrOpen(DeviceStr *dev,uint16_t value);
extern void smarthomeCurtainCmdCtrClose(DeviceStr *dev);
extern void smarthomeAirCondtionCmdCtrOpen(DeviceStr *dev,
		uint8_t temp,
		uint8_t mode,
		uint8_t speed);
extern void smarthomeInit(void);
#endif
