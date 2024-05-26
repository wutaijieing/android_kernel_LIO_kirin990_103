/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: Phy driver for platform apr.
 *
 * This software is distributed under the terms of the GNU General
 * Public License ("GPL") as published by the Free Software Foundation,
 * either version 2 of that License or (at your option) any later version.
 */

#include <linux/clk-provider.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <platform_include/basicplatform/linux/mfd/pmic_platform.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/platform_drivers/usb/chip_usb_interface.h>
#include <linux/platform_drivers/usb/dwc3_usb_interface.h>
#include <linux/platform_drivers/usb/chip_usb_helper.h>
#include <linux/jiffies.h>
#ifdef CONFIG_CONTEXTHUB_PD
#include <linux/platform_drivers/usb/tca.h>
#endif
#include <pmic_interface.h>
#ifdef CONFIG_FMEA_FAULT_INJECTION
#include <debug_fault_injection.h>
#endif
#include <huawei_platform/usb/hw_pd_dev.h>
#include "dwc3-chip.h"
#include "chip_usb3_misctrl.h"
#include "chip_usb3_31phy.h"
#include "usb_dp_ctrl_reg.h"
#include "combophy_regcfg.h"
#include "combophy.h"
#include "vregbypass.h"
#include "chip_usb_hw.h"
#include <chipset_common/hwusb/hw_usb.h>

struct usb30phy_tcxo_power {
	struct regulator *usb30phy_tcxo_power_085v;
	struct regulator *usb30phy_tcxo_power_18v;
	unsigned int usb30phy_tcxo_power_flag;
};

struct plat_chip_usb_phy {
	struct chip_usb_phy phy;

	void __iomem *pericfg_reg_base;
	void __iomem *sctrl_reg_base;
	void __iomem *pctrl_reg_base;
	void __iomem *hsdt_sctrl_reg_base;
	void __iomem *usb_dp_ctrl_base;

	struct clk *clk;
	struct clk *gt_aclk_usb3otg;
	struct clk *gt_hclk_usb3otg;
	struct clk *gt_clk_usb3_tcxo_en;
	struct clk *gt_clk_usb2phy_ref;

	struct regulator *usb20phy_power;
	unsigned int usb20phy_power_flag;
	struct usb30phy_tcxo_power usb30phy_power;
	u32 usb3_phy_term;
};

static struct plat_chip_usb_phy *_chip_usb_phy;

#define CLK_FREQ_19_2M			19200000

#define SCTRL_SCDEEPSLEEPED		0x08
#define USB_INIT_CLK_SEL		(1 << 20)
#define SC_CLK_USB3PHY_3MUX1_SEL	(1 << 25)

#define PERI_CRG_CLKDIV9		0xcc
#define PERI_CRG_CLKDIV29		0x70c
#define CLKDIV_MSK_START		 16
#define SC_GT_CLK_ABB_SYS		 1
#define SEL_ABB_BACKUP			(1 << 4)

#define SC_USB20PHY_MISC_CTRL1		0x604
#define SC_USB20PHY_MISC_CTRL4		0x610

/* PERI_CRG */
#define PERI_CRG_PERSTAT0		0x0c
#define PERI_CRG_PERSTAT4		0x4c
#define PERI_CRG_CLK_EN5                0x50
#define PERI_CRG_CLK_DIS5               0x54
#define PERI_CRG_PERSTAT5		0x5c

#define GT_CLK_ABB_BACKUP		(1 << 7)

#define GT_CLK_USB3OTG_REF		(1 << 10)
#define GT_ACLK_USB3OTG			(1 << 6)

#define USB_REFCLK_ISO_EN		(1 << 11)

/* PCTRL registers */
#define PCTRL_PERI_CTRL3		 0x10
#define USB_TCXO_EN			(1 << 1)
#define PERI_CTRL3_MSK_START		 16

#define PCTRL_PERI_CTRL24		 0x64
#define PERI_CRG_ISODIS			 0x148

/* usb3 phy term default value */
#define TERM_INVALID_PARAM	0xdeaddead

static int usb3_phy_33v_enable(struct plat_chip_usb_phy *usb_phy);
static int usb3_phy_33v_disable(struct plat_chip_usb_phy *usb_phy);
static int usb3_phy_tcxo_buffer_poweron(struct plat_chip_usb_phy *usb_phy);
static int usb3_phy_tcxo_buffer_poweroff(struct plat_chip_usb_phy *usb_phy);

int usb3_open_misc_ctrl_clk(void)
{
	int ret;

	if (!_chip_usb_phy) {
		usb_err("usb driver not setup!\n");
		return -EINVAL;
	}

	/* open hclk gate */
	ret = clk_prepare_enable(_chip_usb_phy->gt_hclk_usb3otg);
	if (ret) {
		usb_err("clk_enable gt_hclk_usb3otg failed\n");
		return ret;
	}

	if (__clk_is_enabled(_chip_usb_phy->gt_hclk_usb3otg) == false) {
		usb_err("gt_hclk_usb3otg  enable err\n");
		return -EFAULT;
	}

	/* open aclk gate */
	ret = clk_prepare_enable(_chip_usb_phy->gt_aclk_usb3otg);
	if (ret) {
		clk_disable_unprepare(_chip_usb_phy->gt_hclk_usb3otg);
		usb_err("clk_enable gt_aclk_usb3otg failed\n");
		return ret;
	}

	if (__clk_is_enabled(_chip_usb_phy->gt_aclk_usb3otg) == false) {
		clk_disable_unprepare(_chip_usb_phy->gt_hclk_usb3otg);
		usb_err("gt_aclk_usb3otg  enable err\n");
		return -EFAULT;
	}

	return 0;
}

void usb3_close_misc_ctrl_clk(void)
{
	if (!_chip_usb_phy) {
		usb_err("usb driver not setup!\n");
		return;
	}

	/* disable usb3otg hclk */
	clk_disable_unprepare(_chip_usb_phy->gt_aclk_usb3otg);
	clk_disable_unprepare(_chip_usb_phy->gt_hclk_usb3otg);
}

#ifdef CONFIG_CONTEXTHUB_PD
static int dwc3_combophy_sw_sysc(enum tcpc_mux_ctrl_type new_mode)
{
	return combophy_sw_sysc(TCPC_USB31_CONNECTED,
			(enum typec_plug_orien_e)chip_usb_otg_get_typec_orien(),
			!get_hifi_usb_retry_count());
}
#endif

