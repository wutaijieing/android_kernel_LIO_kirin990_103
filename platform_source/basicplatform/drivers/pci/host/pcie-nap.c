/*
 * pcie-nap.c
 *
 * PCIe Nap turn-on/off functions
 *
 * Copyright (c) 2021-2021 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "pcie-kport.h"
#include "pcie-kport-common.h"
#include "pcie-kport-idle.h"
#include <platform_include/see/bl31_smc.h>
#include <mm_svm.h>

static atomic_t tcu_urst_cnt = ATOMIC_INIT(0);
static atomic_t tcu_pwr_cnt = ATOMIC_INIT(0);

/* Base Address */
#define HSDT1_IOC_BASE            0xEB041000

/* PCIE reset */
#define RST_PCIE1_BIT             0x16A21
#define RST_PCIE0_BIT             0x29442

/* PCIE CTRL Register bit definition */
#define PCIE_PERST_CONFIG_MASK    0x4

/* PMC register offset */
#define NOC_POWER_IDLEREQ         0x380
#define NOC_POWER_IDLEACK         0x384
#define NOC_POWER_IDLE            0x388

/* HSDT1 */
#define HSDT1_CRG_PEREN1          0x010
#define HSDT1_CRG_PERDIS1         0x014
#define HSDT1_CRG_PERRSTEN2       0x070
#define HSDT1_CRG_PERRSTDIS2      0x074
#define HSDT1_CRG_PEREN2          0x020
#define HSDT1_CRG_PERDIS2         0x024
#define HSDT1_CRG_PCIECTRL0       0x300
#define HSDT1_CRG_PCIECTRL1       0x304
#define PCIE_CTRL_STAT4           0x410
#define HSDT1_CRGPCIEPLL_STAT     0x204
#define HSDT1_CRGFNPLL_CFG0       0x208
#define HSDT1_CRGFNPLL_CFG6       0x228
#define HSDT1_CRGFNPLL_CFG7       0x22c
#define HSDTCRG_PCIECTRL0         0x300
#define HSDTCRG_PCIECTRL1         0x304

#define IP_RST_PCIE1CTRL_POR      (0x1 << 0)
#define IP_RST_PCIE0CTRL_POR      (0x1 << 1)
#define IP_PRST_PCIE1_SYSCTRL     (0x1 << 9)
#define IP_PRST_PCIE0_SYSCTRL     (0x1 << 10)
#define IP_RST_PCIE1_BC           (0x1 << 11)
#define IP_RST_PCIE0_BC           (0x1 << 12)
#define IP_PRST_PCIE1_PHY         (0x1 << 14)
#define IP_PRST_PCIE0_PHY         (0x1 << 15)
#define IP_RST_PCIE0_TBU          (0x1 << 6)
#define IP_RST_PCIE_TCU           (0x1 << 5)
#define IP_RST_PCIE1_TBU          (0x1 << 13)
#define IP_RST_NOC_PCIE1_ASYNCBRG (0x1 << 16)
#define IP_RST_NOC_PCIE0_ASYNCBRG (0x1 << 17)

#define GT_PCLK_PCIE0_PHY          (0x1 << 21)
#define GT_PCLK_PCIE1_PHY          (0x1 << 20)
#define GT_ACLK_NOC_PCIE1_ASYNCBRG (0x1 << 18)
#define GT_ACLK_NOC_PCIE0_ASYNCBRG (0x1 << 19)
#define GT_PCLK_PCIE0_SYSCTRL      (0x1 << 15)
#define GT_PCLK_PCIE1_SYSCTRL      (0x1 << 14)
#define PCIE1_CLK_HP_GATE          (0x1 << 0)
#define PCIE1_CLK_DEBUNCE_GATE     (0x1 << 1)
#define PCIE0_CLK_HP_GATE          (0x1 << 2)
#define PCIE0_CLK_DEBUNCE_GATE     (0x1 << 3)
#define PCIE1_CLK_TBU_GATE         (0x1 << 26)
#define PCIE_CLK_TCU_GATE          (0x1 << 10)
#define PCIE0_CLK_TBU_GATE         (0x1 << 11)

#define PCIEPHY_REF_HW_BYPASS      (0x1 << 1)
#define PCIE1_RST_TCU              (0x1 << 5)
#define PCIE1_RST_TBU              (0x1 << 13)
#define PCIE_PLL_LOCK_BIT          (0x1 << 4)
#define PCIE0_CLK_PHYREF_GATE      (0x1 << 8)
#define PCIE1_CLK_PHYREF_GATE      (0x1 << 7)
#define PCIE1_CLK_IO_GATE          (0x1 << 23)
#define PCIE0_CLK_IO_GATE          (0x1 << 25)
#define PCIEIO_OE_EN_SOFT          (0x1 << 6)
#define PCIEIO_OE_POLAR            (0x1 << 9)
#define PCIEIO_OE_EN_HARD_BYPASS   (0x1 << 11)
#define PCIEIO_IE_EN_HARD_BYPASS   (0x1 << 27)
#define PCIEIO_IE_EN_SOFT          (0x1 << 28)
#define PCIEIO_IE_POLAR            (0x1 << 29)
#define PCIE_PAD_PERST_IN          (0x1 << 27)

