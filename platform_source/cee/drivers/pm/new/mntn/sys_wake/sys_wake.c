/*
 * sys_wake.c
 *
 * debug information of suspend
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2015-2020. All rights reserved.
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
/* platform/common/lowpm/pm_common.h */
#include "pm_common.h"
#include "helper/debugfs/debugfs.h"
#include "helper/register/register_ops.h"
#include "helper/dtsi/dtsi_ops.h"
#include "helper/log/lowpm_log.h"

#include <pm_def.h>
#include <m3_rdr_ddr_map.h>
#include <lpmcu_runtime.h>
#include <soc_sctrl_interface.h>
#ifdef CONFIG_POWER_DUBAI
#include <huawei_platform/power/dubai/dubai_wakeup_stats.h>
#endif
#include <securec.h>

#include <linux/init.h>
#include <linux/io.h>

#define _runtime_read(offset) runtime_read(M3_RDR_SYS_CONTEXT_RUNTIME_VAR_OFFSET + (offset))

static void show_deepsleep_wake_irq(struct seq_file *s)
{
	lp_msg_cont(s, "SR:SYS WAKE_IRQ:");
	lp_msg_cont(s, "[0x%x]", _runtime_read(WAKE_IRQ_OFFSET));
	lp_msg_cont(s, "[0x%x]", _runtime_read(WAKE_IRQ1_OFFSET));
#ifdef WAKE_IRQ2_OFFSET
	lp_msg_cont(s, "[0x%x]", _runtime_read(WAKE_IRQ2_OFFSET));
#endif
#ifdef WAKE_IRQ3_OFFSET
	lp_msg_cont(s, "[0x%x]", _runtime_read(WAKE_IRQ3_OFFSET));
#endif
#ifdef WAKE_IRQ4_OFFSET
	lp_msg_cont(s, "[0x%x]", _runtime_read(WAKE_IRQ4_OFFSET));
#endif
#ifdef WAKE_IRQ5_OFFSET
	lp_msg_cont(s, "[0x%x]", _runtime_read(WAKE_IRQ5_OFFSET));
#endif
	lp_msg_cont(s, "\n");
}

static void show_subsys_sr_times(struct seq_file *s)
{
	lp_msg(s, "SR:system sleeped %u times.", _runtime_read(SYS_DSLEEP_CNT_OFFSET));

	lp_msg(s,
	       "SR:wake times, system:%u, woken up by ap:%u, modem:%u, hifi:%u, iomcu:%u, lpm3:%u.",
	       _runtime_read(SYS_DWAKE_CNT_OFFSET),
	       _runtime_read(AP_WAKE_CNT_OFFSET),
	       _runtime_read(MODEM_WAKE_CNT_OFFSET),
	       _runtime_read(HIFI_WAKE_CNT_OFFSET),
	       _runtime_read(IOMCU_WAKE_CNT_OFFSET),
	       _runtime_read(LPM3_WAKE_CNT_OFFSET));

	lp_msg_cont(s,
		    "SR:sr times of sub system, ap:s-%u, r-%u; modem:s-%u, r-%u; hifi:s-%u, r-%u; iomcu:s-%u, r-%u.",
		    _runtime_read(AP_SUSPEND_CNT_OFFSET),
		    _runtime_read(AP_RESUME_CNT_OFFSET),
		    _runtime_read(MODEM_SUSPEND_CNT_OFFSET),
		    _runtime_read(MODEM_RESUME_CNT_OFFSET),
		    _runtime_read(HIFI_SUSPEND_CNT_OFFSET),
		    _runtime_read(HIFI_RESUME_CNT_OFFSET),
		    _runtime_read(IOMCU_SUSPEND_CNT_OFFSET),
		    _runtime_read(IOMCU_RESUME_CNT_OFFSET));
#if defined(GENERAL_SEE_SUSPEND_CNT_OFFSET) && defined(GENERAL_SEE_RESUME_CNT_OFFSET)
	lp_msg_cont(s, " general_see:s-%u, r-%u.",
		    _runtime_read(GENERAL_SEE_SUSPEND_CNT_OFFSET),
		    _runtime_read(GENERAL_SEE_RESUME_CNT_OFFSET));
#endif
	lp_msg_cont(s, "\n");

#if defined(CCPUNR_SUSPEND_CNT_OFFSET) && defined(CCPUNR_RESUME_CNT_OFFSET)
	lp_msg(s, "SR:sr times of sub system, modem_nr:s-%u, r-%u.",
	       _runtime_read(CCPUNR_SUSPEND_CNT_OFFSET),
	       _runtime_read(CCPUNR_RESUME_CNT_OFFSET));
	lp_msg(s, "SR:wakelock status, modem_nr:%d.",
	       (_runtime_read(WAKE_STATUS_OFFSET) & MODEM_NR_MASK) ? 1 : 0);
#if defined(MODEM_NR_WAKE_CNT_OFFSET)
	lp_msg(s, "SR:woken up by modem_nr:%u.", _runtime_read(MODEM_NR_WAKE_CNT_OFFSET));
#endif
#endif
#if defined(OTHER_WAKE_CNT_OFFSET)
	lp_msg(s, "SR:woken up by other:%u.", _runtime_read(OTHER_WAKE_CNT_OFFSET));
#endif
}

