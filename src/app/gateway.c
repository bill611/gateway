/*
 * =============================================================================
 *
 *       Filename:  gateway.c
 *
 *    Description:  网关设备
 *
 *        Version:  1.0
 *        Created:  2018-05-08 16:26:03
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
#include "device_protocol.h"
#include "alink_export_gateway.h"
#include "alink_export_subdev.h"
#include "platform.h"		/* Header */
#include "iwlib.h"		/* Header */

#include "device_light.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/
extern char *calc_subdev_signature(const char *secret,
		const uint8_t rand[SUBDEV_RAND_BYTES],
        char *sign_buff,
	   	uint32_t buff_size);

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
#define MAX_SUB_DEVICE_NUM 	 79  // 最大支持子设备数量
#define DEVMGR_SERVICE_AUTHORISE_DEVICE_LIST    "AuthoriseDeviceList"
#define DEVMGR_SERVICE_REMOVE_DEVICE            "RemoveDevice"
#define DEVMGR_ATTRIBUTE_JOINED_DEVICE_LIST     "JoinedDeviceList"
#define DEVMGR_SERVICE_PERMITJOIN_DEVICE        "PermitJoin"


#define GW_ATTR_ARM_ENTRY_DELAY                "ArmEntryDelay"
#define GW_SERVICE_FACTORY_RESET                "FactoryReset"

typedef struct {
	unsigned int cnt;
	DeviceStr *dev[MAX_SUB_DEVICE_NUM];
}SubDevice;
/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static SubDevice sub_device;


static char sound_alarm[2] = {'0', '\0'};

static int __factory_reset_service_cb(char *args, char *output_buf, unsigned int buf_sz)
{
    printf("exec gateway factory_reset service, args:%s\n", args);
    int ret = 0;
    //1.firmware reset
    //reset config, ....

    //2.alink reset
    ret = alink_factory_reset(ALINK_REBOOT);
    if (ret != 0) {
        printf("call function alink_factory_reset fail!\n");
    }

    return 0;
}
int register_gateway_service(void)
{
    //register service
    int ret = alink_register_service(GW_SERVICE_FACTORY_RESET, __factory_reset_service_cb);
    if (0 != ret) {
        printf("register service fail, service:%s\n", GW_SERVICE_FACTORY_RESET);
        return -1;
    }

    return 0;
}

static int __sound_alarm_attr_get_cb(char *output_buf, unsigned int buf_sz)
{
    printf("get gateway sound_alarm attr, value:%s\n", sound_alarm);
    snprintf(output_buf, buf_sz - 1, sound_alarm);

    return 0;
}

static int __sound_alarm_attr_set_cb(char *value)
{
    printf("set gateway sound_alarm attr, value:%s\n", value);
    if (*value != '0' && *value != '1') {
        printf("invalid sound_alarm attr value:%s\n", value);
        return -1;
    }
    sound_alarm[0] = *value;

    return 0;
}


int register_gateway_attribute(void)
{
    int ret = alink_register_attribute(GW_ATTR_ARM_ENTRY_DELAY,
		   	__sound_alarm_attr_get_cb, __sound_alarm_attr_set_cb);
    if (0 != ret) {
        printf("register attribute fail, attribute:%s\n", GW_ATTR_ARM_ENTRY_DELAY);
        return -1;
    }

    return 0;

}


/* ---------------------------------------------------------------------------*/
/**
 * @brief register_sub_device 注册子设备
 *
 * @returns 
 */
/* ---------------------------------------------------------------------------*/
int register_sub_device(void)
{
    int ret = -1;
	unsigned int i;
	for (i=0; i<sub_device.cnt; i++) {
		char rand[SUBDEV_RAND_BYTES] = {0};
		char sign[17] = {0};
		if (calc_subdev_signature(sub_device.dev[i]->type_para->secret, rand, sign, sizeof(sign)) == NULL) {
			printf("[%s:%s]__get_device_signature fail\n",sub_device.dev[i]->id,sub_device.dev[i]->type_para->name);
			continue;
		}
		ret = alink_subdev_register_device(sub_device.dev[i]->type_para->proto_type,
				sub_device.dev[i]->id,
				sub_device.dev[i]->type_para->short_model,
				rand, sign);
		if (ret != 0) 
			printf("[%s]register sub device fail,id:%s",
					sub_device.dev[i]->type_para->name,sub_device.dev[i]->id);
	}

    return ret;
}

