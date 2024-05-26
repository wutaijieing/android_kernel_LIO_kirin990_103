/*
 * hisi_ddr_ddrcflux_registers_chamaeleon.h
 *
 * ddrcflux registers message for chamaeleon
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
#ifndef __HISI_DDR_DDRCFLUX_REGISTERS_CHAMAELEON_H__
#define __HISI_DDR_DDRCFLUX_REGISTERS_CHAMAELEON_H__

#define PLATFORM_LIST \
	{ .compatible = "hisilicon,chamaeleon-ddrc-flux", }, \
	{ },

#define DDRC_PLATFORM_NAME		CHAMAELEON
#define DDRC_IRQ_CPU_CORE_NUM		8
#define DDRC_DMC_NUM		4
#define DDRC_ASI_PORT_NUM		0
#define DDRC_QOSB_NUM		0

#define DDRC_4CH_PLATFORM
#define DDR_QOSB

#define ddr_reg_dmc_addr(m, n) DDR_REG_DMC(((m) << 1) + (n))

/* timer5 control info */
#define TIMER_SCTRL_SCPEREN1_GT_CLOCK \
	BIT(SOC_SCTRL_SCPEREN1_gt_pclk_timer5_START)
#define TIMER_CLK_CTRL 0x00
#define TIMER_LOAD_OFFSET	0x04
#define TIMER_VALUE_OFFSET	0x14
#define TIMER_CTRL_OFFSET	0x08
#define TIMER_INTCLR_OFFSET	0x10
#define TIMER_RIS_OFFSET	0x0C
#define TIMER_BGLOAD_OFFSET	BIT(3)
#define TIMER_CTRL_32BIT	BIT(1)
#define TIMER_CTRL_IE	BIT(4)
#define TIMER_CTRL_PERIODIC	BIT(2)
#define TIMER_CTRL_ENABLE	BIT(0)
#define TIMER_SEL_REFH 0x2
#define BW_TIMER_DEFAULT_LOAD	0xFFFFFFFF
#define MAX_DATA_OF_32BIT	0xFFFFFFFF
#define SOC_DDRC_DMC_DDRC_CFG_STAID_ADDR	SOC_DDRC_DMC_DDRC_CFG_STAID0_ADDR
#define SOC_DDRC_DMC_DDRC_CFG_STAIDMSK_ADDR	SOC_DDRC_DMC_DDRC_CFG_STAIDMSK0_ADDR


#define DDR_FLUX_DMSS_ASI_DFX_OSTD

#define DDR_FLUX_DMSS_ASI_STAT_FLUX_WR

#define DDR_FLUX_DMSS_ASI_STAT_FLUX_RD