static void show_subsys_32k_idle_times(struct seq_file *s)
{
	lp_msg(s, "sleep 0x%x times.", _runtime_read(SYS_SLEEP_CNT_OFFSET));

#if defined(SYS_SLEEP_CAN_ENTER_CNT_OFFSET) && defined(SYS_SLEEP_TIME_OFFSET)
	lp_msg(s, "sleep try %d times; enter time %d",
		  _runtime_read(SYS_SLEEP_CAN_ENTER_CNT_OFFSET),
		  _runtime_read(SYS_SLEEP_TIME_OFFSET));
#endif

#if defined(SLEEP_FAIL_ITEM_OFFSET) && defined(SLEEP_FAIL_ITEM_VALUE_OFFSET)
	lp_msg(s, "sleep fail to enter for items 0x%x, item value is 0x%x",
		  _runtime_read(SLEEP_FAIL_ITEM_OFFSET),
		  _runtime_read(SLEEP_FAIL_ITEM_VALUE_OFFSET));
#endif

#if defined(FCM_OFF_CNT_OFFSET) && defined(FCM_ON_CNT_OFFSET)
	lp_msg(s, "CLUSTER/FCM power down 0x%x; up 0x%x.",
		  _runtime_read(FCM_OFF_CNT_OFFSET),
		  _runtime_read(FCM_ON_CNT_OFFSET));
#endif
}

#define pm_mask_bit(val, mask) (!!((val) & (mask)))
static void pm_show_wakelock_status(struct seq_file *s)
{
	int ret;
	unsigned int normal_status_vote;
	char buf[128] = {0};

	normal_status_vote = _runtime_read(WAKE_STATUS_OFFSET);
	ret = sprintf_s(buf, sizeof(buf),
			"ap=%d modem=%d hifi=%d iomcu=%d lpmcu=%d general_see=%d codec=%d asp=%d modem_nr=%d gpu=%d",
			pm_mask_bit(normal_status_vote, AP_MASK), pm_mask_bit(normal_status_vote, MODEM_MASK),
			pm_mask_bit(normal_status_vote, HIFI_MASK), pm_mask_bit(normal_status_vote, IOMCU_MASK),
			pm_mask_bit(normal_status_vote, LPMCU_MASK), pm_mask_bit(normal_status_vote, GENERAL_SEE_MASK),
			pm_mask_bit(normal_status_vote, CODEC_MASK), pm_mask_bit(normal_status_vote, ASP_MASK),
			pm_mask_bit(normal_status_vote, MODEM_NR_MASK), pm_mask_bit(normal_status_vote, GPU_MASK));
	if (ret <= 0)
		return;
	lp_msg(s, "SR:wakelock status[0x%x], %s.", normal_status_vote, buf);
#ifdef CONFIG_POWER_DUBAI
	dubai_log_wakeup_info("DUBAI_TAG_WAKE_STATUS", "%s", buf);
#endif
}

static int pm_status_show_inner(struct seq_file *s, void *data)
{
	no_used(data);

	if (is_runtime_base_inited() != 0) {
		lp_msg(s, "runtime base not init");
		return -EINVAL;
	}

	pm_show_wakelock_status(s);
	show_subsys_sr_times(s);
	show_subsys_32k_idle_times(s);
	show_deepsleep_wake_irq(s);

	return 0;
}

static int pm_status_show(void)
{
	return pm_status_show_inner(NO_SEQFILE, NULL);
}

static struct sr_mntn g_sr_mntn_sys_wake = {
	.owner = "sys_wake",
	.enable = true,
	.suspend = NULL,
	.resume = pm_status_show,
};

static const struct lowpm_debugdir g_lpwpm_debugfs_debug_sr = {
	.dir = "lowpm_func",
	.files = {
		{"debug_sr", 0400, pm_status_show_inner, NULL},
		{},
	},
};

static __init int init_sr_mntn_syswake(void)
{
	int ret;

	if (sr_unsupported())
		return 0;

	ret = init_runtime_base();
	if (ret != 0) {
		lp_err("init runtime base failed");
		return ret;
	}

	ret = lowpm_create_debugfs(&g_lpwpm_debugfs_debug_sr);
	if (ret != 0) {
		lp_err("create debug sr file failed");
		return ret;
	}

	ret = register_sr_mntn(&g_sr_mntn_sys_wake, SR_MNTN_PRIO_M);
	if (ret != 0) {
		lp_err("register mntn module failed");
		return ret;
	}

	lp_crit("success");

	return 0;
}

late_initcall(init_sr_mntn_syswake);
