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
#include <string.h>
#include <stdlib.h>
#include "debug.h"
#include "linklist.h"		/* Header */
#include "device_protocol.h"
#include "sql_handle.h"
#include "zigbee.h"
#include "ali_sdk_platform.h"

#include "device_light.h"
#include "device_fresh_air.h"
#include "device_motion_curtain.h"
#include "device_air_condition.h"
#include "device_alarm_whistle.h"
#include "device_curtain.h"
#include "device_door_contact.h"
#include "device_outlet.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/
void gwGetSwichStatus(void);
void gwDeviceInit(void);

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
#define GW_SERVICE_FACTORY_RESET                "FactoryReset"

enum {
    GW_ARM_ENTRYDELAY,
    GW_ARM_MODE,
};

typedef struct {
	unsigned int cnt;
	List *list;
	DeviceStr *dev;
}SubDevice;

typedef struct {
	int device_type;	
	DeviceStr* (*regist) (char *,uint16_t,uint16_t);
}SubDeviceRegist;

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static GateWayPrivateAttr gw_attrs[];

static List *sub_dev_list = NULL; // 链表保存子设备
// static SubDevice sub_device;
static SubDeviceRegist device_regist[] = {
	{DEVICE_TYPE_DK,registDeviceLight},
	{DEVICE_TYPE_XFXT,registDeviceFreshAir},
	{DEVICE_TYPE_HW,registDeviceMotionCurtain},
	{DEVICE_TYPE_ZYKT,registDeviceAirCondition},
	{DEVICE_TYPE_JD,registDeviceAlarmWhistle},
	{DEVICE_TYPE_CL,registDeviceCurtain},
	{DEVICE_TYPE_MC,registDeviceDoorContact},
	{DEVICE_TYPE_JLCZ10,registDeviceOutlet10},
	{DEVICE_TYPE_JLCZ16,registDeviceOutlet16},
};

static int __factory_reset_service_cb(char *args, char *output_buf, unsigned int buf_sz)
{
    printf("exec gateway factory_reset service, args:%s\n", args);
    int ret = 0;
    //1.firmware reset
    //reset config, ....

    //2.alink reset
    ret = aliSdkReset(1);
    if (ret != 0) {
        printf("call function alink_factory_reset fail!\n");
    }

    return 0;
}
int gwRegisterGatewayService(void)
{
	int ret = aliSdkRegistGwService(GW_SERVICE_FACTORY_RESET,
			__factory_reset_service_cb);
	if (0 != ret) {
		printf("register service fail, service:%s\n", GW_SERVICE_FACTORY_RESET);
		return -1;
	}

    return 0;
}

static int alarmEntryDelayGetCb(char *output_buf, unsigned int buf_sz)
{
    printf("[%s]:%s\n", __FUNCTION__,gw_attrs[GW_ARM_ENTRYDELAY].value);
    snprintf(output_buf, buf_sz - 1, "%s",gw_attrs[GW_ARM_ENTRYDELAY].value);
	gwGetSwichStatus();
    return 0;
}

static int alarmEntryDelaySetCb(char *value)
{
    printf("[%s]:%s\n", __FUNCTION__,value);
    int value_int = atoi(value);
    if (value_int < 0 || value_int > 600) {
        printf("invalid entry delay attr value:%s\n", value);
        return -1;
    }
    sprintf(gw_attrs[GW_ARM_ENTRYDELAY].value,"%s",value);

    return 0;
}

static int alarmAlarmModeGetCb(char *output_buf, unsigned int buf_sz)
{
    printf("[%s]:%s\n", __FUNCTION__,gw_attrs[GW_ARM_MODE].value);
    snprintf(output_buf, buf_sz - 1, "%s",gw_attrs[GW_ARM_MODE].value);
    return 0;
}

static int alarmAlarmModeSetCb(char *value)
{
    printf("[%s]:%s\n", __FUNCTION__,value);
    int value_int = atoi(value);
    if (value_int < 0 || value_int > 3) {
        printf("invalid alarm mode attr value:%s\n", value);
        return -1;
    }
    sprintf(gw_attrs[GW_ARM_MODE].value,"%s",value);

    return 0;
}


static GateWayPrivateAttr gw_attrs[] = {
	{"ArmEntryDelay",alarmEntryDelayGetCb,alarmEntryDelaySetCb,DEVICE_VELUE_TYPE_INT},
	{"ArmMode",alarmAlarmModeGetCb,alarmAlarmModeSetCb,DEVICE_VELUE_TYPE_INT},
	{NULL},
};

