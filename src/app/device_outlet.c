/*
 * =============================================================================
 *
 *       Filename:  device_outlet.c
 *
 *    Description:  插座设备 
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

#include "sql_handle.h"
#include "device_outlet.h"
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
	ATTR_POWER,
	ATTR_QUANTITY,
	ATTR_CLEAR_QUANTITY,
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
			int attr_value_type[2];
			attr_name[0] = dev->type_para->attr[i].name;
			attr_value[0] = dev->value[i];
			// printf("[%s]--->%s\n", attr_name[0],attr_value[0]);
			attr_value_type[0] = dev->type_para->attr[i].value_type;
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
	if (value_int)
		smarthomeLightCmdCtrOpen(dev,1);
	else
		smarthomeLightCmdCtrClose(dev,1);
}

static void cmdGetSwichStatus(DeviceStr *dev)
{
	int i;
	for (i=0; i<3; i++) {
		smarthomeAllDeviceCmdGetSwichStatus(dev,i);
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief cmdClearQuantity 清除总电量
 *
 * @param dev
 */
/* ---------------------------------------------------------------------------*/
static void cmdClearQuantity(DeviceStr *dev)
{
	
}

static void reportPowerOnCb(DeviceStr *dev,char *param)
{
	// 固定为开
	sprintf(dev->value[ATTR_SWICH],"1");
	const char *attr_name[] = {
	// app调节范围为2-4,实际新风调节范围为1-3,所以要+1
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

static void reportEleQuantityCb(DeviceStr *dev,char *param)
{
	int quantity = sqlGetEleQuantity(dev->id);
	quantity += ((int)param[0] << 16) + (int)param[1];
	sqlSetEleQuantity(quantity,dev->id);
	sprintf(dev->value[ATTR_QUANTITY],"%f",quantity/1000.000);
	const char *attr_name[] = {
		dev->type_para->attr[ATTR_QUANTITY].name,
		NULL};
	const char *attr_value[] = {
		dev->value[ATTR_QUANTITY],
		NULL};
	int attr_value_type[] = {
		dev->type_para->attr[ATTR_QUANTITY].value_type,
	};
	aliSdkSubDevReportAttrs(dev,
			attr_name,attr_value,attr_value_type);
}

static void reportElePowerCb(DeviceStr *dev,char *param)
{
	int power = ((int)param[0] << 16) + (int)param[1];
	sprintf(dev->value[ATTR_POWER],"%d",power);
	const char *attr_name[] = {
	// app调节范围为2-4,实际新风调节范围为1-3,所以要+1
		dev->type_para->attr[ATTR_POWER].name,
		NULL};
	const char *attr_value[] = {
		dev->value[ATTR_POWER],
		NULL};
	int attr_value_type[] = {
		dev->type_para->attr[ATTR_POWER].value_type,
	};
	aliSdkSubDevReportAttrs(dev,
			attr_name,attr_value,attr_value_type);
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief 计量插座10A
 */
/* ---------------------------------------------------------------------------*/
static DeviceTypePara outlet10 = {
	.name = "outlet10",
	.short_model = 0x000a23c3,
	.secret = "tPIMShBYjubW3SWJKw1o1XqxRbM8bcTrR2Fi0nsQ",
	.proto_type = ALI_SDK_PROTO_TYPE_ZIGBEE,
	.device_type = DEVICE_TYPE_JLCZ10,
	.attr = {
#if (defined V1)
		{"ErrorCode",NULL,DEVICE_VELUE_TYPE_INT},
		{"Switch",cmdSwich,DEVICE_VELUE_TYPE_INT},
		{"Power",NULL,DEVICE_VELUE_TYPE_INT},
		{"SumElectric",NULL,DEVICE_VELUE_TYPE_DOUBLE},
		{"ClearConsumption",NULL,DEVICE_VELUE_TYPE_INT},
#else
		{"Error",NULL,DEVICE_VELUE_TYPE_INT},
		{"PowerSwitch",cmdSwich,DEVICE_VELUE_TYPE_INT},
		{"RealTimePower",NULL,DEVICE_VELUE_TYPE_INT},
		{"TotalConsumption",NULL,DEVICE_VELUE_TYPE_DOUBLE},
		{"ClearConsumption",NULL,DEVICE_VELUE_TYPE_INT},
#endif
		{NULL,NULL},
	},
	.getAttr = getAttrCb,
	.setAttr = setAttrCb,
	.getSwichStatus = cmdGetSwichStatus,
	.reportPowerOn = reportPowerOnCb,
	.reportPowerOff = reportPowerOffCb,
	.reportElePower = reportElePowerCb,
	.reportEleQuantity = reportEleQuantityCb,
};

/* ---------------------------------------------------------------------------*/
/**
 * @brief 计量插座16A
 */
/* ---------------------------------------------------------------------------*/
static DeviceTypePara outlet16 = {
	.short_model = 0,
};

DeviceStr * registDeviceOutlet10(char *id,uint16_t addr,uint16_t channel)
{
	int i;
	DeviceStr *This = (DeviceStr *)calloc(1,sizeof(DeviceStr));
	strcpy(This->id,id);
	memset(This->value,0,sizeof(This->value));
	outlet10.product_key = theConfig.outlet.product_key;
	outlet10.device_secret = theConfig.outlet.device_secret;
	This->type_para = &outlet10;
	This->addr = addr;
	This->channel = channel;
	// 初始化属性
	for (i=0; This->type_para->attr[i].name != NULL; i++) {
		This->value[i] = (char *)calloc(1,MAX_VALUE_LENG);
		sprintf(This->value[i],"%s","0");
	}	

	return This;
}
DeviceStr * registDeviceOutlet16(char *id,uint16_t addr,uint16_t channel)
{
	int i;
	DeviceStr *This = (DeviceStr *)calloc(1,sizeof(DeviceStr));
	strcpy(This->id,id);
	memset(This->value,0,sizeof(This->value));
	if (outlet16.short_model == 0) {
		memcpy(&outlet16,&outlet10,sizeof(DeviceTypePara));
		outlet16.name = "outlet16";
		outlet16.device_type = DEVICE_TYPE_JLCZ16;
		outlet16.product_key = theConfig.outlet.product_key;
		outlet16.device_secret = theConfig.outlet.device_secret;
	}
	This->type_para = &outlet16;
	This->addr = addr;
	This->channel = channel;
	// 初始化属性
	for (i=0; This->type_para->attr[i].name != NULL; i++) {
		This->value[i] = (char *)calloc(1,MAX_VALUE_LENG);
		sprintf(This->value[i],"%s","0");
	}	

	return This;
}
