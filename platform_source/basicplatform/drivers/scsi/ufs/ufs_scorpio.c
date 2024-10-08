/*
 * ufs_scorpio.c
 *
 * ufs configuration for scorpio
 *
 * Copyright (c) 2020 Huawei Technologies Co., Ltd.
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

#define pr_fmt(fmt) "ufshcd :" fmt
#include "dsm_ufs.h"
#include "hufs_hcd.h"
#include "hufs_plat.h"
#include "ufshcd.h"
#include "unipro.h"
#include <linux/platform_drivers/lpcpu_idle_sleep.h>
#include <platform_include/basicplatform/linux/mfd/pmic_platform.h>
#include <linux/of_address.h>
#include <pmic_interface.h>
#include <soc_actrl_interface.h>
#include <soc_hsdt_crg_interface.h>
#include <soc_pmctrl_interface.h>
#include <soc_sctrl_interface.h>
#include <soc_ufs_sysctrl_interface.h>

#define POLLING_RETRY 2000
#define POLLING_RETRY_4W 40000
#define MPHY_BIT_REG_VCO_SW_VCAL BIT(7)
#define UNIPRO_CLK_AUTO_GATE_DISABLE 0x3000000
#define UNIPRO_CLK_AUTO_GATE_CONFIG 0x3001C0E
#define UFSHC_CLOCK_GATING_CONFIG 0xF
#define UFS_SYSCTRL_RESET_CTRL_OFFSET 0x8
#define UFS_SYSCTRL_CRG_CLK_OFFSET 0x0

#define MPHY_SR_FIX_SIZE 4

static const struct ufs_attr_cfg hufs_mphy_v200_pre_link_attr[] = {
	{ 0x0000156A, 0x00000002 }, /* PA_HSSeries */
	{ 0x00020009, 0x00000001 }, /* Rx SKP_DET_SEL, lane0 */
	{ 0x00030009, 0x00000001 }, /* Rx SKP_DET_SEL, lane1 */
	{ 0x000000DF, 0x00000003 }, /* VCO_AUTO_CHG */
	/* FPGA can only support HS Gear 1, Gear 4 for only test chip */
	{ 0x00000023, 0x00000004 }, /* TX_HSGEAR */
	{ 0x00010023, 0x00000004 }, /* TX_HSGEAR */
	{ 0x000200a3, 0x00000004 }, /* RX_HSGEAR */
	{ 0x000300a3, 0x00000004 }, /* RX_HSGEAR */
	{ 0x000200F1, 0x00000004 }, /* RX_SQ_VREF, lane0 ,RX_SQ_VREF_140mv */
	{ 0x000300F1, 0x00000004 }, /* RX_SQ_VREF, lane1 ,RX_SQ_VREF_140mv */
	{ 0x00020003, 0x0000000A }, /* AD_DIF_P_LS_TIMEOUT_VAL, lane0 */
	{ 0x00030003, 0x0000000A }, /* AD_DIF_P_LS_TIMEOUT_VAL, lane1 */
	{ 0x00020004, 0x00000064 }, /* AD_DIF_N_LS_TIMEOUT_VAL, lane0 */
	{ 0x00030004, 0x00000064 }, /* AD_DIF_N_LS_TIMEOUT_VAL, lane1 */
	{ 0x000200cf, 0x00000002 }, /* RX_STALL */
	{ 0x000300cf, 0x00000002 }, /* RX_STALL */
	{ 0x000200d0, 0x00000002 }, /* RX_SLEEP */
	{ 0x000300d0, 0x00000002 }, /* RX_SLEEP */
	{ 0x000200F0, 0x00000001 }, /* RX enable, lane0 */
	{ 0x000300F0, 0x00000001 }, /* RX enable, lane1 */
	{ 0x00000059, 0x0000000f }, /* reg 0x59,TX0 */
	{ 0x00010059, 0x0000000f }, /* reg 0x59,TX1 */
	{ 0x0000005A, 0x00000000 }, /* reg 0x5A,TX0 */
	{ 0x0001005A, 0x00000000 }, /* reg 0x5A,TX1 */
	{ 0x0000005B, 0x0000000f }, /* reg 0x5B,TX0 */
	{ 0x0001005B, 0x0000000f }, /* reg 0x5B,TX1 */
	{ 0x0000005C, 0x00000000 }, /* reg 0x5C,TX0 */
	{ 0x0001005C, 0x00000000 }, /* reg 0x5C,TX1 */
	{ 0x0000005D, 0x00000000 }, /* reg 0x5D,TX0 */
	{ 0x0001005D, 0x00000000 }, /* reg 0x5D,TX1 */
	{ 0x0000005E, 0x0000000a }, /* reg 0x5E,TX0 */
	{ 0x0001005E, 0x0000000a }, /* reg 0x5E,TX1 */
	{ 0x0000005F, 0x0000000a }, /* reg 0x5F,TX0 */
	{ 0x0001005F, 0x0000000a }, /* reg 0x5F,TX1 */
	{ 0x0000007A, 0x00000000 }, /* reg 0x7A,TX0 */
	{ 0x0001007A, 0x00000000 }, /* reg 0x7A,TX1 */
	{ 0x0000007B, 0x00000000 }, /* reg 0x7B,TX0 */
	{ 0x0001007B, 0x00000000 }, /* reg 0x7B,TX1 */
	/* Tell UFS that RX can exit H8 only when host TX exits H8 first */
	{ 0x000200C3, 0x00000004 }, /* RX_EXTERNAL_H8_EXIT_EN, lane0 */
	{ 0x000300C3, 0x00000004 }, /* RX_EXTERNAL_H8_EXIT_EN, lane1 */
	{ 0x000200f6, 0x00000001 }, /* RX_DLF Lane 0 */
	{ 0x000300f6, 0x00000001 }, /* RX_DLF Lane 1 */
	{ 0x000000C7, 0x00000003 }, /* measure the power, can close it */
	{ 0x000000C8, 0x00000003 }, /* measure the power, can close it */
	{ 0x000000c5, 0x00000003 }, /* RG_PLL_RXHS_EN */
	{ 0x000000c6, 0x00000003 }, /* RG_PLL_RXLS_EN */

	{ 0x000200E9, 0x00000000 }, /* RX_HS_DATA_VALID_TIMER_VAL0 */
	{ 0x000300E9, 0x00000000 }, /* RX_HS_DATA_VALID_TIMER_VAL0 */
	{ 0x000200EA, 0x00000010 }, /* RX_HS_DATA_VALID_TIMER_VAL1 */
	{ 0x000300EA, 0x00000010 }, /* RX_HS_DATA_VALID_TIMER_VAL1 */

	/* rx_setting_better_perform */
	{ 0x000200f4, 0x00000000 },
	{ 0x000300f4, 0x00000000 },
	{ 0x000200f3, 0x00000000 },
	{ 0x000300f3, 0x00000000 },
	{ 0x000200f2, 0x00000003 },
	{ 0x000300f2, 0x00000003 },
	{ 0x000200f6, 0x00000002 },
	{ 0x000300f6, 0x00000002 },
	{ 0x000200f5, 0x00000000 },
	{ 0x000300f5, 0x00000000 },
	{ 0x000200fc, 0x0000001f },
	{ 0x000300fc, 0x0000001f },
	{ 0x000200fd, 0x00000000 },
	{ 0x000300fd, 0x00000000 },
	{ 0x000200fb, 0x00000005 },
	{ 0x000300fb, 0x00000005 },
	{ 0x00020011, 0x00000011 },
	{ 0x00030011, 0x00000011 },
	/* need to check the value of 0x9529 in the next version of test chip */
	{ 0x00009529, 0x00000006 }, /* VS_DebugSaveConfigTime */
	{ 0x0000D014, 0x00000001 },
	/* update */ /* Hufs UniPro */
	{ 0, 0 },
};

