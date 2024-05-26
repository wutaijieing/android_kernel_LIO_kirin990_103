/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: Combophy driver for Apr
 * Create: 2019-6-16
 *
 * This software is distributed under the terms of the GNU General
 * Public License ("GPL") as published by the Free Software Foundation,
 * either version 2 of that License or (at your option) any later version.
 */
#include <linux/module.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/of_address.h>

#include "dwc3-chip.h"
#include "usb_dp_ctrl_reg.h"
#include "chip_usb3_31phy.h"
#include "common.h"
#include "combophy_regcfg.h"

/* PMCTRL */
#define PMCTRL_NOC_POWER_IDLEREQ_0	0x380
#define PMCTRL_NOC_POWER_IDLEACK_0	0x384
#define PMCTRL_NOC_POWER_IDLE_0		0x388

#define NOC_USB_POWER_IDLEREQ		6

#define combophy_to_chip_usb3_31phy(ptr) container_of(ptr, struct chip_usb3_31phy, combophy)

struct chip_usb3_31phy {
	struct chip_usb_combophy combophy;
	void __iomem *usb_dp_ctrl_base;
	void __iomem *pmctrl_reg_base;
};

static struct chip_usb3_31phy *usb3_31phy;

struct chip_usb_combophy *usb3_get_combophy_phandle(void)
{
	if (usb3_31phy)
		return &usb3_31phy->combophy;

	return NULL;
}

static atomic_t is_phy_cr_start = ATOMIC_INIT(0);
static void usb31phy_cr_start(void __iomem *usb_dp_ctrl_base)
{
	volatile u32 reg;

	if (atomic_read(&is_phy_cr_start))
		return;

	/* close phy0_ref_dig_fr_clk */
	reg = readl(usb_dp_ctrl_base + USB_PHY_CFG0);
	reg &= ~PHY0_CR_PARA_CLK_EN;
	writel(reg, usb_dp_ctrl_base + USB_PHY_CFG0);

	/* enable phy_cr_para_sel */
	reg = readl(usb_dp_ctrl_base + USB_PHY_CFG0);
	reg |= PHY0_CR_PARA_SEL;
	writel(reg, usb_dp_ctrl_base + USB_PHY_CFG0);

	/* open phy0_ref_dig_fr_clk */
	reg = readl(usb_dp_ctrl_base + USB_PHY_CFG0);
	reg |= PHY0_CR_PARA_CLK_EN;
	writel(reg, usb_dp_ctrl_base + USB_PHY_CFG0);

	atomic_set(&is_phy_cr_start, 1);
}

static void usb31phy_cr_end(void __iomem *usb_dp_ctrl_base)
{
	volatile u32 reg;

	if (!atomic_read(&is_phy_cr_start))
		return;

	/* close phy0_ref_dig_fr_clk */
	reg = readl(usb_dp_ctrl_base + USB_PHY_CFG0);
	reg &= ~PHY0_CR_PARA_CLK_EN;
	writel(reg, usb_dp_ctrl_base + USB_PHY_CFG0);

	/* disable phy_cr_para_sel */
	reg = readl(usb_dp_ctrl_base + USB_PHY_CFG0);
	reg &= ~PHY0_CR_PARA_SEL;
	writel(reg, usb_dp_ctrl_base + USB_PHY_CFG0);

	atomic_set(&is_phy_cr_start, 0);
}

static u32 __usb31phy_cr_read(void __iomem *usb_dp_ctrl_base, u32 addr)
{
	/* config phy address */
	writel((addr & PHY_REG_ADDR_MASK) | PHY_REG_EN,
			usb_dp_ctrl_base + USB_PHY_REG_ADDR);

	return readl(usb_dp_ctrl_base + USB_PHY_REG_DATA);
}

static void __usb31phy_cr_write(void __iomem *usb_dp_ctrl_base, u32 addr, u32 value)
{
	/* config phy address */
	writel((addr & PHY_REG_ADDR_MASK) | PHY_REG_EN,
			usb_dp_ctrl_base + USB_PHY_REG_ADDR);

	writel(value, usb_dp_ctrl_base + USB_PHY_REG_DATA);
}

u32 usb31phy_cr_read(u32 addr)
{
	u32 reg;

	if (!usb3_31phy) {
		usb_err("usb3_31phy is NULL\n");
		return 0;
	}

	usb31phy_cr_start(usb3_31phy->usb_dp_ctrl_base);
	reg = __usb31phy_cr_read(usb3_31phy->usb_dp_ctrl_base, addr);

	return reg;
}

int usb31phy_cr_write(u32 addr, u32 value)
{
	if (!usb3_31phy) {
		usb_err("usb3_31phy is NULL\n");
		return -ENODEV;
	}

	usb31phy_cr_start(usb3_31phy->usb_dp_ctrl_base);
	__usb31phy_cr_write(usb3_31phy->usb_dp_ctrl_base, addr, value);

	return 0;
}

