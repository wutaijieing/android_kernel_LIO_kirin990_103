/*
 * dtsi_ops.c
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

#include "pub_def.h"
#include "helper/log/lowpm_log.h"

#include <securec.h>

#include <linux/compiler_types.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_address.h>

#define BUFFER_LEN 40

static struct device_node *g_lowpm_dn_root;

const struct device_node *lp_dn_root(void)
{
	if (g_lowpm_dn_root != NULL)
		return g_lowpm_dn_root;

	g_lowpm_dn_root = of_find_compatible_node(NULL, NULL, "hisilicon,lowpm_func");
	return g_lowpm_dn_root;
}

void __iomem *lp_map_dn(struct device_node *from, const char *dn_name)
{
	void __iomem *base = NULL;
	struct device_node *np = NULL;

	if (dn_name == NULL)
		return NULL;

	np = of_find_compatible_node(from, NULL, dn_name);
	if (np == NULL) {
		lp_err("dts[%s] node not found", dn_name);
		return NULL;
	}

	base = of_iomap(np, 0);
	if (base == NULL)
		lp_err("%s of iomap fail", base);

	of_node_put(np);

	return base;
}

static int lp_fill_compatible(char *buffer, int len, const char *pre, int index)
{
	int ret;

	ret = snprintf_s(buffer, len, len - 1, "%s%d", pre, index);
	if (ret < 0) {
		pr_err("%s: snprintf_s buffer err, %s%d.\n", __func__, pre, index);
		return ret;
	}

	buffer[ret] = 0;

	return 0;
}

struct device_node *lp_find_node_index(struct device_node *from, const char *pre, int index)
{
	char compatible[BUFFER_LEN];

	lp_fill_compatible(compatible, BUFFER_LEN, pre, index);

	return of_find_compatible_node(from, NULL, compatible);
}

void __iomem *lp_map_dn_index(struct device_node *from, const char *pre, int index)
{
	char dn_name[BUFFER_LEN];

	if (lp_fill_compatible(dn_name, BUFFER_LEN, pre, index) != 0)
		return NULL;

	return lp_map_dn(from, dn_name);
}

const char **lp_read_dtsi_strings(const struct device_node *np, const char *propname,
				  const char *propsize_name, int *size)
{
	int count, num, ret;
	const char **strings;

	if (np == NULL || propname == NULL)
		return NULL;
	if (size == NULL && propsize_name == NULL)
		return NULL;

	if (propsize_name != NULL) {
		ret = of_property_read_u32(np, propsize_name, &count);
		if (ret != 0) {
			lp_err("read err for %s", propname);
			return NULL;
		}
	} else {
		count = of_property_count_strings(np, propname);
		if (count <= 0) {
			lp_err("read err for %s", propname);
			return NULL;
		}
	}

	strings = kcalloc(count, sizeof(char *), GFP_KERNEL);
	if (strings == NULL)
		return NULL;

	num = of_property_read_string_helper(np, propname, strings, count, 0);
	if (num != count) {
		lp_err("only read %d strings in total %d", count, num);
		lowpm_kfree(strings);
		return NULL;
	}

	if (size != NULL)
		*size = num;

	return strings;
}

int lp_read_property_u32_array(const struct device_node *np, const char *name, u32 **arr)
{
	int count, ret;
	u32 *out = NULL;

	count = of_property_count_u32_elems(np, name);
	if (count < 0) {
		lp_err("no dn %s", name);
		return -ENODEV;
	}

	out = kcalloc(count, sizeof(*out), GFP_KERNEL);
	if (out == NULL)
		return -ENOMEM;

	ret = of_property_read_u32_array(np, name, out, count);
	if (ret != 0) {
		lp_err("parsing %s: failed %d", name, ret);
		lowpm_kfree(out);
		return -EINVAL;
	}

	*arr = out;
	return count;
}

bool lp_is_fpga(void)
{
	static int fpga_flag = -1;
	int ret;

	if (fpga_flag == -1) {
		fpga_flag = 0;
		ret = of_property_read_u32(lp_dn_root(), "fpga_flag", &fpga_flag);
		if (ret != 0)
			lp_debug("not fpga");
	}
	return fpga_flag == 1;
}
