/*
 * =============================================================================
 *
 *       Filename:  device_air_box.c
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

#include "device_air_box.h"
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
	ATTR_TEMP,
	ATTR_HUM,
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
}

static DeviceTypePara air_box = {
	.name = "air_box",

	.short_model = 0,
	.secret = "BCCcnkxFXVdi65csHXxJMfiSIcyjSQZCQHoIXdN7",

	.proto_type = ALI_SDK_PROTO_TYPE_ZIGBEE,
	.device_type = DEVICE_TYPE_KQJCY,
	.attr = {
#if (defined V1)
		{"ErrorCode",NULL,DEVICE_VELUE_TYPE_INT},
		{"CurrentTemperature",NULL,DEVICE_VELUE_TYPE_INT},
		{"CurrentHumidity",NULL,DEVICE_VELUE_TYPE_INT},
		{"PM25",NULL,DEVICE_VELUE_TYPE_INT},
#else
		{"Error",NULL,DEVICE_VELUE_TYPE_INT},
		{"CurrentTemperature",NULL,DEVICE_VELUE_TYPE_DOUBLE},
		{"CurrentHumidity",NULL,DEVICE_VELUE_TYPE_INT},
		{"PM25",NULL,DEVICE_VELUE_TYPE_INT},
#endif
		{NULL,NULL},
	},
	.getAttr = getAttrCb,
	.setAttr = setAttrCb,
	.reportAirPara = reportAirParaCb,
};


DeviceStr * registDeviceAirBox(char *id,uint16_t addr,uint16_t channel)
{
	int i;
	DeviceStr *This = (DeviceStr *)calloc(1,sizeof(DeviceStr));
	strcpy(This->id,id);
	memset(This->value,0,sizeof(This->value));
	air_box.product_key = theConfig.air_box.product_key;
	air_box.device_secret = theConfig.air_box.device_secret;
	This->type_para = &air_box;
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