/* PMC register bit definition */
#define NOC_PCIE0_POWER            (0x1 << 10)
#define NOC_PCIE1_POWER            (0x1 << 11)
#define NOC_PCIE0_POWER_MASK       (0x1 << 26)
#define NOC_PCIE1_POWER_MASK       (0x1 << 27)
#define NOC_HSDT1_POWER_IDLEREQ    (0x1 << 4)
#define NOC_HSDT1_POWER_IDLEREQ_MASK (0x1 << 20)

/* phy */
#define PCIEPHY_CTRL1_BIT0         (0x1 << 0)
#define PCIEPHY_CTRL1_BIT1         (0x1 << 1)
#define PCS_TOP_TXIDLE_EXIT_SEL     0x0500
#define PCS_TOP_TXIDLE_ENTRY_SEL    0x0504
#define PCS_TOP_RXGSATMR            0x0A60
#define PCS_TOP_RXEQATMR            0x0A64

/* SMMU */
#define PCIE0_TBU_BASE              0xEA480000
#define PCIE1_TBU_BASE              0xEA980000
#define PCIE_TBU_SIZE               0x20000
#define PCIE_TBU_CR                 0x0000
#define PCIE_TBU_CRACK              0x0004
#define PCIE_TBU_EN_REQ_BIT        (0x1 << 0)
#define PCIE_TBU_EN_ACK_BIT        (0x1 << 0)
#define PCIE_TBU_CONNCT_STATUS_BIT (0x1 << 1)
#define PCIE_TBU_TOK_TRANS_MSK     (0xFF << 8)
#define PCIE_TBU_TOK_TRANS          0x8
#define PCIE_TBU_DEFALUT_TOK_TRANS  0xF
#define PCIE0_SMMUID                0x3
#define PCIE1_SMMUID                0x4
#define TBU_CONFIG_TIMEOUT          100

#define HSDTSCTRL_AWMMUS            0x604
#define HSDTSCTRL_ARMMUS            0x608
#define PCIE_AWRMMUSSIDV           (0x1 << 24)
#define PCIE_AWRMMUSID             (0xE << 8)
#define PCIE1_AWRMMUSID            (0xF << 8)

#define NOC_TIMEOUT_VAL             1000
#define EP_CHECK_PERST              10000

#define GEN3_RELATED_OFF            0x890
#define GEN3_ZRXDC_NONCOMPL        (0x1 << 0)

static atomic_t pll_init_cnt = ATOMIC_INIT(0);

static u32 hsdt_crg_reg_read(struct pcie_kport *pcie, u32 reg)
{
	return readl(pcie->crg_base + reg);
}

static void hsdt_crg_reg_write(struct pcie_kport *pcie, u32 val, u32 reg)
{
	writel(val, pcie->crg_base + reg);
}

static void pcie_rst(struct pcie_kport *pcie)
{
	u32 val = 0;

	if (pcie->rc_id == PCIE_RC1)
		val |= RST_PCIE1_BIT;
	else
		val |= RST_PCIE0_BIT;

	hsdt_crg_reg_write(pcie, val, HSDT1_CRG_PERRSTEN2);
}

static int pcie_internal_clk(struct pcie_kport *pcie, u32 enable)
{
	int ret = 0;

	if (!enable) {
		clk_disable_unprepare(pcie->apb_sys_clk);
		clk_disable_unprepare(pcie->apb_phy_clk);
		return ret;
	}

	/* pclk for phy */
	ret = clk_prepare_enable(pcie->apb_phy_clk);
	if (ret) {
		PCIE_PR_E("Failed to enable apb_phy_clk");
		return ret;
	}

	/* pclk for ctrl */
	ret = clk_prepare_enable(pcie->apb_sys_clk);
	if (ret) {
		clk_disable_unprepare(pcie->apb_phy_clk);
		PCIE_PR_E("Failed to enable apb_sys_clk");
		return ret;
	}
	return ret;
}

static int pcie_external_clk(struct pcie_kport *pcie, u32 enable)
{
	int ret = 0;
	u32 mask, reg;

	if (!enable) {
		clk_disable_unprepare(pcie->pcie_aux_clk);
		clk_disable_unprepare(pcie->pcie_tbu_clk);
		clk_disable_unprepare(pcie->pcie_tcu_clk);
		clk_disable_unprepare(pcie->pcie_aclk);
		return ret;
	}

	/* ctrl_aux_clk */
	ret = clk_prepare_enable(pcie->pcie_aux_clk);
	if (ret) {
		PCIE_PR_E("Failed to enable aux_clk");
		return ret;
	}

	/* Enable pcie axi clk */
	ret = clk_prepare_enable(pcie->pcie_aclk);
	if (ret) {
		clk_disable_unprepare(pcie->pcie_aux_clk);
		PCIE_PR_E("Failed to enable axi_aclk");
		return ret;
	}

	/* Enable pcie tcu clk */
	ret = clk_prepare_enable(pcie->pcie_tcu_clk);
	if (ret) {
		clk_disable_unprepare(pcie->pcie_aux_clk);
		clk_disable_unprepare(pcie->pcie_aclk);
		PCIE_PR_E("Failed to enable tcu_clk");
		return ret;
	}

	/* Enable pcie tbu clk */
	ret = clk_prepare_enable(pcie->pcie_tbu_clk);
	if (ret) {
		clk_disable_unprepare(pcie->pcie_tcu_clk);
		clk_disable_unprepare(pcie->pcie_aux_clk);
		clk_disable_unprepare(pcie->pcie_aclk);
		PCIE_PR_E("Failed to enable tbu_clk");
		return ret;
	}
	return ret;
}

