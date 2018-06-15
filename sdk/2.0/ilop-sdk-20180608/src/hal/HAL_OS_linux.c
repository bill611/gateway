/*
 * Copyright (c) 2014-2016 Alibaba Group. All rights reserved.
 * License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */



#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <memory.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/time.h>
#include <semaphore.h>
#include <errno.h>
#include <assert.h>
#include <net/if.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>


#include "iot_import.h"

#include "kv.h"

static char DEMO_CASE_PRODUCT_KEY[PRODUCT_KEY_MAXLEN] = {"a139alxxo0W"};
static char DEMO_CASE_DEVICE_NAME[DEVICE_NAME_MAXLEN] = {"IoTGatewayTest"};
static char DEMO_CASE_DEVICE_SECRET[DEVICE_SECRET_MAXLEN] = {"G43smopQ3mJ1Mo8MuzCyONkjnIbybNDR"};
static char DEMO_CASE_PRODUCT_SECRET[PRODUCT_SECRET_MAXLEN] = {0};

void *HAL_MutexCreate(void)
{
    int err_num;
    pthread_mutex_t *mutex = (pthread_mutex_t *)HAL_Malloc(sizeof(pthread_mutex_t));
    if (NULL == mutex) {
        return NULL;
    }

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);//设置锁的属性为可递归

    if (0 != (err_num = pthread_mutex_init(mutex, &attr))) {
        perror("create mutex failed");
        pthread_mutexattr_destroy(&attr);
        HAL_Free(mutex);
        return NULL;
    }
    pthread_mutexattr_destroy(&attr);

    return mutex;
}

void HAL_MutexDestroy(_IN_ void *mutex)
{
    int err_num;
    if (0 != (err_num = pthread_mutex_destroy((pthread_mutex_t *)mutex))) {
        perror("destroy mutex failed");
    }

    HAL_Free(mutex);
}

void HAL_MutexLock(_IN_ void *mutex)
{
    int err_num;
    if (0 != (err_num = pthread_mutex_lock((pthread_mutex_t *)mutex))) {
        perror("lock mutex failed");
    }
}

void HAL_MutexUnlock(_IN_ void *mutex)
{
    int err_num;
    if (0 != (err_num = pthread_mutex_unlock((pthread_mutex_t *)mutex))) {
        perror("unlock mutex failed");
    }
}

void *HAL_Malloc(_IN_ uint32_t size)
{
    return malloc(size);
}

void HAL_Free(_IN_ void *ptr)
{
    free(ptr);
}

uint64_t HAL_UptimeMs(void)
{
    uint64_t            time_ms;
    struct timespec     ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    time_ms = (ts.tv_sec * 1000LL) + (ts.tv_nsec / 1000LL / 1000LL);

    return time_ms;
}

void HAL_SleepMs(_IN_ uint32_t ms)
{
    usleep(1000 * ms);
}

void HAL_Srandom(uint32_t seed)
{
    srandom(seed);
}

uint32_t HAL_Random(uint32_t region)
{
    return (region > 0) ? (random() % region) : 0;
}

int HAL_Snprintf(_IN_ char *str, const int len, const char *fmt, ...)
{
    va_list args;
    int     rc;

    va_start(args, fmt);
    rc = vsnprintf(str, len, fmt, args);
    va_end(args);

    return rc;
}

int HAL_Vsnprintf(_IN_ char *str, _IN_ const int len, _IN_ const char *format, va_list ap)
{
    return vsnprintf(str, len, format, ap);
}

void HAL_Printf(_IN_ const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    fflush(stdout);
}

int HAL_GetPartnerID(char pid_str[PID_STR_MAXLEN])
{
    memset(pid_str, 0x0, PID_STR_MAXLEN);
    strcpy(pid_str, "example.demo.partner-id");
    return strlen(pid_str);
}

