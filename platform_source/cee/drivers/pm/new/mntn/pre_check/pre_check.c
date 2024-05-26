/*
 * pre_check.c
 *
 * check before enter suspend
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
#include "helper/log/lowpm_log.h"
#include "helper/dtsi/dtsi_ops.h"

#include <linux/init.h>
#include <linux/clk.h>
#include <platform_include/basicplatform/linux/mfd/pmic_platform.h>

static int sr_pre_check(void)
{
	get_ip_regulator_state();
	pmclk_monitor_enable();

	return 0;
}

static struct sr_mntn g_sr_mntn_pre_check = {
	.owner = "pre_check",
	.enable = true,
	.suspend = sr_pre_check,
	.resume = NULL,
};

static __init int init_sr_mntn_precheck(void)
{
	int ret;

	if (sr_unsupported())
		return 0;

	ret = register_sr_mntn(&g_sr_mntn_pre_check, SR_MNTN_PRIO_M);
	if (ret != 0) {
		lp_err("fail %d", ret);
		return ret;
	}

	lp_crit("success");
	return 0;
}

late_initcall(init_sr_mntn_precheck);
