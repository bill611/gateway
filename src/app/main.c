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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "alink_export_gateway.h"
#include "alink_export_subdev.h"
#include "platform.h"		/* Header */
#include "iwlib.h"		/* Header */

#define DEVMGR_SERVICE_AUTHORISE_DEVICE_LIST    "AuthoriseDeviceList"
#define DEVMGR_SERVICE_REMOVE_DEVICE            "RemoveDevice"
#define DEVMGR_ATTRIBUTE_JOINED_DEVICE_LIST     "JoinedDeviceList"
#define DEVMGR_SERVICE_PERMITJOIN_DEVICE        "PermitJoin"

#define GW_ATTRIBUTE_SOUND_ALARM                "SoundAlarm"
#define GW_SERVICE_FACTORY_RESET                "FactoryReset"


enum SERVER_ENV {
    DAILY = 0,
    SANDBOX,
    ONLINE,
    PREPUB
};


static char sound_alarm[2] = {'0', '\0'};
extern char *config_get_main_uuid(void);
const char *env_str[] = {"daily", "sandbox", "online"};


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

static int __sound_alarm_attr_get_cb(char *output_buf, unsigned int buf_sz)
{
    printf("get gateway sound_alarm attr, value:%s\n", sound_alarm);
    snprintf(output_buf, buf_sz - 1, sound_alarm);

    return 0;
}

static int __sound_alarm_attr_set_cb(char *value)
{
    printf("set gateway sound_alarm attr, value:%s\n", value);
    if (*value != '0' && *value != '1') {
        printf("invalid sound_alarm attr value:%s\n", value);
        return -1;
    }
    sound_alarm[0] = *value;

    return 0;
}

static int __factory_reset_service_cb(char *args, char *output_buf, unsigned int buf_sz)
{
    printf("exec gateway factory_reset service, args:%s\n", args);
    int ret = 0;
    //1.firmware reset
    //reset config, ....

    //2.alink reset
    ret = alink_factory_reset(ALINK_REBOOT);
    if (ret != 0) {
        printf("call function alink_factory_reset fail!\n");
    }

    return 0;
}


static int __modbus_get_attr_cb(const char *devid, const char *attr_set[])
{
    printf("get attr, devid:%s, attribute name:\n", devid);
    int i = 0;
    while (attr_set[i++]) {
        printf("attr_%d: %s\n", i - 1, attr_set[i - 1]);
    }

    return 0;
}


static int __modbus_set_attr_cb(const char *devid, const char *attr_name, const char *attr_value)
{
    printf("set attr, devid:%s, attrname:%s, attrvalue:%s\n",
           devid, attr_name, attr_value);
    return 0;
}

static int __modbus_exec_cmd_cb(const char *devid, const char *cmd_name, const char *cmd_args)
{
    printf("exec cmd, devid:%s, cmd_name:%s, cmd_args:%s\n",
           devid, cmd_name, cmd_args);
    return 0;
}

static int __modbus_remove_device_cb(const char *devid)
{
    printf("remove device, devid:%s\n",devid);
    return 0;
}


int register_modbus_protocol()
{
    int ret = -1;
    proto_info_t modbus_proto;

    memset(&modbus_proto, 0, sizeof(modbus_proto));
    modbus_proto.proto_type = PROTO_TYPE_MODBUS;
    strncpy(modbus_proto.protocol_name, "modbus", sizeof(modbus_proto.protocol_name) - 1);

    int i = 0;
    modbus_proto.callback[i].cb_type = GET_SUB_DEVICE_ATTR;
    modbus_proto.callback[i].cb_func = __modbus_get_attr_cb;

    i++;
    modbus_proto.callback[i].cb_type = SET_SUB_DEVICE_ATTR;
    modbus_proto.callback[i].cb_func = __modbus_set_attr_cb;

    i++;
    modbus_proto.callback[i].cb_type = EXECUTE_SUB_DEVICE_CMD;
    modbus_proto.callback[i].cb_func = __modbus_exec_cmd_cb;

    i++;
    modbus_proto.callback[i].cb_type = REMOVE_SUB_DEVICE;
    modbus_proto.callback[i].cb_func = __modbus_remove_device_cb;

    ret = alink_subdev_register_device_type(&modbus_proto);

    return ret;
}


int register_gateway_service(void)
{
    //register service
    int ret = alink_register_service(GW_SERVICE_FACTORY_RESET, __factory_reset_service_cb);
    if (0 != ret) {
        printf("register service fail, service:%s\n", GW_SERVICE_FACTORY_RESET);
        return -1;
    }

    return 0;
}