static const struct ufs_attr_cfg hufs_mphy_v200_post_link_attr[] = {
	{ 0x00003000, 0x00000000 }, /* N_ DeviceID */
	{ 0x00003001, 0x00000001 }, /* N_DeviceID_valid */
	{ 0x00004020, 0x00000000 }, /* T_ConnectionState */
	{ 0x00004022, 0x00000000 }, /* T_PeerCPortID */
	{ 0x00004021, 0x00000001 }, /* T_PeerDeviceID */
	{ 0x00004023, 0x00000000 }, /* T_TrafficClass */
	{ 0x00004020, 0x00000001 }, /* T_ConnectionState */
	{ 0x0002000a, 0x0000001E }, /* RX H8_TIMEOUT_VAL, Lane 0 */
	{ 0x0003000a, 0x0000001E }, /* RX H8_TIMEOUT_VAL, Lane 1 */
	{ 0x000200EB, 0x00000064 }, /* AD_DIF_N_TIMEOUT_VAL */
	{ 0x000300EB, 0x00000064 }, /* AD_DIF_N_TIMEOUT_VAL */
	{ 0x0002000E, 0x000000F0 }, /* RX_H8_EXIT_TIME */
	{ 0x0003000E, 0x000000F0 }, /* RX_H8_EXIT_TIME */
	{ 0x000000da, 0x0000004B }, /* PLL_LOCK_TIME_VAL */
	{ 0x000000dd, 0x000000CB }, /* PLL_RSTB_TIME_VAL */
	{ 0, 0 },
};

static const struct ufs_attr_cfg hufs_mphy_v120_pre_link_attr[] = {
	{ 0x0000156A, 0x2 }, /* PA_HSSeries */

	{ 0x00000023, 0x00000004 },
	{ 0x00010023, 0x00000004 },
	{ 0x000200a3, 0x00000004 },
	{ 0x000300a3, 0x00000004 },
	{ 0x000200f1, 0x00000004 },
	{ 0x000300f1, 0x00000004 },
	{ 0x00020004, 0x00000064 },
	{ 0x00030004, 0x00000064 },
	{ 0x00020003, 0x0000000A },
	{ 0x00030003, 0x0000000A },
	{ 0x000200f0, 0x00000001 },
	{ 0x000300f0, 0x00000001 },

	{ 0x00020009, 0x00000001 }, /* Rx SKP_DET_SEL, lane0 */
	{ 0x00030009, 0x00000001 }, /* Rx SKP_DET_SEL, lane1 */
	{ 0x000000DF, 0x00000003 }, /* VCO_AUTO_CHG */
	{ 0x000000C7, 0x3 }, /* measure the power, can close it */
	{ 0x000000C8, 0x3 }, /* measure the power, can close it */
	{ 0x000200cf, 0x00000002 }, /* RX_STALL */
	{ 0x000300cf, 0x00000002 }, /* RX_STALL */
	{ 0x000200d0, 0x00000002 }, /* RX_SLEEP */
	{ 0x000300d0, 0x00000002 }, /* RX_SLEEP */
	{ 0x000200cc, 0x00000003 }, /* RX_HS_CLK_EN */
	{ 0x000300cc, 0x00000003 }, /* RX_HS_CLK_EN */
	{ 0x000200cd, 0x00000003 }, /* RX_LS_CLK_EN */
	{ 0x000300cd, 0x00000003 }, /* RX_LS_CLK_EN */
	{ 0x000000c5, 0x00000003 }, /* RG_PLL_RXHS_EN */
	{ 0x000000c6, 0x00000003 }, /* RG_PLL_RXLS_EN */
	{ 0x000200E9, 0x00000000 }, /* RX_HS_DATA_VALID_TIMER_VAL0 */
	{ 0x000300E9, 0x00000000 }, /* RX_HS_DATA_VALID_TIMER_VAL0 */
	{ 0x000200EA, 0x00000010 }, /* RX_HS_DATA_VALID_TIMER_VAL1 */
	{ 0x000300EA, 0x00000010 }, /* RX_HS_DATA_VALID_TIMER_VAL1 */
	{ 0x00001552, 0x0000004F }, /* PA_TxHsG1SyncLength */
	{ 0x00001554, 0x0000004F }, /* PA_TxHsG2SyncLength */
	{ 0x00001556, 0x0000004F }, /* PA_TxHsG3SyncLength */
	{ 0x00009509, 0x0000004F },
	{ 0x0000950A, 0x0000000F },
	{ 0x000200F4, 0x1 }, /* RX_EQ_SEL_R */
	{ 0x000300F4, 0x1 }, /* RX_EQ_SEL_R */
	{ 0x000200F2, 0x3 }, /* RX_EQ_SEL_C */
	{ 0x000300F2, 0x3 }, /* RX_EQ_SEL_C */
	{ 0x000200FB, 0x3 }, /* RX_VSEL */
	{ 0x000300FB, 0x3 }, /* RX_VSEL */
	{ 0x000200F6, 0x3 }, /* RX_DLF Lane 0 */
	{ 0x000300F6, 0x3 }, /* RX_DLF Lane 1 */
	{ 0x0002000a, 0x00000002 }, /* RX H8_TIMEOUT_VAL, Lane 0 */
	{ 0x0003000a, 0x00000002 }, /* RX H8_TIMEOUT_VAL, Lane 1 */
	{ 0x000000d4, 0x00000031 }, /* RG_PLL_DMY0 */
	{ 0x00000073, 0x00000004 }, /* TX_PHY_CONFIG II */
	{ 0x00010073, 0x00000004 }, /* TX_PHY_CONFIG II */
	{ 0x000200F0, 0x00000001 }, /* RX enable, lane0 */
	{ 0x000300F0, 0x00000001 }, /* RX enable, lane1 */
	{ 0x0000D014, 0x1 }, /* Unipro VS_MphyCfgUpdt */
	{ 0, 0 },
};

static const struct ufs_attr_cfg hufs_mphy_v120_post_link_attr[] = {
	{ 0x00003000, 0x0 }, /* N_ DeviceID */
	{ 0x00003001, 0x1 }, /* N_DeviceID_valid */
	{ 0x00004020, 0x0 }, /* T_ConnectionState */
	{ 0x00004022, 0x0 }, /* T_PeerCPortID */
	{ 0x00004021, 0x1 }, /* T_PeerDeviceID */
	{ 0x00004023, 0x0 }, /* T_TrafficClass */
	{ 0x00004020, 0x1 }, /* T_ConnectionState */
	{ 0x0002000a, 0x0000001E }, /* RX H8_TIMEOUT_VAL, Lane 0 */
	{ 0x0003000a, 0x0000001E }, /* RX H8_TIMEOUT_VAL, Lane 1 */
	{ 0x000200EB, 0x64 }, /* AD_DIF_N_TIMEOUT_VAL */
	{ 0x000300EB, 0x64 }, /* AD_DIF_N_TIMEOUT_VAL */
	{ 0x0002000E, 0xF0 }, /* RX_H8_EXIT_TIME */
	{ 0x0003000E, 0xF0 }, /* RX_H8_EXIT_TIME */
	{ 0x000000ca, 0x3 },
	{ 0x000015a8, 10 }, /* PA_TActivate */
	{ 0, 0 },
};

