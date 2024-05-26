/*
 * pcie-kport-phy-kphy.c
 *
 * kport PCIePHY Driver
 *
 * Copyright (c) 2020 Huawei Technologies Co., Ltd.
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

#include <linux/clk.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/irq.h>
#include <linux/of_address.h>
#include <linux/delay.h>

#include <securec.h>

#include "pcie-kport-phy.h"
#include "pcie-kport-phy-kphy.h"

static u32 pcie_phy_init_tbl[][2] =
{
	{PHY_PLL_REFCLK_SEL_VAL, PHY_PLL_REFCLK_SEL_ADDR},           /* PLL ref clk select */
	{PHY_REG_DA_DFE_CTRL_EN_VAL, PHY_REG_DA_DFE_CTRL_EN_ADDR},   /* dfe_en = 0 */
	{PHY_REG_DA_DFE_CTRL_VAL_VAL, PHY_REG_DA_DFE_CTRL_VAL_ADDR},
	{PHY_RG_BG_VREF_SEL_VAL, PHY_RG_BG_VREF_SEL_ADDR},           /* bg mt en = 1 */
	{PHY_PLL_UPDATE_CTRL_VAL, PHY_PLL_UPDATE_CTRL_ADDR},         /* bias_stben/ldo_stben */
	{PHY_REG_DA_TX_EQ_CTRL_VAL_VAL, PHY_REG_DA_TX_EQ_CTRL_ADDR}, /* close tx ffe */
	{PHY_REG_DA_TX_EQ_FFE_VAL, PHY_REG_DA_TX_EQ_FFE},
	{PHY_RXUPDTCTRL_VAL, PHY_RXUPDTCTRL_EN_ADDR},                /* gs eq en */
	{PHY_DFE_MANUAL_EN_VAL, PHY_DFE_MANUAL_EN_ADDR},             /* sel_r and sel_gain manual controls */
	{PHY_DFE_EN_VAL, PHY_DFE_EN_ADDR},                           /* dfe tap3/2/1_en = 0 */
	{PHY_PMA_RX_ATT_RG_RX0_VAL, PHY_PMA_RX_ATT_RG_RX0_ADDR},     /* PMA RX ATT GEN1-GEN3     PMA RG_RX0_EQ_R GEN1-GEN3 */
	{PHY_RX_EQA_CTRL_SEL_C_VAL, PHY_RX_EQA_CTRL_ADDR},           /* rxctrl eqa sel c */
	{PHY_RX_EQA_TH_VAL, PHY_RX_EQA_CTRL_ADDR},                   /* rxctrl eqa th */
	{PHY_RG_RX0_GAIN_SEL_VAL, PHY_RG_RX0_GAIN_SEL_ADDR},         /* reg RX0 GAIN select */
	{PHY_RG_G1_KI_FINE_VAL, PHY_RG_G1_KI_FINE_ADDR},             /* GEN1 KI fine */
	{PHY_RG_G2_G3_KI_FINE_VAL, PHY_RG_G2_G3_KI_FINE_ADDR},       /* GEN2 GEN3 KI fine */
	{PHY_RG_RX0_LOS_HYS_VAL, PHY_RG_RX0_LOS_HYS_ADDR},           /* reg RX0 LOS HYS */
	{PHY_RG_RX0_GAIN_VREF_VAL, PHY_RG_RX0_GAIN_VREF_ADDR},       /* reg RX0 GAIN VREF */
	{PHY_RG_RX_CTRL_GSA_TH_VAL, PHY_RG_RX0_GAIN_VREF_ADDR},      /* rxctrl gsa th */
	{PHY_RG_RX0_EQ_PAT_MODE_CFG_VAL, PHY_RG_RX0_EQ_PAT_MODE_CFG_ADDR}, /* EQ pattern and EQ CFG */
	{PHY_DCC_WAITING_TIME_VAL, PHY_DCC_WAITING_TIME_ADDR},       /* DCC waiting time */
	{PHY_TX_TERMK_WATING_TIME_VAL, PHY_TX_TERMK_WATING_TIME_ADDR}, /* TX TERMK waiting time */
	{PHY_TX_TERMK_WATING_TIME1_2_VAL, PHY_TX_TERMK_WATING_TIME1_2_ADDR},
	{PHY_PLL_LOCK_WAIT_TIME_VAL, PHY_PLL_LOCK_WAIT_TIME_ADDR},  /* pll lock keep time */
};

