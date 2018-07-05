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
#define FLASH_TIME_SLOW_1S 1  // 1��1s
#define FLASH_TIME_FAST_1S 10 // 1��100ms,10��1��

	typedef enum {
		ENUM_GPIO_ZIGBEE_POWER,	// zigbee ��Դ
		ENUM_GPIO_WIFI_POWER,	// wifi ��Դ
		ENUM_GPIO_LED_POWER,	// ��Դ��
		ENUM_GPIO_LED_RESET,	// ��λ��
		ENUM_GPIO_LED_WIFI,		// wifi��
		ENUM_GPIO_LED_NET_IN,	// ����ָʾ��

		ENUM_GPIO_RESET,	// ��λ����
		ENUM_GPIO_MODE,	// �����
	}GPIO_TBL;

	typedef enum {//10msΪ����
		FLASH_STOP = 0,
		FLASH_FAST = 1,	//80ms
		FLASH_SLOW = 10,	//1s
	}STRUCT_SPEED;

	enum {
		IO_ACTIVE = 0,	// ��Ч (���)
		IO_INACTIVE,	// ��Ч (���)
		IO_INPUT,		// ����
		IO_NO_EXIST	//��IO������ʱ�����ڸ����ͺŲ����ڸ�IO��
	};

	struct _MyGpioPriv;

	typedef struct _MyGpio {
		struct _MyGpioPriv *Priv;
		int io_num;		//GPIO����
		//  ����GPIO��˸����ִ��
		void (*FlashStart)(struct _MyGpio *This,int port,int freq,int times);
		// GPIOֹͣ��˸
		void (*FlashStop)(struct _MyGpio *This,int port);
		//  GPIO��˸ִ�к������ڵ����Ķ�ʱ�߳���ִ��
		void (*FlashTimer)(struct _MyGpio *This);
		// GPIO�������ֵ��������ִ��
		void (*SetValue)(struct _MyGpio *This,int port,int Value);
		// GPIO�������ֵ��������ִ��
		void (*SetValueNow)(struct _MyGpio *This,int port,int Value);
		//  ��ȡ����IOֵ�����ж��Ƿ�IO��Ч
		int (*IsActive)(struct _MyGpio *This,int port);
		//  GPIO���ִ�к���
		void (*Handle)(struct _MyGpio *This);
	    //  ��������IO��������Ч��ƽʱ��
		void (*setActiveTime)(struct _MyGpio *This,int port,int value);
	    //  ��������IO��������Чֵ
		void (*setActiveValue)(struct _MyGpio *This,int port,int value);
	    //  ��ȡ����IO��������Чֵ
		int (*getActiveValue)(struct _MyGpio *This,int port);
	    //  ��ȡ����IO��������Ч��ƽʱ��
		int (*getActiveTime)(struct _MyGpio *This,int port);
	    //  �������IO��ƽ 1Ϊ��Ч 0Ϊ��Ч
		int (*inputHandle)(struct _MyGpio *This,int port);
		// ��������߳�
		void (*creatOutputThread)(struct _MyGpio *This);
		// ���������߳�
		void (*creatInputThread)(struct _MyGpio *This,void *(* checkInputHandle)(void *));

		void (*Destroy)(struct _MyGpio *This);
	}MyGpio;

	typedef struct _MyGpioPriv {
		const char      portid;
		const int       portmask;
		char      *active;		//��Чֵ
		const int 	  default_value;	//Ĭ��ֵ
		int  	active_time;

		int		  current_value;
		int		  portnum;
		int 	  flash_times;
		int 	  flash_set_time;
		int 	  flash_delay_time;
		int 	  flash_even_flag;
		int		  delay_time;
	}MyGpioPriv;

	MyGpio* myGpioPrivCreate(MyGpioPriv *gpio_table);

	extern MyGpio *gpio;
	extern void gpioInit(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
