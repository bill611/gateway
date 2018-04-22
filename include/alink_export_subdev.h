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

/* header file for common sub device */
#ifndef _ALINK_EXPORT_SUBDEV_H_
#define _ALINK_EXPORT_SUBDEV_H_

#if defined(__cplusplus) /* If this is a C++ compiler, use C linkage */
extern "C"
{
#endif

#define STR_NAME_LEN                (32)
#define MAX_SN_BYTES                (8)
#define SUBDEV_RAND_BYTES           (16)

/** @defgroup sub_device callback
 *  @{
 */

/**
 * sub device event, used by alink_register_callback function
 */
enum SUB_DEVICE_CALLBACK {
    /**
     * int get_sub_device_attr_cb(const char *dev_id, const char *attr_name[]);
     */
    GET_SUB_DEVICE_ATTR = 0,

    /**
     * int set_sub_device_attr_cb(const char *dev_id, const char *attr_name, const char *attr_value);
     */
    SET_SUB_DEVICE_ATTR,

    /**
     * int execute_sub_device_cmd_cb(const char *dev_id, const char *cmd_name, const char *cmd_args);
     *
     * @brief execute device cmd(service)
     *
     * @param[in] dev_id: device id
     * @param[in] cmd_name: command name
     * @param[in] cmd_args: command params, json format
     * @retval  0 on success, otherwise -1 will return
     */
    EXECUTE_SUB_DEVICE_CMD,

    /**
     * int remove_sub_device_cb(const char *dev_id);
     *
     * @brief remove sub device's from network
     * @param[in] dev_id: device id
     * @retval  0 on success, otherwise -1 will return
     * @see None.
     * @note None.
     */
    REMOVE_SUB_DEVICE,

	/**
     * int permit_sub_device_join_cb(uint8_t duration);
     *
     * @brief permit zigbee sub device join to zigbee network
     * @param[in] duration: permit join duration time, unit: second
	 * 					duration: 0, disable join; 255, enable join forever
     * @retval  0 on success, otherwise -1 will return
     * @see None.
     * @note None.
     */
    PERMIT_JOIN_SUB_DEVICE,

    MAX_SUB_DEVICE_CALLBACK
};

/** @} */ //end of callback

/*
* subdevice protocol type
*/
typedef enum{
    PROTO_TYPE_WIFI           = 1,
    PROTO_TYPE_ZIGBEE         = 2,
    PROTO_TYPE_BLE            = 3,
    PROTO_TYPE_WIFI_SUB       = 4,
    PROTO_TYPE_WIFI_MESH      = 5,
    PROTO_TYPE_MODBUS         = 6,
    PROTO_TYPE_OTHER          = 7,
    PROTO_TYPE_MAX            = 32,
}proto_type_t;


typedef struct {
    unsigned char proto_type;
    char protocol_name[STR_NAME_LEN];
    struct {
        unsigned char cb_type;
        void *cb_func;
    } callback[MAX_SUB_DEVICE_CALLBACK];
}proto_info_t;


/** @defgroup sub device register & report status
 *  @{
 */

/**
 * @brief register and authentication sub device to aliyun server
 *
 * @param[in] proto_type: 1 bytes device protocol type
 * @param[in] dev_id: device id, maximum length 16 bytes
 * @param[in] short_model: 2 bytes device model identification
 * @param[in] rand: 16 bytes random binary array
 * @param[in] sign: 16 bytes characters of signature, end with '\0', md5(rand|secret)
 * @retval 0 on success, -1 when params invalid
 * @see None.
 * @note when joined network, invoke this function to register sub device
 */
int alink_subdev_register_device(unsigned char proto_type, const char *dev_id,
        unsigned int short_model, const char rand[SUBDEV_RAND_BYTES],
        const char *sign);


/**
 * @brief unregister sub device to aliyun server
 *
 * @param[in] proto_type: 1 bytes device protocol type
 * @param[in] dev_id: device id, maximum length 16 bytes
 * @retval 0 on success, -1 when params invalid
 * @see None.
 * @note when factory reset, invoke this function to unregister sub device
 */
int alink_subdev_unregister_device(unsigned char proto_type, const char *dev_id);

/**
 * @brief update sub device online status, online or offline
 *
 * @param[in] proto_type: 1 bytes device protocol type
 * @param[in] dev_id: device id, maximum length 16 bytes
 * @param[in] online_or_not: 1 online; 0 offline
 * @retval 0 on success, -1 when params invalid
 * @see None.
 * @note alink sdk will keep syncing subdev status with aliyun server
 */
int alink_subdev_update_online_status(unsigned char proto_type, const char *dev_id, char online_or_not);

/**
 * @brief report sub device attribute list to server, at least 1 attribute should be include
 *
 * @param[in] proto_type: 1 bytes device protocol type
 * @param[in] dev_id: device id, maximum length 16 bytes
 * @param[in] attr_name[]: NULL-terminated array which hold the attr name list
 * @param[in] attr_value[]: NULL-terminated array which hold the attr value list,
 *            attr_name[x] & attr_value[x] is a key-value pair.
 * @retval 0 when successfully got response from cloud,
 *          otherwise this func will block until timeout and -1 will return
 * @see None.
 * @note when connection with server is unstable, this func will block
 *      until got response from server or timeout.
 */
int alink_subdev_report_attrs(unsigned char proto_type, const char *dev_id, const char *attr_name[],
        const char *attr_value[]);

/**
 * @brief report sub device event of device to aliyun server
 *
 * @param[in] proto_type: 1 bytes device protocol type
 * @param[in] dev_id: device id, maximum length 16 bytes
 * @param[in] event_name: event name
 * @param[in] event_args: event params, json format
 * @retval 0 when successfully got response from cloud,
 *          otherwise this func will block until timeout and -1 will return
 * @see None.
 * @note when connection with server is unstable, this func will block
 *      until got response from server or timeout.
 */
int alink_subdev_report_event(unsigned char proto_type, const char *dev_id, const char *event_name,
        const char *event_args);


/**
 * @brief register sub device protocol type
 *
 * @param[in] devtype_info: sub device type information
 *
 * @retval  0 on success, otherwise -1 will return
 * @see enum ALINK_GATEWAY_CALLBACK, ALINK_ZIGBEE_CALLBACK.
 * @note None.
 */
int alink_subdev_register_device_type(proto_info_t *proto_info);


/**
 * @brief get sub device uuid by proto_type & device id
 *
 * @param[in] proto_type: 1 bytes device protocol type
 * @param[in] dev_id: device id, maximum length 16 bytes
 * @param[out] uuid_buf: buffer to get uuid
 * @param[int] buf_sz: The length of the buffer, must be greater than 32
 * @retval 0 when successfully got uuid,
 *          otherwise return -1
 * @see None.
 * @note None
 */
int alink_subdev_get_uuid(unsigned char proto_type, const char *dev_id, char *uuid_buf, int buf_sz);


/** @} */ //end of common sub device export api

#if defined(__cplusplus) /* If this is a C++ compiler, use C linkage */
}
#endif

#endif
