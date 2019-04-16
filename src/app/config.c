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
};
static EtcValueChar etc_public_char[]={
};

static EtcValueInt etc_private_int[]={
};

static EtcValueChar etc_private_char[]={
{"gateway",		"device_name",		SIZE_CONFIG(theConfig.gate_way.device_name),		"0"},
{"gateway",	    "product_key",		SIZE_CONFIG(theConfig.gate_way.product_key),		"a1mrCtDYEbi"},
{"gateway",		"device_secret",	SIZE_CONFIG(theConfig.gate_way.device_secret),		"0"},

{"taichuan",	"imei",	            SIZE_CONFIG(theConfig.imei),		"0"},
{"taichuan",	"version",	        SIZE_CONFIG(theConfig.version),		GW_VERSION},

#if (defined V23)
{"ethernet",	"dhcp",	    SIZE_CONFIG(theConfig.net_config.dhcp),		"1"},
{"ethernet",	"ipaddr",	SIZE_CONFIG(theConfig.net_config.ipaddr),	"172.16.10.10"},
{"ethernet",	"netmask",	SIZE_CONFIG(theConfig.net_config.netmask),	"255.255.0.0"},
{"ethernet",	"gateway",	SIZE_CONFIG(theConfig.net_config.gateway),	"172.16.1.1"},
{"ethernet",	"macaddr",	SIZE_CONFIG(theConfig.net_config.macaddr),	"00:01:02:03:04:05"},
{"ethernet",	"firstdns",	SIZE_CONFIG(theConfig.net_config.firstdns),	"114.114.114.114"},
{"ethernet",	"backdns",	SIZE_CONFIG(theConfig.net_config.backdns),	"8.8.8.8"},

{"wireless",	"ssid",	    SIZE_CONFIG(theConfig.net_config.ssid),		"MINI"},
{"wireless",	"mode",	    SIZE_CONFIG(theConfig.net_config.mode),		"Infra"},
{"wireless",	"security",	SIZE_CONFIG(theConfig.net_config.security),	"WPA/WPA2 PSK"},
{"wireless",	"password",	SIZE_CONFIG(theConfig.net_config.password),	"12345678"},
{"wireless",	"running",	SIZE_CONFIG(theConfig.net_config.running),	"station"},

{"softap",	    "s_ssid",	 SIZE_CONFIG(theConfig.net_config.s_ssid),	  "alitest"},
{"softap",	    "s_password",SIZE_CONFIG(theConfig.net_config.s_password),"12345678"},
#endif
};

#if (!defined V23)
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
	.ssid = "MINI",
	.auth_mode = "WPA2PSK",
	.encrypt_type = "AES",
	.auth_key = "12345678",

	.ap_addr = "192.168.100.1",
	.ap_ssid = "AliGateWay",
	.ap_auth_mode = "WPA2PSK",
	.ap_encrypt_type = "AES",
	.ap_auth_key = "12345678",
	.ap_channel = 11,
};
#endif

