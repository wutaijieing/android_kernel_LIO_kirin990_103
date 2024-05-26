/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "stub.h"
#include <linux/string.h>
#include <linux/module.h>

/** The largest number of backup */
#define STUB_MAX_BACKUP_NUM 128

/** Jump instruction */
#define INSTRUCTION  0xe9

/** The length of the jump instruction */
#define INSTRUCTION_LEN   5

/** Return success */
#define RETURN_OK     0

/** Return to fail */
#define RETURN_ERROR  1

/** Backup structure */
typedef struct stub_backup_s
{
	common_func fn_old_func;                         /** The original address */
	unsigned char ibackup_code[INSTRUCTION_LEN];     /** Replace the former data */
} stub_backup_s;

/** Pile backup arrays */
static stub_backup_s ast_stub_backups[STUB_MAX_BACKUP_NUM];

/**
 * @brief Piling backup function, save function address and data before pile replacement
 *
 * @param fn_old_func
 * @param icode
 * @return int
 */
int save_backup_func_stub(common_func fn_old_func, unsigned char icode[], uint32_t len)
{
	int i;
	int ifree = -1;

	for (i = 0; i < STUB_MAX_BACKUP_NUM; i++) {
		if (ast_stub_backups[i].fn_old_func == NULL) {
			ifree = i;
			break;
		} else if (ast_stub_backups[i].fn_old_func == fn_old_func) {
			return RETURN_OK;
		}
	}

	if (ifree != -1) {
		ast_stub_backups[ifree].fn_old_func = fn_old_func;
		memcpy(ast_stub_backups[ifree].ibackup_code, icode, len);
		return RETURN_OK;
	}

	return RETURN_ERROR;
}

/**
 * @brief Removal of specified function in the backups of the array
 *
 * @param fn_old_func
 * @param icode
 * @return int
 */
int clear_backup_func_stub(common_func fn_old_func, unsigned char icode[], uint32_t len)
{
	int i;

	for (i = 0; i < STUB_MAX_BACKUP_NUM; i++) {
		if (ast_stub_backups[i].fn_old_func == fn_old_func) {
			if (NULL != icode)
				memcpy(icode, ast_stub_backups[i].ibackup_code, len);

			ast_stub_backups[i].fn_old_func = NULL;
			return RETURN_OK;
		}
	}

	return RETURN_ERROR;
}

/**
 * @brief Modify the top 5 bytes at the specified function addresses a JMP instruction
 *
 * @param fn_old_func
 * @param icode
 */
void write_to_code_section(common_func fn_old_func, unsigned char icode[], uint32_t len)
{
	memcpy(fn_old_func, icode, len);
}

/**
 * @brief Set the Func Stub None object
 *
 * @param fn_old_func
 * @return int
 */
int set_func_stub_none(common_func fn_old_func)
{
	unsigned char icode[INSTRUCTION_LEN] = {0};

	if (clear_backup_func_stub(fn_old_func, icode, INSTRUCTION_LEN) == RETURN_OK) {
		write_to_code_section(fn_old_func, icode, INSTRUCTION_LEN);
		return RETURN_OK;
	}

	return RETURN_ERROR;
}
EXPORT_SYMBOL(set_func_stub_none);

/**
 * @brief Remove all function
 *
 */
void clear_all_func_stubs(void)
{
	int i;

	for (i = 0; i < STUB_MAX_BACKUP_NUM; i++) {
		if (ast_stub_backups[i].fn_old_func != NULL) {
			write_to_code_section(ast_stub_backups[i].fn_old_func, ast_stub_backups[i].ibackup_code, INSTRUCTION_LEN);
			ast_stub_backups[i].fn_old_func = NULL;
		}
	}
}
EXPORT_SYMBOL(clear_all_func_stubs);

/**
 * @brief Set the Func Stub object
 *
 * @param fn_old_func
 * @param fn_new_func
 * @return int
 */
int set_func_stub(common_func fn_old_func, common_func fn_new_func)
{
	unsigned char icode[INSTRUCTION_LEN] = {0};
	int ioffset;

	if (fn_old_func == NULL || fn_new_func == NULL || fn_old_func == fn_new_func) {
		return RETURN_ERROR;
	}

	memcpy(icode, fn_old_func, INSTRUCTION_LEN);
	if (save_backup_func_stub(fn_old_func, icode, INSTRUCTION_LEN) != RETURN_OK)
		return RETURN_ERROR;

	/* Calculating migration, a jump instruction length is 5 bytes, need to lose five */
	ioffset = (int)((uintptr_t)fn_new_func - (uintptr_t)fn_old_func - INSTRUCTION_LEN);

	/* The format of the jump instruction JMP offset < = = > 0 xe9 + 4 bytes relative address */
	icode[0] = INSTRUCTION;
	*((int *)(icode + 1)) = ioffset;
	write_to_code_section(fn_old_func, icode, INSTRUCTION_LEN);

	return RETURN_OK;
}
EXPORT_SYMBOL(set_func_stub);

MODULE_LICENSE("GPL");
