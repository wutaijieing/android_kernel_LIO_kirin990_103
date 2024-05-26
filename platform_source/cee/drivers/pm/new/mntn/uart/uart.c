/*
 * uart.c
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
#include "helper/log/lowpm_log.h"
#include "helper/dtsi/dtsi_ops.h"

#include <linux/init.h>
#include <linux/console.h>

int init_ap_uart(void)
{
	return 0;
}

static int uart_early_init(void)
{
	int ret;

	if (console_suspend_enabled)
		return 0;

	ret = init_ap_uart();
	if (ret != 0) {
		lp_err("uart init fail after resume");
		return -ENODEV;
	}

	return 0;
}

static struct sr_mntn g_sr_mntn_uart = {
	.owner = "uart",
	.enable = true,
	.suspend = NULL,
	.resume = uart_early_init,
};

static __init int init_sr_mntn_uart(void)
{
	int ret;

	if (sr_unsupported())
		return 0;

	ret = register_sr_mntn(&g_sr_mntn_uart, SR_MNTN_PRIO_C);
	if (ret != 0) {
		lp_err("register mntn module failed");
		return ret;
	}

	lp_crit("success");

	return 0;
}

late_initcall(init_sr_mntn_uart);
