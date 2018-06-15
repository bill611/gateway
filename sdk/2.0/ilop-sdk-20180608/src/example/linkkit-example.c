#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "light.h"
#include "linkkit.h"
#include "iot_import.h"

#include "cJSON.h"

#define DPRINT(...)                                      \
do {                                                     \
    printf("\033[1;31;40m%s.%d: ", __func__, __LINE__);  \
    printf(__VA_ARGS__);                                 \
    printf("\033[0m");                                   \
} while (0)

typedef struct {
    int  ZB_Band;
    int  ZB_Channel;

    char ZB_CO_MAC[32];
    char ZB_PAN_ID[32];
    char EXT_PAN_ID[32];
    char NETWORK_KEY[32];

    int connected;

    int lk_dev;
} gateway_t;

static int gateway_get_property(char *in, char *out, int out_len, void *ctx)
{
    DPRINT("in: %s\n", in);

    gateway_t *gw = ctx;
    if (!gw) {
        DPRINT("gateway not found\n");
        return -1;
    }

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

    int i;
    for (i = 0; i < iSize; i++) {
        cJSON *pSub = cJSON_GetArrayItem(rJson, i);

        if (strcmp(pSub->valuestring, "ZB_Band") == 0) {
            cJSON_AddNumberToObject(pJson, "ZB_Band", gw->ZB_Band);
        } else if (strcmp(pSub->valuestring, "ZB_Channel") == 0) {
            cJSON_AddNumberToObject(pJson, "ZB_Channel", gw->ZB_Channel);
        } else if (strcmp(pSub->valuestring, "ZB_CO_MAC") == 0) {
            cJSON_AddStringToObject(pJson, "ZB_CO_MAC", gw->ZB_CO_MAC);
        } else if (strcmp(pSub->valuestring, "ZB_PAN_ID") == 0) {
            cJSON_AddStringToObject(pJson, "ZB_PAN_ID", gw->ZB_PAN_ID);
        } else if (strcmp(pSub->valuestring, "EXT_PAN_ID") == 0) {
            cJSON_AddStringToObject(pJson, "EXT_PAN_ID", gw->EXT_PAN_ID);
        } else if (strcmp(pSub->valuestring, "NETWORK_KEY") == 0) {
            cJSON_AddStringToObject(pJson, "NETWORK_KEY", gw->NETWORK_KEY);
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

    DPRINT("out: %s\n", out);

    cJSON_Delete(rJson);
    cJSON_Delete(pJson);
    free(p);

    return 0;
}

static int gateway_set_property(char *in, void *ctx)
{
    gateway_t *gw = ctx;

    DPRINT("in: %s\n", in);

    cJSON *rJson = cJSON_Parse(in);
    if (!rJson)
        return -1;

    cJSON *ZB_Band = cJSON_GetObjectItem(rJson, "ZB_Band");
    if (ZB_Band)
        gw->ZB_Band = ZB_Band->valueint;

    cJSON *ZB_Channel = cJSON_GetObjectItem(rJson, "ZB_Channel");
    if (ZB_Channel)
        gw->ZB_Channel = ZB_Channel->valueint;

    cJSON *ZB_PAN_ID = cJSON_GetObjectItem(rJson, "ZB_PAN_ID");
    if (ZB_PAN_ID)
        strncpy(gw->ZB_PAN_ID, ZB_PAN_ID->valuestring, sizeof(gw->ZB_PAN_ID) - 1);

    cJSON *EXT_PAN_ID = cJSON_GetObjectItem(rJson, "EXT_PAN_ID");
    if (EXT_PAN_ID)
        strncpy(gw->EXT_PAN_ID, EXT_PAN_ID->valuestring, sizeof(gw->EXT_PAN_ID) - 1);

    cJSON *ZB_CO_MAC = cJSON_GetObjectItem(rJson, "ZB_CO_MAC");
    if (ZB_CO_MAC)
        strncpy(gw->ZB_CO_MAC, ZB_CO_MAC->valuestring, sizeof(gw->ZB_CO_MAC) - 1);

    cJSON *NETWORK_KEY = cJSON_GetObjectItem(rJson, "NETWORK_KEY");
    if (NETWORK_KEY)
        strncpy(gw->NETWORK_KEY, NETWORK_KEY->valuestring, sizeof(gw->NETWORK_KEY) - 1);

    linkkit_gateway_post_property_json_sync(gw->lk_dev, in, 5000);
    cJSON_Delete(rJson);

    return 0;
}

static int gateway_call_service(char *identifier, char *in, char *out, int out_len, void *ctx)
{
    if (strcmp(identifier, "SetTimerTask") == 0) {
        snprintf(out, out_len, "{\"SetTimer\": \"hello, gateway!\"}");
    } else if (strcmp(identifier, "TimeReset") == 0) {
        DPRINT("TimeReset params: %s\n", in);
    }

    return 0;
}

static int post_all_properties(gateway_t *gw)
{
    cJSON *pJson = cJSON_CreateObject();
    if (!pJson)
        return -1;

    cJSON_AddNumberToObject(pJson, "ZB_Band",     gw->ZB_Band);
    cJSON_AddNumberToObject(pJson, "ZB_Channel",  gw->ZB_Channel);
    cJSON_AddStringToObject(pJson, "ZB_CO_MAC",   gw->ZB_CO_MAC);
    cJSON_AddStringToObject(pJson, "ZB_PAN_ID",   gw->ZB_PAN_ID);
    cJSON_AddStringToObject(pJson, "EXT_PAN_ID",  gw->EXT_PAN_ID);
    cJSON_AddStringToObject(pJson, "NETWORK_KEY", gw->NETWORK_KEY);

    char *p = cJSON_PrintUnformatted(pJson);
    if (!p) {
        cJSON_Delete(pJson);
        return -1;
    }

    DPRINT("property: %s\n", p);

    linkkit_gateway_post_property_json_sync(gw->lk_dev, p, 5000);

    cJSON_Delete(pJson);
    free(p);

    return 0;
}

static int event_handler(linkkit_event_t *ev, void *ctx)
{
    gateway_t *gw = ctx;

    switch (ev->event_type) {
    case LINKKIT_EVENT_CLOUD_CONNECTED:
        DPRINT("cloud connected\n");

        post_all_properties(gw);    /* sync to cloud */
        gw->connected = 1;

        break;
    case LINKKIT_EVENT_CLOUD_DISCONNECTED:
        gw->connected = 0;
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

static linkkit_cbs_t alink_cbs = {
    .get_property = gateway_get_property,
    .set_property = gateway_set_property,

    .call_service = gateway_call_service,
};

static void ota_callback(int event, const char *version, void *ctx)
{
    DPRINT("event: %d\n", event);
    DPRINT("version: %s\n", version);

    linkkit_gateway_ota_update(512);
}

int main(void)
{
    gateway_t gateway;
    memset(&gateway, 0, sizeof(gateway_t));

    /* fill fake zigbee network info */
    gateway.ZB_Band = 25;
    gateway.ZB_Channel = 16;

    strcpy(gateway.ZB_PAN_ID,   "8215");
    strcpy(gateway.EXT_PAN_ID,  "000D6F000ED34E34"); 
    strcpy(gateway.ZB_CO_MAC,   "000D6F000ED34E34");
    strcpy(gateway.NETWORK_KEY, "21B9F385F114B1C4AE07D5753B95355D");

    linkkit_params_t *initParams = linkkit_gateway_get_default_params();
    if (!initParams)
        return -1;

    int maxMsgSize = 20 * 1024;
    linkkit_gateway_set_option(initParams, LINKKIT_OPT_MAX_MSG_SIZE, &maxMsgSize, sizeof(int));

    int maxMsgQueueSize = 8;
    linkkit_gateway_set_option(initParams, LINKKIT_OPT_MAX_MSG_QUEUE_SIZE, &maxMsgQueueSize, sizeof(int));

    int loglevel = 5;
    linkkit_gateway_set_option(initParams, LINKKIT_OPT_LOG_LEVEL, &loglevel, sizeof(int));

    linkkit_gateway_set_event_callback(initParams, event_handler, &gateway);

    if (linkkit_gateway_init(initParams) < 0) {
        DPRINT("linkkit_gateway_init failed\n");
        return -1;
    }

    gateway.lk_dev = linkkit_gateway_start(&alink_cbs, &gateway);
    if (gateway.lk_dev < 0) {
        DPRINT("linkkit_gateway_start failed\n");
        return -1;
    }

#if 0
    while (gateway.connected == 0)
        sleep(1);
#endif

    linkkit_gateway_ota_init(ota_callback, NULL);

    light_init();

    while (1) {
//        linkkit_gateway_trigger_event_json_sync(gateway.lk_dev, "Error", "{\"ErrorCode\": 0}", 10000);
        usleep(1000 * 1000);
    }

    light_exit();

    linkkit_gateway_stop(gateway.lk_dev);
    linkkit_gateway_exit();

    return 0;
}