static int config_usb_phy_controller(struct plat_chip_usb_phy *usb_phy,
		unsigned int support_dp)
{
	struct chip_usb_combophy *combophy = usb3_get_combophy_phandle();

	volatile uint32_t temp;
	static int flag = 1;

	/* Release USB20 PHY out of IDDQ mode */
	usb3_sc_misc_reg_clrbit(0u, 0x608ul);

	if (!support_dp || flag) {
		usb_dbg("set tca\n");
		flag = 0;
		/* Release USB31 PHY out of  TestPowerDown mode */
		usb3_misc_reg_clrbit(23u, 0x50ul);

		/* Tell the PHY power is stable */
		usb3_misc_reg_setvalue((1u << 2) | (1u << 5) | (1u << 7), 0x54ul);

		if (!chip_dwc3_is_fpga()) {
			/*
			 * config the TCA mux mode
			 * register:
			 *      -- 0x204 0x208: write immediately
			 *      -- 0x200 0x210 0x214 0x240: read-update-write
			 */
			usb3_misc_reg_writel(0xffffu, 0x208ul);
			usb3_misc_reg_writel(0x3u, 0x204ul);

			usb3_misc_reg_clrvalue(~SET_NBITS_MASK(0, 1), 0x200ul);
			usb3_misc_reg_setbit(4u, 0x210ul);

			temp = usb3_misc_reg_readl(0x214);
			temp &= ~(SET_NBITS_MASK(0, 1) | SET_NBITS_MASK(3, 4));
			temp |= (0x1 | (0x2 << 3));
			usb3_misc_reg_writel(temp, 0x214);

			usb3_misc_reg_setvalue(0x3u | (0x3u << 2), 0x240ul);

			usb3_misc_reg_setbit(7u, 0xb4ul);
		}

		/*
		 * firmware_update must after out of testpowerdown and
		 * phy power stable
		 */
		combophy_firmware_update(combophy);
	}

	/* Enable SSC */
	temp = readl(usb_phy->usb_dp_ctrl_base + USB_PHY_CFG0);
	temp |= PHY0_SS_MPLLA_SSC_EN;
	writel(temp, usb_phy->usb_dp_ctrl_base + USB_PHY_CFG0);

	return 0;
}

static uint32_t is_abb_clk_selected(const void __iomem *sctrl_base)
{
	volatile uint32_t scdeepsleeped;

	if (!sctrl_base) {
		usb_err("sctrl_base is NULL!\n");
		return 1;
	}

	scdeepsleeped = (uint32_t)readl(SCTRL_SCDEEPSLEEPED + sctrl_base);
	if ((scdeepsleeped & (USB_INIT_CLK_SEL)) == 0)
		return 1;

	return 0;
}

static int set_combophy_clk(struct plat_chip_usb_phy *usb_phy)
{
	void __iomem *pericfg_base = usb_phy->pericfg_reg_base;
	void __iomem *pctrl_base = usb_phy->pctrl_reg_base;
	void __iomem *sctrl_base = usb_phy->sctrl_reg_base;
#define USB_CLK_TYPE_ABB  0xabb
#define USB_CLK_TYPE_PAD  0x10ad
	volatile uint32_t temp;
	int ret;
	int clk_type = USB_CLK_TYPE_ABB;

	ret = usb3_phy_tcxo_buffer_poweron(usb_phy);
	if (ret)
		usb_err("usb3_phy_tcxo_buffer_poweron, maybe cs2 %d\n", ret);

	if (is_abb_clk_selected(sctrl_base)) {
		/* usb refclk iso */
		writel(USB_REFCLK_ISO_EN, PERI_CRG_ISODIS + pericfg_base);

		/* enable usb_tcxo_en */
		ret = clk_prepare_enable(usb_phy->gt_clk_usb3_tcxo_en);
		if (ret) {
			usb_err("clk_prepare_enable gt_clk_usb3_tcxo_en failed\n");
			return ret;
		}

		mdelay(10); /* mdelay time */

		/* select usbphy clk from abb */
		temp = (uint32_t)readl(pctrl_base + PCTRL_PERI_CTRL24);
		temp &= ~SC_CLK_USB3PHY_3MUX1_SEL;
		writel(temp, pctrl_base + PCTRL_PERI_CTRL24);
	} else {
		usb_dbg("pad clk\n");

		writel((SEL_ABB_BACKUP << CLKDIV_MSK_START),
				PERI_CRG_CLKDIV9 + pericfg_base);

		writel(SC_GT_CLK_ABB_SYS | (SC_GT_CLK_ABB_SYS << CLKDIV_MSK_START),
				PERI_CRG_CLKDIV29 + pericfg_base);

		writel(GT_CLK_ABB_BACKUP, PERI_CRG_CLK_EN5 + pericfg_base);

		temp = (uint32_t)readl(pctrl_base + PCTRL_PERI_CTRL24);
		temp |= SC_CLK_USB3PHY_3MUX1_SEL;
		writel(temp, pctrl_base + PCTRL_PERI_CTRL24);

		clk_type = USB_CLK_TYPE_PAD;
	}

	usb_dbg("usb clk type:%x\n", clk_type);
	return ret;
}

static int usb3_clk_is_open(void)
{
	return combophy_regcfg_is_misc_ctrl_clk_en() &&
		combophy_regcfg_is_controller_ref_clk_en() &&
		combophy_regcfg_is_controller_bus_clk_en();
}

int dwc3_set_combophy_clk(void)
{
	if (!_chip_usb_phy)
		return -ENODEV;

	return set_combophy_clk(_chip_usb_phy);
}

static int dwc3_phy_release(void)
{
	int ret;
	struct chip_usb_combophy *combophy = usb3_get_combophy_phandle();

	if (combophy_regcfg_is_misc_ctrl_unreset() &&
			usb3_clk_is_open())
		usb_err("usb3_misc_ctrl_is_unreset && usb3_clk_is_open\n");

	usb_dbg("release combophy\n");
	ret = dwc3_set_combophy_clk();
	if (ret) {
		usb_err("[CLK.ERR] combophy clk set error!\n");
		return ret;
	}

	/* prepare before phy unreset */
	combophy_firmware_update_prepare(combophy);

	/* unreset phy */
	usb3_misc_reg_setbit(1, 0xa0);
	chip_usb_unreset_phy_if_fpga();
	udelay(100); /* udelay time */

	return 0;
}

