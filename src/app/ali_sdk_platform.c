/*
 * Copyright (c) 2014-2016 Alibaba Group. All rights reserved.
 *
 * Alibaba Group retains all right, title and interest (including all
 * intellectual property rights) in and to this computer program, which is
 * protected by applicable intellectual property laws.  Unless you have
 * obtained a separate written license from Alibaba Group., you are not
 * authorized to utilize all or a part of this computer program for any
 * purpose (including reproduction, distribution, modification, and
 * compilation into object code), and you must immediately destroy or
 * return to Alibaba Group all copies of this computer program.  If you
 * are licensed by Alibaba Group, your rights to utilize this computer
 * program are limited by the terms of that license.  To obtain a license,
 * please contact Alibaba Group.
 *
 * This computer program contains trade secrets owned by Alibaba Group.
 * and, unless unauthorized by Alibaba Group in writing, you agree to
 * maintain the confidentiality of this computer program and related
 * information and to not disclose this computer program and related
 * information to any other person or entity.
 *
 * THIS COMPUTER PROGRAM IS PROVIDED AS IS WITHOUT ANY WARRANTIES, AND
 * Alibaba Group EXPRESSLY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED,
 * INCLUDING THE WARRANTIES OF MERCHANTIBILITY, FITNESS FOR A PARTICULAR
 * PURPOSE, TITLE, AND NONINFRINGEMENT.
 */

/* ---------------------------------------------------------------------------*
 *                      include head files
 *----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "debug.h"
#include "sql_handle.h"
#include "ali_sdk_platform.h"
#include "device_protocol.h"
#include "iwlib.h"		/* Header */


/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/
#if (defined V1)
extern char *optarg;
#endif

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
enum SERVER_ENV {
    DAILY = 0,
    SANDBOX,
    ONLINE,
    PREPUB
};

#if (defined V1)
#define DPRINT(...)                                      \
do {                                                     \
    printf("\033[1;35;40m%s.%d: ", __func__, __LINE__);  \
    printf(__VA_ARGS__);                                 \
    printf("\033[0m");                                   \
} while (0)
#else
#define DPRINT(...)                                      \
do {                                                     \
    printf("\033[1;33;40m%s.%d: ", __func__, __LINE__);  \
    printf(__VA_ARGS__);                                 \
    printf("\033[0m");                                   \
} while (0)
#endif

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static int (*service_func)(char *args, char *output_buf, unsigned int buf_sz);
static char *service_name;
#if (defined V1)
const char *env_str[] = {"daily", "sandbox", "online"};
static unsigned char env = ONLINE;
static char loglevel = ALINK_LL_INFO;
#endif
#if (defined V2)
static linkkit_params_t *initParams;
static int link_fd;
static GateWayAttr *gw_attr = NULL;
static GateWayPrivateAttr *gw_priv_attrs = NULL;
static int gateway_get_property(char *in, char *out, int out_len, void *ctx)
{

}
static int gateway_set_property(char *in, void *ctx)
{

}
static int gateway_call_service(char *identifier, char *in, char *out, int out_len, void *ctx)
{

    if (strcmp(identifier, service_name) == 0) {
		service_func(ctx,out,out_len);
    }
}

static linkkit_cbs_t alink_cbs = {
    .get_property = gateway_get_property,
    .set_property = gateway_set_property,
};

static void ota_callback(int event, const char *version, void *ctx)
{
    DPRINT("event: %d\n", event);
    DPRINT("version: %s\n", version);

    linkkit_gateway_ota_update(512);
}
#endif

#if (defined V1)
void cloud_connected(void)
{
    printf("alink cloud connected!\n");
    const char *arrt_set[1] = {NULL};
    alink_report_attrs(arrt_set);
}

void cloud_disconnected(void)
{
    printf("alink cloud disconnected!\n");
}

void cloud_get_device_status(void)
{
    printf("cloud_get_device_status!\n");
}

static void * testUartSendThead(void *arg)
{
	int ret;
	while (1) {
		ret = aws_notify_app_nonblock();
		printf("ret :%d\n", ret);
		sleep(1);
	}
}

static void testUartSend(void)
{
	pthread_t id;
	pthread_attr_t  attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
	pthread_create(&id,&attr,(void *)testUartSendThead,NULL);
	pthread_attr_destroy(&attr);
}
#endif


