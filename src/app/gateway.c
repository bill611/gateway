/*
 * =============================================================================
 *
 *       Filename:  gateway.c
 *
 *    Description:  网关设备
 *
 *        Version:  1.0
 *        Created:  2018-05-08 16:26:03
 *
 *       Revision:  2019-01-07 
 *         Author:  xubin
 *         V2.0版本飞燕平台加入PK校验，将PK传入注册函数
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
#include <pthread.h>
#include "debug.h"
#include "queue.h"
#include "device_protocol.h"
#include "sql_handle.h"
#include "zigbee.h"
#include "ali_sdk_platform.h"

#include "device_light.h"
#include "device_fresh_air.h"
#include "device_motion_curtain.h"
#include "device_air_condition.h"
#include "device_air_condition_midea.h"
#include "device_alarm_whistle.h"
#include "device_curtain.h"
#include "device_door_contact.h"
#include "device_outlet.h"
#include "device_air_box.h"
#include "device_lock.h"
#include "device_scene.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/
void gwGetSwichStatus(void);
void gwDeviceInit(void);
static void createRegistSubThread(void);

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
	DeviceStr* (*regist) (char *id,
			uint16_t addr,
			uint16_t channel,
			char *pk,
			RegistSubDevType regist_type);
}SubDeviceRegist;

struct RegistData{
	char pk[64];
	uint8_t duration;
};
/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static GateWayPrivateAttr gw_attrs[];
static Queue *regist_queue;

static List *sub_dev_list = NULL; // 链表保存子设备
static char regist_tmp_pk[64] = {0}; // 注册时临时保存允许入网的设备pk
// static SubDevice sub_device;
static SubDeviceRegist device_regist[] = {
	{DEVICE_TYPE_DK,		registDeviceLight},
	{DEVICE_TYPE_XFXT,		registDeviceFreshAir},
	{DEVICE_TYPE_HW,		registDeviceMotionCurtain},
	{DEVICE_TYPE_ZYKT_MIDEA,registDeviceAirCondition},
	{DEVICE_TYPE_JD,		registDeviceAlarmWhistle},
	{DEVICE_TYPE_CL,		registDeviceCurtain},
	{DEVICE_TYPE_MC,		registDeviceDoorContact},
	{DEVICE_TYPE_JLCZ10,	registDeviceOutlet10},
	{DEVICE_TYPE_JLCZ16,	registDeviceOutlet16},
	{DEVICE_TYPE_KQJCY,		registDeviceAirBox},
	// {DEVICE_TYPE_ZYKT_MIDEA,registDeviceAirConditionMidea},
	{DEVICE_TYPE_LOCK_XLQ,	registDevicelock},
	{DEVICE_TYPE_QJ,		registDeviceScene},
};

static int __factory_reset_service_cb(char *args, char *output_buf, unsigned int buf_sz)
{
    DPRINT("exec gateway factory_reset service, args:%s\n", args);
    int ret = 0;
    //1.firmware reset
    //reset config, ....

    //2.alink reset
    ret = aliSdkReset(1);
    if (ret != 0) {
        DPRINT("call function alink_factory_reset fail!\n");
    }

    return 0;
}
int gwRegisterGateway(void)
{
	createRegistSubThread();
	int ret = aliSdkRegistGwService(GW_SERVICE_FACTORY_RESET,
			__factory_reset_service_cb);
	if (0 != ret) {
		DPRINT("register service fail, service:%s\n", GW_SERVICE_FACTORY_RESET);
		return -1;
	}
	aliSdkRegisterAttribute(gw_attrs);
#if (defined V2)
	gwDeviceInit();
#endif

    return 0;
}

static int alarmEntryDelayGetCb(char *output_buf, unsigned int buf_sz)
{
    DPRINT("[%s]:%s\n", __FUNCTION__,gw_attrs[GW_ARM_ENTRYDELAY].value);
    snprintf(output_buf, buf_sz - 1, "%s",gw_attrs[GW_ARM_ENTRYDELAY].value);
	gwGetSwichStatus();
    return 0;
}

static int alarmEntryDelaySetCb(char *value)
{
    DPRINT("[%s]:%s\n", __FUNCTION__,value);
    int value_int = atoi(value);
    if (value_int < 0 || value_int > 600) {
        DPRINT("invalid entry delay attr value:%s\n", value);
        return -1;
    }
    sprintf(gw_attrs[GW_ARM_ENTRYDELAY].value,"%s",value);

    return 0;
}

static int netInSwitchGetCb(char *output_buf, unsigned int buf_sz)
{
    DPRINT("[%s]:%s\n", __FUNCTION__,gw_attrs[GW_ARM_MODE].value);
    snprintf(output_buf, buf_sz - 1, "%s",gw_attrs[GW_ARM_MODE].value);
    return 0;
}

static int netInSwitchSetCb(char *value)
{
    DPRINT("[%s]:%s\n", __FUNCTION__,value);
    int value_int = atoi(value);
	if (value_int)
		zigbeeNetIn(60);
	else
		zigbeeNetIn(0);
    return 0;
}


static GateWayPrivateAttr gw_attrs[] = {
	{"ArmEntryDelay",alarmEntryDelayGetCb,alarmEntryDelaySetCb,DEVICE_VELUE_TYPE_INT},
#if (defined V1)
	{"ArmMode",netInSwitchGetCb,netInSwitchSetCb,DEVICE_VELUE_TYPE_INT},
#else
	{"NetInSwich",netInSwitchGetCb,netInSwitchSetCb,DEVICE_VELUE_TYPE_INT},
#endif
	{NULL},
};

/* ---------------------------------------------------------------------------*/
/**
 * @brief gwRegisterSubDevice  注册子设备
 *
 * @param id 子设备IEEE长地址转为字符串作为唯一ID
 * @param type 子设备类型
 * @param addr 子设备短地址
 * @param channel 子设备通道数量
 * @param product_key V2.0平台产品唯一标识
 * @param regist_type 注册子设备类型
 *
 * @returns 0成功 -1失败
 */
