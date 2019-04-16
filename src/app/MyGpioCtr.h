/*
 * =====================================================================================
 *
 *       Filename:  MyGpioCtr.h
 *
 *    Description:  ����GPIO����
 *
 *        Version:  1.0
 *        Created:  2015-12-24 09:26:29 
 *       Revision:  1.0
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =====================================================================================
 */
#ifndef _MY_GPIO_H
#define _MY_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define FLASH_FOREVER	0x7FFFFFFF

	typedef enum {
		ENUM_GPIO_ZIGBEE_POWER,	// zigbee ��Դ
		ENUM_GPIO_WIFI_POWER,	// wifi ��Դ
		ENUM_GPIO_LED_WIFI,		// ��Դ��(ָʾwifi)
		ENUM_GPIO_LED_RESET,	// ��λ��
		ENUM_GPIO_LED_ONLINE,		// wifi��(ָʾ�Ƿ�����ƽ̨)
		ENUM_GPIO_LED_NET_IN,	// ����ָʾ��
		ENUM_GPIO_LED_POWER,	// ��Դָʾ��

		ENUM_GPIO_RESET,	// ��λ����
		ENUM_GPIO_MODE,	// �����
	}GPIO_TBL;

	typedef enum {//50msΪ����
		FLASH_STOP = 0,
		FLASH_FAST = 4,	//200ms
		FLASH_SLOW = 20,	//1s
	}STRUCT_SPEED;

	enum {
		IO_NO_EXIST = -1,	//��IO������ʱ�����ڸ����ͺŲ����ڸ�IO��
		IO_INPUT = 0,		// ����
		IO_ACTIVE ,		// ��Ч (���)
		IO_INACTIVE,	// ��Ч (���)
	};

	struct GpioArgs{
		struct _MyGpio *gpio;
		int port;
	};

	struct _MyGpioPriv;
	typedef struct _MyGpio {
		struct _MyGpioPriv *priv;
		int io_num;		//GPIO����
		//  ����GPIO��˸����ִ��
		void (*FlashStart)(struct _MyGpio *This,int port,int freq,int times);
		// GPIOֹͣ��˸
		void (*FlashStop)(struct _MyGpio *This,int port);
		//  GPIO��˸ִ�к������ڵ����Ķ�ʱ�߳���ִ��
		void (*FlashTimer)(struct _MyGpio *This);
		// GPIO�������ֵ��������ִ��
		int (*SetValue)(struct _MyGpio *This,int port,int Value);
		// GPIO�������ֵ��������ִ��
		void (*SetValueNow)(struct _MyGpio *This,int port,int Value);
		//  ��ȡ����IOֵ�����ж��Ƿ�IO��Ч
		int (*IsActive)(struct _MyGpio *This,int port);
		//  GPIO���ִ�к���
		void (*Handle)(struct _MyGpio *This);
	    //  ��������IO��������Ч��ƽʱ��
		void (*setActiveTime)(struct _MyGpio *This,int port,int value);
	    //  �������IO��ƽ 1Ϊ��Ч 0Ϊ��Ч
		int (*inputHandle)(struct _MyGpio *This,int port);
		// ��������߳�
		void (*addInputThread)(struct _MyGpio *This,
				struct GpioArgs *args,
			   	void *(* thread)(void *));

		void (*Destroy)(struct _MyGpio *This);
	}MyGpio;

	void gpioInit(void);
	extern MyGpio* gpio;
	extern void gpioInit(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