static void close_combophy_clk(struct plat_chip_usb_phy *usb_phy)
{
	void __iomem *sctrl_base = usb_phy->sctrl_reg_base;
	void __iomem *pericfg_base = usb_phy->pericfg_reg_base;
	int ret;

	if (is_abb_clk_selected(sctrl_base)) {
		/* disable usb_tcxo_en */
		clk_disable_unprepare(usb_phy->gt_clk_usb3_tcxo_en);
	} else {
		writel(GT_CLK_ABB_BACKUP, PERI_CRG_CLK_DIS5 + pericfg_base);
	}

	ret = usb3_phy_tcxo_buffer_poweroff(usb_phy);
	if (ret)
		usb_err("usb3_phy_tcxo_buffer_poweroff, maybe cs2 %d\n", ret);
}

void dwc3_close_combophy_clk(void)
{
	if (!_chip_usb_phy)
		return;

	close_combophy_clk(_chip_usb_phy);
}

static void dwc3_phy_reset(struct plat_chip_usb_phy *usb_phy)
{
	struct chip_usb_combophy *combophy = NULL;

	usb_dbg("reset combophy\n");
	if (combophy_regcfg_is_misc_ctrl_unreset() &&
			usb3_clk_is_open()) {
		/* reset phy */
		combophy = usb3_get_combophy_phandle();
		combophy_reset_phy(combophy);

		/* reset USB20 PHY */
		usb3_sc_misc_reg_clrbit(0, 0x618ul);

		/* set USB20 PHY IDDQ mode */
		usb3_sc_misc_reg_setbit(0u, 0x608ul);

		usb_dbg("reset phy\n");

		close_combophy_clk(usb_phy);
	}
}

int dwc3_open_controller_clk(const struct plat_chip_usb_phy *usb_phy)
{
	int ret;

	ret = clk_prepare_enable(usb_phy->gt_aclk_usb3otg);
	if (ret) {
		usb_err("clk_prepare_enable gt_aclk_usb3otg failed\n");
		return -EACCES;
	}

	ret = clk_prepare_enable(usb_phy->clk);
	if (ret) {
		clk_disable_unprepare(usb_phy->gt_aclk_usb3otg);
		usb_err("clk_prepare_enable clk failed\n");
		return -EACCES;
	}

	/*
	 * usb2 phy reference clock source select 100M or 19.2M(default)
	 */

	/*
	 * usb2phy select:
	 * 0:usb31 controller owns the phy
	 * 1:usb3 controller in asp owns the phy
	 */
	usb3_sc_misc_reg_clrbit(5, 0x618ul);

	/* usb2_refclksel: pll ref clock select */
	usb3_sc_misc_reg_setbit(4, 0x60c);

	return ret;
}

static void dwc3_close_controller_clk(const struct plat_chip_usb_phy *usb_phy)
{
	clk_disable_unprepare(usb_phy->gt_aclk_usb3otg);
	clk_disable_unprepare(usb_phy->clk);
}

/* dwc3_set_highspeed_only_step1 called before controller unreset */
static int dwc3_set_highspeed_only_step1(struct plat_chip_usb_phy *usb_phy)
{
	void __iomem *usb_dp_ctrl_base = usb_phy->usb_dp_ctrl_base;
	volatile unsigned int temp;

	/* disable usb3 SS port */
	temp = readl(usb_dp_ctrl_base + USB_CTRL_CFG0);
	temp |= HOST_U3_PORT_DISABLE;
	writel(temp, usb_dp_ctrl_base + USB_CTRL_CFG0);

	/* set ss port num 0 */
	temp = readl(usb_dp_ctrl_base + USB_CTRL_CFG0);
	temp &= ~HOST_NUM_U3_PORT_MASK;
	writel(temp, usb_dp_ctrl_base + USB_CTRL_CFG0);

	udelay(100); /* udelay time */

	return 0;
}

/* dwc3_set_highspeed_only_step2 called after controller unreset */
static void dwc3_set_highspeed_only_step2(void)
{
	dwc3_core_disable_pipe_clock();
}

#ifdef CONFIG_FMEA_FAULT_INJECTION
static int stub_dwc3_open_controller_clk()
{
	if (pmic_read_reg(FAULT_INJECT_REG) == KERNEL_FAULT_USB_CLK)
		return -ENODEV;

	return 0;
}

static int stub_usb3_phy_33v_enable()
{
	if (pmic_read_reg(FAULT_INJECT_REG) == KERNEL_FAULT_USB_POWER)
		return -ENODEV;

	return 0;
}
#endif

static int dwc3_release(struct plat_chip_usb_phy *usb_phy, unsigned int support_dp)
{
	int ret;
	int highspeed_only = 0;
	volatile unsigned int temp;

	usb_dbg("+\n");

	ret = dwc3_open_controller_clk(usb_phy);
	// stub clk enable
#ifdef CONFIG_FMEA_FAULT_INJECTION
	ret = stub_dwc3_open_controller_clk();
#endif
	if (ret) {
		usb_err("[CLK.ERR] open clk error!\n");
		return ret;
	}

	/* SCCLKDIV5(0x264)  bi[14]=0, bit[30]=1 */
	ret = clk_set_rate(usb_phy->gt_clk_usb2phy_ref, CLK_FREQ_19_2M);
	if (ret) {
		dwc3_close_controller_clk(usb_phy);
		usb_err("usb2phy_ref set rate failed, ret=%d\n", ret);
		return ret;
	}

	/* SCPEREN4(0x1b0)  bit[18]=1 */
	ret = clk_prepare_enable(usb_phy->gt_clk_usb2phy_ref);
	if (ret) {
		dwc3_close_controller_clk(usb_phy);
		usb_err("clk_prepare_enable gt_clk_usb2phy_ref failed\n");
		return ret;
	}

	udelay(100); /* udelay time */

	config_usb_phy_controller(usb_phy, support_dp);

	if (chip_dwc3_is_fpga()) {
		/* force utmi clk to 60M */
		usb3_sc_misc_reg_clrvalue(~SET_NBITS_MASK(8, 9), 0x600);
	} else {
		/* force utmi clk to 30M */
		usb3_sc_misc_reg_setvalue(SET_NBITS_MASK(8, 9), 0x600);
	}

	udelay(100); /* udelay time */

	/* release usb2.0 phy */
	usb3_sc_misc_reg_setbit(0, 0x618ul);

	if (!chip_dwc3_is_fpga())
		highspeed_only = combophy_is_highspeed_only();
	if (!chip_dwc3_is_fpga() && highspeed_only) {
		usb_dbg("[USB.DP] DP4 mode, set usb2.0 only, setp 1\n");
		ret = dwc3_set_highspeed_only_step1(usb_phy);
		if (ret) {
			dwc3_close_controller_clk(usb_phy);
			clk_disable_unprepare(usb_phy->gt_clk_usb2phy_ref);
			return ret;
		}
	}

	if (chip_dwc3_get_dt_host_maxspeed() < USB_SPEED_SUPER_PLUS) {
		usb_dbg("[USB.LINK] usb host super-speed only!\n");
		temp = readl(usb_phy->usb_dp_ctrl_base + USB_CTRL_CFG0);
		temp |= HOST_FORCE_GEN1_SPEED;
		writel(temp, usb_phy->usb_dp_ctrl_base + USB_CTRL_CFG0);
	}
	/* unreset controller */
	usb3_misc_reg_setbit(8, 0xa0);
	udelay(100); /* udelay time */

	if (!chip_dwc3_is_fpga() && highspeed_only) {
		usb_dbg("[USB.DP] DP4 mode, set usb2.0 only, step 2\n");
		dwc3_set_highspeed_only_step2();
	}

	/* set vbus valid */
	usb3_misc_reg_setvalue(SET_NBITS_MASK(6, 7), 0x0);
	usb3_sc_misc_reg_setvalue(SET_NBITS_MASK(5, 6), 0x600); /* set vbus valid */

	/* require to delay 10ms */
	mdelay(10);
	usb_dbg("-\n");

	return ret;
}