static int pcie_clk_config(struct pcie_kport *pcie, enum pcie_clk_type clk_type, u32 enable)
{
	int ret = 0;

	switch (clk_type) {
	case PCIE_INTERNAL_CLK:
		ret = pcie_internal_clk(pcie, enable);
		break;
	case PCIE_EXTERNAL_CLK:
		ret = pcie_external_clk(pcie, enable);
		break;
	default:
		PCIE_PR_E("Invalid input parameters");
		ret = -EINVAL;
	}

	return ret;
}

static void pcie_phy_signal_unrst(struct pcie_kport *pcie)
{
	u32 val;

	val = pcie_apb_phy_readl(pcie, SOC_PCIEPHY_CTRL1_ADDR);
	val |= PCIEPHY_CTRL1_BIT0;
	val |= PCIEPHY_CTRL1_BIT1;
	pcie_apb_phy_writel(pcie, val, SOC_PCIEPHY_CTRL1_ADDR);
}

#define FNPLL_CFG(cfg_base, num)	((cfg_base) + (num) * 0x4)

static void fnpll_param(struct pcie_kport *pcie)
{
	int timeout = 0;
	u32 val, pll_cnt;
	u32 pll_cfg_base = HSDT1_CRGFNPLL_CFG0;
	const u32 pll_cfg_val[] = {
		0x8C000004, 0x00B40000, 0x20111FA8,
		0x2404FF20, 0x0004013F, 0x00004046
	};

	for (pll_cnt = 0; pll_cnt < ARRAY_SIZE(pll_cfg_val); pll_cnt++)
		hsdt_crg_reg_write(pcie, pll_cfg_val[pll_cnt], FNPLL_CFG(pll_cfg_base, pll_cnt));

	/* EN = 0, bypass = 1 */
	hsdt_crg_reg_write(pcie, 0x1B403E06, HSDT1_CRGFNPLL_CFG6);
	hsdt_crg_reg_write(pcie, 0x00800000, HSDT1_CRGFNPLL_CFG7);
}

static int pcie_fnpll_ctrl(struct pcie_kport *pcie, u32 enable)
{
	int ret = 0;

	if (enable) {
		fnpll_param(pcie);

		udelay(5);

		ret = clk_enable(pcie->pcie_refclk);
	} else {
		clk_disable(pcie->pcie_refclk);
	}

	return ret;
}

static void pcie_hp_debounce_clk_gt(struct pcie_kport *pcie, u32 enable)
{
	u32 mask, reg;

	reg = enable ? HSDT1_CRG_PEREN2 : HSDT1_CRG_PERDIS2;

	if (pcie->rc_id == PCIE_RC1)
		mask = PCIE1_CLK_HP_GATE | PCIE1_CLK_DEBUNCE_GATE;
	else
		mask = PCIE0_CLK_HP_GATE | PCIE0_CLK_DEBUNCE_GATE;
	hsdt_crg_reg_write(pcie, mask, reg);
}

static void pcie_asyncbrg_clk_gt(struct pcie_kport *pcie, u32 enable)
{
	u32 mask, reg;

	reg = enable ? HSDT1_CRG_PEREN2 : HSDT1_CRG_PERDIS2;
	mask = (pcie->rc_id == PCIE_RC1) ? GT_ACLK_NOC_PCIE1_ASYNCBRG :
			GT_ACLK_NOC_PCIE0_ASYNCBRG;
	hsdt_crg_reg_write(pcie, mask, reg);
}

static void pcie_tcu_rst(struct pcie_kport *pcie, u32 enable)
{
	if(enable && (atomic_sub_return(1, &tcu_urst_cnt) == 0))
		hsdt_crg_reg_write(pcie, IP_RST_PCIE_TCU, HSDT1_CRG_PERRSTEN2);
	else if (!enable && (atomic_add_return(1, &tcu_urst_cnt) == 1))
		hsdt_crg_reg_write(pcie, IP_RST_PCIE_TCU, HSDT1_CRG_PERRSTDIS2);
}

static void pcie_tcu_tbu_rst(struct pcie_kport *pcie, u32 enable)
{
	u32 mask, reg;

	reg = enable ? HSDT1_CRG_PERRSTEN2 : HSDT1_CRG_PERRSTDIS2;

	pcie_tcu_rst(pcie, enable);

	mask = pcie->rc_id == PCIE_RC1 ? IP_RST_PCIE1_TBU : IP_RST_PCIE0_TBU;
	hsdt_crg_reg_write(pcie, mask, reg);
}

/*
 * enable:ENABLE--enable clkreq control phyref clk
 *        DISABLE--disable clkreq control phyref clk
 */
