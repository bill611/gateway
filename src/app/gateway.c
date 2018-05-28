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
#include "debug.h"
#include "linklist.h"		/* Header */
#include "device_protocol.h"
#include "sql_handle.h"
#include "zigbee.h"
#include "alink_export_gateway.h"
#include "alink_export_subdev.h"
#include "platform.h"		/* Header */
#include "iwlib.h"		/* Header */

#include "device_light.h"
#include "device_fresh_air.h"
#include "device_motion_curtain.h"

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
void gwGetSwichStatus(void);

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
#define MAX_SUB_DEVICE_NUM 	 79  // 最大支持子设备数量
#define DEVMGR_SERVICE_AUTHORISE_DEVICE_LIST    "AuthoriseDeviceList"
#define DEVMGR_SERVICE_REMOVE_DEVICE            "RemoveDevice"
#define DEVMGR_ATTRIBUTE_JOINED_DEVICE_LIST     "JoinedDeviceList"
#define DEVMGR_SERVICE_PERMITJOIN_DEVICE        "PermitJoin"


#define GW_SERVICE_FACTORY_RESET                "FactoryReset"

typedef struct {
	unsigned int cnt;
	List *list;
	DeviceStr *dev;
}SubDevice;

typedef struct {
	int device_type;	
	DeviceStr* (*regist) (char *,uint16_t,uint16_t);
}SubDeviceRegist;

typedef struct {
	char *attr;	
	int (*getCb)(char *output_buf, unsigned int buf_sz);
	int (*setCb)(char *value);
	char *default_value;	
}GateWayAttr;

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static List *sub_dev_list = NULL; // 链表保存子设备
// static SubDevice sub_device;
static SubDeviceRegist device_regist[] = {
	{DEVICE_TYPE_DK,registDeviceLight},
	{DEVICE_TYPE_XFXT,registDeviceFreshAir},
	{DEVICE_TYPE_HW,registDeviceMotionCurtain},
};

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
int gwRegisterGatewayService(void)
{
    //register service
    int ret = alink_register_service(GW_SERVICE_FACTORY_RESET, __factory_reset_service_cb);
    if (0 != ret) {
        printf("register service fail, service:%s\n", GW_SERVICE_FACTORY_RESET);
        return -1;
    }

    return 0;
}

static int alarmEntryDelayGetCb(char *output_buf, unsigned int buf_sz)
{
    printf("[%s]:%s\n", __FUNCTION__,sound_alarm);
    snprintf(output_buf, buf_sz - 1, sound_alarm);
	gwGetSwichStatus();
    return 0;
}

static int alarmEntryDelaySetCb(char *value)
{
    printf("[%s]:%s\n", __FUNCTION__,value);
    if (*value != '0' && *value != '1') {
        printf("invalid sound_alarm attr value:%s\n", value);
        return -1;
    }
    sound_alarm[0] = *value;

    return 0;
}

static int alarmAlarmModeGetCb(char *output_buf, unsigned int buf_sz)
{
    printf("[%s]:%s\n", __FUNCTION__,sound_alarm);
    snprintf(output_buf, buf_sz - 1, "1");
    return 0;
}

static int alarmAlarmModeSetCb(char *value)
{
    printf("[%s]:%s\n", __FUNCTION__,value);
    if (*value != '0' && *value != '1') {
        printf("invalid sound_alarm attr value:%s\n", value);
        return -1;
    }
    sound_alarm[0] = *value;

    return 0;
}


static GateWayAttr gw_attrs[] = {
	{"ArmEntryDelay",alarmEntryDelayGetCb,alarmEntryDelaySetCb},
	{"ArmMode",alarmAlarmModeGetCb,alarmAlarmModeSetCb},
	{NULL},
};

int gwRegisterGatewayAttribute(void)
{
	int i;
	for (i=0; gw_attrs[i].attr != NULL; i++) {
		int ret = alink_register_attribute(gw_attrs[i].attr,
				gw_attrs[i].getCb, gw_attrs[i].setCb);
		if (0 != ret) {
			printf("register attribute fail, attribute:%s\n", gw_attrs[i].attr);
		}
	}

    return 0;

}

/* ---------------------------------------------------------------------------*/
/**
 * @brief gwRegisterSubDevice  注册子设备
 *
 * @param id 子设备IEEE长地址转为字符串作为唯一ID
 * @param type 子设备类型
 * @param addr 子设备短地址
 * @param channel 子设备通道数量
 *
 * @returns 0成功 -1失败
 */
/* ---------------------------------------------------------------------------*/
int gwRegisterSubDevice(char *id,int type,uint16_t addr,uint16_t channel)
{
    int ret = -1;
	unsigned int i;
	for (i=0; i<NELEMENTS(device_regist); i++) {
		if (device_regist[i].device_type == type)	
			break;
	}
	if (i == NELEMENTS(device_regist)) {
		printf("unknow device type:%d\n",type );
		return -1;
	}
	DeviceStr *dev = device_regist[i].regist(id,addr,channel);
	sub_dev_list->append(sub_dev_list,&dev);

	char rand[SUBDEV_RAND_BYTES] = {0};
	char sign[17] = {0};
	if (calc_subdev_signature(dev->type_para->secret,
			   	rand, sign, sizeof(sign)) == NULL) {
		printf("[%s:%s]__get_device_signature fail\n",
				dev->id, dev->type_para->name);
	}
	ret = alink_subdev_register_device(
			dev->type_para->proto_type,
			dev->id,
			dev->type_para->short_model,
			rand, sign);
	if (ret != 0) 
		printf("[%s]register sub device fail,id:%s\n",
				dev->type_para->name, dev->id);
    return ret;
}

