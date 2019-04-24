#ifndef CONFIG_H
#define CONFIG_H

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IFNAME                      "wlan0"

/* awss used this interface to sniffer 80211 package */
#define WLAN_IFNAME                 "wlan0"
#define AP_IFNAME                 	"wlan1"
#define WPA_PATH                 	"/tmp/wpa_supplicant/"


#define GW_VERSION	"v2.0.2"
#if (defined ANYKA)
#define UPDATE_FILE	"/tmp/Update.cab"
#define KVFILE_DEFAULT_PATH "/mnt/private/kvfile.db"
#define CFG_PUBLIC_DRIVE "/mnt/private/"
#define CFG_PRIVATE_DRIVE "/mnt/private/"
#define CFG_TEMP_DRIVE "/mnt/private/"
#define DATABSE_PATH "/mnt/private/"
#else
#define UPDATE_FILE	"/mnt/nand1-2/Update.cab"
#define KVFILE_DEFAULT_PATH "/mnt/nand1-2/kvfile.db"
#define CFG_PUBLIC_DRIVE "/mnt/nand1-2/"
#define CFG_PRIVATE_DRIVE "/mnt/nand1-2/"
#define CFG_TEMP_DRIVE "/mnt/nand1-2/"
#define DATABSE_PATH 
#endif


	typedef struct _EtcValueChar {
		const char* section;	
		const char* key;
		char *value;
		unsigned int leng;
		const char *default_char;
	} EtcValueChar;

	typedef struct _EtcValueInt {
		const char* section;	
		const char* key;
		int *value;
		int default_int;
	} EtcValueInt;

	struct DevConfig{
		char device_name[64];
		char product_key[64];
		char device_secret[64];
	};
	typedef struct {
#if (defined V23)
        // ethernet
		char dhcp[3];
		char ipaddr[16];
		char netmask[16];
		char gateway[16];
		char macaddr[20];
		char firstdns[16];
		char backdns[16];

		// station 
		char ssid[64];
		char mode[64];
		char security[128];
		char password[64];
		char running[64];

		// ap
		char s_ssid[64];
		char s_password[64];
#else		
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
#endif
	}TcWifiConfig;

	extern TcWifiConfig tc_wifi_config;
	/**
	 * Configuration definition.
	 */
	typedef struct _Config {
        char imei[64];      // 太川设备机身码
        char version[16];      // 太川软件版本
		struct DevConfig gate_way;	//网关阿里相关参数
        TcWifiConfig net_config;  // 网络设置
	} Config;

	enum {
		TC_SET_STATION,
		TC_SET_AP,
	};

	/**
	 * Global instance variable of configuration.
	 */

	extern Config theConfig;

	/**
	 * Loads configuration file.
	 */
	void configLoad(void);

	/**
	 * Saves the configuration to file.
	 */
	void ConfigSave(void (*func)(void));
	void ConfigSavePrivate(void);
	void ConfigSaveTemp(void);
	void configSync(void);

	/** @defgroup doorbell_indoor_audio Audio Player
	 *  @{
	 */
	typedef int (*AudioPlayCallback)(int state);

	/**
	 * Initializes audio module.
	 */
	void AudioInit(void);

	/**
	 * Exits audio module.
	 */
	void AudioExit(void);

	/**
	 * Plays the specified wav file.
	 *
	 * @param filename The specified wav file to play.
	 * @param func The callback function.
	 * @return 0 for success, otherwise failed.
	 */
	int AudioPlay(char* filename, AudioPlayCallback func);

	/**
	 * Stops playing sound.
	 */
	void AudioStop(void);

	/**
	 * Plays keypad sound.
	 */
	void AudioPlayKeySound(void);
	void AudioPauseKeySound(void);
	void AudioResumeKeySound(void);

	/**
	 * Sets the volume of keypad sound.
	 *
	 * @param level the percentage of volume.
	 */
	void AudioSetKeyLevel(int level);

	/**
	 * Mutes all audio.
	 */
	void AudioMute(void);

	/**
	 * Un-mutes all audio.
	 */
	void AudioUnMute(void);

	/**
	 * Determines whether this audio is muted or not.
	 *
	 * @return true muted, false otherwise.
	 */
	bool AudioIsMuted(void);

	bool AudioIsPlaying(void);

	void AudioSetVolume(int level);
	int AudioGetVolume(void);

	/** @} */ // end of doorbell_indoor_audio

	/** @defgroup doorbell_indoor_audiorecord Audio Recorder
	 *  @{
	 */
	/**
	 * Initializes audio recorder module.
	 */
	void AudioRecordInit(void);

	/**
	 * Exits audio recorder module.
	 */
	void AudioRecordExit(void);

	/**
	 * Start recording to the specified file.
	 *
	 * @param filepath The specified file to record.
	 * @return 0 for success, otherwise failed.
	 */
	int AudioRecordStartRecord(char* filepath);

	/**
	 * Stop recording.
	 *
	 * @return 0 for success, otherwise failed.
	 */
	int AudioRecordStopRecord(void);

	/**
	 * Gets the time length of the specified recorded file, in seconds.
	 *
	 * @param filepath The specified file path.
	 * @return The time length, in seconds.
	 */
	int AudioRecordGetTimeLength(char* filepath);

	/**
	 * Plays the specified recorded file.
	 *
	 * @param filepath The specified recorded file to play.
	 * @return 0 for success, otherwise failed.
	 */
	int AudioRecordStartPlay(char* filepath);

	/**
	 * Stops playing recorded file.
	 */
	void AudioRecordStopPlay(void);

	/** @} */ // end of doorbell_indoor_audiorecord

	/**
	 * Stops audio player.
	 */
	void AudioPlayerStop(void);

	// sdk 调用
	// 获取3000或U9协议 
	int getConfigProtocol(void);
	// 0为U9协议 1为3000协议
	int isProtocolBZ(void);
	// 1为主机 0为分机
	int isConfigMaster(void);

	void tcSetNetwork(int type);
	extern char *auth_mode[];
	extern char *encrypt_type[];
#ifdef __cplusplus
}
#endif

#endif /* CONFIG_H */