static void pcie_phyref_hard_bypass(struct pcie_kport *pcie, bool enable)
{
	u32 val, reg_addr;

	if (pcie->rc_id == PCIE_RC0)
		reg_addr = HSDTCRG_PCIECTRL0;
	else
		reg_addr = HSDTCRG_PCIECTRL1;

	val = hsdt_crg_reg_read(pcie, reg_addr);
	if (enable)
		val &= ~PCIEPHY_REF_HW_BYPASS;
	else
		val |= PCIEPHY_REF_HW_BYPASS;
	hsdt_crg_reg_write(pcie, val, reg_addr);
}

/*
 * Config gt_clk_pciephy_ref_inuse
 * enable: ENABLE--controlled by ~pcie_clkreq_in
 *         FALSE--clock down
 */
static void pcie_phy_ref_clk_gt(struct pcie_kport *pcie, u32 enable)
{
	u32  mask, reg;

	/* HW bypass cfg */
	pcie_phyref_hard_bypass(pcie, enable);

	/* soft ref cfg,Always disable SW control */
	if (pcie->rc_id == PCIE_RC1) {
		mask = PCIE1_CLK_PHYREF_GATE;
		reg = HSDT1_CRG_PERDIS1;
	} else {
		mask = PCIE0_CLK_PHYREF_GATE;
		reg = HSDT1_CRG_PERDIS1;
	}
	hsdt_crg_reg_write(pcie, mask, reg);
}

static void pcie_oe_config(struct pcie_kport *pcie, u32 enable)
{
	u32 val, reg;

	if (pcie->rc_id == PCIE_RC1)
		reg = HSDT1_CRG_PCIECTRL1;
	else
		reg = HSDT1_CRG_PCIECTRL0;

	val = hsdt_crg_reg_read(pcie, reg);
	val |= (PCIEIO_OE_EN_SOFT | PCIEIO_OE_EN_HARD_BYPASS);
	val &= ~PCIEIO_OE_POLAR;
	hsdt_crg_reg_write(pcie, val, reg);
}

static void pcie_ie_config(struct pcie_kport *pcie, u32 enbale)
{
	u32 val, reg;

	if (pcie->rc_id == PCIE_RC1)
		reg = HSDT1_CRG_PCIECTRL1;
	else
		reg = HSDT1_CRG_PCIECTRL0;

	val = hsdt_crg_reg_read(pcie, reg);
	val |= PCIEIO_IE_EN_HARD_BYPASS;
	val &= ~PCIEIO_IE_EN_SOFT;
	val &= ~PCIEIO_IE_POLAR;
	hsdt_crg_reg_write(pcie, val, reg);
}

static void pcie_ioref_gt(struct pcie_kport *pcie, u32 enable)
{
	u32 val, reg;

	if (pcie->rc_id == PCIE_RC1) {
		val = PCIE1_CLK_IO_GATE;
		reg = HSDT1_CRG_PEREN2;
	} else {
		val = PCIE0_CLK_IO_GATE;
		reg = HSDT1_CRG_PEREN2;
	}
	hsdt_crg_reg_write(pcie, val, reg);

	/* enable/disable ie/oe according mode */
	if (unlikely(pcie->dtsinfo.ep_flag)) {
		pcie_oe_config(pcie, DISABLE);
		pcie_ie_config(pcie, enable);
	} else {
		pcie_oe_config(pcie, enable);
		pcie_ie_config(pcie, DISABLE);
	}
}

static int pcie_refclk_on(struct pcie_kport *pcie)
{
	int ret = 0;

	/* enable asyncbrg clk gate */
	pcie_asyncbrg_clk_gt(pcie, ENABLE);

	/* enable pcie hp & debounce clk */
	pcie_hp_debounce_clk_gt(pcie, ENABLE);

	if (likely(pcie->dtsinfo.ep_flag))
		return ret;
	if (atomic_add_return(1, &pll_init_cnt) == 1) {
		ret = pcie_fnpll_ctrl(pcie, ENABLE);
		if (ret) {
			PCIE_PR_E("Failed to enable 100MHz refclk");
			return ret;
		}
	}

	/* open pcie phy clk gate */
	pcie_phy_ref_clk_gt(pcie, ENABLE);
	/* pcie io clk gate */
	pcie_ioref_gt(pcie, ENABLE);

	return ret;
}

static void pcie_refclk_off(struct pcie_kport *pcie)
{
	pcie_asyncbrg_clk_gt(pcie, DISABLE);
	pcie_hp_debounce_clk_gt(pcie, DISABLE);

	if (unlikely(pcie->dtsinfo.ep_flag)) {
		pcie_ioref_gt(pcie, DISABLE);
		pcie_phy_ref_clk_gt(pcie, DISABLE);

		if (atomic_sub_return(1, &pll_init_cnt) == 0)
			pcie_fnpll_ctrl(pcie, DISABLE);
	}
}

