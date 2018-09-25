/*
 * =============================================================================
 *
 *       Filename:  ali_protoco.h
 *
 *    Description:  阿里相关json接口
 *
 *        Version:  virsion
 *        Created:  2018-05-08 16:39:45 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _ALI_PROTOOCL_H
#define _ALI_PROTOOCL_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include <stdint.h>
#include "debug.h"
#include "ali_sdk_platform.h"
#include "cJSON.h"

#define NELEMENTS(array)  \
	        (sizeof (array) / sizeof ((array) [0]))                       

#define SET_PROPERTY_REQ_FMT            "{\"items\":[{\"uuid\":\"%s\",\"properties\":{\"%s\":{\"value\":\"%s\"}}}]}"
#define GET_PROPERTY_REQ_FMT            "{\"items\":[{\"uuid\":\"%s\",\"group\":\"\",\"attrSet\":[\"%s\"]}]}"
#define GET_PROPERTY_RESP_FMT           "{\"items\":[{\"uuid\":\"%s\",\"properties\":{\"%\":{\"value\":\"%s\"}}}]}"

#define MAX_DEVICE_PARA 30   // 每个设备最参数个数

	enum {
		DEVICE_VELUE_TYPE_INT,
		DEVICE_VELUE_TYPE_DOUBLE,
		DEVICE_VELUE_TYPE_STRING,
	};
	struct _DeviceStr;
	typedef struct {
		char *name; // 设备类型名称 如灯控为light,调试使用

		// 1.0平台使用
		uint32_t short_model;  // 设备标识码，由阿里注册产品后生成
		const char *secret;  // 设备密钥，由阿里注册产品后生成
		// 2.0平台使用
		char *product_key;  //  产品密钥，由阿里注册产品后生成
		char *device_secret;  //  设备密钥，由阿里注册产品后生成

		uint8_t proto_type; // 设备协议类型
		uint8_t device_type; // 设备类型(智能家居协议)
		struct {
			char *name;  // 不同设备自身的属性参数
			void (*attrcb)(struct _DeviceStr *dev,char *value);
			int value_type;
			void (*attrMultitermcb)(struct _DeviceStr *dev,char *value,const char *attr_name);
		}attr[MAX_DEVICE_PARA];

		char *event[MAX_DEVICE_PARA];

		int (*getAttr)(struct _DeviceStr *dev, const char *attr_set[]); // 通用获取属性
		int (*setAttr)(struct _DeviceStr *dev, const char *attr_name, const char *attr_value); // 通用设置属性

		int (*checkAttrs)(struct _DeviceStr *dev); // 主动查询设备属性

		void (*getSwichStatus)(struct _DeviceStr *dev);
		void (*getAirPara)(struct _DeviceStr *dev);
		void (*reportPowerOn)(struct _DeviceStr *dev, char *param,int channel); // 上报电源打开
		void (*reportPowerOff)(struct _DeviceStr *dev,int channel); // 上报电源关闭

		void (*reportAlarmStatus)(struct _DeviceStr *dev,char *param); // 上报报警状态

		void (*reportEleQuantity)(struct _DeviceStr *dev,char *param); // 上报电量
		void (*reportElePower)(struct _DeviceStr *dev,char *param);  // 上报功率

		void (*reportAirPara)(struct _DeviceStr *dev,char *param);  // 上报空气质量

		void (*reportArmStatus)(struct _DeviceStr *dev,char *param);  // 上报布防状态
	}DeviceTypePara;

	typedef struct _DeviceStr {
		char id[32];
		int devid;  // 2.0 平台使用，每个设备创建后的id
		uint16_t addr;
		uint16_t channel;
		char *value[MAX_DEVICE_PARA];
		DeviceTypePara *type_para;
	}DeviceStr;

	typedef struct _GateWayPrivateAttr{
		char *attr;	
		int (*getCb)(char *output_buf, unsigned int buf_sz);
		int (*setCb)(char *value);
		int value_type;
		char value[32];	
	}GateWayPrivateAttr;

	typedef struct _GateWayAttr{
		int (*getCb)(const char *devid, const char *attr_set[]);
		int (*setCb)(const char *devid, const char *attr_name, const char *attr_value);
		int (*execCmdCb)(const char *devid, const char *cmd_name, const char *cmd_args);
		int (*removeDeviceCb)(const char *devid);
		int (*permitSubDeviceJoinCb)(uint8_t duration);
	}GateWayAttr;

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
