/*
 * vfmw_proc_stm.c
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

#include "stm_dev.h"
#include "vfmw_sys.h"
#include "vfmw_proc.h"
#include "vfmw_proc_stm.h"

static void stm_read_dev(PROC_FILE *file)
{
	int used = 0;
	stm_dev *dev = NULL;
	stm_dev_state state;
	const char *state_name[STM_DEV_STATE_MAX + 1] = { "null", "idle", "busy", "NA" };

	dev = stm_dev_get_dev();
	if (dev->state < STM_DEV_STATE_IDLE) {
		dprint(PRN_ALWS, "stm dev not init\n");
		return;
	}

	/* dev state dump */
	state = (dev->state < STM_DEV_STATE_MAX) ? dev->state : STM_DEV_STATE_MAX;
	OS_DUMP_PROC(file, 0, &used, 0, LOG_FMT_HEAD, "DEV");
	OS_DUMP_PROC(file, 0, &used, 0, LOG_FMT_D_S, "dev_id", 0, "state",
		state_name[state]);
	OS_DUMP_PROC(file, 0, &used, 0, LOG_FMT_X_X, "reg_phy", dev->reg_phy,
		"reg_vir", dev->reg_vir);
	OS_DUMP_PROC(file, 0, &used, 0, LOG_FMT_X, "reg_size", dev->reg_size);
	OS_DUMP_PROC(file, 0, &used, 0, LOG_FMT_X, "smmu_bypass", dev->smmu_bypass);
	OS_DUMP_PROC(file, 0, &used, 0, LOG_FMT_X, "perf_enable", dev->perf_enable);
	OS_DUMP_PROC(file, 0, &used, 0, LOG_FMT_X, "poll_irq_enable", vfmw_proc_get_entry()->poll_irq_enable);
	OS_DUMP_PROC(file, 0, &used, 0, "\n");
}

int stm_read_proc(PROC_FILE *file, void *data)
{
	stm_read_dev(file);

	return 0;
}

static int stm_write_mmu_bypass(cmd_str_ptr buffer, unsigned int count)
{
	unsigned int argc = 2;
	unsigned int smmu_bypass = 0;
	stm_dev *dev = NULL;

	vfmw_assert_ret_prnt(count == argc, -1, "cmd param count %u invalid\n", count);
	str_to_val((*buffer)[1], &smmu_bypass);
	if (smmu_bypass > 1) {
		dprint(PRN_ALWS, "smmu_bypass: %u, should be 0 or 1\n", smmu_bypass);
		return -1;
	}

	dev = stm_dev_get_dev();
	dev->smmu_bypass = smmu_bypass;
	dprint(PRN_ALWS, "set smmu_bypass %d\n", dev->smmu_bypass);

	return 0;
}

static int stm_write_poll_irq(cmd_str_ptr buffer, unsigned int count)
{
	unsigned int argc = 2;
	unsigned int poll_irq_enable = 0;
	vfmw_proc_entry *entry = NULL;

	vfmw_assert_ret_prnt(count == argc, -1, "cmd param count %u invalid\n", count);
	str_to_val((*buffer)[1], &poll_irq_enable);
	if (poll_irq_enable > 1) {
		dprint(PRN_ALWS, "poll_irq_enable: %u, should be 0 or 1\n", poll_irq_enable);
		return -1;
	}

	entry = vfmw_proc_get_entry();
	entry->poll_irq_enable = poll_irq_enable;
	dprint(PRN_ALWS, "set poll_irq_enable %d\n", entry->poll_irq_enable);

	return 0;
}

static int stm_write_perf_enable(cmd_str_ptr buffer, unsigned int count)
{
	unsigned int argc = 2;
	unsigned int perf_enable = 0;
	stm_dev *dev = NULL;

	vfmw_assert_ret_prnt(count == argc, -1, "cmd param count %u invalid\n", count);
	str_to_val((*buffer)[1], &perf_enable);
	if (perf_enable > 1) {
		dprint(PRN_ALWS, "perf_enable should be 0 or 1\n");
		return -1;
	}

	dev = stm_dev_get_dev();
	dev->perf_enable = perf_enable;
	dprint(PRN_ALWS, "set perf_enable %u\n", dev->perf_enable);

	return 0;
}

static const log_cmd g_stm_cmd[] = {
	{ LOG_CMD_STM_SMMU_BYPASS, stm_write_mmu_bypass },
	{ LOG_CMD_STM_PERF_ENABLE, stm_write_perf_enable },
	{ LOG_CMD_POLL_IRQ_ENABLE, stm_write_poll_irq },
};

static int stm_handle_cmd(cmd_str_ptr cmd_str, int count)
{
	unsigned int i;
	unsigned int cmd_count;
	cmd_handler handle = NULL;
	char *cmd_id = ((*cmd_str)[0]);

	cmd_count = sizeof(g_stm_cmd) / sizeof(log_cmd);
	for (i = 0; i < cmd_count; i++) {
		if (!strncmp(cmd_id, g_stm_cmd[i].cmd_name, strlen(cmd_id))) {
			handle = g_stm_cmd[i].handler;
			break;
		}
	}

	if (!handle) {
		dprint(PRN_ALWS, "invalid cmd\n");
		return -1;
	}

	return handle(cmd_str, count);
}

int stm_write_proc(struct file *file, const char __user *buffer, size_t count,
	loff_t *data)
{
	unsigned int cmd_cnt;
	vfmw_proc_entry *entry = NULL;
	char cmd_str[CMD_PARAM_MAX_COUNT][CMD_PARAM_MAX_LEN] = {};
	stm_dev *dev = stm_dev_get_dev();

	if (dev->state < STM_DEV_STATE_IDLE) {
		dprint(PRN_ALWS, "stm dev not init\n");
		return -1;
	}

	cmd_cnt = vfmw_proc_parse_cmd(buffer, count, (&cmd_str));

	entry = vfmw_proc_get_entry();
	OS_SEMA_DOWN(entry->proc_sema);
	(void)stm_handle_cmd(&cmd_str, cmd_cnt);
	OS_SEMA_UP(entry->proc_sema);

	return count;
}
