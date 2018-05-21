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
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <net/if.h>       // struct ifreq
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>       // IP_MAXPACKET (65535)
#include <net/ethernet.h>     // ETH_P_ALL
#include <linux/if_packet.h>
#include "openssl/aes.h"

#include "externfunc.h"
#include "platform.h"
#include "platform_config.h"
#include "iwlib.h"

typedef struct{
    AES_KEY ctx;
    uint8_t iv[32];
}platform_aes_t;

enum {
	TC_SET_STATION,
	TC_SET_AP,
};
typedef struct {
	// station 
	char boot_proto[64];
	char network_type[64];
	char ssid[128];
	char auth_mode[64];
	char encrypt_type[64];
	char auth_key[64];

	// ap
	char ap_addr[64];
	char ap_ssid[64];
	char ap_auth_mode[128];
	char ap_encrypt_type[64];
	char ap_auth_key[64];
	char ap_channel;
}TcWifiConfig;

static char *auth_mode[] = {
	"OPEN",
	"SHARED",
	"WPAPSK",
	"WPA8021X",
	"WPA2PSK",
	"WPA28021X",
	"WPAPSKWPA2PSK",
	"AWSS_AUTH_TYPE_WPAPSKWPA2PSK",
};

static char *encrypt_type[] = {
	"NONE",
	"WEP",
	"TKIP",
	"AES",
	"TKIPAES",
	"AWSS_ENC_TYPE_TKIPAES",
};

static TcWifiConfig tc_wifi_config = {
	.boot_proto = "DHCP",
	.network_type = "Infra",
	.ssid = "aha",
	.auth_mode = "OPEN",
	.encrypt_type = "NONE",
	.auth_key = "TC.86kb.com",

	.ap_addr = "192.168.100.1",
	.ap_ssid = "AliGateWay",
	.ap_auth_mode = "OPEN",
	.ap_encrypt_type = "NONE",
	.ap_auth_key = "12345678",
	.ap_channel = 11,
};

static void tcSetNetwork(int type)
{
	FILE *fp;
	printf("[%s]\n",__FUNCTION__);
	fp = fopen("network_config","wb");	
	if (fp == NULL) {
		printf("Can't open network_config\n");
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
		excuteCmd("./network.sh","SoftAP",NULL);
	} else {
		excuteCmd("./network.sh","Infra",NULL);
	}
}

//一键配置超时时间, 建议超时时间1-3min, APP侧一键配置1min超时
int platform_awss_get_timeout_interval_ms(void)
{
    return 1 * 60 * 1000;
}

//默认热点配网超时时间
int platform_awss_get_connect_default_ssid_timeout_interval_ms(void)
{
    return 0;
}

//一键配置每个信道停留时间, 建议200ms-400ms
int platform_awss_get_channelscan_interval_ms(void)
{
    return 200;
}

//wifi信道切换，信道1-13
void platform_awss_switch_channel(char primary_channel,
                                  char secondary_channel, uint8_t bssid[ETH_ALEN])
{
	tc_wifi_config.ap_channel = primary_channel;
	char buf[256];
	snprintf(buf, sizeof(buf), "./iwconfig %s channel %d ",
			IFNAME,primary_channel);
	// excuteCmd(buf,NULL);
	// tcSetNetwork(TC_SET_AP);
}

int open_socket(void)
{
	printf("app----------------->[%s]\n", __FUNCTION__);
    int fd;
#if 0
    if (getuid() != 0) {
        err("root privilege needed!\n");
    }
#endif
    //create a raw socket that shall sniff
    fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    assert(fd >= 0);

    struct ifreq ifr;
    int sockopt = 1;

    memset(&ifr, 0, sizeof(ifr));

    /* set interface to promiscuous mode */
    strncpy(ifr.ifr_name, AP_IFNAME, sizeof(ifr.ifr_name));
    if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
        perror("ioctl(SIOCGIFFLAGS)");
        goto exit;
    }
    ifr.ifr_flags |= IFF_PROMISC;
    if (ioctl(fd, SIOCSIFFLAGS, &ifr) < 0) {
        perror("ioctl(SIOCSIFFLAGS)");
        goto exit;
    }

    /* allow the socket to be reused */
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                   &sockopt, sizeof(sockopt)) < 0) {
        perror("setsockopt(SO_REUSEADDR)");
        goto exit;
    }

    /* bind to device */
    struct sockaddr_ll ll;

    memset(&ll, 0, sizeof(ll));
    ll.sll_family = PF_PACKET;
    ll.sll_protocol = htons(ETH_P_ALL);
    ll.sll_ifindex = if_nametoindex(WLAN_IFNAME);
    if (bind(fd, (struct sockaddr *)&ll, sizeof(ll)) < 0) {
        perror("bind[PF_PACKET] failed");
        goto exit;
    }

    return fd;