static int pcie_refclk_ctrl(struct pcie_kport *pcie, bool clk_on)
{
	int ret;

	if (clk_on) {
		ret = clk_prepare(pcie->pcie_refclk);
		if (ret) {
			PCIE_PR_E("Failed to prepare pcie_refclk");
			return ret;
		}

		ret = pcie_refclk_on(pcie);
		if (ret) {
			PCIE_PR_E("Failed to enable pcie_refclk");
			goto REF_CLK;
		}
	} else {
		pcie_refclk_off(pcie);
		clk_unprepare(pcie->pcie_refclk);
	}
	return 0;

REF_CLK:
	clk_unprepare(pcie->pcie_refclk);
	return ret;
}

static int pcie_bc_monitor_asyncbrg_rst(struct pcie_kport *pcie, u32 enable)
{
	u32 reg, val;

	reg = enable ? HSDT1_CRG_PERRSTEN2 : HSDT1_CRG_PERRSTDIS2;

	if (pcie->rc_id == PCIE_RC1)
		val = IP_RST_NOC_PCIE1_ASYNCBRG | IP_RST_PCIE1_BC;
	else
		val = IP_RST_NOC_PCIE0_ASYNCBRG | IP_RST_PCIE0_BC;

	hsdt_crg_reg_write(pcie, val, reg);
	return 0;
}

static void pcie_ctrl_por_n_rst(struct pcie_kport *pcie, u32 enable)
{
	u32 reg, val;

	reg  = enable ? HSDT1_CRG_PERRSTEN2 : HSDT1_CRG_PERRSTDIS2;

	if (pcie->rc_id == PCIE_RC1)
		val = IP_RST_PCIE1CTRL_POR;
	else
		val = IP_RST_PCIE0CTRL_POR;

	hsdt_crg_reg_write(pcie, val, reg);
}

static void pcie_ctrl_perst_n_unrst(struct pcie_kport *pcie)
{
	u32 val;

	/* rst controller perst_n */
	if (pcie->rc_id == PCIE_RC1) {
		val = pcie_apb_ctrl_readl(pcie, SOC_PCIECTRL_CTRL12_ADDR);
		val &= ~PCIE_PERST_CONFIG_MASK;
		pcie_apb_ctrl_writel(pcie, val, SOC_PCIECTRL_CTRL12_ADDR);
	}
}

static void pcie_en_rx_term(struct pcie_kport *pcie)
{
	u32 val;

	val = pcie_apb_phy_readl(pcie, SOC_PCIEPHY_CTRL39_ADDR);
	val |= PCIEPHY_RX_TERMINATION_BIT;
	pcie_apb_phy_writel(pcie, val, SOC_PCIEPHY_CTRL39_ADDR);
}

/*
 * exit or enter noc power idle
 * exit: 1, exit noc power idle
 *       0, enter noc power idle
 */
static int pcie_noc_power_nap(struct pcie_kport *pcie, u32 exit)
{
	u32 mask_bits, val_bits, val;
	u32 time = NOC_TIMEOUT_VAL;

	if (pcie->rc_id == PCIE_RC0) {
		val_bits = NOC_PCIE0_POWER;
		mask_bits = NOC_PCIE0_POWER_MASK;
	} else {
		val_bits = NOC_PCIE1_POWER;
		mask_bits = NOC_PCIE1_POWER_MASK;
	}

	/*
	 * bits in mask_bits set to write the bit
	 * if bit in val_bits is 0, exit noc power idle
	 * or enter noc power idle
	 */
	 if (exit)
	 	val_bits = 0;

	writel(mask_bits | val_bits, pcie->pmctrl_base + NOC_POWER_IDLEREQ);
	val = readl(pcie->pmctrl_base + NOC_POWER_IDLE);
	while ((val & val_bits) != val_bits) {
		udelay(1);
		if (!time) {
			PCIE_PR_E("Failed to %s noc power idle state 0x%x",
				  (exit ? "exit" : "enter"), val);
			return -1;
		}
		time--;
		val = readl(pcie->pmctrl_base + NOC_POWER_IDLE);
	}

	return 0;
}

static bool is_smmu_bypass(struct pcie_kport *pcie)
{
#ifdef CONFIG_PCIE_KPORT_EP_FPGA_VERIFY
	return true;
#endif
	if (pcie->dtsinfo.board_type == BOARD_FPGA)
		return true;

	if (IS_ENABLED(CONFIG_MM_IOMMU_DMA) && pcie->dtsinfo.sup_iommus)
		return false;

	return true;
}

static noinline uint64_t pcie_tbu_bypass(struct pcie_kport *pcie)
{
	struct arm_smccc_res res;

	arm_smccc_smc((u64)FID_PCIE_SET_TBU_BYPASS, (u64)pcie->rc_id,
		      0, 0, 0, 0, 0, 0, &res);

	PCIE_PR_I("Bypass TBU %d", res.a0);
	return res.a0;
}