void configSync(void)
{
#if defined(CFG_NOR_ENABLE) && CFG_NOR_CACHE_SIZE > 0
    ioctl(ITP_DEVICE_NOR, ITP_IOCTL_FLUSH, NULL);
#endif
}
static int etcFileCheck(void)
{
	const char * buf = iniparser_getstring(cfg_private_ini, "gateway:product_key", "0");
	if (strcmp(buf,"0") == 0) {
		recoverData(CFG_PRIVATE_DRIVE  INI_PRIVATE_FILENAME);
		return 0;
	} else {
		return 1;
	}
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
		DPRINT("[%d]iniparser_set:%s--%s\n",i,buf,etc_file->value );
		iniparser_set(cfg_ini, buf, etc_file->value);
		etc_file++;
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief dumpIniFile iniparser_dump_ini 配置
 *
 * @param d
 * @param file_name
 */
/* ---------------------------------------------------------------------------*/
static void dumpIniFile(dictionary *d,char *file_name)
{
    FILE* f;
    // save to file
    f = fopen(file_name, "wb");
	if (!f) {
	    printf("cannot open ini file: %s\n", file_name);
        return;
    }

    iniparser_dump_ini(d, f);
	fflush(f);
    fclose(f);

}

static void SavePublic(void)
{
	configSaveEtcInt(cfg_public_ini,etc_public_int,NELEMENTS(etc_public_int));
	configSaveEtcChar(cfg_public_ini,etc_public_char,NELEMENTS(etc_public_char));
	dumpIniFile(cfg_public_ini,CFG_PUBLIC_DRIVE  INI_PUBLIC_FILENAME);
	printf("[%s]end\n", __FUNCTION__);
}

static void SavePrivate(void)
{
	configSaveEtcChar(cfg_private_ini,etc_private_char,NELEMENTS(etc_private_char));
	dumpIniFile(cfg_private_ini,CFG_PRIVATE_DRIVE  INI_PRIVATE_FILENAME);
	if (etcFileCheck() == 1) {
		backData(CFG_PRIVATE_DRIVE  INI_PRIVATE_FILENAME);
	}
	printf("[%s]end\n", __FUNCTION__);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief loadIniFile 加载ini文件，同时检测字段完整性
 *
 * @param d
 * @param file_path
 * @param sec[]
 *
 * @returns >0 有缺少字段，需要保存更新， 0无缺少字段，正常
 */
/* ---------------------------------------------------------------------------*/
static int loadIniFile(dictionary **d,char *file_path,char *sec[])
{
	int ret = 0;
	int i;
    *d = iniparser_load(file_path);
    if (*d == NULL) {
        *d = dictionary_new(0);
        assert(*d);
		ret++;
		for (i=0; sec[i] != NULL; i++)
			iniparser_set(*d, sec[i], NULL);
	} else {
		int nsec = iniparser_getnsec(*d);
		int j;
		char *  secname;
		for (i=0; sec[i] != NULL; i++) {
			for (j=0; j<nsec; j++) {
				secname = iniparser_getsecname(*d, j);
				if (strcasecmp(secname,sec[i]) == 0)
					break;
			}
			if (j == nsec)  {
				ret++;
				iniparser_set(*d, sec[i], NULL);
			}
		}
	}
	return ret;
}

void configLoad(void)
{
	char *sec_private[] = {"gateway","taichuan","ethernet","wireless","softap",NULL};

	int ret = loadIniFile(&cfg_private_ini,CFG_PRIVATE_DRIVE  INI_PRIVATE_FILENAME,sec_private);
	configLoadEtcInt(cfg_private_ini,etc_private_int,NELEMENTS(etc_private_int));
	configLoadEtcChar(cfg_private_ini,etc_private_char,NELEMENTS(etc_private_char));
	etcFileCheck();
	if (ret || strcmp(theConfig.version,GW_VERSION) != 0)
		strcpy(theConfig.version,GW_VERSION);
		SavePrivate();
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

/* ---------------------------------------------------------------------------*/
/**
 * @brief tcSetNetwork 如果为有线网络时，复位后直接重启
 *
 * @param type
 */
/* ---------------------------------------------------------------------------*/
void tcSetNetwork(int type)
{
#if (defined V23)
	FILE *fd;
	char buf[64] = {0};
	char net_type[16] = {0};
	char connect_status[8] = {0};
	fd = fopen("/mnt/public/net_status","rb");
	if (fd) {
		int ret = fread(buf,sizeof(buf),1,fd);
		sscanf(buf,"net=%s,connect=%s",net_type,connect_status);
		fclose(fd);
	}
	printf("net_type:%s\n",net_type );
	// 只有在无线网卡时，复位时建立AP
	if (strncmp(net_type,"wlan0",strlen("wlan0")) == 0) {
		system(CFG_PRIVATE_DRIVE"/wifi/wifi_softap.sh start");
	} else {
		sleep(5);
		exit(0);
	}
#else
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
#endif
}

void resetWifi(void)
{
#if (defined V1)
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
#endif
}

void activeAp(void)
{
	tcSetNetwork(TC_SET_AP);
}

