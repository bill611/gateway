/*
 * =============================================================================
 *
 *       Filename:  device_scene.c
 *
 *    Description:  情景控制器
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
#include "device_scene.h"
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
	ATTR_TRIGGER,
	ATTR_TRIGGER1,
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
			if (dev->type_para->attr[i].attrcb)
				dev->type_para->attr[i].attrcb(dev,dev->value[i]);
			break;
		}
	}

    return 0;
}

static void cmdGetSwichStatus(DeviceStr *dev)
{
	int i;
	for (i=0; i<3; i++) {
		smarthomeAllDeviceCmdGetSwichStatus(dev,i);
	}
}

static void reportSceneControl(DeviceStr *dev,int channel)
{
	if (channel == 1) {
		sprintf(dev->value[ATTR_TRIGGER],"1");
		sprintf(dev->value[ATTR_TRIGGER1],"0");
	} else if (channel == 2) {
		sprintf(dev->value[ATTR_TRIGGER],"0");
		sprintf(dev->value[ATTR_TRIGGER1],"1");
	}
	const char *attr_name[] = {
		dev->type_para->attr[ATTR_TRIGGER].name,
		dev->type_para->attr[ATTR_TRIGGER1].name,
		NULL};
	const char *attr_value[] = {
		dev->value[ATTR_TRIGGER],
		dev->value[ATTR_TRIGGER1],
		NULL};
	int attr_value_type[] = {
		dev->type_para->attr[ATTR_TRIGGER].value_type,
		dev->type_para->attr[ATTR_TRIGGER1].value_type,
	};
	aliSdkSubDevReportAttrs(dev,
			attr_name,attr_value,attr_value_type);
}

static DeviceTypePara scene = {
	.name = "scene",
	.short_model = 0x00092316,
	.secret = "2rCCZnGzb76Wgp4G2etzWb2d4gYOidx6",
	.product_key = "a1sMCmcx5N2",
	.device_secret = "",
	.proto_type = ALI_SDK_PROTO_TYPE_ZIGBEE,
	.device_type = DEVICE_TYPE_QJ,
	.attr = {
		{"SceneTrigger",NULL,DEVICE_VELUE_TYPE_INT},
		{"SceneTrigger_1",NULL,DEVICE_VELUE_TYPE_INT},
		{NULL,NULL},
	},
	.getAttr = getAttrCb,
	.setAttr = setAttrCb,
	.reportSceneControl = reportSceneControl,
};


DeviceStr * registDeviceScene(char *id,
		uint16_t addr,
		uint16_t channel,
		char *pk,
		RegistSubDevType regist_type)
{
	if (pk) {
		if (strcmp(pk,scene.product_key) != 0) {
			DPRINT("diff pk :allow pk:%s,now pk:%s\n",
					scene.product_key,pk );	
			return NULL;
		}
	}
	int i;
	DeviceStr *This = (DeviceStr *)calloc(1,sizeof(DeviceStr));
	strcpy(This->id,id);
	memset(This->value,0,sizeof(This->value));
	DPRINT("[%s]key:%s,sec:%s\n",__FUNCTION__,scene.product_key,
		scene.device_secret  );
	This->type_para = &scene;
	This->addr = addr;
	This->channel = channel;
	// 初始化属性
	for (i=0; This->type_para->attr[i].name != NULL; i++) {
		This->value[i] = (char *)calloc(1,MAX_VALUE_LENG);
		sprintf(This->value[i],"%s","0");
	}	

	return This;
}
