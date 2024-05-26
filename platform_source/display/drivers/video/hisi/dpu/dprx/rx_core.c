/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include "rx_core.h"
#include "rx_reg.h"
#include "rx_edid.h"

void dprx_dis_reset(struct dprx_ctrl *dprx, bool benable)
{
	uint32_t reg;

	dpu_check_and_no_retval((dprx == NULL), err, "dprx is NULL!\n");

	if (dprx->fpga_flag== true) {
		if (benable) {
			pr_info("[DPRX] dprx%u disreset +\n", dprx->id);
			if (dprx->id == 0) {
				reg = inp32(dprx->hsdt1_crg_base + DPRX0_DIS_RESET_OFFSET); /* 0xEB045000 + 0x64 */
				reg |= DPRX0_APB_DIS_RESET_BIT;
				reg |= DPRX0_CTRL_DIS_RESET_BIT;
				reg |= DPRX0_EDID_DIS_RESET_BIT;
				reg |= DPRX0_APB_BRIDGE_DIS_RESET_BIT;
				outp32(dprx->hsdt1_crg_base + DPRX0_DIS_RESET_OFFSET, reg);

				reg = inp32(dprx->hsdt1_crg_base + DPRX0_CLK_GATE_OFFSET); /* 0xEB045000 + 0x10 */
				reg |= DPRX0_APB_CLK_GATE_BIT;
				reg |= DPRX0_EDID_CLK_GATE_BIT;
				reg |= DPRX0_APB_BRIDGE_CLK_GATE_BIT;
				reg |= DPRX0_AUX_CLK_GATE_BIT;
				// reg |= DPRX0_GTC_CLK_GATE_BIT;
				outp32(dprx->hsdt1_crg_base + DPRX0_CLK_GATE_OFFSET, reg);
			} else {
				// dprx1
				reg = inp32(dprx->hsdt1_crg_base + DPRX1_DIS_RESET_OFFSET); /* 0xEB045000 + 0x64 */
				reg |= DPRX1_APB_DIS_RESET_BIT;
				reg |= DPRX1_CTRL_DIS_RESET_BIT;
				reg |= DPRX1_EDID_DIS_RESET_BIT;
				reg |= DPRX1_APB_BRIDGE_DIS_RESET_BIT;
				outp32(dprx->hsdt1_crg_base + DPRX1_DIS_RESET_OFFSET, reg);

				reg = inp32(dprx->hsdt1_crg_base + DPRX1_CLK_GATE_OFFSET); /* 0xEB045000 + 0x10 */
				reg |= DPRX1_APB_CLK_GATE_BIT;
				reg |= DPRX1_EDID_CLK_GATE_BIT;
				reg |= DPRX1_APB_BRIDGE_CLK_GATE_BIT;
				reg |= DPRX1_AUX_CLK_GATE_BIT;
				// reg |= DPRX1_GTC_CLK_GATE_BIT;
				outp32(dprx->hsdt1_crg_base + DPRX1_CLK_GATE_OFFSET, reg);
			}
		} else {
			pr_info("[DPRX] dprx%u reset -\n", dprx->id);
			if (dprx->id == 0) {
				reg = inp32(dprx->hsdt1_crg_base + DPRX0_RESET_OFFSET); /* 0xEB045000 + 0x60 */
				reg |= DPRX0_APB_RESET_BIT;
				reg |= DPRX0_CTRL_RESET_BIT;
				reg |= DPRX0_EDID_RESET_BIT;
				reg |= DPRX0_APB_BRIDGE_RESET_BIT;
				outp32(dprx->hsdt1_crg_base + DPRX0_RESET_OFFSET, reg);
			} else {
				// dprx1
				reg = inp32(dprx->hsdt1_crg_base + DPRX1_RESET_OFFSET);
				reg |= DPRX1_APB_RESET_BIT;
				reg |= DPRX1_CTRL_RESET_BIT;
				reg |= DPRX1_EDID_RESET_BIT;
				reg |= DPRX1_APB_BRIDGE_RESET_BIT;
				outp32(dprx->hsdt1_crg_base + DPRX1_RESET_OFFSET, reg);
			}
		}
	}
}

/**
 * reset all inner module
 */
