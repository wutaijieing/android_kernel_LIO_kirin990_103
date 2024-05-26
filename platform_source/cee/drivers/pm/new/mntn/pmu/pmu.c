/*
 * pmu.c
 *
 * debug informatpmun of buck & ldo for suspend
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License verspmun 2, as published by the Free Software Foundatpmun, and
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
#include "helper/register/register_ops.h"
#include "helper/debugfs/debugfs.h"

#include <linux/seq_file.h>
#include <linux/of.h>
#include <linux/errno.h>
#include <linux/init.h>

#define CLOSE_BY_PERI_EN	0x00000000
#define ECO_BY_PERI_EN		0x00000002
#define DISABLE_BY_LPMCU	0x00000004
#define DISABLE_BY_ACPU		0x00000008
#define CLOSE_BY_CLK_EN		0x00000010
#define NULL_OWNER		0x00000020
#define ECO_BY_SOFTWARE		0x00000040

struct power_info {
	const char *name;
	const char *desc;
	const char *howto_disable;
	unsigned int controller;
	unsigned int offset;
	unsigned int en_mask;
};
static struct power_info *g_lp_pmu_info;
static unsigned int g_lp_power_num;

static int init_one_power(struct device_node *np, struct power_info *pw)
{
	int ret;

	ret = of_property_read_string(np, "lp-pmu-name", &(pw->name));
	if (ret != 0) {
		lp_err("no lp-pmu-name");
		return -ENODEV;
	}
	ret = of_property_read_string(np, "lp-pmu-module", &(pw->desc));
	if (ret != 0) {
		lp_err("no desc: lp-pmu-name");
		return -ENODEV;
	}
	ret = of_property_read_u32(np, "lp-pmu-status-value", &(pw->controller));
	if (ret != 0) {
		lp_err("no controller: lp-pmu-status-value");
		return -ENODEV;
	}
	ret = of_property_read_string(np, "lp-pmu-status", &(pw->howto_disable));
	if (ret != 0) {
		lp_err("no how to disable desc: lp-pmu-status");
		return -ENODEV;
	}
	ret = of_property_read_u32(np, "offset", &(pw->offset));
	if (ret != 0) {
		lp_err("no offset");
		return -ENODEV;
	}
	ret = of_property_read_u32(np, "mask", &(pw->en_mask));
	if (ret != 0) {
		lp_err("no mask: control bit");
		return -ENODEV;
	}

	return 0;
}

static int init_power_info_table(void)
{
	int i, ret;
	struct device_node *dn = NULL;

	g_lp_power_num = 0;
	for_each_compatible_node(dn, NULL, "lowpm_sr_mntn_pmu")
		g_lp_power_num++;

	lp_info("find lp pmu num: %u", g_lp_power_num);

	g_lp_pmu_info = kcalloc(g_lp_power_num, sizeof(*g_lp_pmu_info), GFP_KERNEL);
	if (g_lp_pmu_info == NULL)
		goto err;

	dn = NULL;
	i = 0;
	for_each_compatible_node(dn, NULL, "lowpm_sr_mntn_pmu") {
		ret = init_one_power(dn, &g_lp_pmu_info[i++]);
		if (ret != 0)
			goto err;
	}

	return 0;

err:
	lowpm_kfree(g_lp_pmu_info);
	g_lp_power_num = 0;

	return -EFAULT;
}

static const int align_left = -1;
static const int index_width = 8 * align_left;
static const int name_width = 16 * align_left;
static const int offset_width = 8 * align_left;
static const int value_width = 8 * align_left;
static const int cur_width = 12 * align_left;
static const int tar_width = 16 * align_left;
static const int err_width = 8 * align_left;
static const int desc_width = 64 * align_left;
static int pmu_status_show_inner(struct seq_file *s, void *data)
{
	unsigned int i, value;
	struct power_info *pw = NULL;

	no_used(data);

	lp_msg(s, "PMU_CTRL register list length is %d:", g_lp_power_num);
	if (g_lp_pmu_info == NULL)
		return -EINVAL;

	lp_msg(s, "%*s%*s%*s%*s%*s%*s%*s%*s",
		  index_width, "index", name_width, "name",
		  offset_width, "offset", value_width, "value",
		  tar_width, "target", cur_width, "cur_status",
		  err_width, "error", desc_width, "desc");

	for (i = 0; i < g_lp_power_num; i++) {
		pw = &g_lp_pmu_info[i];
		value = lp_pmic_readl(pw->offset << 2);

		lp_msg_cont(s, "%*d", index_width, i);
		lp_msg_cont(s, "%*s", name_width, pw->name);
		lp_msg_cont(s, "%#*X", offset_width, pw->offset);
		lp_msg_cont(s, "%#*X", value_width, value);
		lp_msg_cont(s, "%*s", tar_width, pw->howto_disable);

		if ((value & pw->en_mask) == pw->en_mask) {
			lp_msg_cont(s, "%*s", cur_width, "enabled");
			lp_msg_cont(s, "%*s", err_width, (pw->controller & DISABLE_BY_ACPU) ? "-E" : "");
		} else {
			lp_msg_cont(s, "%*s", cur_width, "disabled");
			lp_msg_cont(s, "%*s", err_width, "");
		}

		lp_msg_cont(s, "%*s", desc_width, pw->desc);

		lp_msg_cont(s, "\n");
	}

	return 0;
}

static int pmu_status_show(void)
{
	return pmu_status_show_inner(NO_SEQFILE, NULL);
}
static const struct lowpm_debugdir g_lpwpm_debugfs_pmu = {
	.dir = "lowpm_func",
	.files = {
		{"pmu_status", 0400, pmu_status_show_inner, NULL},
		{},
	},
};

static struct sr_mntn g_sr_mntn_pmu = {
	.owner = "pmu_status",
	.enable = false,
	.suspend = pmu_status_show,
	.resume = NULL,
};

static __init int init_sr_mntn_pmu(void)
{
	int ret;

	if (sr_unsupported())
		return 0;

	ret = init_power_info_table();
	if (ret != 0) {
		lp_err("init_power_info_table failed");
		return ret;
	}

	ret = lowpm_create_debugfs(&g_lpwpm_debugfs_pmu);
	if (ret != 0) {
		lp_err("create debug sr file failed");
		return ret;
	}

	ret = register_sr_mntn(&g_sr_mntn_pmu, SR_MNTN_PRIO_L);
	if (ret != 0) {
		lp_err("register mntn module failed");
		return ret;
	}

	lp_crit("success");

	return 0;
}

late_initcall(init_sr_mntn_pmu);
