/*
 * =============================================================================
 *
 *       Filename:  device_lock.c
 *
 *    Description:  西勒奇门锁/安朗杰门锁
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
#include <pthread.h>

#include "sql_handle.h"
#include "device_lock.h"
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
	ATTR_LOCKSTATE,
	ATTR_ARM_DISARM,
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

static void *reportDisArmThread(void *arg)
{
	DeviceStr *dev = (DeviceStr *)arg;

	sleep(5);
	printf("1111\n");
	sprintf(dev->value[ATTR_ARM_DISARM],"0");
	const char *attr_name[] = {
		dev->type_para->attr[ATTR_ARM_DISARM].name,
		NULL};
	const char *attr_value[] = {
		dev->value[ATTR_ARM_DISARM],
		NULL};
	int attr_value_type[] = {
		dev->type_para->attr[ATTR_ARM_DISARM].value_type,
	};
	aliSdkSubDevReportAttrs(dev,
			attr_name,attr_value,attr_value_type);

	return NULL;
}

static void reportPowerOnCb(DeviceStr *dev,char *param,int channel)
{
	// 固定为开
	sprintf(dev->value[ATTR_LOCKSTATE],"1");
	const char *attr_name[] = {
		dev->type_para->attr[ATTR_LOCKSTATE].name,
		NULL};
	const char *attr_value[] = {
		dev->value[ATTR_LOCKSTATE],
		NULL};
	int attr_value_type[] = {
		dev->type_para->attr[ATTR_LOCKSTATE].value_type,
	};
	aliSdkSubDevReportAttrs(dev,
			attr_name,attr_value,attr_value_type);

	pthread_t m_pthread;					//线程号
	pthread_attr_t threadAttr1;				//线程属性
	pthread_attr_init(&threadAttr1);		//附加参数
	//设置线程为自动销毁
	pthread_attr_setdetachstate(&threadAttr1,PTHREAD_CREATE_DETACHED);
	//创建线程，无传递参数
	pthread_create(&m_pthread,&threadAttr1,reportDisArmThread,dev);
	pthread_attr_destroy(&threadAttr1);		//释放附加参数
}

static void reportPowerOffCb(DeviceStr *dev,int channel)
{
	sprintf(dev->value[ATTR_LOCKSTATE],"0");
	const char *attr_name[] = {
		dev->type_para->attr[ATTR_LOCKSTATE].name,
		NULL};
	const char *attr_value[] = {
		dev->value[ATTR_LOCKSTATE],
		NULL};
	int attr_value_type[] = {
		dev->type_para->attr[ATTR_LOCKSTATE].value_type,
	};
	aliSdkSubDevReportAttrs(dev,
			attr_name,attr_value,attr_value_type);
}

static void reportArmStatus(DeviceStr *dev,char *param)
{
	sprintf(dev->value[ATTR_ARM_DISARM],"1");
	const char *attr_name[] = {
		dev->type_para->attr[ATTR_ARM_DISARM].name,
		NULL};
	const char *attr_value[] = {
		dev->value[ATTR_ARM_DISARM],
		NULL};
	int attr_value_type[] = {
		dev->type_para->attr[ATTR_ARM_DISARM].value_type,
	};
	aliSdkSubDevReportAttrs(dev,
			attr_name,attr_value,attr_value_type);
}

static DeviceTypePara lock = {
	.name = "lock",
	.short_model = 0x00092316,
	.secret = "ZO431NU7020UT9Iu8B8yQnfQbmjagPbRZm7zfuGm",
	.product_key = "a1l4la2xZTl",
	.device_secret = "",
	.proto_type = ALI_SDK_PROTO_TYPE_ZIGBEE,
	.device_type = 	DEVICE_TYPE_LOCK_XLQ,
	.attr = {
		{"LockState",NULL,DEVICE_VELUE_TYPE_INT},
		{"ArmStatus",NULL,DEVICE_VELUE_TYPE_INT},
		{NULL,NULL},
	},
	.getAttr = getAttrCb,
	.setAttr = setAttrCb,
	.reportPowerOn = reportPowerOnCb,
	.reportPowerOff = reportPowerOffCb,
	.reportArmStatus = reportArmStatus,
};


DeviceStr * registDevicelock(char *id,uint16_t addr,uint16_t channel,char *pk)
{
	if (pk) {
		if (strcmp(pk,lock.product_key) != 0) {
			DPRINT("diff pk :allow pk:%s,now pk:%s\n",
					lock.product_key,pk );	
			return NULL;
		}
	}
	int i;
	DeviceStr *This = (DeviceStr *)calloc(1,sizeof(DeviceStr));
	strcpy(This->id,id);
	memset(This->value,0,sizeof(This->value));
	DPRINT("[%s]key:%s,sec:%s\n",__FUNCTION__,lock.product_key,
		lock.device_secret  );
	This->type_para = &lock;
	This->addr = addr;
	This->channel = channel;
	// 初始化属性
	for (i=0; This->type_para->attr[i].name != NULL; i++) {
		This->value[i] = (char *)calloc(1,MAX_VALUE_LENG);
		sprintf(This->value[i],"%s","0");
	}	

	return This;
}
