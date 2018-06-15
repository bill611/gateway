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
			printf("[%s]--->%s\n", attr_name[0],attr_value[0]);
			aliSdkSubDevReportAttrs(dev->type_para->proto_type,
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
		if (dev->value[i])
			free(dev->value[i]);
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
	aliSdkSubDevReportAttrs(dev->type_para->proto_type,
			dev->id, attr_name,attr_value);
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
	aliSdkSubDevReportAttrs(dev->type_para->proto_type,
			dev->id, attr_name,attr_value);
}

static DeviceTypePara outlet = {
	.name = "outlet",
	.short_model = 0x000a23c3,
	.secret = "tPIMShBYjubW3SWJKw1o1XqxRbM8bcTrR2Fi0nsQ",
	.proto_type = ALI_SDK_PROTO_TYPE_ZIGBEE,
	.device_type = DEVICE_TYPE_JLCZ,
	.attr = {
		{"ErrorCode",NULL},
		{"Switch",cmdSwich},
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


DeviceStr * registDeviceOutlet(char *id,uint16_t addr,uint16_t channel)
{
	int i;
	DeviceStr *This = (DeviceStr *)calloc(1,sizeof(DeviceStr));
	strcpy(This->id,id);
	memset(This->value,0,sizeof(This->value));
	This->type_para = &outlet;
	This->addr = addr;
	This->channel = channel;
	// 初始化属性
	for (i=0; This->type_para->attr[i].name != NULL; i++) {
		This->value[i] = (char *)calloc(1,MAX_VALUE_LENG);
		sprintf(This->value[i],"%s","0");
	}	

	return This;
}