static inline void kphy_phy_write(struct pcie_phy *phy, uint32_t val, uint32_t reg)
{
	writel(val, phy->phy_base + reg);
}

static inline uint32_t kphy_phy_read(struct pcie_phy *phy, uint32_t reg)
{
	return readl(phy->phy_base + reg);
}

static inline void apb_phy_write(struct pcie_phy *phy, uint32_t val, uint32_t reg)
{
	writel(val, phy->apb_phy_base + reg);
}

static inline uint32_t apb_phy_read(struct pcie_phy *phy, uint32_t reg)
{
	return readl(phy->apb_phy_base + reg);
}

static inline void kphy_phy_core_write(struct pcie_phy *phy, uint32_t val, uint32_t reg)
{
	writel(val, phy->core_base + reg);
}

static inline uint32_t kphy_phy_core_read(struct pcie_phy *phy, uint32_t reg)
{
	return readl(phy->core_base + reg);
}

static void kphy_phy_w(struct pcie_phy *phy, uint32_t reg, uint32_t msb, uint32_t lsb, uint32_t val)
{
	uint32_t data = kphy_phy_read(phy, reg);

	data = DRV_32BIT_SET_FIELD(data, lsb, msb, val);
	kphy_phy_write(phy, data, reg);
}

static uint32_t kphy_phy_r(struct pcie_phy *phy, uint32_t reg, uint32_t msb, uint32_t lsb)
{
	uint32_t data = kphy_phy_read(phy, reg);

	return DRV_32BIT_READ_FIELD(data, lsb, msb);
}

static void collect_phy3_info(struct pcie_phy *phy)
{
	uint32_t reg_val;

	reg_val = kphy_phy_read(phy, PHY_DCC_INTR_FLAG);
	D("phy3: dcc overflow/underflow intr flag: 0x%x %d\n", reg_val,
			(reg_val & PHY_DCC_INTR_FLAG_MASK) >> PHY_DCC_INTR_FLAG_OFFSET);

	reg_val = kphy_phy_read(phy, PHY_TX_RX_TERMK_INTR_FLAG);
	D("phy3: tx termk overflow/underflow intr: 0x%x %d\n", reg_val,
			(reg_val & PHY_TX_TERMK_INTR_FLAG_MASK) >> PHY_TX_TERMK_INTR_FLAG_OFFSET);
	D("phy3: rx termk overflow/underflow intr: 0x%x %d\n", reg_val,
			(reg_val & PHY_RX_TERMK_INTR_FLAG_MASK) >> PHY_RX_TERMK_INTR_FLAG_OFFSET);

	reg_val = kphy_phy_read(phy, PHY_PLL_GEN12_VCO_BAND);
	D("phy3: vco band value in GEN1/2: 0x%x\n", reg_val);

	reg_val = kphy_phy_read(phy, PHY_PLL_GEN3_VCO_BAND);
	D("phy3: vco band value in GEN3: 0x%x\n", reg_val);

	reg_val = kphy_phy_read(phy, PHY_DCC_CALI_RES);
	D("phy3: dcc calibration result: 0x%x\n", reg_val);

	reg_val = kphy_phy_read(phy, PHY_TX_TERMK_RES);
	D("phy3: tx termk result: 0x%x\n", reg_val);

	reg_val = kphy_phy_read(phy, PHY_RX_TERMK_RES);
	D("phy3: rx termk result: 0x%x\n", reg_val);
}

