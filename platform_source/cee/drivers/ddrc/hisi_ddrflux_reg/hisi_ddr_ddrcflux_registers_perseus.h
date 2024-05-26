/*
 * hisi_ddr_ddrcflux_registers_perseus.h
 *
 * ddrcflux registers message for perseus
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
#ifndef __HISI_DDR_DDRCFLUX_REGISTERS_PERSEUS_H__
#define __HISI_DDR_DDRCFLUX_REGISTERS_PERSEUS_H__

#define PLATFORM_LIST \
	{ .compatible = "hisilicon,perseus-ddrc-flux", }, \
	{ },

#define DDRC_PLATFORM_NAME		PERSEUS
#define DDRC_IRQ_CPU_CORE_NUM		8
#define DDRC_DMC_NUM		4
#define DDRC_ASI_PORT_NUM		14
#define DDRC_QOSB_NUM		0

#define DDRC_4CH_PLATFORM
#define DDR_QOSB

#define ddr_reg_dmc_addr(m, n) DDR_REG_DMC(m, n)

/* timer5 control info */
#define TIMER_SCTRL_SCPEREN1_GT_CLOCK \
	(BIT(SOC_SCTRL_SCPEREN1_gt_pclk_timer5_START) | BIT(SOC_SCTRL_SCPEREN1_gt_clk_timer5_START))
#define TIMER_LOAD_OFFSET	0x20
#define TIMER_VALUE_OFFSET	0x24
#define TIMER_CTRL_OFFSET	0x28
#define TIMER_INTCLR_OFFSET	0x2c
#define TIMER_RIS_OFFSET	0x30
#define TIMER_BGLOAD_OFFSET	0x38
#define TIMER_CTRL_32BIT	BIT(1)
#define TIMER_CTRL_IE	BIT(5)
#define TIMER_CTRL_PERIODIC	BIT(6)
#define TIMER_CTRL_ENABLE	BIT(7)
#define BW_TIMER_DEFAULT_LOAD	0xFFFFFFFF
#define MAX_DATA_OF_32BIT	0xFFFFFFFF

#define DDR_FLUX_DMSS_ASI_DFX_OSTD \
	ddr_flux(DMSS, (0x70c + 0x800 * 0), 1 * 4, "ASI_DFX_OSTD_0,"), \
	ddr_flux(DMSS, (0x70c + 0x800 * 1), 1 * 4, "ASI_DFX_OSTD_1,"), \
	ddr_flux(DMSS, (0x70c + 0x800 * 2), 1 * 4, "ASI_DFX_OSTD_2,"), \
	ddr_flux(DMSS, (0x70c + 0x800 * 3), 1 * 4, "ASI_DFX_OSTD_3,"), \
	ddr_flux(DMSS, (0x70c + 0x800 * 4), 1 * 4, "ASI_DFX_OSTD_4,"), \
	ddr_flux(DMSS, (0x70c + 0x800 * 5), 1 * 4, "ASI_DFX_OSTD_5,"), \
	ddr_flux(DMSS, (0x70c + 0x800 * 6), 1 * 4, "ASI_DFX_OSTD_6,"), \
	ddr_flux(DMSS, (0x70c + 0x800 * 7), 1 * 4, "ASI_DFX_OSTD_7,"), \
	ddr_flux(DMSS, (0x70c + 0x800 * 8), 1 * 4, "ASI_DFX_OSTD_8,"), \
	ddr_flux(DMSS, (0x70c + 0x800 * 9), 1 * 4, "ASI_DFX_OSTD_9,"), \
	ddr_flux(DMSS, (0x70c + 0x800 * 10), 1 * 4, "ASI_DFX_OSTD_10,"), \
	ddr_flux(DMSS, (0x70c + 0x800 * 11), 1 * 4, "ASI_DFX_OSTD_11,"), \
	ddr_flux(DMSS, (0x70c + 0x800 * 12), 1 * 4, "ASI_DFX_OSTD_12,"), \
	ddr_flux(DMSS, (0x70c + 0x800 * 13), 1 * 4, "ASI_DFX_OSTD_13,"),

