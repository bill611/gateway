/*
 * =====================================================================================
 *
 *       Filename:  MyGpioCtr.c
 *
 *    Description:  GPIO控制
 *
 *        Version:  1.0
 *        Created:  2015-12-24 09:06:46
 *       Revision:  1.0
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =====================================================================================
 */
/* ----------------------------------------------------------------*
 *                      include head files
 *-----------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <unistd.h>

#include "debug.h"
#include "MyGpioCtr.h"


/* ----------------------------------------------------------------*
 *                  extern variables declare
 *-----------------------------------------------------------------*/

/* ----------------------------------------------------------------*
 *                  internal functions declare
 *-----------------------------------------------------------------*/

/* ----------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
static pthread_mutex_t gpioLock;
#define GPIO_MUTEX_LOCK()   pthread_mutex_lock(&gpioLock)
#define GPIO_MUTEX_UNLOCK() pthread_mutex_unlock(&gpioLock)
#define GPIO_GROUP_A 'a'
#define GPIO_GROUP_B 'b'
#define GPIO_GROUP_C 'c'
#define GPIO_GROUP_D 'd'
#define GPIO_GROUP_E 'e'
#define GPIO_GROUP_G 'g'
#define GPIO_GROUP_H 'h'


#define GPIO_ZIGBEE_POWER		GPIO_GROUP_C,15
#define GPIO_WIFI_POWER			GPIO_GROUP_E,0
#define GPIO_LED_POWER			GPIO_GROUP_D,10
#define GPIO_LED_RESET			GPIO_GROUP_D,13
#define GPIO_LED_WIFI			GPIO_GROUP_C,14
#define GPIO_LED_RESERVED		GPIO_GROUP_D,11

#define GPIO_RESET				GPIO_GROUP_E,1
#define GPIO_MODE				GPIO_GROUP_D,3

/* ----------------------------------------------------------------*
 *                      variables define
 *-----------------------------------------------------------------*/
MyGpio *gpio = NULL;

MyGpioPriv gpiotbl[]={
	{GPIO_ZIGBEE_POWER,		"1",IO_ACTIVE,0},
	{GPIO_WIFI_POWER,		"1",IO_ACTIVE,0},
	{GPIO_LED_POWER,		"0",IO_ACTIVE,0},
	{GPIO_LED_RESET,		"0",IO_INACTIVE,0},
	{GPIO_LED_WIFI,			"0",IO_INACTIVE,0},
	{GPIO_LED_RESERVED,		"0",IO_INACTIVE,0},

	{GPIO_RESET,			"0",IO_INPUT,10},
	{GPIO_MODE,				"0",IO_INPUT,1},
};

/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioSetValue GPIO口输出赋值，不立即执行
 *
 * @param This
 * @param port IO口
 * @param Value 有效IO_ACTIVE or 无效IO_INACTIVE
 */