exit:
    close(fd);
    exit(EXIT_FAILURE);
}

pthread_t monitor_thread;
char monitor_running;

void *monitor_thread_func(void *arg)
{
	printf("app----------------->[%s]\n", __FUNCTION__);
    platform_awss_recv_80211_frame_cb_t ieee80211_handler = arg;
    /* buffer to hold the 80211 frame */
    char *ether_frame = malloc(IP_MAXPACKET);
    assert(ether_frame);

    int fd = open_socket();
    int len, ret;
    fd_set rfds;
    struct timeval tv;

    while (monitor_running) {
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);

        tv.tv_sec = 0;
        tv.tv_usec = 100 * 1000;//100ms

        ret = select(fd + 1, &rfds, NULL, NULL, &tv);
        assert(ret >= 0);

        if (!ret) {
            continue;
        }

        //memset(ether_frame, 0, IP_MAXPACKET);
        len = recv(fd, ether_frame, IP_MAXPACKET, 0);
        if (len < 0) {
            perror("recv() failed:");
            //Something weird happened
            continue;
        }

        /*
         * Note: use tcpdump -i wlan0 -w file.pacp to check link type and FCS
         */

        /* rtl8188: include 80211 FCS field(4 byte) */
        int with_fcs = 1;
        /* rtl8188: link-type IEEE802_11_RADIO (802.11 plus radiotap header) */
        int link_type = AWSS_LINK_TYPE_80211_RADIO;

        (*ieee80211_handler)(ether_frame, len, link_type, with_fcs);
    }

    free(ether_frame);
    close(fd);

    return NULL;
}

//进入monitor模式, 并做好一些准备工作，如
//设置wifi工作在默认信道6
//若是linux平台，初始化socket句柄，绑定网卡，准备收包
//若是rtos的平台，注册收包回调函数aws_80211_frame_handler()到系统接口
void platform_awss_open_monitor(platform_awss_recv_80211_frame_cb_t cb)
{
    int ret;

	printf("app----------------->[%s]\n", __FUNCTION__);
    assert(cb);
	tcSetNetwork(TC_SET_AP);

    monitor_running = 1;

    ret = pthread_create(&monitor_thread, NULL, monitor_thread_func, cb);
    assert(!ret);
}

//退出monitor模式，回到station模式, 其他资源回收
void platform_awss_close_monitor(void)
{
	printf("app----------------->[%s]\n", __FUNCTION__);
    monitor_running = 0;

    pthread_join(monitor_thread, NULL);

	// excuteCmd("ifconfig",AP_IFNAME,"down",NULL);
	// excuteCmd("killall","hostapd",NULL);
	// excuteCmd("killall","dnsmasq",NULL);
}

static int sys_net_is_ready = 0;

int platform_sys_net_is_ready(void)
{
	printf("app----------------->[%s],%d\n", __FUNCTION__,sys_net_is_ready);
    return sys_net_is_ready;
}

static TcWifiScan ap_info[100] ;
static int ap_cnt = 0;
int platform_awss_connect_ap(
            _IN_ uint32_t connection_timeout_ms,
            _IN_ char ssid[PLATFORM_MAX_SSID_LEN],
            _IN_ char passwd[PLATFORM_MAX_PASSWD_LEN],
            _IN_OPT_ enum AWSS_AUTH_TYPE auth,
            _IN_OPT_ enum AWSS_ENC_TYPE encry,
            _IN_OPT_ uint8_t bssid[ETH_ALEN],
            _IN_OPT_ uint8_t channel)
{
	char buf[256];
	char *ret;
	int cnt = 50;
	int i;
	sys_net_is_ready = 0;
	for (i=0; i<ap_cnt; i++) {
		if (strcmp(ssid,ap_info[i].ssid) == 0) {
			if (ap_info[i].auth < AWSS_AUTH_TYPE_INVALID)
				sprintf(tc_wifi_config.auth_mode,"%s",auth_mode[ap_info[i].auth]);
			if (ap_info[i].encry < AWSS_ENC_TYPE_MAX)
				sprintf(tc_wifi_config.encrypt_type,"%s",encrypt_type[ap_info[i].encry]);
			printf("app----------------->[%s]auth:%d,encry:%d\n", 
					__FUNCTION__,ap_info[i].auth,ap_info[i].encry);
		}
	}
	sprintf(tc_wifi_config.ssid,"%s",ssid);
	sprintf(tc_wifi_config.auth_key,"%s",passwd);
	// tc_wifi_config.ap_channel = channel;
	tcSetNetwork(TC_SET_STATION);

	snprintf(buf, sizeof(buf), "./wpa_cli -p %s -i %s status | grep wpa_state",
			WPA_PATH, WLAN_IFNAME);
	do {
		ret = excuteCmd(buf,NULL);
		usleep(100 * 1000);
	} while ((strncmp(ret,"wpa_state=COMPLETED",strlen("wpa_state=COMPLETED")) != 0) && --cnt != 0);

	sys_net_is_ready = 1;
	snprintf(buf, sizeof(buf), "udhcpc -i %s", WLAN_IFNAME);
	ret = (char *)system(buf);
	printf("system:%s\n",ret);

    //TODO: wait dhcp ready here
    return 0;
}

