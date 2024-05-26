/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: Contexthub dump boot state process.
 * Create: 2021-12-20
 */

#include "iomcu_dump_priv.h"
#ifdef CONFIG_CONTEXTHUB_BOOT_STAT
#include <soc_sctrl_interface.h>
#endif

#ifdef CONFIG_CONTEXTHUB_BOOT_STAT
#define BOOT_STAT_REG_OFFSET SOC_SCTRL_SCBAKDATA27_ADDR(0)
#endif

static void __iomem *g_sysctrl_base = NULL;
static int g_nmi_reg;


int get_sysctrl_base(void)
{
	struct device_node *np = NULL;

	if (g_sysctrl_base == NULL) {
		np = of_find_compatible_node(NULL, NULL, "hisilicon,sysctrl");
		if (np == NULL) {
			pr_err("can not find  sysctrl node !\n");
			return -1;
		}

		g_sysctrl_base = of_iomap(np, 0);
		if (g_sysctrl_base == NULL) {
			pr_err("get g_sysctrl_base  error !\n");
			return -1;
		}
	}
	return 0;
}

int get_nmi_offset(void)
{
	struct device_node *sh_node = NULL;

	sh_node = of_find_compatible_node(NULL, NULL, "huawei,sensorhub_nmi");
	if (!sh_node) {
		pr_err("%s, can not find node sensorhub_nmi \n", __func__);
		return -1;
	}

	if (of_property_read_u32(sh_node, "nmi_reg", &g_nmi_reg)) {
		pr_err("%s:read nmi reg err:value is %d\n", __func__, g_nmi_reg);
		return -1;
	}

	pr_info("%s arch nmi reg is 0x%x\n", __func__, g_nmi_reg);
	return 0;
}

#ifdef CONFIG_CONTEXTHUB_BOOT_STAT
union sctrl_sh_boot get_boot_stat(void)
{
	union sctrl_sh_boot sh_boot;

	if (g_sysctrl_base != NULL)
		sh_boot.value = readl(g_sysctrl_base + BOOT_STAT_REG_OFFSET);
	else
		sh_boot.value = 0;
	pr_info("%s: sensorhub boot_step=%u, boot_stat=%u\n", __func__, sh_boot.reg.boot_step, sh_boot.reg.boot_stat);
	return sh_boot;
}

void reset_boot_stat(void)
{
	if (g_sysctrl_base != NULL)
		writel(0, g_sysctrl_base + BOOT_STAT_REG_OFFSET);
}
#endif

void send_nmi(void)
{
#ifdef CONFIG_CONTEXTHUB_BOOT_STAT
	(void)get_boot_stat();
#endif
	pr_warn("%s: g_sysctrl_base is %pK, nmi_reg is 0x%x\n", __func__, g_sysctrl_base, g_nmi_reg);
	if (g_sysctrl_base != NULL && g_nmi_reg != 0) {
		writel(0x2, g_sysctrl_base + g_nmi_reg); /* 0x2 only here */
		pr_err("IOMCU NMI\n");
	}
}

void clear_nmi(void)
{
	/* fix bug nmi can't be clear by iomcu, or iomcu will not start correctly */
	if ((unsigned int)readl(g_sysctrl_base + g_nmi_reg) & 0x2)
		pr_err("%s nmi remain!\n", __func__);
	writel(0, g_sysctrl_base + g_nmi_reg);
}