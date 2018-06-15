/*
 * =============================================================================
 *
 *       Filename:  sqlHandle.h
 *
 *    Description:  数据库操作接口
 *
 *        Version:  1.0
 *        Created:  2018-05-21 22:48:57 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _SQL_HANDLE_H
#define _SQL_HANDLE_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include <stdint.h>
	extern int sqlGetDeviceStart(void);
	extern void sqlGetDevice(char *id,
			int *dev_type,
			uint16_t *addr,
			uint16_t *channel);
	extern void sqlGetDeviceEnd(void);
	extern void sqlInsertDevice(char *id,
			int dev_type,
			uint16_t addr,
			uint16_t channel);
	extern void sqlDeleteDevice(char *id);
	extern int sqlGetDeviceId(uint16_t addr,char *id);
	extern void sqlInit(void);
	extern void sqlClearDevice(void);


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
