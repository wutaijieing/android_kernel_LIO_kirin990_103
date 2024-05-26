/* Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved. */

#ifndef _HISI_DDR_SUBREASON_MONOCEROS_H_
#define _HISI_DDR_SUBREASON_MONOCEROS_H_

#include <platform_include/basicplatform/linux/rdr_platform.h>
#include <mntn_subtype_exception.h>

enum l1bus_subreason {
	/* DDRC_SEC SUBTYPE REASON */
	MODID_L1BUS_UNKNOWN_MASTER = MODID_DMSS_START,
	MODID_L1BUS_LPMCU,
	MODID_L1BUS_IOMCU_M7,
	MODID_L1BUS_PCIE_1,
	MODID_L1BUS_PERF_STAT,
	MODID_L1BUS_MODEM,
	MODID_L1BUS_DJTAG_M,
	MODID_L1BUS_IOMCU_DMA,
	MODID_L1BUS_UFS,
	MODID_L1BUS_SD,
	MODID_L1BUS_HSDT,
	MODID_L1BUS_DDR,
	MODID_L1BUS_DPC,
	MODID_L1BUS_USB31OTG,
	MODID_L1BUS_DMAC,
	MODID_L1BUS_ASP_HIFI,
	MODID_L1BUS_PCIE_0,
	MODID_L1BUS_HSDT_TCU,
	MODID_L1BUS_POWER_STAT,
	MODID_L1BUS_SEC_HASH,
	MODID_L1BUS_DDR_LPCTRL,
	MODID_L1BUS_SDIO,
	MODID_L1BUS_EPS_SCE1,
	MODID_L1BUS_EPS_SCE2,
	MODID_L1BUS_EPS_SEC_REE,
	MODID_L1BUS_NPU,
	MODID_L1BUS_FCM,
	MODID_L1BUS_GPU,
	MODID_L1BUS_IPP_JPGENC,
	MODID_L1BUS_IPP_JPGDEC,
	MODID_L1BUS_IPP_VBK,
	MODID_L1BUS_IPP_GF,
	MODID_L1BUS_IPP_SLAM,
	MODID_L1BUS_IPP_CORE,
	MODID_L1BUS_VDEC,
	MODID_L1BUS_VENC,
	MODID_L1BUS_DSS,
	MODID_L1BUS_ISP,
	MODID_L1BUS_IDI2AXI,
	MODID_L1BUS_MEDIA_TCU,
	MODID_L1BUS_IVP,
	MODID_L1BUS_MAX = MODID_DMSS_END
};

