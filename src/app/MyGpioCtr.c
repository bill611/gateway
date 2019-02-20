/*
 * =====================================================================================
 *
 *       Filename:  MyGpioCtr.c
 *
 *    Description:  GPIO����
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
#include "hal_gpio.h"
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
#define GPIO_MAX_INPUT_TASK 32
typedef struct _MyGpioTable {
	int       portid;
	int       mask;
	int       active;		//��Чֵ
	int 	  default_value;	//Ĭ��ֵ
	int  	  active_time;

	int		  current_value;
	int		  portnum;
	int 	  flash_times;
	int 	  flash_set_time;
	int 	  flash_delay_time;
	int 	  flash_even_flag;
	int		  delay_time;
}MyGpioTable;

typedef struct _MyGpioInputTask {
	int port;
	void *arg;
	void (* thread)(void *,int);
}MyGpioInputTask;

typedef struct _MyGpioPriv {
	MyGpioTable *table;
	pthread_mutex_t mutex;
	int task_num;
	MyGpioInputTask task[GPIO_MAX_INPUT_TASK];
}MyGpioPriv;

 
#if (defined V23)
#define GPIO_ZIGBEE_POWER		84,-1,1,IO_ACTIVE
#define GPIO_WIFI_POWER			83,-1,1,IO_ACTIVE
#define GPIO_LED_WIFI			80,-1,0,IO_INACTIVE
#define GPIO_LED_RESET			80,-1,0,IO_INACTIVE
#define GPIO_LED_ONLINE			80,-1,0,IO_INACTIVE
#define GPIO_LED_NET_IN			80,-1,0,IO_ACTIVE

#define GPIO_RESET				85,-1,0,IO_INPUT
#define GPIO_MODE				80,-1,0,IO_INPUT
#else

#define GPIO_ZIGBEE_POWER		'c',15,1,IO_ACTIVE
#define GPIO_WIFI_POWER			'e',0, 1,IO_ACTIVE
#define GPIO_LED_WIFI			'c',14,0,IO_INACTIVE
#define GPIO_LED_RESET			'd',11,0,IO_INACTIVE
#define GPIO_LED_ONLINE			'd',10,0,IO_INACTIVE
#define GPIO_LED_NET_IN			'c',13,0,IO_ACTIVE

#define GPIO_RESET				'e',1,0,IO_INPUT
#define GPIO_MODE				'd',3,0,IO_INPUT
#endif
/* ----------------------------------------------------------------*
 *                      variables define
 *-----------------------------------------------------------------*/

static MyGpioTable gpio_normal_tbl[]={
	{GPIO_ZIGBEE_POWER,	0},
	{GPIO_WIFI_POWER,	0},
	{GPIO_LED_WIFI,		0},
	{GPIO_LED_RESET,	0},
	{GPIO_LED_ONLINE,	0},
	{GPIO_LED_NET_IN,	0},

	{GPIO_RESET,		30},
	{GPIO_MODE,			1},
};

MyGpio *gpio = NULL;
/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioSetValue GPIO�������ֵ��������ִ��
 *
 * @param This
 * @param port IO��
 * @param Value ��ЧIO_ACTIVE or ��ЧIO_INACTIVE
 */
