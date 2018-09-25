/*
 * =============================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  阿里网关
 *
 *        Version:  1.0
 *        Created:  2018-06-14 14:47:01
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
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "debug.h"
#include "MyGpioCtr.h"
#include "externfunc.h"
#include "sql_handle.h"
#include "ali_sdk_platform.h"
#include "device_protocol.h"
#include "gateway.h"
#include "smart_home_pro.h"
#include "config.h"


/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/
static void gpioResetHandle(void *arg,int port);
static void gpioRegistHandle(void *arg,int port);

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
typedef struct _GpioInputHandle {
	GPIO_TBL port;
	void (*func)(void *,int);
}GpioInputHandle;
/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static GpioInputHandle gpio_input_handle[] =
{
    {ENUM_GPIO_RESET,	gpioResetHandle},
    {ENUM_GPIO_MODE,	gpioRegistHandle},
};

/* ---------------------------------------------------------------------------*/
/**
 * @brief gpioInputTread IO输入处理线程
 *
 * @param arg
 *
 * @returns 
 */
/* ---------------------------------------------------------------------------*/
static void * gpioInputTread(void *arg)
{
	unsigned int i;
	while (1) {
		for (i=0; i<NELEMENTS(gpio_input_handle); i++) {
			gpio_input_handle[i].func(arg,gpio_input_handle[i].port);
		}
		usleep(100000);
	}
	return NULL;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief gpioResetHandle 复位按键，用于复位网关，重新配网，清除设备
 *
 * @param arg
 * @param port 当前设备端口
 */
/* ---------------------------------------------------------------------------*/
static void gpioResetHandle(void *arg,int port)
{
	MyGpio *This = arg;
	static int cnt = 0;
	int activ_time = This->getActiveTime(This,port);
	if (This->inputHandle(This,port)) {
		if (cnt == activ_time) {
#if (defined V1)
			gpio->FlashStart(gpio,ENUM_GPIO_LED_ONLINE,FLASH_SLOW,FLASH_FOREVER);
			aliSdkresetWifi();
			gpio->FlashStop(gpio,ENUM_GPIO_LED_ONLINE);
			gpio->SetValue(gpio,ENUM_GPIO_LED_ONLINE,IO_INACTIVE);
			aliSdkReset(0);// 清除所有设备
			sqlClearDevice();
			exit(0);
#else
			gpio->FlashStart(gpio,ENUM_GPIO_LED_ONLINE,FLASH_SLOW,FLASH_FOREVER);
			aliSdkresetWifi();
			aliSdkReset(0);// 清除所有设备
			sqlClearDevice();
#endif
		} 
		cnt++;
	} else {
		cnt = 0;
	}
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief gpioRegistHandle 注册按键
 *
 * @param arg
 * @param port 当前设备端口
 */
/* ---------------------------------------------------------------------------*/
static void gpioRegistHandle(void *arg,int port)
{
	MyGpio *This = arg;
	static int cnt = 0;
	int activ_time = This->getActiveTime(This,port);
	if (This->inputHandle(This,port)) {
		if (cnt == activ_time) {
			gwDeviceReportRegist();
			DPRINT("[%s]:%d\n", __FUNCTION__,activ_time);
		} 
		cnt++;
	} else {
		cnt = 0;
	}
}

static void * timer1sThread(void *arg)
{
	int cnt = 600;
	while(1) {
		if (net_detect() < 0) {
			gpio->SetValue(gpio,ENUM_GPIO_LED_WIFI,IO_INACTIVE);
		} else {
			gpio->SetValue(gpio,ENUM_GPIO_LED_WIFI,IO_ACTIVE);
		}
		if (cnt) {
			// printfWifiInfo(cnt);
			cnt--;
		}
		sleep(1);
	}	
	return NULL;
}
static void createTimer1sThread(void)
{
    pthread_t m_pthread;
    pthread_attr_t threadAttr1;

    pthread_attr_init(&threadAttr1);
    pthread_attr_setdetachstate(&threadAttr1,PTHREAD_CREATE_DETACHED);
    pthread_create(&m_pthread,&threadAttr1,timer1sThread,NULL);
    pthread_attr_destroy(&threadAttr1);
}
int main(int argc, char *argv[])
{
	saveLog("start %s------------------------>\n",GW_VERSION);
	configLoad();
	smarthomeInit();
	gpioInit();
	gpio->creatInputThread(gpio,gpioInputTread);
	createTimer1sThread();
	aliSdkInit(argc, argv);
    gwRegisterGateway();
	aliSdkStart();

#if (defined V1)
	gwDeviceInit();
#endif
	while (aliSdkGetOnlineStatus() == 0) {
		sleep(1);
	}
	gwLoadDeviceData();
	WatchDogOpen();

    while (1) {
		WatchDogFeed();
        sleep(2);
    }

	aliSdkEnd();
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief gpioEnableWifiLed 设置wifi灯亮与灭
 *
 * @param type 0灭 1亮
 */
/* ---------------------------------------------------------------------------*/
void gpioEnableWifiLed(int type)
{
	gpio->FlashStop(gpio,ENUM_GPIO_LED_ONLINE);
	if (type)
		gpio->SetValue(gpio,ENUM_GPIO_LED_ONLINE,IO_ACTIVE);
	else
		gpio->SetValue(gpio,ENUM_GPIO_LED_ONLINE,IO_INACTIVE);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief gpioEnableNetInLed 入网指示灯
 *
 * @param time 0 灭， 非0 闪烁
 */
/* ---------------------------------------------------------------------------*/
void gpioEnableNetInLed(int time)
{
	gpio->FlashStop(gpio,ENUM_GPIO_LED_NET_IN);
	if (time)
		gpio->FlashStart(gpio,ENUM_GPIO_LED_NET_IN,FLASH_SLOW,FLASH_FOREVER);
	else
		gpio->SetValue(gpio,ENUM_GPIO_LED_NET_IN,IO_ACTIVE);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief gpioDisableWifiPower 因网络不通时重启前先关掉wifi电源
 */
/* ---------------------------------------------------------------------------*/
void gpioDisableWifiPower(void)
{
	saveLog("reset wifi power\n");
	gpio->SetValue(gpio,ENUM_GPIO_WIFI_POWER,IO_INACTIVE);
}
