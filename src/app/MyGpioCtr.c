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

#include "MyGpioCtr.h"


/* ----------------------------------------------------------------*
 *                  extern variables declare
 *-----------------------------------------------------------------*/

/* ----------------------------------------------------------------*
 *                  internal functions declare
 *-----------------------------------------------------------------*/

/* ----------------------------------------------------------------*
 *                        macro define
 *-----------------------------------------------------------------*/
#define GPIO_GROUP_A 'a'
#define GPIO_GROUP_B 'b'
#define GPIO_GROUP_C 'c'
#define GPIO_GROUP_D 'd'
#define GPIO_GROUP_E 'e'
#define GPIO_GROUP_G 'g'
#define GPIO_GROUP_H 'h'


#define GPIO_ZIGBEE_POWER		GPIO_GROUP_C,15

/* ----------------------------------------------------------------*
 *                      variables define
 *-----------------------------------------------------------------*/
MyGpio *gpio = NULL;

MyGpioPriv gpiotbl[]={
	{GPIO_ZIGBEE_POWER,		"1",IO_ACTIVE,0},
};

/* ----------------------------------------------------------------*/
/**
 * @brief myGpioFlashStop GPIOֹͣ��˸
 *
 * @param this
 * @param port IO��
 */
/* ----------------------------------------------------------------*/
static void myGpioFlashStop(MyGpio *this,int port)
{
	MyGpioPriv *Priv;
	Priv = this->Priv;
	if ((Priv+port)->default_value == IO_NO_EXIST) {
		return;
	}

	(Priv+port)->flash_delay_time = 0;
	(Priv+port)->flash_set_time = FLASH_STOP;
	(Priv+port)->flash_times = 0;
	(Priv+port)->flash_even_flag = 0;
}

/* ----------------------------------------------------------------*/
/**
 * @brief myGpioSetValue GPIO�������ֵ��������ִ��
 *
 * @param this
 * @param port IO��
 * @param Value ��ЧIO_ACTIVE or ��ЧIO_INACTIVE
 */
/* ----------------------------------------------------------------*/
static void myGpioSetValue(MyGpio *this,int port,int  Value)
{
	MyGpioPriv *Priv;
	Priv = this->Priv;
	if (	((Priv+port)->default_value == IO_INPUT)
		||  ((Priv+port)->default_value == IO_NO_EXIST) ) {   //���
		printf("[%d]set value fail,it is input or not exist!\n",port);
		return;
	}

	if (Value == IO_ACTIVE) {
		(Priv+port)->current_value = *((Priv+port)->active) - '0';
	} else {
		(Priv+port)->current_value = !(*((Priv+port)->active) - '0');
	}
}

/* ----------------------------------------------------------------*/
/**
 * @brief myGpioSetValueNow GPIO�������ֵ��������ִ��
 *
 * @param this
 * @param port IO��
 * @param Value ��ЧIO_ACTIVE or ��ЧIO_INACTIVE
 */
/* ----------------------------------------------------------------*/
static void myGpioSetValueNow(MyGpio *this,int port,int  Value)
{
	FILE *fp;
	char string_buf[50],buffer[10];
	MyGpioPriv *Priv;
	Priv = this->Priv;
	if (	((Priv+port)->default_value == IO_INPUT)
		||  ((Priv+port)->default_value == IO_NO_EXIST) ) {   //���
		printf("[%d]set value fail,it is input!\n",port);
		return;
	}

	if (Value == IO_ACTIVE) {
		(Priv+port)->current_value = *((Priv+port)->active) - '0';
	} else {
		(Priv+port)->current_value = !(*((Priv+port)->active) - '0');
	}

	sprintf(string_buf,"/sys/class/gpio/gpio%d/value",(Priv+port)->portnum);
	if ((fp = fopen(string_buf, "rb+")) == NULL) {
		printf("SetValueNow[%d][%c%d]Cannot open value file.\n",
				port,(Priv+port)->portid,(Priv+port)->portmask);
		// exit(1);
	} else {
		sprintf(buffer,"%d",(Priv+port)->current_value);
		fwrite(buffer, sizeof(char), sizeof(buffer) - 1, fp);
		fclose(fp);
	}
}

