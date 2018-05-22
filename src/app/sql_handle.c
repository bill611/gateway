/*
 * =============================================================================
 *
 *       Filename:  sqlHandle.c
 *
 *    Description:  数据库存储操作接口
 *
 *        Version:  1.0
 *        Created:  2018-05-21 22:48:19
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
/* ---------------------------------------------------------------------------*
 *                      include head files
 *----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>
#include "sqlite3.h"
#include "sqlite.h"
#include "sql_handle.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/
static int sqlCheck(TSqlite *sql);

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
TSqliteData sql_local = {
	.file_name = "device.db",
	.sql = NULL,
	.checkFunc = sqlCheck,
};

static int sqlCheck(TSqlite *sql)
{
    int ret;
    if (sql == NULL)
        goto sqlCheck_fail;

    ret = LocalQueryOpen(sql,"select ID from DeviceList limit 1");
    sql->Close(sql);
    if (ret == 1)
        return TRUE;

sqlCheck_fail:
    printf("sql locoal err\n");
	return FALSE;
}

void sqlInsertDevice(char *id,
		int dev_type,
		uint16_t addr,
		uint16_t Channel)
{
	char buf[128];
	sprintf(buf, "INSERT INTO DeviceList(ID,DevType,Addr) VALUES('%s','%d','%d','%d')",
			id, dev_type,addr,Channel);
	LocalQueryExec(sql_local.sql,buf);
	sql_local.checkFunc(sql_local.sql);
	sync();
}

void sqlInit(void)
{
	LocalQueryLoad(&sql_local);
	sql_local.checkFunc(sql_local.sql);
	if (!sql_local.sql)
		printf("sql err\n");
}