/* ---------------------------------------------------------------------------*/
int gwRegisterSubDevice(char *id,
		int dev_type,
		uint16_t addr,
		uint16_t channel,
		char *product_key,
		RegistSubDevType regist_type)
{
    int ret = -1;
	unsigned int i;
	for (i=0; i<NELEMENTS(device_regist); i++) {
		// DPRINT("[%s] device type:%d,type:%d\n",__func__,device_regist[i].device_type,type);
		if (device_regist[i].device_type == dev_type)	
			break;
	}
	if (i == NELEMENTS(device_regist)) {
		DPRINT("unknow device type:%d\n",dev_type );
		return -1;
	}
	DPRINT("id:%s,type:%d,channle:%d\n", id,dev_type,channel);
	// return -1;
	DeviceStr* dev = device_regist[i].regist(id,addr,channel,product_key,regist_type);
	if (dev == NULL)
		return -1;
	ret = aliSdkRegisterSubDevice(dev);
	if (ret != 0)  {
		DPRINT("[%s]register sub device fail,id:%s\n",
				dev->type_para->name, dev->id);
		if (dev)
			free(dev);
		return 1;	
	} else {
		DPRINT("[%s]register sub device ok,id:%s\n",
				dev->type_para->name, dev->id);
		sub_dev_list->append(sub_dev_list,&dev);
	}
	if (regist_type > REGIST_INIT)
		smarthomeAddNewDev(dev->id,
				dev->type_para->device_type,
				dev->addr,dev->channel,dev->type_para->product_key);
    return ret;
}

int gwDeviceReportRegist(void)
{
	return aliSdkRegisterGw("{\"ArmMode\": { \"value\": \"1\"  }}");
}