int gwRegisterGatewayAttribute(void)
{
	aliSdkRegisterAttribute(gw_attrs);
#if (defined V2)
	gwDeviceInit();
#endif
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
	ret = aliSdkRegisterSubDevice(dev);
	if (ret != 0) 
		printf("[%s]register sub device fail,id:%s\n",
				dev->type_para->name, dev->id);
    return ret;
}

int gwDeviceReportRegist(void)
{
	return aliSdkRegisterGw("{\"ArmMode\": { \"value\": \"1\"  }}");
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
			// dev->type_para->execCmd(dev,cmd_name,cmd_args);
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
			aliSdkUnRegisterSubDevice(dev);
			int i;
			for (i=0; dev->type_para->attr[i].name != NULL; i++) {
				if (dev->value[i])
					free(dev->value[i]);
				dev->value[i] = NULL;
			}
			sqlDeleteDevice(dev->id);
			free(dev);
			dev = NULL;
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
static GateWayAttr gw_attr = {
	.getCb = getAttrCb,
	.setCb = setAttrCb,
	.execCmdCb = execCmdCb,
	.removeDeviceCb = removeDeviceCb,
	.permitSubDeviceJoinCb = permitSubDeviceJoinCb,
};
void gwDeviceInit(void)
{
	aliSdkRegistGwAttr("zigbee",ALI_SDK_PROTO_TYPE_ZIGBEE,&gw_attr);
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

/* ---------------------------------------------------------------------------*/
/**
 * @brief gwReportPowerOn 上报设备开启
 *
 * @param id
 * @param param
 */
/* ---------------------------------------------------------------------------*/
void gwReportPowerOn(char *id,char *param)
{
	printf("[%s]id:%s\n", __FUNCTION__,id);
	DeviceStr * dev = getSubDevice(id);
	if (!dev)
		return;
	if (dev->type_para->reportPowerOn) {
		printf("[%s]---->", dev->type_para->name);
		dev->type_para->reportPowerOn(dev,param);
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief gwReportPowerOff 上报设备关闭
 *
 * @param id
 */
/* ---------------------------------------------------------------------------*/
void gwReportPowerOff(char *id)
{
	DeviceStr * dev = getSubDevice(id);
	if (!dev)
		return;
	if (dev->type_para->reportPowerOff) {
		printf("[%s]---->", dev->type_para->name);
		dev->type_para->reportPowerOff(dev);
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief gwReportAlarmStatus 上报报警信息
 *
 * @param id
 * @param param
 */
/* ---------------------------------------------------------------------------*/
void gwReportAlarmStatus(char *id,char *param)
{
	printf("[%s]id:%s\n", __FUNCTION__,id);
	DeviceStr * dev = getSubDevice(id);
	if (!dev)
		return;
	if (dev->type_para->reportAlarmStatus) {
		printf("[%s]---->", dev->type_para->name);
		dev->type_para->reportAlarmStatus(dev,param);
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief gwGetSwichStatus 上报开关状态
 */
/* ---------------------------------------------------------------------------*/
void gwGetSwichStatus(void)
{
	if (!sub_dev_list)
		return;
	sub_dev_list->foreachStart(sub_dev_list,0);
	int i = 1;
	while(sub_dev_list->foreachEnd(sub_dev_list)) {
		DeviceStr *dev;
		sub_dev_list->foreachGetElem(sub_dev_list,&dev);
		if (dev->type_para->getSwichStatus) {
			// printf("[%s]---->", dev->type_para->name);
			dev->type_para->getSwichStatus(dev);
		}
		sub_dev_list->foreachNext(sub_dev_list);
		i++;
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief gwReportEleQuantity 上报电量
 *
 * @param id
 * @param param
 */
/* ---------------------------------------------------------------------------*/
void gwReportEleQuantity(char *id,char *param)
{
	printf("[%s]id:%s\n", __FUNCTION__,id);
	DeviceStr * dev = getSubDevice(id);
	if (!dev)
		return;
	if (dev->type_para->reportEleQuantity) {
		printf("[%s]---->", dev->type_para->name);
		dev->type_para->reportEleQuantity(dev,param);
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief gwReportElePower 上报功率
 *
 * @param id
 * @param param
 */
/* ---------------------------------------------------------------------------*/
void gwReportElePower(char *id,char *param)
{
	printf("[%s]id:%s\n", __FUNCTION__,id);
	DeviceStr * dev = getSubDevice(id);
	if (!dev)
		return;
	if (dev->type_para->reportElePower) {
		printf("[%s]---->", dev->type_para->name);
		dev->type_para->reportElePower(dev,param);
	}
}