int device_private_property_test(void)
{
    int ret = -1;
    char req_params[256] = {0};
    char uuid[33] = {0};
    char key[16] = "ArmMode";
    char value[16] = "1";

    ret = alink_get_main_uuid(uuid, sizeof(uuid));//or alink_subdev_get_uuid
    if (ret != 0)
        return ret;

    snprintf(req_params, sizeof(req_params) - 1, SET_PROPERTY_REQ_FMT, uuid, key, value);
    ret = alink_report("postDeviceData", "{\"ArmMode\": { \"value\": \"1\"  }}");
    if (ret != 0)
        printf("report msg fail, params: %s\n", req_params);

    // ret = set_property_to_cloud(key, value);
    // value[0] = '0';
    // ret = get_property_from_cloud(key, value, sizeof(key));

    return ret;
}

static int getAttrCb(const char *devid, const char *attr_set[])
{
	unsigned int i;
	for (i=0; i<sub_device.cnt; i++) {
		if (strcmp(sub_device.dev[i]->id,devid) == 0) {
			sub_device.dev[i]->type_para->getAttr(sub_device.dev[i],attr_set);
			break;
		}
	}

    return 0;
}


static int setAttrCb(const char *devid, const char *attr_name, const char *attr_value)
{
	unsigned int i;
	for (i=0; i<sub_device.cnt; i++) {
		if (strcmp(sub_device.dev[i]->id,devid) == 0) {
			sub_device.dev[i]->type_para->setAttr(sub_device.dev[i],attr_name,attr_value);
			break;
		}
	}

    return 0;
}

static int execCmdCb(const char *devid, const char *cmd_name, const char *cmd_args)
{
	unsigned int i;
	for (i=0; i<sub_device.cnt; i++) {
		if (strcmp(sub_device.dev[i]->id,devid) == 0) {
			sub_device.dev[i]->type_para->execCmd(sub_device.dev[i],cmd_name,cmd_args);
			break;
		}
	}
    return 0;
}

static int removeDeviceCb(const char *devid)
{
	unsigned int i;
	for (i=0; i<sub_device.cnt; i++) {
		if (strcmp(sub_device.dev[i]->id,devid) == 0) {
			sub_device.dev[i]->type_para->remove(sub_device.dev[i]);
			break;
		}
	}
    return 0;
}

static int permitSubDeviceJoinCb(uint8_t duration)
{
    printf("permitSubDeviceJoinCb, devid:%d\n",duration);
	return 0;
}
void deviceInit(void)
{
    int ret = -1;
	proto_info_t proto_info;
    memset(&proto_info, 0, sizeof(proto_info));
    proto_info.proto_type = PROTO_TYPE_ZIGBEE;
    strncpy(proto_info.protocol_name, "zigbee", sizeof(proto_info.protocol_name) - 1);

    int i = 0;
    proto_info.callback[i].cb_type = GET_SUB_DEVICE_ATTR;
    proto_info.callback[i].cb_func = getAttrCb;

    i++;
    proto_info.callback[i].cb_type = SET_SUB_DEVICE_ATTR;
    proto_info.callback[i].cb_func = setAttrCb;

    i++;
    proto_info.callback[i].cb_type = EXECUTE_SUB_DEVICE_CMD;
    proto_info.callback[i].cb_func = execCmdCb;

    i++;
    proto_info.callback[i].cb_type = REMOVE_SUB_DEVICE;
    proto_info.callback[i].cb_func = removeDeviceCb;

    i++;
    proto_info.callback[i].cb_type = PERMIT_JOIN_SUB_DEVICE;
    proto_info.callback[i].cb_func = permitSubDeviceJoinCb;

	ret = alink_subdev_register_device_type(&proto_info);

	if (ret != 0) 
		printf("register sub device type fail");

	sub_device.dev[sub_device.cnt] = registDeviceLight("123456");
	sub_device.cnt++;
}
