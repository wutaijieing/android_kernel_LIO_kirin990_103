/*
 * namtso_flush_cmo.c
 *
 * namtso flush cmo driver
 *
 * Copyright (c) 2020-2020 Huawei Technologies Co., Ltd.
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
#define pr_fmt(fmt) "namtso_flush: " fmt
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <soc_fcm_namtso_interface.h>
#include <soc_acpu_baseaddr_interface.h>

static void __iomem *g_cmo0_va = NULL;
static void __iomem *g_actlr5_va = NULL;

DEFINE_SPINLOCK(g_cmo_lock);

void flush_cluster_cmo(void)
{
	unsigned long flag;
	unsigned long status;

	if (unlikely(g_cmo0_va == NULL || g_actlr5_va == NULL)) {
		pr_err("invalid g_cmo0_va or g_actlr5_va\n");
		BUG_ON(1);
	}

	dsb(sy);
	spin_lock_irqsave(&g_cmo_lock, flag);
	writeq(1, g_cmo0_va);
	dsb(sy);
	/* check whether cluster flush done */
	do
		status = readq(g_actlr5_va);
	while (status & BIT(SOC_FCM_NAMTSO_ACTLR5_wns_cluster_vld_START));
	dsb(sy);
	spin_unlock_irqrestore(&g_cmo_lock, flag);
}

#ifdef CONFIG_NAMTSO_DEBUG
void __flush_cluster_cmo(void)
{
	unsigned long status;

	if (unlikely(g_cmo0_va == NULL || g_actlr5_va == NULL)) {
		pr_err("invalid g_cmo0_va or g_actlr5_va\n");
		BUG_ON(1);
	}

	dsb(sy);
	writeq(1, g_cmo0_va);
	dsb(sy);
	/* check whether cluster flush done */
	do
		status = readq(g_actlr5_va);
	while (status & BIT(SOC_FCM_NAMTSO_ACTLR5_wns_cluster_vld_START));
	dsb(sy);
}
#endif

static int flush_cmo_init(void)
{
	g_cmo0_va = ioremap(SOC_FCM_NAMTSO_CMO0_ADDR(SOC_ACPU_namtso_cfg_BASE_ADDR), 0x8);
	if (g_cmo0_va == NULL) {
		pr_err("g_cmo0_va ioremap failed\n");
		WARN_ON(1);
	}

	g_actlr5_va = ioremap(SOC_FCM_NAMTSO_ACTLR5_ADDR(SOC_ACPU_namtso_cfg_BASE_ADDR), 0x8);
	if (g_actlr5_va == NULL) {
		pr_err("g_actlr5_va ioremap failed\n");
		iounmap(g_cmo0_va);
		WARN_ON(1);
	}

	return 0;
}
arch_initcall(flush_cmo_init);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Namtso Flush Cmo Driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
