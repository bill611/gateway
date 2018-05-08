/*
 * =============================================================================
 *
 *       Filename:  uart.c
 *
 *    Description:  uart driver
 *
 *        Version:  1.0
 *        Created:  2016-08-06 16:45:01
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
#include <strings.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <sys/times.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <pthread.h>

#include <termios.h>	//termios.tcgetattr(),tcsetattr

#include "uart.h"
#include "queue.h"
/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/
static void uartReceiveCreate(UartServer * This);
static void uartSendCreate(UartServer * This);
/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/

#define TTY_DEV "/dev/ttyS"

typedef struct {
	uint32_t baudrate;
	uint8_t data_bit;
	uint8_t parity;
	uint8_t stop_bit;
}PortInfo ;

typedef struct _UartServerPriv {
	int32_t fd;
	uint8_t terminated;
	Queue *queue;
	pthread_mutex_t mutex;		//队列控制互斥信号
	PortInfo comparam;  // linux接口使用
}UartServerPriv;

typedef struct _UartSendBuf{
	int len;
	uint8_t data[MAX_SEND_BUFF];
} UartSendBuf;

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static void (*call_back_func)(void);
UartServer *uart = NULL;

/* ---------------------------------------------------------------------------*/
/**
 * @brief uartConvbaud 设置波特率
 *
 * @param baudrate 波特率
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static int uartConvbaud(unsigned long int baudrate)
{
	switch(baudrate)
	{
		case 2400 :  return B2400;
		case 4800 :  return B4800;
		case 9600 :  return B9600;
		case 19200:  return B19200;
		case 38400:  return B38400;
		case 57600:  return B57600;
		case 115200: return B115200;
		default:      return B9600;
	}
}
/**********************************
 * Setup comm attr
 * fdcon????????????,pportinfo:
 ***********************************/
static int uartPortSet(UartServer *This)
{
	struct termios termios_old,termios_new;
	int    baudrate,tmp,temp;
	char   databit,stopbit,parity;
	temp = This->priv->fd;
	bzero(&termios_old,sizeof(termios_old));
	bzero(&termios_new,sizeof(termios_new));
	cfmakeraw(&termios_new);
	tcgetattr(temp,&termios_old);

	baudrate = uartConvbaud(This->priv->comparam.baudrate);
	cfsetispeed(&termios_new,baudrate);
	cfsetospeed(&termios_new,baudrate);
	termios_new.c_cflag |= CLOCAL;
	termios_new.c_cflag |= CREAD;
	termios_new.c_iflag |= IXON|IXOFF|IXANY;
	termios_new.c_cflag&=~CSIZE;
	databit = This->priv->comparam.data_bit;
	switch(databit){
		case '5' :
			termios_new.c_cflag |= CS5;
		case '6' :
			termios_new.c_cflag |= CS6;
		case '7' :
			termios_new.c_cflag |= CS7;
		default:
			termios_new.c_cflag |= CS8;

	}
	parity = This->priv->comparam.parity;
	switch(parity)
	{
		default:
		case '0' :
			termios_new.c_cflag &= ~PARENB;
			break;
		case '1' :
			termios_new.c_cflag |= PARENB;
			termios_new.c_cflag &=~PARODD;
			break;
		case '2' :
			termios_new.c_cflag |= PARENB;
			termios_new.c_cflag |= PARODD;
			break;
	}

	stopbit =This->priv->comparam.stop_bit;

	if(stopbit == '2') 
		termios_new.c_cflag |= CSTOPB;                //2 stop bits
	else
		termios_new.c_cflag &=~CSTOPB;             //1 stop bits
	
	termios_new.c_lflag &=~(ICANON|ECHO|ECHOE|ISIG);
	termios_new.c_oflag &= OPOST;                   //
	termios_new.c_cc[VMIN]  = 1;                    //
	termios_new.c_cc[VTIME] = 0;		      //
	termios_new.c_lflag &= (ICANON|ECHO|ECHOE|ISIG);

	tcflush(temp,TCIFLUSH);                              //
	tmp = tcsetattr(temp,TCSANOW,&termios_new);           //
	tcgetattr(temp,&termios_old);
	return(tmp);

}
/* ---------------------------------------------------------------------------*/
/**
 * @brief uartOpen 打开串口
 *
 * @param This
 * @param com 串口编号
 * @param baudrate 波特率
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static int uartOpen(UartServer *This,int com,int baudrate)
{
	char *ptty;

	switch(com){
		case 0:
			ptty = TTY_DEV"0";
			break;
		case 1:
			ptty = TTY_DEV"1";
			break;
		case 2:
			ptty = TTY_DEV"2";
			break;
		default:
			ptty = TTY_DEV"3";
			break;
	}
	if(baudrate>=1200)
		This->priv->comparam.baudrate = baudrate;
	This->priv->fd = open(ptty,O_RDWR|O_NOCTTY|O_NONBLOCK|O_NDELAY);
	uartPortSet(This);
	uartReceiveCreate(This);
	uartSendCreate(This);
	return This->priv->fd > 0;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief printfbuf 调试输出发送串口字节
 *
 * @param pbuf
 * @param size
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
#if 1
static void printfbuf(void *pbuf,int size)
{
	int i;
	unsigned char *pData = (unsigned char *)pbuf;
	printf("SendData[%d]  ",size);
	for(i=0;i<size;i++) {
		printf("%02X ",pData[i]);
	}
	printf("\n");
}
#else
#define printfbuf(pbuf,size)
#endif

/* ---------------------------------------------------------------------------*/
/**
 * @brief uartSend 发送数据
 *
 * @param This
 * @param Buf
 * @param datalen
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static int uartSend(UartServer *This,void *Buf,int datalen)
{
	UartSendBuf send_buf;
	pthread_mutex_lock (&This->priv->mutex);		//加锁
	memset(&send_buf,0,sizeof(UartSendBuf));
	send_buf.len = datalen;
	memcpy(send_buf.data,Buf,datalen);
	This->priv->queue->post(This->priv->queue,
			&send_buf);
	pthread_mutex_unlock (&This->priv->mutex);		//解锁
	return 1;
}

//---------------------------------------------------------------------------
static int uartRecvBuffer(UartServer *This,void *pBuf,uint32_t size)
{
	int leave_size = size;
	while(leave_size) {
		int len = read(This->priv->fd,&((char*)pBuf)[size-leave_size],leave_size);
		if(len <= 0)
			break;
		leave_size -= len;
		usleep(20000);
	}
	return size-leave_size;
}
//---------------------------------------------------------------------------
static void uartClear(UartServer *This)
{
	int result;
	char cBuf[128];
	do {
		result = uartRecvBuffer(This,cBuf,sizeof(cBuf));
	} while(result>0);
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief uartClose 暂时关闭串口
 *
 * @param This
 */
