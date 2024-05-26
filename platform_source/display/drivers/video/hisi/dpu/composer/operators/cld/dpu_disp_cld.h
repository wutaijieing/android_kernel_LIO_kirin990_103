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
#ifndef DPU_DISP_CLD_H
#define DPU_DISP_CLD_H

#include <soc_dte_define.h>

#include "hisi_composer_operator.h"
struct dpu_cld_out_data {
	struct hisi_pipeline_data base;

	/* other data */
};

struct dpu_cld_in_data {
	struct hisi_pipeline_data base;

	/* other data */
};

struct dpu_cld_params {
	DPU_CLD_RGB_UNION               cld_rgb_union;
	DPU_CLD_CLK_EN_UNION            cld_clk_en_union;
	DPU_CLD_CLK_SEL_UNION           cld_clk_sel_union;
	DPU_CLD_DBG_UNION               cld_dbg_union;
	DPU_CLD_DBG1_UNION              cld_dbg1_union;
	DPU_CLD_DBG2_UNION              cld_dbg2_union;
	DPU_CLD_DBG3_UNION              cld_dbg3_union;
	DPU_CLD_DBG4_UNION              cld_dbg4_union;
	DPU_CLD_DBG5_UNION              cld_dbg5_union;
	DPU_CLD_DBG6_UNION              cld_dbg6_union;
	DPU_CLD_DBG7_UNION              cld_dbg7_union;
	DPU_CLD_EN_UNION                cld_en_union;
	DPU_CLD_SIZE_UNION              cld_size_union;
	unsigned int                    rsv1[3];
	DPU_CLD_REG_CTRL_UNION          cld_reg_ctrl_union;
	DPU_CLD_REG_CTRL_FLUSH_EN_UNION cld_reg_ctrl_flush_en_union;
	DPU_CLD_REG_CTRL_DEBUG_UNION    cld_reg_ctrl_debug_union;
	DPU_CLD_REG_CTRL_STATE_UNION    cld_reg_ctrl_state_union;
	unsigned int                    rsv2[10];
	DPU_CLD_REG_RD_SHADOW_UNION     cld_reg_rd_shadow_union;
	DPU_CLD_REG_DEFAULT_UNION       cld_reg_default_union;
};

struct dpu_operator_cld {
	struct hisi_comp_operator base;
	struct dpu_cld_params *cld_params;
	struct cmdlist_client *client;

	/* other overlay data */
};

void dpu_cld_init(struct hisi_operator_type *type_operator, struct dpu_module_ops ops, void *cookie);
#endif
