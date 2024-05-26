/*
 *
 * regesiter_ops.c
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

#include "register_ops.h"
#include "helper/log/lowpm_log.h"
#include "helper/dtsi/dtsi_ops.h"

#include <soc_acpu_baseaddr_interface.h>
#include <m3_rdr_ddr_map.h>
#include <platform_include/basicplatform/linux/mfd/pmic_platform.h>

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/bits.h>

struct reg_base {
	const char *name;
	const char *alias;
	void __iomem *addr;
};

static struct reg_base g_sysreg_base[] = {
	{"hisilicon,sysctrl", "SYSCTRL", NULL},
	{"hisilicon,pctrl", "PMCCTRL", NULL},
	{"hisilicon,crgctrl", "CRGPERI", NULL},
	{"hisilicon,pmu", "PMU", NULL},
	{"lowpm,ipc", "IPC", NULL},
	{"lowpm,ao_ipc", "AO_IPC", NULL},
	{NULL, "REG_INVALID", NULL},
};

enum LP_REG_DOMAIN lp_get_reg_domain(const char *domain)
{
	enum LP_REG_DOMAIN i;

	if (domain == NULL)
		return REG_INVALID;

	for (i = SCTRL; i < REG_INVALID; i++) {
		if (strcmp(g_sysreg_base[i].alias, domain) == 0)
			return i;
	}

	return REG_INVALID;
}

inline unsigned int lp_pmic_readl(int offset)
{
	return pmic_read_reg((int)g_sysreg_base[PMU].addr + offset);
}

inline void lp_pmic_writel(int offset, int val)
{
	pmic_write_reg((int)g_sysreg_base[PMU].addr + offset, val);
}

inline unsigned int read_reg(enum LP_REG_DOMAIN reg_domain, int offset)
{
	return readl(g_sysreg_base[reg_domain].addr + offset);
}

inline void write_reg(enum LP_REG_DOMAIN reg_domain, int offset, int val)
{
	writel(val, g_sysreg_base[reg_domain].addr + offset);
}

inline void enable_bit(enum LP_REG_DOMAIN reg_domain, int offset, unsigned int nr)
{
	unsigned int val;

	val = read_reg(reg_domain, offset) | BIT(nr);
	writel(val, g_sysreg_base[reg_domain].addr + offset);
}

inline void disable_bit(enum LP_REG_DOMAIN reg_domain, int offset, unsigned int nr)
{
	unsigned int val;

	val = read_reg(reg_domain, offset) & (~BIT(nr));
	writel(val, g_sysreg_base[reg_domain].addr + offset);
}

int map_reg_base(enum LP_REG_DOMAIN domain)
{
	if (domain >= REG_INVALID)
		return -EINVAL;

	if (g_sysreg_base[domain].addr != NULL)
		return 0;

	if (g_sysreg_base[domain].name == NULL)
		return -EINVAL;

	g_sysreg_base[domain].addr = lp_map_dn(NULL, g_sysreg_base[domain].name);
	if (g_sysreg_base[domain].addr == NULL) {
		lp_err("map %s failed", g_sysreg_base[domain].name);
		return -ENOMEM;
	}

	lp_debug("map %s success", g_sysreg_base[domain].name);

	return 0;
}

#ifdef CONFIG_LPMCU_BB
static char *g_runtime_base;

inline unsigned int runtime_read(int offset)
{
	return readl((void __iomem *)(g_runtime_base + offset));
}

int init_runtime_base(void)
{
	if (g_runtime_base != NULL)
		return 0;

	if (M3_RDR_SYS_CONTEXT_BASE_ADDR == 0) {
		lp_err("M3_RDR_SYS_CONTEXT_BASE_ADDR is NULL");
		return -EINVAL;
	}

	g_runtime_base = M3_RDR_SYS_CONTEXT_BASE_ADDR;
	return 0;
}

inline int is_runtime_base_inited(void)
{
	if (g_runtime_base != NULL)
		return 0;

	return -EINVAL;
}
#endif