static void _reset_phy(struct chip_usb_combophy *combophy)
{
	struct chip_usb3_31phy *usb3_31phy = NULL;
	void __iomem *usb_dp_ctrl_base = NULL;

	if (!combophy) {
		usb_err("combophy is NULL\n");
		return;
	}

	usb3_31phy = combophy_to_chip_usb3_31phy(combophy);
	usb_dp_ctrl_base = usb3_31phy->usb_dp_ctrl_base;

	if (!chip_dwc3_is_fpga()) {
		/* disable combophy rx sigdet */
		__usb31phy_cr_write(usb_dp_ctrl_base, 0x4135, 0x2a);
		__usb31phy_cr_write(usb_dp_ctrl_base, 0x4235, 0x2a);
		udelay(10); /* delay 10us */
	}

	/* enable testpowerdown */
	combophy_regcfg_enter_testpowerdown();
}

static void _combophy_enter_noc_usb_poweridle(struct chip_usb3_31phy *usb3_31phy)
{
	void __iomem *pmctrl_reg_base = NULL;
	int retrys = 10;

	pmctrl_reg_base = usb3_31phy->pmctrl_reg_base;

	/* usb noc idle enter */
	writel(HM_EN(NOC_USB_POWER_IDLEREQ),
			pmctrl_reg_base + PMCTRL_NOC_POWER_IDLEREQ_0);

	/*
	 * wait for PMCTRL_NOC_POWER_IDLEACK_0 and PMCTRL_NOC_POWER_IDLE_0
	 * bit6 is 1
	 */
	while (retrys--) {
		if ((readl(pmctrl_reg_base + PMCTRL_NOC_POWER_IDLEACK_0) &
				BIT(NOC_USB_POWER_IDLEREQ)) &&
			(readl(pmctrl_reg_base + PMCTRL_NOC_POWER_IDLE_0) &
			 BIT(NOC_USB_POWER_IDLEREQ)))
			break;

		udelay(10); /* delay 10us */
	}

	if (retrys <= 0)
		usb_err("wait noc power idle state timeout\n");
}

static void _reset_misc_ctrl(struct chip_usb_combophy *combophy)
{
	struct chip_usb3_31phy *usb3_31phy = NULL;

	if (!combophy) {
		usb_err("combophy is NULL\n");
		return;
	}

	usb3_31phy = combophy_to_chip_usb3_31phy(combophy);

	/* phy cr end */
	usb31phy_cr_end(usb3_31phy->usb_dp_ctrl_base);

	_combophy_enter_noc_usb_poweridle(usb3_31phy);
}

static void _combophy_exit_noc_usb_poweridle(struct chip_usb3_31phy *usb3_31phy)
{
	void __iomem *pmctrl_reg_base = NULL;
	int retrys = 10;

	pmctrl_reg_base = usb3_31phy->pmctrl_reg_base;

	/* usb noc idle exit */
	writel(HM_DIS(NOC_USB_POWER_IDLEREQ),
			pmctrl_reg_base + PMCTRL_NOC_POWER_IDLEREQ_0);

	/*
	 * wait for PMCTRL_NOC_POWER_IDLEACK_0 and PMCTRL_NOC_POWER_IDLE_0
	 * bit6 is 0
	 */
	while (retrys--) {
		if (!(readl(pmctrl_reg_base + PMCTRL_NOC_POWER_IDLEACK_0) &
				BIT(NOC_USB_POWER_IDLEREQ)) &&
			!(readl(pmctrl_reg_base + PMCTRL_NOC_POWER_IDLE_0) &
			 BIT(NOC_USB_POWER_IDLEREQ)))
			break;

		udelay(10); /* delay 10us */
	}

	if (retrys <= 0)
		usb_err("wait noc power idle state timeout\n");
}

static void _unreset_misc_ctrl(struct chip_usb_combophy *combophy)
{
	struct chip_usb3_31phy *usb3_31phy = NULL;

	if (!combophy) {
		usb_err("combophy is NULL\n");
		return;
	}

	usb3_31phy = combophy_to_chip_usb3_31phy(combophy);
	/* do reset */
	combophy_regcfg_reset_misc();
	/* do unreset */
	combophy_regcfg_unreset_misc();

	udelay(100); /* delay 100us */

	_combophy_exit_noc_usb_poweridle(usb3_31phy);

	/* phy cr start */
	usb31phy_cr_start(usb3_31phy->usb_dp_ctrl_base);
}

#ifdef COMBOPHY_FW_UPDATE
#include "firmware.h"

static void _combophy_firmware_update_prepare(struct chip_usb_combophy *combophy)
{
	struct chip_usb3_31phy *usb3_31phy = NULL;
	void __iomem *usb_dp_ctrl_base = NULL;
	volatile u32 tmp;

	if (!combophy) {
		usb_err("combophy is NULL\n");
		return;
	}

	usb3_31phy = combophy_to_chip_usb3_31phy(combophy);
	usb_dp_ctrl_base = usb3_31phy->usb_dp_ctrl_base;

	tmp = readl(usb_dp_ctrl_base + USB_PHY_CFG6);
	tmp &= ~PHY0_SRAM_BYPASS;
	writel(tmp, usb_dp_ctrl_base + USB_PHY_CFG6);
}