static int pcie_tbu_config(struct pcie_kport *pcie, u32 enable)
{
	u32 reg_val, tok_trans_gnt;
	u32 time = TBU_CONFIG_TIMEOUT;

	reg_val = readl(pcie->hsdtsctrl_base + HSDTSCTRL_AWMMUS);
	if (pcie->rc_id == PCIE_RC0)
		reg_val |= (PCIE_AWRMMUSID | PCIE_AWRMMUSSIDV);
	else
		reg_val |= (PCIE1_AWRMMUSID | PCIE_AWRMMUSSIDV);
	writel(reg_val, pcie->hsdtsctrl_base + HSDTSCTRL_AWMMUS);

	reg_val = readl(pcie->hsdtsctrl_base + HSDTSCTRL_ARMMUS);
	if (pcie->rc_id == PCIE_RC0)
		reg_val |= (PCIE_AWRMMUSID | PCIE_AWRMMUSSIDV);
	else
		reg_val |= (PCIE1_AWRMMUSID | PCIE_AWRMMUSSIDV);
	writel(reg_val, pcie->hsdtsctrl_base + HSDTSCTRL_ARMMUS);

	/* enable req */
	reg_val = readl(pcie->tbu_base + PCIE_TBU_CR);
	if (enable)
		reg_val |= PCIE_TBU_EN_REQ_BIT;
	else
		reg_val &= ~PCIE_TBU_EN_REQ_BIT;
	writel(reg_val, pcie->tbu_base + PCIE_TBU_CR);

	reg_val = readl(pcie->tbu_base + PCIE_TBU_CRACK);
	while (!(reg_val & PCIE_TBU_EN_ACK_BIT)) {
		udelay(1);
		if (time == 0) {
			PCIE_PR_E("TBU req(en/dis) not been acknowledged");
			return -1;
		}
		time--;
		reg_val = readl(pcie->tbu_base + PCIE_TBU_CRACK);
	}

	reg_val = readl(pcie->tbu_base + PCIE_TBU_CRACK);
	if (enable) {
		if (!(reg_val & PCIE_TBU_CONNCT_STATUS_BIT)) {
			PCIE_PR_E("TBU connecting failed");
			return -1;
		}
	} else {
		if (reg_val & PCIE_TBU_CONNCT_STATUS_BIT) {
			PCIE_PR_E("TBU is disconnect from TCU fail");
			return -1;
		}
	}

	if (enable) {
		reg_val = readl(pcie->tbu_base + PCIE_TBU_CRACK);
		tok_trans_gnt = (reg_val & PCIE_TBU_TOK_TRANS_MSK) >> PCIE_TBU_TOK_TRANS;
		if (tok_trans_gnt < PCIE_TBU_DEFALUT_TOK_TRANS) {
			PCIE_PR_E("tok_trans_gnt is less than setting");
			return -1;
		}
	}

	return 0;
}

static int pcie_smmu_cfg(struct pcie_kport *pcie, u32 enable)
{
	int ret = 0;
	int smmuid = PCIE0_SMMUID;

	if (enable) {
		ret = mm_smmu_nonsec_tcuctl(smmuid);
		if (ret) {
			PCIE_PR_E("TCU cfg Failed");
			return -1;
		}

		if (atomic_add_return(1, &tcu_pwr_cnt) == 1) {
			ret = mm_smmu_poweron(pcie->pci->dev);
			if (ret) {
				PCIE_PR_E("TCU power on Failed");
				return -1;
			}
		}

		ret = pcie_tbu_config(pcie, ENABLE);
		if (ret && (atomic_sub_return(1, &tcu_pwr_cnt) == 0)) {
			(void)mm_smmu_poweroff(pcie->pci->dev);
			PCIE_PR_E("TBU config Failed");
			return -1;
		}
		PCIE_PR_I("TBU init Done");
	} else {
		ret = pcie_tbu_config(pcie, DISABLE);
		if (ret) {
			PCIE_PR_E("TBU config(disconnect) Failed");
			return -1;
		}

		if (atomic_sub_return(1, &tcu_pwr_cnt) == 0) {
			ret = mm_smmu_poweroff(pcie->pci->dev);
			if (ret) {
				PCIE_PR_E("TCU power off Failed");
				return -1;
			}
		}
	}

	return ret;
}

static int pcie_smmu_init(struct pcie_kport *pcie, u32 enable)
{
	if (is_smmu_bypass(pcie)) {
		if (enable && pcie_tbu_bypass(pcie)) {
			PCIE_PR_E("Bypass TBU failed");
			return -1;
		}
		return 0;
	}

	return pcie_smmu_cfg(pcie, enable);
}

static void pcie_l1ss_fixup(struct pcie_kport *pcie)
{
	u32 val;
	/* fix l1ss exit issue */
	val = pcie_read_dbi(pcie->pci, pcie->pci->dbi_base, GEN3_RELATED_OFF, 0x4);
	val &= ~GEN3_ZRXDC_NONCOMPL;
	pcie_write_dbi(pcie->pci, pcie->pci->dbi_base, GEN3_RELATED_OFF, 0x4, val);
}

static int pcie_check_perst(struct pcie_kport *pcie)
{
	u32 val;
	u32 time = EP_CHECK_PERST;

	val = pcie_apb_ctrl_readl(pcie, PCIE_CTRL_STAT4);
	while (!(val & (PCIE_PAD_PERST_IN))) {
		udelay(100);
		if (!time) {
			PCIE_PR_E("Failed to unrst from opposite RC");
			return -1;
		}
		time--;
		val = pcie_apb_ctrl_readl(pcie, PCIE_CTRL_STAT4);
	}
	return 0;
}

