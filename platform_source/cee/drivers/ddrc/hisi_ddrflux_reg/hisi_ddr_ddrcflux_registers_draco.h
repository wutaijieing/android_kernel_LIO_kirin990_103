/*
 * hisi_ddr_ddrcflux_registers_draco.h
 *
 * ddrcflux registers message for draco
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
#ifndef __HISI_DDR_DDRCFLUX_REGISTERS_DRACO_H__
#define __HISI_DDR_DDRCFLUX_REGISTERS_DRACO_H__

#define PLATFORM_LIST \
	{ .compatible = "hisilicon,draco-ddrc-flux", }, \
	{ },

#define DDRC_PLATFORM_NAME		DRACO
#define DDRC_IRQ_CPU_CORE_NUM		8
#define DDRC_DMC_NUM		2
#define DDRC_ASI_PORT_NUM		8
#define DDRC_QOSB_NUM		1

#define DDRC_QOSB_PLATFORM

#ifndef DDR_REG_DMC
#define ddr_reg_dmc_addr(m, n)	\
	(SOC_ACPU_DMCPACK0_BASE_ADDR + 0x4000 + 0x20000 * \
	((m) * 2) + 0x20000 * (n))
#endif
#ifndef DDR_REG_QOSBUF
#define ddr_reg_qosbuf_addr(m)	(REG_BASE_DMSS + 0x8000 + 0x4000 * (m))
#endif

#define BIT_DMC_EN	SOC_DDRC_DMC_DDRC_CFG_STADAT_dat_stat_en_START
#define SOC_DDRC_DMC_DDRC_STADAT_ADDR SOC_DDRC_DMC_DDRC_CFG_STADAT_ADDR
#define SOC_DDRC_DMC_DDRC_STACMD_ADDR SOC_DDRC_DMC_DDRC_CFG_STACMD_ADDR
/* timer5 control info */
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

#define DDR_FLUX_DMSS_ASI_DFX_OSTD	DDR_FLUX_DMSS_ASI_QOS_OSTD_ST
#define DDR_FLUX_DMSS_ASI_STAT_FLUX_WR	DDR_FLUX_DMSS_ASI_FLUX_STAT_WR
#define DDR_FLUX_DMSS_ASI_STAT_FLUX_RD	DDR_FLUX_DMSS_ASI_FLUX_STAT_RD

#define DDR_FLUX_DMSS_ASI_QOS_OSTD_ST \
	ddr_flux(DMSS, (0x70c + 0x800 * 0), 1 * 4, "ASI_QOS_OSTD_ST_0,"), \
	ddr_flux(DMSS, (0x70c + 0x800 * 1), 1 * 4, "ASI_QOS_OSTD_ST_1,"), \
	ddr_flux(DMSS, (0x70c + 0x800 * 2), 1 * 4, "ASI_QOS_OSTD_ST_2,"), \
	ddr_flux(DMSS, (0x70c + 0x800 * 3), 1 * 4, "ASI_QOS_OSTD_ST_3,"), \
	ddr_flux(DMSS, (0x70c + 0x800 * 4), 1 * 4, "ASI_QOS_OSTD_ST_4,"), \
	ddr_flux(DMSS, (0x70c + 0x800 * 5), 1 * 4, "ASI_QOS_OSTD_ST_5,"), \
	ddr_flux(DMSS, (0x70c + 0x800 * 6), 1 * 4, "ASI_QOS_OSTD_ST_6,"), \
	ddr_flux(DMSS, (0x70c + 0x800 * 7), 1 * 4, "ASI_QOS_OSTD_ST_7,"),

