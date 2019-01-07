/*
 * =============================================================================
 *
 *       Filename:  device_air_condition_midea.c
 *
 *    Description:  美的空调设备
 *
 *        Version:  1.0
 *        Created:  2018-05-09 08:46:55
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
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#include "device_air_condition_midea.h"
#include "sql_handle.h"
#include "config.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/
static void setMideaData(DeviceStr *This);

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
#define MAX_VALUE_LENG 32
#define MAX_MIDEA_AIR_CONDITION   4   // 最大美的空调数量
enum {
	ATTR_CURRENT_TEMP,

	ATTR_SWICH,
	ATTR_MODE,
	ATTR_SPEED,
	ATTR_TEMP,

	ATTR_SWICH1,
	ATTR_MODE1,
	ATTR_SPEED1,
	ATTR_TEMP1,

	ATTR_SWICH2,
	ATTR_MODE2,
	ATTR_SPEED2,
	ATTR_TEMP2,

	ATTR_SWICH3,
	ATTR_MODE3,
	ATTR_SPEED3,
	ATTR_TEMP3,
};

typedef struct _MideaAirConditionSlave {                
    int slave_addr ;                                    
    int room_addr ;                                     
    int temp ;                                          
    int mode ;                                          
    int speed ;                                         
}MideaAirConditionSlave;                                
                                                        
typedef struct _MideaAirConditionData {                 
    MideaAirConditionSlave dev[MAX_MIDEA_AIR_CONDITION];
}MideaAirConditionData;                                 
/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static MideaAirConditionData dev_midea;

static int getAttrCb(DeviceStr *dev, const char *attr_set[])
{
    DPRINT("get attr, devid:%s, attribute name:\n", dev->id);
    unsigned int i = 0;
    while (attr_set[i++]) {
        DPRINT("attr_%d: %s\n", i - 1, attr_set[i - 1]);
    }
	for (i=0; dev->type_para->attr[i].name != NULL; i++) {
		if (strcmp(attr_set[0],dev->type_para->attr[i].name) == 0) {
			const char *attr_name[2] = {NULL};
			const char *attr_value[2] = {NULL};
			int attr_value_type[2];
			attr_name[0] = dev->type_para->attr[i].name;
			attr_value[0] = dev->value[i];
			attr_value_type[0] = dev->type_para->attr[i].value_type;
			// DPRINT("[%s]--->%s\n", attr_name[0],attr_value[0]);
			aliSdkSubDevReportAttrs(dev, attr_name,attr_value,attr_value_type);
		}
	}


    return 0;
}


static int setAttrCb(DeviceStr *dev, const char *attr_name, const char *attr_value)
{
    unsigned int i = 0;
	for (i=0; dev->type_para->attr[i].name != NULL; i++) {
		if (strcmp(attr_name,dev->type_para->attr[i].name) == 0) {
			sprintf(dev->value[i],"%s",attr_value);
			DPRINT("[%s,%s]%s:%s\n",__FUNCTION__,__FILE__,attr_name,attr_value);
			if (dev->type_para->attr[i].attrMultitermcb)
				dev->type_para->attr[i].attrMultitermcb(dev,dev->value[i],attr_name);
			break;
		}
	}

    return 0;
}

static int getAttrArrayNum(const char *name)
{
	int num = 0;	
	while (*name != '\0') {
		if (*name == '_') {
			name++;
			num = atoi(name);
		}
		name++;
	}
	return num;
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief cmdSwich 空调开关
 *
 * @param dev
 * @param value
 */
