/*
 * ap_wake.c
 *
 * wakeup irq
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
#include "helper/debugfs/debugfs.h"
#include "helper/log/lowpm_log.h"
#include "helper/dtsi/dtsi_ops.h"
#include "helper/register/register_ops.h"
#include "gpio_wake.h"
#include "ipc_wake.h"

#include <securec.h>
#include <lpmcu_runtime.h>
#include <m3_rdr_ddr_map.h>
#ifdef CONFIG_POWER_DUBAI
#include <huawei_platform/power/dubai/dubai_wakeup_stats.h>
#endif

#include <linux/init.h>
#include <linux/bits.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/io.h>
#include <linux/compiler.h>
#include <linux/uaccess.h>

#define GIC_IRQ_GROUP_START 2
#define GIC_IRQ_GROUP_MAX 13

struct lp_gic {
	void __iomem *gic_pending_reg;
	u32 gic_pending_val;
	void __iomem *irq_mask_reg;
	u32 irq_mask_val;
} g_lp_gic_table[GIC_IRQ_GROUP_MAX];

const char **g_ap_irq_name;
static int g_ap_irq_num;
const static char **g_lpmcu_irq_name;
static int g_lpmcu_irq_num;

#define for_each_gic_irq_group(pos) \
	for (pos = GIC_IRQ_GROUP_START; pos < GIC_IRQ_GROUP_MAX; pos++)

#define bit_is_pending(pending_irqs, n) \
	((pending_irqs & BIT_MASK(n)) != 0)

#define bit_is_enabled(enable_mask, n) \
	((enable_mask & BIT_MASK(n)) != 0)

#define _runtime_read(offset) runtime_read(M3_RDR_SYS_CONTEXT_RUNTIME_VAR_OFFSET + (offset))
#define _runtime_write(offset, val) runtime_write(M3_RDR_SYS_CONTEXT_RUNTIME_VAR_OFFSET + (offset), val)

static void notify_dubai_wakeup(u32 irq, int gpio)
{
#ifdef CONFIG_POWER_DUBAI
	if ((int)irq >= g_ap_irq_num)
		return;

	dubai_log_irq_wakeup(DUBAI_IRQ_WAKEUP_TYPE_AP, g_ap_irq_name[irq], gpio);
#endif
}

static void dump_wake_irq(int irq, struct seq_file *s)
{
	int gpio;

	if (irq < g_ap_irq_num)
		lp_msg(s, "wake up irq num: %d, irq name: %s", irq, g_ap_irq_name[irq]);
	else
		lp_msg(s, "wake up irq num: %d, irq name: no name!", irq);

	gpio = pm_get_gpio_by_irq(irq);
	if (gpio >= 0)
		lp_msg(s, "(gpio-%d)", gpio);
	else
		lp_msg(s, "(not gpio)");

	/* cat ap_wake command cannt recorded by dubai */
	if (s == NO_SEQFILE)
		notify_dubai_wakeup(irq, gpio);
}

static void dump_irq_group(int group_index, unsigned int pending_irqs, struct seq_file *s)
{
	int i, irq;
	const int irq_num_per_gic = 32;

	for (i = 0; i < irq_num_per_gic; i++) {
		if (likely((!bit_is_pending(pending_irqs, i))))
			continue;

		if (!bit_is_enabled(g_lp_gic_table[group_index].irq_mask_val, i))
			continue;

		irq = group_index * irq_num_per_gic + i;
		dump_wake_irq(irq, s);
	}
}
#define PM_GIC_STEP 4
static int init_gic_base(const struct device_node *lp_dn)
{
	void __iomem *gic_reg_base = NULL;
	struct device_node *node = NULL;
	void __iomem *enable_start_reg = NULL;
	void __iomem *pending_start_reg = NULL;
	u32 reg_offset = 0;
	int ret, i;

	node = of_find_compatible_node(NULL, NULL, "arm,lp_gic");
	if (node == NULL) {
		lp_err("no gic compatible node found");
		return -ENODEV;
	}

	gic_reg_base = of_iomap(node, 0);
	if (gic_reg_base == NULL) {
		lp_err("iomap failed");
		goto err_put_node;
	}

	ret = of_property_read_u32(node, "enable-offset", &reg_offset);
	if (ret != 0) {
		lp_err("no enable-offset node");
		goto err_put_node;
	}
	enable_start_reg = gic_reg_base + reg_offset;

	ret = of_property_read_u32(node, "pending-offset", &reg_offset);
	if (ret != 0) {
		lp_err("no pending-offset node");
		goto err_put_node;
	}
	pending_start_reg = gic_reg_base + reg_offset;

	of_node_put(node);

	for_each_gic_irq_group(i) {
		g_lp_gic_table[i].irq_mask_reg = enable_start_reg + i * PM_GIC_STEP;
		g_lp_gic_table[i].gic_pending_reg = pending_start_reg + i * PM_GIC_STEP;
	}

	lp_debug("enable start = %pK, pending start = %pK",
		    enable_start_reg, pending_start_reg);

	return 0;

err_put_node:
	of_node_put(node);

	lp_err("failed %d", ret);
	return ret;
}