/* ASIC */
static const struct ufs_attr_cfg hufs_mphy_v300_pre_link_attr[] = {
	{ PA_TACTIVATE, 0x13 },
	{ 0x0000156A, 0x00000002 }, /* PA_HSSeries */
	{ 0x00020009, 0x00000001 }, /* Rx SKP_DET_SEL, lane0 */
	{ 0x00030009, 0x00000001 }, /* Rx SKP_DET_SEL, lane1 */
	{ 0x000000DF, 0x00000002 }, /* VCO_AUTO_CHG */
	{ 0x00020003, 0x0000000A }, /* AD_DIF_P_LS_TIMEOUT_VAL, lane0 */
	{ 0x00030003, 0x0000000A }, /* AD_DIF_P_LS_TIMEOUT_VAL, lane1 */
	{ 0x00020004, 0x00000064 }, /* AD_DIF_N_LS_TIMEOUT_VAL, lane0 */
	{ 0x00030004, 0x00000064 }, /* AD_DIF_N_LS_TIMEOUT_VAL, lane1 */
	{ 0x00020017, 0x00000084 }, /* RX_AFE_CTRL lane 0, HW calibration bit2 and 7 */
	{ 0x00030017, 0x00000084 }, /* RX_AFE_CTRL lane 1, HW calibration bit2 and 7 */
	{ 0x00000057, 0x00000000 }, /* AFE_CONFIG_IX lane 0 */
	{ 0x00010057, 0x00000000 }, /* AFE_CONFIG_IX lane 1 */
	{ 0x00000059, 0x00000022 }, /* AFE_CONFIG_XII lane 0 */
	{ 0x00010059, 0x00000022 }, /* AFE_CONFIG_XII lane 1 */
	{ 0x0000007B, 0x00000023 }, /*  TX_AFE_CONFIG_II lane 0 */
	{ 0x0001007B, 0x00000023 }, /*  TX_AFE_CONFIG_II lane 1 */
	{ 0x000200F0, 0x00000001 }, /* RX enable, lane0 */
	{ 0x000300F0, 0x00000001 }, /* RX enable, lane1 */
	{ 0x000200C7, 0x1 }, /* enable override */
	{ 0x000300C7, 0x1 }, /* enable override */
	{ 0x00020092, 0x1 }, /* RX_HIBERN8TIME_CAPABILITY lane0 */
	{ 0x00030092, 0x1 }, /* RX_HIBERN8TIME_CAPABILITY lane1 */
	{ 0x0002009D, 0x1 }, /* RX_HS_Equalizer_Setting_Capability lane 0 */
	{ 0x0003009D, 0x1 }, /* RX_HS_Equalizer_Setting_Capability lane 1 */
	{ 0x000200C7, 0x0 }, /* disable override */
	{ 0x000300C7, 0x0 }, /* disable override */
	{ 0x000200E9, 0x00000040 }, /* RX_HS_DATA_VALID_TIMER_VAL0 */
	{ 0x000300E9, 0x00000040 }, /* RX_HS_DATA_VALID_TIMER_VAL0 */
	{ 0x000200EA, 0x0000001F }, /* RX_HS_DATA_VALID_TIMER_VAL1 */
	{ 0x000300EA, 0x0000001F }, /* RX_HS_DATA_VALID_TIMER_VAL1 */
};

static const struct ufs_attr_cfg hufs_mphy_v300_post_link_attr[] = {
	{ 0x00003000, 0x00000000 }, /* N_DeviceID */
	{ 0x00003001, 0x00000001 }, /* N_DeviceID_valid */
	{ 0x00004020, 0x00000000 }, /* T_ConnectionState */
	{ 0x00004022, 0x00000000 }, /* T_PeerCPortID */
	{ 0x00004021, 0x00000001 }, /* T_PeerDeviceID */
	{ 0x00004023, 0x00000000 }, /* T_TrafficClass */
	{ 0x00004020, 0x00000001 }, /* T_ConnectionState */
	{ 0x0002000A, 0x00000002 }, /* RX H8_TIMEOUT_VAL, Lane 0 */
	{ 0x0003000A, 0x00000002 }, /* RX H8_TIMEOUT_VAL, Lane 1 */
	{ 0x000200EB, 0x00000064 }, /* AD_DIF_N_TIMEOUT_VAL */
	{ 0x000300EB, 0x00000064 }, /* AD_DIF_N_TIMEOUT_VAL */
	{ 0x0002000E, 0x000000F0 }, /* RX_H8_EXIT_TIME */
	{ 0x0003000E, 0x000000F0 }, /* RX_H8_EXIT_TIME */
	{ 0x000000DF, 0x00000003 }, /* VCO_AUTO_CHG */
	{ 0x000015A8, 0x00000005 }, /* PA_TActivate */
	{ 0, 0 },
};

static const struct ufs_attr_cfg hufs_mphy_v300_pre_pmc_attr[][PMC_REGISTER_SIZE] = {
	{ /* PMC to Slow/Slow auto */
		{ 0x00000057, 0x00000000 }, /* RG_TX0_SL_EN_POST[7:0] TX 0x57 */
		{ 0x00010057, 0x00000000 }, /* RG_TX1_SL_EN_POST[7:0] TX 0x57 */
		{ 0x00020013, 0x00000000 }, /* RX0 CDR KI/KP, RX 0x13 */
		{ 0x00030013, 0x00000000 }, /* RX1 CDR KI/KP, RX 0x13 */
		{ 0x000200F3, 0x00000000 }, /* RX0 RX_EQ_SEL_I, RX 0xF3 */
		{ 0x000300F3, 0x00000000 }, /* RX1 RX_EQ_SEL_I, RX 0xF3 */
		{ 0x0002000F, 0x00000000 }, /* RX0 RG_PI_LPFI/RG_PI_LPFO, RX 0x0F */
		{ 0x0003000F, 0x00000000 }, /* RX1 RG_PI_LPFI/RG_PI_LPFO, RX 0x0F */
		{ 0, 0 },
	},
	{ /* PMC to Fast/Fast auto G1 */
		{ 0x00000057, 0x00000000 }, /* RG_TX0_SL_EN_POST[7:0] TX 0x57 */
		{ 0x00010057, 0x00000000 }, /* RG_TX1_SL_EN_POST[7:0] TX 0x57 */
		{ 0x00020013, 0x00000012 }, /* RX0 CDR KI/KP, RX 0x13 */
		{ 0x00030013, 0x00000012 }, /* RX1 CDR KI/KP, RX 0x13 */
		{ 0x000200F3, 0x00000000 }, /* RX0 RX_EQ_SEL_I, RX 0xF3 */
		{ 0x000300F3, 0x00000000 }, /* RX1 RX_EQ_SEL_I, RX 0xF3 */
		{ 0x0002000F, 0x00000005 }, /* RX0 RG_PI_LPFI/RG_PI_LPFO, RX 0x0F */
		{ 0x0003000F, 0x00000005 }, /* RX1 RG_PI_LPFI/RG_PI_LPFO, RX 0x0F */
		{ 0, 0 },
	},
	{ /* PMC to Fast/Fast auto G2 */
		{ 0x00000057, 0x00000000 }, /* RG_TX0_SL_EN_POST[7:0] TX 0x57 */
		{ 0x00010057, 0x00000000 }, /* RG_TX1_SL_EN_POST[7:0] TX 0x57 */
		{ 0x00020013, 0x00000012 }, /* RX0 CDR KI/KP, RX 0x13 */
		{ 0x00030013, 0x00000012 }, /* RX1 CDR KI/KP, RX 0x13 */
		{ 0x000200F3, 0x00000000 }, /* RX0 RX_EQ_SEL_I, RX 0xF3 */
		{ 0x000300F3, 0x00000000 }, /* RX1 RX_EQ_SEL_I, RX 0xF3 */
		{ 0x0002000F, 0x0000000C }, /* RX0 RG_PI_LPFI/RG_PI_LPFO, RX 0x0F */
		{ 0x0003000F, 0x0000000C }, /* RX1 RG_PI_LPFI/RG_PI_LPFO, RX 0x0F */
		{ 0, 0 },
	},
	{ /* PMC to Fast/Fast auto G3 */
		{ 0x00000057, 0x00000000 }, /* RG_TX0_SL_EN_POST[7:0] TX 0x57 */
		{ 0x00010057, 0x00000000 }, /* RG_TX1_SL_EN_POST[7:0] TX 0x57 */
		{ 0x00020013, 0x00000012 }, /* RX0 CDR KI/KP, RX 0x13 */
		{ 0x00030013, 0x00000012 }, /* RX1 CDR KI/KP, RX 0x13 */
		{ 0x000200F3, 0x00000000 }, /* RX0 RX_EQ_SEL_I, RX 0xF3 */
		{ 0x000300F3, 0x00000000 }, /* RX1 RX_EQ_SEL_I, RX 0xF3 */
		{ 0x0002000F, 0x00000004 }, /* RX0 RG_PI_LPFI/RG_PI_LPFO, RX 0x0F */
		{ 0x0003000F, 0x00000004 }, /* RX1 RG_PI_LPFI/RG_PI_LPFO, RX 0x0F */
		{ 0, 0 },
	},
	{ /* PMC to Fast/Fast auto G4 */
		{ 0x00000057, 0x00000000 }, /* RG_TX0_SL_EN_POST[7:0] TX 0x57 */
		{ 0x00010057, 0x00000000 }, /* RG_TX1_SL_EN_POST[7:0] TX 0x57 */
		{ 0x00020013, 0x00000000 }, /* RX0 CDR KI/KP, RX 0x13 */
		{ 0x00030013, 0x00000000 }, /* RX1 CDR KI/KP, RX 0x13 */
		{ 0x000200F3, 0x00000001 }, /* RX0 RX_EQ_SEL_I, RX 0xF3 */
		{ 0x000300F3, 0x00000001 }, /* RX1 RX_EQ_SEL_I, RX 0xF3 */
		{ 0x0002000F, 0x00000000 }, /* RX0 RG_PI_LPFI/RG_PI_LPFO, RX 0x0F */
		{ 0x0003000F, 0x00000000 }, /* RX1 RG_PI_LPFI/RG_PI_LPFO, RX 0x0F */
		{ 0, 0 },
	}
};