/* ---------------------------------------------------------------------------*/
static void uartClose(UartServer *This)

{
	if(This->priv->fd) {
		close(This->priv->fd);
		This->priv->fd = 0;
		This->priv->terminated = 1;
	}
}

//---------------------------------------------------------------------------
static void * uartReceiveThead(UartServer *This)
{
	int fs_sel;
	fd_set fs_read;
	struct timeval tv_timeout;

	while(!This->priv->terminated){
		FD_ZERO(&fs_read);
		FD_SET(This->priv->fd,&fs_read);
		tv_timeout.tv_sec = 3;
		tv_timeout.tv_usec = 0;
		fs_sel = select(This->priv->fd+1,&fs_read,NULL,NULL,&tv_timeout);
		if(fs_sel>0 && FD_ISSET(This->priv->fd,&fs_read)) {
			if (call_back_func)
				call_back_func();
		}
	}
	pthread_exit(NULL);
}

static void * uartSendThead(UartServer *This)
{
	UartSendBuf send_buf;
	while(!This->priv->terminated){
		This->priv->queue->get(This->priv->queue, &send_buf);
		write(This->priv->fd, send_buf.data, send_buf.len);
		printfbuf(send_buf.data,send_buf.len);
		usleep(100000);
	}
	pthread_exit(NULL);
}

static void uartReceiveCreate(UartServer * This)
{
	int ret;
	pthread_t id;
	pthread_attr_t  attr;
	pthread_attr_init(&attr);
	pthread_attr_setscope(&attr,PTHREAD_SCOPE_SYSTEM);
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
	ret = pthread_create(&id,&attr,(void *)uartReceiveThead,This);
	if(ret)
		printf("[%s pthread failt,Error code:%d\n",__FUNCTION__,ret);

	pthread_attr_destroy(&attr);
}
static void uartSendCreate(UartServer * This)
{
	int ret;
	pthread_t id;
	pthread_attr_t  attr;
	pthread_attr_init(&attr);
	pthread_attr_setscope(&attr,PTHREAD_SCOPE_SYSTEM);
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
	ret = pthread_create(&id,&attr,(void *)uartSendThead,This);
	if(ret)
		printf("[%s pthread failt,Error code:%d\n",__FUNCTION__,ret);

	pthread_attr_destroy(&attr);
}

static void uartDestroy(UartServer * This)
{
	uartClose(This);
	This->priv->terminated = 1;
	pthread_mutex_destroy (&This->priv->mutex);
	free(This);
}

UartServer *uartServerCreate(void (*func)(void))
{
	pthread_mutexattr_t mutexattr;
	UartServer *This = (UartServer *)calloc(1,sizeof(UartServer));
	This->priv = (UartServerPriv *)calloc(1,sizeof(UartServerPriv));

	pthread_mutexattr_init(&mutexattr);
	/* Set the mutex as a recursive mutex */
	pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);

	/* create the mutex with the attributes set */
	pthread_mutex_init(&This->priv->mutex, &mutexattr);
	/* destroy the attribute */
	pthread_mutexattr_destroy(&mutexattr);

	call_back_func = func;

	This->priv->queue =
		queueCreate("uart_socket",QUEUE_BLOCK,sizeof(UartSendBuf));

	This->priv->comparam.baudrate = 38400;
	This->priv->comparam.data_bit = 8;
	This->priv->comparam.parity = 0;
	This->priv->comparam.stop_bit = 1;
	This->open = uartOpen;
	This->close= uartClose;
	This->send = uartSend;
	This->recvBuffer = uartRecvBuffer;
	This->clear = uartClear;
	This->destroy = uartDestroy;

	return This;
}

void uartInit(void(*func)(void))
{
	uart = uartServerCreate(func);
	uart->open(uart,0,38400);
}