static struct rdr_exception_info_s g_ddr_einfo[] = {
	{ { 0, 0 }, MODID_L1BUS_UNKNOWN_MASTER, MODID_L1BUS_UNKNOWN_MASTER, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_UNKNOWN_MASTER,
	 (u32)RDR_UPLOAD_YES, "ap", "ap", 0, 0, 0 },
	{ { 0, 0 }, MODID_L1BUS_LPMCU, MODID_L1BUS_LPMCU, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_LPMCU, (u32)RDR_UPLOAD_YES, "ap",
	 "ap", 0, 0, 0 },
	{ { 0, 0 }, MODID_L1BUS_IOMCU_M7, MODID_L1BUS_IOMCU_M7, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_IOMCU_M7, (u32)RDR_UPLOAD_YES, "ap",
	 "ap", 0, 0, 0 },
	{ { 0, 0 }, MODID_L1BUS_PCIE_1, MODID_L1BUS_PCIE_1, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_PCIE_1, (u32)RDR_UPLOAD_YES, "ap",
	 "ap", 0, 0, 0 },
	{ { 0, 0 }, MODID_L1BUS_PERF_STAT, MODID_L1BUS_PERF_STAT, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_PERF_STAT,
	 (u32)RDR_UPLOAD_YES, "ap", "ap", 0, 0, 0 },
	{ { 0, 0 }, MODID_L1BUS_MODEM, MODID_L1BUS_MODEM, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_MODEM, (u32)RDR_UPLOAD_YES, "ap",
	 "ap", 0, 0, 0 },
	{ { 0, 0 }, MODID_L1BUS_DJTAG_M, MODID_L1BUS_DJTAG_M, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_DJTAG_M, (u32)RDR_UPLOAD_YES, "ap",
	 "ap", 0, 0, 0 },
	{ { 0, 0 }, MODID_L1BUS_IOMCU_DMA, MODID_L1BUS_IOMCU_DMA, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_IOMCU_DMA,
	 (u32)RDR_UPLOAD_YES, "ap", "ap", 0, 0, 0 },
	{ { 0, 0 }, MODID_L1BUS_UFS, MODID_L1BUS_UFS, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_UFS, (u32)RDR_UPLOAD_YES, "ap",
	 "ap", 0, 0, 0 },
	{ { 0, 0 }, MODID_L1BUS_SD, MODID_L1BUS_SD, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_SD, (u32)RDR_UPLOAD_YES, "ap",
	 "ap", 0, 0, 0 },
	{ { 0, 0 }, MODID_L1BUS_HSDT, MODID_L1BUS_HSDT, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_HSDT, (u32)RDR_UPLOAD_YES, "ap",
	 "ap", 0, 0, 0 },
	{ { 0, 0 }, MODID_L1BUS_DDR, MODID_L1BUS_DDR, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_DDR, (u32)RDR_UPLOAD_YES, "ap",
	 "ap", 0, 0, 0 },
	{ { 0, 0 }, MODID_L1BUS_DPC, MODID_L1BUS_DPC, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_DPC, (u32)RDR_UPLOAD_YES, "ap",
	 "ap", 0, 0, 0 },
	{ { 0, 0 }, MODID_L1BUS_USB31OTG, MODID_L1BUS_USB31OTG, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_USB31OTG, (u32)RDR_UPLOAD_YES, "ap",
	 "ap", 0, 0, 0 },
	{ { 0, 0 }, MODID_L1BUS_DMAC, MODID_L1BUS_DMAC, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_DMAC, (u32)RDR_UPLOAD_YES, "ap",
	 "ap", 0, 0, 0 },
	{ { 0, 0 }, MODID_L1BUS_ASP_HIFI, MODID_L1BUS_ASP_HIFI, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_ASP_HIFI, (u32)RDR_UPLOAD_YES, "ap",
	 "ap", 0, 0, 0 },
	{ { 0, 0 }, MODID_L1BUS_PCIE_0, MODID_L1BUS_PCIE_0, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_PCIE_0, (u32)RDR_UPLOAD_YES, "ap",
	 "ap", 0, 0, 0 },
	{ { 0, 0 }, MODID_L1BUS_HSDT_TCU, MODID_L1BUS_HSDT_TCU, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_HSDT_TCU, (u32)RDR_UPLOAD_YES, "ap",
	 "ap", 0, 0, 0 },
	{ { 0, 0 }, MODID_L1BUS_POWER_STAT, MODID_L1BUS_POWER_STAT, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_POWER_STAT, (u32)RDR_UPLOAD_YES, "ap",
	 "ap", 0, 0, 0 },
	{ { 0, 0 }, MODID_L1BUS_SEC_HASH, MODID_L1BUS_SEC_HASH, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_SEC_HASH, (u32)RDR_UPLOAD_YES, "ap",
	 "ap", 0, 0, 0 },
	{ { 0, 0 }, MODID_L1BUS_DDR_LPCTRL, MODID_L1BUS_DDR_LPCTRL, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_DDR_LPCTRL, (u32)RDR_UPLOAD_YES, "ap",
	 "ap", 0, 0, 0 },
	{ { 0, 0 }, MODID_L1BUS_SDIO, MODID_L1BUS_SDIO, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_SDIO, (u32)RDR_UPLOAD_YES, "ap",
	 "ap", 0, 0, 0 },
	{ { 0, 0 }, MODID_L1BUS_EPS_SCE1, MODID_L1BUS_EPS_SCE1, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_EPS_SCE1,
	 (u32)RDR_UPLOAD_YES, "ap", "ap", 0, 0, 0 },
	{ { 0, 0 }, MODID_L1BUS_EPS_SCE2, MODID_L1BUS_EPS_SCE2, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_EPS_SCE2,
	 (u32)RDR_UPLOAD_YES, "ap", "ap", 0, 0, 0 },
	{ { 0, 0 }, MODID_L1BUS_EPS_SEC_REE, MODID_L1BUS_EPS_SEC_REE, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_EPS_SEC_REE,
	 (u32)RDR_UPLOAD_YES, "ap", "ap", 0, 0, 0 },
	{ { 0, 0 }, MODID_L1BUS_NPU, MODID_L1BUS_NPU, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_NPU,
	 (u32)RDR_UPLOAD_YES, "ap", "ap", 0, 0, 0 },
	{ { 0, 0 }, MODID_L1BUS_FCM, MODID_L1BUS_FCM, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_FCM, (u32)RDR_UPLOAD_YES, "ap",
	 "ap", 0, 0, 0 },
	{ { 0, 0 }, MODID_L1BUS_GPU, MODID_L1BUS_GPU, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_GPU,
	 (u32)RDR_UPLOAD_YES, "ap", "ap", 0, 0, 0 },
	{ { 0, 0 }, MODID_L1BUS_IPP_JPGENC, MODID_L1BUS_IPP_JPGENC, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_IPP_JPGENC, (u32)RDR_UPLOAD_YES, "ap",
	 "ap", 0, 0, 0 },
	{ { 0, 0 }, MODID_L1BUS_IPP_JPGDEC, MODID_L1BUS_IPP_JPGDEC, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_IPP_JPGDEC,
	 (u32)RDR_UPLOAD_YES, "ap", "ap", 0, 0, 0 },
	{ { 0, 0 }, MODID_L1BUS_IPP_VBK, MODID_L1BUS_IPP_VBK, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_IPP_VBK, (u32)RDR_UPLOAD_YES, "ap",
	 "ap", 0, 0, 0 },
	{ { 0, 0 }, MODID_L1BUS_IPP_GF, MODID_L1BUS_IPP_GF, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_IPP_GF, (u32)RDR_UPLOAD_YES, "ap",
	 "ap", 0, 0, 0 },
	{ { 0, 0 }, MODID_L1BUS_IPP_SLAM, MODID_L1BUS_IPP_SLAM, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_IPP_SLAM, (u32)RDR_UPLOAD_YES, "ap",
	 "ap", 0, 0, 0 },
	{ { 0, 0 }, MODID_L1BUS_IPP_CORE, MODID_L1BUS_IPP_CORE, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_IPP_CORE, (u32)RDR_UPLOAD_YES, "ap",
	 "ap", 0, 0, 0 },
	{ { 0, 0 }, MODID_L1BUS_VDEC, MODID_L1BUS_VDEC, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_VDEC, (u32)RDR_UPLOAD_YES, "ap",
	 "ap", 0, 0, 0 },
	{ { 0, 0 }, MODID_L1BUS_VENC, MODID_L1BUS_VENC, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_VENC, (u32)RDR_UPLOAD_YES, "ap",
	 "ap", 0, 0, 0 },
	{ { 0, 0 }, MODID_L1BUS_DSS, MODID_L1BUS_DSS, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_DSS, (u32)RDR_UPLOAD_YES, "ap",
	 "ap", 0, 0, 0 },
	{ { 0, 0 }, MODID_L1BUS_ISP, MODID_L1BUS_ISP, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_ISP,
	 (u32)RDR_UPLOAD_YES, "ap", "ap", 0, 0, 0 },
	{ { 0, 0 }, MODID_L1BUS_IDI2AXI, MODID_L1BUS_IDI2AXI, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_IDI2AXI,
	 (u32)RDR_UPLOAD_YES, "ap", "ap", 0, 0, 0 },
	{ { 0, 0 }, MODID_L1BUS_MEDIA_TCU, MODID_L1BUS_MEDIA_TCU, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_MEDIA_TCU, (u32)RDR_UPLOAD_YES, "ap",
	 "ap", 0, 0, 0 },
	{ { 0, 0 }, MODID_L1BUS_IVP, MODID_L1BUS_IVP, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_DDRC_SEC, L1BUS_IVP, (u32)RDR_UPLOAD_YES, "ap",
	 "ap", 0, 0, 0 },
};

#endif
