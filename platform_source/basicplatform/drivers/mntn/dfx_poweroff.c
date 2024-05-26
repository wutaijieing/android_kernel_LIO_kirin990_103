/*
 *
 * Power off or Reboot the hi3xxx device
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/kmsg_dump.h>
#include <linux/reboot.h>
#include <linux/delay.h>
#include <platform_include/basicplatform/linux/mfd/pmic_platform.h>
#include <platform_include/basicplatform/linux/pr_log.h>
#include <linux/pm.h>
#include <linux/gpio.h>

#include <asm/cacheflush.h>
#include <asm/system_misc.h>

#include <soc_gpio_interface.h>
#include <soc_sctrl_interface.h>
#ifdef CONFIG_PRODUCT_CDC_ACE
#include <vsoc_sctrl_interface.h>
#endif
#ifndef SOC_GPIO_GPIODIR_ADDR
#include <soc_gpio_interface.h>
#endif

#include "blackbox/rdr_print.h"
#include "pmic_interface.h"

#ifdef CONFIG_DISABLE_DMSS_INT
#include "bl31/dfx_bl31_exception.h"
#endif
#define PR_LOG_TAG POWEROFF_TAG
#define IOMAP_ADD_INDEX 0
#define POWERHOLD_PROTECT_BIT_OFFSET 16
#define POWERHOLD_GPIO_BIT_OFFSET 2
#define GPIO_PULL_DOWN 0
#define UNUSED_PARAMETER(x) (void)(x)

static void __iomem *powerhold_gpio_base = NULL;
static unsigned int g_powerhold_gpio_offset;
static unsigned int g_powerhold_protect_offset = 0xFFFF;
static unsigned int g_powerhold_protect_bit = 0xFF;
static bool g_gpio_v500_flag = false;
static unsigned int g_powerhold_gpio = 0xFF;

#ifdef CONFIG_DFX_BB
#include <platform_include/basicplatform/linux/rdr_platform.h>
#include "blackbox/rdr_inner.h"
#endif

static void __iomem *sysctrl_base = NULL;

#ifdef CONFIG_PRODUCT_CDC_ACE
static void *g_poweroff_ctrl_reg = NULL;
static void *g_reset_ctrl_reg = NULL;
static void dfx_ap_notify_uvmm_poweroff(void)
{
	BB_PRINT_PN("%s: system power off now", __func__);
	if (!g_poweroff_ctrl_reg) {
		BB_PRINT_ERR("%s: g_poweroff_ctrl_reg is NULL", __func__);
		return;
	}

	writel(VSOC_SCTRL_POWEROFF_VALUE, g_poweroff_ctrl_reg);
}

static void dfx_ap_notify_uvmm_reset(void)
{
	BB_PRINT_PN("%s: system reset now", __func__);
	if (!g_reset_ctrl_reg) {
		BB_PRINT_ERR("%s: g_reset_ctrl_reg is NULL", __func__);
		return;
	}

	writel(VSOC_SCTRL_RESTART_VALUE, g_reset_ctrl_reg);
}
#else
static void clear_powerhold_protect(void)
{
	uintptr_t protect_val;

	if (g_powerhold_protect_offset != 0xFFFF) {
		/* we need clear protect bit and set mask bit */
		protect_val = readl((void *)sysctrl_base + g_powerhold_protect_offset);
		if ((protect_val & BIT(g_powerhold_protect_bit)) == 0)
			BB_PRINT_ERR("protect_offset = 0x%x, protect_bit = 0x%x"
				"powerhold protect is already cleared!\n",
				g_powerhold_protect_offset, g_powerhold_protect_bit);
		protect_val = (protect_val & (~BIT(g_powerhold_protect_bit)))
						| (BIT(g_powerhold_protect_bit + POWERHOLD_PROTECT_BIT_OFFSET));
		writel(protect_val, (void *)sysctrl_base + g_powerhold_protect_offset);
	}
}

static void dfx_powerhold_pulldown(void)
{
	unsigned int out_dir;

	while (1) {
		if (powerhold_gpio_base != NULL) {
			pr_info("system power off now\n");
			/* clear powerhold protect */
			clear_powerhold_protect();
#ifdef SOC_GPIO_GPIODIR_ADDR
			/* set direction */
			out_dir = readl(SOC_GPIO_GPIODIR_ADDR(powerhold_gpio_base));
			out_dir |= (1u << g_powerhold_gpio_offset);
			writel(out_dir, SOC_GPIO_GPIODIR_ADDR(powerhold_gpio_base));
			writel(0, powerhold_gpio_base + (1u << (g_powerhold_gpio_offset + POWERHOLD_GPIO_BIT_OFFSET)));
#else
			UNUSED_PARAMETER(out_dir);
			if (g_gpio_v500_flag && (g_powerhold_gpio != 0xFF)) {
				/* set data_reg val */
				int ret = -1;
				ret = gpio_request(g_powerhold_gpio, "dfx_poweroff");
				if (ret < 0)
					pr_err("%s[%u]: request gpio %u fail!\n", __func__, __LINE__, g_powerhold_gpio);

				ret = gpio_direction_output(g_powerhold_gpio, GPIO_PULL_DOWN);
				if (ret < 0)
					pr_err("%s[%d]: shutdown fail\n", __func__, __LINE__);
			}
			(void)out_dir;
#endif
			mdelay(1000); /* wait 1s before machine_restart */
			machine_restart("chargereboot");
		}
	}
}
#endif

