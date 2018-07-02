#ifndef __SMART_HOME_PRO_H__
#define __SMART_HOME_PRO_H__
#include "zigbee.h"
#include "device_protocol.h"

#define MAX_REGIST_DEVICE 70  // 最多支持的设备

#define CONTROL_MACHINE_ARRD		0
#define DEVICE_HW_VER						10		//硬件版本
#define DEVICE_SW_VER						10		//软件版本					


enum 
{
	SHP_ST_IDIE,//空闲中
	SHP_ST_WAIT,//等待回复
};

typedef enum
{
	DEVICE_TYPE_ZK = 			0,		// 主控设备
	DEVICE_TYPE_DK = 			6,		// 灯控开关
	DEVICE_TYPE_TG,				// 7	调光开关
	DEVICE_TYPE_KT,				// 8	空调开关
	DEVICE_TYPE_CZ,				// 9	插座
	DEVICE_TYPE_CL,				// 10	窗帘
	DEVICE_TYPE_JJ,				// 11	紧急按钮
	DEVICE_TYPE_HW,				// 12	红外报警
	DEVICE_TYPE_YW,				// 13	烟雾报警
	DEVICE_TYPE_WS,				// 14	瓦斯报警
	DEVICE_TYPE_MC,				// 15	门磁报警
	DEVICE_TYPE_CC,				// 16	窗磁报警
	DEVICE_TYPE_TY,				// 17	通用报警
	DEVICE_TYPE_DS,				// 18	电视控制器
	DEVICE_TYPE_WXHW,			// 19	无线红外
	DEVICE_TYPE_QJ,				// 20	情景控制器
	DEVICE_TYPE_DHX,			// 21	单火线灯控
	DEVICE_TYPE_LED,			// 22	LED控制器
	DEVICE_TYPE_ZYKT,			// 23	中央空调
	DEVICE_TYPE_BJYY,			// 24	背景音乐
	DEVICE_TYPE_XFXT,			// 25	新风系统
	DEVICE_TYPE_FS,		  		// 26	风扇
	DEVICE_TYPE_SGBJQ,		 	// 27	声光报警器
	DEVICE_TYPE_JLCZ10 = 47,			// 47	10A计量插座	
	DEVICE_TYPE_JLCZ16 = 48,			// 48	16A计量插座	
	DEVICE_TYPE_JD = 102,		// 102	警笛报警器
}TC_Device_Type;

typedef enum
{
	NetIn_Report					= 0xf0,			//入网上报
	NetIn_Report_Res				= 0xf1,			//入网上报返回
	NetOut_Report					= 0xf2,			//退网通知
	NetOut_Report_Res				= 0xf3,			//退网通知返回
	NetIn_Apply						= 0xf4,			//入网申请
	Report_Status					= 0xf5,			//上报状态
	Device_Syn						= 0xfa,			//设备同步
	Device_Syn_Res					= 0xfb,			//设备同步返回
	Demand_Sw_Status				= 0,			//查询开关状态
	Demand_Sw_Status_Res			= 1,			//查询开关状态返回
	Demand_Device_Alarm_Type		= 0x02,			//查询单元的警报状态
	Demand_Device_Alarm_Type_Res	= 0x03,			//查询单元的警报状态返回
	Demand_Device_Type				= 0x04,			//查询设备类型
	Demand_Device_Type_Res			= 0x05,			//查询设备类型返回
	Demand_Device_Ver				= 0x06,			//查询设备固件版本号
	Demand_Device_Ver_Res			= 0x07,			//查询设备固件版本号返回
	Demand_Time						= 0x08,			//查询时间
	Demand_Time_Res					= 0x09,			//查询时间返回
	Device_Ele_Quantity				= 0x58,			//设备每隔30分钟上报用电量
	Device_Ele_Quantity_Res			= 0x59,			//设备每隔30分钟上报用电量返回
	Device_Ele_Power				= 0x5a,			//设备每隔30分钟上报当前功率，或功率异常时立即上报当前功率，功率异常范围值：
   													//	1、10A：≥1800w；  2、16A：≥3000w；
	Device_Ele_Power_Res			= 0x5b,			//立即上报功率返回
	Device_On						= 0x90,			//开启单元
	Device_On_Res					= 0x91,			//开启单元返回
	Device_Off						= 0x92,			//关闭单元
	Device_Off_Res					= 0x93,			//关闭单元返回
	
	Device_Scene		 			= 0xb0,
	Device_Scene_Res 				= 0xb1,
	Cmd_Null						= 0xff,			//空命令
}TC_CMD;

enum {
	TC_ALARM_ACTION = 1,   // 感应报警
	TC_ALARM_LOWPOWER,   // 低电压报警
	TC_ALARM_TAMPER,   // 防拆报警
	TC_ALARM_OPEN_WINDOW,   // 开门/窗报警
	TC_ALARM_CLOSE_WINDOW,   // 关门/窗报警
	TC_ALARM_ERR,   // 故障报警（机械手）
	TC_ALARM_OPEN_DOOR,   // 非法开门报警
	TC_ALARM_CLOSE_DOOR,   // 门未关报警
}TC_ALARM_STATUS;

#pragma pack(1)
typedef struct 
{
	uint16_t addr;
	uint8_t channel;
	uint8_t id;
}zbDev_TypeDef;	//zigbee周边设备基本信息

typedef struct
{
	uint16_t addr;
	uint8_t  device_type;
	uint8_t  channel_num;	 //通道数量
	uint8_t  current_channel;//	当前通道
	uint8_t  cmd;
	uint8_t  param[8];
}SMART_HOME_PRO;	//单控协议

typedef struct 
{
	uint16_t addr;		//周边设备地址
	uint8_t channel;	//当前通道
	uint8_t cmd;		//功能指令
	uint16_t param;		//参数
}MULTI_CTRL_TypeDef;	//群发控制协议单个设备结构
#pragma pack()

#define MULTI_PACKET_DEV_MAX	((ZIGBEE_UART_MAX-8)/(sizeof(MULTI_CTRL_TypeDef)))	//群控每包最大的设备个数

typedef struct 
{
	uint8_t type[3];		//协议类型
	MULTI_CTRL_TypeDef dev[MULTI_PACKET_DEV_MAX];
}multiPacket_TypeDef;	//群发控制协议包的数据结构



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
