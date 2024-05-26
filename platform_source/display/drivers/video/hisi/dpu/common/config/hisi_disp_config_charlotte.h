/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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

#ifndef HISI_DISP_DTSI_CHARLOTTE_H
#define HISI_DISP_DTSI_CHARLOTTE_H

#include <linux/types.h>
#include <linux/regulator/consumer.h>

/* vcc name */
#define REGULATOR_DPU_NAME       "regulator_dsssubsys"
#define REGULATOR_VIVOBUS_NAME   "regulator_vivobus"
#define REGULATOR_MEDIA1_NAME    "regulator_media_subsys"
#define REGULATOR_SMMU_NAME      "regulator_smmu_tcu"

enum {
	DISP_IP_BASE_DPU      = 0,
	DISP_IP_BASE_PERI_CRG,
	DISP_IP_BASE_SCTRL,
	DISP_IP_BASE_PCTRL,
	DISP_IP_BASE_PMCTRL,
	DISP_IP_BASE_MEDIA1_CRG,
	DISP_IP_BASE_HSDT1_CRG,
	DISP_IP_BASE_HIDPTX0,
	DISP_IP_BASE_HIDPTX1,
	DISP_IP_BASE_HSDT1_SCTL,
	DISP_IP_BASE_CLK_TUNE,
	DISP_IP_MAX,
};

/* GIC->media1_subsys->dte->module */

/* TODO: need confirm the signal name */
enum {
	DISP_IRQ_SIGNAL_NS_MDP = 0,
	DISP_IRQ_SIGNAL_NS_SDP,
	DISP_IRQ_SIGNAL_NS_L2_M1,
	DISP_IRQ_SIGNAL_DSI0,
	DISP_IRQ_SIGNAL_DSI1,
	DISP_IRQ_SIGNAL_DPTX0,
	DISP_IRQ_SIGNAL_DPTX1,
	DISP_IRQ_SIGNAL_MAX
};

enum {
	PM_REGULATOR_DPU = 0,
	PM_REGULATOR_SMMU,
	PM_REGULATOR_VIVOBUS,
	PM_REGULATOR_MEDIA1,
	PM_REGULATOR_MAX,
};

enum {
	PM_CLK_ACLK_GATE = 0,
	PM_CLK_PCKL_GATE,
	PM_CLK_CORE,
	PM_CLK_LDI0,
	PM_CLK_LDI1,
	PM_CLK_AXI_MM,
	PM_CLK_TX_DPHY0_REF,
	PM_CLK_TX_DPHY1_REF,
	PM_CLK_TX_DPHY0_CFG,
	PM_CLK_TX_DPHY1_CFG,
	PM_CLK_DSI0,
	PM_CLK_DSI1,
	PM_CLK_PCLK_PCTRL,
	PM_CLK_DP_CTRL_16M,
	PM_CLK_PCLK_DP_CTRL,
	PM_CLK_ACLK_DP_CTRL,
	PM_CLK_MAX,
};

enum op_id {
	DPU_OV_OP_ID          = 2,
	DPU_HDR_OP_ID         = 3,
	DPU_UVUP_OP_ID        = 4,
	DPU_CLD_OP_ID         = 5,
	DPU_SCL_OP_ID         = 6,
	DPU_DMA_OP_ID         = 7,
	DPU_SROT_OP_ID        = 8,
	DPU_HEMCD_OP_ID       = 9,
	DPU_DPP_OP_ID         = 10,
	DPU_DDIC_OP_ID        = 11,
	DPU_DSC_OP_ID         = 12,
	DPU_WCH_OP_ID         = 13,
	DPU_ITF_OP_ID         = 14,
	DPU_DEMURA0_OP_ID     = 15,
	DPU_DEBN_OP_ID        = 16,
	DPU_PRELOAD_OP_ID     = 17,
	DPU_DEMURA1_OP_ID     = 18,
	DPU_DSD_OP_ID         = 19,
	DPU_DDIC_IDLE_OP_ID   = 30,
	DPU_OP_INVALID_ID     = 31,
	DPU_LAST_OP_ID        = 64,
};