int register_gateway_attribute(void)
{
    int ret = alink_register_attribute(GW_ATTRIBUTE_SOUND_ALARM, __sound_alarm_attr_get_cb, __sound_alarm_attr_set_cb);
    if (0 != ret) {
        printf("register attribute fail, attribute:%s\n", GW_ATTRIBUTE_SOUND_ALARM);
        return -1;
    }

    return 0;

}

char *calc_subdev_signature(const char *secret, const uint8_t rand[SUBDEV_RAND_BYTES],
        char *sign_buff, uint32_t buff_size);
int register_sub_device(void)
{
    int ret = -1;
    unsigned char proto_type = PROTO_TYPE_MODBUS;
    const char *dev_id = "12341241212";

    //device shortmodel
    unsigned short_model = 0x00091940;  //device shortmodel
    //device product secret
    const char *secret = "RlTVYmxb4gUxCMHXAkl2ZItNx7kUQ7TRV2TDBbAO";

    char rand[SUBDEV_RAND_BYTES] = {0};
    char sign[17] = {0};

    if (calc_subdev_signature(secret, rand, sign, sizeof(sign)) == NULL) {
        printf("__get_device_signature fail\n");
        return ret;
    }

    ret = alink_subdev_register_device(proto_type, dev_id, short_model, rand, sign);
    if (ret != 0)
        printf("register sub device fail");

    return ret;
}


#define SET_PROPERTY_REQ_FMT            "{\"items\":[{\"uuid\":\"%s\",\"properties\":{\"%s\":{\"value\":\"%s\"}}}]}"
#define GET_PROPERTY_REQ_FMT            "{\"items\":[{\"uuid\":\"%s\",\"group\":\"\",\"attrSet\":[\"%s\"]}]}"
#define GET_PROPERTY_RESP_FMT           "{\"items\":[{\"uuid\":\"%s\",\"properties\":{\"%\":{\"value\":\"%s\"}}}]}"
#define SET_DEVICE_PROPERTY_METHOD      "setDeviceProperty"
#define GET_DEVICE_PROPERTY_METHOD      "getDeviceProperty"
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


int device_private_property_test(void)
{
    int ret = -1;
    char key[16] = "key_01";
    char value[16] = "value_01";

    ret = set_property_to_cloud(key, value);
    value[0] = '0';
    ret = get_property_from_cloud(key, value, sizeof(key));

    return ret;
}



static unsigned char env = ONLINE;
static char dead_loop = 0;
char loglevel = ALINK_LL_INFO;
extern char *optarg;

int main_loop(void)
{
    int loop = 0;

    //设备私有属性设置和读取
    device_private_property_test();

    //注册子设备
    //register_sub_device();

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


int main(int argc, char *argv[])
{
#if 0
	int ap_cnt = 0;
	TcWifiScan *ap_info[100] ;
	char *cmd[] = { "gw", "wlan0", "scan", };
	iwlist(3,cmd,&ap_info,&ap_cnt);
	int i;
	for (i=0; i<ap_cnt; i++) {
		printf("[%d]ssid:%s\n",i, ap_info[i]->ssid); 
		printf("mac:%02X:%02X:%02X:%02X:%02X:%02X",
				ap_info[i]->bssid[0],
				ap_info[i]->bssid[1],
				ap_info[i]->bssid[2],
				ap_info[i]->bssid[3],
				ap_info[i]->bssid[4],
				ap_info[i]->bssid[5]); 
		printf(";channel:%d", ap_info[i]->channel); 
		printf(";rssid:%d", ap_info[i]->rssi); 
		printf(";auth:%d", ap_info[i]->auth); 
		printf(";encry:%d\n", ap_info[i]->encry); 
		if (ap_info[i]) free(ap_info[i]);
	}
	return 0;
#endif
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

    register_gateway_service();
    register_gateway_attribute();

    //设置设备认证模式:DEFAULT(阿里智能),SDS+DEVICEID,SDS_WHITELIST
    alink_set_auth_mode(ALINK_AUTH_MODE_DEFAULT);

    alink_start();
    register_modbus_protocol();
    alink_wait_connect(ALINK_WAIT_FOREVER);

loop:
    main_loop();

    if (dead_loop) {
        goto loop;
    }

    //暂屏蔽复位代码，避免测试中误重启
    //alink_factory_reset(ALINK_REBOOT);

    alink_end();
}
