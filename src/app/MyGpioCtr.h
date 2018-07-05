/*
 * =====================================================================================
 *
 *       Filename:  MyGpioCtr.h
 *
 *    Description:  创建GPIO对象
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
#define FLASH_TIME_SLOW_1S 1  // 1次1s
#define FLASH_TIME_FAST_1S 10 // 1次100ms,10次1秒

	typedef enum {
		ENUM_GPIO_ZIGBEE_POWER,	// zigbee 电源
		ENUM_GPIO_WIFI_POWER,	// wifi 电源
		ENUM_GPIO_LED_POWER,	// 电源灯
		ENUM_GPIO_LED_RESET,	// 复位灯
		ENUM_GPIO_LED_WIFI,		// wifi灯
		ENUM_GPIO_LED_NET_IN,	// 入网指示灯

		ENUM_GPIO_RESET,	// 复位按键
		ENUM_GPIO_MODE,	// 激活按键
	}GPIO_TBL;

	typedef enum {//10ms为周期
		FLASH_STOP = 0,
		FLASH_FAST = 1,	//80ms
		FLASH_SLOW = 10,	//1s
	}STRUCT_SPEED;

	enum {
		IO_ACTIVE = 0,	// 有效 (输出)
		IO_INACTIVE,	// 无效 (输出)
		IO_INPUT,		// 输入
		IO_NO_EXIST	//当IO不存在时，用于个别型号不存在该IO口
	};

	struct _MyGpioPriv;

	typedef struct _MyGpio {
		struct _MyGpioPriv *Priv;
		int io_num;		//GPIO数量
		//  设置GPIO闪烁，并执行
		void (*FlashStart)(struct _MyGpio *This,int port,int freq,int times);
		// GPIO停止闪烁
		void (*FlashStop)(struct _MyGpio *This,int port);
		//  GPIO闪烁执行函数，在单独的定时线程中执行
		void (*FlashTimer)(struct _MyGpio *This);
		// GPIO口输出赋值，不立即执行
		void (*SetValue)(struct _MyGpio *This,int port,int Value);
		// GPIO口输出赋值，并立即执行
		void (*SetValueNow)(struct _MyGpio *This,int port,int Value);
		//  读取输入IO值，并判断是否IO有效
		int (*IsActive)(struct _MyGpio *This,int port);
		//  GPIO输出执行函数
		void (*Handle)(struct _MyGpio *This);
	    //  设置输入IO的输入有效电平时间
		void (*setActiveTime)(struct _MyGpio *This,int port,int value);
	    //  设置输入IO的输入有效值
		void (*setActiveValue)(struct _MyGpio *This,int port,int value);
	    //  获取输入IO的输入有效值
		int (*getActiveValue)(struct _MyGpio *This,int port);
	    //  获取输入IO的输入有效电平时间
		int (*getActiveTime)(struct _MyGpio *This,int port);
	    //  检测输入IO电平 1为有效 0为无效
		int (*inputHandle)(struct _MyGpio *This,int port);
		// 创建输出线程
		void (*creatOutputThread)(struct _MyGpio *This);
		// 创建输入线程
		void (*creatInputThread)(struct _MyGpio *This,void *(* checkInputHandle)(void *));

		void (*Destroy)(struct _MyGpio *This);
	}MyGpio;

	typedef struct _MyGpioPriv {
		const char      portid;
		const int       portmask;
		char      *active;		//有效值
		const int 	  default_value;	//默认值
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
