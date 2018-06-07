/*
 * =============================================================================
 *
 *       Filename:  device_outlet.h
 *
 *    Description:  灯控设备
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
#ifndef _DEVICE_OUTLET_H
#define _DEVICE_OUTLET_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include "device_protocol.h"
#include "smart_home_pro.h"

	extern DeviceStr * registDeviceOutlet(char *id,uint16_t addr,uint16_t channel);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
