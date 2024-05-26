/*
 * vfmw_proc_dec.c
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

#include "dec_dev.h"
#include "dec_hal.h"
#include "vfmw_sys.h"
#include "vfmw_proc.h"
#include "vfmw_proc_dec.h"

int dec_read_proc(PROC_FILE *file, void *data)
{
	int used = 0;
	int dev_id = 0;
	dec_dev_info *dev = NULL;
	const char *state_name[DEC_DEV_STATE_MAX + 1] = {
		"null", "run", "suspend", "NA"
	};

	(void)dec_dev_get_dev(dev_id, &dev);

	OS_DUMP_PROC(file, 0, &used, 0, LOG_FMT_HEAD, "DEV");
	OS_DUMP_PROC(file, 0, &used, 0, LOG_FMT_D_S, "dev_id", 0, "state",
		state_name[dev->state]);
	OS_DUMP_PROC(file, 0, &used, 0, LOG_FMT_X, "reg_phy",
		dev->reg_phy_addr);
	OS_DUMP_PROC(file, 0, &used, 0, LOG_FMT_X, "reg_size", dev->reg_size);
	OS_DUMP_PROC(file, 0, &used, 0, LOG_FMT_X, "smmu_bypass",
		dev->smmu_bypass);
	OS_DUMP_PROC(file, 0, &used, 0, LOG_FMT_X, "perf_enable",
		dev->perf_enable);
	OS_DUMP_PROC(file, 0, &used, 0, LOG_FMT_X, "poll_irq_enable", vfmw_proc_get_entry()->poll_irq_enable);
	OS_DUMP_PROC(file, 0, &used, 0, LOG_FMT_X, "reg_dump_enable", dev->reg_dump_enable);
	OS_DUMP_PROC(file, 0, &used, 0, "\n");

	return 0;
}

static int dec_write_mmu_bypass(cmd_str_ptr buffer, unsigned int count)
{
	int dev_id = 0;
	unsigned int argc = 2;
	unsigned int smmu = 0;
	dec_dev_info *dev = NULL;

	vfmw_assert_ret_prnt(count == argc, -1, "cmd param count %d invalid\n", count);
	str_to_val((*buffer)[1], &smmu);
	if (smmu > 1) {
		dprint(PRN_ALWS, "smmu should be 0 or 1\n");
		return -1;
	}

	(void)dec_dev_get_dev(dev_id, &dev);

	dev->smmu_bypass = smmu;
	dprint(PRN_ALWS, "set smmu bypass %u\n", dev->smmu_bypass);

	return 0;
}

static int dec_write_perf_enable(cmd_str_ptr buffer, unsigned int count)
{
	int dev_id = 0;
	unsigned int argc = 2;
	unsigned int perf = 0;
	dec_dev_info *dev = NULL;

	vfmw_assert_ret_prnt(count == argc, -1, "cmd param count %u invalid\n", count);
	str_to_val((*buffer)[1], &perf);
	if (perf > 1) {
		dprint(PRN_ALWS, "perfshould be 0 or 1\n");
		return -1;
	}

	(void)dec_dev_get_dev(dev_id, &dev);

	dev->perf_enable = perf;
	dprint(PRN_ALWS, "set perf_enable %u\n", dev->perf_enable);

	return 0;
}

static int dec_write_reset_ctrl(cmd_str_ptr buffer, unsigned int count)
{
	unsigned int argc = 2;
	unsigned int disable = 0;
	int dev_id = 0;
	dec_dev_info *dev = NULL;

	vfmw_assert_ret_prnt(count == argc, -1, "cmd param count %d invalid\n", count);
	str_to_val((*buffer)[1], &disable);
	if (disable > 1) {
		dprint(PRN_ALWS, "disable should be 0 or 1\n");
		return -1;
	}

	(void)dec_dev_get_dev(dev_id, &dev);
	dev->reset_disable = disable;
	dprint(PRN_ALWS, "set vdh reset_disable flag: %d\n", dev->reset_disable);
	return 0;
}


static int dec_force_clk_enable(cmd_str_ptr buffer, unsigned int count)
{
	unsigned int argc = 2;
	unsigned int enable = 0;
	int dev_id = 0;
	dec_dev_info *dev = NULL;

	vfmw_assert_ret_prnt(count == argc, -1, "cmd param count %d invalid\n", count);
	str_to_val((*buffer)[1], &enable);
	if (enable > 1) {
		dprint(PRN_ALWS, "enable should be 0 or 1\n");
		return -1;
	}

	(void)dec_dev_get_dev(dev_id, &dev);
	dev->force_clk = enable;
	dprint(PRN_ALWS, "set vdh clk force enable : %d\n", dev->force_clk);
	return 0;
}

static int dec_write_poll_irq(cmd_str_ptr buffer, unsigned int count)
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

static int dec_reg_dump_enable(cmd_str_ptr buffer, unsigned int count)
{
	unsigned int argc = 2;
	unsigned int enable = 0;
	int dev_id = 0;
	dec_dev_info *dev = NULL;

	vfmw_assert_ret_prnt(count == argc, -1, "cmd param count %d invalid\n", count);
	str_to_val((*buffer)[1], &enable);
	if (enable > 1) {
		dprint(PRN_ALWS, "enable should be 0 or 1\n");
		return -1;
	}

	(void)dec_dev_get_dev(dev_id, &dev);
	dev->reg_dump_enable = enable;
	dprint(PRN_ALWS, "set vdh reg dump enable : %d\n", dev->reg_dump_enable);
	return 0;
}

static const log_cmd g_dec_cmd[] = {
	{ LOG_CMD_DEC_SMMU_BYPASS, dec_write_mmu_bypass },
	{ LOG_CMD_DEC_PERF_ENABLE, dec_write_perf_enable },
	{ LOG_CMD_DEC_RESET_CTRL, dec_write_reset_ctrl },
	{ LOG_CMD_DEC_FORCE_CLK, dec_force_clk_enable },
	{ LOG_CMD_DEC_REG_DUMP, dec_reg_dump_enable },
	{ LOG_CMD_POLL_IRQ_ENABLE, dec_write_poll_irq },
};

static int dec_handle_cmd(cmd_str_ptr cmd_str, int count)
{
	unsigned int i;
	unsigned int cmd_count;
	cmd_handler handle = NULL;
	char *cmd_id = ((*cmd_str)[0]);

	cmd_count = sizeof(g_dec_cmd) / sizeof(log_cmd);
	for (i = 0; i < cmd_count; i++) {
		if (!strncmp(cmd_id, g_dec_cmd[i].cmd_name, strlen(cmd_id))) {
			handle = g_dec_cmd[i].handler;
			break;
		}
	}

	if (!handle) {
		dprint(PRN_ALWS, "invalid cmd\n");
		return -1;
	}

	return handle(cmd_str, count);
}

int dec_write_proc(struct file *file, const char __user *buffer, size_t count,
	loff_t *data)
{
	unsigned int cmd_cnt;
	vfmw_proc_entry *entry = NULL;
	char cmd_str[CMD_PARAM_MAX_COUNT][CMD_PARAM_MAX_LEN] = {};

	cmd_cnt = vfmw_proc_parse_cmd(buffer, count, (&cmd_str));

	entry = vfmw_proc_get_entry();
	OS_SEMA_DOWN(entry->proc_sema);
	(void)dec_handle_cmd(&cmd_str, cmd_cnt);
	OS_SEMA_UP(entry->proc_sema);

	return count;
}