static int init_ap_irq_table(const struct device_node *lp_dn)
{
	g_ap_irq_name = lp_read_dtsi_strings(lp_dn, "ap-irq-table", NULL, &g_ap_irq_num);
	if (g_ap_irq_name == NULL) {
		lp_err("init ap irq name failed");
		return -ENOMEM;
	}

	return 0;
}

static int init_lpmcu_irq_table(const struct device_node *lp_dn)
{
	g_lpmcu_irq_name = lp_read_dtsi_strings(lp_dn, "lpmcu-irq-table", NULL, &g_lpmcu_irq_num);
	if (g_lpmcu_irq_name == NULL) {
		lp_err("init lpmcu irq name failed");
		return -ENOMEM;
	}

	return 0;
}

static void show_wake_irq_name(struct seq_file *s)
{
	int wake_irq;
	const char *irq_name = NULL;

	wake_irq = (int)_runtime_read(AP_WAKE_IRQ_OFFSET);
	if (g_lpmcu_irq_name == NULL || wake_irq >= g_lpmcu_irq_num) {
		lp_msg(s, "SR:AP WAKE IRQ(LPM3 NVIC): %d (no exist)", wake_irq);
		return;
	}
	irq_name = g_lpmcu_irq_name[wake_irq];

	lp_msg(s, "SR:AP WAKE IRQ(LPM3 NVIC): %d (%s)", wake_irq, irq_name);
}

static void show_pie_wake_irq(struct seq_file *s)
{
#ifdef AP_WAKE_IRQ_PIE
	lp_msg(s, "PIE VALUE: 0x%x", _runtime_read(AP_WAKE_IRQ_PIE_OFFSET));
#endif
}

static void gic_pending_dump(struct seq_file *s)
{
	int i;
	unsigned int pending_irqs;

	for_each_gic_irq_group(i) {
		pending_irqs = readl(g_lp_gic_table[i].gic_pending_reg);
		if (pending_irqs == 0)
			continue;

		dump_irq_group(i, pending_irqs, s);
	}
}
static int ap_wake_show_inner(struct seq_file *s, void *data)
{
	no_used(data);

	if (is_runtime_base_inited() != 0) {
		lp_msg(s, "runtime base not init");
		return -EINVAL;
	}

	show_pie_wake_irq(s);
	show_wake_irq_name(s);
	show_wake_ipc(s);
	gic_pending_dump(s);

	return 0;
}
static int ap_wake_show(void)
{
	return ap_wake_show_inner(NO_SEQFILE, NULL);
}

static int  save_gic_irq_mask(void)
{
	unsigned int i;

	for_each_gic_irq_group(i)
		g_lp_gic_table[i].irq_mask_val = readl(g_lp_gic_table[i].irq_mask_reg);

	return 0;
}

static struct sr_mntn g_sr_mntn_ap_wake = {
	.owner = "ap_wake",
	.enable = true,
	.suspend = save_gic_irq_mask,
	.resume = ap_wake_show,
};

static const struct lowpm_debugdir g_lpwpm_debugfs_ap_wake = {
	.dir = "lowpm_func",
	.files = {
		{"ap_wake", 0400, ap_wake_show_inner, NULL},
		{},
	},
};

static __init int init_sr_mntn_apwake(void)
{
	int ret;
	const struct device_node *lp_dn = NULL;

	if (sr_unsupported())
		return 0;

	lp_dn = lp_dn_root();

	ret = init_lpmcu_irq_table(lp_dn);
	if (ret != 0)
		goto init_fail;

	ret = init_ap_irq_table(lp_dn);
	if (ret != 0)
		goto init_fail;

	ret = init_gic_base(lp_dn);
	if (ret != 0)
		goto init_fail;

	ret = init_wake_gpio_table(lp_dn);
	if (ret != 0)
		goto init_fail;

	ret = init_ipc_table(lp_dn);
	if (ret != 0)
		goto init_fail;

	ret = lowpm_create_debugfs(&g_lpwpm_debugfs_ap_wake);
	if (ret != 0)
		goto init_fail;

	ret = register_sr_mntn(&g_sr_mntn_ap_wake, SR_MNTN_PRIO_M);
	if (ret != 0)
		goto init_fail;

	lp_crit("success");
	return 0;

init_fail:
	lp_err("fail %d", ret);
	lowpm_kfree(g_lpmcu_irq_name);
	lowpm_kfree(g_ap_irq_name);

	return ret;
}

late_initcall(init_sr_mntn_apwake);