void dprx_soft_reset_all(struct dprx_ctrl *dprx)
{
	uint32_t phyifctrl;

	dpu_check_and_no_retval((dprx == NULL), err, "dprx is NULL!\n");

	pr_info("[DPRX] dprx_soft_reset_all +\n");
	pr_info("[DPRX] dprx base = 0x%x \n", dprx->base);
	phyifctrl = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_RESET_CTRL);
	phyifctrl |= DPRX_CORE_RESET_BIT;
	phyifctrl |= DPRX_LANE_RESET_BIT;
	phyifctrl |= DPRX_AUX_RESET_BIT;
	/* for edp mode */
	phyifctrl |= DPRX_PSR_RESET_BIT; // for test
	phyifctrl |= DPRX_ATC_RESET_BIT;
	phyifctrl |= DPRX_PHY_CTRL_RESET_BIT;
	phyifctrl |= DPRX_RX2TX_RESET_BIT;
	phyifctrl |= DPRX_AUDIO_LINK_RESET_BIT;
	dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_RESET_CTRL, phyifctrl);
	pr_info("[DPRX] dprx_soft_reset_all -\n");
}

/**
 * enable inner clk gate
 */
void dprx_inner_clk_enable(struct dprx_ctrl *dprx)
{
	uint32_t phyifctrl;

	dpu_check_and_no_retval((dprx == NULL), err, "dprx is NULL!\n");

	pr_info("[DPRX] dprx_inner_clk_enable +\n");
	phyifctrl = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_CLK_CTRL);
	phyifctrl |= DPRX_AUX_CLK_EN_BIT;
	phyifctrl |= DPRX_DSS_CORE_CLK_EN_BIT;
	/* for edp mode */
	phyifctrl |= DPRX_PSR_CLK_EN_BIT; // for test
	phyifctrl |= DPRX_RX2TX_CLK_EN_BIT;
	dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_CLK_CTRL, phyifctrl);
	pr_info("[DPRX] dprx_inner_clk_enable -\n");
}

/**
 * 1 byte for 1 dpcd, while apb write/read with 32bit
 * Ex: We need write dpcd 0x101, then we need write to DPCD_103_100's bit15-8
 */
uint8_t dprx_read_dpcd(struct dprx_ctrl *dprx, uint32_t addr)
{
	uint32_t start_addr;
	uint32_t offset;
	uint32_t reg;
	uint8_t val;

	start_addr = (addr / DPCD_WR_BYTE_LEN) * DPCD_WR_BYTE_LEN;
	offset = addr % DPCD_WR_BYTE_LEN;
	reg = dprx_readl(dprx, start_addr);
	reg &= GENMASK(offset * 8 + 7, offset * 8);
	val = reg >> (offset * 8);

	return val;
}

void dprx_write_dpcd(struct dprx_ctrl *dprx, uint32_t addr, uint8_t val)
{
	uint32_t start_addr;
	uint32_t offset;
	uint32_t reg;

	start_addr = (addr / DPCD_WR_BYTE_LEN) * DPCD_WR_BYTE_LEN;
	offset = addr % DPCD_WR_BYTE_LEN;
	reg = dprx_readl(dprx, start_addr);
	reg &= ~GENMASK(offset * 8 + 7, offset * 8);
	reg |= val << (offset * 8);
	dprx_writel(dprx, start_addr, reg);
}

void dprx_init_rsv_value(struct dprx_ctrl *dprx)
{
	uint32_t reg;
	/* OUI for test */
	dprx_write_dpcd(dprx, 0x400, 0xa4);
	dprx_write_dpcd(dprx, 0x401, 0xc0);
	dprx_write_dpcd(dprx, 0x402, 0x00);
	dprx_write_dpcd(dprx, 0x403, 0x12);

	/* for dpcd 0x205 */
	reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_SINKSTATUS_CTRL);
	reg |= BIT(0);
	reg |= BIT(1);
	dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_SINKSTATUS_CTRL, reg);

	dprx_write_dpcd(dprx, 0x2204, 0x01);
	dprx_write_dpcd(dprx, 0x220A, 0x06);
}

