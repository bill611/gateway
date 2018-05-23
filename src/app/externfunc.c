/*
 * =====================================================================================
 *
 *       Filename:  externfunc.c
 *
 *    Description:  �ⲿ����
 *
 *        Version:  1.0
 *        Created:  2015-12-11 11:56:30
 *       Revision:  none
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
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#include <strings.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/ip.h>
#include <net/if.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <linux/rtc.h>
#include <linux/fb.h>
#include <linux/types.h>
#include <linux/watchdog.h>
#include <sys/statfs.h>
#include <sys/mman.h>

#include "externfunc.h"

/* ----------------------------------------------------------------*
 *                  extern variables declare
 *-----------------------------------------------------------------*/

/* ----------------------------------------------------------------*
 *                  internal functions declare
 *-----------------------------------------------------------------*/

/* ----------------------------------------------------------------*
 *                        macro define
 *-----------------------------------------------------------------*/
#define	IOCTL_PD	0x1001
#define	IOCTL_PU	0x1002
#define	IOCTL_NM	0x1003

/* ----------------------------------------------------------------*
 *                      variables define
 *-----------------------------------------------------------------*/


/* ---------------------------------------------------------------------------*/
/**
 * @brief WatchDogOpen �򿪲���ʼ�����Ź�
 */
/* ---------------------------------------------------------------------------*/
void WatchDogOpen(void)
{
#ifndef WATCHDOG_DEBUG
	// if(Public.WatchDog_fd > 0) {
		// return;
	// }

	// Public.WatchDog_fd = open("/dev/watchdog", O_WRONLY);
	// if (Public.WatchDog_fd == -1) {
		// perror("watchdog");
	// } else {
		// printf("Init WatchDog!!!!!!!!!!!!!!!!!!\n");
	// }
#endif
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief WatchDogFeed ι������
 */
/* ---------------------------------------------------------------------------*/
void WatchDogFeed(void)
{
#ifndef WATCHDOG_DEBUG
	// if(Public.WatchDog_fd <= 0) {
		// return;
	// }
	// ioctl(Public.WatchDog_fd, WDIOC_KEEPALIVE);
#endif
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief WatchDogClose �رտ��Ź�
 */
/* ---------------------------------------------------------------------------*/
void WatchDogClose(void)
{
#ifndef WATCHDOG_DEBUG
	// if(Public.WatchDog_fd <= 0) {
		// return;
	// }
	// char * closestr="V";
	// write(Public.WatchDog_fd,closestr,strlen(closestr));
	// close(Public.WatchDog_fd);
	// Public.WatchDog_fd = -2;
#endif
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief ErrorLog ��¼������־
 *
 * @param ecode
 * @param fmt
 * @param ...
 */
/* ---------------------------------------------------------------------------*/
void ErrorLog(int ecode,const char *fmt,...)
{
	va_list fmtargs;
	va_start(fmtargs,fmt);
	vfprintf(stderr,fmt,fmtargs);
	va_end(fmtargs);

	fprintf(stderr,"\n");
	if(ecode) {
		fprintf(stderr,"*** Error cause: %s\n",strerror(ecode));
	}
}
//---------------------------------------------------------------------------
#ifndef WIN32
//windows ��GetLocalIP������capture.c��ʵ��

int GetLocalIP(char *IP,char *NetMask,char *MAC)
{
	struct ifreq ifr;
	int sock;
	if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		return 0;
	}
	strcpy(ifr.ifr_name, "eth0");
	if(ioctl(sock, SIOCGIFADDR, &ifr) < 0)
		goto error;

	strcpy(IP,(char*)inet_ntoa(((struct sockaddr_in *)(&ifr.ifr_addr))->sin_addr));

	if (ioctl(sock, SIOCGIFNETMASK, &ifr) < 0)
		goto error;
	strcpy(NetMask,(char*)inet_ntoa(((struct sockaddr_in *)(&ifr.ifr_addr))->sin_addr));

	//��ȡMAC��ַ
	if (!(ioctl (sock, SIOCGIFHWADDR, &ifr))) {
		sprintf(MAC,"%02X:%02X:%02X:%02X:%02X:%02X",
			(unsigned char)ifr.ifr_hwaddr.sa_data[0],
			(unsigned char)ifr.ifr_hwaddr.sa_data[1],
			(unsigned char)ifr.ifr_hwaddr.sa_data[2],
			(unsigned char)ifr.ifr_hwaddr.sa_data[3],
			(unsigned char)ifr.ifr_hwaddr.sa_data[4],
			(unsigned char)ifr.ifr_hwaddr.sa_data[5]);
	}
   close(sock);
   return 1;
error:
	close(sock);
	return 0;
}
//---------------------------------------------------------------------------
#endif

/* ---------------------------------------------------------------------------*/
/**
 * @brief GetDate ȡ��ǰ����ʱ���ʽ
 *
 * @param cBuf
 * @param Size
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
char * GetDate(char *cBuf,int Size)
{
	time_t timer;
    struct tm *tm1;
	if(Size<20) {
		if(cBuf) cBuf[0]=0;
		return cBuf;
	}
	timer = time(&timer);
	tm1 = localtime(&timer);
	sprintf(cBuf,
			"%04d-%02d-%02d %02d:%02d:%02d",
			tm1->tm_year+1900,
			tm1->tm_mon+1,
			tm1->tm_mday,
			tm1->tm_hour,
			tm1->tm_min,
			tm1->tm_sec);
	return cBuf;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief GetMs ��õ�ǰϵͳ������
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
uint64_t GetMs(void)
{
	struct  timeval    tv;
    gettimeofday(&tv,NULL);
	return ((tv.tv_usec / 1000) + tv.tv_sec  * 1000 );

}

/* ---------------------------------------------------------------------------*/
/**
 * @brief fileexists �ж��ļ��Ƿ����
 *
 * @param FileName �ļ���
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
int fileexists(const char *FileName)
{
	int ret = access(FileName,0);
	return ret == 0;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief strupper Сдת��д
 *
 * @param pdst
 * @param pstr
 * @param Size
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
char *strupper(char *pdst,const char *pstr,int Size)
{
	char *pchar = pdst;
	if(pstr==NULL)
		return NULL;
	strncpy(pdst,pstr,Size);
	while(*pchar) {
		if(*pchar>='a' && *pchar<='z')
			*pchar = *pchar - 'a' + 'A';
		pchar++;
	}
	return pdst;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief DelayMs �߳�ʹ�� ��ʱ����
 *
 * @param ms
 */
/* ---------------------------------------------------------------------------*/
void DelayMs(int ms)
{
#if 0
	unsigned long long start_time;
	start_time = GetMs();
	while (!(GetMs() - start_time >= ms)) ;
#else
	usleep(ms*1000);
#endif
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief adjustdate ����ϵͳʱ��
 *
 * @param year
 * @param mon
 * @param mday
 * @param hour
 * @param min
 * @param sec
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
int adjustdate(int year,int mon,int mday,int hour,int min,int sec)
{
	//�豸ϵͳʱ��
	int rtc;
	time_t t;
	struct tm nowtime;
	nowtime.tm_sec=sec;			/*   Seconds.[0-60]   (1   leap   second)*/
	nowtime.tm_min=min;			/*   Minutes.[0-59]		*/
	nowtime.tm_hour=hour;		/*   Hours. [0-23]		*/
	nowtime.tm_mday=mday;		/*   Day.[1-31]			*/
	nowtime.tm_mon=mon-1;		/*   Month. [0-11]		*/
	nowtime.tm_year=year-1900;	/*   Year-   1900.		*/
	nowtime.tm_isdst=-1;		/*   DST.[-1/0/1]		*/
	t=mktime(&nowtime);
	stime(&t);

	//����ʵʱʱ��
	rtc = open("/dev/rtc0",O_WRONLY);
	if(rtc<0) {
		rtc = open("/dev/misc/rtc",O_WRONLY);
		if(rtc<0) {
			printf("can't open /dev/misc/rtc\n");
			return -1;
		}
	}
	if (ioctl( rtc, RTC_SET_TIME, &nowtime) < 0 ) {
		printf("Could not set the RTC time\n");
		close(rtc);
		return -1;
	}
	close(rtc);
	return 0;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief recoverData �ָ�����
 *
 * @param file �ļ�����
 * @param reboot 1��Ҫ���� 0����Ҫ����
 */
/* ---------------------------------------------------------------------------*/
void recoverData(char *file,int reboot)
{
	int size = strlen(file);
	char *backfile = (char *) malloc (sizeof(char) * size + 5);
	sprintf(backfile,"%s_bak",file);
	excuteCmd("cp",backfile,file,NULL);
	sync();
	free(backfile);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief backData   ��������
 *
 * @param file
 */
/* ---------------------------------------------------------------------------*/
void backData(char *file)
{
	int size = strlen(file);
	char *backfile = (char *) malloc (sizeof(char) * size + 5);
	sprintf(backfile,"%s_bak",file);
	excuteCmd("cp",file,backfile,NULL);
	sync();
	free(backfile);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief excuteCmd ִ��shell����,��NULL��β
 *
 * @param Cmd
 * @param ...
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static char   cmd_buf[1024] = {0};
char * excuteCmd(char *Cmd,...)
{
	int i;
	FILE *fp;
	int ret;
	va_list argp;
	char *argv;
	char commond[512] = {0};
	strcat(commond,Cmd);
	va_start( argp, Cmd);
	for(i=1;i<20;i++) {
		argv = va_arg(argp,char *);
		if(argv == NULL)
			break;
		strcat(commond," ");
		strcat(commond,argv);
	}
	va_end(argp);
	printf("cmd :%s\n",commond);
	if ((fp = popen(commond, "r") ) == 0) {
		perror("popen");
		return NULL;
	}
	memset(cmd_buf,0,sizeof(cmd_buf));
	ret = fread( cmd_buf, sizeof(cmd_buf), sizeof(char), fp ); //���ո�FILE* stream����������ȡ��cmd_buf
	printf("r:%d\n",ret );
	if ( (ret = pclose(fp)) == -1 ) {
		printf("close popen file pointer fp error!\n");
	}
	return cmd_buf;

	// �ô˺������²�����ʬ����
	// system(commond);
	// return 0;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief GetSendIP �жϱ���IP��Ŀ��IP�Ƿ�ͬһ������,
 * �������ͬһ�����ν�������С�Ĺ㲥��
 *
 * @param pSrcIP ����IP��ַ
 * @param pDestIP Ŀ��IP��ַ
 * @param pMask ������������
 *
 * @returns ��С�Ĺ㲥��IP��ַ
 */
/* ---------------------------------------------------------------------------*/
const char * GetSendIP(const char *pSrcIP,const char *pDestIP,const char *pMask)
{
	//ȡ�㲥��
	static char cIP[16];
	unsigned int SrcIP,DestIP,Mask;
	unsigned char *pIP = (unsigned char *)&DestIP;
	//ת���ַ���IP������IP��ַ
	SrcIP = inet_addr(pSrcIP);
	DestIP = inet_addr(pDestIP);
	Mask = inet_addr(pMask);
	if((SrcIP & Mask)!=(DestIP & Mask)) {
		DestIP = (SrcIP & Mask) | (~Mask & 0xFFFFFFFF);//DestIP = 0xFFFFFFFF;
	}
	//ת�����ַ���IP��ַ
	sprintf(cIP,"%d.%d.%d.%d",pIP[0],pIP[1],pIP[2],pIP[3]);
	return cIP;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief jugdeRecIP �ж�Ŀ��IP�ͱ���IP�Ƿ���ͬһ����
 *
 * @param pSrcIP ����IP
 * @param pDestIP Ŀ��IP
 * @param pMask ��������
 *
 * @returns 0����ͬһ���� 1��ͬһ����
 */
/* ---------------------------------------------------------------------------*/
int jugdeRecIP(const char *pSrcIP,const char *pDestIP,const char *pMask)
{
	unsigned int SrcIP,DestIP,Mask;
	//ת���ַ���IP������IP��ַ
	SrcIP = inet_addr(pSrcIP);
	DestIP = inet_addr(pDestIP);
	Mask = inet_addr(pMask);
	if((SrcIP & Mask)!=(DestIP & Mask)) 
		return 0;

	return 1;
}


/* ---------------------------------------------------------------------------*/
/**
 * @brief SetNetMac ����ϵ�к�����mac��ַ
 *
 * @param pImei �������к�
 * @param MAC ���ú��mac��ַ
 */
/* ---------------------------------------------------------------------------*/
void SetNetMac(unsigned char *pImei,char *MAC)
{
	unsigned char LastMacAddr; // ����õ���Mac��ַ���ֵ
	unsigned char tmpbuf[3] = {0};
	char AddressBuf[20];

//	struct timeval tpstart;

	memset(AddressBuf, 0, sizeof(AddressBuf));
//  	gettimeofday(&tpstart,NULL);
//    srand(tpstart.tv_usec);

//	LastMacAddr = (1+(int) (255.0*rand()/(RAND_MAX+1.0)));
//	sprintf(&AddressBuf[0], "%02X:", LastMacAddr);

//	LastMacAddr = (1+(int) (255.0*rand()/(RAND_MAX+1.0)));
//	sprintf(&AddressBuf[3], "%02X:", LastMacAddr);

//	LastMacAddr = (1+(int) (255.0*rand()/(RAND_MAX+1.0)));
//	sprintf(&AddressBuf[6], "%02X:", LastMacAddr);

	memcpy(AddressBuf,MAC,sizeof(AddressBuf));

	LastMacAddr = pImei[0]^pImei[3];
	sprintf(tmpbuf, "%02X", LastMacAddr);
	AddressBuf[16] = tmpbuf[1];
	AddressBuf[15] = tmpbuf[0];

	LastMacAddr = pImei[1]^pImei[4];
	sprintf(tmpbuf, "%02X:", LastMacAddr);
	AddressBuf[14] = tmpbuf[2];
	AddressBuf[13] = tmpbuf[1];
	AddressBuf[12] = tmpbuf[0];

	LastMacAddr = pImei[2]^pImei[5];
	sprintf(tmpbuf, "%02X:", LastMacAddr);
	AddressBuf[11] = tmpbuf[2];
	AddressBuf[10] = tmpbuf[1];
	AddressBuf[9] = tmpbuf[0];

	memset(MAC,0,sizeof(AddressBuf));
	memcpy(MAC,AddressBuf,sizeof(AddressBuf));
}
//---------------------------------------------------------------------------
// flag =0 ��ʱʹ��IP  =1��ʽʹ��IP
void SetNetwork(int flag,const char *cIp,const char *cMask,const char *cGateWay,const char *cMac)
{
	FILE *fp;
	char shfile[16];

	memset(shfile, 0, sizeof(shfile));
	if(flag == 1) // ��ʽ�ļ�
		strcpy(shfile,"./route.sh");
	else
		strcpy(shfile,"./route_temp.sh");
	fp = fopen(shfile,"wb");
	if(fp)
	{
		fprintf(fp,"#!/bin/sh\n");
		fprintf(fp,"/sbin/ifconfig eth0 down\n");
		if(cIp && cMask)
			fprintf(fp,"/sbin/ifconfig eth0 %s netmask %s\n",cIp,cMask);
		if(cMac)
			fprintf(fp,"/sbin/ifconfig eth0 hw ether %s\n",cMac);
		if(cGateWay)
			fprintf(fp,"/sbin/route add default gw %s dev eth0\n",cGateWay);
		fprintf(fp,"/sbin/ifconfig eth0 up\n");
		fclose(fp);
		fp = NULL;
		excuteCmd(shfile,NULL);
		sync();
	}
	else
		printf("Can't open %s\n", shfile);
	if(fp)
		fclose(fp);
}

/* ----------------------------------------------------------------*/
/**
 * @brief GetFileSize ����ļ���С
 *
 * @param file	�ļ���
 *
 * @returns
 */
/* ----------------------------------------------------------------*/
int GetFileSize(char *file)
{
	struct stat stat_buf;
	stat(file, &stat_buf) ;
	return stat_buf.st_size;
}

time_t MyGetTickCount(void)
{
	return time(NULL)*1000;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief net_detect �����������״̬
 *
 * @param net_name ��������
 *
 * @returns 0���� -1������
 */
/* ---------------------------------------------------------------------------*/
int net_detect(char* net_name)
{
}

/* ----------------------------------------------------------------*/
/**
 * @brief print_data ��ʽ����ӡ����
 *
 * @param data ��������
 * @param len ����
 */
/* ----------------------------------------------------------------*/
void print_data(char *data,int len)
{
	int i;
	for (i = 0; i < len; ++i) {
		if (i) {
			printf("[%02d]0x%02x ",i,data[i] );
			if ((i%5) == 0)
				printf("\n");
		} else {
			printf("\n");
		}
	}
	printf("\n");
}

/* ----------------------------------------------------------------*/
/**
 * @brief GetFilesNum ��ȡ�ļ���Ŀ¼�ļ����������ļ���Ŀ
 *
 * @param pPathDir �ļ���Ŀ¼
 * @param func ÿ���һ���ļ�������
 *
 * @returns �ļ���Ŀ¼�µ��ļ���Ŀ
 */
/* ----------------------------------------------------------------*/
int GetFilesNum(char *pPathDir,void (*func)(void *))
{
	// int i=0 ;
	// DIR *dir = NULL;
    // struct dirent *dirp = NULL;
	// struct _st_dir temp_file; // �����ļ����ṹ��
	// if((dir=opendir(pPathDir)) == NULL) {
		// printf("Open File %s Error %s\n",pPathDir,strerror(errno));
		// return 0;
    // }

	// while((dirp=readdir(dir)) != NULL) {
		// if ((strcmp(".",dirp->d_name) == 0) || (strcmp("..",dirp->d_name) == 0)) {
			// continue;
		// }
		// i++;
		// sprintf(temp_file.path,"%s/%s",pPathDir,dirp->d_name);
		// if (func) {
			// func(&temp_file);
		// }
		// // printf("i:%d,name:%s\n",i,dirp->d_name);
	// }
	// closedir(dir);
	// return i;
}

/* ----------------------------------------------------------------*/
/**
 * @brief ClearFramebuffer ���fb0
 */
/* ----------------------------------------------------------------*/
void ClearFramebuffer(void)
{
	int fd = -1;
	unsigned int VpostWidth=0, VpostHeight=0,VpostBpp=0;
	unsigned int g_fb_vaddress=0;
	unsigned int g_u32VpostBufMapSize=0;
	struct	fb_var_screeninfo g_fb_var;
	fd = open("/dev/fb0", O_RDWR);
	if (fd < 0) {
		perror("/dev/fb0");
		return;
	}
	if (ioctl(fd, FBIOGET_VSCREENINFO, &g_fb_var) < 0) {
		perror("/dev/fb0");
		close(fd);
		return;
	}
	VpostWidth = g_fb_var.xres;
	VpostHeight = g_fb_var.yres;
	VpostBpp = g_fb_var.bits_per_pixel/8;

	g_u32VpostBufMapSize = VpostWidth*VpostHeight*VpostBpp*2;

	g_fb_vaddress = (unsigned int)mmap( NULL, g_u32VpostBufMapSize,
			PROT_READ|PROT_WRITE,
			MAP_SHARED,
			fd,
			0);

	memset ((void*)g_fb_vaddress, 0x0, g_u32VpostBufMapSize );

	if (g_fb_vaddress)
		munmap((void *)g_fb_vaddress, g_u32VpostBufMapSize);

	close(fd);

}
/* ---------------------------------------------------------------------------*/
/**
 * @brief JudgeMonth �жϵ��¶�����
 *
 * @param year ���
 * @param month �·�
 *
 * @returns ����
 */
/* ---------------------------------------------------------------------------*/
int JudgeMonth(int year,int month)
{
	if ((month == 1)
		|| (month == 3)
		|| (month == 5)
		|| (month == 7)
		|| (month == 8)
		|| (month == 10)
		|| (month == 12)) {
		return 31;	//����
	} else if ((month == 4)
		|| (month == 6)
		|| (month == 9)
		|| (month == 11)) {
		return 30;
	} else if (month == 2) {
		if ((year % 4) == 0) {
			return 28;
		} else {
			return 29;
		}
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief hexToChar  16�����ַ���ת��n�����ַ���
 *
 * @param num ��ת����
 * @param d_str ����ַ���
 * @param radix ����(1-52)
 */
/* ---------------------------------------------------------------------------*/
void hexToChar( unsigned long long int num, char* d_str, unsigned int radix)
{
	const char a[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

	char *ptr = d_str;
	if(num == 0) {
		*ptr++ = 0;
		*ptr = '\0';
		return;

	}

	while(num) {
		*ptr++ = a[num % (unsigned long long int)radix];
		num /= (unsigned long long int)radix;

	}

	*ptr = '\0';
	ptr--;
	char *start = d_str;
	while(start < ptr) {
		char tmp = *ptr;
		*ptr = *start;
		*start = tmp;
		ptr--;
		start++;

	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief getDiffSysTick ����32λ��ֵ
 *
 * @param new
 * @param old
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
uint32_t getDiffSysTick(uint64_t new,uint64_t old)
{
    uint32_t diff;
    if (new >= old)
        diff = new - old;
    else
        diff = 0XFFFFFFFF - old + new;
    return diff;
}