#define DDR_FLUX_DMSS_ASI_FLUX_STAT_WR \
	ddr_flux(DMSS, (0x218 + 0x800 * 0), 1 * 4, "ASI_FLUX_STAT_WR_0,"), \
	ddr_flux(DMSS, (0x218 + 0x800 * 1), 1 * 4, "ASI_FLUX_STAT_WR_1,"), \
	ddr_flux(DMSS, (0x218 + 0x800 * 2), 1 * 4, "ASI_FLUX_STAT_WR_2,"), \
	ddr_flux(DMSS, (0x218 + 0x800 * 3), 1 * 4, "ASI_FLUX_STAT_WR_3,"), \
	ddr_flux(DMSS, (0x218 + 0x800 * 4), 1 * 4, "ASI_FLUX_STAT_WR_4,"), \
	ddr_flux(DMSS, (0x218 + 0x800 * 5), 1 * 4, "ASI_FLUX_STAT_WR_5,"), \
	ddr_flux(DMSS, (0x218 + 0x800 * 6), 1 * 4, "ASI_FLUX_STAT_WR_6,"), \
	ddr_flux(DMSS, (0x218 + 0x800 * 7), 1 * 4, "ASI_FLUX_STAT_WR_7,"),

#define DDR_FLUX_DMSS_ASI_FLUX_STAT_RD \
	ddr_flux(DMSS, (0x21c + 0x800 * 0), 1 * 4, "ASI_FLUX_STAT_RD_0,"), \
	ddr_flux(DMSS, (0x21c + 0x800 * 1), 1 * 4, "ASI_FLUX_STAT_RD_1,"), \
	ddr_flux(DMSS, (0x21c + 0x800 * 2), 1 * 4, "ASI_FLUX_STAT_RD_2,"), \
	ddr_flux(DMSS, (0x21c + 0x800 * 3), 1 * 4, "ASI_FLUX_STAT_RD_3,"), \
	ddr_flux(DMSS, (0x21c + 0x800 * 4), 1 * 4, "ASI_FLUX_STAT_RD_4,"), \
	ddr_flux(DMSS, (0x21c + 0x800 * 5), 1 * 4, "ASI_FLUX_STAT_RD_5,"), \
	ddr_flux(DMSS, (0x21c + 0x800 * 6), 1 * 4, "ASI_FLUX_STAT_RD_6,"), \
	ddr_flux(DMSS, (0x21c + 0x800 * 7), 1 * 4, "ASI_FLUX_STAT_RD_7,"),