static void apr_notify_speed(unsigned int speed)
{
	usb_dbg("+device speed is %d\n", speed);

#ifdef CONFIG_TCPC_CLASS
	if ((speed != USB_CONNECT_HOST) && (speed != USB_CONNECT_DCP))
		hw_usb_set_usb_speed(speed);
#endif

	usb_dbg("-\n");
}

static void dwc3_reset(struct plat_chip_usb_phy *usb_phy)
{
	usb_dbg("+\n");
	if (combophy_regcfg_is_misc_ctrl_unreset() &&
			usb3_clk_is_open()) {
		/* reset controller */
		usb3_misc_reg_clrbit(8, 0xa0ul);
		usb_err("reset controller\n");

		/* reset usb2.0 phy */
		usb3_sc_misc_reg_clrbit(0, 0x618);

		/* set USB20 PHY IDDQ mode */
		usb3_sc_misc_reg_setbit(0u, 0x608ul);
	}

	clk_disable_unprepare(usb_phy->gt_clk_usb2phy_ref);
	clk_disable_unprepare(usb_phy->clk);
	clk_disable_unprepare(usb_phy->gt_aclk_usb3otg);

	usb_dbg("-\n");
}

static void set_usb2_eye_diagram_param(struct plat_chip_usb_phy *usb_phy,
		unsigned int eye_diagram_param)
{
	void __iomem *sctrl = usb_phy->hsdt_sctrl_reg_base;

	if (chip_dwc3_is_fpga())
		return;

	/* set high speed phy parameter */
	writel(eye_diagram_param, sctrl + SC_USB20PHY_MISC_CTRL1);
	usb_dbg("set hs phy param 0x%x\n",
			readl(sctrl + SC_USB20PHY_MISC_CTRL1));
}

#define TX_VBOOST_LVL_REG  0x21
#define TX_VBOOST_LVL_VALUE_START  4
#define TX_VBOOST_LVL_VALUE_END  6
#define TX_VBOOST_LVL_ENABLE SET_BIT_MASK(7)

static void set_vboost_for_usb3(uint32_t usb3_phy_tx_vboost_lvl)
{
	volatile u16 temp;

	if (usb3_phy_tx_vboost_lvl > 5) { /* limit value */
		usb_dbg("bad vboost %d use default\n", usb3_phy_tx_vboost_lvl);
		return;
	}

	temp = usb31phy_cr_read(TX_VBOOST_LVL_REG);
	temp = usb31phy_cr_read(TX_VBOOST_LVL_REG);
	temp &= ~SET_NBITS_MASK(TX_VBOOST_LVL_VALUE_START, TX_VBOOST_LVL_VALUE_END);
	temp |= (u16)((TX_VBOOST_LVL_ENABLE | (usb3_phy_tx_vboost_lvl <<
		TX_VBOOST_LVL_VALUE_START)) & SET_NBITS_MASK(0, 15)); /* tx vboost set */
	usb31phy_cr_write(TX_VBOOST_LVL_REG, temp);

	temp = usb31phy_cr_read(TX_VBOOST_LVL_REG);
	usb_dbg("[0x%x]usb31 cr param:%x\n", TX_VBOOST_LVL_REG, temp);
}

#define USB3_PHY_TERM_LANE0_REG	0x301a
#define USB3_PHY_TERM_LANE1_REG	0x311a
#define USB3_PHY_TERM_LANE2_REG	0x321a
#define USB3_PHY_TERM_LANE3_REG	0x331a

static void set_phy_term_for_usb3(u32 usb3_phy_term)
{
	usb31phy_cr_write(USB3_PHY_TERM_LANE0_REG, usb3_phy_term);
	usb31phy_cr_write(USB3_PHY_TERM_LANE1_REG, usb3_phy_term);
	usb31phy_cr_write(USB3_PHY_TERM_LANE2_REG, usb3_phy_term);
	usb31phy_cr_write(USB3_PHY_TERM_LANE3_REG, usb3_phy_term);
	usb_dbg("[0x%x]usb31 cr param:%x\n", USB3_PHY_TERM_LANE0_REG,
			usb31phy_cr_read(USB3_PHY_TERM_LANE0_REG));
	usb_dbg("[0x%x]usb31 cr param:%x\n", USB3_PHY_TERM_LANE1_REG,
			usb31phy_cr_read(USB3_PHY_TERM_LANE1_REG));
	usb_dbg("[0x%x]usb31 cr param:%x\n", USB3_PHY_TERM_LANE2_REG,
			usb31phy_cr_read(USB3_PHY_TERM_LANE2_REG));
	usb_dbg("[0x%x]usb31 cr param:%x\n", USB3_PHY_TERM_LANE3_REG,
			usb31phy_cr_read(USB3_PHY_TERM_LANE3_REG));
}

static int apr_set_phy_term(u32 usb3_phy_term)
{
	usb_dbg("+\n");
	if (!_chip_usb_phy)
		return -ENODEV;

	_chip_usb_phy->usb3_phy_term = usb3_phy_term;

	return 0;
}

static int apr_get_phy_term(u32 *usb3_phy_term)
{
	usb_dbg("+\n");
	if (!_chip_usb_phy)
		return -ENODEV;

	*usb3_phy_term = _chip_usb_phy->usb3_phy_term;

	return 0;
}