static void kphy_phy_init(struct pcie_phy *phy)
{
	uint32_t i, array_size;

	I("+%s+\n", __func__);

	array_size = ARRAY_SIZE(pcie_phy_init_tbl);
	for (i = 0; i < array_size; i++)
		kphy_phy_write(phy, pcie_phy_init_tbl[i][0], pcie_phy_init_tbl[i][1]);

	I("-%s-\n", __func__);
}

static bool kphy_phy_ready(struct pcie_phy *phy)
{
	uint32_t reg_val, lock_res;
	int timeout = 0;

	while(1) {
		reg_val = kphy_phy_read(phy, PHY_PCS_POWER_STATUS_ADDR);
		reg_val &= ~PHY_PCS_STATUS_MASK;
		if ((reg_val >> PHY_POWERON_STATUS_BIT) & 1) {
			D("PHY3 power on in %d us\n", timeout);

			lock_res = kphy_phy_read(phy, PHY_DIGITAL_PLL_LOCK_RES);
			D("phy3: digital pll lock result: 0x%x\n", lock_res);
			return true;
		}

		udelay(5);
		timeout++;
		if (timeout > PHY_POWERON_TIMEOUT) {
			E("PHY3 power on timeout!\n");
			return false;
		}
	}

	return true;
}

static void kphy_phy_ffe_cfg(struct pcie_phy *phy)
{
	uint32_t reg_tx_rtermk;

	udelay(350);
	reg_tx_rtermk = kphy_phy_read(phy, PHY_RG_TX_RTERMK_V48_ADDR);
	reg_tx_rtermk = (reg_tx_rtermk >> PHY_RG_TX_RTERMK_V48_BIT) & 1;
	if (reg_tx_rtermk) {
		kphy_phy_write(phy, PHY_TXDEEMPHCTRL6DB_VAL, PHY_TXDEEMPHCTRL6DB_ADDR);
		kphy_phy_write(phy, PHY_TXDEEMPHCTRL3D5DB_VAL, PHY_TXDEEMPHCTRL3D5DB_ADDR);
		kphy_phy_write(phy, PHY_TXDEEMPHCTRL0DB_VAL, PHY_TXDEEMPHCTRL0DB_ADDR);
		kphy_phy_write(phy, PHY_RG_PIPE_TXDEEMPH_CTRL_VAL, PHY_RG_PIPE_TXDEEMPH_CTRL_ADDR);
		/* config FS, LF */
		kphy_phy_write(phy, PHY_RG_LOCAL_LF_FS_VAL, PHY_RG_LOCAL_LF_FS_ADDR);
	} else {
		/* config FS, LF */
		kphy_phy_write(phy, PHY_RG_LOCAL_LF_FS_V48_VAL, PHY_RG_LOCAL_LF_FS_ADDR);
	}

	collect_phy3_info(phy);

	phy->rtermk_v48 = reg_tx_rtermk;
}

