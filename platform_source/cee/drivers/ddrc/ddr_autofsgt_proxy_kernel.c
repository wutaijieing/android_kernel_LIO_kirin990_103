/*
 * ddr_autofsgt_proxy_kernel.c
 *
 * autofsgt proxy kernel for ddr
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/bitops.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/semaphore.h>
#include <linux/printk.h>
#include "hisi_ddr_autofsgt_proxy_kernel.h"
#include <soc_acpu_baseaddr_interface.h>
#include <soc_crgperiph_interface.h>
#include <soc_sctrl_interface.h>

#define AUTOFSGT_PERICRG_SIZE	0x1000

static char *g_autofsgt_pericrg_base;
static struct semaphore g_ctrl_sem;

int ddr_autofsgt_bypass(unsigned int client, unsigned int enable)
{
	unsigned int mask_bit = client +
				SOC_CRGPERIPH_PERI_COMMON_CTRL1_bitmasken_START;

	if (enable != 0)
		writel(BIT(mask_bit) | BIT(client),
		       SOC_CRGPERIPH_PERI_COMMON_CTRL1_ADDR(g_autofsgt_pericrg_base));
	else
		writel(BIT(mask_bit),
		       SOC_CRGPERIPH_PERI_COMMON_CTRL1_ADDR(g_autofsgt_pericrg_base));

	return 0;
}

int ddr_autofsgt_opt(unsigned int client, unsigned int cmd)
{
	switch (cmd) {
	case DDR_AUTOFSGT_LOGIC_EN:
		ddr_autofsgt_bypass(client, 0);
		break;
	case DDR_AUTOFSGT_LOGIC_DIS:
		ddr_autofsgt_bypass(client, 1);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

int ddr_autofsgt_ctrl(unsigned int client, unsigned int cmd)
{
	int ret;
	unsigned int opt_cmd = 0;

	down(&g_ctrl_sem);

	switch (cmd) {
	case DDR_AUTOFSGT_LOGIC_EN:
		/* no bypass */
		opt_cmd = DDR_AUTOFSGT_LOGIC_EN;
		break;
	case DDR_AUTOFSGT_LOGIC_DIS:
		/* bypass */
		opt_cmd = DDR_AUTOFSGT_LOGIC_DIS;
		break;
	default:
		up(&g_ctrl_sem);
		return -EINVAL;
	}
	ret = ddr_autofsgt_opt(client, opt_cmd);
	if (ret != 0) {
		pr_err("[%s] opt_cmd err:[0x%x]\n", __func__, opt_cmd);
		up(&g_ctrl_sem);
		return -EINVAL;
	}

	up(&g_ctrl_sem);
	return 0;
}

static int __init ddr_autofsgt_proxy_init(void)
{
	sema_init(&g_ctrl_sem, 1);
	g_autofsgt_pericrg_base =
		(char *)ioremap((phys_addr_t)SOC_ACPU_PERI_CRG_BASE_ADDR, /*lint !e446*/
				AUTOFSGT_PERICRG_SIZE);

	if (g_autofsgt_pericrg_base == NULL)
		return -ENOMEM;

	return 0;
}

module_init(ddr_autofsgt_proxy_init);