void dprx_config_dsd_capbility(struct dprx_ctrl *dprx)
{
	/* config DSD capbility */
	dprx_write_dpcd(dprx, 0x60, 0x0); // 0x1 DSD
	dprx->dpcds.rx_caps[0x60] = 0x0; // 0x1 DSD
	dprx_write_dpcd(dprx, 0x61, 0x21);
	dprx->dpcds.rx_caps[0x61] = 0x21;
	dprx_write_dpcd(dprx, 0x62, 0x0);
	dprx->dpcds.rx_caps[0x62] = 0x0;
	dprx_write_dpcd(dprx, 0x63, 0x20);
	dprx->dpcds.rx_caps[0x63] = 0x20;

	dprx_write_dpcd(dprx, 0x64, 0xB);
	dprx->dpcds.rx_caps[0x64] = 0xB;
	dprx_write_dpcd(dprx, 0x65, 0x2);
	dprx->dpcds.rx_caps[0x65] = 0x2;
	dprx_write_dpcd(dprx, 0x66, 0x1);
	dprx->dpcds.rx_caps[0x66] = 0x1;
	dprx_write_dpcd(dprx, 0x67, 0xD0);
	dprx->dpcds.rx_caps[0x67] = 0xD0;

	dprx_write_dpcd(dprx, 0x68, 0x1);
	dprx->dpcds.rx_caps[0x68] = 0x1;
	dprx_write_dpcd(dprx, 0x69, 0x1B);
	dprx->dpcds.rx_caps[0x69] = 0x1B;
	dprx_write_dpcd(dprx, 0x6A, 0x6);
	dprx->dpcds.rx_caps[0x6A] = 0x6;
	dprx_write_dpcd(dprx, 0x6B, 0x11);
	dprx->dpcds.rx_caps[0x6B] = 0x11;

	dprx_write_dpcd(dprx, 0x6C, 0xC);
	dprx->dpcds.rx_caps[0x6C] = 0xC;
	dprx_write_dpcd(dprx, 0x6D, 0x0);
	dprx->dpcds.rx_caps[0x6D] = 0x0;
	dprx_write_dpcd(dprx, 0x6E, 0x0);
	dprx->dpcds.rx_caps[0x6E] = 0x0;
	dprx_write_dpcd(dprx, 0x6F, 0x4);
	dprx->dpcds.rx_caps[0x6F] = 0x4;
}

