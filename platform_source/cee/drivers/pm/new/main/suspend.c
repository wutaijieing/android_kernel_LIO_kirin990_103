/*
 * suspend.c
 *
 * suspend
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2016-2020. All rights reserved.
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
#include "sr_mntn.h"
#include "pub_def.h"
#include "helper/log/lowpm_log.h"
#include "helper/dtsi/dtsi_ops.h"
#include "helper/register/register_ops.h"

#include <soc_sctrl_interface.h>

#include <linux/init.h>
#include <linux/errno.h>
#include <linux/suspend.h>
#include <linux/psci.h>
#include <linux/types.h>
#include <asm/suspend.h>
#include <asm/cputype.h>

#define REG_AP_SUSPEND_OFFSET SOC_SCTRL_SCBAKDATA8_ADDR(0)
#define AP_SUSPEND_BIT 16

static void set_ap_suspend_flag(void)
{
#ifndef CONFIG_PRODUCT_CDC_ACE
	enable_bit(SCTRL, REG_AP_SUSPEND_OFFSET, AP_SUSPEND_BIT);
#endif
	pmu_write_sr_tick(KERNEL_SUSPEND_SETFLAG);
}

static void clear_ap_suspend_flag(void)
{
#ifndef CONFIG_PRODUCT_CDC_ACE
	disable_bit(SCTRL, REG_AP_SUSPEND_OFFSET, AP_SUSPEND_BIT);
#endif
}

static int sr_psci_suspend(unsigned long index)
{
	const int sr_power_state_suspend = 0x01010000;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	return psci_ops.cpu_suspend(sr_power_state_suspend,
				__pa_function(cpu_resume));
#else
	return psci_ops.cpu_suspend(sr_power_state_suspend,
				    virt_to_phys(cpu_resume));
#endif
}

static void pm_cpu_suspend(void)
{
	int ret;

	set_ap_suspend_flag();

	ret = cpu_suspend(0, sr_psci_suspend);
	if (ret != 0)
		lp_crit("cpu suspend failed %d",ret);

	clear_ap_suspend_flag();
}

static int pm_enter(suspend_state_t state)
{
	pmu_write_sr_tick(KERNEL_SUSPEND_IN);
	lp_crit("++");

	lowpm_sr_mntn_suspend();

	pm_cpu_suspend();

	lowpm_sr_mntn_resume();

	lp_crit("--");
	pmu_write_sr_tick(KERNEL_RESUME);

	return 0;
}

static const struct platform_suspend_ops g_suspend_ops = {
	.enter = pm_enter,
	.valid = suspend_valid_only_mem,
};

static __init int pm_init(void)
{
	if (sr_unsupported()) {
		/* if not suspend_set_ops ok, don't support suspend to s2idle */
		mem_sleep_current = PM_SUSPEND_ON;
		lp_crit("not support suspend function");
		return 0;
	}

	if (map_reg_base(SCTRL) != 0) {
		lp_err("map sctrl reg failed");
		return -ENODATA;
	}

	suspend_set_ops(&g_suspend_ops);

	lp_crit("success");

	return 0;
}

arch_initcall(pm_init);
