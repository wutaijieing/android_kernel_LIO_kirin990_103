/*
 * dpm_kernel.c
 *
 * freqdump test
 *
 * Copyright (C) 2020-2020 Huawei Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include "luna/dpm_kernel.h"

void dpm_common_platform_init(void)
{
	g_calc_monitor_time_media_hook = NULL;

	return;
}

/*lint -e485 -e651*/
struct monitor_dump_info g_monitor_info = {
	.monitor_delay = 0,
	.mdev[CC712] = { "CC712", 0, 0, 0, 0 },
	.mdev[PCIE0_CTRL_PM_LINKST_IN_L0S] = { "PCIE0_CTRL_PM_LINKST_IN_L0S",
		0, 0, 0, 0 },
	.mdev[PCIE0_CTRL_PM_LINKST_IN_L1] = { "PCIE0_CTRL_PM_LINKST_IN_L1",
		0, 0, 0, 0 },
	.mdev[PCIE0_CTRL_PM_LINKST_IN_L1SUB] = { "PCIE0_CTRL_PM_LINKST_IN_L1SUB",
		0, 0, 0, 0 },
	.mdev[USB_20PHY_WORK] = { "USB_20PHY_WORK", 0, 0, 0, 0 },
	.mdev[USB2_PHY_SLEEP_MOM] = { "USB2_PHY_SLEEP_MOM", 0, 0, 0, 0 },
	.mdev[USB2_PHY_SUSPEND_MOM] = { "USB2_PHY_SUSPEND_MOM", 0, 0, 0, 0 },
	.mdev[DDRC_LP_IND0] = { "DDRC_LP_IND0", 0, 0, 0, 0 },
	.mdev[DDRC_LP_IND1] = { "DDRC_LP_IND1", 0, 0, 0, 0 },
	.mdev[AICORE_CUBE8] = { "AICORE_CUBE8", 0, 0, 0, 0 },
	.mdev[AICORE_MET8] = { "AICORE_MET8", 0, 0, 0, 0 },
	.mdev[AICOREVEC8] = { "AICOREVEC8", 0, 0, 0, 0 },
	.mdev[AICORE_BIU8] = { "AICORE_BIU8", 0, 0, 0, 0 },
	.mdev[CLK_AIC_TOP_EN] = { "CLK_AIC_TOP_EN", 0, 0, 0, 0 }, /* zone peri0 */
	.mdev[SYS_BUS] = { "SYS_BUS", 0, 0, 0, 0 },
	.mdev[LPM3] = { "LPM3", 0, 0, 0, 0 },
	.mdev[DMA_BUS] = { "DMA_BUS", 0, 0, 0, 0 },
	.mdev[SYSBUS_AUTODIV] = { "SYSBUS_AUTODIV", 0, 0, 0, 0 },
	.mdev[LPMCU_AUTODIV] = { "LPMCU_AUTODIV", 0, 0, 0, 0 },
	.mdev[CFGBUS_AUTODIV] = { "CFGBUS_AUTODIV", 0, 0, 0, 0 },
	.mdev[DMABUS_AUTODIV] = { "DMABUS_AUTODIV", 0, 0, 0, 0 },
	.mdev[VDEC_AUTODIV] = { "VDEC_AUTODIV", 0, 0, 0, 0 },
	.mdev[VENC_AUTODIV] = { "VENC_AUTODIV", 0, 0, 0, 0 },
	.mdev[VENC2_AUTODIV] = { "VENC2_AUTODIV", 0, 0, 0, 0 },
	.mdev[PERI_AUTODIV_STAT11] = { "PERI_AUTODIV_STAT11", 0, 0, 0, 0 },
	.mdev[PERI_AUTODIV_STAT13] = { "PERI_AUTODIV_STAT13", 0, 0, 0, 0 },
	.mdev[PERI_AUTODIV_STAT14] = { "PERI_AUTODIV_STAT14", 0, 0, 0, 0 },
	.mdev[PERI_AUTODIV_STAT15] = { "PERI_AUTODIV_STAT15", 0, 0, 0, 0 },
	.mdev[DMSS_DDR_BUSY] = { "DMSS_DDR_BUSY", 0, 0, 0, 0 },
	.mdev[DMSS_BUSY] = { "DMSS_BUSY", 0, 0, 0, 0 },
	.mdev[PERI_AUTODIV_STAT16] = { "PERI_AUTODIV_STAT16", 0, 0, 0, 0 },
	.mdev[PERI_AUTODIV_STAT17] = { "PERI_AUTODIV_STAT17", 0, 0, 0, 0 },
	.mdev[PERI_AUTODIV_STAT18] = { "PERI_AUTODIV_STAT18", 0, 0, 0, 0 },
	.mdev[MIDDLE0_SVFD] = { "MIDDLE0_SVFD", 0, 0, 0, 0 },
	.mdev[MIDDLE0_IDLE] = { "MIDDLE0_IDLE", 0, 0, 0, 0 },
	.mdev[LITTLE_SVFD] = { "LITTLE_SVFD", 0, 0, 0, 0 },
	.mdev[L3_SVFD] = { "L3_SVFD", 0, 0, 0, 0 },
	.mdev[LITTLE_IDLE] = { "LITTLE_IDLE", 0, 0, 0, 0 },
	.mdev[GPU_IDLE] = { "GPU_IDLE", 0, 0, 0, 0 },
	.mdev[L3_IDLE] = { "L3_IDLE", 0, 0, 0, 0 },		/* zone peri1 */
	.mdev[VIVOBUS_IDLE_FLAG] = { "VIVOBUS_IDLE_FLAG", 0, 0, 0, 0 },
	.mdev[RCH0] = { "RCH0", 0, 0, 0, 0 },
	.mdev[RCH1] = { "RCH1", 0, 0, 0, 0 },
	.mdev[RCH2] = { "RCH2", 0, 0, 0, 0 },
	.mdev[RCH3] = { "RCH3", 0, 0, 0, 0 },
	.mdev[RCH4] = { "RCH4", 0, 0, 0, 0 },
	.mdev[RCH5] = { "RCH5", 0, 0, 0, 0 },
	.mdev[RCH6] = { "RCH6", 0, 0, 0, 0 },
	.mdev[RCH7] = { "RCH7", 0, 0, 0, 0 },
	.mdev[WCH0] = { "WCH0", 0, 0, 0, 0 },
	.mdev[WCH1] = { "WCH1", 0, 0, 0, 0 },
	.mdev[RCH8] = { "RCH8", 0, 0, 0, 0 },
	.mdev[ARSR_PRE] = { "ARSR_PRE", 0, 0, 0, 0 },
	.mdev[ARSR_POST] = { "ARSR_POST", 0, 0, 0, 0 },
	.mdev[HIACE] = { "HIACE", 0, 0, 0, 0 },
	.mdev[DPP] = { "DPP", 0, 0, 0, 0 },
	.mdev[IFBC] = { "IFBC", 0, 0, 0, 0 },
	.mdev[DBUF] = { "DBUF", 0, 0, 0, 0 },
	.mdev[DBCU] = { "DBCU", 0, 0, 0, 0 },
	.mdev[CMDLIST] = { "CMDLIST", 0, 0, 0, 0 },
	.mdev[MCTL] = { "MCTL", 0, 0, 0, 0 },
	.mdev[ISP_IM_BUSY0] = { "ISP_IM_BUSY0", 0, 0, 0, 0 },
	.mdev[ISP_IM_BUSY1] = { "ISP_IM_BUSY1", 0, 0, 0, 0 },
	.mdev[ISP_IM_BUSY2] = { "ISP_IM_BUSY2", 0, 0, 0, 0 },
	.mdev[ISP_IM_BUSY3] = { "ISP_IM_BUSY3", 0, 0, 0, 0 },
	.mdev[ISP_BUSY] = { "ISP_BUSY", 0, 0, 0, 0 },		/* zone media0 */
	.mdev[IVP_AXI_BUS] = { "IVP_AXI_BUS", 0, 0, 0, 0 },
	.mdev[IVP_DSP] = { "IVP_DSP", 0, 0, 0, 0 },
	.mdev[VCODECBUS] = { "VCODECBUS", 0, 0, 0, 0 },
	.mdev[VENC] = { "VENC", 0, 0, 0, 0 },
	.mdev[BUSY_TQITQ] = { "BUSY_TQITQ", 0, 0, 0, 0 },
	.mdev[BUSY_IME] = { "BUSY_IME", 0, 0, 0, 0 },
	.mdev[BUSY_FME] = { "BUSY_FME", 0, 0, 0, 0 },
	.mdev[BUSY_MRG] = { "BUSY_MRG", 0, 0, 0, 0 },
	.mdev[BUSY_SAO] = { "BUSY_SAO", 0, 0, 0, 0 },
	.mdev[BUSY_SEL] = { "BUSY_SEL", 0, 0, 0, 0 },
	.mdev[BUSY_PME] = { "BUSY_PME", 0, 0, 0, 0 },
	.mdev[BUSY_DBLK] = { "BUSY_DBLK", 0, 0, 0, 0 },
	.mdev[BUSY_INTRA] = { "BUSY_INTRA", 0, 0, 0, 0 },
	.mdev[VDEC] = { "VDEC", 0, 0, 0, 0 },
	.mdev[SED_IDLE] = { "SED_IDLE", 0, 0, 0, 0 },
	.mdev[PMV_IDLE] = { "PMV_IDLE", 0, 0, 0, 0 },
	.mdev[PRC_IDLE] = { "PRC_IDLE", 0, 0, 0, 0 },
	.mdev[PRF_IDLE] = { "PRF_IDLE", 0, 0, 0, 0 },
	.mdev[ITRANS_IDLE] = { "ITRANS_IDLE", 0, 0, 0, 0 },
	.mdev[RCN_IDLE] = { "RCN_IDLE", 0, 0, 0, 0 },
	.mdev[DBLK_SAO_IDLE] = { "DBLK_SAO_IDLE", 0, 0, 0, 0 },
	.mdev[CMP_IDLE] = { "CMP_IDLE", 0, 0, 0, 0 },
	.mdev[HIEPS2LM_BUSY0] = { "HIEPS2LM_BUSY0", 0, 0, 0, 0 },
	.mdev[HIEPS2LM_BUSY1] = { "HIEPS2LM_BUSY1", 0, 0, 0, 0 },
	.mdev[HIEPS2LM_BUSY2] = { "HIEPS2LM_BUSY2", 0, 0, 0, 0 }, /* zone media1 */
	.mdev[ASP_HIFI] = { "ASP_HIFI", 0, 0, 0, 0 },
	.mdev[ASP_BUS] = { "ASP_BUS", 0, 0, 0, 0 },
	.mdev[ASP_NOC_PENDINGTRANS] = { "ASP_NOC_PENDINGTRANS", 0, 0, 0, 0 },
	.mdev[IOMCU] = { "IOMCU", 0, 0, 0, 0 },
	.mdev[IOMCU_NOC_PENDINGTRANS] = { "IOMCU_NOC_PENDINGTRANS", 0, 0, 0, 0 },
	.mdev[UFSBUS] = { "UFSBUS", 0, 0, 0, 0 },
	.mdev[UFSBUS_IDLE] = { "UFSBUS_IDLE", 0, 0, 0, 0 },
	.mdev[AOBUS_STAT] = { "AOBUS_STAT", 0, 0, 0, 0 },
	.mdev[PLL_AUTODIV] = { "PLL_AUTODIV", 0, 0, 0, 0 },
	.mdev[FLL_AUTODIV] = { "FLL_AUTODIV", 0, 0, 0, 0 },
	.mdev[UFSBUS_STAT] = { "UFSBUS_STAT", 0, 0, 0, 0 },
	.mdev[AO_AUTODIV_STAT4] = { "AO_AUTODIV_STAT4", 0, 0, 0, 0 },
	.mdev[AO_AUTODIV_STAT5] = { "AO_AUTODIV_STAT5", 0, 0, 0, 0 },
	.mdev[AO_AUTODIV_STAT6] = { "AO_AUTODIV_STAT6", 0, 0, 0, 0 },
	.mdev[AO_AUTODIV_STAT7] = { "AO_AUTODIV_STAT7", 0, 0, 0, 0 },
	.mdev[AO_AUTODIV_STAT8] = { "AO_AUTODIV_STAT8", 0, 0, 0, 0 },
	.mdev[PDC_SLEEP] = { "PDC_SLEEP", 0, 0, 0, 0 },
	.mdev[PDC_DEEP_SLEEP] = { "PDC_DEEP_SLEEP", 0, 0, 0, 0 },
	.mdev[PDC_SLOW] = { "PDC_SLOW", 0, 0, 0, 0 },
	.mdev[LP_STAT_S0] = { "LP_STAT_S0", 0, 0, 0, 0 },
	.mdev[LP_STAT_S1] = { "LP_STAT_S1", 0, 0, 0, 0 },
	.mdev[LP_STAT_S2] = { "LP_STAT_S2", 0, 0, 0, 0 },
	.mdev[LP_STAT_S3] = { "LP_STAT_S3", 0, 0, 0, 0 },
	.mdev[LP_STAT_S4] = { "LP_STAT_S4", 0, 0, 0, 0 },		/* zone ao */
};

/*lint +e485 +e651*/
