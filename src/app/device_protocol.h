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
#include "ali_sdk_platform.h"

#define NELEMENTS(array)  \
	        (sizeof (array) / sizeof ((array) [0]))                       

#define SET_PROPERTY_REQ_FMT            "{\"items\":[{\"uuid\":\"%s\",\"properties\":{\"%s\":{\"value\":\"%s\"}}}]}"
#define GET_PROPERTY_REQ_FMT            "{\"items\":[{\"uuid\":\"%s\",\"group\":\"\",\"attrSet\":[\"%s\"]}]}"
#define GET_PROPERTY_RESP_FMT           "{\"items\":[{\"uuid\":\"%s\",\"properties\":{\"%\":{\"value\":\"%s\"}}}]}"

#define MAX_DEVICE_PARA 30   // 每个设备最参数个数

	enum {
		DEVICE_VELUE_TYPE_NUMBER,
		DEVICE_VELUE_TYPE_STRING,
	};
	struct _DeviceStr;
	typedef struct {
		char *name; // 设备类型名称 如灯控为light,调试使用
		uint32_t short_model;  // 设备标识码，由阿里注册产品后生成
		const char *secret;  // 设备密钥，由阿里注册产品后生成
		uint8_t proto_type; // 设备协议类型
		uint8_t device_type; // 设备类型(智能家居协议)
		struct {
			char *name;  // 不同设备自身的参数
			void (*attrcb)(struct _DeviceStr *dev,char *value);
		}attr[MAX_DEVICE_PARA];

		int (*getAttr)(struct _DeviceStr *dev, const char *attr_set[]);
		int (*setAttr)(struct _DeviceStr *dev, const char *attr_name, const char *attr_value);
		int (*execCmd)(struct _DeviceStr *dev, const char *cmd_name, const char *cmd_args);
		int (*remove)(struct _DeviceStr **dev);

		void (*getSwichStatus)(struct _DeviceStr *dev);
		void (*reportPowerOn)(struct _DeviceStr *dev, char *param);
		void (*reportPowerOff)(struct _DeviceStr *dev);

		void (*reportAlarmStatus)(struct _DeviceStr *dev,char *param);
	}DeviceTypePara;

	typedef struct _DeviceStr {
		char id[32];
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