void usage(void)
{
    printf("\nalink_sample -e enviroment -l log_level\n");
    printf("\t -e alink server environment, 'daily', 'sandbox' or 'online'(default)\n");
    printf("\t -d dead loop, never return\n");
    printf("\t -l log level, trace/debug/info/warn/error/fatal/none\n");
    printf("\t -h show help text\n");
}


void parse_opt(int argc, char *argv[])
{
#if (defined V1)
    int ch;
    while ((ch = getopt(argc, argv, "e:d::l:h")) != -1) {
        switch ((char)ch) {
            case 'e':
                if (!strcmp(optarg, "daily")) {
                    env = DAILY;
                } else if (!strcmp(optarg, "sandbox")) {
                    env = SANDBOX;
                } else if (!strcmp(optarg, "online")) {
                    env = ONLINE;
                } else if((!strcmp(optarg, "prepub"))){
					env = PREPUB;
				}
				else {
                    env = ONLINE;
                    printf("unknow opt %s, use default env\n", optarg);
                }
                break;
            case 'l':
                if (!strcmp(optarg, "trace")) {
                    loglevel = ALINK_LL_TRACE;
                } else if (!strcmp(optarg, "debug")) {
                    loglevel = ALINK_LL_DEBUG;
                } else if (!strcmp(optarg, "info")) {
                    loglevel = ALINK_LL_INFO;
                } else if (!strcmp(optarg, "warn")) {
                    loglevel = ALINK_LL_WARN;
                } else if (!strcmp(optarg, "error")) {
                    loglevel = ALINK_LL_ERROR;
                } else if (!strcmp(optarg, "fatal")) {
                    loglevel = ALINK_LL_FATAL;
                } else if (!strcmp(optarg, "none")) {
                    loglevel = ALINK_LL_NONE;
                } else {
                    loglevel = ALINK_LL_INFO;
                }
                break;
            case 'h':
            default:
                usage();
                exit(0);
        }
    }

    printf("alink server: %s,  log level: %d\n",
           env_str[env],  loglevel);
#else
#endif
}

#if (defined V2)
static int event_handler(linkkit_event_t *ev, void *ctx)
{
	GateWayAttr  *gw = ctx;

    switch (ev->event_type) {
    case LINKKIT_EVENT_CLOUD_CONNECTED:
        DPRINT("cloud connected\n");

        // post_all_properties(gw);    [> sync to cloud <]
        // gw->connected = 1;

        break;
    case LINKKIT_EVENT_CLOUD_DISCONNECTED:
        // gw->connected = 0;
        DPRINT("cloud disconnected\n");
        break;
    case LINKKIT_EVENT_SUBDEV_DELETED:
        {
            char *productKey = ev->event_data.subdev_deleted.productKey;
            char *deviceName = ev->event_data.subdev_deleted.deviceName;
			gw->removeDeviceCb(deviceName);
            DPRINT("delete subdev %s<%s>\n", productKey, deviceName);
        }
        break;
    case LINKKIT_EVENT_SUBDEV_PERMITED:
        {
            char *productKey = ev->event_data.subdev_permited.productKey;
            int   timeoutSec = ev->event_data.subdev_permited.timeoutSec;

			gw->permitSubDeviceJoinCb(timeoutSec);
            DPRINT("permit subdev %s in %d seconds\n", productKey, timeoutSec);
        }
        break;
    }

    return 0;
}
#endif
void aliSdkInit(int argc, char *argv[])
{
#if (defined V1)
    parse_opt(argc, argv);
	alink_set_loglevel(loglevel);
    if (env == SANDBOX) {
        alink_enable_sandbox_mode();
    } else if (env == DAILY) {
        alink_enable_daily_mode(NULL, 0);
    } else if (env == PREPUB){
		alink_enable_prepub_mode();
	}
    alink_register_callback(ALINK_CLOUD_CONNECTED, &cloud_connected);
    alink_register_callback(ALINK_CLOUD_DISCONNECTED, &cloud_disconnected);

    alink_register_callback(3, &cloud_get_device_status);

#else
	initParams = linkkit_gateway_get_default_params();
    int maxMsgSize = 20 * 1024;
    linkkit_gateway_set_option(initParams, LINKKIT_OPT_MAX_MSG_SIZE, &maxMsgSize, sizeof(int));
    int maxMsgQueueSize = 8;
    linkkit_gateway_set_option(initParams, LINKKIT_OPT_MAX_MSG_QUEUE_SIZE, &maxMsgQueueSize, sizeof(int));
    int loglevel = 5;
    linkkit_gateway_set_option(initParams, LINKKIT_OPT_LOG_LEVEL, &loglevel, sizeof(int));


#endif
}
void aliSdkStart(void)
{
#if (defined V1)
    //设置设备认证模式:DEFAULT(阿里智能),SDS+DEVICEID,SDS_WHITELIST
    alink_set_auth_mode(ALINK_AUTH_MODE_DEFAULT);
    alink_start();
    alink_wait_connect(ALINK_WAIT_FOREVER);
#else
    linkkit_gateway_set_event_callback(initParams, event_handler, gw_attr);
    if (linkkit_gateway_init(initParams) < 0) {
        DPRINT("linkkit_gateway_init failed\n");
        return ;
    }
    link_fd = linkkit_gateway_start(&alink_cbs, gw_priv_attrs);
    if (link_fd < 0) {
        DPRINT("linkkit_gateway_start failed\n");
        return ;
    }
    linkkit_gateway_ota_init(ota_callback, NULL);
#endif
}
void aliSdkEnd(void)
{
#if (defined V1)
    alink_end();
#else
    linkkit_gateway_stop(link_fd);
    linkkit_gateway_exit();
#endif
}
int aliSdkReset(int is_reboot)
{
#if (defined V1)
	if (is_reboot)
		return	alink_factory_reset(ALINK_REBOOT);
	else
		return 	alink_factory_reset(ALINK_NOT_REBOOT);
#else
	linkkit_gateway_reset(link_fd);
#endif
}