static int apr_usb31_core_enable_u3(struct plat_chip_usb_phy *usb_phy)
{
	int ret;
	int power_down_flag;

	usb_dbg("+\n");

	/*
	 * check if misc ctrl is release
	 * check if usb clk is open
	 */
	if (!combophy_regcfg_is_misc_ctrl_unreset() || !combophy_regcfg_is_misc_ctrl_clk_en()) {
		usb_err("misc ctrl or usb3 clk not ready.\n");
		return -ENOENT;
	}

	power_down_flag = get_chip_dwc3_power_flag();
	if (chip_dwc3_is_powerdown()) {
		/* open usb phy clk gate */
		ret = dwc3_open_controller_clk(usb_phy);
		if (ret) {
			usb_err("[CLK.ERR] open clk fail\n");
			return ret;
		}

		if (!__clk_is_enabled(usb_phy->clk) ||
				!__clk_is_enabled(usb_phy->gt_aclk_usb3otg)) {
			usb_err("usb3 ref clk and gt_aclk_usb3otg is disabled\n");
			return -ENOENT;
		}

		/* Release USB20 PHY out of IDDQ mode */
		usb3_sc_misc_reg_clrbit(0u, 0x608ul);

		/* release usb2.0 phy */
		usb3_sc_misc_reg_setbit(0, 0x618ul);

		udelay(100); /* udelay time */

		/* unreset controller */
		usb3_misc_reg_setbit(8, 0xa0);
		udelay(100); /* udelay time */
		set_chip_dwc3_power_flag(USB_POWER_ON);
	}

	ret = dwc3_core_enable_u3();
	if (get_chip_dwc3_power_flag() != power_down_flag)
		set_chip_dwc3_power_flag(power_down_flag);

	usb_dbg("-\n");
	return ret;
}

static int apr_usb31_core_disable_u3(struct plat_chip_usb_phy *usb_phy)
{
	usb_dbg("+\n");

	if (!combophy_regcfg_is_misc_ctrl_unreset() || !combophy_regcfg_is_misc_ctrl_clk_en()) {
		usb_err("usb core is busy! please disconnect usb first!\n");
		return -EBUSY;
	}
	if (chip_dwc3_is_powerdown()) {
		/* need reset controller */
		usb3_misc_reg_clrbit(8, 0xa0ul);

		/* reset usb2.0 phy */
		usb3_sc_misc_reg_clrbit(0, 0x618ul);

		/* set USB20 PHY IDDQ mode */
		usb3_sc_misc_reg_setbit(0u, 0x608ul);

		/* close misc clk gate */
		clk_disable_unprepare(usb_phy->clk);
		clk_disable_unprepare(usb_phy->gt_aclk_usb3otg);
	}

	usb_dbg("-\n");
	return 0;
}

int chip_usb_combophy_notify(enum phy_change_type type)
{
	int ret = 0;

	usb_dbg("+\n");
	if (!_chip_usb_phy) {
		usb_err("_chip_usb_phy is NULL, dwc3-usb have some problem!\n");
		return -ENOMEM;
	}

	/*
	 * check if usb controller is busy.
	 */
	if (!chip_dwc3_is_powerdown())
		usb_err("usb core is busy! maybe !\n");

	if (type == PHY_MODE_CHANGE_BEGIN) {
		ret = apr_usb31_core_enable_u3(_chip_usb_phy);
	} else if (type == PHY_MODE_CHANGE_END) {
		apr_usb31_core_disable_u3(_chip_usb_phy);
	} else {
		usb_err("Bad param!\n");
		ret = -EINVAL;
	}
	usb_dbg("-\n");
	return ret;
}

static int apr_usb3phy_shutdown(unsigned int support_dp)
{
	static int flag;

	usb_dbg("+\n");

	if (!_chip_usb_phy)
		return -ENODEV;

	if (chip_dwc3_is_powerdown()) {
		usb_dbg("already shutdown, just return!\n");
		return 0;
	}
	set_chip_dwc3_power_flag(USB_POWER_HOLD);

	if (!support_dp || !flag) {
		dwc3_phy_reset(_chip_usb_phy);
		flag = 1;
	}

	dwc3_reset(_chip_usb_phy);

	dwc3_misc_ctrl_put(MICS_CTRL_USB);

	usb_dbg(":DP_AUX_LDO_CTRL_USB disable\n");
	(void)usb3_phy_33v_disable(_chip_usb_phy);
	set_chip_dwc3_power_flag(USB_POWER_OFF);
	usb_dbg("-\n");

	return 0;
}

static int usb3_phy_33v_enable(struct plat_chip_usb_phy *usb_phy)
{
	int ret;
	struct regulator *ldo_supply = usb_phy->usb20phy_power;

	usb_dbg("+\n");
	if (ldo_supply == NULL) {
		usb_err("usb3.3v ldo enable!\n");
		return -ENODEV;
	}

	if (usb_phy->usb20phy_power_flag == 1)
		return 0;

	ret = regulator_enable(ldo_supply);
	if (ret) {
		usb_err("regulator enable failed %d!\n", ret);
		return -EPERM;
	}

	usb_phy->usb20phy_power_flag = 1;
	usb_dbg("regulator enable success!\n");
	return 0;
}

static int usb3_phy_33v_disable(struct plat_chip_usb_phy *usb_phy)
{
	int ret;
	struct regulator *ldo_supply = usb_phy->usb20phy_power;

	usb_dbg("+\n");
	if (ldo_supply == NULL) {
		usb_err("usb3.3v ldo disable!\n");
		return -ENODEV;
	}

	if (usb_phy->usb20phy_power_flag == 0) {
		WARN_ON(1);
		return 0;
	}

	ret = regulator_disable(ldo_supply);
	if (ret) {
		usb_err("regulator disable failed %d!\n", ret);
		return -EPERM;
	}

	usb_phy->usb20phy_power_flag = 0;
	usb_dbg("regulator disable success!\n");
	return 0;
}

static struct regulator *usb3_phy_ldo_33v_dts(struct device *dev)
{
	struct regulator *supply = NULL;
	int cur_uv;

	supply = regulator_get(dev, "usb_phy_ldo_33v");
	if (IS_ERR(supply)) {
		usb_err("get usb3 phy ldo 3.3v failed!\n");
		return NULL;
	}

	cur_uv = regulator_get_voltage(supply);

	usb_dbg("phy voltage:[%d]", cur_uv);
	return supply;
}

static struct regulator *usb3_phy_tcxo_ldo_085v_dts(struct device *dev)
{
	struct regulator *supply = NULL;

