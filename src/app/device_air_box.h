/*
 * =============================================================================
 *
 *       Filename:  device_air_box.h
 *
 *    Description:  空气检测仪
 *
 *        Version:  1.0
 *        Created:  2018-05-09 08:47:22 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _DEVICE_AIRBOX_H
#define _DEVICE_AIRBOX_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include "device_protocol.h"
#include "smart_home_pro.h"

	extern DeviceStr * registDeviceAirBox(char *id,uint16_t addr,uint16_t channel,char *pk);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