static int pcie_turn_on(struct pcie_kport *pcie, enum rc_power_status on_flag)
{
	int ret = 0;
	unsigned long flags = 0;
	PCIE_PR_I("+%s+", __func__);

	mutex_lock(&pcie->power_lock);

	if (atomic_read(&(pcie->is_power_on))) {
		PCIE_PR_I("PCIe%u already power on", pcie->rc_id);
		goto MUTEX_UNLOCK;
	}

	pcie_rst(pcie);

	/* pclk for phy & ctrl */
	ret = pcie_clk_config(pcie, PCIE_INTERNAL_CLK, ENABLE);
	if (ret)
		goto INTERNAL_CLK;

	/* unrst pcie_phy_apb_presetn pcie_ctrl_apb_presetn */
	pcie_reset_ctrl(pcie, RST_DISABLE);

	if (pcie->dtsinfo.ep_flag) {
		ret = pcie_check_perst(pcie);
		if (ret)
			goto INTERNAL_CLK;
	}

	ret = pcie_clk_config(pcie, PCIE_EXTERNAL_CLK, ENABLE);
	if (ret)
		goto EXTERNAL_CLK;

	ret = pcie_refclk_ctrl(pcie, ENABLE);
	if (ret)
		goto REF_CLK;

	/* unrst BC & monitor , asyncbrg */
	pcie_bc_monitor_asyncbrg_rst(pcie, DISABLE);

	/* unrst smmu */
	pcie_tcu_tbu_rst(pcie, DISABLE);

	/* adjust output refclk amplitude, currently no adjust */
	pcie_io_adjust(pcie);

	pcie_config_axi_timeout(pcie);

	pcie_natural_cfg(pcie);

	pcie_phy_signal_unrst(pcie);

	ret = pcie_port_phy_init(pcie);
	if (ret) {
		PCIE_PR_E("PHY init Failed");
		goto PHY_INIT;
	}

	pcie_ctrl_por_n_rst(pcie, DISABLE);

	if (!is_pipe_clk_stable(pcie)) {
		ret = -1;
		PCIE_PR_E("PIPE clk is not stable");
		pcie_phy_state(pcie);
		goto GPIO_DISABLE;
	}

	if (!pcie_phy_ready(pcie)) {
		ret = -1;
		goto GPIO_DISABLE;
	}

	pcie_en_rx_term(pcie);

	ret = pcie_noc_power_nap(pcie, ENABLE);
	if (ret) {
		PCIE_PR_E("Fail to exit noc idle");
		goto GPIO_DISABLE;
	}

	ret = pcie_smmu_init(pcie, ENABLE);
	if (ret) {
		PCIE_PR_E("SMMU init Failed");
		goto SMMU_INIT_FAIL;
	}
	PCIE_PR_I("SMMU init Done");

	flags = pcie_idle_spin_lock(pcie->idle_sleep, flags);
	atomic_add(1, &(pcie->is_power_on));
	pcie_idle_spin_unlock(pcie->idle_sleep, flags);

	pcie_l1ss_fixup(pcie);

	pcie_phy_irq_init(pcie);

#ifdef XW_PHY_MCU
	phy_core_on(pcie);
	udelay(50);
#endif

	/* pcie 32K idle */
	pcie_32k_idle_init(pcie->idle_sleep);

	PCIE_PR_I("-%s-", __func__);

	goto MUTEX_UNLOCK;

SMMU_INIT_FAIL:
	ret = pcie_noc_power_nap(pcie, DISABLE);
	if (ret)
		PCIE_PR_E("Fail to enter noc idle");
GPIO_DISABLE:
	pcie_tcu_tbu_rst(pcie, ENABLE);
	pcie_perst_cfg(pcie, DISABLE);
PHY_INIT:

REF_CLK:
	pcie_reset_ctrl(pcie, RST_ENABLE);
	(void)pcie_clk_config(pcie, PCIE_EXTERNAL_CLK, DISABLE);
EXTERNAL_CLK:
	(void)pcie_clk_config(pcie, PCIE_INTERNAL_CLK, DISABLE);
	(void)pcie_refclk_ctrl(pcie, DISABLE);
INTERNAL_CLK:
	PCIE_PR_E("Failed to PowerOn");
MUTEX_UNLOCK:
	mutex_unlock(&pcie->power_lock);

	return ret;
}