int gwDevicePrivatePropertyTest(void)
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
	sub_dev_list->foreachStart(sub_dev_list,0);
	while(sub_dev_list->foreachEnd(sub_dev_list)) {
		DeviceStr *dev;
		sub_dev_list->foreachGetElem(sub_dev_list,&dev);
		if (strcmp(dev->id,devid) == 0) {
			dev->type_para->getAttr(dev,attr_set);
			break;
		}		
		sub_dev_list->foreachNext(sub_dev_list);
	}
    return 0;
}


static int setAttrCb(const char *devid, const char *attr_name, const char *attr_value)
{
	printf("gate way id:%s,%s:%s\n", devid,attr_name,attr_value);
	sub_dev_list->foreachStart(sub_dev_list,0);
	while(sub_dev_list->foreachEnd(sub_dev_list)) {
		DeviceStr *dev;
		sub_dev_list->foreachGetElem(sub_dev_list,&dev);
		printf("dev:%s\n", dev->id);
		if (strcmp(dev->id,devid) == 0) {
			dev->type_para->setAttr(dev,attr_name,attr_value);
			break;
		}		
		sub_dev_list->foreachNext(sub_dev_list);
	}
    return 0;
}

static int execCmdCb(const char *devid, const char *cmd_name, const char *cmd_args)
{
	sub_dev_list->foreachStart(sub_dev_list,0);
	while(sub_dev_list->foreachEnd(sub_dev_list)) {
		DeviceStr *dev;
		sub_dev_list->foreachGetElem(sub_dev_list,&dev);
		if (strcmp(dev->id,devid) == 0) {
			dev->type_para->execCmd(dev,cmd_name,cmd_args);
			break;
		}		
		sub_dev_list->foreachNext(sub_dev_list);
	}
    return 0;
}

static int removeDeviceCb(const char *devid)
{
	sub_dev_list->foreachStart(sub_dev_list,0);
	int i = 0;
	while(sub_dev_list->foreachEnd(sub_dev_list)) {
		DeviceStr *dev;
		sub_dev_list->foreachGetElem(sub_dev_list,&dev);
		if (strcmp(dev->id,devid) == 0) {
			dev->type_para->remove(&dev);
			sub_dev_list->delete(sub_dev_list,i);
			break;
		}		
		sub_dev_list->foreachNext(sub_dev_list);
		i++;
	}
    return 0;
}

static int permitSubDeviceJoinCb(uint8_t duration)
{
    printf("permitSubDeviceJoinCb, duration:%d\n",duration);
	zigbeeNetIn(duration);
	return 0;
}
void gwDeviceInit(void)
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

}

void gwLoadDeviceData(void)
{
	int i;
	int device_num = sqlGetDeviceStart();
	char id[32];
	int type;
	uint16_t addr,channel;
	sub_dev_list = listCreate(sizeof(DeviceStr *));
	for (i=0; i<device_num; i++) {
		sqlGetDevice(id,&type,&addr,&channel);	
		gwRegisterSubDevice(id,type,addr,channel);
	}
	sqlGetDeviceEnd();
}

static DeviceStr *getSubDevice(char *id)
{
	DeviceStr *dev = NULL;
	sub_dev_list->foreachStart(sub_dev_list,0);
	while(sub_dev_list->foreachEnd(sub_dev_list)) {
		sub_dev_list->foreachGetElem(sub_dev_list,&dev);
		if (strcmp(dev->id,id) == 0) {
			break;
		}		
		sub_dev_list->foreachNext(sub_dev_list);
	}

	return dev;
}

void gwReportPowerOn(char *id,char *param)
{
	printf("[%s]id:%s\n", __FUNCTION__,id);
	DeviceStr * dev = getSubDevice(id);
	if (!dev)
		return;
	if (dev->type_para->reportPowerOn)
		dev->type_para->reportPowerOn(dev,param);
}

void gwReportPowerOff(char *id)
{
	DeviceStr * dev = getSubDevice(id);
	if (!dev)
		return;
	if (dev->type_para->reportPowerOff)
		dev->type_para->reportPowerOff(dev);
}

void gwReportAlarmStatus(char *id,char *param)
{
	printf("[%s]id:%s\n", __FUNCTION__,id);
	DeviceStr * dev = getSubDevice(id);
	if (!dev)
		return;
	if (dev->type_para->reportAlarmStatus)
		dev->type_para->reportAlarmStatus(dev,param);
}

void gwGetSwichStatus(void)
{
	if (!sub_dev_list)
		return;
	sub_dev_list->foreachStart(sub_dev_list,0);
	int i = 1;
	while(sub_dev_list->foreachEnd(sub_dev_list)) {
		DeviceStr *dev;
		sub_dev_list->foreachGetElem(sub_dev_list,&dev);
		if (dev->type_para->getSwichStatus)
			dev->type_para->getSwichStatus(dev);
		sub_dev_list->foreachNext(sub_dev_list);
		i++;
	}
}