int HAL_GetModuleID(char mid_str[MID_STR_MAXLEN])
{
    memset(mid_str, 0x0, MID_STR_MAXLEN);
    strcpy(mid_str, "example.demo.module-id");
    return strlen(mid_str);
}

char *HAL_GetChipID(_OU_ char cid_str[HAL_CID_LEN])
{
    memset(cid_str, 0x0, HAL_CID_LEN);
    strncpy(cid_str, "rtl8188eu 12345678", HAL_CID_LEN);
    cid_str[HAL_CID_LEN - 1] = '\0';
    return cid_str;
}

int HAL_GetDeviceID(_OU_ char device_id[DEVICE_ID_MAXLEN])
{
    memset(device_id, 0x0, DEVICE_ID_MAXLEN);
    HAL_Snprintf(device_id, DEVICE_ID_MAXLEN, "%s.%s", DEMO_CASE_PRODUCT_KEY, DEMO_CASE_DEVICE_NAME);
    return strlen(device_id);
}

int HAL_GetDeviceName(_OU_ char device_name[DEVICE_NAME_MAXLEN])
{
    memset(device_name, 0x0, DEVICE_NAME_MAXLEN);
    HAL_Snprintf(device_name, DEVICE_NAME_MAXLEN, "%s", DEMO_CASE_DEVICE_NAME);
    return strlen(device_name);
}

int HAL_GetDeviceSecret(_OU_ char device_secret[DEVICE_SECRET_MAXLEN])
{
    memset(device_secret, 0x0, DEVICE_SECRET_MAXLEN);
    HAL_Snprintf(device_secret, DEVICE_SECRET_MAXLEN, "%s", DEMO_CASE_DEVICE_SECRET);
    return strlen(device_secret);
}

int HAL_GetFirmwareVesion(_OU_ char version[FIRMWARE_VERSION_MAXLEN])
{
    memset(version, 0x0, FIRMWARE_VERSION_MAXLEN);
    strncpy(version, "1.0", FIRMWARE_VERSION_MAXLEN);
    version[FIRMWARE_VERSION_MAXLEN - 1] = '\0';
    return strlen(version);
}

int HAL_GetProductKey(_OU_ char product_key[PRODUCT_KEY_MAXLEN])
{
    memset(product_key, 0x0, PRODUCT_KEY_MAXLEN);
    HAL_Snprintf(product_key, PRODUCT_KEY_MAXLEN, "%s", DEMO_CASE_PRODUCT_KEY);
    return strlen(product_key);
}

int HAL_GetProductSecret(_OU_ char product_secret[PRODUCT_SECRET_MAXLEN])
{
    memset(product_secret, 0, PRODUCT_SECRET_MAXLEN);
    HAL_Snprintf(product_secret, PRODUCT_SECRET_MAXLEN, "%s", DEMO_CASE_PRODUCT_SECRET);
    return strlen(product_secret);
}

void *HAL_SemaphoreCreate(void)
{
    sem_t *sem = (sem_t *)malloc(sizeof(sem_t));
    if (NULL == sem) {
        return NULL;
    }

    if (0 != sem_init(sem, 0, 0)) {
        free(sem);
        return NULL;
    }

    return sem;
}

void HAL_SemaphoreDestroy(_IN_ void *sem)
{
    sem_destroy((sem_t *)sem);
    free(sem);
}

void HAL_SemaphorePost(_IN_ void *sem)
{
    sem_post((sem_t *)sem);
}

int HAL_SemaphoreWait(_IN_ void *sem, _IN_ uint32_t timeout_ms)
{
    if (PLATFORM_WAIT_INFINITE == timeout_ms) {
        sem_wait(sem);
        return 0;
    } else {
        struct timespec ts;
        int s;
        /* Restart if interrupted by handler */
        do {
            if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
                return -1;
            }

            s = 0;
            ts.tv_nsec += (timeout_ms % 1000) * 1000000;
            if (ts.tv_nsec >= 1000000000) {
                ts.tv_nsec -= 1000000000;
                s = 1;
            }

            ts.tv_sec += timeout_ms / 1000 + s;

        } while (((s = sem_timedwait(sem, &ts)) != 0) && errno == EINTR);

        return (s == 0) ? 0 : -1;
    }
}

