/*
 * =============================================================================
 *
 *       Filename:  device_air_condition.c
 *
 *    Description:  空调设备 
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
#include <pthread.h>

#include "device_air_condition.h"
#include "sql_handle.h"
#include "timer.h"
#include "config.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/
static void setMideaData(DeviceStr *This);

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
#define MAX_VALUE_LENG 32
enum {
	ATTR_ERROR,
	ATTR_SWICH,
	ATTR_MODE,
	ATTR_SPEED,
	ATTR_CURRENT_TEMP,
	ATTR_TEMP,
	ATTR_SLAVE_ADDR,
	ATTR_ROOM_ADDR,
};

struct ProductKey {
	char *pk; // 空调PK
	int total_num;  // 对应的空调数量
	char id[32]; // 空调devicename
	int cur_num; // 当前已注册的空调数量
	uint16_t addr; // 相同设备共享地址
	uint16_t channel; // 相同设备共享通道
};
typedef struct _MideaAirConditionSlave {                
    int slave_addr ;                                    
    int room_addr ;                                     
    int temp ;                                          
    int mode ;                                          
    int speed ;                                         
}MideaAirConditionSlave;                                
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

static void cmdSwich(DeviceStr *dev,char *value)
{
	int leng = strlen(dev->id);
	// channel 从1开始计数
	int channel = atoi(&dev->id[leng-1]);
	int value_int = atoi(value);
	sprintf(dev->value[ATTR_SWICH],"%s",value);
	DPRINT("[%s]value:%s,int:%d,buf:%s,speed:%s\n",
			__FUNCTION__,
			value,
			value_int,
			dev->value[ATTR_SWICH],
			dev->value[ATTR_SPEED] );
	if (value_int) {
		smarthomeAirCondtionCmdCtrOpen(dev,
				atoi(dev->value[ATTR_TEMP]),
				atoi(dev->value[ATTR_MODE]),
				atoi(dev->value[ATTR_SPEED]),channel);
	} else
		smarthomeAirCondtionCmdCtrClose(dev,channel);
	setMideaData(dev);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief cmdWorkMode 空调工作模式
 *
 * @param dev
 * @param value APP端对应模式为 0（自动） 1（制冷） 2（制热） 3（通风） 4（除湿）
 * 协议对应为
 * 第1个Byte为空调温度，范围16-32℃
 * 第2个byte:
 * 高4位:0制冷  1制热  2自动
 *        3除湿  4送风
 *        低4位: 0自动    1风高速 
 *        2风中速  3风低速
 *
 */
