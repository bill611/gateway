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
#include "alink_export_gateway.h"
#include "alink_export_subdev.h"
#include "device_protocol.h"
#include "platform.h"		/* Header */
#include "iwlib.h"		/* Header */


/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/
extern char *config_get_main_uuid(void);
extern char *optarg;

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
#define SET_DEVICE_PROPERTY_METHOD      "setDeviceProperty"
#define GET_DEVICE_PROPERTY_METHOD      "getDeviceProperty"

enum SERVER_ENV {
    DAILY = 0,
    SANDBOX,
    ONLINE,
    PREPUB
};

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
const char *env_str[] = {"daily", "sandbox", "online"};
static unsigned char env = ONLINE;
static char dead_loop = 0;
static char loglevel = ALINK_LL_INFO;


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

/*设备私有属性设置*/
int set_property_to_cloud(const char *key, const char *value)
{
    int ret = -1;
    char req_params[256] = {0};
    char uuid[33] = {0};

    ret = alink_get_main_uuid(uuid, sizeof(uuid));//or alink_subdev_get_uuid
    if (ret != 0)
        return ret;

    snprintf(req_params, sizeof(req_params) - 1, SET_PROPERTY_REQ_FMT, uuid, key, value);
    printf("request params: %s\n", req_params);
    ret = alink_report(SET_DEVICE_PROPERTY_METHOD, req_params);
    if (ret != 0)
        printf("report msg fail, params: %s\n", req_params);

    return ret;
}


/*设备私有属性读取*/
int get_property_from_cloud(const char *key, char *value_buf, int buf_sz)
{
    int ret = -1;
    char req_params[256] = {0};
    char resp_params[256] = {0};
    char uuid[33] = {0};
    int params_sz = sizeof(resp_params);

    ret = alink_get_main_uuid(uuid, sizeof(uuid));
    if (ret != 0)
        return ret;

    snprintf(req_params, sizeof(req_params) - 1, GET_PROPERTY_REQ_FMT, uuid, key);
    printf("request params: %s\n", req_params);
    ret = alink_query(GET_DEVICE_PROPERTY_METHOD, req_params, resp_params, &params_sz);
    if (ret != 0) {
        printf("query msg fail, params: %s\n", req_params);
        return ret;
    }

    printf("============response data: %s\n", resp_params);

    /*parser resp_params*/
    //...

    return ret;
}



int main_loop(void)
{
    int loop = 0;

    //设备私有属性设置和读取
    device_private_property_test();

    //注册子设备
	// register_sub_device();

    while (loop++ < 2) {
        sleep(5);
    }

    return 0;
}


void usage(void)
{
    printf("\nalink_sample -e enviroment -d enable_dead_loop -l log_level\n");
    printf("\t -e alink server environment, 'daily', 'sandbox' or 'online'(default)\n");
    printf("\t -d dead loop, never return\n");
    printf("\t -l log level, trace/debug/info/warn/error/fatal/none\n");
    printf("\t -h show help text\n");
}


void parse_opt(int argc, char *argv[])
{
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
            case 'd':
                dead_loop = 1;
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

    printf("alink server: %s, dead_loop: %s, log level: %d\n",
           env_str[env], dead_loop ? "true" : "false", loglevel);
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

int main(int argc, char *argv[])
{
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

    register_gateway_service();
    register_gateway_attribute();

    //设置设备认证模式:DEFAULT(阿里智能),SDS+DEVICEID,SDS_WHITELIST
    alink_set_auth_mode(ALINK_AUTH_MODE_DEFAULT);
	// awss_start();
    alink_start();
	// awss_end();
    alink_wait_connect(ALINK_WAIT_FOREVER);
	deviceInit();
	zigbeeInit();
	smarthomeInit();
	register_sub_device();
	// testUartSend();
loop:
    main_loop();

    if (dead_loop) {
        goto loop;
    }

    //暂屏蔽复位代码，避免测试中误重启
    //alink_factory_reset(ALINK_REBOOT);

    alink_end();
}