void dprx_init_dpcd(struct dprx_ctrl *dprx)
{
	uint32_t reg;
	uint32_t dpcd_addr;

	reg = dprx_readl(dprx, 0x100);
	reg = 0x40A;
	dprx_writel(dprx, 0x100, reg);
	dprx->dpcds.link_cfgs[2] = 0x0;

	dprx_write_dpcd(dprx, 0x0, 0x12);
	dprx->dpcds.rx_caps[0x0] = 0x12;
	dprx_write_dpcd(dprx, 0x1, 0xA);
	dprx->dpcds.rx_caps[0x1] = 0xA;
	dprx_write_dpcd(dprx, 0x2, 0x84);
	dprx->dpcds.rx_caps[0x2] = 0x84;
	dprx_write_dpcd(dprx, 0x3, 0x0);
	dprx->dpcds.rx_caps[0x3] = 0x0;

	dprx_write_dpcd(dprx, 0x4, 0x1);
	dprx->dpcds.rx_caps[0x4] = 0x1;
	dprx_write_dpcd(dprx, 0x5, 0x0);
	dprx->dpcds.rx_caps[0x5] = 0x0;
	dprx_write_dpcd(dprx, 0x6, 0x1);
	dprx->dpcds.rx_caps[0x6] = 0x1;
	dprx_write_dpcd(dprx, 0x7, 0xc0);
	dprx->dpcds.rx_caps[0x7] = 0xc0;

	dprx_write_dpcd(dprx, 0x8, 0x2);
	dprx->dpcds.rx_caps[0x8] = 0x2;
	dprx_write_dpcd(dprx, 0x9, 0x0);
	dprx->dpcds.rx_caps[0x9] = 0x0;
	dprx_write_dpcd(dprx, 0xA, 0x6);
	dprx->dpcds.rx_caps[0xA] = 0x6;
	dprx_write_dpcd(dprx, 0xB, 0x0);
	dprx->dpcds.rx_caps[0xB] = 0x0;

	dprx_write_dpcd(dprx, 0xC, 0x0);
	dprx->dpcds.rx_caps[0xC] = 0x0;
	dprx_write_dpcd(dprx, 0xD, 0x0);
	dprx->dpcds.rx_caps[0xD] = 0x0;
	dprx_write_dpcd(dprx, 0xE, 0x84);
	dprx->dpcds.rx_caps[0xE] = 0x84;
	dprx_write_dpcd(dprx, 0xF, 0x0);
	dprx->dpcds.rx_caps[0xF] = 0x0;

	/* config PSR or PSR2 */
	reg = dprx_read_dpcd(dprx, 0x70);
	reg &= ~BIT(1);
	reg &= ~BIT(0);
	dprx_write_dpcd(dprx, 0x70, reg);
	dprx->dpcds.rx_caps[0x70] = 0x0;
	dprx_write_dpcd(dprx, 0x71, reg);
	dprx->dpcds.rx_caps[0x71] = 0x0;
	dprx_write_dpcd(dprx, 0x72, reg);
	dprx->dpcds.rx_caps[0x72] = 0x0;
	dprx_write_dpcd(dprx, 0x73, reg);
	dprx->dpcds.rx_caps[0x73] = 0x0;

	dprx_config_dsd_capbility(dprx);

	dprx_write_dpcd(dprx, 0x70, 0x3);
	dprx->dpcds.rx_caps[0x70] = 0x3;
	dprx_write_dpcd(dprx, 0x71, 0x71);
	dprx->dpcds.rx_caps[0x71] = 0x71;
	dprx_write_dpcd(dprx, 0x72, 0x0);
	dprx->dpcds.rx_caps[0x72] = 0x0;
	dprx_write_dpcd(dprx, 0x73, 0x1);
	dprx->dpcds.rx_caps[0x73] = 0x1;

	/* fec capability */
	if (dprx->mode == DP_MODE) {
		dprx_write_dpcd(dprx, 0x90, 0xBF);
		dprx->dpcds.rx_caps[0x90] = 0xBF;
	}

	dprx_write_dpcd(dprx, 0x200, 0x1);
	dprx_write_dpcd(dprx, 0x2002, 0x1);

	dprx_write_dpcd(dprx, 0x2201, 0xA);
	dprx->dpcds.extended_rx_caps[1] = 0xA;
	/* for test FPGA RX phy: not support SSC */
	dprx_write_dpcd(dprx, 0x2203, 0x80);
	dprx->dpcds.extended_rx_caps[3] = 0x80;
	/* init dpcp for cts */
	dprx_init_rsv_value(dprx);

	/* for test */
	for (dpcd_addr = 0; dpcd_addr < 0x100; dpcd_addr++)
		dprx->dpcds.rx_caps[dpcd_addr] = dprx_read_dpcd(dprx, dpcd_addr);
	for (dpcd_addr = 0x100; dpcd_addr < 0x200; dpcd_addr++)
		dprx->dpcds.link_cfgs[dpcd_addr - 0x100] = dprx_read_dpcd(dprx, dpcd_addr);
	for (dpcd_addr = 0x700; dpcd_addr < 0x800; dpcd_addr++)
		dprx->dpcds.edp_spec[dpcd_addr - 0x700] = dprx_read_dpcd(dprx, dpcd_addr);
	for (dpcd_addr = 0x2000; dpcd_addr < 0x2200; dpcd_addr++)
		dprx->dpcds.event_status_indicator[dpcd_addr - 0x2000] = dprx_read_dpcd(dprx, dpcd_addr);
	for (dpcd_addr = 0x2200; dpcd_addr < 0x2300; dpcd_addr++)
		dprx->dpcds.extended_rx_caps[dpcd_addr - 0x2200] = dprx_read_dpcd(dprx, dpcd_addr);
	dprx->dpcds.power_state = dprx_read_dpcd(dprx, DPRX_POWER_CTRL);
}