static void show_bgr_trimming_res(struct pcie_phy *phy)
{
	uint32_t reg_val;
	int err, i;
	int min = 200;

	/* BG_CTRL_REG_0 */
	kphy_phy_w(phy, PHY_RG_BG_VREF_SEL_ADDR, 31, 0, 0x0);
	/* en bandgap */
	kphy_phy_w(phy, PHY_INTF_COM_SIG_CTRL_EN_REG, 31, 0, 0x3);
	/* reg_da_pll_bias_en_ctrl_val */
	kphy_phy_w(phy, PHY_REG_DA_PLL_BIAS_EN_ADDR, 31, 0, 0x2);
	/* reg_da_pll_ldo_en_ctrl_val */
	kphy_phy_w(phy, PHY_REG_DA_PLL_LDO_EN_ADDR, 31, 0, 0x00010001);
	/* DA_RX0_CDR_LDO_EN=1 */
	kphy_phy_w(phy, PHY_DA_RX0_CDR_LDO_EN_ADDR, 31, 0, 0x8);
	/* reg_da_rx_cdr_ldo_en_ctrl_val */
	kphy_phy_w(phy, PHY_REG_DA_RX_CDR_LDO_EN_ADDR, 31, 0, 0x80);
	/* RG_COM_TST<7:0>=8'b01100001 */
	kphy_phy_w(phy, PHY_REG_COM_TST_ADDR, 31, 0, 0x61);
	/* PMA_PCS_IO_NEW_ADD[17:16]:reg_adc_ck_div */
	kphy_phy_w(phy, PHY_ADC_CLK_DIV_ADDR, 31, 0, 0x10000);
	/* rg_valid_dy_sel and rg_adc_en */
	kphy_phy_w(phy, PHY_REG_VALID_DY_SEL_ADC_EN_ADDR, 31, 0, 0x4C);

	reg_val = kphy_phy_r(phy, PHY_ADC_CLK_DIV_ADDR, 7, 0);
	if (reg_val <= ADC_VALID_MIN || reg_val >= ADC_VALID_MAX)
		E("Init adc value: %d\n", reg_val);

	/* RG_COM_TST<7:0>=8'b01100011 */
	kphy_phy_w(phy, PHY_REG_COM_TST_ADDR, 31, 0, 0x63);
	/* RG_RX0_DC_TEST_SEL<3:0>=4'b0101 */
	kphy_phy_w(phy, PHY_REG_RX0_DC_TEST_SEL, 31, 0, 0x0514A245);

	reg_val = kphy_phy_r(phy, PHY_ADC_CLK_DIV_ADDR, 7, 0);
	err = ADC_APPR_VALUE - reg_val;
	min = abs(err) < min ? abs(err) : min;
	if (min <= 3) {
		I("BG trimming down.\n");
	} else {
		E("Start BG trimming.\n");
		for (i = 0; i < TRIMMING_TIMES; i++) {
			kphy_phy_w(phy, PHY_RG_BG_VREF_SEL_ADDR, 3, 0, i);
			udelay(100);

			reg_val = kphy_phy_r(phy, PHY_ADC_CLK_DIV_ADDR, 7, 0);
			err = ADC_APPR_VALUE - reg_val;
			min = abs(err) < min ? abs(err) : min;
			if (min <= 3) {
				I("BG trimming down, index is %d.\n", i);
				break;
			}
		}
	}

	if (min > 3)
		E("BG trimming error, adc value: %d, os_code = %d mV\n", min + ADC_APPR_VALUE, min * 2 * 6);
}