	supply = regulator_get(dev, "usb_phy_ldo_085v");
	if (IS_ERR(supply)) {
		usb_err("get usb3 phy tcxo buffer ldo 0.85v, maybe cs2\n");
		return NULL;
	}
	return supply;
}

static struct regulator *usb3_phy_tcxo_ldo_18v_dts(struct device *dev)
{
	struct regulator *supply = NULL;

	supply = regulator_get(dev, "usb_phy_ldo_18v");
	if (IS_ERR(supply)) {
		usb_err("get usb3 phy tcxo buffer ldo 1.8v, maybe cs2\n");
		return NULL;
	}
	return supply;
}

static int usb3_phy_tcxo_buffer_poweron(struct plat_chip_usb_phy *usb_phy)
{
	int ret;
	struct usb30phy_tcxo_power *tcxo_power = &usb_phy->usb30phy_power;

	usb_dbg("+\n");
	if (tcxo_power->usb30phy_tcxo_power_085v == NULL) {
		usb_err("usb3 phy tcxo 0.85v ldo NULL!\n");
		return -ENODEV;
	}

	if (tcxo_power->usb30phy_tcxo_power_18v == NULL) {
		usb_err("usb3 phy tcxo 18v ldo NULL!\n");
		return -ENODEV;
	}

	if (tcxo_power->usb30phy_tcxo_power_flag == 1)
		return 0;

	ret = regulator_enable(tcxo_power->usb30phy_tcxo_power_085v);
	if (ret) {
		usb_err("regulator 085v enable %d!\n", ret);
		return ret;
	}

	ret = regulator_enable(tcxo_power->usb30phy_tcxo_power_18v);
	if (ret) {
		usb_err("regulator 18v enable %d!\n", ret);
		if (regulator_disable(tcxo_power->usb30phy_tcxo_power_085v))
			usb_err("regulator 18v disable!\n");
		return ret;
	}
	tcxo_power->usb30phy_tcxo_power_flag = 1;

	usb_dbg("regulator enable success!\n");
	return ret;
}

static int usb3_phy_tcxo_buffer_poweroff(struct plat_chip_usb_phy *usb_phy)
{
	int ret;
	struct usb30phy_tcxo_power *tcxo_power = &usb_phy->usb30phy_power;

	usb_dbg("+\n");
	if (tcxo_power->usb30phy_tcxo_power_085v == NULL) {
		usb_err("usb3 phy tcxo 0.85v ldo NULL!\n");
		return -ENODEV;
	}

	if (tcxo_power->usb30phy_tcxo_power_18v == NULL) {
		usb_err("usb3 phy tcxo 18v ldo NULL!\n");
		return -ENODEV;
	}

	if (tcxo_power->usb30phy_tcxo_power_flag == 0)
		return 0;

	ret = regulator_disable(tcxo_power->usb30phy_tcxo_power_085v);
	if (ret) {
		usb_err("regulator 085v disable %d!\n", ret);
		return ret;
	}

	ret = regulator_disable(tcxo_power->usb30phy_tcxo_power_18v);
	if (ret) {
		usb_err("regulator 18v disable %d!\n", ret);
		if (regulator_enable(tcxo_power->usb30phy_tcxo_power_085v))
			usb_err("regulator 18v enable !\n");
		return ret;
	}
	tcxo_power->usb30phy_tcxo_power_flag = 0;

	usb_dbg("regulator disable success!\n");
	return ret;
}

static void usb20phy_init_vregbypass(const void __iomem *sctrl_base)
{
	volatile uint32_t scdeepsleeped;
	bool need_init = false;

	scdeepsleeped = (uint32_t)readl(SCTRL_SCDEEPSLEEPED + sctrl_base);
	if ((scdeepsleeped & (1 << 7)) != 0)
		need_init = false;
	else
		need_init = true;

	pr_info("%s: usb20phy vregbypass %s init\n",
		__func__, need_init ? "need" : "not need");

	if (need_init)
		usb20phy_vregbypass_init();
}

static void config_usbphy_power(const void __iomem *sctrl_base)
{
	usb3_sc_misc_reg_clrbit(12, 0x60c);
	usb_dbg("clear usb vregbypass\n");
}

static int apr_usb3phy_init(unsigned int support_dp,
		unsigned int eye_diagram_param,
		unsigned int usb3_phy_tx_vboost_lvl)
{
	int ret;
	static int flag;
	void __iomem *sctrl_base;

	usb_dbg("+\n");

	if (!_chip_usb_phy)
		return -ENODEV;

	sctrl_base = _chip_usb_phy->sctrl_reg_base;

	if (!chip_dwc3_is_powerdown()) {
		usb_dbg("already release, just return!\n");
		return 0;
	}

	config_usbphy_power(sctrl_base);

	ret = usb3_phy_33v_enable(_chip_usb_phy);
#ifdef CONFIG_FMEA_FAULT_INJECTION
	ret = stub_usb3_phy_33v_enable();
#endif
	if (ret) {
		usb_err("usb3_phy_33v_enable failed!\n");
		goto out;
	}

	ret = dwc3_misc_ctrl_get(MICS_CTRL_USB);
	if (ret) {
		usb_err("usb driver not ready!\n");
		goto out;
	}

	if (!support_dp || !flag) {
		ret = dwc3_phy_release();
		if (ret) {
			usb_err("phy release err!\n");
			goto out_misc_put;
		}
	}

	if (support_dp && !combophy_regcfg_is_misc_ctrl_unreset()) {
		usb_err("[USBDP.DBG] goto here, need check who call.\n");
		goto out_phy_reset;
	}

	ret = dwc3_release(_chip_usb_phy, support_dp);
	if (ret) {
		usb_err("[RELEASE.ERR] release error, need check clk!\n");
		goto out_phy_reset;
	}

	set_usb2_eye_diagram_param(_chip_usb_phy, eye_diagram_param);
	set_vboost_for_usb3(usb3_phy_tx_vboost_lvl);
	if (_chip_usb_phy->usb3_phy_term != TERM_INVALID_PARAM)
		set_phy_term_for_usb3(_chip_usb_phy->usb3_phy_term);

	set_chip_dwc3_power_flag(USB_POWER_ON);
	flag = 1;

	usb_dbg("-\n");
	return ret;

out_phy_reset:
	if (!support_dp || !flag)
		dwc3_phy_reset(_chip_usb_phy);
out_misc_put:
	dwc3_misc_ctrl_put(MICS_CTRL_USB);
out:
	return ret;
}

