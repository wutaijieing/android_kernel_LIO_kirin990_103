/*
 * suspend_host.c
 *
 * suspend on host
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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

#include <linux/bitops.h>
#include <linux/io.h>
#include <linux/printk.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/syscore_ops.h>

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "lowpm: " fmt

static void __iomem *g_ap_suspend_flag_addr = NULL;
#define REG_ADDR_AP_SUSPEND_FLAG 0xfa89b42c
#define REG_SIZE 4
#define AP_SUSPEND_BIT 16

static void set_ap_suspend_flag(void)
{
	unsigned int val;
	val = readl(g_ap_suspend_flag_addr) | (unsigned int)BIT(AP_SUSPEND_BIT);
	writel(val, g_ap_suspend_flag_addr);
}

static void clear_ap_suspend_flag(void)
{
	unsigned int val;
	val = readl(g_ap_suspend_flag_addr) & (~(unsigned int)BIT(AP_SUSPEND_BIT));
	writel(val, g_ap_suspend_flag_addr);
}

static int pm_syscore_suspend(void)
{
	pr_info("%s ++", __func__);
	set_ap_suspend_flag();
	pr_info("%s --", __func__);
	return 0;
}

static void pm_syscore_resume(void)
{
	pr_info("%s ++", __func__);
	clear_ap_suspend_flag();
	pr_info("%s --", __func__);
	return;
}

static struct syscore_ops g_suspend_syscore_ops = {
	.suspend = pm_syscore_suspend,
	.resume = pm_syscore_resume,
};

static __init int pm_init(void)
{
	g_ap_suspend_flag_addr = ioremap(REG_ADDR_AP_SUSPEND_FLAG, REG_SIZE);
	if (g_ap_suspend_flag_addr == NULL) {
		pr_err("%s map AP suspend flag reg failed", __func__);
		return -ENODATA;
	}

	register_syscore_ops(&g_suspend_syscore_ops);
	pr_info("%s success", __func__);
	return 0;
}

arch_initcall(pm_init);