#ifdef CONFIG_PCIE_KPORT_TEST
static int phy3_eye_monitor_para(struct pcie_phy *phy, u32 quadrant, int vref, int pi)
{
	u32 reg_val;
	int timeout = PHY3_FOM_TIMEOUT;
	int loop_cnt = 0;
	int ch;

	kphy_phy_w(phy, PHY_RXUPDTCTRL_EN_ADDR, 11, 11, 0x0);
	kphy_phy_w(phy, PHY_RXFOMCTRL1_ADDR, 6, 6, 0x0);
	/* quadrant sel */
	kphy_phy_w(phy, PHY_RXFOMCTRL1_ADDR, 7, 0, quadrant);
	/* vref */
	kphy_phy_w(phy, PHY_RXFOMCTRL1_ADDR, 15, 12, vref);
	/* pi_step */
	kphy_phy_w(phy, PHY_RXFOMCTRL1_ADDR, 11, 8, pi);
	/* fom_sw_load */
	kphy_phy_w(phy, PHY_RXFOMCTRL0_ADDR, 1, 1, 0x1);
	udelay(1);
	/* fom en */
	kphy_phy_w(phy, PHY_RXFOMCTRL0_ADDR, 2, 2, 0x1);
	/* rx_rxg en */
	kphy_phy_w(phy, PHY_RXFOMCTRL0_ADDR, 5, 5, 0x1);
	/* PMA_CLK_CG_EN */
	kphy_phy_w(phy, PHY_CRG_PER_LANE_ADDR, 13, 13, 0x1);

	udelay(1);
	kphy_phy_w(phy, PHY_RXFOMCTRL0_ADDR, 0, 0, 0x1);
	udelay(2);

	/* wait from cpl intr. */
	reg_val = kphy_phy_r(phy, PHY_PCLK_LANE_INT_SRC_REG_ADDR, 18, 18);
	while (reg_val != 0x1) {
		reg_val = kphy_phy_r(phy, PHY_PCLK_LANE_INT_SRC_REG_ADDR, 18, 18);
		loop_cnt++;
		udelay(5);
		if (loop_cnt > timeout) {
			E("Timeout to wait fom cpl intr\n");
			return 'x';
		}
	}
	reg_val = kphy_phy_r(phy, PHY_RXOFSTSTATUS_ADDR, 15, 8);
	/* fom en = 0 */
	kphy_phy_w(phy, PHY_RXFOMCTRL0_ADDR, 2, 0, 0);
	kphy_phy_w(phy, PHY_RXUPDTCTRL_EN_ADDR, 5, 5, 0);

	if (reg_val == FOM_COUNT_MAX)
		ch = ' ';
	else if (reg_val == 0)
		ch = '-';
	else
		ch = '+';

	return ch;
}

u32 g_quadrant[4] = {QUADRANT1, QUADRANT4, QUADRANT2, QUADRANT3};
int g_dir[][2] = {{-1, -1}, {1, -1}, {-1, 1}, {1, 1}};
char g_res[GRAPH_LEN][GRAPH_LEN + 1];
char g_eye_title[] = "FEDCBA98765432100123456789ABCDEF";
static int kphy_phy_eye_monitor(struct pcie_phy *phy)
{
	int i, j, pi_step, vref, ch;
	char out[IMAGE_WIDTH];

	if (memset_s(g_res, sizeof(g_res), 'x', sizeof(g_res)))
		return -1;

	for (i = 0; i < 4; i++) {
		for (vref = 0; vref < QUA_LEN; vref++) {
			for (pi_step = 0; pi_step < QUA_LEN; pi_step++) {
				ch = phy3_eye_monitor_para(phy, g_quadrant[i], vref, pi_step);
				g_res[QUA_LEN + (vref * g_dir[i][1])][QUA_LEN + (pi_step * g_dir[i][0])] = ch;
			}
		}
	}

	if (memcpy_s(g_res[0], sizeof(g_res), g_eye_title, sizeof(g_eye_title)))
		return -1;

	for (i = 0; i < GRAPH_LEN; i++) {
		g_res[i][GRAPH_LEN - 1] = 0;
		g_res[i][0] = g_eye_title[i];
		if (memset_s(out, sizeof(out), ' ', sizeof(out)))
			return -1;

		for (j = 0; j < GRAPH_LEN; j++)
			out[2 * j] = g_res[i][j];
		out[IMAGE_WIDTH - 1] = 0;
		I("%s", out);
	}

	return 0;
}

