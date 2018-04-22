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

/* header file for alink gateway device */
#ifndef _ALINK_EXPORT_GATEWAY_H_
#define _ALINK_EXPORT_GATEWAY_H_

#if defined(__cplusplus) /* If this is a C++ compiler, use C linkage */
extern "C"
{
#endif

/** @defgroup gateway_device gateway API
 *  @{
 */


/** @defgroup gateway_debug debug
 *  @{
 */

/**
 * log level def.
 */
enum ALINK_LOG_LEVEL {
    ALINK_LL_NONE, /**< disable log */
    ALINK_LL_FATAL, /**< fatal error log will output */
    ALINK_LL_ERROR, /**< error + fatal log will output */
    ALINK_LL_WARN, /**< warn + error + fatal log will output(default level) */
    ALINK_LL_INFO, /**< info + warn + error + fatal log will output */
    ALINK_LL_DEBUG, /**< debug + info + warn + error + fatal log will output */
    ALINK_LL_TRACE, /**< trace + debug + info + warn + error + fatal will output */
};

/**
 * @brief log level control
 *
 * @param[in] loglevel
 * @return None.
 * @see enum ALINK_LOG_LEVEL.
 * @note None.
 */
void alink_set_loglevel(enum ALINK_LOG_LEVEL loglevel);

/**
 * @brief enable sandbox mode, for debug
 *
 * @param None.
 * @retval 0 on success, otherwise -1 will return
 * @see None.
 * @note None.
 */
int alink_enable_sandbox_mode(void);

/**
 * @brief enable prepub mode, for cloud side pre-publish debug
 *
 * @param None.
 * @retval 0 on success, otherwise -1 will return
 * @see None.
 * @note None.
 */
int alink_enable_prepub_mode(void);

/**
 * @brief enable daily mode, for debug
 *
 * @param None.
 * @retval 0 on success, otherwise -1 will return
 * @see None.
 * @note None.
 */
int alink_enable_daily_mode(const char *server_ip, int port);

/** @} */ //end of debug


/** @defgroup gateway_entry main
 *  @{
 */

enum ALINK_AUTH_MODE {
    ALINK_AUTH_MODE_DEFAULT,        /**< default authentication mode */
    ALINK_AUTH_MODE_SDS_DEVICE_ID,  /**< SDS with deviceId  authentication mode */
    ALINK_AUTH_MODE_SDS_WHITELIST,  /**< SDS with whitelist authentication mode */
};

/**
 * @brief alink authentication mode
 *
 * @param[in] authentication mode.
 * @return None.
 * @retval 0 on success, otherwise -1 will return
 * @see enum ALINK_AUTH_MODE.
 * @note None.
 */
int alink_set_auth_mode(enum ALINK_AUTH_MODE mode);

/**
 * @brief alink authentication mode
 *
 * @param[in] None.
 * @return None.
 * @retval authentication mode.
 * @see enum ALINK_AUTH_MODE.
 * @note None.
 */
enum ALINK_AUTH_MODE alink_get_auth_mode(void);



/**
 * @brief entry function. start alink gateway service.
 *
 * @param None.
 * @retval 0 on success, otherwise -1 will return
 * @see None.
 * @note None.
 */
int alink_start(void);

#define ALINK_WAIT_FOREVER      (0)
/**
 * @brief waiting alink connect to aliyun server
 *
 * @param[in] timeout_ms: time in milliseconds,
 *              use ALINK_WAIT_FOREVER to wait until aliyun server is connected
 * @retval 0 when connect to server successfully, otherwise -1 will return
 * @see None.
 * @note None.
 */
int alink_wait_connect(int timeout_ms);

/**
 * @brief destroy alink service and free resources
 *
 * @param None.
 * @retval 0 on success, otherwise -1 will return
 * @see None.
 * @note this func will block at most 15 seconds to
 *      stop all alink related process(thread)
 */
int alink_end(void);

/** @} */ //end of entry

/** @defgroup gateway_feature register attr & service
 *  @{
 */

/**
 * @brief device attribute get callback function type definition
 * @param[in] buf: buffer to get attribute value
 * @param[in] buf_sz: buffer to get attribute value
 * @retval 0 on success, otherwise -1 will return
 */
typedef int (*ALINK_ATTR_GET_CB)(char *buf, unsigned int buf_sz);

/***
 * @brief device attribute set callback function type definition
 * @param[in] json_in: attribute value to set, end by '\0'
 * @retval 0 on success, otherwise -1 will return
 */
typedef int (*ALINK_ATTR_SET_CB)(char *json_in);

/***
 * @brief register gateway's attribute
 * @param[in] name: the name of the attribute
 * @param[in] get_cb: callback function to get the value of the attribute
 * @param[in] set_cb: callback function to set the value of the attribute
 * @retval 0 on success, otherwise -1 will return
 */
int alink_register_attribute(const char *name,
        ALINK_ATTR_GET_CB get_cb,
        ALINK_ATTR_SET_CB set_cb);
/***
 * @brief device service execute callback function type definition
 * @param[in] json_in: input parameters, end by '\0'
 * @param[in] json_out_buf: buffer to get output parameters
 * @param[in] buf_sz: buffer size
 * @retval 0 on success, otherwise -1 will return
 */
typedef int (*ALINK_SERVICE_EXECUTE_CB)(char *json_in,
        char *json_out_buf, unsigned int buf_sz);

/***
 * @brief register gateway's service
 * @param[in] name: the name of the service
 * @param[in] exec_cb: callback function to execute the service
 * @retval 0 on success, otherwise -1 will return
 * @Note service list defined by alink.
 */
int alink_register_service(const char *name, ALINK_SERVICE_EXECUTE_CB exec_cb);

/***
 * @brief unregister gateway's service
 * @param[in] name: the name of the service
 * @retval 0 on success, otherwise -1 will return
 */
int alink_unregister_service(const char *name);

enum ALINK_REBOOT_FLAG {
    ALINK_NOT_REBOOT,
    ALINK_REBOOT
};

/**
 * @brief reset user account binding.
 * @param[in] rebootflag: 1-reboot after factory reset ,0-will not reboot
 * @retval 0 on success, -1 when params invalid
 * @see None.
 * @note None.
 */
int alink_factory_reset(enum ALINK_REBOOT_FLAG rebootflag);


/**
 * @brief report alink message, it is a fundamental func.
 *
 * @param[in] method: alink protocol method,
 *          i.e. "postDeviceRawData", "postDeviceData" "setDeviceProperty".
 * @param[in] json_buffer: json format buffer, like
 *              {"OnOff":"1", "Light":"80"} for ""postDeviceData",
 *              or {"rawData":"F5F5F5F50100041501","length":"20"} for "postDeviceRawData"
 * @retval 0 when successfully got response from cloud,
 *          otherwise this func will block until timeout and -1 will return
 * @see None.
 * @note when connection with server is unstable, this func will block
 *      until got response from server or timeout.
 */
int alink_report(const char *method, const char *json_buffer);


/**
 * @brief query alink cloud service, like getAlinkTime...
 *
 * @param[in] method: alink protocol method,
 *          i.e. "getAlinkTime", "retrieveDeviceData" "getDeviceProperty".
 * @param[in] json_buffer: json format buffer, like {} for "getAlinkTime",
 *              or {"dataKey":"param_probe_num"} for "retrieveDeviceData"
 * @param[in] result_buf: to hold json string return from cloud
 * @param[in/out] buf_len: [in] size of result_buf, [out] strlen of json string
 *              return from cloud
 * @retval 0 when successfully got response from cloud,
 *          otherwise this func will block until timeout and -1 will return
 * @see None.
 * @note when connection with server is unstable, this func will block
 *      until got response from server or timeout.
 */
int alink_query(const char *method, const char *json_buffer,
        char *result_buf, int *buf_len);

/** @} */ //end of gateway feature



/** @defgroup gateway_status report status
 *  @{
 */

/**
 * @brief report gateway's attr list to server, at least 1 attr will be include
 *
 * @param[in] attr_name[]: NULL-terminated array which hold the attr name list
 * @retval 0 when successfully got response from cloud,
 *          otherwise this func will block until timeout and -1 will return
 * @see None.
 * @note when connection with server is unstable, this func will block
 *      until got response from server or timeout.
 */
int alink_report_attrs(const char *attr_name[]);


/**
 * @brief get gateway's uuid.
 *
 * @param[out] uuid_buf: buffer to get uuid
 * @param[int] buf_sz: The length of the buffer, must be greater than 32
 * @retval 0 when successfully got uuid,
 *          otherwise return -1
 * @see None.
 * @note None.
 */
int alink_get_main_uuid(char *uuid_buf, int buf_sz);


/** @} */ //end of status

/** @defgroup gateway_callback    callback
 *  @{
 */

/**
 * alink callback event
 */
enum ALINK_GATEWAY_CALLBACK {
    /**
     * void callback_cloud_connected(void)
     * @n@n called each time when gateway successfully connect(or reconnect)
     * to aliyun server
     */
    ALINK_CLOUD_CONNECTED = 0,

    /**
     * void callback_cloud_disconnected(void)
     * @n@n called each time when gateway lose connection with aliyun server
     */
    ALINK_CLOUD_DISCONNECTED,
};

/**
 * @brief register misc callback
 *
 * @param[in] cb_type: callback type.
 * @param[in] cb_func: callback func pointer, func declaration see related comments.
 *
 * @retval  0 on success, otherwise -1 will return
 * @see enum ALINK_GATEWAY_CALLBACK, ALINK_ZIGBEE_CALLBACK.
 * @note None.
 */
int alink_register_callback(unsigned char cb_type, void *cb_func);
/** @} */ //end of callback

/** @defgroup gateway_awss awss
 *  @{
 */

/**
 * @brief start awss service, block method,
 *      block until awss succeed, or timeout(default 1min).
 *
 * @retval  0 on success, otherwise -1 will return
 * @see None.
 * @note None.
 */
int awss_start(void);

/**
 * @brief exit awss
 *      calling this func force awss_start exit.
 *
 * @param None.
 * @retval  0 on success, otherwise -1 will return
 * @see None.
 * @note None.
 */
int awss_end(void);

/** @} */ //end of awss

/** @} */ //end of gateway api

#if defined(__cplusplus) /* If this is a C++ compiler, use C linkage */
}
#endif

#endif