static int pcie_turn_off(struct pcie_kport *pcie, enum rc_power_status on_flag)
{
	int ret = 0;
	unsigned long flags = 0;

	PCIE_PR_I("+%s+", __func__);

	mutex_lock(&pcie->power_lock);
	if (!atomic_read(&(pcie->is_power_on))) {
		PCIE_PR_I("PCIe%u already power off", pcie->rc_id);
		goto MUTEX_UNLOCK;
	}

	flags = pcie_idle_spin_lock(pcie->idle_sleep, flags);
	atomic_set(&(pcie->is_power_on), 0);
	pcie_idle_spin_unlock(pcie->idle_sleep, flags);

	pcie_32k_idle_deinit(pcie->idle_sleep);

	pcie_phy_irq_deinit(pcie);

#ifdef XW_PHY_MCU
	phy_core_off(pcie);
#endif

	pcie_axi_timeout_mask(pcie);

	ret = pcie_smmu_init(pcie, DISABLE);
	if (ret)
		PCIE_PR_E("SMMU deinit failed");

	ret = pcie_noc_power_nap(pcie, DISABLE);
	if (ret)
		PCIE_PR_E("Fail to enter noc idle");

	pcie_bc_monitor_asyncbrg_rst(pcie, ENABLE);
	pcie_ctrl_por_n_rst(pcie, ENABLE);
	pcie_tcu_tbu_rst(pcie, ENABLE);
	pcie_ctrl_perst_n_unrst(pcie);

	clk_disable_unprepare(pcie->pcie_aux_clk);
	clk_disable_unprepare(pcie->pcie_tbu_clk);
	clk_disable_unprepare(pcie->pcie_tcu_clk);
	clk_disable_unprepare(pcie->pcie_aclk);
	(void)pcie_refclk_ctrl(pcie, DISABLE);

	/* rst pcie_phy_apb_presetn pcie_ctrl_apb_presetn */
	pcie_reset_ctrl(pcie, RST_ENABLE);
	clk_disable_unprepare(pcie->apb_sys_clk);
	clk_disable_unprepare(pcie->apb_phy_clk);

	PCIE_PR_I("-%s-", __func__);
MUTEX_UNLOCK:
	mutex_unlock(&pcie->power_lock);
	return ret;
}

#ifdef CONFIG_PCIE_KPORT_IDLE
static void pcie_ref_clk_on(struct pcie_kport *pcie)
{
	if (!atomic_read(&pcie->is_power_on))
		return;

	pcie_refclk_on(pcie);
}

static void pcie_ref_clk_off(struct pcie_kport *pcie)
{
	if (!atomic_read(&pcie->is_power_on))
		return;

	pcie_refclk_off(pcie);
}

static void pcie_pll_gate_status(struct pcie_kport *pcie)
{
	uint32_t pll_gate_reg = HSDT1_CRGPCIEPLL_STAT;

	PCIE_PR_I("[PCIe%u] PCIEPLL STATE 0x%-8x: %8x",
		pcie->rc_id,
		pll_gate_reg,
		hsdt_crg_reg_read(pcie, pll_gate_reg));
}
#endif

struct pcie_platform_ops plat_ops = {
	.plat_on       = pcie_turn_on,
	.plat_off      = pcie_turn_off,
	.sram_ext_load = NULL,
#ifdef CONFIG_PCIE_KPORT_IDLE
	.ref_clk_on    = pcie_ref_clk_on,
	.ref_clk_off   = pcie_ref_clk_off,
	.pll_status     = pcie_pll_gate_status,
#endif
};

static int pcie_get_clk(struct platform_device *pdev, struct pcie_kport *pcie)
{
	pcie->pcie_tcu_clk = devm_clk_get(&pdev->dev, "pcie_tcu_clk");
	if (IS_ERR(pcie->pcie_tcu_clk)) {
		PCIE_PR_E("Failed to get pcie_tcu_clk");
		return PTR_ERR(pcie->pcie_tcu_clk);
	}

	pcie->pcie_tbu_clk = devm_clk_get(&pdev->dev, "pcie_tbu_clk");
	if (IS_ERR(pcie->pcie_tbu_clk)) {
		PCIE_PR_E("Failed to get pcie_tbu_clk");
		return PTR_ERR(pcie->pcie_tbu_clk);
	}

	pcie->pcie_refclk = devm_clk_get(&pdev->dev, "pcie_refclk");
	if (IS_ERR(pcie->pcie_refclk)) {
		PCIE_PR_E("Failed to get pcie_refclk");
		return PTR_ERR(pcie->pcie_refclk);
	}

	return 0;
}

int pcie_plat_init(struct platform_device *pdev, struct pcie_kport *pcie)
{
	PCIE_PR_I("+%s+", __func__);
	struct device_node *np = NULL;

	if (pcie_get_clk(pdev, pcie))
		return -1;

	np = of_find_compatible_node(NULL, NULL, "hisilicon,hsdt1_crg");
	if (!np) {
		PCIE_PR_E("Failed to get hsdt-crg Node");
		return -1;
	}

	pcie->crg_base = of_iomap(np, 0);
	if (!pcie->crg_base) {
		PCIE_PR_E("Failed to iomap hsdt_base");
		return -1;
	}

	if (pcie->rc_id == PCIE_RC0)
		pcie->tbu_base = ioremap(PCIE0_TBU_BASE, PCIE_TBU_SIZE);
	else
		pcie->tbu_base = ioremap(PCIE1_TBU_BASE, PCIE_TBU_SIZE);

	if (!pcie->tbu_base) {
		PCIE_PR_E("Failed to iomap tbu_base");
		goto CRG_BASE_UNMAP;
	}

	pcie->plat_ops = &plat_ops;
	PCIE_PR_I("-%s-", __func__);

	return 0;

CRG_BASE_UNMAP:
	iounmap(pcie->crg_base);
	return -1;
}
