/*
 * =============================================================================
 *
 *       Filename:  debug.h
 *
 *    Description:  调试接口
 *
 *        Version:  1.0
 *        Created:  2016-11-26 22:38:57 
 *       Revision:  1.0
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _TC_DEBUG_H
#define _TC_DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include <stdint.h>
#define NELEMENTS(array)		/* number of elements in an array */ \
		(sizeof (array) / sizeof ((array) [0]))

#define DBG_FLAG(x) printf("flag------->[%ld]\n",x)
#define DBG_STR(x)  printf("flag------->[%s]\n",x)

		

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