static struct ufs_attr_cfg hufs_mphy_sr_fix_attr[] = {
	{ ATTR_MTX0(TX_DMY1), 0x00000000 },
	{ ATTR_MTX1(TX_DMY1), 0x00000000 },
	{ ATTR_MRX0(RX_R_CAL), 0x00000000 },
	{ ATTR_MRX1(RX_R_CAL), 0x00000000 },
};

void hufs_regulator_init(struct ufs_hba *hba)
{
}

void ufs_clk_init(struct ufs_hba *hba)
{
}

static void ufs_release_reset_subsys_crg(struct hufs_host *host)
{
	u32 value;
	int retry = POLLING_RETRY;

	sys_ctrl_set_bits(host, BIT(IP_RST_UFS_SUBSYS_CRG),
			  SOC_SCTRL_SCPERRSTDIS1);
	while (retry--) {
		value = readl(SOC_SCTRL_SCPERRSTSTAT1_ADDR(host->sysctrl));
		if (!(value & IP_RST_UFS_SUBSYS_CRG))
			break;
		udelay(1);
	}
	if (retry < 0)
		pr_err("UFS Sub-sys CRG reset failed\n");
}

void ufs_soc_init(struct ufs_hba *hba)
{
	struct hufs_host *host = NULL;

	host = (struct hufs_host *)hba->priv;

	dev_info(hba->dev, "%s ++\n", __func__);

	/* 1. Set SD and DS to 0 */
	writel(0x00030000, SOC_ACTRL_AO_MEM_CTRL2_ADDR(host->actrl));

	/* 2. release reset of UFS subsystem CRG */
	ufs_release_reset_subsys_crg(host);

	/* 5. enable ip_rst_ufs_subsys, reset ufs sub-system */
	sys_ctrl_set_bits(host, BIT(IP_RST_UFS_SUBSYS), SOC_SCTRL_SCPERRSTEN1);
	/* 6. setup ufs sub-system clk to 224MHz */
	writel(0x003F0006, SOC_SCTRL_SCCLKDIV9_ADDR(host->sysctrl));

	/* 10. Release MPHY related ISO */
	writel(0x00210000, SOC_ACTRL_ISO_EN_GROUP0_PERI_ADDR(host->actrl));

	/* 11. Enable clk_ufs_subsys clock */
	writel(BIT(SOC_SCTRL_SCPEREN4_gt_clk_ufs_subsys_START),
	       SOC_SCTRL_SCPEREN4_ADDR(host->sysctrl));

	/* device reset */
	hufs_device_hw_reset(hba);

	/* 15. release ufs mast-reset */
	sys_ctrl_set_bits(host, BIT(IP_RST_UFS_SUBSYS), SOC_SCTRL_SCPERRSTDIS1);
	/* release internal resets */
	if (ufshcd_is_hufs_hc(hba))
		writel(0xFFFFFFFF,
		       host->ufs_sys_ctrl + UFS_SYSCTRL_RESET_CTRL_OFFSET);

	mdelay(1);

	/* set SOC_SCTRL_SCBAKDATA11_ADDR ufs bit to 1 when init */
	if (!ufshcd_is_auto_hibern8_allowed(hba))
		lpcpu_idle_sleep_vote(ID_UFS, 1);
	dev_info(hba->dev, "%s --\n", __func__);
}

void unipro_clk_auto_gate_enable(struct ufs_hba *hba)
{
	hufs_uic_write_reg(hba, DME_CTRL1, UNIPRO_CLK_AUTO_GATE_CONFIG);
}

void unipro_clk_auto_gate_disable(struct ufs_hba *hba)
{
	hufs_uic_write_reg(hba, DME_CTRL1, UNIPRO_CLK_AUTO_GATE_DISABLE);
}

static void mphy_sr_fix_store(struct ufs_hba *hba)
{
	u16 i;

	for(i = 0; i < MPHY_SR_FIX_SIZE; i++)
		hufs_uic_read_reg(hba, hufs_mphy_sr_fix_attr[i].addr,
			&hufs_mphy_sr_fix_attr[i].val);
}

static int mphy_sr_fix_load(struct ufs_hba *hba, enum ufs_pm_op pm_op)
{
	u16 i;
	int ret = 0;

	if (ufshcd_is_runtime_pm(pm_op) || ufshcd_is_shutdown_pm(pm_op))
		return 0;

	/* use exit h8 updata register adapt */
	ret = ufshcd_uic_hibern8_enter(hba);
	if (ret == 0)
		ufshcd_set_link_hibern8(hba);
	else
		goto out;

	for(i = 0; i < MPHY_SR_FIX_SIZE; i++)
		hufs_uic_write_reg(hba, hufs_mphy_sr_fix_attr[i].addr,
			hufs_mphy_sr_fix_attr[i].val);

	ret = ufshcd_uic_hibern8_exit(hba);
	if (ret == 0)
		ufshcd_set_link_active(hba);
	else
		goto out;

	return ret;

out:
	kthread_flush_work(&hba->eh_work);
	dev_err(hba->dev, "flush error handle work done\n");
	ret = 0;

	return ret;
}

int hufs_suspend_before_set_link_state(
	struct ufs_hba *hba, enum ufs_pm_op pm_op)
{
	unipro_clk_auto_gate_disable(hba);
	return 0;
}

int hufs_resume_after_set_link_state(
	struct ufs_hba *hba, enum ufs_pm_op pm_op)
{
	int ret = 0;

	ret = mphy_sr_fix_load(hba, pm_op);
	if (ret < 0)
		goto out;
	unipro_clk_auto_gate_enable(hba);

out:
	return ret;
}

static void mphy_iso_enable(struct ufs_hba *hba)
{
	struct hufs_host *host = hba->priv;

	/* MPHY power off on FPGA */
	if (host->caps & USE_HUFS_MPHY_TC)
		writel(0x00010001,
		       SOC_ACTRL_ISO_EN_GROUP0_PERI_ADDR(host->actrl));
	else
		writel(0x00200020,
		       SOC_ACTRL_ISO_EN_GROUP0_PERI_ADDR(host->actrl));
}

static void mphy_iso_disable(struct ufs_hba *hba)
{
	struct hufs_host *host = hba->priv;

	if (host->caps & USE_HUFS_MPHY_TC)
		writel(0x00010000,
		       SOC_ACTRL_ISO_EN_GROUP0_PERI_ADDR(host->actrl));
	else
		writel(0x00200000,
		       SOC_ACTRL_ISO_EN_GROUP0_PERI_ADDR(host->actrl));
}

