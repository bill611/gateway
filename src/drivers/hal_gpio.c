/*
 * =============================================================================
 *
 *       Filename:  hal_gpio.c
 *
 *    Description:  硬件层 GPIO控制
 *
 *        Version:  1.0
 *        Created:  2018-12-12 16:26:55
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
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include "hal_gpio.h"

#if (defined V23)
#include "akgpio.h"
#define GPIO_DEVICE "/dev/akgpio"
#else
#endif

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static int fd;

void halGpioSetMode(int port_id,int port_mask,int dir)
{
#if (defined V23)
	if (fd <= 0) {
		fd = open(GPIO_DEVICE, O_RDWR);
		if (fd <= 0) {
			printf("Init:%s open failed.\n",GPIO_DEVICE);
		}
	}
#else
	char string_buf[50];
	FILE *fp;
	if ((fp = fopen("/sys/class/gpio/export", "w")) == NULL) {
		printf("Init:/sys/class/gpio/export fopen failed.\n");
		return;
	}
	switch (port_id)
	{
		case 'a' : break;
		case 'b' : port_mask += 32;   break;
		case 'c' : port_mask += 32*2; break;
		case 'd' : port_mask += 32*3; break;
		case 'e' : port_mask += 32*4; break;
		case 'g' : port_mask += 32*5; break;
		case 'h' : port_mask += 32*6; break;
		default : printf("GPIO should be:a-h\n");break;
	}
	fprintf(fp,"%d",port_mask);
	fclose(fp);
	sprintf(string_buf,"/sys/class/gpio/gpio%d/direction",port_mask);
	if ((fp = fopen(string_buf, "rb+")) == NULL) {
		printf("Init:%s,fopen failed.\n",string_buf);
		return	;
	}
	if (dir != HAL_INPUT)
		fprintf(fp,"out");
	else
		fprintf(fp,"in");
	fclose(fp);
#endif
}

void halGpioOut(int port_id,int port_mask,int value)
{
#if (defined V23)
    struct gpio_info info;
	info.pin = port_id;
	info.dir = AK_GPIO_DIR_OUTPUT;
	info.pulldown = AK_PULLDOWN_ENABLE;
	info.pullup   = -1;
	info.int_pol   = -1;
	if (value)
		info.value   = AK_GPIO_OUT_HIGH;
	else
		info.value   = AK_GPIO_OUT_LOW;
	if (fd)
		ioctl(fd, SET_GPIO_FUNC, &info);
#else
	char string_buf[50],buffer[10];
	FILE *fp;
	switch (port_id)
	{
		case 'a' : break;
		case 'b' : port_mask += 32;   break;
		case 'c' : port_mask += 32*2; break;
		case 'd' : port_mask += 32*3; break;
		case 'e' : port_mask += 32*4; break;
		case 'g' : port_mask += 32*5; break;
		case 'h' : port_mask += 32*6; break;
		default : printf("GPIO should be:a-h\n");break;
	}
	sprintf(string_buf,"/sys/class/gpio/gpio%d/value",port_mask);
	if ((fp = fopen(string_buf, "rb+")) == NULL) {
		return;
	} else {
		if (value)
			sprintf(buffer,"1");
		else
			sprintf(buffer,"0");
		fwrite(buffer, sizeof(char), sizeof(buffer) - 1, fp);
		fclose(fp);
	}
#endif
}

int halGpioIn(int port_id,int port_mask)
{
#if (defined V23)
    struct gpio_info info;
	info.pin = port_id;
	info.dir = AK_GPIO_DIR_OUTPUT;
	info.pulldown = AK_PULLDOWN_ENABLE;
	info.pullup   = -1;
	info.value   = AK_GPIO_OUT_HIGH;
	info.int_pol   = -1;
	if (fd)
		ioctl(fd, GET_GPIO_VALUE, &info);
	return info.value;
#else
	char string_buf[50],buffer[10];
	FILE *fp;
	switch (port_id)
	{
		case 'a' : break;
		case 'b' : port_mask += 32;   break;
		case 'c' : port_mask += 32*2; break;
		case 'd' : port_mask += 32*3; break;
		case 'e' : port_mask += 32*4; break;
		case 'g' : port_mask += 32*5; break;
		case 'h' : port_mask += 32*6; break;
		default : printf("GPIO should be:a-h\n");break;
	}
	sprintf(string_buf,"/sys/class/gpio/gpio%d/value",port_mask);
	if ((fp = fopen(string_buf, "rb")) == NULL) {
		return 0;
	} else {
		fread(buffer, sizeof(char), sizeof(buffer) - 1, fp);
		fclose(fp);
		return atoi(buffer);
	}
#endif
}
