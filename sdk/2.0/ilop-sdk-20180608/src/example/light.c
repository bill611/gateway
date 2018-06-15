#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>

#include "linkkit.h"
#include "light.h"
#include "cJSON.h"

#define NELEMS(x)   (sizeof(x) / sizeof((x)[0]))

#define DPRINT(...)                                      \
do {                                                     \
    printf("\033[1;31;40m%s.%d: ", __func__, __LINE__);  \
    printf(__VA_ARGS__);                                 \
    printf("\033[0m");                                   \
} while (0)

#define LIGHT_MAX_NUM         (2)

typedef struct {
    int  devid;
    char productKey[32];
    char deviceName[64];
    char deviceSecret[64];

    int LightSwitch;
    int NightLightSwitch;
    int ColorTemperature;
} light_t;

typedef struct {
    char *productKey;
    char *deviceName;
    char *deviceSecret;
} light_conf_t;

static const light_conf_t light_maps[] = {
    {"a1DQA90NlFe", "Light01", "xQXstmFgUqWc36oOqzvHAHynAta843yZ"},
    {"a1DQA90NlFe", "Light02", "FQz3CiUyQ8bCMc6vGKTdu5xRTBx2CJ4x"},
};

static light_t *lights[LIGHT_MAX_NUM];

static int light_get_property(char *in, char *out, int out_len, void *ctx)
{
    DPRINT("in: %s\n", in);

    light_t *light = ctx;

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

        if (strcmp(pSub->valuestring, "LightSwitch") == 0) {
            cJSON_AddNumberToObject(pJson, "LightSwitch", light->LightSwitch);
        } else if (strcmp(pSub->valuestring, "NightLightSwitch") == 0) {
            cJSON_AddNumberToObject(pJson, "NightLightSwitch", light->NightLightSwitch);
        } else if (strcmp(pSub->valuestring, "ColorTemperature") == 0) {
            cJSON_AddNumberToObject(pJson, "ColorTemperature", light->ColorTemperature);
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

static int light_set_property(char *in, void *ctx)
{
    light_t *light = ctx;

    DPRINT("%s.%s: in %s\n", light->productKey, light->deviceName, in);

    cJSON *rJson = cJSON_Parse(in);
    if (!rJson)
        return -1;

    cJSON *LightSwitch = cJSON_GetObjectItem(rJson, "LightSwitch");
    if (LightSwitch)
        light->LightSwitch = LightSwitch->valueint;

    cJSON *NightLightSwitch = cJSON_GetObjectItem(rJson, "NightLightSwitch");
    if (NightLightSwitch)
        light->NightLightSwitch = NightLightSwitch->valueint;

    cJSON *ColorTemperature = cJSON_GetObjectItem(rJson, "ColorTemperature");
    if (ColorTemperature)
        light->ColorTemperature = ColorTemperature->valueint;

    cJSON_Delete(rJson);

    linkkit_gateway_post_property_json_sync(light->devid, in, 10000);

    return 0;
}

static int light_call_service(char *identifier, char *in, char *out, int out_len, void *ctx)
{
    light_t *light = ctx;

    DPRINT("%s.%s: in %s\n", light->productKey, light->deviceName, in);
    
    linkkit_gateway_post_property_json_sync(light->devid, "{\"SetTimer\": \"hello, world!\"}", 5000);

    return 0;
}

static linkkit_cbs_t light_cbs = {
    .get_property = light_get_property,
    .set_property = light_set_property,
    .call_service = light_call_service,
};

int light_init(void)
{
    int i;
    for (i = 0; i < LIGHT_MAX_NUM; i++) {
        light_t *light = malloc(sizeof(light_t));
        if (!light)
            break;
        memset(light, 0, sizeof(light_t));

        const light_conf_t *conf = &light_maps[i];

        strncpy(light->productKey,   conf->productKey,   sizeof(light->productKey) - 1);
        strncpy(light->deviceName,   conf->deviceName,   sizeof(light->deviceName) - 1);
        strncpy(light->deviceSecret, conf->deviceSecret, sizeof(light->deviceSecret) - 1);

        light->LightSwitch = 0;
        light->NightLightSwitch = 0;
        light->ColorTemperature = 4500;

        if (linkkit_gateway_subdev_register(light->productKey, light->deviceName, light->deviceSecret) < 0) {
            free(light);
            break;
        }

        light->devid = linkkit_gateway_subdev_create(light->productKey, light->deviceName, &light_cbs, light);
        if (light->devid < 0) {
            DPRINT("linkkit_gateway_subdev_create %s<%s> failed\n", light->deviceName, light->productKey);
            linkkit_gateway_subdev_unregister(light->productKey, light->deviceName);
            free(light);
            break;
        }

        if (linkkit_gateway_subdev_login(light->devid) < 0) {
            DPRINT("linkkit_gateway_subdev_login %s<%s> failed\n", light->deviceName, light->productKey);
            linkkit_gateway_subdev_destroy(light->devid);
            linkkit_gateway_subdev_unregister(light->productKey, light->deviceName);
            free(light);
            break;
        }

        lights[i] = light;
    }

    return 0;
}

int light_exit(void)
{
    int i;
    for (i = 0; i < LIGHT_MAX_NUM; i++) {
        light_t *light = lights[i];
        if (!light)
            continue;

        linkkit_gateway_subdev_logout(light->devid);
        linkkit_gateway_subdev_destroy(light->devid);
        linkkit_gateway_subdev_unregister(light->productKey, light->deviceName);
        free(light);

        lights[i] = NULL;
    }

    return 0;
}
