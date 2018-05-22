/*
 * =============================================================================
 *
 *       Filename:  device_light.c
 *
 *    Description:  灯控设备 
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

#include "cJSON.h"
#include "device_light.h"

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
	// const char *attr_name["a","b",NULL];
	// const char *attr_value["1","2",NULL];
			// alink_subdev_report_attrs(dev->type_para->proto_type,
					// dev->id, attr_name,attr_value);
	for (i=0; dev->type_para->attr[i] != NULL; i++) {
		if (strcmp(attr_set[0],dev->type_para->attr[i]) == 0) {
			const char *attr_name[2] = {NULL};
			const char *attr_value[2] = {NULL};
			attr_name[0] = dev->type_para->attr[i];
			attr_value[0] = dev->value[i];
			printf("[%s]--->%s\n", attr_name[0],attr_value[0]);
			alink_subdev_report_attrs(dev->type_para->proto_type,
					dev->id, attr_name,attr_value);
		}
	}


	// cJSON *root,*par,*par1;
	// char uuid[33] = {0};
	// int ret = -1;
	// char req_params[256] = {0};
	// ret = alink_subdev_get_uuid(light.proto_type,light.dev[0].id,uuid, sizeof(uuid));//or alink_subdev_get_uuid
	// if (ret != 0)
		// return ret;
	// root = cJSON_CreateObject();
	// // cJSON_AddItemToObject(root, "items", par=cJSON_CreateArray());
	// cJSON_AddStringToObject(root, "uuid", uuid);
	// cJSON_AddItemToObject(root, "Luminance", par1=cJSON_CreateObject());
	// cJSON_AddStringToObject(par1, "value", "50");
	// // cJSON_AddStringToObject(par1, "Luminance", "50");
	// char *out = cJSON_PrintUnformatted(root);
	// cJSON_Delete(root);
	// // snprintf(req_params, sizeof(req_params) - 1, SET_PROPERTY_REQ_FMT, uuid, "Luminance", "50");
	// ret = alink_report("postDeviceData", out);
	// printf("req_params:%s\n", out);
	// if (ret != 0)
		// printf("report msg fail, params: %s\n", out);
	// free(out);

    return 0;
}


static int setAttrCb(DeviceStr *dev, const char *attr_name, const char *attr_value)
{
    unsigned int i = 0;
	for (i=0; dev->type_para->attr[i] != NULL; i++) {
		if (strcmp(attr_name,dev->type_para->attr[i]) == 0) {
			sprintf(dev->value[i],"%s",attr_value);
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

static int removeDeviceCb(DeviceStr *dev)
{
    printf("remove device, devid:%s\n",dev->id);
	int i;
	for (i=0; dev->type_para->attr[i] != NULL; i++) {
		if (dev->value[i])
			free(dev->value);
		dev->value[i] = NULL;
	}
	free(dev);
    return 0;
}


static DeviceTypePara light = {
	.name = "light",
	.short_model = 0x00092316,
	.secret = "ZO431NU7020UT9Iu8B8yQnfQbmjagPbRZm7zfuGm",
	.proto_type = PROTO_TYPE_ZIGBEE,
	.attr = {
		"ErrorCode",
		"Switch",
		"Luminance",
	},
	.getAttr = getAttrCb,
	.setAttr = setAttrCb,
	.execCmd = execCmdCb,
	.remove = removeDeviceCb,
};


DeviceStr * registDeviceLight(char *id)
{
	int i;
	DeviceStr *This = (DeviceStr *)calloc(1,sizeof(DeviceStr));
	strcpy(This->id,id);
	memset(This->value,0,sizeof(This->value));
	This->type_para = &light;
	// 初始化属性
	for (i=0; This->type_para->attr[i] != NULL; i++) {
		This->value[i] = (char *)calloc(1,MAX_VALUE_LENG);
		sprintf(This->value[i],"%s","0");
	}	

	return This;
}