int hufs_suspend(struct ufs_hba *hba, enum ufs_pm_op pm_op)
{
	int ret;
	struct hufs_host *host = NULL;
	u32 value;

	host = hba->priv;

	/* set SOC_SCTRL_SCBAKDATA11_ADDR ufs bit to 0 when idle */
	if (!ufshcd_is_auto_hibern8_allowed(hba))
		lpcpu_idle_sleep_vote(ID_UFS, 0);

	if (ufshcd_is_runtime_pm(pm_op))
		return 0;

	if (host->in_suspend) {
		WARN_ON(1);
		return 0;
	}
	udelay(10); /* wait 10 us */

	mphy_sr_fix_store(hba);
	/* Step 13. check if the store is complete */
	ret = ufs_access_register_and_check(hba, UFS_CFG_SUSPEND);
	if (ret)
		return ret;

	hufs_disable_all_irq(hba);
	/* Step 14. clear bit0 of ufs_cfg_ram_ctrl */
	value = ufshcd_readl(hba, UFS_CFG_RAM_CTRL);
	value &= (~0x01);
	ufshcd_writel(hba, value, UFS_CFG_RAM_CTRL);

	/* Step 15. enable MPHY ISO */
	mphy_iso_enable(hba);

	/* Step 17. enable ip_rst_ufs_subsys, reset ufs sub-system */
	sys_ctrl_set_bits(host, BIT(IP_RST_UFS_SUBSYS), SOC_SCTRL_SCPERRSTEN1);

	/* Step 18. disable input clk of UFS sub-system */
	sys_ctrl_set_bits(host, GT_CLK_UFS_SUBSYS, SOC_SCTRL_SCPERDIS4);

	/* Step 19. enable reset of UFS sub-system CRG */
	sys_ctrl_set_bits(host, BIT(IP_RST_UFS_SUBSYS_CRG),
			  SOC_SCTRL_SCPERRSTEN1);

	/* Step 20. enable RET of dual-rail RAM, set DS =1 , SD = 0 */
	writel(0x00030002, SOC_ACTRL_AO_MEM_CTRL2_ADDR(host->actrl));

	pmic_write_reg(PMIC_CLK_UFS_EN_ADDR(0), 0);

	host->in_suspend = true;

	return 0;
}

static void ufs_sub_sys_crg_reset_release(struct ufs_hba *hba)
{
	u32 value;
	int retry = POLLING_RETRY;
	struct hufs_host *host = hba->priv;

	sys_ctrl_set_bits(host, BIT(IP_RST_UFS_SUBSYS_CRG),
			  SOC_SCTRL_SCPERRSTDIS1);
	while (retry--) {
		value = readl(SOC_SCTRL_SCPERRSTSTAT1_ADDR(host->sysctrl));
		if (!(value & BIT(IP_RST_UFS_SUBSYS_CRG)))
			return;
		udelay(1);
	}
	dev_err(hba->dev, "UFS Sub-sys CRG reset failed\n");
}

int hufs_main_resume(struct ufs_hba *hba, enum ufs_pm_op pm_op)
{
	int ret;
	u32 value;
	struct hufs_host *host = NULL;

	host = hba->priv;

	if (hba->host->is_emulator)
		goto out;

	/* set SOC_SCTRL_SCBAKDATA11_ADDR ufs bit to 1 when busy */
	if (!ufshcd_is_auto_hibern8_allowed(hba))
		lpcpu_idle_sleep_vote(ID_UFS, 1);

	if (!host->in_suspend)
		return 0;

	/* Step 1. ufs clk enable */
	pmic_write_reg(PMIC_CLK_UFS_EN_ADDR(0), 1);
	/* 250us to ensure the clk stable */
	udelay(PMIC_CLK_EN_WAIT_TIME);

	/* Step 3. disable MPHY ISO */
	mphy_iso_disable(hba);

	/* Step 4. disable RET of dual-rail RAM, DS = 0, SD = 0 */
	writel(0x00030000, SOC_ACTRL_AO_MEM_CTRL2_ADDR(host->actrl));

	/* Step 5. setup ufs sub-system clk to 238MHz */
	writel(0x003F0006, SOC_SCTRL_SCCLKDIV9_ADDR(host->sysctrl));
	udelay(1);

	/* Step 8. release the reset of UFS subsys CRG */
	ufs_sub_sys_crg_reset_release(hba);

	/* Step 9. Release the reset of UFS_SUBSYS */
	sys_ctrl_set_bits(host, BIT(IP_RST_UFS_SUBSYS), SOC_SCTRL_SCPERRSTDIS1);

	/* Step 10. Enable clk_ufs_subsys clock */
	writel(BIT(SOC_SCTRL_SCPEREN4_gt_clk_ufs_subsys_START),
	       SOC_SCTRL_SCPEREN4_ADDR(host->sysctrl));

	/* Step 11. release internal release of UFS Subsys */
	writel(0xFFFFFFFF, host->ufs_sys_ctrl + UFS_SYSCTRL_RESET_CTRL_OFFSET);

	/* Step 13. wait 500us */
	udelay(MPHY_PLL_LOCK_WAIT_TIME);

	hufs_enable_all_irq(hba);
	/*
	 * Step 1. and Step 5.
	 * request UFSHCI and MPHY to restore register from dual-rail RAM and
	 * check if the restore is complete
	 */
	ret = ufs_access_register_and_check(hba, UFS_CFG_RESUME);
	if (ret)
		return ret;

	/* Step 6. */
	value = ufshcd_readl(hba, UFS_CFG_RAM_CTRL);
	value &= (~UFS_CFG_RESUME);
	ufshcd_writel(hba, value, UFS_CFG_RAM_CTRL);

	/* Step 8. */
	hufs_uic_write_reg(hba, HIBERNATE_EXIT_MODE, HIBERNATE_EXIT_VAL);
	/*
	 * after harden power off in SR, dme_enable_intrs register will lost
	 * its value, restore it
	 */
	ufshcd_hufs_enable_unipro_intr(hba, DME_ENABLE_INTRS);

out:
	host->in_suspend = false;

	return 0;
}

int hufs_spare_resume(struct ufs_hba *hba, enum ufs_pm_op pm_op)
{
	int ret;
	u32 value;
	struct hufs_host *host = NULL;

	host = hba->priv;

	/* set SOC_SCTRL_SCBAKDATA11_ADDR ufs bit to 1 when busy */
	if (!ufshcd_is_auto_hibern8_allowed(hba))
		lpcpu_idle_sleep_vote(ID_UFS, 1);

	if (!host->in_suspend)
		return 0;

	pmic_write_reg(PMIC_CLK_UFS_EN_ADDR(0), 1);
	/* 250us to ensure the clk stable */
	udelay(250);

	/* Step 3. release the reset of UFS subsys CRG */
	ufs_sub_sys_crg_reset_release(hba);

	/* Step 6. disable RET of dual-rail RAM, DS = 0, SD = 0 */
	writel(0x00030000, SOC_ACTRL_AO_MEM_CTRL2_ADDR(host->actrl));

	/* Step 7. Reset UFS_SUBSYS */
	sys_ctrl_set_bits(host, BIT(IP_RST_UFS_SUBSYS), SOC_SCTRL_SCPERRSTEN1);

	/* Step 8. setup ufs sub-system clk to 224MHz */
	writel(0x003F0006, SOC_SCTRL_SCCLKDIV9_ADDR(host->sysctrl));
	udelay(1);

	/* Step 9. Release the reset of UFS_SUBSYS */
	sys_ctrl_set_bits(host, BIT(IP_RST_UFS_SUBSYS), SOC_SCTRL_SCPERRSTDIS1);

	/* Step 10. Enable clk_ufs_subsys clock */
	writel(BIT(SOC_SCTRL_SCPEREN4_gt_clk_ufs_subsys_START),
	       SOC_SCTRL_SCPEREN4_ADDR(host->sysctrl));

	/* Step 11. release internal release of UFS Subsys */
	writel(0xFFFFFFFF, host->ufs_sys_ctrl + UFS_SYSCTRL_RESET_CTRL_OFFSET);

	/* Step 12. disable UFS CFG clk */
	writel(0x00080000, host->ufs_sys_ctrl + UFS_SYSCTRL_CRG_CLK_OFFSET);

	/* Step 13. disable MPHY ISO */
	mphy_iso_disable(hba);

	/* Step 14. enable UFS CFG clk */
	writel(0x00080008, host->ufs_sys_ctrl + UFS_SYSCTRL_CRG_CLK_OFFSET);

	/* Step 15. wait for UFS MPHY PLL lock */
	udelay(MPHY_PLL_LOCK_WAIT_TIME);

	hufs_enable_all_irq(hba);
	/*
	 * Step 1. and Step 5.
	 * request UFSHCI and MPHY to restore register from dual-rail RAM and
	 * check if the restore is complete
	 */
	ret = ufs_access_register_and_check(hba, UFS_CFG_RESUME);
	if (ret)
		return ret;

	/* Step 6. */
	value = ufshcd_readl(hba, UFS_CFG_RAM_CTRL);
	value &= (~0x02);
	ufshcd_writel(hba, value, UFS_CFG_RAM_CTRL);

	/* Step 8. */
	hufs_uic_write_reg(hba, HIBERNATE_EXIT_MODE, 0x1);
	/*
	 * after harden power off in SR, dme_enable_intrs register will lost
	 * its value, restore it
	 */
	ufshcd_hufs_enable_unipro_intr(hba, DME_ENABLE_INTRS);

	host->in_suspend = false;

	return 0;
}