static int apr_get_clk_resource(struct device *dev, struct plat_chip_usb_phy *usb_phy)
{
	/* get abb clk handler */
	usb_phy->clk = devm_clk_get(dev, "clk_usb3phy_ref");
	if (IS_ERR_OR_NULL(usb_phy->clk)) {
		usb_err("get usb3phy ref clk failed\n");
		return -EINVAL;
	}

	/* get aclk handler */
	usb_phy->gt_aclk_usb3otg = devm_clk_get(dev, "aclk_usb3otg");
	if (IS_ERR_OR_NULL(usb_phy->gt_aclk_usb3otg)) {
		usb_err("get aclk_usb3otg failed\n");
		return -EINVAL;
	}

	/* get hclk handler */
	usb_phy->gt_hclk_usb3otg = devm_clk_get(dev, "hclk_usb3otg");
	if (IS_ERR_OR_NULL(usb_phy->gt_hclk_usb3otg)) {
		usb_err("get hclk_usb3otg failed\n");
		return -EINVAL;
	}

	/* get tcxo clk handler */
	usb_phy->gt_clk_usb3_tcxo_en = devm_clk_get(dev, "clk_usb3_tcxo_en");
	if (IS_ERR_OR_NULL(usb_phy->gt_clk_usb3_tcxo_en)) {
		usb_err("get clk_usb3_tcxo_en failed\n");
		return -EINVAL;
	}

	/* get usb2phy ref clk handler */
	usb_phy->gt_clk_usb2phy_ref = devm_clk_get(dev, "clk_usb2phy_ref");
	if (IS_ERR_OR_NULL(usb_phy->gt_clk_usb2phy_ref)) {
		usb_err("get clk_usb2phy_ref failed\n");
		return -EINVAL;
	}

	return 0;
}

static int apr_get_dts_resource(struct device *dev, struct plat_chip_usb_phy *usb_phy)
{
	void __iomem *regs;

	if (apr_get_clk_resource(dev, usb_phy))
		return -EINVAL;

	/*
	 * map misc ctrl region
	 */
	usb_phy->phy.otg_bc_reg_base = of_iomap(dev->of_node, 0);
	if (!usb_phy->phy.otg_bc_reg_base) {
		dev_err(dev, "ioremap res 0 failed\n");
		return -ENOMEM;
	}

	/*
	 * map PERI CRG region
	 */
	regs = of_devm_ioremap(dev, "hisilicon,crgctrl");
	if (IS_ERR(regs)) {
		usb_err("iomap peri cfg failed!\n");
		return PTR_ERR(regs);
	}
	usb_phy->pericfg_reg_base = regs;

	/*
	 * map SCTRL region
	 */
	regs = of_devm_ioremap(dev, "hisilicon,sysctrl");
	if (IS_ERR(regs)) {
		usb_err("iomap sctrl_reg_base failed!\n");
		return PTR_ERR(regs);
	}
	usb_phy->sctrl_reg_base = regs;

	/*
	 * map PCTRL region
	 */
	regs = of_devm_ioremap(dev, "hisilicon,pctrl");
	if (IS_ERR(regs)) {
		usb_err("iomap pctrl failed!\n");
		return PTR_ERR(regs);
	}
	usb_phy->pctrl_reg_base = regs;

	/*
	 * map HSDT SYSCTRL region
	 */
	regs = of_devm_ioremap(dev, "hisilicon,hsdt_sysctrl");
	if (IS_ERR(regs)) {
		usb_err("iomap hsdt_sctrl_reg_base failed!\n");
		return PTR_ERR(regs);
	}
	usb_phy->hsdt_sctrl_reg_base = regs;

	/*
	 * map USB_DP_CTRL region
	 */
	regs = of_devm_ioremap(dev, "hisilicon,usb_dp_ctrl");
	if (IS_ERR(regs)) {
		usb_err("iomap usb dp ctrl node failed!\n");
		return PTR_ERR(regs);
	}
	usb_phy->usb_dp_ctrl_base = regs;

	if (of_property_read_u32(dev->of_node, "usb3_phy_term",
			&(usb_phy->usb3_phy_term))) {
		usb_dbg("get usb3_phy_term form dt failed, set invalid value\n");
		usb_phy->usb3_phy_term = TERM_INVALID_PARAM;
	}
	usb_dbg("usb3_phy_term: %x\n", usb_phy->usb3_phy_term);

	usb_phy->usb30phy_power.usb30phy_tcxo_power_085v = usb3_phy_tcxo_ldo_085v_dts(dev);
	usb_phy->usb30phy_power.usb30phy_tcxo_power_18v = usb3_phy_tcxo_ldo_18v_dts(dev);
	usb_phy->usb20phy_power = usb3_phy_ldo_33v_dts(dev);
	usb_phy->usb20phy_power_flag = 0;

	init_misc_ctrl_addr(usb_phy->phy.otg_bc_reg_base);
	init_sc_misc_ctrl_addr(usb_phy->hsdt_sctrl_reg_base);

	return 0;
}

static void __iomem *apr_get_bc_ctrl_reg(void)
{
	if (!_chip_usb_phy)
		return NULL;

	return _chip_usb_phy->hsdt_sctrl_reg_base + SC_USB20PHY_MISC_CTRL4;
}

static void apr_disable_usb3(void)
{
	/* reset controller */
	usb3_misc_reg_clrbit(8, 0xa0);
	udelay(100); /* udelay time */

	/* disable usb3 SS port */
	dwc3_set_highspeed_only_step1(_chip_usb_phy);

	/* unreset controller */
	usb3_misc_reg_setbit(8, 0xa0);

	udelay(100); /* udelay time */
	dwc3_set_highspeed_only_step2();
}