/* ---------------------------------------------------------------------------*/
static void cmdWorkMode(DeviceStr *dev,char *value)
{
	int leng = strlen(dev->id);
	// channel 从1开始计数
	int channel = atoi(&dev->id[leng-1]);
	sprintf(dev->value[ATTR_MODE],"%s",value);
	smarthomeAirCondtionCmdCtrOpen(dev,
			atoi(dev->value[ATTR_TEMP]),
			atoi(dev->value[ATTR_MODE]),
			atoi(dev->value[ATTR_SPEED]),channel);
	setMideaData(dev);
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief cmdWindSpeed APP端对应模式为 0（自动） 2（低档） 3（中档） 4（高档） 
 * 协议对应为
 * 第1个Byte为空调温度，范围16-32℃
 * 第2个byte:
 * 高4位:0制冷  1制热  2自动
 *        3除湿  4送风
 *        低4位: 0自动    1风高速 
 *        2风中速  3风低速
 * @param dev
 * @param value
 */
/* ---------------------------------------------------------------------------*/
static void cmdWindSpeed(DeviceStr *dev,char *value)
{
	int leng = strlen(dev->id);
	// channel 从1开始计数
	int channel = atoi(&dev->id[leng-1]);
	sprintf(dev->value[ATTR_SPEED],"%s",value);
	smarthomeAirCondtionCmdCtrOpen(dev,
			atoi(dev->value[ATTR_TEMP]),
			atoi(dev->value[ATTR_MODE]),
			atoi(dev->value[ATTR_SPEED]),channel);
	setMideaData(dev);
}

static void cmdTemperature(DeviceStr *dev,char *value)
{
	int leng = strlen(dev->id);
	// channel 从1开始计数
	int channel = atoi(&dev->id[leng-1]);
	sprintf(dev->value[ATTR_TEMP],"%s",value);
	smarthomeAirCondtionCmdCtrOpen(dev,
			atoi(dev->value[ATTR_TEMP]),
			atoi(dev->value[ATTR_MODE]),
			atoi(dev->value[ATTR_SPEED]),channel);
	setMideaData(dev);
}

static void cmdGetSwichStatus(DeviceStr *dev)
{
	int leng = strlen(dev->id);
	// channel 从1开始计数
	int channel = atoi(&dev->id[leng-1]) + 1;
	smarthomeAllDeviceCmdGetSwichStatus(dev,channel);
}

static void cmdSetModbusSlaveAddress(DeviceStr *dev,char *value)
{
	int leng = strlen(dev->id);
	int channel = atoi(&dev->id[leng-1]);
	
	sprintf(dev->value[ATTR_SLAVE_ADDR],"%s",value);
	smarthomeAirCondtionMideaCmdSlaveAddr(dev,
			atoi(dev->value[ATTR_SLAVE_ADDR]),
			atoi(dev->value[ATTR_ROOM_ADDR]),channel);
	setMideaData(dev);
}
static void cmdSetRoomAdress(DeviceStr *dev,char *value)
{
	int leng = strlen(dev->id);
	int channel = atoi(&dev->id[leng-1]);
	
	sprintf(dev->value[ATTR_ROOM_ADDR],"%s",value);
	smarthomeAirCondtionMideaCmdSlaveAddr(dev,
			atoi(dev->value[ATTR_SLAVE_ADDR]),
			atoi(dev->value[ATTR_ROOM_ADDR]),channel);
	setMideaData(dev);
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief reportPowerOnCb 
 *
 * APP端对应风速为 0（自动） 2（低档） 3（中档） 4（高档） 
 * 		  模式为 0（自动） 1（制冷） 2（制热） 3（通风） 4（除湿）
 * 协议对应为
 * 第1个Byte为空调温度，范围16-32℃
 * 第2个byte:
 * 高4位:0制冷  1制热  2自动
 *        3除湿  4送风
 *        低4位: 0自动    1风高速 
 *        2风中速  3风低速
 * @param dev
 * @param param
 */
/* ---------------------------------------------------------------------------*/
static void reportPowerOnCb(DeviceStr *dev,char *param,int channel)
{
	char speed_change[] = {0,4,3,2}; // 速度转换
	char mode_change[] = {1,2,0,4,3}; // 模式转换
	// 固定为开
	sprintf(dev->value[ATTR_SWICH],"1");
	sprintf(dev->value[ATTR_TEMP],"%d",param[0]); 
	sprintf(dev->value[ATTR_SPEED],"%d",speed_change[param[1] & 0x0f]); 
	sprintf(dev->value[ATTR_MODE],"%d",mode_change[param[1] >> 4]); 
	const char *attr_name[] = {
		dev->type_para->attr[ATTR_SWICH].name,
		dev->type_para->attr[ATTR_SPEED].name,
		dev->type_para->attr[ATTR_MODE].name,
		dev->type_para->attr[ATTR_TEMP].name,
		NULL};
	const char *attr_value[] = {
		dev->value[ATTR_SWICH],
		dev->value[ATTR_SPEED],
		dev->value[ATTR_MODE],
		dev->value[ATTR_TEMP],
		NULL};
	int attr_value_type[] = {
		dev->type_para->attr[ATTR_SWICH].value_type,
		dev->type_para->attr[ATTR_SPEED].value_type,
		dev->type_para->attr[ATTR_MODE].value_type,
		dev->type_para->attr[ATTR_TEMP].value_type,
	};
	aliSdkSubDevReportAttrs(dev,
			attr_name,attr_value,attr_value_type);
}

static void reportPowerOffCb(DeviceStr *dev,int channel)
{
	sprintf(dev->value[ATTR_SWICH],"0");
	const char *attr_name[] = {
		dev->type_para->attr[ATTR_SWICH].name,
		NULL};
	const char *attr_value[] = {
		dev->value[ATTR_SWICH],
		NULL};
	int attr_value_type[] = {
		dev->type_para->attr[ATTR_SWICH].value_type,
	};
	aliSdkSubDevReportAttrs(dev,
			attr_name,attr_value,attr_value_type);
}

static struct ProductKey pks[] =
{
	{"a1o4mhNwSUD",4},
	{"a1VdfF3IHzT",5},
	{NULL,0},
};

static DeviceTypePara air_condition = {
	.name = "air_condition",
	.short_model = 0x00022531,
	.secret = "Xf3r8BQV1Utz5o6EnJfFXF4tE3BhfAKH3ABYaQDr",
	.product_key = "a1o4mhNwSUD",
	.device_secret = "",

	.proto_type = ALI_SDK_PROTO_TYPE_ZIGBEE,
	.device_type = DEVICE_TYPE_ZYKT_MIDEA,
	.attr = {
#if (defined V1)
		{"ErrorCode",NULL,DEVICE_VELUE_TYPE_INT},
		{"Switch",cmdSwich,DEVICE_VELUE_TYPE_INT},
		{"WorkMode",cmdWorkMode,DEVICE_VELUE_TYPE_INT},
		{"WindSpeed",cmdWindSpeed,DEVICE_VELUE_TYPE_INT},
		{"CurrentTemp",NULL,DEVICE_VELUE_TYPE_INT},
		{"Temperature",cmdTemperature,DEVICE_VELUE_TYPE_INT},
#else
		{"Error",NULL,DEVICE_VELUE_TYPE_INT},
		{"PowerSwitch",cmdSwich,DEVICE_VELUE_TYPE_INT},
		{"WorkMode",cmdWorkMode,DEVICE_VELUE_TYPE_INT},
		{"WindSpeed",cmdWindSpeed,DEVICE_VELUE_TYPE_INT},
		{"CurrentTemperature",NULL,DEVICE_VELUE_TYPE_INT},
		{"TargetTemperature",cmdTemperature,DEVICE_VELUE_TYPE_INT},
		{"ModbusSlaveAddress",cmdSetModbusSlaveAddress,DEVICE_VELUE_TYPE_INT},
		{"RoomAdress",cmdSetRoomAdress,DEVICE_VELUE_TYPE_INT},
#endif
		{NULL,NULL},
	},
	.getAttr = getAttrCb,
	.setAttr = setAttrCb,
	.getSwichStatus = cmdGetSwichStatus,
	.reportPowerOn = reportPowerOnCb,
	.reportPowerOff = reportPowerOffCb,
};

static void *sendMideaSlaveAddrThread(void *arg)
{
	DeviceStr *This = (DeviceStr *)arg;
	int leng = strlen(This->id);
	// channel 从1开始计数
	int channel = atoi(&This->id[leng-1]);
	while (1) {
		if (zigbeeIsReady() == 0) {
			usleep(10000);
			continue;
		}
		sleep(5);
		smarthomeAirCondtionMideaCmdSlaveAddr(This,
				atoi(This->value[ATTR_SLAVE_ADDR]),
				atoi(This->value[ATTR_ROOM_ADDR]),channel);
		break;
	}
	pthread_exit(NULL);
}

static void getMideaData(DeviceStr *dev)
{
	MideaAirConditionSlave dev_midea;
	sqlGetMideaAddr(dev->id,&dev_midea);
	if (dev_midea.temp < 16)
		dev_midea.temp = 16;
		
	sprintf(dev->value[ATTR_TEMP],"%d",dev_midea.temp);
	sprintf(dev->value[ATTR_SPEED],"%d",dev_midea.speed);
	sprintf(dev->value[ATTR_MODE],"%d",dev_midea.mode);
	sprintf(dev->value[ATTR_SLAVE_ADDR],"%d",dev_midea.slave_addr);
	sprintf(dev->value[ATTR_ROOM_ADDR],"%d",dev_midea.room_addr);


	pthread_t task;
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&task, &attr, sendMideaSlaveAddrThread, dev);

}
static void sendMideaData5s(void *arg)
{
	DeviceStr *dev = (DeviceStr *)arg;
	dev->timer->stop(dev->timer);
	MideaAirConditionSlave dev_midea;

	dev_midea.temp = atoi(dev->value[ATTR_TEMP]);
	dev_midea.mode = atoi(dev->value[ATTR_MODE]);
	dev_midea.speed = atoi(dev->value[ATTR_SPEED]);
	dev_midea.slave_addr = atoi(dev->value[ATTR_SLAVE_ADDR]);
	dev_midea.room_addr = atoi(dev->value[ATTR_ROOM_ADDR]);

	sqlSetMideaAddr(dev->id,&dev_midea,sizeof(dev_midea));
}

static void setMideaData(DeviceStr *dev)
{
	if (!dev->timer) {
		dev->timer = timerCreate(1000 * 5 ,sendMideaData5s,dev); // 5s定时器
		dev->timer->start(dev->timer);
	} else  {
		dev->timer->start(dev->timer);
		dev->timer->resetTick(dev->timer);
	}
}

DeviceStr* registDeviceAirCondition(char *id,
		uint16_t addr,
		uint16_t channel,
		char *pk,
		RegistSubDevType regist_type)
{
	int i;
	struct ProductKey *pk_key;
	for (i=0; pks[i].pk != NULL ; i++) {
		if (strcmp(pk,pks[i].pk) == 0) {
			break;
		}
	}
	if (pks[i].pk == NULL) {
		DPRINT("diff pk :now pk:%s\n",pk );	
		return NULL;
	}
	pk_key = &pks[i];

	// 当达到当前空调上限，则返回允许入网,添加新设备
	DPRINT("cur:%d,total:%d,id:%s\n",pk_key->cur_num , pk_key->total_num,id );
	if (regist_type == REGIST_INIT) {
		strcpy(pk_key->id,id);
		int len = strlen(pk_key->id);
		pk_key->id[len-1] = '\0';
		pk_key->channel = channel;	
		pk_key->addr = addr;	
		if (pk_key->cur_num >= pk_key->total_num)
			pk_key->cur_num = 0;	
	} else if (regist_type == REGIST_PERMIT) {
		if (pk_key->cur_num == 0) {
			pk_key->channel = channel;	
			pk_key->addr = addr;	
			return NULL;
		}
		if (pk_key->cur_num >= pk_key->total_num) {
			pk_key->cur_num = 0;	
			return NULL;
		}
	} else if (regist_type == REGIST_NORMAL) {
		pk_key->channel = channel;	
		pk_key->addr = addr;	
		strcpy(pk_key->id,id);
	}
	DeviceStr *This = (DeviceStr *)calloc(1,sizeof(DeviceStr));
	sprintf(This->id,"%s%d",pk_key->id,pk_key->cur_num++);
	memset(This->value,0,sizeof(This->value));
	This->type_para = &air_condition;
	This->type_para->product_key = pk_key->pk;
	This->addr = pk_key->addr;
	This->channel = pk_key->channel;
	DPRINT("[%s]addr:%x,channel:%d\n",__FUNCTION__,This->addr,This->channel );
	// 初始化属性
	int j;
	for (j=0; This->type_para->attr[j].name != NULL; j++) {
		This->value[j] = (char *)calloc(1,MAX_VALUE_LENG);
		sprintf(This->value[j],"%s","0");
	}	
	getMideaData(This);

	pthread_t task;
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&task, &attr, sendMideaSlaveAddrThread, This);
	return This;
}