#define DDR_FLUX_DMSS_ASI_STAT_FLUX_WR \
	ddr_flux(DMSS, (0x218 + 0x800 * 0), 1 * 4, "ASI_STAT_FLUX_WR_0,"), \
	ddr_flux(DMSS, (0x218 + 0x800 * 1), 1 * 4, "ASI_STAT_FLUX_WR_1,"), \
	ddr_flux(DMSS, (0x218 + 0x800 * 2), 1 * 4, "ASI_STAT_FLUX_WR_2,"), \
	ddr_flux(DMSS, (0x218 + 0x800 * 3), 1 * 4, "ASI_STAT_FLUX_WR_3,"), \
	ddr_flux(DMSS, (0x218 + 0x800 * 4), 1 * 4, "ASI_STAT_FLUX_WR_4,"), \
	ddr_flux(DMSS, (0x218 + 0x800 * 5), 1 * 4, "ASI_STAT_FLUX_WR_5,"), \
	ddr_flux(DMSS, (0x218 + 0x800 * 6), 1 * 4, "ASI_STAT_FLUX_WR_6,"), \
	ddr_flux(DMSS, (0x218 + 0x800 * 7), 1 * 4, "ASI_STAT_FLUX_WR_7,"), \
	ddr_flux(DMSS, (0x218 + 0x800 * 8), 1 * 4, "ASI_STAT_FLUX_WR_8,"), \
	ddr_flux(DMSS, (0x218 + 0x800 * 9), 1 * 4, "ASI_STAT_FLUX_WR_9,"), \
	ddr_flux(DMSS, (0x218 + 0x800 * 10), 1 * 4, "ASI_STAT_FLUX_WR_10,"), \
	ddr_flux(DMSS, (0x218 + 0x800 * 11), 1 * 4, "ASI_STAT_FLUX_WR_11,"), \
	ddr_flux(DMSS, (0x218 + 0x800 * 12), 1 * 4, "ASI_STAT_FLUX_WR_12,"), \
	ddr_flux(DMSS, (0x218 + 0x800 * 13), 1 * 4, "ASI_STAT_FLUX_WR_13,"),

#define DDR_FLUX_DMSS_ASI_STAT_FLUX_RD \
	ddr_flux(DMSS, (0x21c + 0x800 * 0), 1 * 4, "ASI_STAT_FLUX_RD_0,"), \
	ddr_flux(DMSS, (0x21c + 0x800 * 1), 1 * 4, "ASI_STAT_FLUX_RD_1,"), \
	ddr_flux(DMSS, (0x21c + 0x800 * 2), 1 * 4, "ASI_STAT_FLUX_RD_2,"), \
	ddr_flux(DMSS, (0x21c + 0x800 * 3), 1 * 4, "ASI_STAT_FLUX_RD_3,"), \
	ddr_flux(DMSS, (0x21c + 0x800 * 4), 1 * 4, "ASI_STAT_FLUX_RD_4,"), \
	ddr_flux(DMSS, (0x21c + 0x800 * 5), 1 * 4, "ASI_STAT_FLUX_RD_5,"), \
	ddr_flux(DMSS, (0x21c + 0x800 * 6), 1 * 4, "ASI_STAT_FLUX_RD_6,"), \
	ddr_flux(DMSS, (0x21c + 0x800 * 7), 1 * 4, "ASI_STAT_FLUX_RD_7,"), \
	ddr_flux(DMSS, (0x21c + 0x800 * 8), 1 * 4, "ASI_STAT_FLUX_RD_8,"), \
	ddr_flux(DMSS, (0x21c + 0x800 * 9), 1 * 4, "ASI_STAT_FLUX_RD_9,"), \
	ddr_flux(DMSS, (0x21c + 0x800 * 10), 1 * 4, "ASI_STAT_FLUX_RD_10,"), \
	ddr_flux(DMSS, (0x21c + 0x800 * 11), 1 * 4, "ASI_STAT_FLUX_RD_11,"), \
	ddr_flux(DMSS, (0x21c + 0x800 * 12), 1 * 4, "ASI_STAT_FLUX_RD_12,"), \
	ddr_flux(DMSS, (0x21c + 0x800 * 13), 1 * 4, "ASI_STAT_FLUX_RD_13,"),

