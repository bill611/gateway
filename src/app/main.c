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
#if (defined V23)
#include "tc_interface.h"
#endif


/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/
static void *gpioResetThread(void *arg);
static void *gpioRegistThread(void *arg);
static void gpioNetStateLed(int type);
static void gpioResetStateLed(void);
static void loadInterface(void);

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
typedef struct _GpioInputThread {
	struct GpioArgs args;
	void *(*func)(void *);
}GpioInputThread;
/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static int net_connect = 0;
static GpioInputThread gpio_input_handle[] =
{
    {{NULL,ENUM_GPIO_RESET},	gpioResetThread},
    {{NULL,ENUM_GPIO_MODE},		gpioRegistThread},
};

static void gpioInputRegist(void)
{
	unsigned int i;
	for (i=0; i<NELEMENTS(gpio_input_handle); i++) {
		gpio_input_handle[i].args.gpio = gpio;
		gpio->addInputThread(gpio,&gpio_input_handle[i].args,gpio_input_handle[i].func);
	}
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief gpioResetThread 复位按键，用于复位网关，重新配网，清除设备
 *
 * @param arg
 * @param port 当前设备端口
 */
/* ---------------------------------------------------------------------------*/
static void *gpioResetThread(void *arg)
{
	struct GpioArgs *This = arg;
	static int status = 0,status_old = 0;
	while (1) {
		status = This->gpio->inputHandle(This->gpio,This->port);
		if (status && status_old == 0) {
#if (defined V1)
			gpio->FlashStart(gpio,ENUM_GPIO_LED_ONLINE,FLASH_SLOW,FLASH_FOREVER);
			aliSdkresetWifi();
			gpio->FlashStop(gpio,ENUM_GPIO_LED_ONLINE);
			gpio->SetValue(gpio,ENUM_GPIO_LED_ONLINE,IO_INACTIVE);
			aliSdkReset(0);// 清除所有设备
			sqlClearDevice();
			exit(0);
#else
			gpioResetStateLed();
			if (net_connect) {
				aliSdkReset(0);// 清除所有设备
				sqlClearDevice();
			}
			aliSdkresetWifi();
#endif
		}
		status_old = status;
		usleep(10000);
	}
	return NULL;
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief gpioRegistThread 注册按键
 *
 * @param arg
 * @param port 当前设备端口
 */
/* ---------------------------------------------------------------------------*/
static void *gpioRegistThread(void *arg)
{
	struct GpioArgs *This = arg;
	static int status = 0,status_old = 0;
	while (1) {
		status = This->gpio->inputHandle(This->gpio,This->port);
		if (status && status_old == 0) {
			gwDeviceReportRegist();
		}
		status_old = status;
		usleep(10000);
	}
	return NULL;
}

static void * timer1sThread(void *arg)
{
	int cnt = 600;
	while(1) {
		if (net_detect() < 0) {
			net_connect = 0;
			gpioNetStateLed(0);
		} else {
			net_connect = 1;
			gpioNetStateLed(1);
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
	loadInterface();
	smarthomeInit();
	gpioInit();
	gpioInputRegist();
	createTimer1sThread();
	// 等待网络连接上才进行阿里设备入网
	while (net_connect == 0) {
		sleep(1);
	}
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
	halWatchDogOpen();

    while (1) {
		halWatchDogFeed();
        sleep(2);
    }

	aliSdkEnd();
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief gpioOnlineStateLed  指示是否登上阿里平台
 *
 * @param type 已登录1 未登陆
 */
/* ---------------------------------------------------------------------------*/
void gpioOnlineStateLed(int type)
{
	gpio->FlashStop(gpio,ENUM_GPIO_LED_ONLINE);
	if (type)
		gpio->SetValue(gpio,ENUM_GPIO_LED_ONLINE,IO_ACTIVE);
	else
		gpio->SetValue(gpio,ENUM_GPIO_LED_ONLINE,IO_INACTIVE);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief gpioNetStateLed 指示网络连接状态,是否连上网络
 *
 * @param type
 */
/* ---------------------------------------------------------------------------*/
static void gpioNetStateLed(int type)
{
	if (type)
		gpio->SetValue(gpio,ENUM_GPIO_LED_WIFI,IO_ACTIVE);
	else
		gpio->SetValue(gpio,ENUM_GPIO_LED_WIFI,IO_INACTIVE);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief gpioZigbeeInLed 入网指示灯
 *
 * @param time 0 灭， 非0 闪烁
 */
/* ---------------------------------------------------------------------------*/
void gpioZigbeeInLed(int time)
{
	gpio->FlashStop(gpio,ENUM_GPIO_LED_ONLINE);
	if (time)
		gpio->FlashStart(gpio,ENUM_GPIO_LED_ONLINE,FLASH_SLOW,FLASH_FOREVER);
	else
		gpio->SetValue(gpio,ENUM_GPIO_LED_ONLINE,IO_ACTIVE);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief gpioResetStateLed 复位时灯状态，wifi与在线灯同时慢闪
 */
/* ---------------------------------------------------------------------------*/
static void gpioResetStateLed(void)
{
	gpio->FlashStart(gpio,ENUM_GPIO_LED_ONLINE,FLASH_SLOW,FLASH_FOREVER);
	gpio->FlashStart(gpio,ENUM_GPIO_LED_WIFI,FLASH_SLOW,FLASH_FOREVER);
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

static char* getUpdateFilePath(void)
{
	return UPDATE_FILE;
}
static char* getKvFilePath(void)
{
	return KVFILE_DEFAULT_PATH;
}
static char* getVersion(void)
{
	return GW_VERSION;
}
static char* getDeviceName(void)
{
	return theConfig.gate_way.device_name;
}
static char* getProductKey(void)
{
	return theConfig.gate_way.product_key;
}
static char* getDeviceSecret(void)
{
	return theConfig.gate_way.device_secret;
}
static char* getProductSecret(void)
{
	// return "iCgDBldcZHNJ8iuM";
}
static void watchdogOpen(void)
{
	halWatchDogOpen();
}
static void watchdogClose(void)
{
	halWatchDogClose();
}
static void watchdogFeed(void)
{
	halWatchDogFeed();
}

static void loadInterface(void)
{
#if (defined V23)
#if (!defined X86)
	tc_interface = tcInterfaceCreate();
	tc_interface->getVersion = getVersion;
	tc_interface->getUpdateFilePath = getUpdateFilePath;
	tc_interface->getKvFilePath = getKvFilePath;
	tc_interface->getDeviceName = getDeviceName;
	tc_interface->getProductKey = getProductKey;
	tc_interface->getProductSecret = getProductSecret;
	tc_interface->getDeviceSecret = getDeviceSecret;
	tc_interface->watchdogOpen = watchdogOpen;
	tc_interface->watchdogClose = watchdogClose;
	tc_interface->watchdogFeed = watchdogFeed;
#endif
#endif
}
