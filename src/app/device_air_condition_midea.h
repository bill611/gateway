/*
 * =============================================================================
 *
 *       Filename:  device_air_condition_midea.h
 *
 *    Description:  美的空调设备
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
#ifndef _DEVICE_AIR_CONDITION_MIDEA_H
#define _DEVICE_AIR_CONDITION_MIDEA_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include "device_protocol.h"
#include "smart_home_pro.h"

	extern DeviceStr * registDeviceAirConditionMidea(char *id,uint16_t addr,uint16_t channel);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
