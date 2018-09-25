/*
 * =============================================================================
 *
 *       Filename:  device_door_contact.c
 *
 *    Description:  门磁设备
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

#include "device_door_contact.h"
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
	ATTR_ALARM, // 开关门状态
	ATTR_ARM_DISARM,  // 布防撤防
};
enum {
	EVENT_ERROR,
	EVENT_LOWELECTRICITYALARM,// 低电报警
	EVENT_TAMPERALARM,// 防撬报警
	EVENT_ACTIVEALARM,// 感应报警,布防状态时才上报报警
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

static void cmdTemplateEnableSwitch(DeviceStr *dev,char *value)
{
	int value_int = atoi(value);
	sprintf(dev->value[ATTR_ARM_DISARM],"%d",value_int);
	sqlSetMotionCurtainArmStatus(dev->id,value_int);
}

static void reportAlarmStatus(DeviceStr *dev,char *param)
{
	DPRINT("[%s]%d\n",__FUNCTION__,param[0] );
	int alarm_type = param[0];
	if (alarm_type == TC_ALARM_OPEN_WINDOW) {
		sprintf(dev->value[ATTR_ALARM],"1");
	} else if (alarm_type == TC_ALARM_CLOSE_WINDOW){
		sprintf(dev->value[ATTR_ALARM],"0");
	}
	const char *attr_name[] = {
		dev->type_para->attr[ATTR_ALARM].name,
		dev->type_para->attr[ATTR_ARM_DISARM].name,
		NULL};
	const char *attr_value[] = {
		dev->value[ATTR_ALARM],
		dev->value[ATTR_ARM_DISARM],
		NULL};
	int attr_value_type[] = {
		dev->type_para->attr[ATTR_ALARM].value_type,
		dev->type_para->attr[ATTR_ARM_DISARM].value_type,
	};
	// 属性上报
	aliSdkSubDevReportAttrs(dev,
			attr_name,attr_value,attr_value_type);
	// 报警事件上报
	const char *event_name[] = { NULL};
	const char *event_value[] = {NULL};
	int event_value_type[] = {};
	if (alarm_type == TC_ALARM_LOWPOWER) {
		aliSdkSubDevReportEvent(dev,
				dev->type_para->event[EVENT_LOWELECTRICITYALARM],
				event_name,event_value,event_value_type);
	} else if (alarm_type == TC_ALARM_TAMPER) {
		aliSdkSubDevReportEvent(dev,
				dev->type_para->event[EVENT_TAMPERALARM],
				event_name,event_value,event_value_type);
	}
	int arm_status = atoi(dev->value[ATTR_ARM_DISARM]);
	if (arm_status && alarm_type == TC_ALARM_OPEN_WINDOW) {
		aliSdkSubDevReportEvent(dev,
				dev->type_para->event[EVENT_ACTIVEALARM],
				event_name,event_value,event_value_type);
	}
}


static DeviceTypePara door_contact = {
	.name = "door_contact",
	.short_model = 0x00332560,
	.secret = "i7ctpLwEHLYoOys5IjCnEiUGxyAIlwcMUEQus385",
	.product_key = "a1SjmioRLkD",
	.device_secret = "",

	.proto_type = ALI_SDK_PROTO_TYPE_ZIGBEE,
	.device_type = DEVICE_TYPE_MC,
	.attr = {
#if (defined V1)
		{"ContactAlarm",NULL},
		{"BatteryPercentage",NULL,DEVICE_VELUE_TYPE_INT},
		{"TamperAlarm",NULL,DEVICE_VELUE_TYPE_INT},
#else
		{"ContactState",NULL,DEVICE_VELUE_TYPE_INT},
		{"TemplateEnableSwitch",cmdTemplateEnableSwitch,DEVICE_VELUE_TYPE_INT},
#endif
		{NULL,NULL},
	},
#if (defined V2)
	.event = {
		"Error",
		"LowElectricityAlarm",
		"TamperAlarm",
		"ActiveAlarm",
	},
#endif
	.getAttr = getAttrCb,
	.setAttr = setAttrCb,
	.reportAlarmStatus = reportAlarmStatus,
};


DeviceStr * registDeviceDoorContact(char *id,uint16_t addr,uint16_t channel)
{
	int i;
	DeviceStr *This = (DeviceStr *)calloc(1,sizeof(DeviceStr));
	int arm_status = 0;
	strcpy(This->id,id);
	memset(This->value,0,sizeof(This->value));
	This->type_para = &door_contact;
	This->addr = addr;
	This->channel = channel;
	DPRINT("[%s]addr:%x,channel:%d\n",__FUNCTION__,This->addr,This->channel );
	// 初始化属性
	for (i=0; This->type_para->attr[i].name != NULL; i++) {
		This->value[i] = (char *)calloc(1,MAX_VALUE_LENG);
		sprintf(This->value[i],"%s","0");
	}
	sqlGetDoorContactArmStatus(id,&arm_status);
	sprintf(This->value[ATTR_ARM_DISARM],"%d",arm_status);
	for (i=0; This->type_para->attr[i].name != NULL; i++) {
		printf("[%s]name:%s,value:%s\n",__FUNCTION__,This->type_para->attr[i].name,This->value[i]);
	}

	return This;
}
