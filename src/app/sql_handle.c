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
#include "externfunc.h"
#include "sqlite3.h"
#include "sqlite.h"
#include "sql_handle.h"
#include "debug.h"

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
	char *string = "CREATE TABLE  DeviceList(\
		ID char(32) PRIMARY KEY,\
		DevType INTEGER,\
		Addr INTEGER,\
		Channel INTEGER\
		EleQuantity INTEGER\
	   	)";
    if (sql == NULL)
        goto sqlCheck_fail;

    ret = LocalQueryOpen(sql,"select ID from DeviceList limit 1");
    sql->Close(sql);
	if (ret == 1) {
		backData(sql->file_name);
		return TRUE;
	}

sqlCheck_fail:
    DPRINT("sql locoal err\n");
	if (recoverData(sql_local.file_name) == 0) {
		DPRINT("creat new db\n");
		LocalQueryExec(sql_local.sql,string);
	} else {
		sql_local.sql->Destroy(sql_local.sql);
		sql_local.sql = CreateLocalQuery(sql_local.sql->file_name);
	}
	return FALSE;
}

int sqlGetDeviceStart(void)
{
	LocalQueryOpen(sql_local.sql,"select * from DeviceList ");
	return sql_local.sql->RecordCount(sql_local.sql);
}

void sqlGetDevice(char *id,
		int *dev_type,
		uint16_t *addr,
		uint16_t *channel)
{
	if (id)
		LocalQueryOfChar(sql_local.sql,"ID",id,32);
	if (dev_type)
		*dev_type = LocalQueryOfInt(sql_local.sql,"DevType");
	if (addr)
		*addr = LocalQueryOfInt(sql_local.sql,"Addr");
	if (channel)
		*channel = LocalQueryOfInt(sql_local.sql,"Channel");
	sql_local.sql->Next(sql_local.sql);
}
void sqlGetDeviceEnd(void)
{
	sql_local.sql->Close(sql_local.sql);
}

void sqlInsertDevice(char *id,
		int dev_type,
		uint16_t addr,
		uint16_t channel)
{
	char buf[128];
	sprintf(buf, "INSERT INTO DeviceList(ID,DevType,Addr,Channel) VALUES('%s','%d','%d','%d')",
			id, dev_type,addr,channel);
	LocalQueryExec(sql_local.sql,buf);
	sql_local.checkFunc(sql_local.sql);
	sync();
}

void sqlDeleteDevice(char *id)
{
	char buf[128];
	sprintf(buf, "Delete From DeviceList Where ID=\"%s\"", id);
	DPRINT("%s\n",buf);
	LocalQueryExec(sql_local.sql,buf);
	sql_local.checkFunc(sql_local.sql);
	sync();
}

void sqlClearDevice(void)
{
	char buf[128];
	sprintf(buf, "Delete From DeviceList");
	DPRINT("%s\n",buf);
	LocalQueryExec(sql_local.sql,buf);
	sql_local.checkFunc(sql_local.sql);
	sync();
}
int sqlGetDeviceId(uint16_t addr,char *id)
{
	char buf[128];
	sprintf(buf, "select ID From DeviceList Where Addr=\"%d\"", addr);
	LocalQueryOpen(sql_local.sql,buf);
	int ret = sql_local.sql->RecordCount(sql_local.sql);
	if (ret)
		LocalQueryOfChar(sql_local.sql,"ID",id,32);
	// DPRINT("ret:%d,id:%s\n", ret,id);
	sql_local.sql->Close(sql_local.sql);
	return ret;
}

void sqlSetEleQuantity(int value,char *id)
{
	char buf[128];
	sprintf(buf, "UPDATE DeviceList SET EleQuantity ='%d' Where id = \"%s\"",
			value,id);
	LocalQueryExec(sql_local.sql,buf);
	sql_local.checkFunc(sql_local.sql);
	sync();
}
int sqlGetEleQuantity(char *id)
{
	char buf[128];
	sprintf(buf, "select EleQuantity From DeviceList Where ID=\"%s\"", id);
	LocalQueryOpen(sql_local.sql,buf);
	int ret = LocalQueryOfInt(sql_local.sql,"EleQuantity");
	sql_local.sql->Close(sql_local.sql);
	return ret;
}

void sqlInit(void)
{
	LocalQueryLoad(&sql_local);
	sql_local.checkFunc(sql_local.sql);
	if (!sql_local.sql)
		DPRINT("sql err\n");
}