void dprx_edid_init_config(struct dprx_ctrl *dprx)
{
	uint32_t phyifctrl;
	uint32_t reg;
	uint32_t tmp;
	uint16_t i;
	uint16_t j;
	uint8_t *block = NULL;

	dpu_check_and_no_retval((dprx == NULL), err, "dprx is NULL!\n");

	pr_info("[DPRX] dprx_edid_init_config +\n");

	phyifctrl = inp32(dprx->hsdt1_sys_ctrl_base + HSDT1_EDID_BASE_ADDR_CONFIG); /* 0xEB040000 + 0x508 */
	if (dprx->id == 0) {
		phyifctrl &= ~DPRX0_EDID_BASE_ADDR_CONFIG;
		phyifctrl |= (DPRX0_EDID_BASE_ADDR << DPRX0_EDID_BASE_ADDR_CONFIG_OFFSET);
	} else if (dprx->id == 1) {
		phyifctrl &= ~DPRX1_EDID_BASE_ADDR_CONFIG;
		phyifctrl |= (DPRX1_EDID_BASE_ADDR << DPRX1_EDID_BASE_ADDR_CONFIG_OFFSET);
	}
	outp32(dprx->hsdt1_sys_ctrl_base + HSDT1_EDID_BASE_ADDR_CONFIG, phyifctrl);

	for (i = 0; i < dprx->edid.valid_block_num; i++) {
		block = dprx->edid.block[i].payload;
		for (j = 0; j < EDID_BLOCK_SIZE; j += 4) {
			reg = 0;
			tmp = block[j + 3];
			reg |= tmp << 24;
			tmp = block[j + 2];
			reg |= tmp << 16;
			tmp = block[j + 1];
			reg |= tmp << 8;
			reg |= block[j];
			outp32(dprx->edid_base + i * EDID_BLOCK_SIZE + j, reg);
		}
	}

	pr_info("[DPRX] dprx_edid_init_config -\n");
}

static void dprx_global_intr_dis(struct dprx_ctrl *dprx)
{
	dprx_writel(dprx, DPRX_SCTRL_OFFSET + SDP_RPT_EN, 0);

	dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_INTR_ENABLE, 0);
	dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_TIMING_INTR_ENABLE, 0);
	dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_TRAINING_INTR_ENABLE, 0);
	dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_PSR_INTR_ENABLE, 0);
	dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_SDP_VIDEOFORMAT_INTR_ENABLE, 0);
}

void dprx_fpga_test_all_irq(struct dprx_ctrl *dprx)
{
	uint32_t irq_ien;
	irq_ien = DPRX_IRQ_VSYNC | DPRX_IRQ_VBS | DPRX_IRQ_VSTART | DPRX_IRQ_VFP | DPRX_IRQ_VACTIVE1 | DPRX_IRQ_TIME_COUNT;
	pr_info("[DPRX] 0x%x\n", irq_ien);
	dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_TIMING_INTR_ENABLE, irq_ien);
	/* for test final del: test linkconfig irq */
	irq_ien = GENMASK(17, 0);
	dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB18, irq_ien); // LINKCFG INT

	irq_ien = GENMASK(13, 0);
	dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB24, irq_ien); // GTC INT

	irq_ien = GENMASK(1, 0);
	dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB54, irq_ien); // error INT

	irq_ien = 0;
	irq_ien = BIT(0) | BIT(1) | BIT(3) | BIT(4);
	dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB60, irq_ien); // debug INT

	irq_ien = GENMASK(28, 0);
	dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB6C, irq_ien); // rsv INT

	irq_ien = GENMASK(23, 0);
	dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB78, irq_ien); // mon

	irq_ien = GENMASK(23, 0);
	dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB84, irq_ien); // mon en

	irq_ien = GENMASK(10, 0);
	dprx_writel(dprx, DPRX_SCTRL_OFFSET + 0xB88, irq_ien); // mon type
}

static void dprx_global_intr_en(struct dprx_ctrl *dprx)
{
	uint32_t reg;
	uint32_t irq_ien;

	/* enable sdp types */
	irq_ien = DPRX_ALL_SDP_EN;
	dprx_writel(dprx, DPRX_SCTRL_OFFSET + SDP_RPT_EN, irq_ien);
	reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + SDP_FIFO_CLR);
	reg |= SDP_REPORT_TIMER_EN;
	/* SDP_REPORT_TIMEOUT_VALUE default is 17ms */
	dprx_writel(dprx, DPRX_SCTRL_OFFSET + SDP_FIFO_CLR, reg);

	/* enable first-level interrupts */
	irq_ien = DPRX_ALL_INTR_ENABLE;
	dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_INTR_ENABLE, irq_ien);

	/* enable second-level interrupts: timing related irq */
	irq_ien = DPRX_IRQ_VSYNC | DPRX_IRQ_VBS | DPRX_IRQ_VSTART | DPRX_IRQ_VFP;
	dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_TIMING_INTR_ENABLE, irq_ien);

	/* enable second-level interrupts: training related irq */
	irq_ien = DPRX_ALL_TRAINING_INTR;
	dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_TRAINING_INTR_ENABLE, irq_ien);

	irq_ien = DPRX_ALL_PSR_INTR;
	dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_PSR_INTR_ENABLE, irq_ien);

	/* enable second-level interrupts: sdp/format related irq */
	irq_ien = DPRX_ALL_SDP_VIDEOFORMAT_INTR;
	dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_SDP_VIDEOFORMAT_INTR_ENABLE, irq_ien);
}