/* ----------------------------------------------------------------*/
/**
 * @brief myGpioRead ������ڵ�ֵ
 *
 * @param this
 * @param port IO��
 *
 * @returns ������ֵ 0��1
 */
/* ----------------------------------------------------------------*/
static int myGpioRead(MyGpio *this,int port)
{
	FILE *fp;
	char string_buf[50];
	char buffer[10];
	MyGpioPriv *Priv;
	Priv = this->Priv;
	if ((Priv+port)->default_value == IO_INPUT) {
		sprintf(string_buf,"/sys/class/gpio/gpio%d/value",(Priv+port)->portnum);
		if ((fp = fopen(string_buf, "rb")) == NULL) {
			printf("Read[%d][%c%d]Cannot open value file.\n",
					port,Priv->portid,Priv->portmask);
			// exit(1);
		} else {
			fread(buffer, sizeof(char), sizeof(buffer) - 1, fp);
			(Priv+port)->current_value = atoi(buffer);
			fclose(fp);
		}
	}
	return (Priv+port)->current_value;
}

/* ----------------------------------------------------------------*/
/**
 * @brief myGpioIsActive ��ȡ����IOֵ�����ж��Ƿ�IO��Ч
 *
 * @param this
 * @param port IO��
 *
 * @returns ��ЧIO_ACTIVE or ��ЧIO_INACTIVE
 */
/* ----------------------------------------------------------------*/
static int myGpioIsActive(MyGpio *this,int port)
{
	char value[2];
	MyGpioPriv *Priv;
	Priv = this->Priv;
	if ((Priv+port)->default_value == IO_NO_EXIST) {
		return IO_NO_EXIST;
	}
	sprintf(value,"%d",myGpioRead(this,port));
	if (strcmp(value,(Priv+port)->active) == 0){
		return IO_ACTIVE;
	} else {
		return IO_INACTIVE;
	}
}

/* ----------------------------------------------------------------*/
/**
 * @brief myGpioFlashTimer GPIO��˸ִ�к������ڵ����Ķ�ʱ�߳���ִ��
 * IO�ڵ�ƽ�仯����ԭ��һ��
 *
 * @param this
 */
/* ----------------------------------------------------------------*/
static void myGpioFlashTimer(MyGpio *this)
{
	int i;
	MyGpioPriv *Priv;
	Priv = this->Priv;
	for (i=0; i<this->io_num; i++) {
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

			if (myGpioIsActive(this,i) == IO_ACTIVE)
				myGpioSetValue(this,i,IO_INACTIVE);
			else
				myGpioSetValue(this,i,IO_ACTIVE);
		}
		Priv++;
	}
}

/* ----------------------------------------------------------------*/
/**
 * @brief myGpioFlashStart ����GPIO��˸����ִ��
 *
 * @param this
 * @param port IO��
 * @param freq Ƶ�� ����myGpioFlashTimer ִ��ʱ�����
 * @param times ��˸�ܴ��� FLASH_FOREVERΪѭ����˸
 */
/* ----------------------------------------------------------------*/
static void myGpioFlashStart(MyGpio *this,int port,int freq,int times)
{
	MyGpioPriv *Priv;
	Priv = this->Priv;
	if ((Priv+port)->default_value == IO_NO_EXIST) {
		return;
	}
	if ((Priv+port)->flash_set_time != freq) {
		(Priv+port)->flash_delay_time = freq;
		(Priv+port)->flash_set_time = freq;
		(Priv+port)->flash_times = times;
	}
}

/* ----------------------------------------------------------------*/
/**
 * @brief myGpioInit GPIO��ʼ�����ڴ���IO��������ִ��
 *
 * @param this
 */