#define DDR_FLUX_DMC \
	ddr_flux(DMC00, 0x948, 1 * 4, "DDRC_HIS_FLUX_WR_00,"), \
	ddr_flux(DMC00, 0x94C, 1 * 4, "DDRC_HIS_FLUX_RD_00,"), \
	ddr_flux(DMC00, 0x950, 1 * 4, "DDRC_HIS_FLUX_WCMD_00,"), \
	ddr_flux(DMC00, 0x954, 1 * 4, "DDRC_HIS_FLUX_RCMD_00,"), \
	ddr_flux(DMC00, 0x958, 1 * 4, "DDRC_HIS_FLUXID_WR_00,"), \
	ddr_flux(DMC00, 0x95C, 1 * 4, "DDRC_HIS_FLUXID_RD_00,"), \
	ddr_flux(DMC00, 0x960, 1 * 4, "DDRC_HIS_FLUXID_WCMD_00,"), \
	ddr_flux(DMC00, 0x964, 1 * 4, "DDRC_HIS_FLUXID_RCMD_00,"), \
	ddr_flux(DMC00, 0x968, 1 * 4, "DDRC_HIS_WLATCNT0_00,"), \
	ddr_flux(DMC00, 0x96C, 1 * 4, "DDRC_HIS_WLATCNT1_00,"), \
	ddr_flux(DMC00, 0x970, 1 * 4, "DDRC_HIS_RLATCNT0_00,"), \
	ddr_flux(DMC00, 0x974, 1 * 4, "DDRC_HIS_RLATCNT1_00,"), \
	ddr_flux(DMC00, 0x978, 1 * 4, "DDRC_HIS_INHERE_RLAT_CNT_00,"), \
	ddr_flux(DMC00, 0x97C, 1 * 4, "DDRC_HIS_CMD_SUM_00,"), \
	ddr_flux(DMC00, 0x984, 1 * 4, "DDRC_HIS_PRE_CMD_00,"), \
	ddr_flux(DMC00, 0x988, 1 * 4, "DDRC_HIS_ACT_CMD_00,"), \
	ddr_flux(DMC00, 0x98C, 1 * 4, "DDRC_HIS_BNK_CHG_00,"), \
	ddr_flux(DMC00, 0x990, 1 * 4, "DDRC_HIS_RNK_CHG_00,"), \
	ddr_flux(DMC00, 0x994, 1 * 4, "DDRC_HIS_RW_CHG_00,"), \
	ddr_flux(DMC01, 0x948, 1 * 4, "DDRC_HIS_FLUX_WR_01,"), \
	ddr_flux(DMC01, 0x94C, 1 * 4, "DDRC_HIS_FLUX_RD_01,"), \
	ddr_flux(DMC01, 0x950, 1 * 4, "DDRC_HIS_FLUX_WCMD_01,"), \
	ddr_flux(DMC01, 0x954, 1 * 4, "DDRC_HIS_FLUX_RCMD_01,"), \
	ddr_flux(DMC01, 0x958, 1 * 4, "DDRC_HIS_FLUXID_WR_01,"), \
	ddr_flux(DMC01, 0x95C, 1 * 4, "DDRC_HIS_FLUXID_RD_01,"), \
	ddr_flux(DMC01, 0x960, 1 * 4, "DDRC_HIS_FLUXID_WCMD_01,"), \
	ddr_flux(DMC01, 0x964, 1 * 4, "DDRC_HIS_FLUXID_RCMD_01,"), \
	ddr_flux(DMC01, 0x968, 1 * 4, "DDRC_HIS_WLATCNT0_01,"), \
	ddr_flux(DMC01, 0x96C, 1 * 4, "DDRC_HIS_WLATCNT1_01,"), \
	ddr_flux(DMC01, 0x970, 1 * 4, "DDRC_HIS_RLATCNT0_01,"), \
	ddr_flux(DMC01, 0x974, 1 * 4, "DDRC_HIS_RLATCNT1_01,"), \
	ddr_flux(DMC01, 0x978, 1 * 4, "DDRC_HIS_INHERE_RLAT_CNT_01,"), \
	ddr_flux(DMC01, 0x97C, 1 * 4, "DDRC_HIS_CMD_SUM_01,"), \
	ddr_flux(DMC01, 0x984, 1 * 4, "DDRC_HIS_PRE_CMD_01,"), \
	ddr_flux(DMC01, 0x988, 1 * 4, "DDRC_HIS_ACT_CMD_01,"), \
	ddr_flux(DMC01, 0x98C, 1 * 4, "DDRC_HIS_BNK_CHG_01,"), \
	ddr_flux(DMC01, 0x990, 1 * 4, "DDRC_HIS_RNK_CHG_01,"), \
	ddr_flux(DMC01, 0x994, 1 * 4, "DDRC_HIS_RW_CHG_01,"), \
	ddr_flux(DMC10, 0x948, 1 * 4, "DDRC_HIS_FLUX_WR_10,"), \
	ddr_flux(DMC10, 0x94C, 1 * 4, "DDRC_HIS_FLUX_RD_10,"), \
	ddr_flux(DMC10, 0x950, 1 * 4, "DDRC_HIS_FLUX_WCMD_10,"), \
	ddr_flux(DMC10, 0x954, 1 * 4, "DDRC_HIS_FLUX_RCMD_10,"), \
	ddr_flux(DMC10, 0x958, 1 * 4, "DDRC_HIS_FLUXID_WR_10,"), \
	ddr_flux(DMC10, 0x95C, 1 * 4, "DDRC_HIS_FLUXID_RD_10,"), \
	ddr_flux(DMC10, 0x960, 1 * 4, "DDRC_HIS_FLUXID_WCMD_10,"), \
	ddr_flux(DMC10, 0x964, 1 * 4, "DDRC_HIS_FLUXID_RCMD_10,"), \
	ddr_flux(DMC10, 0x968, 1 * 4, "DDRC_HIS_WLATCNT0_10,"), \
	ddr_flux(DMC10, 0x96C, 1 * 4, "DDRC_HIS_WLATCNT1_10,"), \
	ddr_flux(DMC10, 0x970, 1 * 4, "DDRC_HIS_RLATCNT0_10,"), \
	ddr_flux(DMC10, 0x974, 1 * 4, "DDRC_HIS_RLATCNT1_10,"), \
	ddr_flux(DMC10, 0x978, 1 * 4, "DDRC_HIS_INHERE_RLAT_CNT_10,"), \
	ddr_flux(DMC10, 0x97C, 1 * 4, "DDRC_HIS_CMD_SUM_10,"), \
	ddr_flux(DMC10, 0x984, 1 * 4, "DDRC_HIS_PRE_CMD_10,"), \
	ddr_flux(DMC10, 0x988, 1 * 4, "DDRC_HIS_ACT_CMD_10,"), \
	ddr_flux(DMC10, 0x98C, 1 * 4, "DDRC_HIS_BNK_CHG_10,"), \
	ddr_flux(DMC10, 0x990, 1 * 4, "DDRC_HIS_RNK_CHG_10,"), \
	ddr_flux(DMC10, 0x994, 1 * 4, "DDRC_HIS_RW_CHG_10,"), \
	ddr_flux(DMC11, 0x948, 1 * 4, "DDRC_HIS_FLUX_WR_11,"), \
	ddr_flux(DMC11, 0x94C, 1 * 4, "DDRC_HIS_FLUX_RD_11,"), \
	ddr_flux(DMC11, 0x950, 1 * 4, "DDRC_HIS_FLUX_WCMD_11,"), \
	ddr_flux(DMC11, 0x954, 1 * 4, "DDRC_HIS_FLUX_RCMD_11,"), \
	ddr_flux(DMC11, 0x958, 1 * 4, "DDRC_HIS_FLUXID_WR_11,"), \
	ddr_flux(DMC11, 0x95C, 1 * 4, "DDRC_HIS_FLUXID_RD_11,"), \
	ddr_flux(DMC11, 0x960, 1 * 4, "DDRC_HIS_FLUXID_WCMD_11,"), \
	ddr_flux(DMC11, 0x964, 1 * 4, "DDRC_HIS_FLUXID_RCMD_11,"), \
	ddr_flux(DMC11, 0x968, 1 * 4, "DDRC_HIS_WLATCNT0_11,"), \
	ddr_flux(DMC11, 0x96C, 1 * 4, "DDRC_HIS_WLATCNT1_11,"), \
	ddr_flux(DMC11, 0x970, 1 * 4, "DDRC_HIS_RLATCNT0_11,"), \
	ddr_flux(DMC11, 0x974, 1 * 4, "DDRC_HIS_RLATCNT1_11,"), \
	ddr_flux(DMC11, 0x978, 1 * 4, "DDRC_HIS_INHERE_RLAT_CNT_11,"), \
	ddr_flux(DMC11, 0x97C, 1 * 4, "DDRC_HIS_CMD_SUM_11,"), \
	ddr_flux(DMC11, 0x984, 1 * 4, "DDRC_HIS_PRE_CMD_11,"), \
	ddr_flux(DMC11, 0x988, 1 * 4, "DDRC_HIS_ACT_CMD_11,"), \
	ddr_flux(DMC11, 0x98C, 1 * 4, "DDRC_HIS_BNK_CHG_11,"), \
	ddr_flux(DMC11, 0x990, 1 * 4, "DDRC_HIS_RNK_CHG_11,"), \
	ddr_flux(DMC11, 0x994, 1 * 4, "DDRC_HIS_RW_CHG_11\n"),

#endif
