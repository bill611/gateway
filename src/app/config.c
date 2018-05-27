#include <sys/ioctl.h>
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include "iniparser/iniparser.h"
#include "config.h"

#define CFG_PUBLIC_DRIVE "/mnt/nand1-2"
#define CFG_PRIVATE_DRIVE "/mnt/nand1-2"
#define CFG_TEMP_DRIVE "/mnt/nand1-2"

#define INI_PUBLIC_FILENAME "config_public.ini"
#define INI_PRIVATE_FILENAME "config_private.ini"
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
// {"Public",	"zigbeeOk",				&theConfig.zigbee_ok,		0},
};
// static EtcValueChar etc_public_char[]={
// };

static EtcValueInt etc_private_int[]={
// {"Public",	"devicetype",		&theConfig.devType,6},
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
        printf("[%s]%s,%d\n", __FUNCTION__,buf,*etc_file->value);
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
        printf("[%s]%s,%s\n", __FUNCTION__,buf,etc_file->value);
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

void ConfigLoad(void)
{
    cfg_public_ini = iniparser_load(CFG_PUBLIC_DRIVE ":/" INI_PUBLIC_FILENAME);
    if (!cfg_public_ini) {
        cfg_public_ini = dictionary_new(0);
        assert(cfg_public_ini);

    }

    cfg_private_ini = iniparser_load(CFG_PRIVATE_DRIVE ":/" INI_PRIVATE_FILENAME);
	if (!cfg_private_ini) {
	    cfg_private_ini = dictionary_new(0);
        assert(cfg_private_ini);

        dictionary_set(cfg_private_ini, "doorbell", NULL);
	}

    // cfg_temp_ini = iniparser_load(CFG_TEMP_DRIVE ":/" INI_PUBLIC_FILENAME);
	// if (!cfg_temp_ini) {
		// cfg_temp_ini = dictionary_new(0);
        // assert(cfg_temp_ini);

        // dictionary_set(cfg_temp_ini, "doorbell", NULL);
	// }
	configLoadEtcInt(cfg_public_ini,etc_public_int,NELEMENTS(etc_public_int));
	// configLoadEtcChar(cfg_public_ini,etc_public_char,NELEMENTS(etc_public_char));
	configLoadEtcInt(cfg_private_ini,etc_private_int,NELEMENTS(etc_private_int));
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
    f = fopen(CFG_PUBLIC_DRIVE ":/" INI_PUBLIC_FILENAME, "wb");
	if (!f) {
	    printf("cannot open ini file: %s\n", CFG_PUBLIC_DRIVE ":/" INI_PUBLIC_FILENAME);
        return;
    }

    iniparser_dump_ini(cfg_public_ini, f);
	fflush(f);
    fclose(f);
	printf("[%s]end\n", __FUNCTION__);
}

static void SavePrivate(void)
{
    FILE* f;

	// configSaveEtcInt(etc_public_int,NELEMENTS(etc_public_int));
	configSaveEtcInt(cfg_private_ini,etc_private_int,NELEMENTS(etc_private_int));

    // save to file
    f = fopen(CFG_PRIVATE_DRIVE ":/" INI_PRIVATE_FILENAME, "wb");
	if (!f) {
	    printf("cannot open ini file: %s\n", CFG_PRIVATE_DRIVE ":/" INI_PRIVATE_FILENAME);
        return;
    }

    iniparser_dump_ini(cfg_private_ini, f);
	fflush(f);
    fclose(f);
	printf("[%s]end\n", __FUNCTION__);
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
		// printf("cannot open ini file: %s\n", CFG_TEMP_DRIVE ":/" INI_PUBLIC_FILENAME);
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
    args[1] = (int)CFG_PUBLIC_DRIVE ":/" INI_PUBLIC_FILENAME;
	args[2] = (int)func;

    CreateWorkerThread(ConfigSaveTask, args);
}

void ConfigSavePrivate(void)
{
    static int args[3];

    args[0] = CONFIG_SAVE_PRIVATE;
    args[1] = (int)CFG_PRIVATE_DRIVE ":/" INI_PRIVATE_FILENAME;

    CreateWorkerThread(ConfigSaveTask, args);
}

void ConfigSavePrivateCallback(void (*func)(void))
{
    static int args[3];

    args[0] = CONFIG_SAVE_PRIVATE;
    args[1] = (int)CFG_PRIVATE_DRIVE ":/" INI_PRIVATE_FILENAME;
	args[2] = (int)func;

    CreateWorkerThread(ConfigSaveTask, args);
}
void ConfigSaveTemp(void)
{
    static int args[3];

    args[0] = CONFIG_SAVE_TEMP;
    args[1] = (int)CFG_TEMP_DRIVE ":/" INI_PUBLIC_FILENAME;

    CreateWorkerThread(ConfigSaveTask, args);
}

