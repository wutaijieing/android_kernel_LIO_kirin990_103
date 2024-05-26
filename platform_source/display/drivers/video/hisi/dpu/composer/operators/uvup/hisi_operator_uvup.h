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
#ifndef HISI_OPERATOR_UVUP_H
#define HISI_OPERATOR_UVUP_H

#include <linux/platform_device.h>
#include <linux/types.h>

#include "hisi_disp.h"
#include "hisi_composer_operator.h"

#define UVUP_THREHOLD_DEFAULT 2
struct dpu_uvsamp {
	DPU_UVSAMP_RD_SHADOW_UNION rd_shadow;
	DPU_UVSAMP_REG_DEFAULT_UNION reg_default;
	DPU_UVSAMP_TOP_CLK_SEL_UNION uvsamp_top_clk_sel;
	DPU_UVSAMP_TOP_CLK_EN_UNION uvsamp_top_clk_en;

	DPU_UVSAMP_REG_CTRL_UNION reg_ctrl;
	DPU_UVSAMP_REG_CTRL_FLUSH_EN_UNION reg_ctrl_flush_en;
	DPU_UVSAMP_REG_CTRL_DEBUG_UNION reg_ctrl_debug;
	DPU_UVSAMP_REG_CTRL_STATE_UNION reg_ctrl_state;

	DPU_UVSAMP_CSC_IDC0_UNION csc_idc0;
	DPU_UVSAMP_CSC_IDC2_UNION csc_idc2;
	DPU_UVSAMP_CSC_ODC0_UNION csc_odc0;
	DPU_UVSAMP_CSC_ODC2_UNION csc_odc2;

	DPU_UVSAMP_CSC_P00_UNION csc_p00;
	DPU_UVSAMP_CSC_P01_UNION csc_p01;
	DPU_UVSAMP_CSC_P02_UNION csc_p02;
	DPU_UVSAMP_CSC_P10_UNION csc_p10;
	DPU_UVSAMP_CSC_P11_UNION csc_p11;
	DPU_UVSAMP_CSC_P12_UNION csc_p12;
	DPU_UVSAMP_CSC_P20_UNION csc_p20;
	DPU_UVSAMP_CSC_P21_UNION csc_p21;
	DPU_UVSAMP_CSC_P22_UNION csc_p22;

	DPU_UVSAMP_CSC_ICG_MODULE_UNION csc_icg_module;
	DPU_UVSAMP_CSC_MPREC_UNION csc_mprec;
	DPU_UVSAMP_MEM_CTRL_UNION uvsamp_mem_ctrl;
	DPU_UVSAMP_SIZE_UNION uvsamp_size;
	DPU_UVSAMP_CTRL_UNION uvsamp_ctrl;

	DPU_UVSAMP_COEF_0_UNION uvsamp_coef_0;
	DPU_UVSAMP_COEF_1_UNION uvsamp_coef_1;
	DPU_UVSAMP_COEF_2_UNION uvsamp_coef_2;
	DPU_UVSAMP_COEF_3_UNION uvsamp_coef_3;

	DPU_UVSAMP_DEBUG_UNION uvsamp_debug;

	DPU_UVSAMP_DBLK_MEM_CTRL_UNION dblk_mem_ctrl;
	DPU_UVSAMP_DBLK_SIZE_UNION dblk_size;
	DPU_UVSAMP_DBLK_CTRL_UNION dblk_ctrl;
	DPU_UVSAMP_DBLK_SLICE0_FRAME_INFO_UNION dblk_slice0_frame_info;
	DPU_UVSAMP_DBLK_SLICE1_FRAME_INFO_UNION dblk_slice1_frame_info;
	DPU_UVSAMP_DBLK_SLICE2_FRAME_INFO_UNION dblk_slice2_frame_info;
	DPU_UVSAMP_DBLK_SLICE3_FRAME_INFO_UNION dblk_slice3_frame_info;
	DPU_UVSAMP_DBLK_SLICE0_INFO_UNION dblk_slice0_info;
	DPU_UVSAMP_DBLK_SLICE1_INFO_UNION dblk_slice1_info;
	DPU_UVSAMP_DBLK_SLICE2_INFO_UNION dblk_slice2_info;
	DPU_UVSAMP_DBLK_SLICE3_INFO_UNION dblk_slice3_info;
	/* other data */
};

struct hisi_uvup_out_data {
	struct hisi_pipeline_data base;

	/* other data */
};

struct hisi_uvup_in_data {
	struct hisi_pipeline_data base;

	/* other data */
};

struct hisi_operator_uvup {
	struct hisi_comp_operator base;

	/* other data */
};

enum SAMPLING_MODE {
	UP_SAMPLING = 0,
	DOWN_SAMPLING = 1,
};

void hisi_uvup_init(struct hisi_operator_type *type_operators, struct dpu_module_ops ops, void *cookie);
#endif /* HISI_OPERATOR_UVUP_H */
