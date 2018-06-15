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
static int gateway_get_property(char *in, char *out, int out_len, void *ctx);
static int gateway_set_property(char *in, void *ctx);

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
enum SERVER_ENV {
    DAILY = 0,
    SANDBOX,
    ONLINE,
    PREPUB
};

#define DPRINT(...)                                      \
do {                                                     \
    printf("\033[1;31;40m%s.%d: ", __func__, __LINE__);  \
    printf(__VA_ARGS__);                                 \
    printf("\033[0m");                                   \
} while (0)

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
static linkkit_cbs_t alink_cbs = {
    .get_property = gateway_get_property,
    .set_property = gateway_set_property,
};

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
    // gateway_t *gw = ctx;

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
            DPRINT("delete subdev %s<%s>\n", productKey, deviceName);
        }
        break;
    case LINKKIT_EVENT_SUBDEV_PERMITED:
        {
            char *productKey = ev->event_data.subdev_permited.productKey;
            int   timeoutSec = ev->event_data.subdev_permited.timeoutSec;
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

    linkkit_gateway_set_event_callback(initParams, event_handler, NULL);

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
    if (linkkit_gateway_init(initParams) < 0) {
        DPRINT("linkkit_gateway_init failed\n");
        return ;
    }
    link_fd = linkkit_gateway_start(&alink_cbs, NULL);
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
#endif
}
int aliSdkRegisterSubDevice(DeviceStr *dev)
{
#if (defined V1)
    int ret = -1;
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
    proto_info.proto_type = ALI_SDK_PROTO_TYPE_ZIGBEE;
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
#endif
}
void aliSdkSubDevReportAttrs(int protocol,char *id,const char *attr_name[],const char *attr_value[])
{
#if (defined V1)
	alink_subdev_report_attrs(protocol,id,attr_name,attr_value);
#else
#endif
}
void aliSdkresetWifi(void)
{
#if (defined V1)
	resetWifi();
#else
#endif
}