static void dfx_pm_system_off(void)
{
#ifdef CONFIG_DFX_BB
#ifndef CONFIG_DFX_REBOOT_REASON_SEC
	set_reboot_reason(AP_S_COLDBOOT);
#else
	set_reboot_reason_subtype(AP_S_COLDBOOT, 0);
#endif
#endif

#ifdef CONFIG_PRODUCT_CDC_ACE
	dfx_ap_notify_uvmm_poweroff();
#else
	dfx_powerhold_pulldown();
#endif
}

#ifdef CONFIG_DFX_BB
void dfx_pm_system_reset_comm(const char *cmd)
{
	unsigned int i;
	unsigned int curr_reboot_type = UNKNOWN;
	const struct cmdword *reboot_reason_map = NULL;

	if (cmd == NULL || *cmd == '\0') {
		BB_PRINT_PN("%s cmd is null\n", __func__);
		cmd = "COLDBOOT";
	} else {
		BB_PRINT_PN("%s cmd is %s\n", __func__, cmd);
	}

	reboot_reason_map = get_reboot_reason_map();
	if (reboot_reason_map == NULL) {
		BB_PRINT_ERR("reboot_reason_map is NULL\n");
		return;
	}
	for (i = 0; i < get_reboot_reason_map_size(); i++) {
		if (!strncmp((char *)reboot_reason_map[i].name, cmd, sizeof(reboot_reason_map[i].name))) {
			curr_reboot_type = reboot_reason_map[i].num;
			break;
		}
	}
	if (curr_reboot_type == UNKNOWN) {
		curr_reboot_type = COLDBOOT;
		console_verbose();
		dump_stack();
	}
#ifndef CONFIG_DFX_REBOOT_REASON_SEC
	set_reboot_reason(curr_reboot_type);
#else
	set_reboot_reason_subtype(curr_reboot_type, 0);
#endif
}

#ifdef CONFIG_DFX_QIC
#define RESET_KEYPOINT_FLAG 0x66
#endif

/*
 * Description:   Callback function registered with the machine_restart function.
 *                When the system is reset, the callback function is invoked
 * Input:         cmd  Reset Type
 * Output:        NA
 * Return:        NA
 */
static void dfx_pm_system_reset(enum reboot_mode mode, const char *cmd)
{
	dfx_pm_system_reset_comm(cmd);
	/* if EFI_RUNTIME_SERVICES enabled, then uefi is responsible of system reset */
#ifdef CONFIG_DFX_QIC
	/* 3moths stable test,then open this function for all soc */
	dfx_ap_set_reset_keypoint(RESET_KEYPOINT_FLAG);
#endif
#ifdef CONFIG_DISABLE_DMSS_INT
	(void)bl31_disable_dmss_int_smc();
#endif

#ifdef CONFIG_PRODUCT_CDC_ACE
	dfx_ap_notify_uvmm_reset();
#else
	dfx_ap_nmi_notify_lpm3();
	BB_PRINT_PN("ap send nmi to lpm3, then goto dead loop\n");
#endif
	while (1);
}
#else
void dfx_pm_system_reset_comm(const char *cmd)
{
	if (cmd == NULL || *cmd == '\0') {
		BB_PRINT_PN("%s cmd is null\n", __func__);
		cmd = "COLDBOOT";
	} else {
		BB_PRINT_PN("%s cmd is %s\n", __func__, cmd);
	}
}

/*
 * Description:   Callback function registered with the machine_restart function.
 *                When the system is reset, the callback function is invoked
 * Input:         cmd  Reset Type
 * Output:        NA
 * Return:        NA
 */
static void dfx_pm_system_reset(enum reboot_mode mode, const char *cmd)
{
	dfx_pm_system_reset_comm(cmd);
	BB_PRINT_PN("ap send nmi to lpm3, then goto dead loop\n");
	while (1);
}
#endif

