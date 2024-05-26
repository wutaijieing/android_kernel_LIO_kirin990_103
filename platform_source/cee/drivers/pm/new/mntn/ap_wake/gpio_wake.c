/*
 * gpio_wake.c
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
#include "helper/log/lowpm_log.h"
#include "helper/dtsi/dtsi_ops.h"

#include <soc_gpio_interface.h>
#ifdef CONFIG_POWER_DUBAI
#include <huawei_platform/power/dubai/dubai_wakeup_stats.h>
#endif

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/io.h>
#include <linux/errno.h>
#include <linux/slab.h>

#define GPIO_SECURITY 1
#define GPIO_NON_SEC 0
#define is_security_gpio(sec_attr) (sec_attr == GPIO_SECURITY)

#ifdef SOC_HI_GPIO_V500_SOURCE_DATA_ADDR
	#define GPIO_GROUP_STEP	32
	#define SOC_GPIO_GPIOMIS_ADDR(n) SOC_HI_GPIO_V500_INTR_MSK_##n##_ADDR(0)
#else
	#define GPIO_GROUP_STEP	8
#endif

struct gpio_group_info {
	u32 index;
	void __iomem *reg_base;
	/* kernel is in no security level, so it cannt access security gpio */
	u32 security;
};

struct wake_gpio_info {
	int irq;
	struct gpio_group_info group;
};

static struct wake_gpio_info *g_wake_gpio;
static int g_wake_gpio_num;

static int find_gpio_group_by_irq(int irq)
{
	int i;

	for (i = 0; i < g_wake_gpio_num; i++) {
		if (g_wake_gpio[i].irq == irq)
			return i;
	}

	lp_err("cannt find irq %d", irq);
	return -EINVAL;
}

int pm_get_gpio_by_irq(int irq)
{
	int index, gpio_id;
	unsigned int gpio_mis;

	if (g_wake_gpio_num <= 0 || g_wake_gpio == NULL) {
		lp_err("invalid irq %d, num %d", irq, g_wake_gpio_num);
		return -EINVAL;
	}

	index = find_gpio_group_by_irq(irq);
	if (index < 0)
		return -EINVAL;

	if (is_security_gpio(g_wake_gpio[index].group.security))
		return -EINVAL;

	gpio_mis = readl(g_wake_gpio[index].group.reg_base + SOC_GPIO_GPIOMIS_ADDR(0));
	if (gpio_mis == 0) {
		lp_err("mis is 0, maybe something is error");
		return -EINVAL;
	}

	gpio_id = first_setted_bit(gpio_mis);
	gpio_id += (int)(g_wake_gpio[index].group.index * GPIO_GROUP_STEP);

	return gpio_id;
}

static int init_wake_gpio_arr(struct device_node *lp_dn)
{
	g_wake_gpio_num = of_property_count_u32_elems(lp_dn, "ao-gpio-irq");
	if (g_wake_gpio_num < 0) {
		lp_err("no ao gpio irq");
		return -ENODEV;
	}

	lp_info("wake gpio num is %d", g_wake_gpio_num);

	g_wake_gpio = kcalloc(g_wake_gpio_num, sizeof(*g_wake_gpio), GFP_KERNEL);
	if (g_wake_gpio == NULL)
		return -ENOMEM;

	return 0;
}

static int init_gpio_wake_irq(struct device_node *np, int index)
{
	int ret;

	ret = of_property_read_u32_index(np, "ao-gpio-irq",
					 index, &(g_wake_gpio[index].irq));
	if (ret != 0) {
		lp_err("no ao-gpio-irq, %d", index);
		return -ENODEV;
	}

	return 0;
}

static int init_gpio_group_reg_base(struct device_node *gpio_dn, int index)
{
	void __iomem *base = of_iomap(gpio_dn, 0);

	if (base == NULL)
		return -ENODEV;

	g_wake_gpio[index].group.reg_base = base;

	return 0;
}

static void init_gpio_group_security(const struct device_node *gpio_dn, int index)
{
	int ret;

	ret = of_property_read_u32(gpio_dn, "secure-mode",
				   &g_wake_gpio[index].group.security);
	if (ret != 0)
		g_wake_gpio[index].group.security = GPIO_NON_SEC;
}

#define GPIO_DTSI_COMPATIBLE "arm,pl061"
static int init_gpio_group(struct device_node *lp_dn, int index)
{
	int ret;
	struct device_node *gpio_dn = NULL;

	ret = of_property_read_u32_index(lp_dn, "ao-gpio-group-idx",
					 index, &(g_wake_gpio[index].group.index));
	if (ret != 0) {
		lp_err("no wake gpio: %d", index);
		return -ENODEV;
	}

	gpio_dn = lp_find_node_index(NULL, "arm,primecell", g_wake_gpio[index].group.index);
	if (gpio_dn == NULL)
		return -ENODEV;

	ret = init_gpio_group_reg_base(gpio_dn, index);
	if (ret != 0)
		goto free_node;

	init_gpio_group_security(gpio_dn, index);

	of_node_put(gpio_dn);
	return 0;

free_node:
	of_node_put(gpio_dn);
	return ret;
}

int init_wake_gpio_table(struct device_node *lp_dn)
{
	int ret, i;

	if (init_wake_gpio_arr(lp_dn) != 0)
		return -ENOMEM;

	for (i = 0; i < g_wake_gpio_num; i++) {
		ret = init_gpio_wake_irq(lp_dn, i);
		if (ret != 0)

			goto err;
		ret = init_gpio_group(lp_dn, i);
		if (ret != 0)
			goto err;
	}

	return 0;

err:
	lowpm_kfree(g_wake_gpio);
	g_wake_gpio_num = 0;

	return ret;
}
