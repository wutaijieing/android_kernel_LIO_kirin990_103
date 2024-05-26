/**********************************************************************************
* Copyright (C), 2010-2020, Hisilicon Tech. Co., Ltd.
*
* 文件名称 : Stub.c
*
* 版    本 :
*
* 功    能 : 打桩函数实现
*
* 作    者 :
*
* 创建日期 : 2011-03-07
*
* 修改记录 :
*
*    1: <时  间> : 2011-03-25
*
*       <作  者> :
*
*       <版  本> :
*
*       <内  容> :  修改写内存函数，优化函数结构
*
*    2:
**********************************************************************************/

#include "Stub.h"
#include <linux/string.h>
#include <linux/module.h>

/** 最大备份数. */
#define STUB_MAX_BACKUP_NUM 128

/** 跳转指令. */
#define INSTRUCTION  0xe9

/** 跳转指令长度. */
#define INSTRUCTION_LEN   5

/** 返回 成功. */
#define RETURN_OK     0

/** 返回失败. */
#define RETURN_ERROR  1

/** 桩备份结构体. */
typedef struct STUB_BACKUP_S
{
	CommonFunc fnOldFunc;                              /** 原函数地址. */
	unsigned char    iBackupCode[INSTRUCTION_LEN];     /** 桩替换前数据. */
} STUB_BACKUP_S;

/** 桩备份数组. */
static STUB_BACKUP_S astStubBackups[STUB_MAX_BACKUP_NUM];


/********************************************************
  * 函数名称: SaveBackupFuncStub
  *
  * 功能描述: 打桩备份函数，保存原函数
  *                         地址和桩替换前数据
  *
  * 调用函数:
  *
  * 被调函数:SetFuncStub
  *
  * 输入参数: Param1: CommonFunc fnOldFunc
  *                         Param2: unsigned char iCode[]
  *
  * 输出参数:
  *
  * 返 回 值:
  *                       RETURN_OK                         执行成功
  *                       RETURN_ERROR                    执行失败
  * 其    他
  *
  * 作    者:
  *
  * 日    期: 2010-03-07
  *
  * 修改日期
  *
  * 修改作者
  **********************************************************/
int SaveBackupFuncStub(CommonFunc fnOldFunc, unsigned char iCode[])
{
	int i;
	int iFree = -1;

	for (i = 0; i < STUB_MAX_BACKUP_NUM; i++) {
		if (astStubBackups[i].fnOldFunc == NULL) {
			iFree = i;
			break;
		} else if (astStubBackups[i].fnOldFunc == fnOldFunc) {
			// 已经备份过了，返回成功
			return RETURN_OK;
		}
	}

	if (iFree != -1) {
		astStubBackups[iFree].fnOldFunc = fnOldFunc;
		memcpy(astStubBackups[iFree].iBackupCode,iCode,INSTRUCTION_LEN);
		return RETURN_OK;
	}

	// 已经达到最大值，失败
	return RETURN_ERROR;
}


/********************************************************
  * 函数名称: ClearBackupFuncStub
  *
  * 功能描述: 清除指定函数在
  *                         备份数组中的备份
  *
  * 调用函数:
  *
  * 被调函数:SetFuncStubNone
  *
  * 输入参数: Param1: CommonFunc fnOldFunc
  *                         Param2: unsigned char iCode[]
  *
  * 输出参数:
  *
  * 返 回 值:
  *                       RETURN_OK                         执行成功
  *                       RETURN_ERROR                    执行失败
  * 其    他
  *
  * 作    者:
  *
  * 日    期: 2010-03-07
  *
  * 修改日期
  *
  * 修改作者
  **********************************************************/
int ClearBackupFuncStub(CommonFunc fnOldFunc, unsigned char iCode[])
{
	int i;

	for (i = 0; i < STUB_MAX_BACKUP_NUM; i++) {
		if (astStubBackups[i].fnOldFunc == fnOldFunc) {
			if (NULL != iCode)
				memcpy(iCode,astStubBackups[i].iBackupCode,INSTRUCTION_LEN);

			astStubBackups[i].fnOldFunc = NULL;
			return RETURN_OK;
		}
	}

	// 没有找到，返回失败
	return RETURN_ERROR;
}


/********************************************************
  * 函数名称: WriteToCodeSection
  *
  * 功能描述: 修改指定函数地址的
  *                         前5 个字节为一条JMP 指令
  *
  * 调用函数:
  *
  * 被调函数:SetFuncStub    SetFuncStubNone
  *
  * 输入参数: Param1: CommonFunc fnOldFunc
  *                         Param2: unsigned char iCode[]
  *
  * 输出参数:
  *
  * 返 回 值:
  *                       无
  * 其    他
  *
  * 作    者:
  *
  * 日    期: 2010-03-07
  *
  * 修改日期
  *
  * 修改作者
  **********************************************************/
