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
#include <pthread.h>
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
static pthread_mutex_t mutex;

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
Channel INTEGER,\
EleQuantity INTEGER,\
AirConditionSpeed INTEGER,\
AirConditionMode INTEGER,\
AirConditionTemp INTEGER,\
MideaSlaveAddr INTEGER,\
MideaRoomAddr INTEGER\
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
    saveLog("sql locoal err\n");
	if (recoverData(sql_local.file_name) == 0) {
		saveLog("creat new db\n");
		LocalQueryExec(sql_local.sql,string);
	} else {
		sql_local.sql->Destroy(sql_local.sql);
		sql_local.sql = CreateLocalQuery(sql_local.sql->file_name);
	}
	return FALSE;
}

int sqlGetDeviceStart(void)
{
	pthread_mutex_lock(&mutex);
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
	pthread_mutex_unlock(&mutex);
}

void sqlInsertDevice(char *id,
		int dev_type,
		uint16_t addr,
		uint16_t channel)
{
	char buf[128];
	pthread_mutex_lock(&mutex);
	sprintf(buf, "INSERT INTO DeviceList(ID,DevType,Addr,Channel) VALUES('%s','%d','%d','%d')",
			id, dev_type,addr,channel);
	saveLog("%s\n",buf);
	LocalQueryExec(sql_local.sql,buf);
	sql_local.checkFunc(sql_local.sql);
	sync();
	pthread_mutex_unlock(&mutex);
}

void sqlDeleteDevice(char *id)
{
	char buf[128];
	pthread_mutex_lock(&mutex);
	sprintf(buf, "Delete From DeviceList Where ID=\"%s\"", id);
	DPRINT("%s\n",buf);
	saveLog("%s\n",buf);
	LocalQueryExec(sql_local.sql,buf);
	sql_local.checkFunc(sql_local.sql);
	sync();
	pthread_mutex_unlock(&mutex);
}

void sqlClearDevice(void)
{
	char buf[128];
	pthread_mutex_lock(&mutex);
	sprintf(buf, "Delete From DeviceList");
	DPRINT("%s\n",buf);
	saveLog("%s\n",buf);
	LocalQueryExec(sql_local.sql,buf);
	sql_local.checkFunc(sql_local.sql);
	sync();
	pthread_mutex_unlock(&mutex);
}
int sqlGetDeviceId(uint16_t addr,char *id)
{
	char buf[128];
	pthread_mutex_lock(&mutex);
	sprintf(buf, "select ID From DeviceList Where Addr=\"%d\"", addr);
	LocalQueryOpen(sql_local.sql,buf);
	int ret = sql_local.sql->RecordCount(sql_local.sql);
	if (ret)
		LocalQueryOfChar(sql_local.sql,"ID",id,32);
	// DPRINT("ret:%d,id:%s\n", ret,id);
	sql_local.sql->Close(sql_local.sql);
	pthread_mutex_unlock(&mutex);
	return ret;
}

void sqlSetEleQuantity(int value,char *id)
{
	char buf[128];
	pthread_mutex_lock(&mutex);
	sprintf(buf, "UPDATE DeviceList SET EleQuantity ='%d' Where id = \"%s\"",
			value,id);
	LocalQueryExec(sql_local.sql,buf);
	sql_local.checkFunc(sql_local.sql);
	sync();
	pthread_mutex_unlock(&mutex);
}
int sqlGetEleQuantity(char *id)
{
	char buf[128];
	pthread_mutex_lock(&mutex);
	sprintf(buf, "select EleQuantity From DeviceList Where ID=\"%s\"", id);
	LocalQueryOpen(sql_local.sql,buf);
	int ret = LocalQueryOfInt(sql_local.sql,"EleQuantity");
	sql_local.sql->Close(sql_local.sql);
	pthread_mutex_unlock(&mutex);
	return ret;
}
void sqlSetMideaAddr(char *id,int slave_addr,int room_addr)
{
	char buf[128];
	pthread_mutex_lock(&mutex);
	sprintf(buf, "UPDATE DeviceList SET MideaSlaveAddr ='%d',\
		   MideaRoomAddr ='%d'	Where id = \"%s\"",
			slave_addr,room_addr,id);
	LocalQueryExec(sql_local.sql,buf);
	sql_local.checkFunc(sql_local.sql);
	sync();
	pthread_mutex_unlock(&mutex);
}

void sqlGetMideaAddr(char *id,int *slave_addr,int *room_addr)
{
	char buf[128];
	pthread_mutex_lock(&mutex);
	sprintf(buf, "select MideaSlaveAddr,MideaRoomAddr From DeviceList Where ID=\"%s\"", id);
	LocalQueryOpen(sql_local.sql,buf);
	if (slave_addr)
		*slave_addr = LocalQueryOfInt(sql_local.sql,"MideaSlaveAddr");
	if (room_addr)
		*room_addr = LocalQueryOfInt(sql_local.sql,"MideaRoomAddr");
	sql_local.sql->Close(sql_local.sql);
	pthread_mutex_unlock(&mutex);
}

void sqlSetAirConditionPara(char *id,int temp,int mode,int speed)
{
	char buf[128];
	pthread_mutex_lock(&mutex);
	sprintf(buf, "UPDATE DeviceList SET AirConditionSpeed ='%d',\
		   	AirConditionMode ='%d',AirConditionTemp ='%d' Where id = \"%s\"",
			speed,mode,temp,id);
	LocalQueryExec(sql_local.sql,buf);
	sql_local.checkFunc(sql_local.sql);
	sync();
	pthread_mutex_unlock(&mutex);
}
void sqlGetAirConditionPara(char *id,int *temp,int *mode,int *speed)
{
	char buf[128];
	pthread_mutex_lock(&mutex);
	sprintf(buf, "select AirConditionTemp,AirConditionMode,AirConditionSpeed From DeviceList Where ID=\"%s\"", id);
	LocalQueryOpen(sql_local.sql,buf);
	if (temp)
		*temp = LocalQueryOfInt(sql_local.sql,"AirConditionTemp");
	if (mode)
		*mode = LocalQueryOfInt(sql_local.sql,"AirConditionMode");
	if (speed)
		*speed = LocalQueryOfInt(sql_local.sql,"AirConditionSpeed");
	sql_local.sql->Close(sql_local.sql);
	pthread_mutex_unlock(&mutex);
}

void sqlInit(void)
{
	pthread_mutexattr_t mutexattr;
	pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&mutex, &mutexattr);

	LocalQueryLoad(&sql_local);
	sql_local.checkFunc(sql_local.sql);
	if (!sql_local.sql) {
		DPRINT("sql err\n");
		saveLog("sql err\n");
	}
}