/* ---------------------------------------------------------------------------*/
static void cmdSwich(DeviceStr *dev,char *value,const char *attr_name)
{
	int num = getAttrArrayNum(attr_name);
	num--;
	DPRINT("num = %d\n", num);
	int value_int = atoi(value);
	sprintf(dev->value[ATTR_SWICH + num*MAX_MIDEA_AIR_CONDITION],"%s",value);
	DPRINT("[%s]value:%s,int:%d,buf:%s,speed:%s\n",
			__FUNCTION__,
			value,
			value_int,
			dev->value[ATTR_SWICH + num*MAX_MIDEA_AIR_CONDITION],
			dev->value[ATTR_SPEED + num*MAX_MIDEA_AIR_CONDITION] );
	if (value_int) {
		smarthomeAirCondtionCmdCtrOpen(dev,
				atoi(dev->value[ATTR_TEMP + num*MAX_MIDEA_AIR_CONDITION]),
				atoi(dev->value[ATTR_MODE + num*MAX_MIDEA_AIR_CONDITION]),
				atoi(dev->value[ATTR_SPEED + num*MAX_MIDEA_AIR_CONDITION]),num);
	} else
		smarthomeFreshAirCmdCtrClose(dev,num);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief cmdWorkMode 空调工作模式
 *
 * @param dev
 * @param value APP端对应模式为 0（自动） 1（制冷） 2（制热） 3（通风） 4（除湿）
 * 协议对应为
 * 第1个Byte为空调温度，范围16-32℃
 * 第2个byte:
 * 高4位:0制冷  1制热  2自动
 *        3除湿  4送风
 *        低4位: 0自动    1风高速
 *        2风中速  3风低速
 *
 */
/* ---------------------------------------------------------------------------*/
static void cmdWorkMode(DeviceStr *dev,char *value,const char *attr_name)
{
	int num = getAttrArrayNum(attr_name);
	num--;
	DPRINT("num = %d\n", num);
	sprintf(dev->value[ATTR_MODE + num*MAX_MIDEA_AIR_CONDITION],"%s",value);
	smarthomeAirCondtionCmdCtrOpen(dev,
			atoi(dev->value[ATTR_TEMP + num*MAX_MIDEA_AIR_CONDITION]),
			atoi(dev->value[ATTR_MODE + num*MAX_MIDEA_AIR_CONDITION]),
			atoi(dev->value[ATTR_SPEED + num*MAX_MIDEA_AIR_CONDITION]),num);
	dev_midea.dev[num].mode = atoi(value);
	setMideaData(dev);
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief cmdWindSpeed APP端对应模式为 0（自动） 2（低档） 3（中档） 4（高档）
 * 协议对应为
 * 第1个Byte为空调温度，范围16-32℃
 * 第2个byte:
 * 高4位:0制冷  1制热  2自动
 *        3除湿  4送风
 *        低4位: 0自动    1风高速
 *        2风中速  3风低速
 * @param dev
 * @param value
 */
/* ---------------------------------------------------------------------------*/
static void cmdWindSpeed(DeviceStr *dev,char *value,const char *attr_name)
{
	int num = getAttrArrayNum(attr_name);
	num--;
	DPRINT("num = %d\n", num);
	sprintf(dev->value[ATTR_SPEED + num*MAX_MIDEA_AIR_CONDITION],"%s",value);
	smarthomeAirCondtionCmdCtrOpen(dev,
			atoi(dev->value[ATTR_TEMP + num*MAX_MIDEA_AIR_CONDITION]),
			atoi(dev->value[ATTR_MODE + num*MAX_MIDEA_AIR_CONDITION]),
			atoi(dev->value[ATTR_SPEED + num*MAX_MIDEA_AIR_CONDITION]),num);
	dev_midea.dev[num].speed = atoi(value);
	setMideaData(dev);
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief cmdTemperature 设置温度
 *
 * @param dev
 * @param value
 */
/* ---------------------------------------------------------------------------*/
static void cmdTemperature(DeviceStr *dev,char *value,const char *attr_name)
{
	int num = getAttrArrayNum(attr_name);
	num--;
	DPRINT("num = %d\n", num);
	sprintf(dev->value[ATTR_TEMP + num*MAX_MIDEA_AIR_CONDITION],"%s",value);
	smarthomeAirCondtionCmdCtrOpen(dev,
			atoi(dev->value[ATTR_TEMP + num*MAX_MIDEA_AIR_CONDITION]),
			atoi(dev->value[ATTR_MODE + num*MAX_MIDEA_AIR_CONDITION]),
			atoi(dev->value[ATTR_SPEED + num*MAX_MIDEA_AIR_CONDITION]),num);
	dev_midea.dev[num].temp = atoi(value);
	setMideaData(dev);
}

static void cmdGetSwichStatus(DeviceStr *dev)
{
	smarthomeAllDeviceCmdGetSwichStatus(dev,1);
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief reportPowerOnCb
 *
 * APP端对应风速为 0（自动） 2（低档） 3（中档） 4（高档）
 * 		  模式为 0（自动） 1（制冷） 2（制热） 3（通风） 4（除湿）
 * 协议对应为
 * 第1个Byte为空调温度，范围16-32℃
 * 第2个byte:
 * 高4位:0制冷  1制热  2自动
 *        3除湿  4送风
 *        低4位: 0自动    1风高速
 *        2风中速  3风低速
 * @param dev
 * @param param
 */
/* ---------------------------------------------------------------------------*/
static void reportPowerOnCb(DeviceStr *dev,char *param,int channel)
{
	char speed_change[] = {0,4,3,2}; // 速度转换
	char mode_change[] = {1,2,0,4,3}; // 模式转换
	// 固定为开
	sprintf(dev->value[ATTR_SWICH + (channel-1)*MAX_MIDEA_AIR_CONDITION],"1");
	sprintf(dev->value[ATTR_TEMP + (channel-1)*MAX_MIDEA_AIR_CONDITION],"%d",param[0]);
	sprintf(dev->value[ATTR_SPEED + (channel-1)*MAX_MIDEA_AIR_CONDITION],"%d",speed_change[param[1] & 0x0f]);
	sprintf(dev->value[ATTR_MODE + (channel-1)*MAX_MIDEA_AIR_CONDITION],"%d",mode_change[param[1] >> 4]);
	const char *attr_name[] = {
		dev->type_para->attr[ATTR_SWICH + (channel-1)*MAX_MIDEA_AIR_CONDITION].name,
		dev->type_para->attr[ATTR_SPEED + (channel-1)*MAX_MIDEA_AIR_CONDITION].name,
		dev->type_para->attr[ATTR_MODE + (channel-1)*MAX_MIDEA_AIR_CONDITION].name,
		dev->type_para->attr[ATTR_TEMP + (channel-1)*MAX_MIDEA_AIR_CONDITION].name,
		NULL};
	const char *attr_value[] = {
		dev->value[ATTR_SWICH + (channel-1)*MAX_MIDEA_AIR_CONDITION],
		dev->value[ATTR_SPEED + (channel-1)*MAX_MIDEA_AIR_CONDITION],
		dev->value[ATTR_MODE + (channel-1)*MAX_MIDEA_AIR_CONDITION],
		dev->value[ATTR_TEMP + (channel-1)*MAX_MIDEA_AIR_CONDITION],
		NULL};
	int attr_value_type[] = {
		dev->type_para->attr[ATTR_SWICH + (channel-1)*MAX_MIDEA_AIR_CONDITION].value_type,
		dev->type_para->attr[ATTR_SPEED + (channel-1)*MAX_MIDEA_AIR_CONDITION].value_type,
		dev->type_para->attr[ATTR_MODE + (channel-1)*MAX_MIDEA_AIR_CONDITION].value_type,
		dev->type_para->attr[ATTR_TEMP + (channel-1)*MAX_MIDEA_AIR_CONDITION].value_type,
	};
	aliSdkSubDevReportAttrs(dev,
			attr_name,attr_value,attr_value_type);
}

static void reportPowerOffCb(DeviceStr *dev,int channel)
{
	sprintf(dev->value[ATTR_SWICH],"0");
	const char *attr_name[] = {
		dev->type_para->attr[ATTR_SWICH + (channel)*MAX_MIDEA_AIR_CONDITION].name,
		NULL};
	const char *attr_value[] = {
		dev->value[ATTR_SWICH + (channel)*MAX_MIDEA_AIR_CONDITION],
		NULL};
	int attr_value_type[] = {
		dev->type_para->attr[ATTR_SWICH + (channel)*MAX_MIDEA_AIR_CONDITION].value_type,
	};
	aliSdkSubDevReportAttrs(dev,
			attr_name,attr_value,attr_value_type);
}

static DeviceTypePara air_condition_midea = {
	.name = "air_condition_midea",
	.short_model = 0x00022531,
	.secret = "Xf3r8BQV1Utz5o6EnJfFXF4tE3BhfAKH3ABYaQDr",
	.product_key = "a1XuFDaBfIg",
	.device_secret = "",
	.proto_type = ALI_SDK_PROTO_TYPE_ZIGBEE,
	.device_type = DEVICE_TYPE_ZYKT_MIDEA,
	.attr = {
#if (defined V1)
		{"ErrorCode",NULL,DEVICE_VELUE_TYPE_INT},
		{"Switch",cmdSwich,DEVICE_VELUE_TYPE_INT},
		{"WorkMode",cmdWorkMode,DEVICE_VELUE_TYPE_INT},
		{"WindSpeed",cmdWindSpeed,DEVICE_VELUE_TYPE_INT},
		{"CurrentTemp",NULL,DEVICE_VELUE_TYPE_INT},
		{"Temperature",cmdTemperature,DEVICE_VELUE_TYPE_INT},
#else
		{"CurrentTemperature",NULL,DEVICE_VELUE_TYPE_INT},

		// 4组空调数据
		{"PowerSwitch_1",NULL,DEVICE_VELUE_TYPE_INT,cmdSwich},
		{"WorkMode_1",NULL,DEVICE_VELUE_TYPE_INT,cmdWorkMode},
		{"WindSpeed_1",NULL,DEVICE_VELUE_TYPE_INT,cmdWindSpeed},
		{"TargetTemperature_1",NULL,DEVICE_VELUE_TYPE_INT,cmdTemperature},

		{"PowerSwitch_2",NULL,DEVICE_VELUE_TYPE_INT,cmdSwich},
		{"WorkMode_2",NULL,DEVICE_VELUE_TYPE_INT,cmdWorkMode},
		{"WindSpeed_2",NULL,DEVICE_VELUE_TYPE_INT,cmdWindSpeed},
		{"TargetTemperature_2",NULL,DEVICE_VELUE_TYPE_INT,cmdTemperature},

		{"PowerSwitch_3",NULL,DEVICE_VELUE_TYPE_INT,cmdSwich},
		{"WorkMode_3",NULL,DEVICE_VELUE_TYPE_INT,cmdWorkMode},
		{"WindSpeed_3",NULL,DEVICE_VELUE_TYPE_INT,cmdWindSpeed},
		{"TargetTemperature_3",NULL,DEVICE_VELUE_TYPE_INT,cmdTemperature},

		{"PowerSwitch_4",NULL,DEVICE_VELUE_TYPE_INT,cmdSwich},
		{"WorkMode_4",NULL,DEVICE_VELUE_TYPE_INT,cmdWorkMode},
		{"WindSpeed_4",NULL,DEVICE_VELUE_TYPE_INT,cmdWindSpeed},
		{"TargetTemperature_4",NULL,DEVICE_VELUE_TYPE_INT,cmdTemperature},

#endif
		{NULL,NULL},
	},
	.getAttr = getAttrCb,
	.setAttr = setAttrCb,
	.getSwichStatus = cmdGetSwichStatus,
	.reportPowerOn = reportPowerOnCb,
	.reportPowerOff = reportPowerOffCb,
};

static void *sendMideaSlaveAddrThread(void *arg)
{
	DeviceStr *This = (DeviceStr *)arg;
	while (1) {
		if (zigbeeIsReady() == 0) {
			usleep(10000);
			continue;
		}
		sleep(5);
		smarthomeAirCondtionMideaCmdSlaveAddr(This,
				dev_midea.dev[0].slave_addr,dev_midea.dev[0].room_addr,0);
		smarthomeAirCondtionMideaCmdSlaveAddr(This,
				dev_midea.dev[1].slave_addr,dev_midea.dev[1].room_addr,1);
		smarthomeAirCondtionMideaCmdSlaveAddr(This,
				dev_midea.dev[2].slave_addr,dev_midea.dev[2].room_addr,2);
		smarthomeAirCondtionMideaCmdSlaveAddr(This,
				dev_midea.dev[3].slave_addr,dev_midea.dev[3].room_addr,3);
		break;
	}
	pthread_exit(NULL);
}

static void getMideaData(DeviceStr *This,char *id)
{
	int i;
	sqlGetMideaAddr(id,&dev_midea);
	for (i=0; i<MAX_MIDEA_AIR_CONDITION; i++) {
		if (dev_midea.dev[i].temp < 16)
			dev_midea.dev[i].temp = 16;
		
	}
	sprintf(This->value[ATTR_TEMP],"%d",dev_midea.dev[0].temp);
	sprintf(This->value[ATTR_SPEED],"%d",dev_midea.dev[0].speed);
	sprintf(This->value[ATTR_MODE],"%d",dev_midea.dev[0].mode);

	sprintf(This->value[ATTR_TEMP1],"%d",dev_midea.dev[1].temp);
	sprintf(This->value[ATTR_SPEED1],"%d",dev_midea.dev[1].speed);
	sprintf(This->value[ATTR_MODE1],"%d",dev_midea.dev[1].mode);

	sprintf(This->value[ATTR_TEMP2],"%d",dev_midea.dev[2].temp);
	sprintf(This->value[ATTR_SPEED2],"%d",dev_midea.dev[2].speed);
	sprintf(This->value[ATTR_MODE2],"%d",dev_midea.dev[2].mode);

	sprintf(This->value[ATTR_TEMP3],"%d",dev_midea.dev[3].temp);
	sprintf(This->value[ATTR_SPEED3],"%d",dev_midea.dev[3].speed);
	sprintf(This->value[ATTR_MODE3],"%d",dev_midea.dev[3].mode);

	pthread_t task;
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&task, &attr, sendMideaSlaveAddrThread, This);

}
static void setMideaData(DeviceStr *This)
{
	sqlSetMideaAddr(This->id,&dev_midea,sizeof(dev_midea));
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief registDeviceAirCondition 美的空调
 *
 * @param id
 * @param addr
 * @param channel
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
DeviceStr * registDeviceAirConditionMidea(char *id,uint16_t addr,uint16_t channel,char *pk)
{
	if (pk) {
		if (strcmp(pk,air_condition_midea.product_key) != 0) {
			DPRINT("diff pk :allow pk:%s,now pk:%s\n",
					air_condition_midea.product_key,pk );	
			return NULL;
		}
	}
	int i;
	DeviceStr *This = (DeviceStr *)calloc(1,sizeof(DeviceStr));
	strcpy(This->id,id);
	memset(This->value,0,sizeof(This->value));
	This->type_para = &air_condition_midea;
	This->addr = addr;
	This->channel = channel;
	DPRINT("[%s]addr:%x,channel:%d\n",__FUNCTION__,This->addr,This->channel );
	// 初始化属性
	for (i=0; This->type_para->attr[i].name != NULL; i++) {
		This->value[i] = (char *)calloc(1,MAX_VALUE_LENG);
		sprintf(This->value[i],"%s","0");
	}

	getMideaData(This,id);

	for (i=0; This->type_para->attr[i].name != NULL; i++) {
		printf("[%s]name:%s,value:%s\n",__FUNCTION__,This->type_para->attr[i].name,This->value[i]);
	}

	return This;
}