static void dprx_link_training_init_config(struct dprx_ctrl *dprx)
{
	uint32_t reg;

	reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_ATC_CTRL0);
	reg &= ~DPRX_ATC_MODE;
	reg |= DPRX_ATC_WDT_EN;
	reg |= DPRX_ATC_EN;
	reg |= DPRX_ATC_LT_RST_GEN_EN;
	dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_ATC_CTRL0, reg);

	reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_ATC_CTRL1);
	reg &= ~DPRX_FULL_EQ_LOOP_CNT;
	reg |= (0x3F << DPRX_FULL_EQ_LOOP_CNT_OFFSET); /* for test: 0x3F = (400us * 0.8 / 5) - 1 */
	reg &= ~DPRX_FAST_EQ_LOOP_CNT;
	reg |= 0xF;
	dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_ATC_CTRL1, reg);

	reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_HS_DET_CTRL0);
	reg &= ~DPRX_HS_DET_SYM_LEN;
	reg |= (0x20 << DPRX_HS_DET_SYM_LEN_OFFSET);
	dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_HS_DET_CTRL0, reg);

	reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_HS_DET_CTRL1);
	reg &= ~DPRX_HS_DET_DLY_TIME;
	reg |= 0x2;
	dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_HS_DET_CTRL1, reg);

	reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_ALPM_ENABLE);
	reg |= DPRX_HW_ALPM_EN;
	dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_ALPM_ENABLE, reg);
}

static void dprx_phy_init_config(struct dprx_ctrl *dprx)
{
	uint32_t reg;

	if (dprx->fpga_flag== true) {
		reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_PHY_RST_CTRL);
		reg |= DPRX_PHY_RESET;
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_PHY_RST_CTRL, reg);
		reg = 0;
		reg |= BIT(0);
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_POWER_CTRL, reg);
		pr_info("[DPRX] hardware phy rst, reg = %x\n", reg);

		reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_PHY_RST_CTRL);
		reg |= DPRX_PHY_LANE_RESET;
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_PHY_RST_CTRL, reg);
		pr_info("[DPRX] dprx_phy_init_config, reg = %x\n", reg);

		reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_PHY_RATE_CTRL);
		reg |= 1; // for test, HBR
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_PHY_RATE_CTRL, reg);
		udelay(2);

		reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_PHY_RST_CTRL);
		reg &= ~DPRX_PHY_LANE_POWER_CTRL_SEL;
		reg |= (0xF << DPRX_PHY_LANE_POWER_CTRL_SEL_OFFSET);
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_PHY_RST_CTRL, reg);
		reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_PHY_RWR_CTRL);
		reg &= ~GENMASK(15, 0); // for test, P0
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_PHY_RWR_CTRL, reg);
		reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_PHY_RST_CTRL);
		reg &= ~DPRX_PHY_LANE_POWER_CTRL_SEL;
		reg |= (0x0 << DPRX_PHY_LANE_POWER_CTRL_SEL_OFFSET);
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_PHY_RST_CTRL, reg);

		reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_PHY_RST_CTRL);
		reg |= DPRX_PHY_READY;
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_PHY_RST_CTRL, reg);

		/* for new logic */
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_GTC_CTRL, 0x5000b);

		reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_PCS_CTRL0);
		reg &= ~DPRX_ICFG_BS_LOCK_NUM_OFFSET;
		reg |= (0x3 << DPRX_ICFG_BS_LOCK_NUM_OFFSET);
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_PCS_CTRL0, reg);

		reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_PCS_CTRL1);
		reg &= ~DPRX_TRAIN_LOST_SYM_ERR_MASK;
		reg |= (0x40 << DPRX_TRAIN_LOST_SYM_ERR_OFFSET);
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_PCS_CTRL1, reg);

		reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_PCS_CTRL3);
		reg &= ~DPRX_ICFG_DESKEW_TIME_MASK;
		reg |= 0x40;
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_PCS_CTRL3, reg);

		reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_GTC_CTRL5);
		reg |= ICFG_TX_GTC_MISS_EN;
		dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_GTC_CTRL5, reg);
	}
}

