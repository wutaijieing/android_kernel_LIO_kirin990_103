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

#ifndef HISI_OPERATOR_DSD_H
#define HISI_OPERATOR_DSD_H

#include <linux/types.h>

#include "hisi_drv_utils.h"
#include "hisi_disp.h"
#include "hisi_composer_operator.h"

struct hisi_dsd_out_data {
	struct hisi_pipeline_data base;

	/* other data */
};

struct hisi_dsd_in_data {
	struct hisi_pipeline_data base;

	/* other data */
};

struct hisi_operator_dsd {
	struct hisi_comp_operator base;

	/* other sdma data */
};

struct dpu_dsd_params {
	/**/
	DPU_DSD_WRAP_ICE_DSCD_CTRL_UNION dscd_ctrl;
	/**/
	DPU_DSD_WRAP_ICE_DSCD_PIC_RESO_UNION pic_reso;
	/**/
	DPU_DSD_WRAP_REG_CTRL_UNION reg_ctrl;
	DPU_DSD_WRAP_REG_CTRL_FLUSH_EN_UNION reg_ctrl_flush_en;
	/**/
	DPU_DSD_WRAP_CHUNK_NUM_UNION chunk_num;
	DPU_DSD_WRAP_HBLANK_NUM_UNION hblank_num;
	DPU_DSD_WRAP_PIC_SIZE_UNION pic_size;
	DPU_DSD_WRAP_MEM_CTRL_UNION mem_ctrl;
	DPU_DSD_WRAP_CLK_SEL_UNION clk_sel;
	DPU_DSD_WRAP_CLK_EN_UNION clk_en;
	DPU_DSD_WRAP_RELOAD_SW_UNION reload_sw;
	DPU_DSD_WRAP_RD_SHADOW_UNION rd_shadow;
	DPU_DSD_WRAP_REG_DEFAULT_UNION reg_default;
};

void hisi_dsd_init(struct hisi_operator_type *operators, struct dpu_module_ops ops, void *cookie);
#endif /* HISI_OPERATOR_DSD_H */