int hufs_resume(struct ufs_hba *hba, enum ufs_pm_op pm_op)
{
	int ret;
	struct hufs_host *host = NULL;

	host = hba->priv;

	if (host->caps & USE_MAIN_RESUME) {
		ret = hufs_main_resume(hba,pm_op);
	} else {
		ret = hufs_spare_resume(hba,pm_op);
	}

	return ret;
}

static void hufs_device_hw_reset_hybrid(struct ufs_hba *hba)
{
	struct hufs_host *host = hba->priv;

	if (likely(!(host->caps & USE_HUFS_MPHY_TC))) {
		ufs_sys_ctrl_writel(host, MASK_UFS_DEVICE_RESET | 0,
				    UFS_DEVICE_RESET_CTRL);
		mdelay(1);
	} else if (!is_v200_mphy(hba)) {
		ufs_i2c_writel(hba, (unsigned int)UFS_BIT_DEVICE_RSTDIS,
			       SC_RSTDIS);
		mdelay(1);
	} else {
		ufs_i2c_writel(hba, 0x20000, SC_RSTEN_V200);
		mdelay(2); /* wait 2 ms */
	}

	if (likely(!(host->caps & USE_HUFS_MPHY_TC))) {
		ufs_sys_ctrl_writel(host, MASK_UFS_DEVICE_RESET |
						  BIT_UFS_DEVICE_RESET,
				    UFS_DEVICE_RESET_CTRL);
	} else if (!is_v200_mphy(hba)) {
		ufs_i2c_writel(hba, (unsigned int)UFS_BIT_DEVICE_RSTEN,
			       SC_RSTEN);
	} else {
		ufs_i2c_writel(hba, 0x20000, SC_RSTDIS_V200);
	}
	mdelay(10); /* wait 10 ms */
}

void hufs_device_hw_reset(struct ufs_hba *hba)
{
	struct hufs_host *host = NULL;

	host = hba->priv;

	if (!ufshcd_is_hufs_hc(hba)) {
		hufs_device_hw_reset_hybrid(hba);
		return;
	}

	if (likely(!(host->caps & USE_HUFS_MPHY_TC))) {
		/*
		 * Enable device reset, bit[10] is rst_device_rst_n
		 * bit[26] is bitmask
		 */
		writel(0x04000000,
		       SOC_ACTRL_BITMSK_NONSEC_CTRL1_ADDR((host->actrl)));
		ufshcd_vops_vcc_power_on_off(hba);
		/* wait 2 ms */
		mdelay(2);
		/* Disable device reset */
		writel(0x04000400,
		       SOC_ACTRL_BITMSK_NONSEC_CTRL1_ADDR((host->actrl)));
	} else {
		if (!is_v200_mphy(hba)) {
			ufs_i2c_writel(hba, (unsigned int)UFS_BIT_DEVICE_RSTDIS,
				       SC_RSTDIS);
			mdelay(1);
			ufs_i2c_writel(hba, (unsigned int)UFS_BIT_DEVICE_RSTEN,
				       SC_RSTEN);
		} else {
			ufs_i2c_writel(hba, 0x20000, SC_RSTEN_V200);
			mdelay(2); /* wait 2 ms */
			ufs_i2c_writel(hba, 0x20000, SC_RSTDIS_V200);
		}
	}

	mdelay(10); /* wait 10 ms */
}

static void tx_rx_r_calc(struct ufs_hba *hba)
{
	u32 val = 0;

	hufs_uic_read_reg(hba, ATTR_MTX0(TX_DMY1), &val);
	val = val & MAX_TX_R;
	val = (val + TX_LEVEL) * R_MULTIPLIER;
	val = (val % R_DIVISOR) ? (val / R_DIVISOR + MIN_TX_R) :
				  (val / R_DIVISOR);
	val -= TX_LEVEL;
	if (val > MAX_TX_R) {
		dev_err(hba->dev, "new TX_R exceed MAX_TX_R\n");
		val = MAX_TX_R;
	}
	hufs_uic_write_reg(hba, ATTR_MTX0(TX_DMY1), val);
	hufs_uic_write_reg(hba, ATTR_MTX1(TX_DMY1), val);
	hufs_uic_write_reg(hba, DME_MPHYCFGUPDT, DME_MPHYCFGUPDT_EN);
	hufs_uic_read_reg(hba, ATTR_MRX0(RX_R_CAL), &val);
	val = val & MAX_RX_R;
	val = (val + RX_LEVEL) * R_MULTIPLIER;
	val = (val % R_DIVISOR > HALF_R_DIVISOR) ?
		      (val / R_DIVISOR + MIN_RX_R) :
		      (val / R_DIVISOR);
	val -= RX_LEVEL;
	if (val > MAX_RX_R) {
		dev_err(hba->dev, "new TX_R exceed MAX_TX_R\n");
		val = MAX_RX_R;
	}
	hufs_uic_write_reg(hba, ATTR_MRX0(RX_R_CAL), val);
	hufs_uic_write_reg(hba, ATTR_MRX1(RX_R_CAL), val);
}

static void cdr_reset(struct ufs_hba *hba)
{
	u32 val = 0;

	/* reset CDR to keep PMC to HS working well */
	hufs_uic_read_reg(hba, ATTR_MRX0(RX_AFE_CTRL_IV), &val);
	val |= CDR_RESET_EN;
	hufs_uic_write_reg(hba, ATTR_MRX0(RX_AFE_CTRL_IV), val);
	hufs_uic_write_reg(hba, ATTR_MRX1(RX_AFE_CTRL_IV), val);
}

static void ana_eq_slicer_en(struct ufs_hba *hba)
{
	u32 value = 0;

	hufs_uic_read_reg(hba, ATTR_MTX0(AFE_CONFIG_VIII), &value);
	/* BIT 0 is reg_afe_sl_en_eq */
	value |= BIT(0);
	hufs_uic_write_reg(hba, ATTR_MTX0(AFE_CONFIG_VIII), value);
	hufs_uic_write_reg(hba, ATTR_MTX1(AFE_CONFIG_VIII), value);
}

static void trailing_clocks_config(struct ufs_hba *hba)
{
	/* PA_TxTrailingClocks */
	hufs_uic_write_reg(hba, TX_TRAILING_CLOCKS_REG, TX_TRAILING_CLK_NUMS);
}

static void update_mphy_config(struct ufs_hba *hba)
{
	hufs_uic_write_reg(hba, DME_MPHYCFGUPDT, DME_MPHYCFGUPDT_EN);
}

static void saveconfig_handle(struct ufs_hba *hba)
{
	/*
	 * save config 62.5us is safe for ufs.
	 * 0x9529 = 0xFA(250)
	 * 250 * 250ns = 62.5us
	 */
	hufs_uic_write_reg(hba, VS_DDBUG_SAVE_CONFIG_TIME, SAVECFG_250NS);
	pr_err("Set Saveconfig time to 62.5us\n");
}