static void dprx_set_hpd(struct dprx_ctrl *dprx, bool enable)
{
	uint32_t reg;

	reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_HPD_CTRL0);
	reg |= DPRX_HPD_EN;
	dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_HPD_CTRL0, reg);

	reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_HS_DET_CTRL0);
	reg |= DPRX_HS_DET_EN;
	dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_HS_DET_CTRL0, reg);

	/* for test: set rate/lane num before fast link training */
	reg = dprx_readl(dprx, 0x100);
	reg = 0x40A;
	dprx_writel(dprx, 0x100, reg);

	/* for test */
	reg = inp32(dprx->hsdt1_sys_ctrl_base + DPRX_MUX_SEL); /* 0xEB040000 + 0x30 */
	reg &= ~DPRX_DPTX_SEL;
	if (dprx->id == 0) { // dprx0->dsd0
		reg &= ~DPRX_DSS0_SEL;
		reg |= (1 << DPRX0_DSS0_SEL_OFFSET);
		reg |= (1 << DPRX0_DPTX_SEL_OFFSET);
	} else { // dprx1->dsd1
		reg &= ~DPRX_DSS1_SEL;
		reg |= (1 << DPRX1_DSS1_SEL_OFFSET);
		reg |= (1 << DPRX1_DPTX_SEL_OFFSET);
	}
	outp32(dprx->hsdt1_sys_ctrl_base + DPRX_MUX_SEL, reg);
}

void dprx_fec_init_config(struct dprx_ctrl *dprx)
{
	/* 1、Deassert Reset；2、Open the control； 3、Enable */
	uint32_t reg;
	reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET);
	reg |= DPRX_FEC_RESET_BIT;
	dprx_writel(dprx, DPRX_SCTRL_OFFSET, reg);

	reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_CLK_CTRL);
	reg |= DPRX_FEC_CLK_BIT;
	reg |= DPRX_FEC_EUC_EN_BIT;
	reg |= DPRX_FEC_SEARCH_EN_BIT;
	dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_CLK_CTRL, reg);

	reg = dprx_readl(dprx, DPRX_SCTRL_OFFSET + DPRX_FEC_CTRL);
	reg &= ~DPRX_FEC_BYPASS_BIT;
	reg |= DPRX_FEC_EN_BIT;
	dprx_writel(dprx, DPRX_SCTRL_OFFSET + DPRX_FEC_CTRL, reg);
}

void dprx_core_on(struct dprx_ctrl *dprx)
{
	dpu_check_and_no_retval((dprx == NULL), err, "dprx is NULL!\n");

	pr_info("[DPRX] dprx_core_on +\n");

	dprx_soft_reset_all(dprx);
	dprx_inner_clk_enable(dprx);

	dprx_init_dpcd(dprx);
	dprx_edid_init_config(dprx);

	dprx_global_intr_dis(dprx);
	enable_irq(dprx->irq);
	/* Enable all top-level interrupts */
	dprx_global_intr_en(dprx);

	dprx_link_training_init_config(dprx); // support FULL/ Fast LK

	dprx_phy_init_config(dprx);
	/* fec config */
	if (dprx->mode == DP_MODE)
		dprx_fec_init_config(dprx);

	dprx_set_hpd(dprx, true);

	pr_info("[DPRX] dprx_core_on -\n");
}

void dprx_core_off(struct dprx_ctrl *dprx)
{
	dpu_check_and_no_retval((dprx == NULL), err, "dprx is NULL!\n");

	dprx_set_hpd(dprx, false);

	dprx_global_intr_dis(dprx);

	disable_irq(dprx->irq);

	dprx_soft_reset_all(dprx);
}