void WriteToCodeSection(CommonFunc fnOldFunc, unsigned char iCode[])
{
	memcpy(fnOldFunc, iCode, INSTRUCTION_LEN);
}


/********************************************************
  * 函数名称: SetFuncStubNone
  *
  * 功能描述: 清除指定函数的桩
  *
  * 调用函数: ClearBackupFuncStub
  *                         WriteToCodeSection
  *
  * 被调函数:SetFuncStub   ClearAllFuncStubs
  *                        ClearAllFuncStubs
  *
  * 输入参数: Param1: CommonFunc fnOldFunc
  *
  * 输出参数:
  *
  * 返 回 值:
  *                       RETURN_OK                         执行成功
  *                       RETURN_ERROR                    执行失败
  * 其    他
  *
  * 作    者:
  *
  * 日    期: 2010-03-07
  *
  * 修改日期
  *
  * 修改作者
  **********************************************************/
int SetFuncStubNone(CommonFunc fnOldFunc)
{
	unsigned char iCode[INSTRUCTION_LEN] = {0};

	if (ClearBackupFuncStub(fnOldFunc, iCode) == RETURN_OK) {
		WriteToCodeSection(fnOldFunc, iCode);
		return RETURN_OK;
	}

	return RETURN_ERROR;
}


/********************************************************
  * 函数名称: ClearAllFuncStubs
  *
  * 功能描述: 清除所有桩函数
  *
  * 调用函数: WriteToCodeSection
  *
  * 被调函数:
  *
  * 输入参数:
  *
  * 输出参数:
  *
  * 返 回 值:
  *                       无
  * 其    他
  *
  * 作    者:
  *
  * 日    期: 2010-03-07
  *
  * 修改日期
  *
  * 修改作者
  **********************************************************/
void ClearAllFuncStubs(void)
{
	int i;

	for (i = 0; i < STUB_MAX_BACKUP_NUM; i++) {
		if (astStubBackups[i].fnOldFunc != NULL) {
			WriteToCodeSection(astStubBackups[i].fnOldFunc,astStubBackups[i].iBackupCode);
			astStubBackups[i].fnOldFunc = NULL;
		}
	}
}


/********************************************************
  * 函数名称: SetFuncStub
  *
  * 功能描述: 对指定函数打桩
  *
  * 调用函数: SetFuncStubNone
  *                         SaveBackupFuncStub
  *                         WriteToCodeSection
  *
  * 被调函数:
  *
  * 输入参数: Param1: CommonFunc fnOldFunc
  *                         Param2: CommonFunc fnNewFunc
  *
  * 输出参数:
  *
  * 返 回 值:
  *                       RETURN_OK                         执行成功
  *                       RETURN_ERROR                    执行失败
  * 其    他
  *
  * 作    者:
  *
  * 日    期: 2010-03-07
  *
  * 修改日期
  *
  * 修改作者
  **********************************************************/
int SetFuncStub(CommonFunc fnOldFunc, CommonFunc fnNewFunc)
{
	unsigned char iCode[INSTRUCTION_LEN]={0};
	int  iOffset;

	if (fnOldFunc == NULL || fnNewFunc == NULL || fnOldFunc == fnNewFunc) {
		return RETURN_ERROR;
	}

	memcpy(iCode, fnOldFunc, INSTRUCTION_LEN);
	if (SaveBackupFuncStub(fnOldFunc, iCode) != RETURN_OK)
		return RETURN_ERROR;

	/* 计算偏移，一条跳转指令长度为5字节，需要减5 */
	iOffset = (int)((long)fnNewFunc - (long)fnOldFunc - INSTRUCTION_LEN);

	/* 跳转指令的格式 jmp offset <==> 0xE9 + 4字节相对地址 */
	iCode[0] = INSTRUCTION;
	*((int *)(iCode + 1)) = iOffset;
	WriteToCodeSection(fnOldFunc,iCode);

	return RETURN_OK;
}

/**对指定函数打桩. */
EXPORT_SYMBOL(SetFuncStub);
/**清除所有桩函数. */
EXPORT_SYMBOL(ClearAllFuncStubs);
/**清除指定函数的桩. */
EXPORT_SYMBOL(SetFuncStubNone);
MODULE_LICENSE("GPL");