int HAL_ThreadCreate(
            _OU_ void **thread_handle,
            _IN_ void *(*work_routine)(void *),
            _IN_ void *arg,
            _IN_ hal_os_thread_param_t *hal_os_thread_param,
            _OU_ int *stack_used)
{
    int ret = -1;
    *stack_used = 0;

    ret = pthread_create((pthread_t *)thread_handle, NULL, work_routine, arg);

    return ret;
}

void HAL_ThreadDetach(_IN_ void *thread_handle)
{
    pthread_detach((pthread_t)thread_handle);
}

void HAL_ThreadDelete(_IN_ void *thread_handle)
{
    if (NULL == thread_handle) {
        pthread_exit(0);
    } else {
        /*main thread delete child thread*/
        pthread_cancel((pthread_t)thread_handle);
    }
}

static FILE *fp;

#define otafilename "/tmp/alinkota.bin"
void HAL_Firmware_Persistence_Start(void)
{
    fp = fopen(otafilename, "w");
    assert(fp);
    return;
}

int HAL_Firmware_Persistence_Write(_IN_ char *buffer, _IN_ uint32_t length)
{
    unsigned int written_len = 0;
    written_len = fwrite(buffer, 1, length, fp);

    if (written_len != length) {
        return -1;
    }
    return 0;
}

int HAL_Firmware_Persistence_Stop(void)
{
    if (fp != NULL) {
        fclose(fp);
    }

    /* check file md5, and burning it to flash ... finally reboot system */

    return 0;
}

int HAL_Config_Write(const char *buffer, int length)
{
    FILE *fp;
    size_t written_len;
    char filepath[128] = {0};

    if (!buffer || length <= 0) {
        return -1;
    }

    snprintf(filepath, sizeof(filepath), "./%s", "alinkconf");
    fp = fopen(filepath, "w");
    if (!fp) {
        return -1;
    }

    written_len = fwrite(buffer, 1, length, fp);

    fclose(fp);

    return ((written_len != length) ? -1 : 0);
}

int HAL_Config_Read(char *buffer, int length)
{
    FILE *fp;
    size_t read_len;
    char filepath[128] = {0};

    if (!buffer || length <= 0) {
        return -1;
    }

    snprintf(filepath, sizeof(filepath), "./%s", "alinkconf");
    fp = fopen(filepath, "r");
    if (!fp) {
        return -1;
    }

    read_len = fread(buffer, 1, length, fp);
    fclose(fp);

    return ((read_len != length) ? -1 : 0);
}

#define REBOOT_CMD "reboot"
void HAL_Sys_reboot(void)
{
    if (system(REBOOT_CMD)) {
        perror("HAL_Sys_reboot failed");
    }
}

#define ROUTER_INFO_PATH        "/proc/net/route"
#define ROUTER_RECORD_SIZE      256

char *_get_default_routing_ifname(char *ifname, int ifname_size)
{
    FILE *fp = NULL;
    char line[ROUTER_RECORD_SIZE] = {0};
    char iface[IFNAMSIZ] = {0};
    char *result = NULL;
    unsigned int destination, gateway, flags, mask;
    unsigned int refCnt, use, metric, mtu, window, irtt;

    fp = fopen(ROUTER_INFO_PATH, "r");
    if (fp == NULL) {
        perror("fopen");
        return result;
    }

    char *buff = fgets(line, sizeof(line), fp);
    if (buff == NULL) {
        perror("fgets");
        goto out;
    }

    while (fgets(line, sizeof(line), fp)) {
        if (11 !=
            sscanf(line, "%s %08x %08x %x %d %d %d %08x %d %d %d",
                   iface, &destination, &gateway, &flags, &refCnt, &use,
                   &metric, &mask, &mtu, &window, &irtt)) {
            perror("sscanf");
            continue;
        }

        /*default route */
        if ((destination == 0) && (mask == 0)) {
            strncpy(ifname, iface, ifname_size - 1);
            result = ifname;
            break;
        }
    }

out:
    if (fp) {
        fclose(fp);
    }

    return result;
}