int hufs_link_startup_pre_change(struct ufs_hba *hba)
{
	int err = 0;
	const struct ufs_attr_cfg *cfg = NULL;
	struct hufs_host *host = NULL;

	host = hba->priv;

	dev_info(hba->dev, "%s ++\n", __func__);

	/* for FPGA hufs MPHY */
	if ((host->caps & USE_HUFS_MPHY_TC)) {
		if (is_v200_mphy(hba)) {
			cfg = hufs_mphy_v200_pre_link_attr;
			hufs_set_each_cfg_attr(hba, cfg);
			mdelay(40); /* wait 40 ms */
		} else {
			cfg = hufs_mphy_v120_pre_link_attr;
			hufs_set_each_cfg_attr(hba, cfg);
		}
	} else {
		/* for ASIC hufs MPHY and emu */
		cfg = hufs_mphy_v300_pre_link_attr;
		hufs_set_each_cfg_attr(hba, cfg);

		cdr_reset(hba);
		ana_eq_slicer_en(hba);
		trailing_clocks_config(hba);
		/*
		 * At this moment, we knew the info of devices,
		 * check save config time for ufs
		 */
		saveconfig_handle(hba);
		update_mphy_config(hba);

		err = wait_mphy_init_done(hba);
		if (err)
			return err;
		/* update TX/RX R base on the result of calibratioin */
		tx_rx_r_calc(hba);
		update_mphy_config(hba);
	}

	/* for hufs MPHY */
	if ((host->caps & USE_HUFS_MPHY_TC)) {
		if (is_v200_mphy(hba))
			hufs_mphy_v200_updata_fsm(hba, host);
		else
			hufs_mphy_updata_fsm_ocs5(hba, host);

		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL((u32)PA_ADAPTAFTERLRSTINPA_INIT, 0x0), 0x1);

		pr_info("%s --\n", __func__);

		return err;
	}
	pr_info("%s --\n", __func__);

	return err;
}

static void hufs_mphy_link_post_config(
	struct ufs_hba *hba, struct hufs_host *host)
{
	u32 tx_lane_num = 1;
	u32 rx_lane_num = 1;

	if (host->caps & USE_HUFS_MPHY_TC) {
		/*
		 * set the PA_TActivate to 128. need to check in ASIC
		 * H8's workaround
		 */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(PA_TACTIVATE, 0x0), 0x5);
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x80da, 0x0), 0x2d);

		ufshcd_dme_get(hba, UIC_ARG_MIB(PA_CONNECTEDTXDATALANES), &tx_lane_num);
		ufshcd_dme_get(hba, UIC_ARG_MIB(PA_CONNECTEDRXDATALANES), &rx_lane_num);

		/* RX_MC_PRESENT */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c2, 0x4), 0x0);
		if (tx_lane_num > 1 && rx_lane_num > 1)
			/* RX_MC_PRESENT */
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c2, 0x5), 0x0);
	}
}

void set_device_clk(struct ufs_hba *hba)
{
}

static void ufs_hcd_dme_set_value(struct ufs_hba *hba)
{
	u32 val = 0;

	ufshcd_dme_get(hba, UIC_ARG_MIB(PA_TXHSG1SYNCLENGTH), &val);
	/* sync length devided into four step 4B 4C 4D 4E */
	if (val < 0x4B)
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(PA_TXHSG1SYNCLENGTH, 0x0), 0x4B);

	ufshcd_dme_get(hba, UIC_ARG_MIB(PA_TXHSG2SYNCLENGTH), &val);
	if (val < 0x4C)
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(PA_TXHSG2SYNCLENGTH, 0x0), 0x4C);

	ufshcd_dme_get(hba, UIC_ARG_MIB(PA_TXHSG3SYNCLENGTH), &val);
	if (val < 0x4D)
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(PA_TXHSG3SYNCLENGTH, 0x0), 0x4D);

	ufshcd_dme_get(hba, UIC_ARG_MIB(PA_TXHSG4SYNCLENGTH), &val);
	if (val < 0x4E)
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(PA_TXHSG4SYNCLENGTH, 0x0), 0x4E);
}

int hufs_link_startup_post_change(struct ufs_hba *hba)
{
	struct hufs_host *host = NULL;
	const struct ufs_attr_cfg *cfg = NULL;
	u32 value = 0;
	u32 value_bak = 0;

	host = hba->priv;

	pr_info("%s ++\n", __func__);

	/* for hufs asic mphy and emu, use USE_HUFS_MPHY_ASIC on ASIC later */
	if (!(host->caps & USE_HUFS_MPHY_TC) || hba->host->is_emulator) {
		cfg = hufs_mphy_v300_post_link_attr;
		hufs_set_each_cfg_attr(hba, cfg);
		if (!get_peer_rx_activate_time(hba, &value))
			/* add 0x3 * 100us time compensation for tactivate */
			hufs_uic_write_reg(hba, PA_TACTIVATE, value + 0x3);
	}
	/* boost sequlch voltage to fix early exit h8 bug */
	hufs_uic_write_reg(hba, ATTR_MRX0(RX_SQ_VREF), 0);
	hufs_uic_write_reg(hba, ATTR_MRX1(RX_SQ_VREF), 0);

	/*
	 * boost Sense Amp LDO Voltage to fix phy error bug,
	 * value 3 means 850mv
	 */
	if (hba->caps & USE_MPHY_LDO_VOL_WORKROUND) {
		hufs_uic_write_reg(hba, ATTR_MRX0(RX_AFE_CTRL_VII), 3);
		hufs_uic_write_reg(hba, ATTR_MRX1(RX_AFE_CTRL_VII), 3);
	}

	if ((host->caps & USE_HUFS_MPHY_TC)) {
		if (is_v200_mphy(hba)) {
			hufs_mphy_v200_link_post_config(hba, host);
			cfg = hufs_mphy_v200_post_link_attr;
			hufs_set_each_cfg_attr(hba, cfg);

			ufs_hcd_dme_set_value(hba);

			ufshcd_dme_get(hba, UIC_ARG_MIB_SEL(0x00ba, 0x0),
				       &value_bak);
			ufshcd_dme_get(hba, UIC_ARG_MIB_SEL(0x00ae, 0x0),
				       &value);
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00ae, 0x0),
				       value | MPHY_BIT_REG_VCO_SW_VCAL);
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00ba, 0x0),
				       value_bak);
		} else {
			hufs_mphy_link_post_config(hba, host);
			cfg = hufs_mphy_v120_post_link_attr;
			hufs_set_each_cfg_attr(hba, cfg);
		}
	}

	unipro_clk_auto_gate_enable(hba);

	/* select debug counter0 mask0, bit 0: 1K symbol received */
	ufshcd_dme_set(hba, UIC_ARG_MIB((u32)VS_DEBUG_COUNTER0_MASK0), BIT(0));
	/* clear debug counter0 and enable it */
	ufshcd_dme_set(hba, UIC_ARG_MIB((u32)DEBUGCOUNTER_CLR),
		       BIT_DBG_CNT0_CLR);
	ufshcd_dme_set(hba, UIC_ARG_MIB((u32)DEBUGCOUNTER_EN), BIT_DBG_CNT0_EN);

	pr_info("%s --\n", __func__);

	return 0;
}

/**
 * Soc init will reset host controller, all register value will lost
 * including memory address, doorbell and AH8 AGGR
 */
void hufs_full_reset(struct ufs_hba *hba)
{
#ifdef CONFIG_HUAWEI_UFS_DSM
	dsm_ufs_disable_volt_irq(hba);
#endif
	disable_irq(hba->irq);

	/*
	 * Cancer platform need a full reset when error handler occurs.
	 * If a request sending in ufshcd_queuecommand passed through
	 * ufshcd_state check. And eh may reset host controller, a NOC
	 * error happens.
	 */
	msleep(1000); /* 1000ms sleep is enough for waiting those requests. */

	ufs_soc_init(hba);

	enable_irq(hba->irq);
#ifdef CONFIG_HUAWEI_UFS_DSM
	dsm_ufs_enable_volt_irq(hba);
#endif
}