static int kphy_phy_debug_info(struct pcie_phy *phy)
{
	I("RC[%u]  BGR_trimming: 0x%x", phy->id, kphy_phy_r(phy, PHY_RG_BG_VREF_SEL_ADDR, 3, 0));

	I("RC[%u]  ana_pll_lock: 0x%x", phy->id, kphy_phy_r(phy, PHY_AD_PLL_LOCK_ADDR, 1, 0));

	I("RC[%u]  ad_pll_lock: 0x%x", phy->id, kphy_phy_r(phy, PHY_AD_PLL_LOCK_STATUS_ADDR, 8, 8));

	I("RC[%u]  pll lock det dbg addr[0xB6C]: 0x%x", phy->id, kphy_phy_r(phy, 0xB6C, 31, 0));

	I("RC[%u]  dig_pll_lock: 0x%x", phy->id, kphy_phy_r(phy, PHY_AD_PLL_LOCK_ADDR, 2, 2));

	I("RC[%u]  pll_lock_overtime: 0x%x", phy->id, kphy_phy_r(phy, PHY_AD_PLL_LOCK_ADDR, 3, 3));

	kphy_phy_w(phy, PHY_REG_PV_DEBUG_CFG_ADDR, 3, 0, 0x7);

	I("RC[%u]  pv_out: 0x%x", phy->id, kphy_phy_r(phy, PHY_ADC_CLK_DIV_ADDR, 15, 8));

	I("RC[%u]  vco_band_gen2: 0x%x", phy->id, kphy_phy_r(phy, PHY_PLL_BANDCTRLR1_ADDR, 9, 0));

	I("RC[%u]  vco_band_gen3: 0x%x", phy->id, kphy_phy_r(phy, PHY_PLL_BANDCTRLR2_ADDR, 9, 0));

	return 0;
}
#endif

static void kphy_phy_stat_dump(struct pcie_phy *phy)
{
	I("+%s+\n", __func__);

	/* PCLK_COM Interrupt Source register */
	pr_info("PCLK_COM Interrupt Source register\n");
	pr_info("0x%-8x: %8x\n", 0x404, kphy_phy_read(phy, 0x404));
	/* APBCLK_LANE_INT_SRC_REG */
	pr_info("APBCLK_LANE_INT_SRC_REG\n");
	pr_info("0x%-8x: %8x\n", 0x220, kphy_phy_read(phy, 0x220));
	/* PCLK_LANE_INT_SRC_REG */
	pr_info("PCLK_LANE_INT_SRC_REG\n");
	pr_info("0x%-8x: %8x\n", 0x1000, kphy_phy_read(phy, 0x1000));
#ifdef CONFIG_PCIE_KPORT_TEST
	kphy_phy_debug_info(phy);
#endif

	I("+%s+\n", __func__);
}

static struct mcu_irq_info kphy_phy_irq_mcu[MCU_IRQ_MASK_NUM] = {
	{ 0x410, 0x9, }, /* MCU intr mask reg */
	{ 0x10C, 0x1, }, /* TALK TOP */
};

static void init_phy_irq_to_mcu(struct pcie_phy *phy, bool enable)
{
	uint32_t val;
	uint32_t reg;
	int32_t i;

	for (i = 0; i < MCU_IRQ_MASK_NUM; i++) {
		reg = kphy_phy_irq_mcu[i].mask_reg;
		val = kphy_phy_read(phy, reg);
		if (enable)
			val &= ~(kphy_phy_irq_mcu[i].mask_bits);
		else
			val |= kphy_phy_irq_mcu[i].mask_bits;
		kphy_phy_write(phy, val, reg);
	}
}

static void init_phy_irq_to_acore(struct pcie_phy *phy, bool enable)
{
	uint32_t val;

	val = kphy_phy_read(phy, PCS_TOP_MASK_REG);
	if (enable)
		val &= ~PCS_TOP_MASK_BIT;
	else
		val |= PCS_TOP_MASK_BIT;
	kphy_phy_write(phy, val, PCS_TOP_MASK_REG);
}

#ifdef CONFIG_PCIE_KPORT_JUN
/* trigger condition, for pcie-jun phy bug fix.
 * gp int3 cplt = 1, powerdown 4->2
 */