#define DDR_FLUX_DMC \
	ddr_flux(DMC00, 0x380, 1 * 4, "DDRC_HIS_FLUX_WR_00,"), \
	ddr_flux(DMC00, 0x384, 1 * 4, "DDRC_HIS_FLUX_RD_00,"), \
	ddr_flux(DMC00, 0x388, 1 * 4, "DDRC_HIS_FLUX_WCMD_00,"), \
	ddr_flux(DMC00, 0x38C, 1 * 4, "DDRC_HIS_FLUX_RCMD_00,"), \
	ddr_flux(DMC00, 0x390, 1 * 4, "DDRC_HIS_FLUXID_WR_00,"), \
	ddr_flux(DMC00, 0x394, 1 * 4, "DDRC_HIS_FLUXID_RD_00,"), \
	ddr_flux(DMC00, 0x398, 1 * 4, "DDRC_HIS_FLUXID_WCMD_00,"), \
	ddr_flux(DMC00, 0x39C, 1 * 4, "DDRC_HIS_FLUXID_RCMD_00,"), \
	ddr_flux(DMC00, 0x3A0, 1 * 4, "DDRC_HIS_WLATCNT0_00,"), \
	ddr_flux(DMC00, 0x3A4, 1 * 4, "DDRC_HIS_WLATCNT1_00,"), \
	ddr_flux(DMC00, 0x3A8, 1 * 4, "DDRC_HIS_RLATCNT0_00,"), \
	ddr_flux(DMC00, 0x3AC, 1 * 4, "DDRC_HIS_RLATCNT1_00,"), \
	ddr_flux(DMC00, 0x3B0, 1 * 4, "DDRC_HIS_INHERE_RLAT_CNT_00,"), \
	ddr_flux(DMC00, 0x3B8, 1 * 4, "DDRC_HIS_CMD_SUM_00,"), \
	ddr_flux(DMC00, 0x3BC, 1 * 4, "DDRC_HIS_DAT_SUM_00,"), \
	ddr_flux(DMC00, 0x3C0, 1 * 4, "DDRC_HIS_PRE_CMD_00,"), \
	ddr_flux(DMC00, 0x3C4, 1 * 4, "DDRC_HIS_ACT_CMD_00,"), \
	ddr_flux(DMC00, 0x3C8, 1 * 4, "DDRC_HIS_BNK_CHG_00,"), \
	ddr_flux(DMC00, 0x3CC, 1 * 4, "DDRC_HIS_RNK_CHG_00,"), \
	ddr_flux(DMC00, 0x3D0, 1 * 4, "DDRC_HIS_RW_CHG_00,"), \
	ddr_flux(DMC01, 0x380, 1 * 4, "DDRC_HIS_FLUX_WR_01,"), \
	ddr_flux(DMC01, 0x384, 1 * 4, "DDRC_HIS_FLUX_RD_01,"), \
	ddr_flux(DMC01, 0x388, 1 * 4, "DDRC_HIS_FLUX_WCMD_01,"), \
	ddr_flux(DMC01, 0x38C, 1 * 4, "DDRC_HIS_FLUX_RCMD_01,"), \
	ddr_flux(DMC01, 0x390, 1 * 4, "DDRC_HIS_FLUXID_WR_01,"), \
	ddr_flux(DMC01, 0x394, 1 * 4, "DDRC_HIS_FLUXID_RD_01,"), \
	ddr_flux(DMC01, 0x398, 1 * 4, "DDRC_HIS_FLUXID_WCMD_01,"), \
	ddr_flux(DMC01, 0x39C, 1 * 4, "DDRC_HIS_FLUXID_RCMD_01,"), \
	ddr_flux(DMC01, 0x3A0, 1 * 4, "DDRC_HIS_WLATCNT0_01,"), \
	ddr_flux(DMC01, 0x3A4, 1 * 4, "DDRC_HIS_WLATCNT1_01,"), \
	ddr_flux(DMC01, 0x3A8, 1 * 4, "DDRC_HIS_RLATCNT0_01,"), \
	ddr_flux(DMC01, 0x3AC, 1 * 4, "DDRC_HIS_RLATCNT1_01,"), \
	ddr_flux(DMC01, 0x3B0, 1 * 4, "DDRC_HIS_INHERE_RLAT_CNT_01,"), \
	ddr_flux(DMC01, 0x3B8, 1 * 4, "DDRC_HIS_CMD_SUM_01,"), \
	ddr_flux(DMC01, 0x3BC, 1 * 4, "DDRC_HIS_DAT_SUM_01,"), \
	ddr_flux(DMC01, 0x3C0, 1 * 4, "DDRC_HIS_PRE_CMD_01,"), \
	ddr_flux(DMC01, 0x3C4, 1 * 4, "DDRC_HIS_ACT_CMD_01,"), \
	ddr_flux(DMC01, 0x3C8, 1 * 4, "DDRC_HIS_BNK_CHG_01,"), \
	ddr_flux(DMC01, 0x3CC, 1 * 4, "DDRC_HIS_RNK_CHG_01,"), \
	ddr_flux(DMC01, 0x3D0, 1 * 4, "DDRC_HIS_RW_CHG_01\n"),

#define DDR_QOSB \
	ddr_flux(QOSB0, CMD_SUM_0_OFFSET, DDR_FLUX_QOSB_LEN, \
		 "QOSB_CMD_SUM_0,"), \
	ddr_flux(QOSB0, CMD_CNT_0_OFFSET, DDR_FLUX_QOSB_LEN, \
		 "QOSB_CMD_CNT_0,"), \
	ddr_flux(QOSB0, OSTD_CNT_0_OFFSET, DDR_FLUX_QOSB_LEN, \
		 "QOSB_OSTD_CNT_0,"), \
	ddr_flux(QOSB0, WR_SUM_0_OFFSET, DDR_FLUX_QOSB_LEN, \
		 "QOSB_WR_SUM_0,"), \
	ddr_flux(QOSB0, RD_SUM_0_OFFSET, DDR_FLUX_QOSB_LEN, \
		 "QOSB_RD_SUM_0,"),

#endif
