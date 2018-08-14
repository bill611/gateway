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

#include "device_air_condition_midea.h"
#include "sql_handle.h"
#include "config.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
#define MAX_VALUE_LENG 32
enum {
	ATTR_ERROR,
	ATTR_SWICH,
	ATTR_MODE,
	ATTR_SPEED,
	ATTR_CURRENT_TEMP,
	ATTR_TEMP,
	ATTR_SLAVE_ADDR,
	ATTR_ROOM_ADDR,
};

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
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
			if (dev->type_para->attr[i].attrcb)
				dev->type_para->attr[i].attrcb(dev,dev->value[i]);
			break;
		}
	}

    return 0;
}

static void cmdSwich(DeviceStr *dev,char *value)
{
	int value_int = atoi(value);
	sprintf(dev->value[ATTR_SWICH],"%s",value);
	DPRINT("[%s]value:%s,int:%d,buf:%s,speed:%s\n",
			__FUNCTION__,
			value,
			value_int,
			dev->value[ATTR_SWICH],
			dev->value[ATTR_SPEED] );
	if (value_int) {
		smarthomeAirCondtionCmdCtrOpen(dev,
				atoi(dev->value[ATTR_TEMP]),
				atoi(dev->value[ATTR_MODE]),
				atoi(dev->value[ATTR_SPEED]));
	} else
		smarthomeFreshAirCmdCtrClose(dev);
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
static void cmdWorkMode(DeviceStr *dev,char *value)
{
	sprintf(dev->value[ATTR_MODE],"%s",value);
	smarthomeAirCondtionCmdCtrOpen(dev,
			atoi(dev->value[ATTR_TEMP]),
			atoi(dev->value[ATTR_MODE]),
			atoi(dev->value[ATTR_SPEED]));
	sqlSetAirConditionPara(dev->id,
			atoi(dev->value[ATTR_TEMP]),
			atoi(dev->value[ATTR_MODE]),
			atoi(dev->value[ATTR_SPEED]));
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
static void cmdWindSpeed(DeviceStr *dev,char *value)
{
	sprintf(dev->value[ATTR_SPEED],"%s",value);
	smarthomeAirCondtionCmdCtrOpen(dev,
			atoi(dev->value[ATTR_TEMP]),
			atoi(dev->value[ATTR_MODE]),
			atoi(dev->value[ATTR_SPEED]));
	sqlSetAirConditionPara(dev->id,
			atoi(dev->value[ATTR_TEMP]),
			atoi(dev->value[ATTR_MODE]),
			atoi(dev->value[ATTR_SPEED]));
}

static void cmdTemperature(DeviceStr *dev,char *value)
{
	sprintf(dev->value[ATTR_TEMP],"%s",value);
	smarthomeAirCondtionCmdCtrOpen(dev,
			atoi(dev->value[ATTR_TEMP]),
			atoi(dev->value[ATTR_MODE]),
			atoi(dev->value[ATTR_SPEED]));
	sqlSetAirConditionPara(dev->id,
			atoi(dev->value[ATTR_TEMP]),
			atoi(dev->value[ATTR_MODE]),
			atoi(dev->value[ATTR_SPEED]));
}

static void cmdGetSwichStatus(DeviceStr *dev)
{
	smarthomeAllDeviceCmdGetSwichStatus(dev,1);
}
static void cmdModbusSlaveAddress(DeviceStr *dev,char *value)
{
	sprintf(dev->value[ATTR_SLAVE_ADDR],"%s",value);
	smarthomeAirCondtionMideaCmdSlaveAddr(dev,
			atoi(dev->value[ATTR_SLAVE_ADDR]),
			atoi(dev->value[ATTR_ROOM_ADDR]));
	sqlSetMideaAddr(dev->id,atoi(dev->value[ATTR_SLAVE_ADDR]),
				atoi(dev->value[ATTR_ROOM_ADDR]));
}

static void cmdRoomAdress(DeviceStr *dev,char *value)
{
	sprintf(dev->value[ATTR_ROOM_ADDR],"%s",value);
	smarthomeAirCondtionMideaCmdSlaveAddr(dev,
			atoi(dev->value[ATTR_SLAVE_ADDR]),
			atoi(dev->value[ATTR_ROOM_ADDR]));
	sqlSetMideaAddr(dev->id,atoi(dev->value[ATTR_SLAVE_ADDR]),
				atoi(dev->value[ATTR_ROOM_ADDR]));
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
static void reportPowerOnCb(DeviceStr *dev,char *param)
{
	char speed_change[] = {0,4,3,2}; // 速度转换
	char mode_change[] = {1,2,0,4,3}; // 模式转换
	// 固定为开
	sprintf(dev->value[ATTR_SWICH],"1");
	sprintf(dev->value[ATTR_TEMP],"%d",param[0]); 
	sprintf(dev->value[ATTR_SPEED],"%d",speed_change[param[1] & 0x0f]); 
	sprintf(dev->value[ATTR_MODE],"%d",mode_change[param[1] >> 4]); 
	const char *attr_name[] = {
		dev->type_para->attr[ATTR_SWICH].name,
		dev->type_para->attr[ATTR_SPEED].name,
		dev->type_para->attr[ATTR_MODE].name,
		dev->type_para->attr[ATTR_TEMP].name,
		NULL};
	const char *attr_value[] = {
		dev->value[ATTR_SWICH],
		dev->value[ATTR_SPEED],
		dev->value[ATTR_MODE],
		dev->value[ATTR_TEMP],
		NULL};
	int attr_value_type[] = {
		dev->type_para->attr[ATTR_SWICH].value_type,
		dev->type_para->attr[ATTR_SPEED].value_type,
		dev->type_para->attr[ATTR_MODE].value_type,
		dev->type_para->attr[ATTR_TEMP].value_type,
	};
	aliSdkSubDevReportAttrs(dev,
			attr_name,attr_value,attr_value_type);
}

static void reportPowerOffCb(DeviceStr *dev)
{
	sprintf(dev->value[ATTR_SWICH],"0");
	const char *attr_name[] = {
		dev->type_para->attr[ATTR_SWICH].name,
		NULL};
	const char *attr_value[] = {
		dev->value[ATTR_SWICH],
		NULL};
	int attr_value_type[] = {
		dev->type_para->attr[ATTR_SWICH].value_type,
	};
	aliSdkSubDevReportAttrs(dev,
			attr_name,attr_value,attr_value_type);
}

static DeviceTypePara air_condition_midea = {
	.name = "air_condition_midea",
	.short_model = 0x00022531,
	.secret = "Xf3r8BQV1Utz5o6EnJfFXF4tE3BhfAKH3ABYaQDr",
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
		{"Error",NULL,DEVICE_VELUE_TYPE_INT},
		{"PowerSwitch",cmdSwich,DEVICE_VELUE_TYPE_INT},
		{"WorkMode",cmdWorkMode,DEVICE_VELUE_TYPE_INT},
		{"WindSpeed",cmdWindSpeed,DEVICE_VELUE_TYPE_INT},
		{"CurrentTemperature",NULL,DEVICE_VELUE_TYPE_INT},
		{"TargetTemperature",cmdTemperature,DEVICE_VELUE_TYPE_INT},
		{"ModbusSlaveAddress",cmdModbusSlaveAddress,DEVICE_VELUE_TYPE_INT},
		{"RoomAdress",cmdRoomAdress,DEVICE_VELUE_TYPE_INT},
#endif
		{NULL,NULL},
	},
	.getAttr = getAttrCb,
	.setAttr = setAttrCb,
	.getSwichStatus = cmdGetSwichStatus,
	.reportPowerOn = reportPowerOnCb,
	.reportPowerOff = reportPowerOffCb,
};

/* ---------------------------------------------------------------------------*/
/**
 * @brief registDeviceAirCondition 中央空调/大金空调
 *
 * @param id
 * @param addr
 * @param channel
 *
 * @returns 
 */
/* ---------------------------------------------------------------------------*/
DeviceStr * registDeviceAirConditionMidea(char *id,uint16_t addr,uint16_t channel)
{
	int i;
	DeviceStr *This = (DeviceStr *)calloc(1,sizeof(DeviceStr));
	int slave_addr = 0,room_addr = 0,temp = 0,mode = 0,speed = 0;
	strcpy(This->id,id);
	memset(This->value,0,sizeof(This->value));
	air_condition_midea.product_key = theConfig.air_condition_midea.product_key;
	air_condition_midea.device_secret = theConfig.air_condition_midea.device_secret;
	This->type_para = &air_condition_midea;
	This->addr = addr;
	This->channel = channel;
	DPRINT("[%s]addr:%x,channel:%d\n",__FUNCTION__,This->addr,This->channel );
	// 初始化属性
	for (i=0; This->type_para->attr[i].name != NULL; i++) {
		This->value[i] = (char *)calloc(1,MAX_VALUE_LENG);
		sprintf(This->value[i],"%s","0");
	}	
	sqlGetMideaAddr(id,&slave_addr,&room_addr);
	sqlGetAirConditionPara(id,&temp,&mode,&speed);
	if (temp < 16)
		temp = 16;
	sprintf(This->value[ATTR_TEMP],"%d",temp);
	sprintf(This->value[ATTR_SPEED],"%d",speed);
	sprintf(This->value[ATTR_MODE],"%d",mode);
	sprintf(This->value[ATTR_SLAVE_ADDR],"%d",slave_addr);
	sprintf(This->value[ATTR_ROOM_ADDR],"%d",room_addr);
	for (i=0; This->type_para->attr[i].name != NULL; i++) {
		printf("[%s]name:%s,value:%s\n",__FUNCTION__,This->type_para->attr[i].name,This->value[i]);
	}	

	return This;
}

