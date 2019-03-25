/*
 * =============================================================================
 *
 *       Filename:  gateway.h
 *
 *    Description:  网关设备
 *
 *        Version:  1.0
 *        Created:  2018-05-08 16:27:39 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _GATEWAY_H
#define _GATEWAY_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */


	extern int gwRegisterSubDevice(char *id,
			int type,
			uint16_t addr,
			uint16_t channel,
			char *pk,
			int add_sql);
	extern void gwReportPowerOn(char *id,char *param,int channel);
	extern void gwReportPowerOff(char *id,int channel);
	extern void gwReportAlarmStatus(char *id,char *param);
	extern void gwReportEleQuantity(char *id,char *param);
	extern void gwReportElePower(char *id,char *param);
	extern void gwReportAirPara(char *id,char *param);
	extern void gwGetSwichStatus(void);
	extern void gwLoadDeviceData(void);
	extern void gwDeviceInit(void);
	extern int gwRegisterGateway(void);
	extern int gwRegisterGatewayAttribute(void);
	extern int gwDeviceReportRegist(void);
	extern char *gwGetTempProductKey(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
