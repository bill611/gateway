#include <sys/ioctl.h>
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "iniparser/iniparser.h"
#include "externfunc.h"
#include "config.h"
#include "debug.h"

#define CFG_PUBLIC_DRIVE "/mnt/nand1-2/"
#define CFG_PRIVATE_DRIVE "/mnt/nand1-2/"
#define CFG_TEMP_DRIVE "/mnt/nand1-2/"

#define INI_PUBLIC_FILENAME "config_para.ini"
#define INI_PRIVATE_FILENAME "config.ini"
#define SIZE_CONFIG(x)  x,sizeof(x) - 1

#define NELEMENTS(array)  (sizeof (array) / sizeof ((array) [0]))

typedef enum
{
    CONFIG_CRC,
    CONFIG_SAVE,
    CONFIG_SAVE_PRIVATE,
    CONFIG_SAVE_TEMP
} ConfigAction;

Config theConfig;
static dictionary* cfg_public_ini;
static dictionary* cfg_private_ini;
// static dictionary* cfg_temp_ini;
static pthread_mutex_t cfg_mutex  = PTHREAD_MUTEX_INITIALIZER;

static EtcValueInt etc_public_int[]={
// {"Params",	"ele_quantity",				&theConfig.ele_quantity,		0},
};
// static EtcValueChar etc_public_char[]={
// };

static EtcValueInt etc_private_int[]={
// {"Public",	"devicetype",		&theConfig.devType,6},
};

static EtcValueChar etc_private_char[]={
{"GateWay",			"id",				SIZE_CONFIG(theConfig.gate_way_id),		"0"},
{"GateWay",			"product_key",		SIZE_CONFIG(theConfig.gate_way.product_key),		"0"},
{"GateWay",			"device_secret",	SIZE_CONFIG(theConfig.gate_way.device_secret),		"0"},
};

char *auth_mode[] = {
	"OPEN",
	"SHARED",
	"WPAPSK",
	"WPA8021X",
	"WPA2PSK",
	"WPA28021X",
	"WPAPSKWPA2PSK",
	"AWSS_AUTH_TYPE_WPAPSKWPA2PSK",
};

char *encrypt_type[] = {
	"NONE",
	"WEP",
	"TKIP",
	"AES",
	"TKIPAES",
	"AWSS_ENC_TYPE_TKIPAES",
};


TcWifiConfig tc_wifi_config = {
	.boot_proto = "DHCP",
	.network_type = "Infra",
	.ssid = "alitest",
	.auth_mode = "WPA2PSK",
	.encrypt_type = "AES",
	.auth_key = "alitest123",

	.ap_addr = "192.168.100.1",
	.ap_ssid = "AliGateWay",
	.ap_auth_mode = "WPA2PSK",
	.ap_encrypt_type = "AES",
	.ap_auth_key = "12345678",
	.ap_channel = 11,
};