/* ---------------------------------------------------------------------------*/
static void myGpioSetValue(MyGpio *This,int port,int  Value)
{
	MyGpioPriv *Priv;
	Priv = This->Priv+port;
	if (	(Priv->default_value == IO_INPUT)
		||  (Priv->default_value == IO_NO_EXIST) ) {   //输出
		DPRINT("[%d]set value fail,it is input or not exist!\n",port);
		return;
	}

	if (Value == IO_ACTIVE) {
		Priv->current_value = *(Priv->active) - '0';
	} else {
		Priv->current_value = !(*(Priv->active) - '0');
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioSetValueNow GPIO口输出赋值，并立即执行
 *
 * @param This
 * @param port IO口
 * @param Value 有效IO_ACTIVE or 无效IO_INACTIVE
 */
/* ---------------------------------------------------------------------------*/
static void myGpioSetValueNow(MyGpio *This,int port,int  Value)
{
	GPIO_MUTEX_LOCK();
	FILE *fp;
	char string_buf[50],buffer[10];
	MyGpioPriv *Priv;
	Priv = This->Priv+port;
	if (	(Priv->default_value == IO_INPUT)
		||  (Priv->default_value == IO_NO_EXIST) ) {   //输出
		DPRINT("[%d]set value fail,it is input!\n",port);
		GPIO_MUTEX_UNLOCK();
		return;
	}

	if (Value == IO_ACTIVE) {
		Priv->current_value = *(Priv->active) - '0';
	} else {
		Priv->current_value = !(*(Priv->active) - '0');
	}

	// DPRINT("port:%d,value:%d\n",Priv->portnum,Priv->current_value);
	sprintf(string_buf,"/sys/class/gpio/gpio%d/value",Priv->portnum);
	if ((fp = fopen(string_buf, "rb+")) == NULL) {
		DPRINT("SetValueNow[%d][%c%d]Cannot open value file.\n",
				port,Priv->portid,Priv->portmask);
		// exit(1);
	} else {
		sprintf(buffer,"%d",Priv->current_value);
		fwrite(buffer, sizeof(char), sizeof(buffer) - 1, fp);
		fclose(fp);
	}
	GPIO_MUTEX_UNLOCK();
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioRead 读输入口的值
 *
 * @param This
 * @param port IO口
 *
 * @returns 真正的值 0或1
 */
/* ---------------------------------------------------------------------------*/
static int myGpioRead(MyGpio *This,int port)
{
	FILE *fp;
	char string_buf[50];
	char buffer[10];
	MyGpioPriv *Priv;
	Priv = This->Priv+port;
	if (Priv->default_value != IO_INPUT) {
		goto return_value;
	}
	sprintf(string_buf,"/sys/class/gpio/gpio%d/value",Priv->portnum);
	if ((fp = fopen(string_buf, "rb")) == NULL) {
		DPRINT("Read[%d][%c%d]Cannot open value file.\n",
				port,Priv->portid,Priv->portmask);
		// exit(1);
	} else {
		fread(buffer, sizeof(char), sizeof(buffer) - 1, fp);
		Priv->current_value = atoi(buffer);
		fclose(fp);
	}
return_value:
	return Priv->current_value;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioIsActive 读取输入IO值，并判断是否IO有效
 *
 * @param This
 * @param port IO口
 *
 * @returns 有效IO_ACTIVE or 无效IO_INACTIVE
 */
/* ---------------------------------------------------------------------------*/
static int myGpioIsActive(MyGpio *This,int port)
{
	char value[2];
	MyGpioPriv *Priv;
	Priv = This->Priv+port;
	if (Priv->default_value == IO_NO_EXIST) {
		return IO_NO_EXIST;
	}
	sprintf(value,"%d",myGpioRead(This,port));
	if (strcmp(value,Priv->active) == 0){
		return IO_ACTIVE;
	} else {
		return IO_INACTIVE;
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioFlashTimer GPIO闪烁执行函数，在单独的定时线程中执行
 * IO口电平变化到还原算一次
 *
 * @param This
 */
/* ---------------------------------------------------------------------------*/
static void myGpioFlashTimer(MyGpio *This)
{
	int i;
	MyGpioPriv *Priv;
	Priv = This->Priv;
	for (i=0; i<This->io_num; i++) {
		if (Priv->default_value == IO_NO_EXIST) {
			Priv++;
			continue;
		}
		if (Priv->default_value == IO_INPUT) {
			Priv++;
			continue;
		}
		if ((Priv->flash_delay_time == 0) || (Priv->flash_times == 0)) {
			Priv++;
			continue;
		}
		if (--Priv->flash_delay_time == 0) {
			Priv->flash_even_flag++;
			if (Priv->flash_even_flag == 2) {  //亮灭算一次
				Priv->flash_times--;
				Priv->flash_even_flag = 0;
			}

			Priv->flash_delay_time = Priv->flash_set_time;

			if (myGpioIsActive(This,i) == IO_ACTIVE)
				myGpioSetValueNow(This,i,IO_INACTIVE);
			else
				myGpioSetValueNow(This,i,IO_ACTIVE);
		}
		Priv++;
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioFlashStart 设置GPIO闪烁，并执行
 *
 * @param This
 * @param port IO口
 * @param freq 频率 根据myGpioFlashTimer 执行时间决定
 * @param times 闪烁总次数 FLASH_FOREVER为循环闪烁
 */
/* ---------------------------------------------------------------------------*/
static void myGpioFlashStart(MyGpio *This,int port,int freq,int times)
{
	MyGpioPriv *Priv;
	Priv = This->Priv+port;
	if (Priv->default_value == IO_NO_EXIST) {
		return;
	}
	if (Priv->flash_set_time != freq) {
		Priv->flash_delay_time = freq;
		Priv->flash_set_time = freq;
		Priv->flash_times = times;
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioFlashStop GPIO停止闪烁
 *
 * @param This
 * @param port IO口
 */
/* ---------------------------------------------------------------------------*/
static void myGpioFlashStop(MyGpio *This,int port)
{
	MyGpioPriv *Priv;
	Priv = This->Priv+port;
	if (Priv->default_value == IO_NO_EXIST) {
		return;
	}

	Priv->flash_delay_time = 0;
	Priv->flash_set_time = FLASH_STOP;
	Priv->flash_times = 0;
	Priv->flash_even_flag = 0;
	myGpioSetValue(This,port,IO_INACTIVE);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioSetActiveTime 设置输入IO口的有效电平时间
 *
 * @param This
 * @param port IO口
 * @param value 时间
 */
/* ---------------------------------------------------------------------------*/
static void myGpioSetActiveTime(struct _MyGpio *This,int port,int value)
{
	MyGpioPriv *Priv;
	Priv = This->Priv+port;
	if (Priv->default_value != IO_INPUT) {
		return;
	}
	Priv->active_time = value;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioSetActiveValue 设置输入IO口的有效值
 *
 * @param This
 * @param port IO口
 * @param value 时间
 */
/* ---------------------------------------------------------------------------*/
static void myGpioSetActiveValue(struct _MyGpio *This,int port,int value)
{
	MyGpioPriv *Priv;
	Priv = This->Priv+port;

	char buf[2] = {0};
	sprintf(buf,"%d",value);
	if (strcmp(buf,Priv->active) != 0) {
		if (value == 1 )
			Priv->active = "1";
		else
			Priv->active = "0";
	}
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioGetActiveValue 获取输入IO口的有效值
 *
 * @param This
 * @param port IO口
 * @param value 时间
 */
/* ---------------------------------------------------------------------------*/
static int myGpioGetActiveValue(struct _MyGpio *This,int port)
{
	MyGpioPriv *Priv;
	Priv = This->Priv+port;

	return atoi(Priv->active);
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioInputHandle 检测输入IO电平
 *
 * @param This
 * @param port
 *
 * @returns 1为有效 0为无效
 */
/* ---------------------------------------------------------------------------*/
static int myGpioInputHandle(struct _MyGpio *This,int port)
{
	MyGpioPriv *Priv;
	Priv = This->Priv+port;
	int ret = myGpioIsActive(This,port);
	// DPRINT("port:%d,ret:%d,delay_time:%d\n",
		 // port,ret,(This->Priv+port)->delay_time );
	if (ret != IO_ACTIVE) {
		return 0;
	} else 
		return 1;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioGetActiveTime 获取输入IO的输入有效电平时间
 *
 * @param This
 * @param port
 *
 * @returns 
 */
/* ---------------------------------------------------------------------------*/
static int myGpioGetActiveTime(struct _MyGpio *This,int port)
{
	MyGpioPriv *Priv;
	Priv = This->Priv+port;
	return Priv->active_time;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioInit GPIO初始化，在创建IO对象后必须执行
 *
 * @param This
 */
/* ---------------------------------------------------------------------------*/
static void myGpioInit(MyGpio *This)
{
	FILE *fp;
	int i;
	char string_buf[50];
	MyGpioPriv *Priv;
	Priv = This->Priv;
	for (i=0; i<This->io_num; i++) {
		if (Priv->default_value == IO_NO_EXIST) {
			Priv++;
			continue;
		}
		if ((fp = fopen("/sys/class/gpio/export", "w")) == NULL) {
			DPRINT("Init:/sys/class/gpio/export fopen failed.\n");
			Priv++;
			continue;
			// exit(1);
		}
		// DPRINT("[%s](%d,df_value:%d)\n",__FUNCTION__,i,Priv->default_value);
		Priv->portnum = Priv->portmask;
		switch (Priv->portid)
		{
			case GPIO_GROUP_A : break;
			case GPIO_GROUP_B : Priv->portnum += 32;   break;
			case GPIO_GROUP_C : Priv->portnum += 32*2; break;
			case GPIO_GROUP_D : Priv->portnum += 32*3; break;
			case GPIO_GROUP_E : Priv->portnum += 32*4; break;
			case GPIO_GROUP_G : Priv->portnum += 32*5; break;
			case GPIO_GROUP_H : Priv->portnum += 32*6; break;
			default : DPRINT("GPIO should be:a-h\n");break;
		}
		fprintf(fp,"%d",Priv->portnum);
		fclose(fp);

		sprintf(string_buf,"/sys/class/gpio/gpio%d/direction",Priv->portnum);

		if ((fp = fopen(string_buf, "rb+")) == NULL) {
			DPRINT("Init:%s,fopen failed.\n",string_buf);
			Priv++;
			continue;
			// exit(1);
		}
		if (Priv->default_value != IO_INPUT) {
			fprintf(fp,"out");
		} else {
			fprintf(fp,"in");
		}
		fclose(fp);
		if (Priv->default_value != IO_INPUT)//设置默认值
			myGpioSetValueNow(This,i,Priv->default_value);

		Priv->flash_delay_time = 0;
		Priv->flash_set_time = FLASH_STOP;
		Priv->flash_times = 0;
		Priv->flash_even_flag = 0;

		Priv++;
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioDestroy 销毁GPIO对象
 *
 * @param This
 */
/* ---------------------------------------------------------------------------*/
static void myGpioDestroy(MyGpio *This)
{
	free(This->Priv);
	free(This);
	This = NULL;
}

/* ----------------------------------------------------------------*/
/**
 * @brief myGpioHandle GPIO输出执行函数，真正对IO口赋值，在单独线程
 * 中执行，IO输出以脉冲形式，防止非正常情况导致IO口电平一时错误
 *
 * @param this
 */
/* ---------------------------------------------------------------------------*/
static void myGpioHandle(MyGpio *This)
{
	FILE *fp;
	int i;
	char string_buf[50],buffer[10];
	MyGpioPriv *Priv;
	Priv = This->Priv;
	for (i=0; i<This->io_num; i++) {
		if (Priv->default_value == IO_NO_EXIST) {
			Priv++;
			continue;
		}
		if (Priv->default_value == IO_INPUT) {
			Priv++;
			continue;
		}

		sprintf(string_buf,"/sys/class/gpio/gpio%d/value",Priv->portnum);
		if ((fp = fopen(string_buf, "rb+")) == NULL) {
			DPRINT("Handle GPIO[%d][%c%d]Cannot open value file.\n",
					i,Priv->portid,Priv->portmask);
			// exit(1);
		} else {
			sprintf(buffer,"%d",Priv->current_value);
			fwrite(buffer, sizeof(char), sizeof(buffer) - 1, fp);
			fclose(fp);
		}
		Priv++;
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioThread Gpio处理函数，50ms处理一次
 *
 * @param arg
 */
/* ---------------------------------------------------------------------------*/
static void * myGpioThread(void *arg)
{
	MyGpio *This = arg;
	while (This != NULL) {
		This->Handle(This);
		This->FlashTimer(This);
		usleep(50000);// 50ms
	}
	return NULL;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioHandleThreadCreate 创建Gpio处理线程
 */
/* ---------------------------------------------------------------------------*/
static void myGpioHandleThreadCreate(MyGpio *This)
{
	int result;
	pthread_t m_pthread;					//线程号
	pthread_attr_t threadAttr1;				//线程属性
	pthread_attr_init(&threadAttr1);		//附加参数
	//设置线程为自动销毁
	pthread_attr_setdetachstate(&threadAttr1,PTHREAD_CREATE_DETACHED);
	//创建线程，无传递参数
	result = pthread_create(&m_pthread,&threadAttr1,myGpioThread,This);
	if(result) {
		DPRINT("[%s] pthread failt,Error code:%d\n",__FUNCTION__,result);
	}
	pthread_attr_destroy(&threadAttr1);		//释放附加参数
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioInputThreadCreate 创建检测输入线程
 */
/* ---------------------------------------------------------------------------*/
static void myGpioInputThreadCreate(MyGpio *This,
		void *(*checkInputHandle)(void *))
{
    int result;
    pthread_t m_pthread;
    pthread_attr_t threadAttr1;

    pthread_attr_init(&threadAttr1);
    pthread_attr_setdetachstate(&threadAttr1,PTHREAD_CREATE_DETACHED);
    result = pthread_create(&m_pthread,&threadAttr1,checkInputHandle,This);
    if(result) {
        DPRINT("[%s] pthread failt,Error code:%d\n",__FUNCTION__,result);
    }
    pthread_attr_destroy(&threadAttr1);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioPrivCreate 创建GPIO对象
 *
 * @param gpio_table GPIO列表
 *
 * @returns GPIO对象
 */
/* ---------------------------------------------------------------------------*/
MyGpio* myGpioPrivCreate(MyGpioPriv *gpio_table)
{
	pthread_mutexattr_t mutexattr;
	pthread_mutexattr_init(&mutexattr);
	MyGpio *This = (MyGpio *)malloc(sizeof(MyGpio));
	memset(This,0,sizeof(MyGpio));
    if (gpio_table == gpiotbl) {
		This->io_num = sizeof(gpiotbl) / sizeof(MyGpioPriv);
		// DPRINT("gpio_tbl:%d\n",This->io_num);
	}
	This->Priv = (MyGpioPriv *)malloc(sizeof(MyGpioPriv) * This->io_num);
	memset(This->Priv,0,sizeof(MyGpioPriv) * This->io_num);
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&gpioLock, &mutexattr);
	pthread_mutexattr_destroy(&mutexattr);

	This->Priv = gpio_table;
	This->SetValue = myGpioSetValue;
	This->SetValueNow = myGpioSetValueNow;
	This->FlashStart = myGpioFlashStart;
	This->FlashStop = myGpioFlashStop;
	This->FlashTimer = myGpioFlashTimer;
	This->Destroy = myGpioDestroy;
	This->Handle = myGpioHandle;
	This->IsActive = myGpioIsActive;
	This->setActiveTime = myGpioSetActiveTime;
	This->getActiveTime = myGpioGetActiveTime;
	This->setActiveValue = myGpioSetActiveValue;
	This->getActiveValue = myGpioGetActiveValue;
	This->inputHandle = myGpioInputHandle;
	This->creatOutputThread = myGpioHandleThreadCreate;
	This->creatInputThread = myGpioInputThreadCreate;
	myGpioInit(This);
	return This;
}

void gpioInit(void)
{
	DPRINT("gpio init\n");
	gpio = myGpioPrivCreate(gpiotbl);	
	gpio->creatOutputThread(gpio);
}