#define DDR_FLUX_DMC \
	ddr_flux(DMC00, 0x540, 1 * 4, "DDRC_HIS_FLUX_WR_00,"), \
	ddr_flux(DMC00, 0x544, 1 * 4, "DDRC_HIS_FLUX_RD_00,"), \
	ddr_flux(DMC00, 0x548, 1 * 4, "DDRC_HIS_FLUX_WCMD_00,"), \
	ddr_flux(DMC00, 0x54C, 1 * 4, "DDRC_HIS_FLUX_RCMD_00,"), \
	ddr_flux(DMC00, 0x550, 1 * 4, "DDRC_HIS_FLUXID_WR_00,"), \
	ddr_flux(DMC00, 0x554, 1 * 4, "DDRC_HIS_FLUXID_RD_00,"), \
	ddr_flux(DMC00, 0x558, 1 * 4, "DDRC_HIS_FLUXID_WCMD_00,"), \
	ddr_flux(DMC00, 0x55C, 1 * 4, "DDRC_HIS_FLUXID_RCMD_00,"), \
	ddr_flux(DMC00, 0x560, 1 * 4, "DDRC_HIS_WLATCNT0_00,"), \
	ddr_flux(DMC00, 0x564, 1 * 4, "DDRC_HIS_WLATCNT1_00,"), \
	ddr_flux(DMC00, 0x568, 1 * 4, "DDRC_HIS_RLATCNT0_00,"), \
	ddr_flux(DMC00, 0x56C, 1 * 4, "DDRC_HIS_RLATCNT1_00,"), \
	ddr_flux(DMC00, 0x570, 1 * 4, "DDRC_HIS_INHERE_RLAT_CNT_00,"), \
	ddr_flux(DMC00, 0x574, 1 * 4, "DDRC_HIS_CMD_SUM_00,"), \
	ddr_flux(DMC00, 0x578, 1 * 4, "DDRC_HIS_DAT_SUM_00,"), \
	ddr_flux(DMC00, 0x57C, 1 * 4, "DDRC_HIS_PRE_CMD_00,"), \
	ddr_flux(DMC00, 0x580, 1 * 4, "DDRC_HIS_ACT_CMD_00,"), \
	ddr_flux(DMC00, 0x584, 1 * 4, "DDRC_HIS_BNK_CHG_00,"), \
	ddr_flux(DMC00, 0x588, 1 * 4, "DDRC_HIS_RNK_CHG_00,"), \
	ddr_flux(DMC00, 0x58C, 1 * 4, "DDRC_HIS_RW_CHG_00,"), \
	ddr_flux(DMC01, 0x540, 1 * 4, "DDRC_HIS_FLUX_WR_01,"), \
	ddr_flux(DMC01, 0x544, 1 * 4, "DDRC_HIS_FLUX_RD_01,"), \
	ddr_flux(DMC01, 0x548, 1 * 4, "DDRC_HIS_FLUX_WCMD_01,"), \
	ddr_flux(DMC01, 0x54C, 1 * 4, "DDRC_HIS_FLUX_RCMD_01,"), \
	ddr_flux(DMC01, 0x550, 1 * 4, "DDRC_HIS_FLUXID_WR_01,"), \
	ddr_flux(DMC01, 0x554, 1 * 4, "DDRC_HIS_FLUXID_RD_01,"), \
	ddr_flux(DMC01, 0x558, 1 * 4, "DDRC_HIS_FLUXID_WCMD_01,"), \
	ddr_flux(DMC01, 0x55C, 1 * 4, "DDRC_HIS_FLUXID_RCMD_01,"), \
	ddr_flux(DMC01, 0x560, 1 * 4, "DDRC_HIS_WLATCNT0_01,"), \
	ddr_flux(DMC01, 0x564, 1 * 4, "DDRC_HIS_WLATCNT1_01,"), \
	ddr_flux(DMC01, 0x568, 1 * 4, "DDRC_HIS_RLATCNT0_01,"), \
	ddr_flux(DMC01, 0x56C, 1 * 4, "DDRC_HIS_RLATCNT1_01,"), \
	ddr_flux(DMC01, 0x570, 1 * 4, "DDRC_HIS_INHERE_RLAT_CNT_01,"), \
	ddr_flux(DMC01, 0x574, 1 * 4, "DDRC_HIS_CMD_SUM_01,"), \
	ddr_flux(DMC01, 0x578, 1 * 4, "DDRC_HIS_DAT_SUM_01,"), \
	ddr_flux(DMC01, 0x57C, 1 * 4, "DDRC_HIS_PRE_CMD_01,"), \
	ddr_flux(DMC01, 0x580, 1 * 4, "DDRC_HIS_ACT_CMD_01,"), \
	ddr_flux(DMC01, 0x584, 1 * 4, "DDRC_HIS_BNK_CHG_01,"), \
	ddr_flux(DMC01, 0x588, 1 * 4, "DDRC_HIS_RNK_CHG_01,"), \
	ddr_flux(DMC01, 0x58C, 1 * 4, "DDRC_HIS_RW_CHG_01,"), \
	ddr_flux(DMC10, 0x540, 1 * 4, "DDRC_HIS_FLUX_WR_10,"), \
	ddr_flux(DMC10, 0x544, 1 * 4, "DDRC_HIS_FLUX_RD_10,"), \
	ddr_flux(DMC10, 0x548, 1 * 4, "DDRC_HIS_FLUX_WCMD_10,"), \
	ddr_flux(DMC10, 0x54C, 1 * 4, "DDRC_HIS_FLUX_RCMD_10,"), \
	ddr_flux(DMC10, 0x550, 1 * 4, "DDRC_HIS_FLUXID_WR_10,"), \
	ddr_flux(DMC10, 0x554, 1 * 4, "DDRC_HIS_FLUXID_RD_10,"), \
	ddr_flux(DMC10, 0x558, 1 * 4, "DDRC_HIS_FLUXID_WCMD_10,"), \
	ddr_flux(DMC10, 0x55C, 1 * 4, "DDRC_HIS_FLUXID_RCMD_10,"), \
	ddr_flux(DMC10, 0x560, 1 * 4, "DDRC_HIS_WLATCNT0_10,"), \
	ddr_flux(DMC10, 0x564, 1 * 4, "DDRC_HIS_WLATCNT1_10,"), \
	ddr_flux(DMC10, 0x568, 1 * 4, "DDRC_HIS_RLATCNT0_10,"), \
	ddr_flux(DMC10, 0x56C, 1 * 4, "DDRC_HIS_RLATCNT1_10,"), \
	ddr_flux(DMC10, 0x570, 1 * 4, "DDRC_HIS_INHERE_RLAT_CNT_10,"), \
	ddr_flux(DMC10, 0x574, 1 * 4, "DDRC_HIS_CMD_SUM_10,"), \
	ddr_flux(DMC10, 0x578, 1 * 4, "DDRC_HIS_DAT_SUM_10,"), \
	ddr_flux(DMC10, 0x57C, 1 * 4, "DDRC_HIS_PRE_CMD_10,"), \
	ddr_flux(DMC10, 0x580, 1 * 4, "DDRC_HIS_ACT_CMD_10,"), \
	ddr_flux(DMC10, 0x584, 1 * 4, "DDRC_HIS_BNK_CHG_10,"), \
	ddr_flux(DMC10, 0x588, 1 * 4, "DDRC_HIS_RNK_CHG_10,"), \
	ddr_flux(DMC10, 0x58C, 1 * 4, "DDRC_HIS_RW_CHG_10,"), \
	ddr_flux(DMC11, 0x540, 1 * 4, "DDRC_HIS_FLUX_WR_11,"), \
	ddr_flux(DMC11, 0x544, 1 * 4, "DDRC_HIS_FLUX_RD_11,"), \
	ddr_flux(DMC11, 0x548, 1 * 4, "DDRC_HIS_FLUX_WCMD_11,"), \
	ddr_flux(DMC11, 0x54C, 1 * 4, "DDRC_HIS_FLUX_RCMD_11,"), \
	ddr_flux(DMC11, 0x550, 1 * 4, "DDRC_HIS_FLUXID_WR_11,"), \
	ddr_flux(DMC11, 0x554, 1 * 4, "DDRC_HIS_FLUXID_RD_11,"), \
	ddr_flux(DMC11, 0x558, 1 * 4, "DDRC_HIS_FLUXID_WCMD_11,"), \
	ddr_flux(DMC11, 0x55C, 1 * 4, "DDRC_HIS_FLUXID_RCMD_11,"), \
	ddr_flux(DMC11, 0x560, 1 * 4, "DDRC_HIS_WLATCNT0_11,"), \
	ddr_flux(DMC11, 0x564, 1 * 4, "DDRC_HIS_WLATCNT1_11,"), \
	ddr_flux(DMC11, 0x568, 1 * 4, "DDRC_HIS_RLATCNT0_11,"), \
	ddr_flux(DMC11, 0x56C, 1 * 4, "DDRC_HIS_RLATCNT1_11,"), \
	ddr_flux(DMC11, 0x570, 1 * 4, "DDRC_HIS_INHERE_RLAT_CNT_11,"), \
	ddr_flux(DMC11, 0x574, 1 * 4, "DDRC_HIS_CMD_SUM_11,"), \
	ddr_flux(DMC11, 0x578, 1 * 4, "DDRC_HIS_DAT_SUM_11,"), \
	ddr_flux(DMC11, 0x57C, 1 * 4, "DDRC_HIS_PRE_CMD_11,"), \
	ddr_flux(DMC11, 0x580, 1 * 4, "DDRC_HIS_ACT_CMD_11,"), \
	ddr_flux(DMC11, 0x584, 1 * 4, "DDRC_HIS_BNK_CHG_11,"), \
	ddr_flux(DMC11, 0x588, 1 * 4, "DDRC_HIS_RNK_CHG_11,"), \
	ddr_flux(DMC11, 0x58C, 1 * 4, "DDRC_HIS_RW_CHG_11\n"),

#endif
