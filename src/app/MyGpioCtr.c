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
 * @brief myGpioSetValue GPIO�������ֵ��������ִ��
 *
 * @param This
 * @param port IO��
 * @param Value ��ЧIO_ACTIVE or ��ЧIO_INACTIVE
 */
/* ---------------------------------------------------------------------------*/
static void myGpioSetValue(MyGpio *This,int port,int  Value)
{
	MyGpioPriv *Priv;
	Priv = This->Priv+port;
	if (	(Priv->default_value == IO_INPUT)
		||  (Priv->default_value == IO_NO_EXIST) ) {   //���
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
 * @brief myGpioSetValueNow GPIO�������ֵ��������ִ��
 *
 * @param This
 * @param port IO��
 * @param Value ��ЧIO_ACTIVE or ��ЧIO_INACTIVE
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
		||  (Priv->default_value == IO_NO_EXIST) ) {   //���
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
 * @brief myGpioFlashTimer GPIO��˸ִ�к������ڵ����Ķ�ʱ�߳���ִ��
 * IO�ڵ�ƽ�仯����ԭ��һ��
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
			if (Priv->flash_even_flag == 2) {  //������һ��
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
 * @brief myGpioFlashStop GPIOֹͣ��˸
 *
 * @param This
 * @param port IO��
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
 * @brief myGpioSetActiveTime ��������IO�ڵ���Ч��ƽʱ��
 *
 * @param This
 * @param port IO��
 * @param value ʱ��
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
 * @brief myGpioSetActiveValue ��������IO�ڵ���Чֵ
 *
 * @param This
 * @param port IO��
 * @param value ʱ��
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
 * @brief myGpioGetActiveValue ��ȡ����IO�ڵ���Чֵ
 *
 * @param This
 * @param port IO��
 * @param value ʱ��
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
 * @brief myGpioGetActiveTime ��ȡ����IO��������Ч��ƽʱ��
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
 * @brief myGpioInit GPIO��ʼ�����ڴ���IO��������ִ��
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
		if (Priv->default_value != IO_INPUT)//����Ĭ��ֵ
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
 * @brief myGpioDestroy ����GPIO����
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
 * @brief myGpioHandle GPIO���ִ�к�����������IO�ڸ�ֵ���ڵ����߳�
 * ��ִ�У�IO�����������ʽ����ֹ�������������IO�ڵ�ƽһʱ����
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
 * @brief myGpioThread Gpio��������50ms����һ��
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
 * @brief myGpioHandleThreadCreate ����Gpio�����߳�
 */
/* ---------------------------------------------------------------------------*/
static void myGpioHandleThreadCreate(MyGpio *This)
{
	int result;
	pthread_t m_pthread;					//�̺߳�
	pthread_attr_t threadAttr1;				//�߳�����
	pthread_attr_init(&threadAttr1);		//���Ӳ���
	//�����߳�Ϊ�Զ�����
	pthread_attr_setdetachstate(&threadAttr1,PTHREAD_CREATE_DETACHED);
	//�����̣߳��޴��ݲ���
	result = pthread_create(&m_pthread,&threadAttr1,myGpioThread,This);
	if(result) {
		DPRINT("[%s] pthread failt,Error code:%d\n",__FUNCTION__,result);
	}
	pthread_attr_destroy(&threadAttr1);		//�ͷŸ��Ӳ���
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myGpioInputThreadCreate ������������߳�
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
 * @brief myGpioPrivCreate ����GPIO����
 *
 * @param gpio_table GPIO�б�
 *
 * @returns GPIO����
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

