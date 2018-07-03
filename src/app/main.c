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
			gpio->FlashStart(gpio,ENUM_GPIO_LED_RESET,FLASH_SLOW,FLASH_FOREVER);
			aliSdkresetWifi();
			gpio->FlashStop(gpio,ENUM_GPIO_LED_RESET);
			gpio->SetValue(gpio,ENUM_GPIO_LED_RESET,IO_INACTIVE);
			aliSdkReset(0);// 清除所有设备
			sqlClearDevice();
			exit(0);
#else
			gpio->FlashStart(gpio,ENUM_GPIO_LED_WIFI,FLASH_SLOW,FLASH_FOREVER);
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

int main(int argc, char *argv[])
{
	configLoad();
	smarthomeInit();
	gpioInit();
	gpio->creatInputThread(gpio,gpioInputTread);

	aliSdkInit(argc, argv);
    gwRegisterGateway();
	aliSdkStart();

#if (defined V1)
	gpio->FlashStop(gpio,ENUM_GPIO_LED_RESET);
	gpio->SetValue(gpio,ENUM_GPIO_LED_RESET,IO_INACTIVE);
#else
	gpio->FlashStop(gpio,ENUM_GPIO_LED_WIFI);
	gpio->SetValue(gpio,ENUM_GPIO_LED_WIFI,IO_ACTIVE);
#endif

#if (defined V1)
	gwDeviceInit();
#endif
	gwLoadDeviceData();
	WatchDogOpen();

    while (1) {
		WatchDogFeed();
        sleep(2);
    }

	aliSdkEnd();
}