static void dfx_powerhold_init(void)
{
	struct device_node *np = NULL;
	unsigned int offset = 0;
	int ret;

	/* get powerhold gpio */
	np = of_find_compatible_node(NULL, NULL, "hisilicon,powerhold");
	if (np == NULL) {
		BB_PRINT_ERR("get powerhold np error !\n");
		return;
	}

	powerhold_gpio_base = of_iomap(np, IOMAP_ADD_INDEX);
	if (powerhold_gpio_base == NULL)
		BB_PRINT_ERR("%s: powerhold_gpio_base is NULL\n", __func__);

	ret = of_property_read_u32(np, "offset", (u32 *)&offset);
	if (ret) {
		BB_PRINT_ERR("get offset error !\n");
		if (powerhold_gpio_base) {
			iounmap(powerhold_gpio_base);
			powerhold_gpio_base = 0;
		}
	} else {
		pr_info("offset = 0x%x !\n", offset);
		g_powerhold_gpio_offset = offset;
	}

	ret = of_property_read_u32(np, "powerhold_protect_offset", (u32 *)&offset);
	if (ret) {
		BB_PRINT_ERR("no powerhold_protect_offset !\n");
	} else {
		pr_info("powerhold_protect_offset = 0x%x !\n", offset);
		g_powerhold_protect_offset = offset;
	}

	ret = of_property_read_u32(np, "powerhold_protect_bit", (u32 *)&offset);
	if (ret) {
		BB_PRINT_ERR("no powerhold_protect_bit !\n");
		g_powerhold_protect_offset = 0xFFFF;
	} else {
		pr_info("powerhold_protect_bit = 0x%x !\n", offset);
		g_powerhold_protect_bit = offset;
	}

	g_gpio_v500_flag = of_property_read_bool(np, "gpio_v500_flag");
	if (g_gpio_v500_flag) {
		pr_info("gpio version is v500 !\n");
		ret = of_property_read_u32(np, "powerhold_gpio", (u32 *)&offset);
		if (ret) {
			BB_PRINT_ERR("no powerhold_gpio !\n");
		} else {
			pr_info("powerhold_gpio = %u !\n", offset);
			g_powerhold_gpio = offset;
		}
	}
}

static int dfx_sysctrl_init(void)
{
	struct device_node *np = NULL;

	np = of_find_compatible_node(NULL, NULL, "hisilicon,sysctrl");
	if (np == NULL) {
		BB_PRINT_ERR("%s: sysctrl No compatible node found\n", __func__);
		return -ENODEV;
	}

	sysctrl_base = of_iomap(np, IOMAP_ADD_INDEX);
	if (sysctrl_base == NULL) {
		BB_PRINT_ERR("%s: sysctrl_base is NULL\n", __func__);
		return -ENOMEM;
	}

	return 0;
}

#ifdef CONFIG_PRODUCT_CDC_ACE
static int dfx_vsysctrl_init(void)
{
	g_poweroff_ctrl_reg = ioremap(VSOC_SCTRL_POWEROFF_REG_ADDR, VSOC_SCTRL_NORMAL_REG_SIZE);
	if (!g_poweroff_ctrl_reg) {
		BB_PRINT_ERR("FUNC[%s]: ioremap(0x%lx, %d) error\n", __func__,
			VSOC_SCTRL_POWEROFF_REG_ADDR, VSOC_SCTRL_NORMAL_REG_SIZE);
		return -ENOMEM;
	}

	g_reset_ctrl_reg = ioremap(VSOC_SCTRL_RESTART_REG_ADDR, VSOC_SCTRL_NORMAL_REG_SIZE);
	if (!g_reset_ctrl_reg) {
		BB_PRINT_ERR("FUNC[%s]: ioremap(0x%lx, %d) error\n", __func__,
			VSOC_SCTRL_RESTART_REG_ADDR, VSOC_SCTRL_NORMAL_REG_SIZE);
		return -ENOMEM;
	}

	return 0;
}
#endif

static int hi3xxx_reset_probe(struct platform_device *pdev)
{
	int ret;

	dfx_powerhold_init();

	ret = dfx_sysctrl_init();
	if (ret)
		return ret;

#ifdef CONFIG_PRODUCT_CDC_ACE
	ret = dfx_vsysctrl_init();
	if (ret)
		return ret;
#endif

	pm_power_off = dfx_pm_system_off;
	arm_pm_restart = dfx_pm_system_reset;
	return 0;
}

static int hi3xxx_reset_remove(struct platform_device *pdev)
{
	if (pm_power_off == dfx_pm_system_off)
		pm_power_off = NULL;

	if (arm_pm_restart == dfx_pm_system_reset)
		arm_pm_restart = NULL;

	return 0;
}

static const struct of_device_id of_hi3xxx_reset_match[] = {
	{ .compatible = "hisilicon,hi3xxx-reset", },
	{},
};

static struct platform_driver hi3xxx_reset_driver = {
	.probe = hi3xxx_reset_probe,
	.remove = hi3xxx_reset_remove,
	.driver = {
		.name = "hi3xxx-reset",
		.owner = THIS_MODULE,
		.of_match_table = of_hi3xxx_reset_match,
	},
};

static int __init hi3xxx_reset_init(void)
{
	return platform_driver_register(&hi3xxx_reset_driver);
}
module_init(hi3xxx_reset_init);

static void __exit hi3xxx_reset_exit(void)
{
	platform_driver_unregister(&hi3xxx_reset_driver);
}
module_exit(hi3xxx_reset_exit);

MODULE_LICENSE("GPL v2");