/* ----------------------------------------------------------------*/
static void myGpioInit(MyGpio *this)
{
	FILE *fp;
	int i;
	char string_buf[50];
	MyGpioPriv *Priv;
	Priv = this->Priv;
	for (i=0; i<this->io_num; i++) {
		if (Priv->default_value == IO_NO_EXIST) {
			Priv++;
			continue;
		}
		if ((fp = fopen("/sys/class/gpio/export", "w")) == NULL) {
			printf("Init:/sys/class/gpio/export fopen failed.\n");
			Priv++;
			continue;
			// exit(1);
		}
		// printf("[%s](%d,df_value:%d)\n",__FUNCTION__,i,Priv->default_value);
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
			default : printf("GPIO should be:a-h\n");break;
		}
		fprintf(fp,"%d",Priv->portnum);
		fclose(fp);

		sprintf(string_buf,"/sys/class/gpio/gpio%d/direction",Priv->portnum);

		if ((fp = fopen(string_buf, "rb+")) == NULL) {
			printf("Init:%s,fopen failed.\n",string_buf);
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
			myGpioSetValueNow(this,i,Priv->default_value);

		Priv->flash_delay_time = 0;
		Priv->flash_set_time = FLASH_STOP;
		Priv->flash_times = 0;
		Priv->flash_even_flag = 0;

		Priv++;
	}
}

/* ----------------------------------------------------------------*/
/**
 * @brief myGpioDestroy ����GPIO����
 *
 * @param this
 */
/* ----------------------------------------------------------------*/
static void myGpioDestroy(MyGpio *this)
{
	free(this->Priv);
	free(this);
	this = NULL;
}

/* ----------------------------------------------------------------*/
/**
 * @brief myGpioHandle GPIO���ִ�к�����������IO�ڸ�ֵ���ڵ����߳�
 * ��ִ�У�IO�����������ʽ����ֹ�������������IO�ڵ�ƽһʱ����
 *
 * @param this
 */
/* ----------------------------------------------------------------*/
static void myGpioHandle(MyGpio *this)
{
	FILE *fp;
	int i;
	char string_buf[50],buffer[10];
	MyGpioPriv *Priv;
	Priv = this->Priv;
	for (i=0; i<this->io_num; i++) {
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
			printf("Handle GPIO[%d][%c%d]Cannot open value file.\n",
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

/* ----------------------------------------------------------------*/
/**
 * @brief myGpioPrivCreate ����GPIO����
 *
 * @param gpio_table GPIO�б�
 *
 * @returns GPIO����
 */
/* ----------------------------------------------------------------*/
MyGpio* myGpioPrivCreate(MyGpioPriv *gpio_table)
{
	MyGpio *this = (MyGpio *)malloc(sizeof(MyGpio));
	memset(this,0,sizeof(MyGpio));
    if (gpio_table == gpiotbl) {
		this->io_num = sizeof(gpiotbl) / sizeof(MyGpioPriv);
		// printf("gpio_tbl:%d\n",this->io_num);
	}
	this->Priv = (MyGpioPriv *)malloc(sizeof(MyGpioPriv) * this->io_num);
	memset(this->Priv,0,sizeof(MyGpioPriv) * this->io_num);

	this->Priv = gpio_table;
	this->Init = myGpioInit;
	this->SetValue = myGpioSetValue;
	this->SetValueNow = myGpioSetValueNow;
	this->FlashStart = myGpioFlashStart;
	this->FlashStop = myGpioFlashStop;
	this->FlashTimer = myGpioFlashTimer;
	this->Destroy = myGpioDestroy;
	this->Handle = myGpioHandle;
	this->IsActive = myGpioIsActive;
	return this;
}

void gpioInit(void)
{
	printf("gpio init\n");
	gpio = myGpioPrivCreate(gpiotbl);	
	gpio->Init(gpio);
}

