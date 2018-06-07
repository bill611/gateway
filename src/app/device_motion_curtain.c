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
	const char *attr_name[4] = {
		dev->type_para->attr[ATTR_ALARM].name,
		dev->type_para->attr[TC_ALARM_LOWPOWER].name,
		dev->type_para->attr[ATTR_TAMPERALARM].name,
		NULL};
	const char *attr_value[4] = {
		dev->value[ATTR_ALARM],
		dev->value[TC_ALARM_LOWPOWER],
		dev->value[ATTR_TAMPERALARM],
		NULL};
	alink_subdev_report_attrs(dev->type_para->proto_type,
			dev->id, attr_name,attr_value);
}


static DeviceTypePara motion_cuntain = {
	.name = "motion_curtain",
	.short_model = 0x005c2503,
	.secret = "RoBoY85GiDdhdxyfhVuJ8peRav2HLKQjlW57880S",
	.proto_type = PROTO_TYPE_ZIGBEE,
	.device_type = DEVICE_TYPE_HW,
	.attr = {
		{"MotionCurtainAlarm",NULL},
		{"BatteryPercentage",NULL},
		{"TamperAlarm",NULL},
		{NULL,NULL},
	},
	.getAttr = getAttrCb,
	.setAttr = setAttrCb,
	.execCmd = execCmdCb,
	.remove = removeDeviceCb,
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
