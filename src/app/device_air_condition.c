/*
 * =============================================================================
 *
 *       Filename:  device_air_condition.c
 *
 *    Description:  空调设备 
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

#include "device_air_condition.h"
#include "sql_handle.h"

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
};

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static int getAttrCb(DeviceStr *dev, const char *attr_set[])
{
    printf("get attr, devid:%s, attribute name:\n", dev->id);
    unsigned int i = 0;
    while (attr_set[i++]) {
        printf("attr_%d: %s\n", i - 1, attr_set[i - 1]);
    }
	for (i=0; dev->type_para->attr[i].name != NULL; i++) {
		if (strcmp(attr_set[0],dev->type_para->attr[i].name) == 0) {
			const char *attr_name[2] = {NULL};
			const char *attr_value[2] = {NULL};
			attr_name[0] = dev->type_para->attr[i].name;
			attr_value[0] = dev->value[i];
			// printf("[%s]--->%s\n", attr_name[0],attr_value[0]);
			alink_subdev_report_attrs(dev->type_para->proto_type,
					dev->id, attr_name,attr_value);
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
			printf("[%s,%s]%s:%s\n",__FUNCTION__,__FILE__,attr_name,attr_value);
			if (dev->type_para->attr[i].attrcb)
				dev->type_para->attr[i].attrcb(dev,dev->value[i]);
			break;
		}
	}

    return 0;
}

static int execCmdCb(DeviceStr *dev, const char *cmd_name, const char *cmd_args)
{
    printf("exec cmd, devid:%s, cmd_name:%s, cmd_args:%s\n",
           dev->id, cmd_name, cmd_args);
    return 0;
}

static int removeDeviceCb(DeviceStr **device)
{
	DeviceStr *dev = *device;
    printf("remove device, devid:%s\n",dev->id);
	int i;
	for (i=0; dev->type_para->attr[i].name != NULL; i++) {
		if (dev->value[i]) {
			free(dev->value[i]);
		}
		dev->value[i] = NULL;
	}
	sqlDeleteDevice(dev->id);
	free(dev);
	*device = NULL;
    return 0;
}

static void cmdSwich(DeviceStr *dev,char *value)
{
	int value_int = atoi(value);
	sprintf(dev->value[ATTR_SWICH],"%s",value);
	printf("[%s]value:%s,int:%d,buf:%s,speed:%s\n",
			__FUNCTION__,
			value,
			value_int,
			dev->value[ATTR_SWICH],
			dev->value[ATTR_SPEED] );
	if (value_int) {
		uint8_t speed = atoi(dev->value[ATTR_SPEED]);
		if (speed)// app调节范围为2-4,实际新风调节范围为1-3,所以要-1
			speed -= 1;
		printf("%s:%d\n", __FUNCTION__,speed);
		smarthomeFreshAirCmdCtrOpen(dev,speed);
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
	// char change[] = {2,0,1,4,3}; // 模式转换
	// int value_int = atoi(value);
	sprintf(dev->value[ATTR_MODE],"%s",value);
	smarthomeAirCondtionCmdCtrOpen(dev,
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
	// char change[] = {0,0,3,2,1}; // 模式转换
	// int value_int = atoi(value);
	sprintf(dev->value[ATTR_SPEED],"%s",value);
	smarthomeAirCondtionCmdCtrOpen(dev,
			atoi(dev->value[ATTR_TEMP]),
			atoi(dev->value[ATTR_MODE]),
			atoi(dev->value[ATTR_SPEED]));
}

static void cmdTemperature(DeviceStr *dev,char *value)
{
	// int value_int = atoi(value);
	sprintf(dev->value[ATTR_TEMP],"%s",value);
	// if (value_int) {
		// printf("[%s]%d\n", __FUNCTION__,value_int);
		// // uint8_t speed = atoi(dev->value[ATTR_TEMP]);
		// // if (speed)// app调节范围为2-4,实际新风调节范围为1-3,所以要-1
			// // speed -=1;
		// smarthomeFreshAirCmdCtrOpen(dev,value_int);
	// }
	smarthomeAirCondtionCmdCtrOpen(dev,
			atoi(dev->value[ATTR_TEMP]),
			atoi(dev->value[ATTR_MODE]),
			atoi(dev->value[ATTR_SPEED]));
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
	alink_subdev_report_attrs(dev->type_para->proto_type,
			dev->id, attr_name,attr_value);
}

static void reportPowerOffCb(DeviceStr *dev)
{
	sprintf(dev->value[ATTR_SWICH],"0");
	const char *attr_name[2] = {
		dev->type_para->attr[ATTR_SWICH].name,
		NULL};
	const char *attr_value[2] = {
		dev->value[ATTR_SWICH],
		NULL};
	alink_subdev_report_attrs(dev->type_para->proto_type,
			dev->id, attr_name,attr_value);
}

static DeviceTypePara air_condition = {
	.name = "air_condition",
	.short_model = 0x00022531,
	.secret = "Xf3r8BQV1Utz5o6EnJfFXF4tE3BhfAKH3ABYaQDr",
	.proto_type = PROTO_TYPE_ZIGBEE,
	.device_type = DEVICE_TYPE_ZYKT,
	.attr = {
		{"ErrorCode",NULL},
		{"Switch",cmdSwich},
		{"WorkMode",cmdWorkMode},
		{"WindSpeed",cmdWindSpeed},
		{"CurrentTemp",NULL},
		{"Temperature",cmdTemperature},
		{NULL,NULL},
	},
	.getAttr = getAttrCb,
	.setAttr = setAttrCb,
	.execCmd = execCmdCb,
	.remove = removeDeviceCb,
	.getSwichStatus = cmdGetSwichStatus,
	.reportPowerOn = reportPowerOnCb,
	.reportPowerOff = reportPowerOffCb,
};


DeviceStr * registDeviceAirCondition(char *id,uint16_t addr,uint16_t channel)
{
	int i;
	DeviceStr *This = (DeviceStr *)calloc(1,sizeof(DeviceStr));
	strcpy(This->id,id);
	memset(This->value,0,sizeof(This->value));
	This->type_para = &air_condition;
	This->addr = addr;
	This->channel = channel;
	printf("[%s]addr:%x,channel:%d\n",__FUNCTION__,This->addr,This->channel );
	// 初始化属性
	for (i=0; This->type_para->attr[i].name != NULL; i++) {
		This->value[i] = (char *)calloc(1,MAX_VALUE_LENG);
		sprintf(This->value[i],"%s","0");
	}	

	return This;
}