char *HAL_Wifi_Get_Mac(char mac_str[HAL_MAC_LEN])
{
    struct ifreq ifr;
    int sock = -1;

    memset(mac_str, 0, HAL_MAC_LEN);

    char ifname_buff[IFNAMSIZ] = {0};
    char *ifname = _get_default_routing_ifname(ifname_buff, sizeof(ifname_buff));
    if (!ifname) {
        perror("get default routeing ifname");
        goto fail;
    }

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        goto fail;
    }

    ifr.ifr_addr.sa_family = AF_INET; //ipv4 address
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);

    if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0) {
        close(sock);
        perror("ioctl");
        goto fail;
    }

    close(sock);

    char *ptr = mac_str;
    char *end = mac_str + HAL_MAC_LEN;

    int i;
    for (i = 0; i < 6; i++) {
        if (i == 5)
            ptr += snprintf(ptr, end - ptr, "%02x",  (uint8_t)ifr.ifr_hwaddr.sa_data[i]);
        else
            ptr += snprintf(ptr, end - ptr, "%02x:", (uint8_t)ifr.ifr_hwaddr.sa_data[i]);
    }

    return mac_str;

fail:
    strncpy(mac_str, "de:ad:be:ef:00:00", HAL_MAC_LEN - 1);
    return mac_str;
}

uint32_t HAL_Wifi_Get_IP(char ip_str[NETWORK_ADDR_LEN], const char *ifname)
{
    struct ifreq ifreq;
    int sock = -1;
    char ifname_buff[IFNAMSIZ] = {0};

    if((NULL == ifname || strlen(ifname) == 0) &&
        NULL == (ifname = _get_default_routing_ifname(ifname_buff, sizeof(ifname_buff)))){
        perror("get default routeing ifname");
        return -1;
    }

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        return -1;
    }

    ifreq.ifr_addr.sa_family = AF_INET; //ipv4 address
    strncpy(ifreq.ifr_name, ifname, IFNAMSIZ - 1);

    if (ioctl(sock, SIOCGIFADDR, &ifreq) < 0) {
        close(sock);
        perror("ioctl");
        return -1;
    }

    close(sock);

    strncpy(ip_str,
            inet_ntoa(((struct sockaddr_in *)&ifreq.ifr_addr)->sin_addr),
            NETWORK_ADDR_LEN);

    return ((struct sockaddr_in *)&ifreq.ifr_addr)->sin_addr.s_addr;
}

static kv_file_t *kvfile = NULL;

#define KVFILE_DEFAULT_PATH "/tmp/kvfile.db"

int HAL_Kv_Set(const char *key, const void *val, int len, int sync)
{
    if (!kvfile) {
        kvfile = kv_open(KVFILE_DEFAULT_PATH);
        if (!kvfile)
            return -1;
    }

    return kv_set_blob(kvfile, (char *)key, (char *)val, len);
}

int HAL_Kv_Get(const char *key, void *buffer, int *buffer_len)
{
    if (!kvfile) {
        kvfile = kv_open(KVFILE_DEFAULT_PATH);
        if (!kvfile)
            return -1;
    }

    return kv_get_blob(kvfile, (char *)key, buffer, buffer_len);
}

int HAL_Kv_Del(const char *key)
{
    if (!kvfile) {
        kvfile = kv_open(KVFILE_DEFAULT_PATH);
        if (!kvfile)
            return -1;
    }

    return kv_del(kvfile, (char *)key);
}
