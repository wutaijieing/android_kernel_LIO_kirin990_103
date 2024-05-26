/*
 * vfmw_proc.h
 *
 * This is for vfmw proc.
 *
 * Copyright (c) 2019-2020 Huawei Technologies CO., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef VFMW_PROC
#define VFMW_PROC

#include "vfmw_osal.h"

#define FILE_NAME_MAX_LEN 100

#define CMD_PARAM_MAX_COUNT 6
#define CMD_PARAM_MAX_LEN 20
#define CMD_STR_MAX_LEN 128

#define LOG_FMT_HEAD "----------------------- %-3s -----------------------\n"
#define LOG_FMT_D "%-15s: %-10d\n"
#define LOG_FMT_X "%-15s: 0x%-9x\n"
#define LOG_FMT_P "%-15s: %-10p\n"
#define LOG_FMT_S "%-15s: %-10s\n"
#define LOG_FMT_L "%-15s: %-10lld\n"
#define LOG_FMT_I_D "%-15s: %d.%d\n"
#define LOG_FMT_I_D_D "%-15s: %d/%d\n"
#define LOG_FMT_D_D "%-15s: %-10d | %-15s: %-10d\n"
#define LOG_FMT_X_X "%-15s: 0x%-9x | %-15s: 0x%-9x\n"
#define LOG_FMT_P_P "%-15s: %-10p | %-15s: %-10p\n"
#define LOG_FMT_S_S "%-15s: %-10s | %-15s: %-10s\n"
#define LOG_FMT_U_U "%-15s: %-10u | %-15s: %-10u\n"
#define LOG_FMT_D_X "%-15s: %-10d | %-15s: %-10x\n"
#define LOG_FMT_D_P "%-15s: %-10d | %-15s: %-10p\n"
#define LOG_FMT_D_S "%-15s: %-10d | %-15s: %-10s\n"
#define LOG_FMT_D_U "%-15s: %-10d | %-15s: %-10u\n"
#define LOG_FMT_X_D "%-15s: 0x%-9x | %-15s: %-10d\n"
#define LOG_FMT_X_P "%-15s: 0x%-9x | %-15s: %-10p\n"
#define LOG_FMT_X_S "%-15s: 0x%-9x | %-15s: %-10s\n"
#define LOG_FMT_X_U "%-15s: 0x%-9x | %-15s: %-10u\n"
#define LOG_FMT_P_D "%-15s: %-10p | %-15s: %-10d\n"
#define LOG_FMT_P_X "%-15s: %-10p | %-15s: 0x%-9x\n"
#define LOG_FMT_P_S "%-15s: %-10p | %-15s: %-10s\n"
#define LOG_FMT_P_U "%-15s: %-10p | %-15s: %-10u\n"
#define LOG_FMT_S_D "%-15s: %-10s | %-15s: %-10d\n"
#define LOG_FMT_S_X "%-15s: %-10s | %-15s: 0x%-9x\n"
#define LOG_FMT_S_P "%-15s: %-10s | %-15s: %-10p\n"
#define LOG_FMT_S_U "%-15s: %-10s | %-15s: %-10u\n"
#define LOG_FMT_U_D "%-15s: %-10u | %-15s: %-10d\n"
#define LOG_FMT_U_X "%-15s: %-10u | %-15s: 0x%-9x\n"
#define LOG_FMT_U_P "%-15s: %-10u | %-15s: %-10p\n"
#define LOG_FMT_U_S "%-15s: %-10u | %-15s: %-10s\n"
#define LOG_FMT_D_F "%-15s: %-10d | %-15s: %d.%d\n"
#define LOG_FMT_LX_LX "%-15s: 0x%-9llx | %-15s: 0x%-9llx\n"
#define LOG_FMT_LX_X "%-15s: 0x%-9llx | %-15s: 0x%-9x\n"

/* stm log command */
#define LOG_CMD_STM_SMMU_BYPASS "stm_mmu_bypass"
#define LOG_CMD_STM_PERF_ENABLE "stm_perf_enable"

/* dec log command */
#define LOG_CMD_DEC_SMMU_BYPASS "dec_mmu_bypass"
#define LOG_CMD_DEC_PERF_ENABLE "dec_perf_enable"
#define LOG_CMD_DEC_RESET_CTRL  "dec_reset_disable"
#define LOG_CMD_DEC_FORCE_CLK "dec_force_clk"
#define LOG_CMD_DEC_REG_DUMP "dec_reg_dump"

#define LOG_CMD_POLL_IRQ_ENABLE "poll_irq_enable"

#define CMD_STR_LEN (CMD_PARAM_MAX_COUNT * CMD_PARAM_MAX_LEN * sizeof(char))

typedef char (*cmd_str_ptr)[CMD_PARAM_MAX_COUNT][CMD_PARAM_MAX_LEN];
typedef int (*cmd_handler)(cmd_str_ptr, unsigned int);

typedef enum {
	LOG_MODULE_STM = 0,
	LOG_MODULE_DEC,
	LOG_MODULE_COM,
} log_module;

typedef struct {
	char cmd_name[CMD_PARAM_MAX_LEN];
	cmd_handler handler;
} log_cmd;

typedef struct {
	int chan_id;
	int reg_id;
	int init;
	os_sema proc_sema;
	unsigned int poll_irq_enable;
} vfmw_proc_entry;

vfmw_proc_entry *vfmw_proc_get_entry(void);
void vfmw_proc_parse_str(const char *cmd, unsigned int count, int *index,
	char *str);
unsigned int vfmw_proc_parse_cmd(const char *buf, int count,
	cmd_str_ptr cmd_str);
int str_to_val(const char *str, unsigned int *data);
int vfmw_create_proc(void);
void vfmw_destroy_proc(void);

#endif

