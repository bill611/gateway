/*
 * =============================================================================
 *
 *       Filename:  device_fresh_air.c
 *
 *    Description:  新风设备
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

#include "device_fresh_air.h"
#include "config.h"
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
	ATTR_SPEED,
	ATTR_TEMP,
	ATTR_HUM,
	ATTR_TVOC,
	ATTR_PM25,
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
		uint8_t speed = atoi(dev->value[ATTR_SPEED]);
		if (speed)// app调节范围为2-4,实际新风调节范围为1-3,所以要-1
			speed -= 1;
		DPRINT("%s:%d\n", __FUNCTION__,speed);
		smarthomeFreshAirCmdCtrOpen(dev,speed);
	} else
		smarthomeFreshAirCmdCtrClose(dev,0);
}

static void cmdWindSpeed(DeviceStr *dev,char *value)
{
	int value_int = atoi(value);
	sprintf(dev->value[ATTR_SPEED],"%s",value);
	if (value_int) {
		uint8_t speed = atoi(dev->value[ATTR_SPEED]);
		if (speed)// app调节范围为2-4,实际新风调节范围为1-3,所以要-1
			speed -=1;
		smarthomeFreshAirCmdCtrOpen(dev,speed);
	}
}

static void cmdGetSwichStatus(DeviceStr *dev)
{
	// smarthomeAllDeviceCmdGetSwichStatus(dev,1);
}

static void cmdGetAirPara(DeviceStr *dev)
{
	smarthomeFreshAirCmdGetPara(dev,
			atoi(dev->value[ATTR_TEMP]),
			atoi(dev->value[ATTR_HUM]),
			atoi(dev->value[ATTR_TVOC]),
			atoi(dev->value[ATTR_PM25]));
}

static void reportPowerOnCb(DeviceStr *dev,char *param,int channel)
{
	// 固定为开
	sprintf(dev->value[ATTR_SWICH],"1");
	// app调节范围为2-4,实际新风调节范围为1-3,所以要+1
	sprintf(dev->value[ATTR_SPEED],"%d",param[0] + 1);
	const char *attr_name[] = {
		dev->type_para->attr[ATTR_SWICH].name,
		dev->type_para->attr[ATTR_SPEED].name,
		NULL};
	const char *attr_value[] = {
		dev->value[ATTR_SWICH],
		dev->value[ATTR_SPEED],
		NULL};
	int attr_value_type[] = {
		dev->type_para->attr[ATTR_SWICH].value_type,
		dev->type_para->attr[ATTR_SPEED].value_type,
	};
	aliSdkSubDevReportAttrs(dev,
			attr_name,attr_value,attr_value_type);
}

static void reportPowerOffCb(DeviceStr *dev,int channel)
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
		dev->type_para->attr[ATTR_SPEED].value_type,
	};
	aliSdkSubDevReportAttrs(dev,
			attr_name,attr_value,attr_value_type);
}

static void reportAirParaCb(DeviceStr *dev,char *param)
{
	int value = ((int)param[0] << 16) + (int)param[1];
	sprintf(dev->value[ATTR_PM25],"%d",value);
	value = ((int)param[2] << 16) + (int)param[3];
	sprintf(dev->value[ATTR_TEMP],"%d",value);
	value = ((int)param[4] << 16) + (int)param[5];
	sprintf(dev->value[ATTR_HUM],"%d",value);
	const char *attr_name[] = {
		dev->type_para->attr[ATTR_TEMP].name,
		dev->type_para->attr[ATTR_HUM].name,
		dev->type_para->attr[ATTR_PM25].name,
		NULL};
	const char *attr_value[] = {
		dev->value[ATTR_TEMP],
		dev->value[ATTR_HUM],
		dev->value[ATTR_PM25],
		NULL};
	int attr_value_type[] = {
		dev->type_para->attr[ATTR_TEMP].value_type,
		dev->type_para->attr[ATTR_HUM].value_type,
		dev->type_para->attr[ATTR_PM25].value_type,
	};
	aliSdkSubDevReportAttrs(dev,
			attr_name,attr_value,attr_value_type);
	smarthomeFreshAirCmdGetPara(dev,
			atoi(dev->value[ATTR_TEMP]),
			atoi(dev->value[ATTR_HUM]),
			atoi(dev->value[ATTR_TVOC]),
			atoi(dev->value[ATTR_PM25]));
}


static DeviceTypePara fresh_air = {
	.name = "fresh_air",

	.short_model = 0x002824cd,
	.secret = "BCCcnkxFXVdi65csHXxJMfiSIcyjSQZCQHoIXdN7",

	.proto_type = ALI_SDK_PROTO_TYPE_ZIGBEE,
	.device_type = DEVICE_TYPE_XFXT,
	.attr = {
#if (defined V1)
		{"ErrorCode",NULL,DEVICE_VELUE_TYPE_INT},
		{"Switch",cmdSwich,DEVICE_VELUE_TYPE_INT},
		{"WindSpeed",cmdWindSpeed,DEVICE_VELUE_TYPE_INT},
		{"CurrentTemperature",NULL,DEVICE_VELUE_TYPE_INT},
		{"CurrentHumidity",NULL,DEVICE_VELUE_TYPE_INT},
		{"TVOC",NULL,DEVICE_VELUE_TYPE_DOUBLE},
		{"PM25",NULL,DEVICE_VELUE_TYPE_INT},
#else
		{"Error",NULL,DEVICE_VELUE_TYPE_INT},
		{"PowerSwitch",cmdSwich,DEVICE_VELUE_TYPE_INT},
		{"WindSpeed",cmdWindSpeed,DEVICE_VELUE_TYPE_INT},
		{"CurrentTemperature",NULL,DEVICE_VELUE_TYPE_DOUBLE},
		{"CurrentHumidity",NULL,DEVICE_VELUE_TYPE_INT},
		{"TVOC",NULL,DEVICE_VELUE_TYPE_DOUBLE},
		{"PM25",NULL,DEVICE_VELUE_TYPE_INT},
#endif
		{NULL,NULL},
	},
	.getAttr = getAttrCb,
	.setAttr = setAttrCb,
	.getSwichStatus = cmdGetSwichStatus,
	.getAirPara = cmdGetAirPara,
	.reportPowerOn = reportPowerOnCb,
	.reportPowerOff = reportPowerOffCb,
	.reportAirPara = reportAirParaCb,
};


DeviceStr * registDeviceFreshAir(char *id,uint16_t addr,uint16_t channel)
{
	int i;
	DeviceStr *This = (DeviceStr *)calloc(1,sizeof(DeviceStr));
	strcpy(This->id,id);
	memset(This->value,0,sizeof(This->value));
	fresh_air.product_key = theConfig.fresh_air.product_key;
	fresh_air.device_secret = theConfig.fresh_air.device_secret;
	This->type_para = &fresh_air;
	This->addr = addr;
	This->channel = channel;
	DPRINT("[%s]addr:%x,channel:%d\n",__FUNCTION__,This->addr,This->channel );
	// 初始化属性
	for (i=0; This->type_para->attr[i].name != NULL; i++) {
		This->value[i] = (char *)calloc(1,MAX_VALUE_LENG);
		sprintf(This->value[i],"%s","0");
	}

	return This;
}
