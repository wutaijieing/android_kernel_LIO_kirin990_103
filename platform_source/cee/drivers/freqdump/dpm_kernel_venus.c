/*
 * dpm_kernel_venus.c
 *
 * dpm
 *
 * Copyright (C) 2017-2020 Huawei Technologies Co., Ltd.
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
#include "dpm_kernel_venus.h"
#include "soc_pmctrl_interface.h"
#include "soc_acpu_baseaddr_interface.h"

static void __iomem *g_dpm_peri_volt_virt_addr;
static phys_addr_t g_dpm_peri_volt_phy_addr;

static void calc_monitor_time_media_venus(int index, u32 media_pclk,
				   u64 *busy_nor, u64 *busy_low,
				   unsigned long monitor_delay)
{
	u32 peri_volt;

	if (busy_nor == NULL || busy_low == NULL) {
		pr_err("%s, busy_nor or busy_low is NULL", __func__);
		return;
	}

	if (monitor_delay == 0 || media_pclk == 0) {
		pr_err("monitor_delay or media_pclk is zero-error");
		return;
	}

	peri_volt = readl(g_dpm_peri_volt_virt_addr);
	if ((peri_volt & PERI_VDD_MASK) >> PERI_VDD_SHIFT == PERI_VDD_0_65V) {
		*busy_nor = ((u64)(g_dpm_loadmonitor_data_peri[MAX_PERI_DEVICE + index] /
			     media_pclk) * 1000000) / monitor_delay;
		*busy_low = *busy_nor + ((u64)(g_dpm_loadmonitor_data_peri[MAX_PERI_DEVICE * 2 + index] /
					 media_pclk) * 1000000) / monitor_delay;
		*busy_nor = 0;
	} else {
		*busy_nor = ((u64)(g_dpm_loadmonitor_data_peri[MAX_PERI_DEVICE + index] /
			     media_pclk) * 1000000) / monitor_delay;
		*busy_low = ((u64)(g_dpm_loadmonitor_data_peri[MAX_PERI_DEVICE * 2 + index] /
			     media_pclk) * 1000000) / monitor_delay;
	}
}

void dpm_common_platform_init(void)
{
	g_calc_monitor_time_media_hook = calc_monitor_time_media_venus;

	g_dpm_peri_volt_phy_addr = (phys_addr_t)(SOC_PMCTRL_PERI_CTRL4_ADDR(SOC_ACPU_PMC_BASE_ADDR));
	g_dpm_peri_volt_virt_addr = ioremap(g_dpm_peri_volt_phy_addr, sizeof(int));
	if (g_dpm_peri_volt_virt_addr == NULL) {
		pr_err("dpm_peri_volt ioremap failed!");
		return;
	}
}

struct monitor_dump_info g_monitor_info = {
	.monitor_delay = 0,
	.mdev[SYS_BUS] = {"SYS_BUS", 0, 0, 0, 0 },
	.mdev[LPM3] = {"LPM3", 0, 0, 0, 0 },
	.mdev[CFG_BUS] = {"CFG_BUS", 0, 0, 0, 0 },
	.mdev[DMA_BUS] = {"DMA_BUS", 0, 0, 0, 0 },
	.mdev[DMSS_DDR] = {"DMSS_DDR", 0, 0, 0, 0 },
	.mdev[DMSS] = {"DMSS", 0, 0, 0, 0 },
	.mdev[PERI_AUTODIV] = { "PERI_AUTODIV", 0, 0, 0, 0 },
	.mdev[DDRC_IND0] = { "DDRC_IND0", 0, 0, 0, 0 },
	.mdev[DDRC_IND1] = { "DDRC_IND1", 0, 0, 0, 0 },
	.mdev[DDRC_IND2] = { "DDRC_IND2", 0, 0, 0, 0 },
	.mdev[DDRC_IND3] = { "DDRC_IND3", 0, 0, 0, 0 },
	.mdev[MODEM_CCPU] = { "MODEM_CCPU", 0, 0, 0, 0 },
	.mdev[MODEM_DFC] = { "MODEM_DFC", 0, 0, 0, 0 },
	.mdev[MODEM_DSP0] = { "MODEM_DSP0", 0, 0, 0, 0 },
	.mdev[MODEM_GLBUS] = { "MODEM_GLBUS", 0, 0, 0, 0 },
	.mdev[MODEM_CIPHER] = { "MODEM_CIPHER", 0, 0, 0, 0 },
	.mdev[MODEM_GUC_DMA] = { "MODEM_GUC_DMA", 0, 0, 0, 0 },
	.mdev[MODEM_TL_DMA] = { "MODEM_TL_DMA", 0, 0, 0, 0 },
	.mdev[MODEM_EDMA1] = { "MODEM_EDMA1", 0, 0, 0, 0 },
	.mdev[MODEM_EDMA0] = { "MODEM_EDMA0", 0, 0, 0, 0 },
	.mdev[MODEM_UPACC] = { "MODEM_UPACC", 0, 0, 0, 0 },
	.mdev[MODEM_HDLC] = { "MODEM_HDLC", 0, 0, 0, 0 },
	.mdev[MODEM_CICOM0] = { "MODEM_CICOM0", 0, 0, 0, 0 },
	.mdev[MODEM_CICOM1] = { "MODEM_CICOM1", 0, 0, 0, 0 },
	.mdev[MMC0_PERIBUS] = { "MMC0_PERIBUS", 0, 0, 0, 0 },
	.mdev[MMC1_PERIBUS] = { "MMC1_PERIBUS", 0, 0, 0, 0 },
	.mdev[PM_LINKST_I0] = { "PM_LINKST_I0", 0, 0, 0, 0 },
	.mdev[PM_LINKST_I1] = { "PM_LINKST_I1", 0, 0, 0, 0 },
	.mdev[PM_LINKST_I1SUB] = { "PM_LINKST_I1SUB", 0, 0, 0, 0 },
	.mdev[CC712] = { "CC712", 0, 0, 0, 0 },
	.mdev[SYSBUS_AUTODIV] = { "SYSBUS_AUTODIV", 0, 0, 0, 0 },
	.mdev[BIG_SVFD] = { "BIG_SVFD", 0, 0, 0, 0 },
	.mdev[MID_SVFD] = { "MID_SVFD", 0, 0, 0, 0 },
	.mdev[LITTLE_SVFD] = { "LITTLE_SVFD", 0, 0, 0, 0 },
	.mdev[GPU_SVFD] = { "GPU_SVFD", 0, 0, 0, 0 },
	.mdev[L3_SVFD] = { "L3_SVFD", 0, 0, 0, 0 },
	.mdev[BIG_IDLE] = { "BIG_IDLE", 0, 0, 0, 0 },
	.mdev[MID_IDLE] = { "MID_IDLE", 0, 0, 0, 0 },
	.mdev[LITTLE_IDLE] = { "LITTLE_IDLE", 0, 0, 0, 0 },
	.mdev[GPU_IDLE] = { "GPU_IDLE", 0, 0, 0, 0 },
	.mdev[L3_IDLE] = { "L3_IDLE", 0, 0, 0, 0 },
	.mdev[LPM3_AUTODIV] = { "LPM3_AUTODIV", 0, 0, 0, 0 },
	.mdev[VENC_AUTODIV] = { "VENC_AUTODIV", 0, 0, 0, 0 },
	.mdev[CFGBUS_AUTODIV] = { "CFGBUS_AUTODIV", 0, 0, 0, 0 },
	.mdev[DMABUS_AUTODIV] = { "DMABUS_AUTODIV", 0, 0, 0, 0 },
	.mdev[VDEC_AUTODIV] = { "VDEC_AUTODIV", 0, 0, 0, 0 },
	.mdev[MMC0_AUTODIV] = { "MMC0_AUTODIV", 0, 0, 0, 0 },
	.mdev[MMC1_AUTODIV] = { "MMC1_AUTODIV", 0, 0, 0, 0 },
	.mdev[ICS_AUTODIV] = { "ICS_AUTODIV", 0, 0, 0, 0 },
	.mdev[IVP_AXI] = { "IVP_AXI", 0, 0, 0, 0 },
	.mdev[IVP_DSP] = { "IVP_DSP", 0, 0, 0, 0 },
	.mdev[VIVO_BUS] = { "VIVO_BUS", 0, 0, 0, 0 },
	.mdev[MDCOM] = { "MDCOM", 0, 0, 0, 0 },
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
	.mdev[ICS2_IM_BUSY0] = { "ICS2_IM_BUSY0", 0, 0, 0, 0 },
	.mdev[ICS2_IM_BUSY1] = { "ICS2_IM_BUSY1", 0, 0, 0, 0 },
	.mdev[ICS2_IM_BUSY2] = { "ICS2_IM_BUSY2", 0, 0, 0, 0 },
	.mdev[ICS2_IM_BUSY3] = { "ICS2_IM_BUSY3", 0, 0, 0, 0 },
	.mdev[ICS_BUSY0] = { "ICS_BUSY0", 0, 0, 0, 0 },
	.mdev[ICS_BUSY1] = { "ICS_BUSY1", 0, 0, 0, 0 },
	.mdev[VCODECBUS] = { "VCODECBUS", 0, 0, 0, 0 },
	.mdev[VENC] = { "VENC", 0, 0, 0, 0 },
	.mdev[VENC_IM_BUSY0] = { "VENC_IM_BUSY0", 0, 0, 0, 0 },
	.mdev[VENC_IM_BUSY1] = { "VENC_IM_BUSY1", 0, 0, 0, 0 },
	.mdev[VENC_IM_BUSY2] = { "VENC_IM_BUSY2", 0, 0, 0, 0 },
	.mdev[VENC_IM_BUSY3] = { "VENC_IM_BUSY3", 0, 0, 0, 0 },
	.mdev[VENC_IM_BUSY4] = { "VENC_IM_BUSY4", 0, 0, 0, 0 },
	.mdev[VENC_IM_BUSY5] = { "VENC_IM_BUSY5", 0, 0, 0, 0 },
	.mdev[VENC_IM_BUSY6] = { "VENC_IM_BUSY6", 0, 0, 0, 0 },
	.mdev[VENC_IM_BUSY7] = { "VENC_IM_BUSY7", 0, 0, 0, 0 },
	.mdev[VENC_IM_BUSY8] = { "VENC_IM_BUSY8", 0, 0, 0, 0 },
	.mdev[VENC_IM_BUSY9] = { "VENC_IM_BUSY9", 0, 0, 0, 0 },
	.mdev[VENC_IM_BUSY10] = { "VENC_IM_BUSY10", 0, 0, 0, 0 },
	.mdev[VENC_IM_BUSY11] = { "VENC_IM_BUSY11", 0, 0, 0, 0 },
	.mdev[VDEC] = { "VDEC", 0, 0, 0, 0 },
	.mdev[VDEC_IM_BUSY0] = { "VDEC_IM_BUSY0", 0, 0, 0, 0 },
	.mdev[VDEC_IM_BUSY1] = { "VDEC_IM_BUSY1", 0, 0, 0, 0 },
	.mdev[VDEC_IM_BUSY2] = { "VDEC_IM_BUSY2", 0, 0, 0, 0 },
	.mdev[VDEC_IM_BUSY3] = { "VDEC_IM_BUSY3", 0, 0, 0, 0 },
	.mdev[VDEC_IM_BUSY4] = { "VDEC_IM_BUSY4", 0, 0, 0, 0 },
	.mdev[VDEC_IM_BUSY5] = { "VDEC_IM_BUSY5", 0, 0, 0, 0 },
	.mdev[VDEC_IM_BUSY6] = { "VDEC_IM_BUSY6", 0, 0, 0, 0 },
	.mdev[VDEC_IM_BUSY7] = { "VDEC_IM_BUSY7", 0, 0, 0, 0 },
	.mdev[VDEC_IM_BUSY8] = { "VDEC_IM_BUSY8", 0, 0, 0, 0 },
	.mdev[VDEC_IM_BUSY9] = { "VDEC_IM_BUSY9", 0, 0, 0, 0 },
	.mdev[VDEC_IM_BUSY10] = { "VDEC_IM_BUSY10", 0, 0, 0, 0 },
	.mdev[VDEC_IM_BUSY11] = { "VDEC_IM_BUSY11", 0, 0, 0, 0 },
	.mdev[VDEC_IM_BUSY12] = { "VDEC_IM_BUSY12", 0, 0, 0, 0 },
	.mdev[VDEC_IM_BUSY13] = { "VDEC_IM_BUSY13", 0, 0, 0, 0 },
	.mdev[ICS_RSV_OUT0] = { "ICS_RSV_OUT0", 0, 0, 0, 0 },
	.mdev[ASP_HIFI] = { "ASP_HIFI", 0, 0, 0, 0 },
	.mdev[ASP_BUS] = { "ASP_BUS", 0, 0, 0, 0 },
	.mdev[ASP_MST] = { "ASP_MST", 0, 0, 0, 0 },
	.mdev[ASP_USB] = { "ASP_USB", 0, 0, 0, 0 },
	.mdev[IOMCU] = { "IOMCU", 0, 0, 0, 0 },
	.mdev[IOMCU_NOPENDING] = { "IOMCU_NOPENDING", 0, 0, 0, 0 },
	.mdev[UFSBUS_AUTODIV] = { "UFSBUS_AUTODIV", 0, 0, 0, 0 },
	.mdev[UFS_STAT] = { "UFS_STAT", 0, 0, 0, 0 },
	.mdev[AOBUS_STAT] = { "AOBUS_STAT", 0, 0, 0, 0 },
	.mdev[PLL_AUTODIV] = { "PLL_AUTODIV", 0, 0, 0, 0 },
	.mdev[FLL_AUTODIV] = { "FLL_AUTODIV", 0, 0, 0, 0 },
	.mdev[UFSBUS_STAT] = { "UFSBUS_STAT", 0, 0, 0, 0 },
	.mdev[PDC_SLEEP] = { "PDC_SLEEP", 0, 0, 0, 0 },
	.mdev[PDC_DEEP_SLEEP] = { "PDC_DEEP_SLEEP", 0, 0, 0, 0 },
	.mdev[PDC_SLOW] = { "PDC_SLOW", 0, 0, 0, 0 },
	.mdev[LP_STAT_S0] = { "LP_STAT_S0", 0, 0, 0, 0 },
	.mdev[LP_STAT_S1] = { "LP_STAT_S1", 0, 0, 0, 0 },
	.mdev[LP_STAT_S2] = { "LP_STAT_S2", 0, 0, 0, 0 },
	.mdev[LP_STAT_S3] = { "LP_STAT_S3", 0, 0, 0, 0 },
	.mdev[LP_STAT_S4] = { "LP_STAT_S4", 0, 0, 0, 0 },
};