void v300_gears_config(struct ufs_hba *hba, u32 gear_rx)
{
	const struct ufs_attr_cfg *cfg = NULL;
	if ((gear_rx >= UFS_HS_G1) && (gear_rx <= UFS_HS_G4))
		cfg = hufs_mphy_v300_pre_pmc_attr[gear_rx];
	else
		cfg = hufs_mphy_v300_pre_pmc_attr[0];
	hufs_set_each_cfg_attr(hba, cfg);
}

/* compatible with old platform */
void hufs_pwr_change_pre_change(
	struct ufs_hba *hba, struct ufs_pa_layer_attr *dev_req_params)
{
	u32 value;
	struct hufs_host *host = NULL;

	host = hba->priv;

	if (ufshcd_is_hufs_hc(hba)) {
		ufs_pwr_change_pre_change(hba, dev_req_params);
		return;
	}

	pr_info("%s ++\n", __func__);
#ifdef CONFIG_DFX_DEBUG_FS
	pr_info("device manufacturer_id is 0x%x\n", hba->manufacturer_id);
#endif
	/*
	 * ARIES platform need to set SaveConfigTime to 0x13, and change sync
	 * length to maximum value
	 */
	/* VS_DebugSaveConfigTime */
	ufshcd_dme_set(hba, UIC_ARG_MIB((u32)0xD0A0), 0x13);
	ufshcd_dme_set(hba, UIC_ARG_MIB((u32)PA_TXHSG1SYNCLENGTH),
		       0x4f); /* g1 sync length */
	ufshcd_dme_set(hba, UIC_ARG_MIB((u32)PA_TXHSG2SYNCLENGTH),
		       0x4f); /* g2 sync length */
	ufshcd_dme_set(hba, UIC_ARG_MIB((u32)PA_TXHSG3SYNCLENGTH),
		       0x4f); /* g3 sync length */

	ufshcd_dme_get(hba, UIC_ARG_MIB(PA_HIBERN8TIME), &value);
	if (value < 0xA)
		/* PA_Hibern8Time */
		ufshcd_dme_set(hba, UIC_ARG_MIB((u32)PA_HIBERN8TIME), 0xA);
	ufshcd_dme_set(hba, UIC_ARG_MIB((u32)PA_TACTIVATE), 0xA); /* PA_Tactivate */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xd085, 0x0), 0x01);

	ufshcd_dme_set(hba, UIC_ARG_MIB(PA_TXSKIP), 0x0); /* PA_TxSkip */

	/* PA_PWRModeUserData0 = 8191, default is 0 */
	ufshcd_dme_set(hba, UIC_ARG_MIB(PA_PWRMODEUSERDATA0), 8191);
	/* PA_PWRModeUserData1 = 65535, default is 0 */
	ufshcd_dme_set(hba, UIC_ARG_MIB(PA_PWRMODEUSERDATA1), 65535);
	/* PA_PWRModeUserData2 = 32767, default is 0 */
	ufshcd_dme_set(hba, UIC_ARG_MIB(PA_PWRMODEUSERDATA2), 32767);
	/* DME_FC0ProtectionTimeOutVal = 8191, default is 0 */
	ufshcd_dme_set(hba, UIC_ARG_MIB((u32)DME_FC0_PROTECTION_TIMEOUT_VAL), 8191);
	/* DME_TC0ReplayTimeOutVal = 65535, default is 0 */
	ufshcd_dme_set(hba, UIC_ARG_MIB((u32)DME_TC0_REPLAY_TIMEOUT_VAL), 65535);
	/* DME_AFC0ReqTimeOutVal = 32767, default is 0 */
	ufshcd_dme_set(hba, UIC_ARG_MIB((u32)DME_AFC0_REQ_TIMEOUT_VAL), 32767);
	/* PA_PWRModeUserData3 = 8191, default is 0 */
	ufshcd_dme_set(hba, UIC_ARG_MIB(PA_PWRMODEUSERDATA3), 8191);
	/* PA_PWRModeUserData4 = 65535, default is 0 */
	ufshcd_dme_set(hba, UIC_ARG_MIB(PA_PWRMODEUSERDATA4), 65535);
	/* PA_PWRModeUserData5 = 32767, default is 0 */
	ufshcd_dme_set(hba, UIC_ARG_MIB(PA_PWRMODEUSERDATA5), 32767);
	/* DME_FC1ProtectionTimeOutVal = 8191, default is 0 */
	ufshcd_dme_set(hba, UIC_ARG_MIB((u32)DME_FC1_PROTECTION_TIMEOUT_VAL), 8191);
	/* DME_TC1ReplayTimeOutVal = 65535, default is 0 */
	ufshcd_dme_set(hba, UIC_ARG_MIB((u32)DME_TC1_REPLAY_TIMEOUT_VAL), 65535);
	/* DME_AFC1ReqTimeOutVal = 32767, default is 0 */
	ufshcd_dme_set(hba, UIC_ARG_MIB((u32)DME_AFC1_REQ_TIMEOUT_VAL), 32767);

	if ((host->caps & USE_HUFS_MPHY_TC)) {
		if (is_v200_mphy(hba))
			hufs_mphy_v200_pwr_change_pre_config(hba, host);
	}
	pr_info("%s --\n", __func__);
}

void hufs_pre_hce_notify(struct ufs_hba *hba)
{
	struct hufs_host *host = (struct hufs_host *)hba->priv;

	BUG_ON(!host->pericrg || !host->ufs_sys_ctrl || !host->pctrl ||
		!host->sysctrl || !host->pmctrl);

	if (ufshcd_is_hufs_hc(host->hba))
		BUG_ON(!host->actrl);

	return;
}

int ufs_get_hufs_hc(struct hufs_host *host)
{
	struct device_node *np = NULL;

	if (!ufshcd_is_hufs_hc(host->hba))
		return 0;

	np = of_find_compatible_node(NULL, NULL, "hisilicon,actrl");
	if (!np) {
		dev_err(host->hba->dev,
			"can't find device node \"hisilicon,actrl\"\n");
		return -ENXIO;
	}
	host->actrl = of_iomap(np, 0);
	if (!host->actrl) {
		dev_err(host->hba->dev, "actrl iomap error!\n");
		return -ENOMEM;
	}

	np = of_find_compatible_node(NULL, NULL, "hisilicon,hsdt_crg");
	if (!np) {
		dev_err(host->hba->dev,
			"can't find device node hisilicon,hsdt_crg\n");
		return -ENXIO;
	}

	host->hsdt_crg = of_iomap(np, 0);
	if (!host->hsdt_crg) {
		dev_err(host->hba->dev, "hsdt_crg iomap error!\n");
		return -ENOMEM;
	}
#ifdef CONFIG_SCSI_UFS_INTR_HUB
	np = of_find_compatible_node(NULL, NULL, "hisilicon,hsdt0_intr_hub");
	if (!np) {
		dev_err(host->hba->dev,
			"can't find device node hisilicon,hsdt0_intr_hub\n");
		return -ENXIO;
	}

	host->hsdt0_intr_hub = of_iomap(np, 0);
	if (!host->hsdt0_intr_hub) {
		dev_err(host->hba->dev, "hsdt0_intr_hub iomap error!\n");
		return -ENOMEM;
	}
#endif
	return 0;
}

void ufshcd_idle_auto_gating(struct ufs_hba *hba)
{
	ufshcd_writel(hba, UFSHC_CLOCK_GATING_CONFIG, UFS_BLOCK_CG_CFG);
}

void hufs_txpost_enable(struct ufs_pa_layer_attr *dev_req_params,u32 *value)
{
	UNUSED(dev_req_params);
	UNUSED(value);
	return;
}

void hufs_mphy_tactivate_adapt(struct ufs_hba *hba)
{
	check_pa_granularity(hba);
}
