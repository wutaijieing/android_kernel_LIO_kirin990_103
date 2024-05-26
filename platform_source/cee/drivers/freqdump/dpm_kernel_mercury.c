/*
 * dpm_kernel_mercury.c
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
#include "dpm_kernel_mercury.h"

void dpm_common_platform_init(void)
{
	g_calc_monitor_time_media_hook = NULL;
}

struct monitor_dump_info g_monitor_info = {
	.monitor_delay = 0,
	.mdev[CC712_STAT] = { "CC712", 0, 0, 0, 0 },
	.mdev[PCI0_LNKST_L0] = { "PM_LINKST_I0", 0, 0, 0, 0 },
	.mdev[PCI0_LNKST_L1] = { "PM_LINKST_I1", 0, 0, 0, 0 },
	.mdev[PCI0_LNKST_L1SUB] = { "PM_LINKST_I1SUB", 0, 0, 0, 0 },
	.mdev[PCI1_LNKST_L0] = { "PCI1_LNKST_L0", 0, 0, 0, 0 },
	.mdev[PCI1_LNKST_L1] = { "PCI1_LNKST_L1", 0, 0, 0, 0 },
	.mdev[PCI1_LNKST_L1SUB] = { "PCI1_LNKST_L1SUB", 0, 0, 0, 0 },
	.mdev[NPU_CUBE] = { "NPU_CUBE", 0, 0, 0, 0 },
	.mdev[NPU_MTE] = { "NPU_MTE", 0, 0, 0, 0 },
	.mdev[NPU_VEC] = { "NPU_VEC", 0, 0, 0, 0 },
	.mdev[NPU_BIU] = { "NPU_BIU", 0, 0, 0, 0 },
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
	.mdev[MODEM_CICOM1] = { "MODEM_CICOM1", 0, 0, 0, 0 }, /* zone peri0 */
	.mdev[SYS_BUS] = { "SYS_BUS", 0, 0, 0, 0 },
	.mdev[LPM3] = { "LPM3", 0, 0, 0, 0 },
	.mdev[NPU_AUTODIV] = { "NPU_AUTODIV", 0, 0, 0, 0 },
	.mdev[DMA_BUS] = { "DMA_BUS", 0, 0, 0, 0 },
	.mdev[SYSBUS_AUTODIV] = { "SYSBUS_AUTODIV", 0, 0, 0, 0 },
	.mdev[LPMCU_AUTODIV] = { "LPM3_AUTODIV", 0, 0, 0, 0 },
	.mdev[CFGBUS_AUTODIV] = { "CFGBUS_AUTODIV", 0, 0, 0, 0 },
	.mdev[DMABUS_AUTODIV] = { "DMABUS_AUTODIV", 0, 0, 0, 0 },
	.mdev[VDEC_AUTODIV] = { "VDEC_AUTODIV", 0, 0, 0, 0 },
	.mdev[VENC_AUTODIV] = { "VENC_AUTODIV", 0, 0, 0, 0 },
	.mdev[VENC2_AUTODIV] = { "VENC2_AUTODIV", 0, 0, 0, 0 },
	.mdev[DMSS_DDR_BUSY] = { "DMSS_DDR", 0, 0, 0, 0 },
	.mdev[DMSS_BUSY] = { "DMSS", 0, 0, 0, 0 },
	.mdev[BIG_SVFD] = { "BIG_SVFD", 0, 0, 0, 0 },
	.mdev[MID_SVFD] = { "MID_SVFD", 0, 0, 0, 0 },
	.mdev[BIG_IDLE] = { "BIG_IDLE", 0, 0, 0, 0 },
	.mdev[MID_IDLE] = { "MID_IDLE", 0, 0, 0, 0 },
	.mdev[LITTLE_SVFD] = { "LITTLE_SVFD", 0, 0, 0, 0 },
	.mdev[NPU_SVFD] = { "NPU_SVFD", 0, 0, 0, 0 },
	.mdev[L3_SVFD] = { "L3_SVFD", 0, 0, 0, 0 },
	.mdev[LITTLE_IDLE] = { "LITTLE_IDLE", 0, 0, 0, 0 },
	.mdev[GPU_IDLE] = { "GPU_IDLE", 0, 0, 0, 0 },
	.mdev[L3_IDLE] = { "L3_IDLE", 0, 0, 0, 0 }, /* zone peri1 */
	.mdev[VIVO_BUS] = { "VIVO_BUS", 0, 0, 0, 0 },
	.mdev[MDCOM] = { "MDCOM", 0, 0, 0, 0 },
	.mdev[DSS_RCH0] = { "RCH0", 0, 0, 0, 0 },
	.mdev[DSS_RCH1] = { "RCH1", 0, 0, 0, 0 },
	.mdev[DSS_RCH2] = { "RCH2", 0, 0, 0, 0 },
	.mdev[DSS_RCH3] = { "RCH3", 0, 0, 0, 0 },
	.mdev[DSS_RCH4] = { "RCH4", 0, 0, 0, 0 },
	.mdev[DSS_RCH5] = { "RCH5", 0, 0, 0, 0 },
	.mdev[DSS_RCH6] = { "RCH6", 0, 0, 0, 0 },
	.mdev[DSS_RCH7] = { "RCH7", 0, 0, 0, 0 },
	.mdev[DSS_WCH0] = { "WCH0", 0, 0, 0, 0 },
	.mdev[DSS_WCH1] = { "WCH1", 0, 0, 0, 0 },
	.mdev[DSS_RCH8] = { "RCH8", 0, 0, 0, 0 },
	.mdev[DSS_ARSR_PRE] = { "ARSR_PRE", 0, 0, 0, 0 },
	.mdev[DSS_ARSR_POST] = { "ARSR_POST", 0, 0, 0, 0 },
	.mdev[DSS_HIACE] = { "HIACE", 0, 0, 0, 0 },
	.mdev[DSS_DPP] = { "DPP", 0, 0, 0, 0 },
	.mdev[DSS_IFBC] = { "IFBC", 0, 0, 0, 0 },
	.mdev[DSS_DBUF] = { "DBUF", 0, 0, 0, 0 },
	.mdev[DSS_DBCU] = { "DBCU", 0, 0, 0, 0 },
	.mdev[DSS_CMDLIST] = { "CMDLIST", 0, 0, 0, 0 },
	.mdev[DSS_MCTL] = { "MCTL", 0, 0, 0, 0 },
	.mdev[ISP_VDO_REC] = { "ISP_IM_BUSY0", 0, 0, 0, 0 },
	.mdev[ISP_BOKEH] = { "ISP_IM_BUSY1", 0, 0, 0, 0 },
	.mdev[ISP_PREVIEW] = { "ISP_IM_BUSY2", 0, 0, 0, 0 },
	.mdev[ISP_CAP] = { "ISP_IM_BUSY3", 0, 0, 0, 0 }, /* zone media0 */
	.mdev[IVP_MST] = { "IVP_AXI", 0, 0, 0, 0 },
	.mdev[IVP_DSP_BUSY] = { "IVP_DSP", 0, 0, 0, 0 },
	.mdev[VCODECBUS] = { "VCODECBUS", 0, 0, 0, 0 },
	.mdev[VENC] = { "VENC", 0, 0, 0, 0 },
	.mdev[VENC_264I] = { "VENC_IM_BUSY0", 0, 0, 0, 0 },
	.mdev[VENC_264P] = { "VENC_IM_BUSY1", 0, 0, 0, 0 },
	.mdev[VENC_265I] = { "VENC_IM_BUSY2", 0, 0, 0, 0 },
	.mdev[VENC_265P] = { "VENC_IM_BUSY3", 0, 0, 0, 0 },
	.mdev[VENC_TQITQ] = { "VENC_IM_BUSY4", 0, 0, 0, 0 },
	.mdev[VENC_SEL] = { "VENC_IM_BUSY5", 0, 0, 0, 0 },
	.mdev[VENC_IME] = { "VENC_IM_BUSY6", 0, 0, 0, 0 },
	.mdev[VENC2] = { "VENC2", 0, 0, 0, 0 },
	.mdev[VENC2_264I] = { "VENC2_264I", 0, 0, 0, 0 },
	.mdev[VENC2_264P] = { "VENC2_264P", 0, 0, 0, 0 },
	.mdev[VENC2_265I] = { "VENC2_265I", 0, 0, 0, 0 },
	.mdev[VENC2_265P] = { "VENC2_265P", 0, 0, 0, 0 },
	.mdev[VENC2_TQITQ] = { "VENC2_TQITQ", 0, 0, 0, 0 },
	.mdev[VENC2_SEL] = { "VENC2_SEL", 0, 0, 0, 0 },
	.mdev[VENC2_IME] = { "VENC2_IME", 0, 0, 0, 0 },
	.mdev[VDEC] = { "VDEC", 0, 0, 0, 0 },
	.mdev[VDEC_SED] = { "VDEC_IM_BUSY0", 0, 0, 0, 0 },
	.mdev[VDEC_PMV] = { "VDEC_IM_BUSY1", 0, 0, 0, 0 },
	.mdev[VDEC_PRC] = { "VDEC_IM_BUSY2", 0, 0, 0, 0 },
	.mdev[VDEC_PRF] = { "VDEC_IM_BUSY3", 0, 0, 0, 0 },
	.mdev[VDEC_ITRANS] = { "VDEC_IM_BUSY4", 0, 0, 0, 0 },
	.mdev[VDEC_RCN] = { "VDEC_IM_BUSY5", 0, 0, 0, 0 },
	.mdev[VDEC_DBLK_SAO] = { "VDEC_IM_BUSY6", 0, 0, 0, 0 },
	.mdev[VDEC_CMP] = { "VDEC_IM_BUSY7", 0, 0, 0, 0 },
	.mdev[EPS_BUSY0] = { "EPS_BUSY0", 0, 0, 0, 0 },
	.mdev[EPS_BUSY1] = { "EPS_BUSY1", 0, 0, 0, 0 },
	.mdev[EPS_BUSY2] = { "EPS_BUSY2", 0, 0, 0, 0 },
	.mdev[NN_BUSY] = { "NN_BUSY", 0, 0, 0, 0 }, /* zone media1 */
	.mdev[ASP_HIFI] = { "ASP_HIFI", 0, 0, 0, 0 },
	.mdev[ASP_BUS] = { "ASP_BUS", 0, 0, 0, 0 },
	.mdev[ASP_MST] = { "ASP_MST", 0, 0, 0, 0 },
	.mdev[ASP_USB] = { "ASP_USB", 0, 0, 0, 0 },
	.mdev[IOMCU] = { "IOMCU", 0, 0, 0, 0 },
	.mdev[IOMCU_MST] = { "IOMCU_NOPENDING", 0, 0, 0, 0 },
	.mdev[UFSBUS_STAT] = { "UFSBUS_STAT", 0, 0, 0, 0 },
	.mdev[UFS] = { "UFS_STAT", 0, 0, 0, 0 },
	.mdev[AOBUS_AUTODIV] = { "AOBUS_AUTODIV", 0, 0, 0, 0 },
	.mdev[PLL_AUTODIV] = { "PLL_AUTODIV", 0, 0, 0, 0 },
	.mdev[FLL_AUTODIV] = { "FLL_AUTODIV", 0, 0, 0, 0 },
	.mdev[UFSBUS_AUTODIV] = { "UFSBUS_AUTODIV", 0, 0, 0, 0 },
	.mdev[FD_MST] = { "FD_MST", 0, 0, 0, 0 },
	.mdev[FD_BUSMON] = { "FD_BUSMON", 0, 0, 0, 0 },
	.mdev[FD_HWACC] = { "FD_HWACC", 0, 0, 0, 0 },
	.mdev[FD_AOBUS] = { "FD_AOBUS", 0, 0, 0, 0 },
	.mdev[FD_MINIISP] = { "FD_MINIISP", 0, 0, 0, 0 },
	.mdev[FD_DMA] = { "FD_DMA", 0, 0, 0, 0 },
	.mdev[FD_TINY] = { "FD_TINY", 0, 0, 0, 0 },
	.mdev[PDC_SLEEP] = { "PDC_SLEEP", 0, 0, 0, 0 },
	.mdev[PDC_DEEPSLEEP] = { "PDC_DEEPSLEEP", 0, 0, 0, 0 },
	.mdev[PDC_SLOW] = { "PDC_SLOW", 0, 0, 0, 0 },
	.mdev[LP_STAT_S0] = { "LP_STAT_S0", 0, 0, 0, 0 },
	.mdev[LP_STAT_S1] = { "LP_STAT_S1", 0, 0, 0, 0 },
	.mdev[LP_STAT_S2] = { "LP_STAT_S2", 0, 0, 0, 0 },
	.mdev[LP_STAT_S3] = { "LP_STAT_S3", 0, 0, 0, 0 },
	.mdev[LP_STAT_S4] = { "LP_STAT_S4", 0, 0, 0, 0 }, /* zone ao */
};