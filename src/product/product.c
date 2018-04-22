#include <stdio.h>
#include <string.h>
#include "product.h"

//TODO: update these product info
#define product_model           "ALINKTEST_LIVING_LIGHT_SMARTLED_LUA"
#define product_key             "iv24VQiby0PJonv3YYtt"
#define product_secret          "rW7pnj2x8t7vfuVzj41SkRI8k2e1WoYeXNUyD5ey"
#define product_debug_key       "GG81ieHbhgl7Igm1KxUm"
#define product_debug_secret    "p7C7sBS0uGA2cs95E32Kf1xlgiJ4KjXTX06ZD78k"

char *product_get_name(char name_str[PRODUCT_NAME_LEN])
{
	return strncpy(name_str, "alink_product", PRODUCT_NAME_LEN);
}

char *product_get_version(char ver_str[PRODUCT_VERSION_LEN])
{
	return strncpy(ver_str, "1.0", PRODUCT_VERSION_LEN);
}

char *product_get_model(char model_str[PRODUCT_MODEL_LEN])
{
	return strncpy(model_str, product_model, PRODUCT_MODEL_LEN);
}

char *product_get_key(char key_str[PRODUCT_KEY_LEN])
{
	return strncpy(key_str, product_key, PRODUCT_KEY_LEN);
}

char *product_get_secret(char secret_str[PRODUCT_SECRET_LEN])
{
	return strncpy(secret_str, product_secret, PRODUCT_SECRET_LEN);
}

char *product_get_debug_key(char key_str[PRODUCT_KEY_LEN])
{
	return strncpy(key_str, product_debug_key, PRODUCT_KEY_LEN);
}

char *product_get_debug_secret(char secret_str[PRODUCT_SECRET_LEN])
{
	return strncpy(secret_str, product_debug_secret, PRODUCT_SECRET_LEN);
}

char *product_get_sn(char sn_str[PRODUCT_SN_LEN])
{
	return strncpy(sn_str, "6923450656860", PRODUCT_SN_LEN);
}

char *product_get_device_key(char key_str[DEVICE_KEY_LEN])
{
	
}
char *product_get_device_secret(char secret_str[DEVICE_SECRET_LEN])
{
	
}