int platform_wifi_scan(platform_wifi_scan_result_cb_t cb)
{
	int i;
	printf("app----------------->[%s]\n", __FUNCTION__);
	char *cmd[] = { "gw", "wlan0", "scan" };
	memset(ap_info,0,sizeof(TcWifiScan)*100);
	iwlist(3,cmd,&ap_info,&ap_cnt);
    for (i=0; i<ap_cnt; i++) {
        int is_last_ap = 0;
        if(i == ap_cnt - 1)
            is_last_ap = 1;
		cb(ap_info[i].ssid,
				ap_info[i].bssid,
				ap_info[i].auth,
				ap_info[i].encry,
				ap_info[i].channel,
				ap_info[i].rssi,
				is_last_ap);
    } 
    return 0;
}

p_aes128_t platform_aes128_init(
            const uint8_t *key,
            const uint8_t *iv,
            AES_DIR_t dir)
{
    platform_aes_t *aes = malloc(sizeof(platform_aes_t));
    memset(aes, 0, sizeof(platform_aes_t));

    if (dir == PLATFORM_AES_ENCRYPTION)
        AES_set_encrypt_key(key, 128, &aes->ctx);
    else
        AES_set_decrypt_key(key, 128, &aes->ctx);

    memcpy(aes->iv, iv, 16);

    return aes;
}

int platform_aes128_destroy(
            p_aes128_t aes)
{
    free(aes);
    return 0;
}

int platform_aes128_cbc_encrypt(
            p_aes128_t aes,
            const void *src,
            size_t blockNum,
            void *dst)
{
    int i = 0;
    for (i = 0; i < blockNum; i++) {
        AES_cbc_encrypt(src, dst, AES_BLOCK_SIZE, &((platform_aes_t *)aes)->ctx, ((platform_aes_t *)aes)->iv, AES_ENCRYPT);
        src += AES_BLOCK_SIZE;
        dst += AES_BLOCK_SIZE;
    }

    return 0;
}

int platform_aes128_cbc_decrypt(
            p_aes128_t aes,
            const void *src,
            size_t blockNum,
            void *dst)
{
    int i = 0;
    for (i = 0; i < blockNum; i++) {
        AES_cbc_encrypt(src, dst, AES_BLOCK_SIZE, &((platform_aes_t *)aes)->ctx, ((platform_aes_t *)aes)->iv, AES_DECRYPT);
        src += AES_BLOCK_SIZE;
        dst += AES_BLOCK_SIZE;
    }

    return 0;
}


p_aes128_t platform_aes256_init(
            const uint8_t *key,
            const uint8_t *iv,
            AES_DIR_t dir)
{
    platform_aes_t *aes = malloc(sizeof(platform_aes_t));
    memset(aes, 0, sizeof(platform_aes_t));

    if (dir == PLATFORM_AES_ENCRYPTION)
        AES_set_encrypt_key(key, 256, &aes->ctx);
    else
        AES_set_decrypt_key(key, 256, &aes->ctx);

    memcpy(aes->iv, iv, 32);

    return aes;
}

int platform_aes256_destroy(
            p_aes128_t aes)
{
    free(aes);
    return 0;
}

int platform_aes256_cbc_encrypt(
            p_aes128_t aes,
            const void *src,
            size_t blockNum,
            void *dst)
{
    int i = 0;
    for (i = 0; i < blockNum; i++) {
        AES_cbc_encrypt(src, dst, AES_BLOCK_SIZE, &((platform_aes_t *)aes)->ctx, ((platform_aes_t *)aes)->iv, AES_ENCRYPT);
        src += AES_BLOCK_SIZE;
        dst += AES_BLOCK_SIZE;
    }

    return 0;
}

int platform_aes256_cbc_decrypt(
            p_aes128_t aes,
            const void *src,
            size_t blockNum,
            void *dst)
{
    int i = 0;
    for (i = 0; i < blockNum/2; i++) {
        AES_cbc_encrypt(src, dst, AES_BLOCK_SIZE * 2, &((platform_aes_t *)aes)->ctx, ((platform_aes_t *)aes)->iv, AES_DECRYPT);
        src += (AES_BLOCK_SIZE * 2);
        dst += (AES_BLOCK_SIZE * 2);
    }

    return 0;
}