enum comp_operator_type {
	COMP_OPS_SDMA = 0,
	COMP_OPS_HEMCD = 1,
	COMP_OPS_SROT  = 2,
	COMP_OPS_ARSR  = 3,
	COMP_OPS_VSCF  = 4,
	COMP_OPS_HDR  = 5,
	COMP_OPS_CLD  = 6,
	COMP_OPS_UVUP = 7,
	COMP_OPS_OVERLAY = 8,
	COMP_OPS_DPP = 9,
	COMP_OPS_DDIC,
	COMP_OPS_DSC,
	COMP_OPS_DEMURA,
	COMP_OPS_DEBN,
	COMP_OPS_PRELOAD,
	COMP_OPS_WCH,
	COMP_OPS_ITF,
	COMP_OPS_DSD,
	COMP_OPS_TYPE_MAX,
	COMP_OPS_INVALID,
};

/* pipe_sw_post_idx */
enum {
	PIPE_SW_POST_CH_DSI0 = 0,
	PIPE_SW_POST_CH_DSI1,
	PIPE_SW_POST_CH_DSI2,
	PIPE_SW_POST_CH_DP0,
	PIPE_SW_POST_CH_DP1,
	PIPE_SW_POST_CH_MAX,
};

static inline const char *hisi_config_get_operator_name(uint16_t type)
{
	const char* type_name[COMP_OPS_TYPE_MAX] = {
		"SDMA", "HEMCD", "SROT", "ARSR", "SCL", "HDR",
		"CLD",  "UVUP",  "OVERLAY", "DPP", "DDIC", "DSC",
		"DEMA", "DEBN", "PRELOAD", "WCH", "ITF", "DSD"
	};

	return type_name[type % COMP_OPS_TYPE_MAX];
}

static inline const char *hisi_config_get_irq_name(uint32_t irq_signal)
{
	const char* irq_name[DISP_IRQ_SIGNAL_MAX] = {
		"disp_ns_mdp", "disp_ns_sdp", "disp_l2_m1",
		"disp_dsi0", "disp_dsi1", "disp_dptx"
	};

	return irq_name[irq_signal % DISP_IRQ_SIGNAL_MAX];
}

static inline void hisi_config_get_regulators(struct platform_device *device, struct regulator_bulk_data *regulators, uint32_t count)
{
	regulators[PM_REGULATOR_DPU].supply = REGULATOR_DPU_NAME;
	regulators[PM_REGULATOR_VIVOBUS].supply = REGULATOR_VIVOBUS_NAME;
	regulators[PM_REGULATOR_MEDIA1].supply = REGULATOR_MEDIA1_NAME;
	regulators[PM_REGULATOR_SMMU].supply = REGULATOR_SMMU_NAME;
	devm_regulator_bulk_get(&(device->dev), count, regulators);
}

static inline uint32_t hisi_get_op_id_by_op_type(uint32_t op_type)
{
	switch (op_type) {
	case COMP_OPS_HEMCD:
		return DPU_HEMCD_OP_ID;
	case COMP_OPS_SROT:
		return DPU_SROT_OP_ID;
	case COMP_OPS_ARSR:
	case COMP_OPS_VSCF:
		return DPU_SCL_OP_ID;
	case COMP_OPS_HDR:
		return DPU_HDR_OP_ID;
	case COMP_OPS_CLD:
		return DPU_CLD_OP_ID;
	case COMP_OPS_UVUP:
		return DPU_UVUP_OP_ID;
	case COMP_OPS_OVERLAY:
		return DPU_OV_OP_ID;
	case COMP_OPS_DPP:
		return DPU_DPP_OP_ID;
	case COMP_OPS_DDIC:
		return DPU_DDIC_OP_ID;
	case COMP_OPS_DSC:
		return DPU_DSC_OP_ID;
	case COMP_OPS_DEMURA:
		return DPU_DEMURA0_OP_ID;
	case COMP_OPS_WCH:
		return DPU_WCH_OP_ID;
	case COMP_OPS_ITF:
		return DPU_ITF_OP_ID;
	case COMP_OPS_DEBN:
		return DPU_DEBN_OP_ID;
	default:
		return 0;
	}
}

#endif /* HISI_DISP_DTSI_CHARLOTTE_H */
