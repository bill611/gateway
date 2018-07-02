/*
 * =============================================================================
 *
 *       Filename:  device_motion_curtain.c
 *
 *    Description:  红外幕帘设备 
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

#include "device_motion_curtain.h"
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
	ATTR_ALARM,
	ATTR_BATTERYPERCENTAGE,
	ATTR_TAMPERALARM,
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
			attr_value_type[0] = dev->type_para->attr[i].value_type;
			// printf("[%s]--->%s\n", attr_name[0],attr_value[0]);
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
			printf("[%s,%s]%s:%s\n",__FUNCTION__,__FILE__,attr_name,attr_value);
			if (dev->type_para->attr[i].attrcb)
				dev->type_para->attr[i].attrcb(dev,dev->value[i]);
			break;
		}
	}

    return 0;
}

static void reportAlarmStatus(DeviceStr *dev,char *param)
{
	printf("[%s]%d\n",__FUNCTION__,param[0] );
	int alarm_type = param[0];
	if (alarm_type == TC_ALARM_ACTION)
		sprintf(dev->value[ATTR_ALARM],"1");
	else if (alarm_type == TC_ALARM_LOWPOWER)
		sprintf(dev->value[ATTR_BATTERYPERCENTAGE],"20");
	else if (alarm_type == TC_ALARM_TAMPER)
		sprintf(dev->value[ATTR_TAMPERALARM],"0");
	const char *attr_name[] = {
		dev->type_para->attr[ATTR_ALARM].name,
		dev->type_para->attr[ATTR_BATTERYPERCENTAGE].name,
		dev->type_para->attr[ATTR_TAMPERALARM].name,
		NULL};
	const char *attr_value[] = {
		dev->value[ATTR_ALARM],
		dev->value[ATTR_BATTERYPERCENTAGE],
		dev->value[ATTR_TAMPERALARM],
		NULL};
	int attr_value_type[] = {
		dev->type_para->attr[ATTR_ALARM].value_type,
		dev->type_para->attr[ATTR_BATTERYPERCENTAGE].value_type,
		dev->type_para->attr[ATTR_TAMPERALARM].value_type,
	};
	aliSdkSubDevReportAttrs(dev,
			attr_name,attr_value,attr_value_type);
}


static DeviceTypePara motion_cuntain = {
	.name = "motion_curtain",

	.short_model = 0x005c2503,
	.secret = "RoBoY85GiDdhdxyfhVuJ8peRav2HLKQjlW57880S",

	.product_key = "a1bZnQDt4Sk",
	.device_secret = "UAcApq4PzN7uxu6cAxLpciEN6fkGPusq",

	.proto_type = ALI_SDK_PROTO_TYPE_ZIGBEE,
	.device_type = DEVICE_TYPE_HW,
	.attr = {
#if (defined V1)
		{"MotionCurtainAlarm",NULL},
		{"BatteryPercentage",NULL,DEVICE_VELUE_TYPE_INT},
		{"TamperAlarm",NULL,DEVICE_VELUE_TYPE_INT},
#else
		{"MotionAlarmState",NULL,DEVICE_VELUE_TYPE_INT},
		{"BatteryPercentage",NULL,DEVICE_VELUE_TYPE_INT},
		{"TamperAlarm",NULL,DEVICE_VELUE_TYPE_INT},
#endif
		{NULL,NULL},
	},
	.getAttr = getAttrCb,
	.setAttr = setAttrCb,
	.reportAlarmStatus = reportAlarmStatus,
};


DeviceStr * registDeviceMotionCurtain(char *id,uint16_t addr,uint16_t channel)
{
	int i;
	DeviceStr *This = (DeviceStr *)calloc(1,sizeof(DeviceStr));
	strcpy(This->id,id);
	memset(This->value,0,sizeof(This->value));
	This->type_para = &motion_cuntain;
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