static void _combophy_firmware_write(struct chip_usb_combophy *combophy)
{
	struct chip_usb3_31phy *usb3_31phy = NULL;
	void __iomem *usb_dp_ctrl_base = NULL;
	int i, cnt;
	int fw_size = ARRAY_SIZE(firmware);
	volatile u32 tmp;

	if (!combophy) {
		usb_err("combophy is NULL\n");
		return;
	}

	usb_info("+\n");
	usb3_31phy = combophy_to_chip_usb3_31phy(combophy);
	usb_dp_ctrl_base = usb3_31phy->usb_dp_ctrl_base;

	/* start write firmware */
	usb31phy_cr_start(usb_dp_ctrl_base);
	cnt = 10; /* cycle cnt 10 times */
	while (cnt--) {
		tmp = readl(usb_dp_ctrl_base + USB_PHY_STAT0);
		if (tmp & PHY0_SRAM_INIT_DONE)
			break;
		udelay(100); /* delay 100us */
	}
	if ((tmp & PHY0_SRAM_INIT_DONE) == 0)
		pr_err("[USB.DBG]%s:wait PHY0_SRAM_INIT_DONE timeout\n", __func__);

	i = 0;
	__usb31phy_cr_write(usb_dp_ctrl_base, 0xc000, firmware[i]);
	for (i = 0; i < fw_size; i++)
		__usb31phy_cr_write(usb_dp_ctrl_base, 0xc000 + i, firmware[i]);

	tmp = readl(usb_dp_ctrl_base + USB_PHY_CFG0);
	tmp |= PIPE_RX_CDR_LEGACY_EN;
	writel(tmp, usb_dp_ctrl_base + USB_PHY_CFG0);

	/* combophy sram external ld done */
	tmp = readl(usb_dp_ctrl_base + USB_PHY_CFG6);
	tmp |= PHY0_SRAM_EXT_LD_DONE;
	writel(tmp, usb_dp_ctrl_base + USB_PHY_CFG6);

	udelay(100); /* delay 100us */
	usb_info("-\n");
}
#endif

static int __init usb3_31phy_v2_module_init(void)
{
	struct device_node *np = NULL;
	int ret;

	usb3_31phy = kzalloc(sizeof(*usb3_31phy), GFP_KERNEL);
	if (!usb3_31phy)
		return -ENOMEM;

	/*
	 * map USB_DP_CTRL region
	 */
	np = of_find_compatible_node(NULL, NULL, "hisilicon,usb_dp_ctrl");
	if (!np) {
		usb_err("get usb dp ctrl node failed!\n");
		ret = -EINVAL;
		goto err_free;
	}

	usb3_31phy->usb_dp_ctrl_base = of_iomap(np, 0);
	if (!usb3_31phy->usb_dp_ctrl_base) {
		ret = -ENOMEM;
		goto err_free;
	}

	/*
	 * map PMCTRL region
	 */
	np = of_find_compatible_node(NULL, NULL, "hisilicon,pmctrl");
	if (!np) {
		usb_err("get pmctrl node failed!\n");
		ret = -EINVAL;
		goto err_unmap_dp_ctrl;
	}

	usb3_31phy->pmctrl_reg_base = of_iomap(np, 0);
	if (!usb3_31phy->pmctrl_reg_base) {
		usb_err("iomap pmctrl_reg_base failed!\n");
		ret = -ENOMEM;
		goto err_unmap_dp_ctrl;
	}

#ifdef COMBOPHY_FW_UPDATE
	usb3_31phy->combophy.firmware_update_prepare =
		_combophy_firmware_update_prepare;
	usb3_31phy->combophy.firmware_update = _combophy_firmware_write;
#endif
	usb3_31phy->combophy.reset_phy = _reset_phy;
	usb3_31phy->combophy.reset_misc_ctrl = _reset_misc_ctrl;
	usb3_31phy->combophy.unreset_misc_ctrl = _unreset_misc_ctrl;

	return 0;

err_unmap_dp_ctrl:
	iounmap(usb3_31phy->usb_dp_ctrl_base);
err_free:
	kfree(usb3_31phy);
	usb3_31phy = NULL;
	return ret;
}

static void __exit usb3_31phy_v2_module_exit(void)
{
	iounmap(usb3_31phy->usb_dp_ctrl_base);
	iounmap(usb3_31phy->pmctrl_reg_base);
	kfree(usb3_31phy);
	usb3_31phy = NULL;
}

module_init(usb3_31phy_v2_module_init);
module_exit(usb3_31phy_v2_module_exit);