static int getAttrCb(const char *devid, const char *attr_set[])
{
	if (!sub_dev_list)
		return 0;
	sub_dev_list->foreachStart(sub_dev_list,0);
	while(sub_dev_list->foreachEnd(sub_dev_list)) {
		DeviceStr *dev = NULL;
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
	DPRINT("gate way id:%s,%s:%s\n", devid,attr_name,attr_value);
	if (!sub_dev_list)
		return 0;
	sub_dev_list->foreachStart(sub_dev_list,0);
	while(sub_dev_list->foreachEnd(sub_dev_list)) {
		DeviceStr *dev;
		sub_dev_list->foreachGetElem(sub_dev_list,&dev);
		DPRINT("dev:%s\n", dev->id);
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
	if (!sub_dev_list)
		return 0;
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
	if (!sub_dev_list)
		return 0;
	sub_dev_list->foreachStart(sub_dev_list,0);
	int i = 0;
	while(sub_dev_list->foreachEnd(sub_dev_list)) {
		DeviceStr *dev;
		sub_dev_list->foreachGetElem(sub_dev_list,&dev);
		if (strcmp(dev->id,devid) == 0) {
			aliSdkUnRegisterSubDevice(dev);
			int j;
			for (j=0; dev->type_para->attr[j].name != NULL; j++) {
				if (dev->value[j])
					free(dev->value[j]);
				dev->value[j] = NULL;
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

static int permitSubDeviceJoinCb(char *pk,uint8_t duration)
{
	struct RegistData regist_data;
	memset(regist_tmp_pk,0,sizeof(regist_tmp_pk));
	memset(&regist_data,0,sizeof(regist_data));
	if (pk) {
		sprintf(regist_tmp_pk,"%s",pk);
		strcpy(regist_data.pk,regist_tmp_pk);
	}
	regist_data.duration = duration;
	regist_queue->post(regist_queue,&regist_data);

	return 0;
}
static GateWayAttr gw_attr = {
	.getCb = getAttrCb,
	.setCb = setAttrCb,
	.execCmdCb = execCmdCb,
	.removeDeviceCb = removeDeviceCb,
	.permitSubDeviceJoinCb = permitSubDeviceJoinCb,
};
static DeviceStr *getSubDevice(char *id)
{
	if (!id)
		return NULL;
	if (!sub_dev_list)
		return NULL;
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

void gwDeviceInit(void)
{
	aliSdkRegistGwAttr("zigbee",ALI_SDK_PROTO_TYPE_ZIGBEE,&gw_attr);
}

void gwLoadDeviceData(void)
{
	int i;
	int device_num = sqlGetDeviceCnt();
	char id[32] = {0};
	int type;
	uint16_t addr,channel;
	char product_key[32];
	sub_dev_list = listCreate(sizeof(DeviceStr *));
	for (i=0; i<device_num; i++) {
		sqlGetDevice(id,&type,&addr,&channel,product_key,i);	
		gwRegisterSubDevice(id,type,addr,channel,product_key,REGIST_INIT);
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief gwReportPowerOn 上报设备开启
 *
 * @param id
 * @param param
 */
/* ---------------------------------------------------------------------------*/
void gwReportPowerOn(char *id,char *param,int channel)
{
	DPRINT("[%s]id:%s\n", __FUNCTION__,id);
	DeviceStr * dev = getSubDevice(id);
	if (!dev)
		return;
	if (dev->type_para->reportPowerOn) {
		DPRINT("[%s]---->", dev->type_para->name);
		dev->type_para->reportPowerOn(dev,param,channel);
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief gwReportPowerOff 上报设备关闭
 *
 * @param id
 */
/* ---------------------------------------------------------------------------*/
void gwReportPowerOff(char *id,int channel)
{
	DeviceStr * dev = getSubDevice(id);
	if (!dev)
		return;
	if (dev->type_para->reportPowerOff) {
		DPRINT("[%s]---->", dev->type_para->name);
		dev->type_para->reportPowerOff(dev,channel);
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
	DPRINT("[%s]id:%s\n", __FUNCTION__,id);
	DeviceStr * dev = getSubDevice(id);
	if (!dev)
		return;
	if (dev->type_para->reportAlarmStatus) {
		DPRINT("[%s]---->", dev->type_para->name);
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
			// DPRINT("[%s]---->", dev->type_para->name);
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
	DPRINT("[%s]id:%s\n", __FUNCTION__,id);
	DeviceStr * dev = getSubDevice(id);
	if (!dev)
		return;
	if (dev->type_para->reportEleQuantity) {
		DPRINT("[%s]---->", dev->type_para->name);
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
	DPRINT("[%s]id:%s\n", __FUNCTION__,id);
	DeviceStr * dev = getSubDevice(id);
	if (!dev)
		return;
	if (dev->type_para->reportElePower) {
		DPRINT("[%s]---->", dev->type_para->name);
		dev->type_para->reportElePower(dev,param);
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief gwReportAirPara 上报空气参数
 *
 * @param id
 * @param param
 */
/* ---------------------------------------------------------------------------*/
void gwReportAirPara(char *id,char *param)
{
	DPRINT("[%s]id:%s\n", __FUNCTION__,id);
	if (!sub_dev_list)
		return;
	sub_dev_list->foreachStart(sub_dev_list,0);
	int i = 1;
	while(sub_dev_list->foreachEnd(sub_dev_list)) {
		DeviceStr *dev;
		sub_dev_list->foreachGetElem(sub_dev_list,&dev);
		if (dev->type_para->reportAirPara) {
			// DPRINT("[%s]---->", dev->type_para->name);
			dev->type_para->reportAirPara(dev,param);
		}
		sub_dev_list->foreachNext(sub_dev_list);
		i++;
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief gwGetAirPara 查询空气参数,有空气盒子等设备时使用j
 *
 * @param id
 * @param param
 */
/* ---------------------------------------------------------------------------*/
void gwGetAirPara(char *id,char *param)
{
	// DPRINT("[%s]id:%s\n", __FUNCTION__,id);
	DeviceStr * dev = getSubDevice(id);
	if (!dev) {
		return;
	}
	if (dev->type_para->getAirPara) {
		DPRINT("[%s]---->", dev->type_para->name);
		dev->type_para->getAirPara(dev);
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief gwReportArmStatus 上报报警
 *
 * @param id
 * @param param
 */
/* ---------------------------------------------------------------------------*/
void gwReportArmStatus(char *id,char *param)
{
	DPRINT("[%s]id:%s\n", __FUNCTION__,id);
	DeviceStr * dev = getSubDevice(id);
	if (!dev)
		return;
	if (dev->type_para->reportArmStatus) {
		DPRINT("[%s]---->", dev->type_para->name);
		dev->type_para->reportArmStatus(dev,param);
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief gwReportAlarmWhistleStatus 上报声光报警器报警
 *
 * @param id
 * @param param
 */
/* ---------------------------------------------------------------------------*/
void gwReportAlarmWhistleStatus(char *param)
{
	if (!sub_dev_list)
		return;
	sub_dev_list->foreachStart(sub_dev_list,0);
	int i = 1;
	while(sub_dev_list->foreachEnd(sub_dev_list)) {
		DeviceStr *dev;
		sub_dev_list->foreachGetElem(sub_dev_list,&dev);
		if (dev->type_para->reportAlarmWhistleOpen) {
			// DPRINT("[%s]---->", dev->type_para->name);
			dev->type_para->reportAlarmWhistleOpen(dev,param);
		}
		sub_dev_list->foreachNext(sub_dev_list);
		i++;
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief gwGetTempProductKey 返回注册时的product key
 *
 * @returns 
 */
/* ---------------------------------------------------------------------------*/
char *gwGetTempProductKey(void)
{
	return regist_tmp_pk;	
}

void gwReportSceneControl(char *id,int channel)
{
	DPRINT("[%s]id:%s\n", __FUNCTION__,id);
	DeviceStr * dev = getSubDevice(id);
	if (!dev)
		return;
	if (dev->type_para->reportSceneControl) {
		DPRINT("[%s]---->", dev->type_para->name);
		dev->type_para->reportSceneControl(dev,channel);
	}
}

static void* regsitThread(void *arg)
{
	struct RegistData regist_data;
	while(1) {
		memset(&regist_data,0,sizeof(regist_data));
		regist_queue->get(regist_queue,&regist_data);
		// 注册中央空调较特殊，选择空调后，再次注册空调，则不需要进入入网
		// 直接返回注册成功
		DeviceStr* dev = registDeviceAirCondition(NULL,0,0,regist_data.pk,REGIST_PERMIT);
		if (dev == NULL)
			zigbeeNetIn(regist_data.duration);
		else {
			int ret = aliSdkRegisterSubDevice(dev);
			if (ret != 0)  {
				DPRINT("[%s]register sub device fail1,id:%s\n",
						dev->type_para->name, dev->id);
				if (dev)
					free(dev);
			} else {
				DPRINT("[%s]register sub device ok1,id:%s\n",
						dev->type_para->name, dev->id);
				sub_dev_list->append(sub_dev_list,&dev);
			}
			smarthomeAddNewDev(dev->id,
					dev->type_para->device_type,
					dev->addr,dev->channel,dev->type_para->product_key);
		}
	}	
}

static void createRegistSubThread(void)
{
	regist_queue = queueCreate("regist",QUEUE_BLOCK,sizeof(struct RegistData));
    pthread_t m_pthread;		//线程号
	pthread_attr_t threadAttr1;			//线程属性
	pthread_attr_init(&threadAttr1);		//附加参数
	pthread_attr_setdetachstate(&threadAttr1,PTHREAD_CREATE_DETACHED);	//设置线程为自动销毁
	pthread_create(&m_pthread,&threadAttr1,regsitThread,NULL);	//创建线程
	pthread_attr_destroy(&threadAttr1);		//释放附加参数
}