/* ---------------------------------------------------------------------------*/
static int myGpioSetValue(MyGpio *This,int port,int  Value)
{
	MyGpioTable *table;
	table = This->priv->table+port;

#ifdef WIN32
	if (table->default_value == IO_NO_EXIST ) {   //���
#else
	if (	(table->default_value == IO_INPUT)
		||  (table->default_value == IO_NO_EXIST) ) {   //���
#endif
		// printf("[%d]set value fail,it is input or not exist!\n",port);
		return 0;
	}

	if (Value == IO_ACTIVE) {
		table->current_value = table->active;
	} else {
		table->current_value = !(table->active);
	}
	return 1;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioSetValueNow GPIO�������ֵ��������ִ��
 *
 * @param This
 * @param port IO��
 * @param Value ��ЧIO_ACTIVE or ��ЧIO_INACTIVE
 */
/* ---------------------------------------------------------------------------*/
static void myGpioSetValueNow(MyGpio *This,int port,int  Value)
{
	MyGpioTable *table;
	table = This->priv->table+port;
	if (myGpioSetValue(This,port,Value) == 0)
		return;

	pthread_mutex_lock(&This->priv->mutex);
	if (table->current_value)
		halGpioOut(table->portid,table->mask,1);
	else
		halGpioOut(table->portid,table->mask,0);
	pthread_mutex_unlock(&This->priv->mutex);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioRead ������ڵ�ֵ
 *
 * @param This
 * @param port IO��
 *
 * @returns ������ֵ 0��1
 */
/* ---------------------------------------------------------------------------*/
static int myGpioRead(MyGpio *This,int port)
{
	MyGpioTable *table;
	table = This->priv->table+port;
	if (table->default_value != IO_INPUT) {
		goto return_value;
	}
#ifndef WIN32
	if (halGpioIn(table->portid,table->mask))
		table->current_value = 1;
	else
		table->current_value = 0;
#endif

return_value:
	return table->current_value;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioIsActive ��ȡ����IOֵ�����ж��Ƿ�IO��Ч
 *
 * @param This
 * @param port IO��
 *
 * @returns ��ЧIO_ACTIVE or ��ЧIO_INACTIVE
 */
/* ---------------------------------------------------------------------------*/
static int myGpioIsActive(MyGpio *This,int port)
{
	MyGpioTable *table;
	table = This->priv->table+port;
	if (table->default_value == IO_NO_EXIST) {
		return IO_NO_EXIST;
	}
	if (myGpioRead(This,port) == table->active){
		return IO_ACTIVE;
	} else {
		return IO_INACTIVE;
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioFlashTimer GPIO��˸ִ�к������ڵ����Ķ�ʱ�߳���ִ��
 * IO�ڵ�ƽ�仯����ԭ��һ��
 *
 * @param This
 */
/* ---------------------------------------------------------------------------*/
static void myGpioFlashTimer(MyGpio *This)
{
	int i;
	MyGpioTable *table;
	table = This->priv->table;
	for (i=0; i<This->io_num; i++) {
		if (table->default_value == IO_NO_EXIST) {
			table++;
			continue;
		}
		if (table->default_value == IO_INPUT) {
			table++;
			continue;
		}
		if ((table->flash_delay_time == 0) || (table->flash_times == 0)) {
			table++;
			continue;
		}
		if (--table->flash_delay_time == 0) {
			table->flash_even_flag++;
			if (table->flash_even_flag == 2) {  //������һ��
				table->flash_times--;
				table->flash_even_flag = 0;
			}

			table->flash_delay_time = table->flash_set_time;

			if (myGpioIsActive(This,i) == IO_ACTIVE)
				myGpioSetValueNow(This,i,IO_INACTIVE);
			else
				myGpioSetValueNow(This,i,IO_ACTIVE);
		}
		table++;
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioFlashStart ����GPIO��˸����ִ��
 *
 * @param This
 * @param port IO��
 * @param freq Ƶ�� ����myGpioFlashTimer ִ��ʱ�����
 * @param times ��˸�ܴ��� FLASH_FOREVERΪѭ����˸
 */
/* ---------------------------------------------------------------------------*/
static void myGpioFlashStart(MyGpio *This,int port,int freq,int times)
{
	MyGpioTable *table;
	table = This->priv->table+port;
	if (table->default_value == IO_NO_EXIST) {
		return;
	}
	if (table->flash_set_time != freq) {
		table->flash_delay_time = freq;
		table->flash_set_time = freq;
		table->flash_times = times;
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioFlashStop GPIOֹͣ��˸
 *
 * @param This
 * @param port IO��
 */
/* ---------------------------------------------------------------------------*/
static void myGpioFlashStop(MyGpio *This,int port)
{
	MyGpioTable *table;
	table = This->priv->table+port;
	if (table->default_value == IO_NO_EXIST) {
		return;
	}

	table->flash_delay_time = 0;
	table->flash_set_time = FLASH_STOP;
	table->flash_times = 0;
	table->flash_even_flag = 0;
	myGpioSetValue(This,port,IO_INACTIVE);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioSetActiveTime ��������IO�ڵ���Ч��ƽʱ��
 *
 * @param This
 * @param port IO��
 * @param value ʱ��
 */
/* ---------------------------------------------------------------------------*/
static void myGpioSetActiveTime(struct _MyGpio *This,int port,int value)
{
	MyGpioTable *table;
	table = This->priv->table+port;
	if (table->default_value != IO_INPUT) {
		return;
	}
	table->active_time = value;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioInputHandle �������IO��ƽ
 *
 * @param This
 * @param port
 *
 * @returns 1Ϊ��Ч 0Ϊ��Ч
 */
/* ---------------------------------------------------------------------------*/
static int myGpioInputHandle(struct _MyGpio *This,int port)
{
	MyGpioTable *table;
	table = This->priv->table+port;
	int ret = myGpioIsActive(This,port);
	// printf("port:%d,ret:%d,delay_time:%d\n",
		 // port,ret,table->delay_time );
	if (ret != IO_ACTIVE) {
		table->delay_time = 0;
		return 0;
	}
	if (table->delay_time < table->active_time) {
		++table->delay_time;
		return 0;
	} else {
		return 1;
	}
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioInit GPIO��ʼ�����ڴ���IO��������ִ��
 *
 * @param This
 */
/* ---------------------------------------------------------------------------*/
static void myGpioInit(MyGpio *This)
{
	int i;
	MyGpioTable *table;
	table = This->priv->table;
	for (i=0; i<This->io_num; i++) {
		if (table->portid < 0) // �����������ļ�������IOд-1
			table->default_value = IO_NO_EXIST;

		if (table->default_value == IO_NO_EXIST) {
			table++;
			continue;
		}

		if (table->default_value != IO_INPUT) {
			halGpioSetMode(table->portid,table->mask,HAL_OUTPUT);
		} else {
			halGpioSetMode(table->portid,table->mask,HAL_INPUT);
		}
		if (table->default_value != IO_INPUT)//����Ĭ��ֵ
			myGpioSetValueNow(This,i,table->default_value);

		table->flash_delay_time = 0;
		table->flash_set_time = FLASH_STOP;
		table->flash_times = 0;
		table->flash_even_flag = 0;

		table++;
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioDestroy ����GPIO����
 *
 * @param This
 */
/* ---------------------------------------------------------------------------*/
static void myGpioDestroy(MyGpio *This)
{
	free(This->priv);
	free(This);
	This = NULL;
}

/* ----------------------------------------------------------------*/
/**
 * @brief myGpioHandle GPIO���ִ�к�����������IO�ڸ�ֵ���ڵ����߳�
 * ��ִ�У�IO�����������ʽ����ֹ�������������IO�ڵ�ƽһʱ����
 *
 * @param this
 */
/* ---------------------------------------------------------------------------*/
static void myGpioHandle(MyGpio *This)
{
	int i;
	MyGpioTable *table;
	table = This->priv->table;
	for (i=0; i<This->io_num; i++) {
		if (table->default_value == IO_NO_EXIST) {
			table++;
			continue;
		}
		if (table->default_value == IO_INPUT) {
			table++;
			continue;
		}

		if (table->current_value)
			halGpioOut(table->portid,table->mask,1);
		else
			halGpioOut(table->portid,table->mask,0);
		table++;
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioThread Gpio��������50ms����һ��
 *
 * @param arg
 */
/* ---------------------------------------------------------------------------*/
static void * myGpioOutputThread(void *arg)
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
 * @brief myGpioHandleThreadCreate ����Gpio�����߳�
 */
/* ---------------------------------------------------------------------------*/
static void myGpioOutputThreadCreate(MyGpio *This)
{
	int result;
	pthread_t m_pthread;					//�̺߳�
	pthread_attr_t threadAttr1;				//�߳�����
	pthread_attr_init(&threadAttr1);		//���Ӳ���
	//�����߳�Ϊ�Զ�����
	pthread_attr_setdetachstate(&threadAttr1,PTHREAD_CREATE_DETACHED);
	//�����̣߳��޴��ݲ���
	result = pthread_create(&m_pthread,&threadAttr1,myGpioOutputThread,This);
	if(result)
		printf("[%s] pthread failt,Error code:%d\n",__FUNCTION__,result);

	pthread_attr_destroy(&threadAttr1);		//�ͷŸ��Ӳ���
}

static void myGpioAddInputThread(MyGpio *This,
		struct GpioArgs *args,
		void *(* thread)(void *))
{
#if 0
	if (This->priv->task_num == GPIO_MAX_INPUT_TASK) {
		printf("Err: input thread task full!!\n");
		return;
	}
	pthread_mutex_lock(&This->priv->mutex);
	This->priv->task[This->priv->task_num].thread = thread;
	This->priv->task[This->priv->task_num].arg = arg;
	This->priv->task[This->priv->task_num].port = port;
	This->priv->task_num++;
	pthread_mutex_unlock(&This->priv->mutex);
#else
    int result;
    pthread_t m_pthread;
    pthread_attr_t threadAttr1;

    pthread_attr_init(&threadAttr1);
    pthread_attr_setdetachstate(&threadAttr1,PTHREAD_CREATE_DETACHED);
    result = pthread_create(&m_pthread,&threadAttr1,thread,args);
    if(result)
        printf("[%s] pthread failt,Error code:%d\n",__FUNCTION__,result);

    pthread_attr_destroy(&threadAttr1);
#endif
}

static void * myGpioInputThread(void *arg)
{
	int i;
	MyGpio *This = arg;
	while (This != NULL) {
		for (i=0; i<This->priv->task_num; i++) {
	       This->priv->task[i].thread(This->priv->task[i].arg,
				   This->priv->task[i].port);
		}
		usleep(10000);
	}
	return NULL;
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioInputThreadCreate ������������߳�
 */
/* ---------------------------------------------------------------------------*/
static void myGpioInputThreadCreate(MyGpio *This)
{
    int result;
    pthread_t m_pthread;
    pthread_attr_t threadAttr1;

    pthread_attr_init(&threadAttr1);
    pthread_attr_setdetachstate(&threadAttr1,PTHREAD_CREATE_DETACHED);
    result = pthread_create(&m_pthread,&threadAttr1,myGpioInputThread,This);
    if(result)
        printf("[%s] pthread failt,Error code:%d\n",__FUNCTION__,result);

    pthread_attr_destroy(&threadAttr1);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioPrivCreate ����GPIO����
 *
 * @param gpio_table GPIO�б�
 *
 * @returns GPIO����
 */
/* ---------------------------------------------------------------------------*/
MyGpio* myGpioPrivCreate(MyGpioTable *gpio_table,int io_num)
{

	MyGpio *This = (MyGpio *)calloc(1,sizeof(MyGpio));
	This->priv = (MyGpioPriv *)calloc(1,sizeof(MyGpioPriv));
	pthread_mutexattr_t mutexattr;
	pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&This->priv->mutex, &mutexattr);
	pthread_mutexattr_destroy(&mutexattr);

	This->priv->table = gpio_table;
	This->io_num = io_num;
	This->SetValue = myGpioSetValue;
	This->SetValueNow = myGpioSetValueNow;
	This->FlashStart = myGpioFlashStart;
	This->FlashStop = myGpioFlashStop;
	This->FlashTimer = myGpioFlashTimer;
	This->Destroy = myGpioDestroy;
	This->Handle = myGpioHandle;
	This->IsActive = myGpioIsActive;
	This->setActiveTime = myGpioSetActiveTime;
	This->inputHandle = myGpioInputHandle;
	This->addInputThread = myGpioAddInputThread;
	myGpioInit(This);
	myGpioOutputThreadCreate(This);
	// myGpioInputThreadCreate(This);
	return This;
}

void gpioInit(void)
{
	DPRINT("gpio init\n");
	gpio = myGpioPrivCreate(gpio_normal_tbl,
			sizeof(gpio_normal_tbl) / sizeof(MyGpioTable));

}