int aliSdkRegistGwService(char *name, void *func)
{
	service_name = name;
	service_func = (int (*)(char *, char *, unsigned int ))func;
#if (defined V1)
    int ret = alink_register_service(name, service_func);
    if (0 != ret) {
        printf("register service fail, service:%s\n", name);
        return -1;
    }
#else
    alink_cbs.call_service = gateway_call_service;
	return 0;
#endif
}
#if (defined V2)
static int subDevGetProperty(char *in, char *out, int out_len, void *ctx)
{
	printf("[%s]in:%s,out:%s,out_len:%d\n",
			__FUNCTION__,in,out,out_len);
	DeviceStr *dev = ctx;
	cJSON *rJson = cJSON_Parse(in);
	if (!rJson)
		return -1;

	int iSize = cJSON_GetArraySize(rJson);
	if (iSize <= 0) {
		cJSON_Delete(rJson);
		return -1;
	}
	cJSON *pJson = cJSON_CreateObject();
	if (!pJson) {
		cJSON_Delete(rJson);
		return -1;
	}
	int i,j;
	for (i = 0; i < iSize; i++) {
		cJSON *pSub = cJSON_GetArrayItem(rJson, i);
		for (j=0; dev->type_para->attr[j].name != NULL; j++) {
			if (strcmp(pSub->valuestring,dev->type_para->attr[j].name) == 0) {
				switch(dev->type_para->attr[j].value_type)
				{
					case DEVICE_VELUE_TYPE_INT:
						cJSON_AddNumberToObject(pJson, 
								dev->type_para->attr[j].name, atoi(dev->value[j]));
						break;
					case DEVICE_VELUE_TYPE_DOUBLE:
						cJSON_AddNumberToObject(pJson, 
								dev->type_para->attr[j].name, atof(dev->value[j]));
						break;
					case DEVICE_VELUE_TYPE_STRING:
						cJSON_AddStringToObject(pJson, 
								dev->type_para->attr[j].name, dev->value[j]);
						break;
					default:
						break;
				}
				break;
			}
		}
	}
	char *p = cJSON_PrintUnformatted(pJson);
	if (!p) {
		cJSON_Delete(rJson);
		cJSON_Delete(pJson);
		return -1;
	}

	if (strlen(p) >= out_len) {
		cJSON_Delete(rJson);
		cJSON_Delete(pJson);
		free(p);
		return -1;
	}

	strcpy(out, p);

	printf("out: %s\n", out);

	cJSON_Delete(rJson);
	cJSON_Delete(pJson);
	free(p);
	return 0;
}
static int subDevSetProperty(char *in, void *ctx)
{
	printf("[%s]in:%s\n", __FUNCTION__, in);
	DeviceStr *dev = ctx;
	cJSON *rJson = cJSON_Parse(in);
	if (!rJson)
		return -1;
	int i = 0;
	for (i=0; dev->type_para->attr[i].name != NULL; i++) {
		cJSON *json_value = cJSON_GetObjectItem(rJson, dev->type_para->attr[i].name);
		if (json_value) {
			switch(dev->type_para->attr[i].value_type)
			{
				case DEVICE_VELUE_TYPE_INT:
					sprintf(dev->value[i],"%d",json_value->valueint);
					break;
				case DEVICE_VELUE_TYPE_DOUBLE:
					sprintf(dev->value[i],"%f",json_value->valuedouble);
					break;
				case DEVICE_VELUE_TYPE_STRING:
					sprintf(dev->value[i],"%s",json_value->valuestring);
					break;
				default:
					break;
			}
			printf("[%s,%s]%s:%s\n",__FUNCTION__,__FILE__,
					dev->type_para->attr[i].name,dev->value[i]);
			if (dev->type_para->attr[i].attrcb)
				dev->type_para->attr[i].attrcb(dev,dev->value[i]);
		}
	}
	cJSON_Delete(rJson);
	linkkit_gateway_post_property_json_sync(dev->devid, in, 10000);
	return 0;
}
static int subDevService(char *identifier, char *in, char *out, int out_len, void *ctx) 
{
	printf("[%s]iden:%s,in:%s\n", __FUNCTION__,identifier, in);
	DeviceStr *dev = ctx;
	int i = 0;
	// for (i=0; dev->type_para->attr[i].name != NULL; i++) {
		// if (strcmp(identifier,dev->type_para->attr[i].name) == 0) {
			// switch(dev->type_para->attr[i].value_type)
			// {
				// case DEVICE_VELUE_TYPE_INT:
					// sprintf(dev->value[i],"%d",json_value->valueint);
					// break;
				// case DEVICE_VELUE_TYPE_DOUBLE:
					// sprintf(dev->value[i],"%f",json_value->valuedouble);
					// break;
				// case DEVICE_VELUE_TYPE_STRING:
					// sprintf(dev->value[i],"%s",json_value->valuestring);
					// break;
				// default:
					// break;
			// }
			// printf("[%s,%s]%s:%s\n",__FUNCTION__,__FILE__,
					// dev->type_para->attr[i].name,dev->value[i]);
			// if (dev->type_para->attr[i].attrcb)
				// dev->type_para->attr[i].attrcb(dev,dev->value[i]);
		// }
	// }
	linkkit_gateway_post_property_json_sync(dev->devid, in, 5000);	
}
#endif
int aliSdkRegisterSubDevice(DeviceStr *dev)
{
    int ret = -1;
#if (defined V1)
	char rand[SUBDEV_RAND_BYTES] = {0};
	char sign[17] = {0};
	if (calc_subdev_signature(dev->type_para->secret,
			   	rand, sign, sizeof(sign)) == NULL) {
		printf("[%s:%s]__get_device_signature fail\n",
				dev->id, dev->type_para->name);
	}
	ret = alink_subdev_register_device(
			dev->type_para->proto_type,
			dev->id,
			dev->type_para->short_model,
			rand, sign);
#else
	static linkkit_cbs_t cbs = {
		.get_property = subDevGetProperty,
		.set_property = subDevSetProperty,
		.call_service = subDevService,
	};
	// linkkit_gateway_subdev_unregister(dev->type_para->product_key, dev->id);
	// return -1;
	if (linkkit_gateway_subdev_register(dev->type_para->product_key,
			   	dev->id, dev->type_para->device_secret) < 0) {
		DPRINT("subdev regist fail:%d\n", dev->devid);
		// return -1;
	}
	dev->devid = linkkit_gateway_subdev_create(dev->type_para->product_key,
		   	dev->id, &cbs, dev);
	DPRINT("dev_id:%d\n", dev->devid);
	if (dev->devid < 0) {
		DPRINT("linkkit_gateway_subdev_create %s<%s> failed\n", dev->id, dev->type_para->product_key);
		linkkit_gateway_subdev_unregister(dev->type_para->product_key, dev->id);
		return -1;
	}
	if (linkkit_gateway_subdev_login(dev->devid) < 0) {
		DPRINT("linkkit_gateway_subdev_login %s<%s> failed\n", dev->id, dev->type_para->product_key);
		linkkit_gateway_subdev_destroy(dev->devid);
		linkkit_gateway_subdev_unregister(dev->type_para->product_key, dev->id);
		return -1;
	}
	ret = 0;
#endif
	return ret;
}
int aliSdkUnRegisterSubDevice(DeviceStr *dev)
{
#if (defined V2)
	linkkit_gateway_subdev_logout(dev->devid);
	linkkit_gateway_subdev_destroy(dev->devid);
	linkkit_gateway_subdev_unregister(dev->type_para->product_key, dev->id);

	DPRINT("linkkit_reset %d\n", dev->devid);
	// linkkit_gateway_reset(dev->devid);
#endif
}