static void kphy_gp3_trigger_init(struct pcie_phy *phy)
{
	kphy_phy_w(phy, PHY_DFX_CRG_ADDR, 2, 2, 0x1);       /* REG Contorl DFX CLK Gating enable */
	kphy_phy_w(phy, PHY_PCLK_COM_INTR_MASK_REG, 3, 3, 0x0); /* enable gp3 intr */
	kphy_phy_w(phy, PHY_GP_INTR3_TRIG_MASK, 31, 0, 0x0);    /* clear gp3 trigger mask */
	kphy_phy_w(phy, PHY_GP_INTR3_TRIG_VAL, 31, 0, 0x0);
	kphy_phy_w(phy, PHY_GP_INTR3_TRIG_MASK, 10, 7, 0xF);
	kphy_phy_w(phy, PHY_GP_INTR3_TRIG_MASK, 27, 24, 0xF);
	kphy_phy_w(phy, PHY_GP_INTR3_TRIG_VAL, 10, 7, 0x2);
	kphy_phy_w(phy, PHY_GP_INTR3_TRIG_VAL, 27, 24, 0x4);
	kphy_phy_w(phy, PHY_GP_INTR23_TRIG_MASK_H, 31, 16, 0x0);
	kphy_phy_w(phy, PHY_GP_INTR23_TRIG_VAL_H, 31, 16, 0x0);
}
#endif

static void kphy_phy_irq_init(struct pcie_phy *phy)
{
	I("%s\n", __func__);

#ifdef XW_PHY_MCU
	I("clear init phy irq done\n");
	init_phy_irq_to_mcu(phy, ENABLE);
#else
	init_phy_irq_to_mcu(phy, DISABLE);
#endif
	I("init phy irq to mcu done\n");
	init_phy_irq_to_acore(phy, DISABLE);

#ifdef CONFIG_PCIE_KPORT_JUN
	kphy_gp3_trigger_init(phy);
#endif
}

static void kphy_phy_irq_disable(struct pcie_phy *phy)
{
	I("%s\n", __func__);
	init_phy_irq_to_mcu(phy, DISABLE);
	init_phy_irq_to_acore(phy, DISABLE);
}

struct pcie_phy_ops kphy_phy_ops = {
	.irq_init = kphy_phy_irq_init,
	.irq_disable = kphy_phy_irq_disable,
	.irq_handler = NULL,
	.phy_init = kphy_phy_init,
	.is_phy_ready = kphy_phy_ready,
	.phy_stat_dump = kphy_phy_stat_dump,
	.phy_r = kphy_phy_r,
	.phy_w = kphy_phy_w,
	.phy_ffe_cfg = kphy_phy_ffe_cfg,
#ifdef CONFIG_PCIE_KPORT_TEST
	.phy3_eye_monitor = kphy_phy_eye_monitor,
	.phy3_debug_info = kphy_phy_debug_info,
#endif
};

/* parse rc-id from pcie_port device_node */
static int32_t kphy_phy_get_host_id(struct platform_device *pdev,
		struct pcie_phy *phy_info)
{
	int32_t ret;

	ret = of_property_read_u32(pdev->dev.of_node, "rc-id", &phy_info->id);
	if (ret) {
		E("Fail to read Host id\n");
		return ret;
	}

	D("Host id: %u\n", phy_info->id);
	return 0;
}

int32_t pcie_phy_register(struct platform_device *pdev)
{
	int32_t ret;
	struct pcie_phy *phy_info = NULL;
	struct device_node *phy_np = NULL;

	I("+%s+\n", __func__);
	phy_info = devm_kzalloc(&pdev->dev, sizeof(*phy_info), GFP_KERNEL);
	if (!phy_info)
		return -ENOMEM;

	ret = kphy_phy_get_host_id(pdev, phy_info);
	if (ret)
		return ret;

	phy_np = of_find_compatible_node(pdev->dev.of_node,
			NULL, "pcie,xw-phy");
	if (!phy_np) {
		E("Get PHY Node\n");
		return -ENODEV;
	}

	ret = pcie_phy_ops_register(phy_info->id, phy_info, &kphy_phy_ops);
	if (ret) {
		E("register phy ops\n");
		return ret;
	}

	mutex_init(&phy_info->irq_mutex_lock);

	I("-%s-\n", __func__);
	return 0;
}