int platform_wifi_get_ap_info(
            char ssid[PLATFORM_MAX_SSID_LEN],
            char passwd[PLATFORM_MAX_PASSWD_LEN],
    uint8_t bssid[ETH_ALEN])
{
	printf("app----------------->[%s]ssid:%s,password:%s,bssid:%s\n", 
			__FUNCTION__,ssid,passwd,bssid);
	if (ssid)
		memcpy(ssid,tc_wifi_config.ap_ssid,PLATFORM_MAX_SSID_LEN);
	if (passwd)
		memcpy(passwd,tc_wifi_config.ap_auth_key,PLATFORM_MAX_PASSWD_LEN);
    struct ifreq ifreq;
    int sock = -1;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return -1;
    }
    strcpy(ifreq.ifr_name, IFNAME);

    if (ioctl(sock, SIOCGIFHWADDR, &ifreq) < 0) {
        close(sock);
        perror("ioctl");
        return -1;
    }

	if (bssid)
		memcpy(bssid,ifreq.ifr_hwaddr.sa_data,ETH_ALEN);

    close(sock);
	return 0;
}


int platform_wifi_low_power(int timeout_ms)
{
	printf("app----------------->[%s]\n", __FUNCTION__);
    //wifi_enter_power_saving_mode();
    usleep(timeout_ms);
    //wifi_exit_power_saving_mode();

    return 0;
}

/**
 * @brief enable/disable filter specific management frame in wifi station mode
 *
 * @param[in] filter_mask @n see mask macro in enum platform_awss_frame_type,
 *                      currently only FRAME_PROBE_REQ_MASK & FRAME_BEACON_MASK is used
 * @param[in] vendor_oui @n oui can be used for precise frame match, optional
 * @param[in] callback @n see platform_wifi_mgnt_frame_cb_t, passing 80211
 *                      frame or ie to callback. when callback is NULL
 *                      disable sniffer feature, otherwise enable it.
 * @return
   @verbatim
   =  0, success
   = -1, fail
   = -2, unsupported.
   @endverbatim
 * @see None.
 * @note awss use this API to filter specific mgnt frame in wifi station mode
 */
int platform_wifi_enable_mgnt_frame_filter(
            _IN_ uint32_t filter_mask,
            _IN_OPT_ uint8_t vendor_oui[3],
            _IN_ platform_wifi_mgnt_frame_cb_t callback)
{
	// printf("app----------------->[%s]\n", __FUNCTION__);
    return -2;
}

/**
 * @brief send 80211 raw frame in current channel with basic rate(1Mbps)
 *
 * @param[in] type @n see enum platform_awss_frame_type, currently only FRAME_BEACON
 *                      FRAME_PROBE_REQ is used
 * @param[in] buffer @n 80211 raw frame, include complete mac header & FCS field
 * @param[in] len @n 80211 raw frame length
 * @return
   @verbatim
   =  0, send success.
   = -1, send failure.
   = -2, unsupported.
   @endverbatim
 * @see None.
 * @note awss use this API send raw frame in wifi monitor mode & station mode
 */
int platform_wifi_send_80211_raw_frame(_IN_ enum platform_awss_frame_type type,
                                       _IN_ uint8_t *buffer, _IN_ int len)
{
    int bytes_sent;
	struct sockaddr addr;
	memset(&addr,0,sizeof(addr));
	int fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    struct ifreq ifr;
    int sockopt = 1;

    memset(&ifr, 0, sizeof(ifr));

    /* set interface to promiscuous mode */
    strncpy(ifr.ifr_name, IFNAME, sizeof(ifr.ifr_name));
    if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
        perror("ioctl(SIOCGIFFLAGS)");
        return -2;
    }
    ifr.ifr_flags |= IFF_PROMISC;
    if (ioctl(fd, SIOCSIFFLAGS, &ifr) < 0) {
        perror("ioctl(SIOCSIFFLAGS)");
        return -2;
    }

    /* allow the socket to be reused */
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                   &sockopt, sizeof(sockopt)) < 0) {
        perror("setsockopt(SO_REUSEADDR)");
        return -2;
    }
    /* bind to device */
    struct sockaddr_ll ll;

    memset(&ll, 0, sizeof(ll));
    ll.sll_family = PF_PACKET;
    ll.sll_protocol = htons(ETH_P_ALL);
    ll.sll_ifindex = if_nametoindex(WLAN_IFNAME);


    bytes_sent = sendto((long)fd, buffer, len, 0,(struct sockaddr*)&ll,sizeof(struct sockaddr_ll));

    close(fd);
	// printf("app----------------->[%s]fd:%d,type:%d,len:%d,ret:%d\n", 
			// __FUNCTION__,fd,len,type,bytes_sent);
	return 0;
    // return bytes_sent > 0 ? 0 : -1;
    // return 0;
}