void configSync(void)
{
#if defined(CFG_NOR_ENABLE) && CFG_NOR_CACHE_SIZE > 0
    ioctl(ITP_DEVICE_NOR, ITP_IOCTL_FLUSH, NULL);
#endif
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief configoadEtcInt 加载int型配置文件
 *
 * @param etc_file 文件数组地址
 * @param length 数组长度
 */
/* ---------------------------------------------------------------------------*/
static void configLoadEtcInt(dictionary *cfg_ini, EtcValueInt *etc_file,
		unsigned int length)
{
	unsigned int i;
	char buf[64];
	for (i=0; i<length; i++) {
		sprintf(buf,"%s:%s",etc_file->section,etc_file->key);
		*etc_file->value = iniparser_getint(cfg_ini, buf, etc_file->default_int);
        DPRINT("[%s]%s,%d\n", __FUNCTION__,buf,*etc_file->value);
		etc_file++;
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief configLoadEtcChar 加载char型配置文件
 *
 * @param etc_file 文件数组地址
 * @param length 数组长度
 */
/* ---------------------------------------------------------------------------*/
static void configLoadEtcChar(dictionary *cfg_ini, EtcValueChar *etc_file,
		unsigned int length)
{
	unsigned int i;
	char buf[64];
	for (i=0; i<length; i++) {
		sprintf(buf,"%s:%s",etc_file->section,etc_file->key);
		strncpy(etc_file->value,
			   	iniparser_getstring(cfg_ini, buf, etc_file->default_char),
			   	etc_file->leng);
        DPRINT("[%s]%s,%s\n", __FUNCTION__,buf,etc_file->value);
		etc_file++;
	}
}

void configPrivateFileCheck(void)
{
	int ret = true;
	unsigned int i;
	char check = 0;
	// if (atoi(theConfig.has_check_fuc) == 0)
		// return;
	// for (i=0; i<NELEMENTS(etc_private_char) - 1; i++) {
		// unsigned int k;
		// for (k=0; k<etc_private_char[i].leng; k++) {
			// check += etc_private_char[i].value[k];
		// }
	// }
	// if (check  != theConfig.check[0])
		// copyfile(CFG_PRIVATE_DRIVE ":/" INI_PRIVATE_FILENAME,CFG_PRIVATE_DRIVE ":/" INI_PRIVATE_FILENAME "_bak");
}

void configLoad(void)
{
	// cfg_public_ini = iniparser_load(CFG_PUBLIC_DRIVE  INI_PUBLIC_FILENAME);
	// if (!cfg_public_ini) {
		// cfg_public_ini = dictionary_new(0);
		// assert(cfg_public_ini);

	// }

    cfg_private_ini = iniparser_load(CFG_PRIVATE_DRIVE INI_PRIVATE_FILENAME);
	if (!cfg_private_ini) {
	    cfg_private_ini = dictionary_new(0);
        assert(cfg_private_ini);
	}

    // cfg_temp_ini = iniparser_load(CFG_TEMP_DRIVE ":/" INI_PUBLIC_FILENAME);
	// if (!cfg_temp_ini) {
		// cfg_temp_ini = dictionary_new(0);
        // assert(cfg_temp_ini);

        // dictionary_set(cfg_temp_ini, "doorbell", NULL);
	// }
	// configLoadEtcInt(cfg_public_ini,etc_public_int,NELEMENTS(etc_public_int));
	// configLoadEtcChar(cfg_public_ini,etc_public_char,NELEMENTS(etc_public_char));
	configLoadEtcInt(cfg_private_ini,etc_private_int,NELEMENTS(etc_private_int));
	configLoadEtcChar(cfg_private_ini,etc_private_char,NELEMENTS(etc_private_char));
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief configSaveEtcInt 加载int型配置文件
 *
 * @param etc_file 文件数组地址
 * @param length 数组长度
 */
/* ---------------------------------------------------------------------------*/
static void configSaveEtcInt(dictionary *cfg_ini, EtcValueInt *etc_file,
		unsigned int length)
{
	unsigned int i;
	char buf[64];
	char data[64];
	for (i=0; i<length; i++) {
		sprintf(buf,"%s:%s",etc_file->section,etc_file->key);
		sprintf(data,"%d",*etc_file->value);
		iniparser_set(cfg_ini, buf, data);
		etc_file++;
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief configSaveEtcChar 加载char型配置文件
 *
 * @param etc_file 文件数组地址
 * @param length 数组长度
 */
/* ---------------------------------------------------------------------------*/
static void configSaveEtcChar(dictionary *cfg_ini, EtcValueChar *etc_file,
		unsigned int length)
{
	unsigned int i;
	char buf[64];
	for (i=0; i<length; i++) {
		sprintf(buf,"%s:%s",etc_file->section,etc_file->key);
		iniparser_set(cfg_ini, buf, etc_file->value);
		etc_file++;
	}
}
static void SavePublic(void)
{
    FILE* f;

	configSaveEtcInt(cfg_public_ini,etc_public_int,NELEMENTS(etc_public_int));
	// configSaveEtcChar(cfg_public_ini,etc_public_char,NELEMENTS(etc_public_char));

    // save to file
    f = fopen(CFG_PUBLIC_DRIVE  INI_PUBLIC_FILENAME, "wb");
	if (!f) {
	    DPRINT("cannot open ini file: %s\n", CFG_PUBLIC_DRIVE  INI_PUBLIC_FILENAME);
        return;
    }

    iniparser_dump_ini(cfg_public_ini, f);
	fflush(f);
    fclose(f);
	DPRINT("[%s]end\n", __FUNCTION__);
}

static void SavePrivate(void)
{
    FILE* f;

	// configSaveEtcInt(etc_public_int,NELEMENTS(etc_public_int));
	configSaveEtcInt(cfg_private_ini,etc_private_int,NELEMENTS(etc_private_int));
	configSaveEtcChar(cfg_private_ini,etc_private_char,NELEMENTS(etc_private_char));

    // save to file
    f = fopen(CFG_PRIVATE_DRIVE  INI_PRIVATE_FILENAME, "wb");
	if (!f) {
	    DPRINT("cannot open ini file: %s\n", CFG_PRIVATE_DRIVE  INI_PRIVATE_FILENAME);
        return;
    }

    iniparser_dump_ini(cfg_private_ini, f);
	fflush(f);
    fclose(f);
	DPRINT("[%s]end\n", __FUNCTION__);
}

static void SaveTemp(void)
{
    // FILE* f;
    // char buf[8];

    // temp
    // sprintf(buf, "%d", theConfig.missed_call_count);
    // iniparser_set(cfg_temp_ini, "doorbell:missed_call_count", buf);

    // iniparser_set(cfg_temp_ini, "doorbell:muted", theConfig.muted ? "y" : "n");

    // // save to file
    // f = fopen(CFG_TEMP_DRIVE ":/" INI_PUBLIC_FILENAME, "wb");
	// if (!f)
    // {
		// DPRINT("cannot open ini file: %s\n", CFG_TEMP_DRIVE ":/" INI_PUBLIC_FILENAME);
        // return;
    // }

    // iniparser_dump_ini(cfg_temp_ini, f);
    // fclose(f);
}

static void* ConfigSaveTask(void* arg)
{
    int* args = (int*) arg;
    ConfigAction action = (ConfigAction) args[0];
	int func = 0;

    pthread_mutex_lock(&cfg_mutex);

	if (action == CONFIG_SAVE) {
		func = (int)args[2];
        SavePublic();
	} else if (action == CONFIG_SAVE_PRIVATE)  {
		func = (int)args[2];
		SavePrivate();
	} 
    else if (action == CONFIG_SAVE_TEMP)
        SaveTemp();

#ifdef CFG_CHECK_FILES_CRC_ON_BOOTING
    char* filepath = (char*) args[1];
    if (action != CONFIG_SAVE_PRIVATE)
        UpgradeSetFileCrc(filepath);
#endif
	configSync();
    // sendCmdConfigSave(func);

    pthread_mutex_unlock(&cfg_mutex);

    return NULL;
}

void ConfigUpdateCrc(char* filepath)
{
#ifdef CFG_CHECK_FILES_CRC_ON_BOOTING
    static int args[2];
    static char path[256];

    strcpy(path, filepath);

    args[0] = CONFIG_CRC;
    args[1] = (int)path;

    CreateWorkerThread(ConfigSaveTask, args);
#endif // CFG_CHECK_FILES_CRC_ON_BOOTING
}

void ConfigSave(void (*func)(void))
{
    static int args[3];

    args[0] = CONFIG_SAVE;
    args[1] = (int)CFG_PUBLIC_DRIVE  INI_PUBLIC_FILENAME;
	args[2] = (int)func;

    CreateWorkerThread(ConfigSaveTask, args);
}

void ConfigSavePrivate(void)
{
    static int args[3];

    args[0] = CONFIG_SAVE_PRIVATE;
    args[1] = (int)CFG_PRIVATE_DRIVE  INI_PRIVATE_FILENAME;

    CreateWorkerThread(ConfigSaveTask, args);
}

void ConfigSavePrivateCallback(void (*func)(void))
{
    static int args[3];

    args[0] = CONFIG_SAVE_PRIVATE;
    args[1] = (int)CFG_PRIVATE_DRIVE  INI_PRIVATE_FILENAME;
	args[2] = (int)func;

    CreateWorkerThread(ConfigSaveTask, args);
}
void ConfigSaveTemp(void)
{
    static int args[3];

    args[0] = CONFIG_SAVE_TEMP;
    args[1] = (int)CFG_TEMP_DRIVE  INI_PUBLIC_FILENAME;

    CreateWorkerThread(ConfigSaveTask, args);
}

void tcSetNetwork(int type)
{
	FILE *fp;
	char *ret;
	DPRINT("[%s]\n",__FUNCTION__);
	fp = fopen("network_config","wb");	
	if (fp == NULL) {
		DPRINT("Can't open network_config\n");
		return;
	}
	fprintf(fp,"BOOTPROTO %s\n",	tc_wifi_config.boot_proto);
	fprintf(fp,"NETWORK_TYPE %s\n",	tc_wifi_config.network_type);
	fprintf(fp,"SSID %s\n",			tc_wifi_config.ssid);
	fprintf(fp,"AUTH_MODE %s\n"	,	tc_wifi_config.auth_mode);
	fprintf(fp,"ENCRYPT_TYPE %s\n",	tc_wifi_config.encrypt_type);
	fprintf(fp,"AUTH_KEY %s\n",		tc_wifi_config.auth_key);

	fprintf(fp,"AP_IPADDR %s\n",	tc_wifi_config.ap_addr);
	fprintf(fp,"AP_SSID %s\n",		tc_wifi_config.ap_ssid);
	fprintf(fp,"AP_AUTH_MODE %s\n",	tc_wifi_config.ap_auth_mode);
	fprintf(fp,"AP_ENCRYPT_TYPE %s\n",tc_wifi_config.ap_encrypt_type);
	fprintf(fp,"AP_AUTH_KEY %s\n",	tc_wifi_config.ap_auth_key);
	fprintf(fp,"AP_CHANNEL %d\n",	tc_wifi_config.ap_channel);
	fflush(fp);
	fclose(fp);
	sync();

	if (type == TC_SET_AP){
		// 开启AP服务，用cmd时会在hostapd阻塞，目前还没找到原因，暂时用system替代
		system("./network.sh SoftAP");
	} else {
		ret = excuteCmd("./network.sh","Infra",NULL);
		printf("%s\n", ret);
	}
}

void resetWifi(void)
{
	char *ret;
	char buf[256];
	int cnt = 50;
	sprintf(tc_wifi_config.ssid,"aha");
	sprintf(tc_wifi_config.auth_key,"12345678");
	tcSetNetwork(TC_SET_STATION);
	snprintf(buf, sizeof(buf), "./wpa_cli -p %s -i %s status | grep wpa_state",
			WPA_PATH, WLAN_IFNAME);
	do {
		ret = excuteCmd(buf,NULL);
		usleep(100 * 1000);
	} while ((strncmp(ret,"wpa_state=COMPLETED",strlen("wpa_state=COMPLETED")) != 0) && --cnt != 0);

	if (cnt == 0)
		return;
	// sys_net_is_ready = 1;
	snprintf(buf, sizeof(buf), "udhcpc -i %s", WLAN_IFNAME);
	int ret_sys = system(buf);
	DPRINT("[%s]system:%s,%d\n",__FUNCTION__,buf,ret_sys);
}

void activeAp(void)
{
	tcSetNetwork(TC_SET_AP);
}

void printfWifiInfo(int cnt )
{
	FILE *fd = fopen("network_config","rb");	
	char buff[32] = {0};
	char ssid[32] = {0};
	char pass[32] = {0};
	if (fd) {
		while (!feof(fd))	 {
			memset(ssid,0,sizeof(ssid));
			memset(pass,0,sizeof(pass));
			fgets(buff,sizeof(buff),fd);
			sscanf(buff,"SSID %s\n",ssid);
			if (ssid[0])
				printf("%d-->ssid:[%s],",cnt, ssid);
			sscanf(buff,"AUTH_KEY %s\n",pass);
			if (pass[0])
				printf("password:[%s]\n", pass);
		}
		fclose(fd);
	}
	
}