int aliSdkRegisterAttribute(GateWayPrivateAttr *attrs)
{
#if (defined V1)
	int i;
	for (i=0; attrs[i].attr != NULL; i++) {
		int ret = alink_register_attribute(attrs[i].attr,
				attrs[i].getCb, attrs[i].setCb);
		if (0 != ret) {
			printf("register attribute fail, attribute:%s\n", attrs[i].attr);
		}
	}

#else
	gw_priv_attrs = attrs;
#endif
}
int aliSdkRegisterGw(char *value)
{
#if (defined V1)
    int ret = -1;
    ret = alink_report("postDeviceData",value);
    if (ret != 0)
        printf("report msg fail, params: %s\n",value);
#else
#endif
}
void aliSdkRegistGwAttr(char *proto_name,int proto_type,GateWayAttr *attr)
{
#if (defined V1)
	proto_info_t proto_info;
    memset(&proto_info, 0, sizeof(proto_info));
    proto_info.proto_type = proto_type;
    strncpy(proto_info.protocol_name, proto_name, sizeof(proto_info.protocol_name) - 1);

    int i = 0;
    proto_info.callback[i].cb_type = GET_SUB_DEVICE_ATTR;
    proto_info.callback[i].cb_func = attr->getCb;

    i++;
    proto_info.callback[i].cb_type = SET_SUB_DEVICE_ATTR;
    proto_info.callback[i].cb_func = attr->setCb;

    i++;
    proto_info.callback[i].cb_type = EXECUTE_SUB_DEVICE_CMD;
    proto_info.callback[i].cb_func = attr->execCmdCb;

    i++;
    proto_info.callback[i].cb_type = REMOVE_SUB_DEVICE;
    proto_info.callback[i].cb_func = attr->removeDeviceCb;

    i++;
    proto_info.callback[i].cb_type = PERMIT_JOIN_SUB_DEVICE;
    proto_info.callback[i].cb_func = attr->permitSubDeviceJoinCb;

	int	ret = alink_subdev_register_device_type(&proto_info);

	if (ret != 0)
		printf("register sub device type fail");
#else
	gw_attr = attr;
#endif
}
void aliSdkSubDevReportAttrs(DeviceStr *dev,
		const char *attr_name[],
		const char *attr_value[],
		int attr_value_type[])
{
#if (defined V1)
	alink_subdev_report_attrs(dev->type_para->proto_type,dev->id,attr_name,attr_value);
#else
	cJSON *pJson = cJSON_CreateObject();
	if (!pJson) {
		return ;
	}
	int i;
	for (i = 0; attr_name[i] != NULL; i++) {
		switch(attr_value_type[i])
		{
			case DEVICE_VELUE_TYPE_INT:
				cJSON_AddNumberToObject(pJson, 
						attr_name[i], atoi(attr_value[i]));
				break;
			case DEVICE_VELUE_TYPE_DOUBLE:
				cJSON_AddNumberToObject(pJson, 
						attr_name[i], atof(attr_value[i]));
				break;
			case DEVICE_VELUE_TYPE_STRING:
				cJSON_AddStringToObject(pJson, 
						attr_name[i], attr_value[i]);
				break;
			default:
				break;
		}
	}
	char *p = cJSON_PrintUnformatted(pJson);
	if (!p) {
		cJSON_Delete(pJson);
		return ;
	}
	printf("out: %s\n", p);
	linkkit_gateway_post_property_json_sync(dev->devid, p, 5000);	
	cJSON_Delete(pJson);
	free(p);
#endif
}
void aliSdkresetWifi(void)
{
	resetWifi();
}