static int apr_shared_phy_init(unsigned int support_dp, unsigned int eye_diagram_param,
		unsigned int combophy_flag)
{
	int ret;
	uint32_t temp;

	usb_dbg("+\n");

	if (!_chip_usb_phy)
		return -ENODEV;

#ifdef CONFIG_CONTEXTHUB_PD
	if (support_dp && combophy_flag)
		combophy_poweroff();
#endif

	usb_dbg(":DP_AUX_LDO_CTRL_HIFIUSB enable\n");
	ret = usb3_phy_33v_enable(_chip_usb_phy);
	if (ret && ret != -ENODEV) {
		usb_err("usb3_phy_33v_enable failed!\n");
		return ret;
	}

	udelay(100); /* udelay time */

	/* enable 2.0 phy refclk */
	ret = clk_prepare_enable(_chip_usb_phy->gt_clk_usb2phy_ref);
	if (ret) {
		usb_err("clk_prepare_enable gt_clk_usb2phy_ref failed\n");
		goto err_misc_clk_usb2phy;
	}

	/* hifi use 16bit-30M utmi */
	usb3_sc_misc_reg_setvalue(SET_NBITS_MASK(8, 9), 0x600);

	/* Release USB20 PHY out of IDDQ mode */
	usb3_sc_misc_reg_clrbit(0u, 0x608ul);

	/* Usb20phy port control signal usb2_refclksel select */
	temp = usb3_sc_misc_reg_readl(0x60cul);
	temp &= ~(0x3 << 3);
	temp |= (0x2 << 3);
	usb3_sc_misc_reg_writel(temp, 0x60cul);

	/* select usb20phy for audio */
	usb3_sc_misc_reg_setbit(5, 0x618ul);
	chip_usb_switch_sharedphy_if_fpga(1);

	/* select usb 20phy drv vbus for audio */
	temp = usb3_sc_misc_reg_readl(0x618ul);
	temp &= ~(0x3 << 6);
	temp |= (0x2 << 6);
	usb3_sc_misc_reg_writel(temp, 0x618ul);

	/* release 2.0 phy */
	usb3_sc_misc_reg_setbit(0, 0x618ul);
	chip_usb_unreset_phy_if_fpga();
	udelay(100); /* udelay time */

	set_usb2_eye_diagram_param(_chip_usb_phy, eye_diagram_param);

	usb_dbg("-\n");
	return 0;

err_misc_clk_usb2phy:
#ifdef CONFIG_CONTEXTHUB_PD
	if (support_dp && combophy_flag) {
		usb_dbg("combophy_sw_sysc +\n");
		ret = dwc3_combophy_sw_sysc(TCPC_USB31_CONNECTED);
		if (ret)
			usb_err("combophy_sw_sysc failed %d\n", ret);
		usb_dbg("combophy_sw_sysc -\n");
	}
#endif
	usb_dbg(":DP_AUX_LDO_CTRL_HIFIUSB disable\n");
	(void)usb3_phy_33v_disable(_chip_usb_phy);

	return ret;
}

static int apr_shared_phy_shutdown(unsigned int support_dp,
		unsigned int combophy_flag, unsigned int keep_power)
{
	uint32_t temp;
	int ret = 0;

	usb_dbg("+\n");

	if (!_chip_usb_phy)
		return -ENODEV;

	usb_dbg("combophy_flag %u, keep_power %u\n", combophy_flag, keep_power);

	/* reset 2.0 phy */
	usb3_sc_misc_reg_clrbit(0, 0x618ul);

	/* switch drv vbus */
	temp = usb3_sc_misc_reg_readl(0x618ul);
	temp &= ~(0x3 << 6);
	usb3_sc_misc_reg_writel(temp, 0x618ul);

	/* switch phy */
	usb3_sc_misc_reg_clrbit(5, 0x618ul);
	chip_usb_switch_sharedphy_if_fpga(0);

	/* enable siddq, clock source select */
	usb3_sc_misc_reg_setbit(0u, 0x608ul);

	/* disable 2.0 phy refclk */
	clk_disable_unprepare(_chip_usb_phy->gt_clk_usb2phy_ref);

#ifdef CONFIG_CONTEXTHUB_PD
	if (support_dp && combophy_flag) {
		usb_dbg("combophy_sw_sysc +\n");
		ret = dwc3_combophy_sw_sysc(TCPC_USB31_CONNECTED);
		if (ret)
			usb_err("combophy_sw_sysc failed %d\n", ret);
		usb_dbg("combophy_sw_sysc -\n");
	}
#endif

	if (!keep_power) {
		usb_dbg(":DP_AUX_LDO_CTRL_HIFIUSB disable\n");
		ret = usb3_phy_33v_disable(_chip_usb_phy);
		if (ret && ret != -ENODEV)
			usb_err("usb3_phy_33v_disable failed!\n");
	}

	usb_dbg("-\n");
	return ret;
}

static int dwc3_apr_probe(struct platform_device *pdev)
{
	int ret;

	_chip_usb_phy = devm_kzalloc(&pdev->dev, sizeof(*_chip_usb_phy),
			GFP_KERNEL);
	if (!_chip_usb_phy)
		return -ENOMEM;

	ret = apr_get_dts_resource(&pdev->dev, _chip_usb_phy);
	if (ret) {
		usb_err("get_dts_resource failed\n");
		goto err_out;
	}

	usb20phy_init_vregbypass(_chip_usb_phy->sctrl_reg_base);
	_chip_usb_phy->phy.init = apr_usb3phy_init;
	_chip_usb_phy->phy.shutdown = apr_usb3phy_shutdown;
	_chip_usb_phy->phy.shared_phy_init = apr_shared_phy_init;
	_chip_usb_phy->phy.shared_phy_shutdown = apr_shared_phy_shutdown;
	_chip_usb_phy->phy.notify_speed = apr_notify_speed;
	_chip_usb_phy->phy.get_bc_ctrl_reg = apr_get_bc_ctrl_reg;
	_chip_usb_phy->phy.disable_usb3 = apr_disable_usb3;
	_chip_usb_phy->phy.get_term = apr_get_phy_term;
	_chip_usb_phy->phy.set_term = apr_set_phy_term;

	ret = chip_usb_dwc3_register_phy(&_chip_usb_phy->phy);
	if (ret) {
		usb_err("chip_usb_dwc3_register_phy failed\n");
		goto err_out;
	}

	return 0;
err_out:
	_chip_usb_phy = NULL;
	return ret;
}

static int dwc3_apr_remove(struct platform_device *pdev)
{
	int ret;

	ret = chip_usb_dwc3_unregister_phy(&_chip_usb_phy->phy);
	_chip_usb_phy = NULL;

	return ret;
}

#ifdef CONFIG_OF
static const struct of_device_id dwc3_apr_match[] = {
	{ .compatible = "hisilicon,apr-dwc3" },
	{},
};
MODULE_DEVICE_TABLE(of, dwc3_apr_match);
#else
#define dwc3_apr_match NULL
#endif

static struct platform_driver dwc3_apr_driver = {
	.probe		= dwc3_apr_probe,
	.remove		= dwc3_apr_remove,
	.driver		= {
		.name	= "usb3-apr",
		.of_match_table = of_match_ptr(dwc3_apr_match),
	},
};
module_platform_driver(dwc3_apr_driver);
MODULE_LICENSE("GPL");
