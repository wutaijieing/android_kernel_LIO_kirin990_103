/*
 * tick_mark.c
 *
 * SR Duration
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include "pm.h"
#include "pub_def.h"
#include "helper/log/lowpm_log.h"
#include "helper/register/register_ops.h"
#include "helper/dtsi/dtsi_ops.h"
#include "helper/debugfs/debugfs.h"

#include <pm_def.h>
#include <m3_rdr_ddr_map.h>

#include <linux/init.h>
#include <linux/suspend.h>

static int sr_tick_pm_notify(struct notifier_block *nb,
			     unsigned long mode, void *_unused)
{
	no_used(_unused);

	switch (mode) {
	case PM_SUSPEND_PREPARE:
		pmu_write_sr_tick(KERNEL_SUSPEND_PREPARE);
		break;
	case PM_POST_SUSPEND:
		pmu_write_sr_tick(KERNEL_RESUME_OUT);
		break;
	default:
		break;
	}

	return 0;
}

static struct notifier_block sr_tick_pm_nb = {
	.notifier_call = sr_tick_pm_notify,
};

#define BYTES_PER_TICK_POS 4
#define _runtime_read(offset) runtime_read(M3_RDR_SYS_CONTEXT_LPMCU_STAT_OFFSET + (offset))
#define read_tickmark(offset) _runtime_read((offset) * BYTES_PER_TICK_POS)

#define TICK_PER_SECOND 32768
#define US_PER_SECOND 1000000
#define US_PER_TICK (US_PER_SECOND / TICK_PER_SECOND)

static int tickmark_show_inner(struct seq_file *s, void *data)
{
	int i;
	const int ticks_per_line = 10;

	no_used(data);

	if (is_runtime_base_inited() != 0) {
		lp_msg(s, "runtime base not init");
		return -EINVAL;
	}

	lp_msg(s, "lpm suspend cost %d us: begin tick:0x%x, end tick:0x%x.",
		(read_tickmark(TICK_DEEPSLEEP_WFI_ENTER) -
		read_tickmark(TICK_SYS_SUSPEND_ENTRY)) * US_PER_TICK,
		read_tickmark(TICK_SYS_SUSPEND_ENTRY),
		read_tickmark(TICK_DEEPSLEEP_WFI_ENTER));

	lp_msg(s, "lpm resume cost %d us: begin tick:0x%x, end tick:0x%x.",
		(read_tickmark(TICK_SYS_WAKEUP_END) -
		read_tickmark(TICK_SYS_WAKEUP)) * US_PER_TICK,
		read_tickmark(TICK_SYS_WAKEUP),
		read_tickmark(TICK_SYS_WAKEUP_END));

	lp_msg(s, "lpm sr tick mark: ");
	for (i = 0; i < TICK_MARK_END_FLAG; i++) {
		lp_msg_cont(s, "[%02d]0x%08x ", i, read_tickmark(i));
		if ((i + 1) % ticks_per_line == 0)
			lp_msg_cont(s, "\n");
	}
	lp_msg_cont(s, "\n");

	return 0;
}

static int tickmark_show(void)
{
	return tickmark_show_inner(NO_SEQFILE, NULL);
}

static struct sr_mntn g_sr_mntn_tickmark = {
	.owner = "tick_mark",
	.enable = true,
	.suspend = NULL,
	.resume = tickmark_show,
};

static const struct lowpm_debugdir g_lpwpm_debugfs_tickmark = {
	.dir = "lowpm_func",
	.files = {
		{"tick_mark", 0400, tickmark_show_inner, NULL},
		{},
	},
};

static __init int init_sr_mntn_tickmark(void)
{
	int ret;

	if (sr_unsupported())
		return 0;

	ret = init_runtime_base();
	if (ret != 0) {
		lp_err("init runtime base failed");
		return ret;
	}

	ret = lowpm_create_debugfs(&g_lpwpm_debugfs_tickmark);
	if (ret != 0) {
		lp_err("create debug sr file failed");
		return ret;
	}

	ret = register_pm_notifier(&sr_tick_pm_nb);
	if (ret != 0) {
		lp_err("register_pm_notifier failed");
		return -ENODEV;
	}

	ret = register_sr_mntn(&g_sr_mntn_tickmark, SR_MNTN_PRIO_L);
	if (ret != 0) {
		lp_err("register mntn module failed");
		return ret;
	}

	lp_crit("success");

	return 0;
}

late_initcall(init_sr_mntn_tickmark);
