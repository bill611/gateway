/*
 * =============================================================================
 *
 *       Filename:  device_curtain.h
 *
 *    Description:  窗帘设备
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
#ifndef _DEVICE_CURTAIN_H
#define _DEVICE_CURTAIN_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include "device_protocol.h"
#include "smart_home_pro.h"

	extern DeviceStr * registDeviceCurtain(char *id,
		uint16_t addr,
		uint16_t channel,
		char *pk,
		RegistSubDevType regist_type);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
