/*
 * clock.c
 *
 * debug information of suspend
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
#include "helper/dtsi/dtsi_ops.h"
#include "helper/debugfs/debugfs.h"
#include "helper/register/register_ops.h"

#include <linux/seq_file.h>
#include <linux/of.h>
#include <linux/errno.h>

struct lp_clk_info {
	unsigned int num;
	const char **name;
};

struct lp_clk_reg {
	unsigned int offset;
	enum LP_REG_DOMAIN domain;
	const char *domain_name;
};

struct lp_clk_group {
	struct lp_clk_reg reg;
	struct lp_clk_info clks;
};

static struct lp_clk_group *g_lp_clk_groups;
static unsigned int g_lp_clk_group_num;
static unsigned int g_lp_clk_num;

static int init_group_reginfo(const struct device_node *dn, struct lp_clk_reg *reg)
{
	int ret;

	ret = of_property_read_string(dn, "lp-clk-reg-domain", &reg->domain_name);
	if (ret != 0) {
		lp_err("no lp-clk-reg-domain");
		return -ENODEV;
	}
	reg->domain = lp_get_reg_domain(reg->domain_name);
	if (reg->domain >= REG_INVALID) {
		lp_err("invalid reg domain %s", reg->domain_name);
		return -EINVAL;
	}

	ret = map_reg_base(reg->domain);
	if (ret != 0) {
		lp_err("map reg %s failed", reg->domain_name);
		return -EFAULT;
	}

	ret = of_property_read_u32(dn, "offset", &reg->offset);
	if (ret != 0) {
		lp_err("no offset!");
		return -EINVAL;
	}

	return 0;
}

static int init_clks_in_group(const struct device_node *dn, struct lp_clk_info *clks)
{
	int ret, nums;

	ret = of_property_read_u32(dn, "bit-num", &clks->num);
	if (ret != 0) {
		lp_err("no bit-num");
		return -ENODEV;
	}

	clks->name = kcalloc(clks->num, sizeof(*clks->name), GFP_KERNEL);
	if (clks->name == NULL)
		return -ENOMEM;

	nums = of_property_read_string_array(dn, "lp-clk-name", clks->name, clks->num);
	if (nums != (int)clks->num) {
		lp_err("mismatch %d %d", nums, clks->num);
		return -EINVAL;
	}

	return 0;
}

static int init_clk_group(struct device_node *dn, struct lp_clk_group *gp)
{
	int ret;

	ret = init_group_reginfo(dn, &gp->reg);
	if (ret != 0)
		return ret;

	ret = init_clks_in_group(dn, &gp->clks);
	if (ret != 0)
		return ret;

	return 0;
}

static int init_clk_group_table(void)
{
	int ret;
	unsigned int i;
	struct device_node *dn = NULL;

	g_lp_clk_group_num = 0;
	for_each_compatible_node(dn, NULL, "lowpm_sr_mntn_clock")
		g_lp_clk_group_num++;

	lp_info("find lp clk num: %u", g_lp_clk_group_num);

	g_lp_clk_groups = kcalloc(g_lp_clk_group_num, sizeof(*g_lp_clk_groups), GFP_KERNEL);
	if (g_lp_clk_groups == NULL)
		goto err;

	i = 0;
	dn = NULL;
	g_lp_clk_num = 0;
	for_each_compatible_node(dn, NULL, "lowpm_sr_mntn_clock") {
		ret = init_clk_group(dn, &g_lp_clk_groups[i]);
		if (ret != 0)
			goto err;

		g_lp_clk_num += g_lp_clk_groups[i].clks.num;
		i++;
	}

	return 0;

err:
	for (i = 0; (i < g_lp_clk_group_num) && unlikely(g_lp_clk_groups != NULL); i++)
		lowpm_kfree(g_lp_clk_groups[i].clks.name);
	lowpm_kfree(g_lp_clk_groups);
	g_lp_clk_group_num = 0;
	g_lp_clk_num = 0;

	return -EFAULT;
}

static void clk_show_one_group(struct seq_file *s, struct lp_clk_group *gp)
{
	unsigned int i;
	u32 clk_status;

	lp_msg(s, "[%s] offset:0x%x regval:0x%x",
		gp->reg.domain_name, gp->reg.offset,
		read_reg(gp->reg.domain, gp->reg.offset));

	for (i = 0; i < gp->clks.num; i++) {
		clk_status = read_reg(gp->reg.domain, gp->reg.offset) & BIT(i);

		lp_msg(s, " bit_idx:%02d state:%s %s", i,
			    (clk_status != 0) ? "enable" : "disable",
			    gp->clks.name[i]);
	}
}

static int clk_status_show_inner(struct seq_file *s, void *data)
{
	unsigned int i;

	no_used(data);

	if (g_lp_clk_group_num <= 0 || g_lp_clk_groups == NULL) {
		lp_msg(s, "clock show: there is no data.");
		return -ENODEV;
	}

	lp_msg(s, "clock list length is %u:", g_lp_clk_num);

	for (i = 0; i < g_lp_clk_group_num; i++)
		clk_show_one_group(s, &g_lp_clk_groups[i]);

	return 0;
}

static int clk_status_show(void)
{
	return clk_status_show_inner(NO_SEQFILE, NULL);
}

static const struct lowpm_debugdir g_lpwpm_debugfs_clock = {
	.dir = "lowpm_func",
	.files = {
		{"clock_status", 0400, clk_status_show_inner, NULL},
		{},
	},
};

static struct sr_mntn g_sr_mntn_clock = {
	.owner = "clock_status",
	.enable = false,
	.suspend = clk_status_show,
	.resume = NULL,
};

static __init int init_sr_mntn_clock(void)
{
	int ret;

	if (sr_unsupported())
		return 0;

	ret = init_clk_group_table();
	if (ret != 0) {
		lp_err("register mntn module failed");
		return ret;
	}

	ret = lowpm_create_debugfs(&g_lpwpm_debugfs_clock);
	if (ret != 0) {
		lp_err("create debug sr file failed");
		return ret;
	}

	ret = register_sr_mntn(&g_sr_mntn_clock, SR_MNTN_PRIO_L);
	if (ret != 0) {
		lp_err("register mntn module failed");
		return ret;
	}


	lp_crit("success");

	return 0;
}

late_initcall(init_sr_mntn_clock);
