/** @file
 * Copyright (c) 2021-2021, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/platform_device.h>
#include <linux/types.h>

#include "hisi_drv_utils.h"
#include "hisi_disp.h"
#include "hisi_disp_composer.h"
#include "hisi_operator_ddic.h"
#include "hisi_dpu_module.h"
#include "hisi_disp_gadgets.h"
#include "hisi_operator_tool.h"

struct demura_coe {
	DPU_DDIC_DEMURA_PXPOINT_UNION p0p1_pt;
	DPU_DDIC_DEMURA_PXPOINT_UNION p2p3_pt;

	DPU_DDIC_DEMURA_PXPOINT_UNION p4p5_pt;
	DPU_DDIC_DEMURA_PXPOINT_UNION p6p7_pt;

	DPU_DDIC_DEMURA_PXPX_SLOPE_UNION p0p1p2_slop;
	DPU_DDIC_DEMURA_PXPX_SLOPE_UNION p2p3p4_slop;
};

struct demura_common_coe {
	DPU_DDIC_DEMURA_CUT_OFF_SLOPE_B_UNION p0lcucp7_slop;
	DPU_DDIC_DEMURA_PXPX_SLOPE_UNION p4p5p6_slop;
	DPU_DDIC_DEMURA_PXPX_SLOPE_UNION p6p7_slop;
};

static struct demura_common_coe g_common_coe = {
	.p0lcucp7_slop = {
		.reg = {
			.demura_p0lc_slope_b = 21,
			.demura_ucp7_slope_b = 32,
		},
	},
	.p4p5p6_slop = {
		.reg = {
			.demura_pxpx_slope_L = 16,
			.demura_pxpx_slope_H = 16,
		},
	},
	.p6p7_slop = {
		.value = 16,
	},
};

static struct demura_coe g_demura_3_plane = {
	.p0p1_pt = {
		.reg = {
			.demura_PxPoint_L = 512,
			.demura_PxPoint_H = 2048,
		},
	},
	.p2p3_pt = {
		.reg = {
			.demura_PxPoint_L = 3584,
			.demura_PxPoint_H = 3585, // change for duplication
		},
	},
	.p4p5_pt = {
		.reg = {
			.demura_PxPoint_L = 3586,
			.demura_PxPoint_H = 3587,
		},
	},
	.p6p7_pt = {
		.reg = {
			.demura_PxPoint_L = 3588,
			.demura_PxPoint_H = 3589,
		},
	},
	.p0p1p2_slop = {
		.reg = {
			.demura_pxpx_slope_L = 5,
			.demura_pxpx_slope_H = 5,
		},
	},
	.p2p3p4_slop = {
		.reg = {
			.demura_pxpx_slope_L = 16,
			.demura_pxpx_slope_H = 16,
		},
	},
};

static struct demura_coe g_demura_4_plane = {
	.p0p1_pt = {
		.reg = {
			.demura_PxPoint_L = 512,
			.demura_PxPoint_H = 1024,
		},
	},
	.p2p3_pt = {
		.reg = {
			.demura_PxPoint_L = 2048,
			.demura_PxPoint_H = 3584,
		},
	},
	.p4p5_pt = {
		.reg = {
			.demura_PxPoint_L = 3585,
			.demura_PxPoint_H = 3586,
		},
	},
	.p6p7_pt = {
		.reg = {
			.demura_PxPoint_L = 3587,
			.demura_PxPoint_H = 3588,
		},
	},
	.p0p1p2_slop = {
		.reg = {
			.demura_pxpx_slope_L = 16,
			.demura_pxpx_slope_H = 8,
		},
	},
	.p2p3p4_slop = {
		.reg = {
			.demura_pxpx_slope_L = 5,
			.demura_pxpx_slope_H = 16,
		},
	},
};

static struct demura_coe g_demura_8_plane = {
	.p0p1_pt = {
		.reg = {
			.demura_PxPoint_L = 256,
			.demura_PxPoint_H = 512,
		},
	},
	.p2p3_pt = {
		.reg = {
			.demura_PxPoint_L = 1024,
			.demura_PxPoint_H = 1536,
		},
	},
	.p4p5_pt = {
		.reg = {
			.demura_PxPoint_L = 2048,
			.demura_PxPoint_H = 2560,
		},
	},
	.p6p7_pt = {
		.reg = {
			.demura_PxPoint_L = 3072,
			.demura_PxPoint_H = 3584,
		},
	},
	.p0p1p2_slop = {
		.reg = {
			.demura_pxpx_slope_L = 32,
			.demura_pxpx_slope_H = 16,
		},
	},
	.p2p3p4_slop = {
		.reg = {
			.demura_pxpx_slope_L = 16,
			.demura_pxpx_slope_H = 16,
		},
	},
};
static struct demura_coe* g_demura_coe_arry[3] = {
	&g_demura_3_plane, &g_demura_4_plane, &g_demura_8_plane,
};
static uint32_t g_demura_lcp0_slope = 64;
static uint32_t g_demura_p7uc_slope = 16;
static uint32_t g_demura_pxscaler_w02 = 16383;
static uint32_t g_demura_pxscaler_w1 = 8192;
static uint32_t g_demura_inpx_scaler = 4096;
static uint32_t g_demura_updefault_scaler_w02 = 16383;
static uint32_t g_demura_updefault_scaler_w1 = 8192;
static uint32_t g_demura_lcut_scaler_w02 = 16383;
static uint32_t g_demura_lcut_scaler_w1 = 8192;
static uint32_t g_demura_upcut_scaler_w02 = 16383;
static uint32_t g_demura_upcut_scaler_w1 = 8192;
static uint32_t g_gcbc_dbv_regiona = 2048;
static uint32_t g_gcbc_dbv_regionb = 2048;
static uint32_t g_gcbc_dbv_regionc = 2048;
static uint32_t g_demura_lcut_pt = 128;
static uint32_t g_demura_upcut_pt = 3840;
static uint32_t g_demura_dp0lc_slope = 64;
static uint32_t g_demura_ucdp4095_slope = 32;
static uint32_t win1_mb0 = 1;
static uint32_t win1_mb1 = 0;
static uint32_t win2_mb0 = 1;
static uint32_t win2_mb1 = 0;
static uint32_t win3_mb0 = 1;
static uint32_t win3_mb1 = 0;

static void demura_config_dbv_scalers(struct hisi_comp_operator *operator, char __iomem *ddic_base,
	uint32_t win, uint32_t color)
{
	uint32_t band;
	uint32_t iuc;
	for (band = 0; band < 2; band++) {
		iuc = win * 6 + color * 2 + band;
		if (win == 1) {
			hisi_module_set_reg(&operator->module_desc,
			DPU_DDIC_DEMURA_DBV_UCSCALER_ADDR(ddic_base, iuc), g_demura_updefault_scaler_w1);
			hisi_module_set_reg(&operator->module_desc,
			DPU_DDIC_DEMURA_DBV_LCSCALER_ADDR(ddic_base, iuc), g_demura_lcut_scaler_w1);
			hisi_module_set_reg(&operator->module_desc,
			DPU_DDIC_DEMURA_DBV_UDSCALER_ADDR(ddic_base, iuc), g_demura_upcut_scaler_w1);
		} else {
			hisi_module_set_reg(&operator->module_desc,
			DPU_DDIC_DEMURA_DBV_UCSCALER_ADDR(ddic_base, iuc), g_demura_updefault_scaler_w02);
			hisi_module_set_reg(&operator->module_desc,
			DPU_DDIC_DEMURA_DBV_LCSCALER_ADDR(ddic_base, iuc), g_demura_lcut_scaler_w02);
			hisi_module_set_reg(&operator->module_desc,
			DPU_DDIC_DEMURA_DBV_UDSCALER_ADDR(ddic_base, iuc), g_demura_upcut_scaler_w02);
		}
	}
}

static void demura_set_dbv_scalers(struct hisi_comp_operator *operator, char __iomem *ddic_base)
{
	uint32_t win;
	uint32_t color;
	uint32_t iip;

	// win=0~2,color=0~2(R=0,G=1,B=2),band=0~1 iuc=win*6+color*2+band
	// DEMURA_DBV_INPXSCALER/DEMURA_DBV_UCSCALER/DEMURA_DBV_LCSCALER/DEMURA_DBV_UDSCALER
	for (win = 0; win < 3; win++) {
		for (color = 0; color < 3; color++) {
			iip = win * 3 + color;
			hisi_module_set_reg(&operator->module_desc,
			DPU_DDIC_DEMURA_DBV_INPXSCALER_ADDR(ddic_base, iip), g_demura_inpx_scaler << 16 | g_demura_inpx_scaler);
			demura_config_dbv_scalers(operator, ddic_base, win, color);
		}
	}
}

static void demura_config_dbv_pxscaler(struct hisi_comp_operator *operator, char __iomem *ddic_base,
	uint32_t win, uint32_t color, uint32_t plane)
{
	uint32_t band;
	uint32_t ide;

	for (band = 0; band < 2; band++) {
		ide= win * 48 + color * 16 + plane * 2 + band;
		if (win == 1)
			hisi_module_set_reg(&operator->module_desc,
			DPU_DDIC_DEMURA_DBV_PXSCALER_ADDR(ddic_base, ide), g_demura_pxscaler_w1);
		else
			hisi_module_set_reg(&operator->module_desc,
			DPU_DDIC_DEMURA_DBV_PXSCALER_ADDR(ddic_base, ide), g_demura_pxscaler_w02);
	}
}

static void demura_set_dbv_pxscaler(struct hisi_comp_operator *operator, char __iomem *ddic_base)
{
	uint32_t win;
	uint32_t color;
	uint32_t plane;

	disp_pr_debug("enter demura lut \n");
	// win=0~2,color=0~2(R=0,G=1,B=2),plane=0~7,band=0~1
	// DEMURA_DBV_PXSCALER
	for (win = 0; win < 3; win++) {
		for (color = 0; color < 3; color++) {
			for (plane = 0; plane < 8; plane++) {
				demura_config_dbv_pxscaler(operator, ddic_base, win, color, plane);
			}
		}
	}
}

static void hisi_ddic_demura_set_luts(struct hisi_comp_operator *operator, char __iomem *dpu_base_addr)
{
	char __iomem *ddic_base = NULL;
	ddic_base = dpu_base_addr + DSS_DDIC0_OFFSET;
	struct demura_coe *coe = NULL;

	coe = g_demura_coe_arry[g_demura_idx];
	disp_pr_info("plane_num_idx=%u", g_demura_idx);
	if (!coe) {
		disp_pr_err("coe is null");
		return;
	}
	demura_set_dbv_pxscaler(operator, ddic_base);
	demura_set_dbv_scalers(operator, ddic_base);
	// color=0~2(R=0,G=1,B=2),plane=0~3,ipx=color*4+plane
	// DEMURA_PXPOINT
	hisi_module_set_reg(&operator->module_desc,
		DPU_DDIC_DEMURA_PXPOINT_ADDR(ddic_base, 0), coe->p0p1_pt.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_DDIC_DEMURA_PXPOINT_ADDR(ddic_base, 1), coe->p2p3_pt.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_DDIC_DEMURA_PXPOINT_ADDR(ddic_base, 2), coe->p4p5_pt.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_DDIC_DEMURA_PXPOINT_ADDR(ddic_base, 3), coe->p6p7_pt.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_DDIC_DEMURA_PXPOINT_ADDR(ddic_base, 4), coe->p0p1_pt.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_DDIC_DEMURA_PXPOINT_ADDR(ddic_base, 5), coe->p2p3_pt.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_DDIC_DEMURA_PXPOINT_ADDR(ddic_base, 6), coe->p4p5_pt.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_DDIC_DEMURA_PXPOINT_ADDR(ddic_base, 7), coe->p6p7_pt.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_DDIC_DEMURA_PXPOINT_ADDR(ddic_base, 8), coe->p0p1_pt.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_DDIC_DEMURA_PXPOINT_ADDR(ddic_base, 9), coe->p2p3_pt.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_DDIC_DEMURA_PXPOINT_ADDR(ddic_base, 10), coe->p4p5_pt.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_DDIC_DEMURA_PXPOINT_ADDR(ddic_base, 11), coe->p6p7_pt.value);

	// color=0~2(R=0,G=1,B=2),plane=0~3,ipx=color*4+plane
	// DEMURA_PXPX_SLOPE
	hisi_module_set_reg(&operator->module_desc,
		DPU_DDIC_DEMURA_PXPX_SLOPE_ADDR(ddic_base, 0), coe->p0p1p2_slop.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_DDIC_DEMURA_PXPX_SLOPE_ADDR(ddic_base, 1), coe->p2p3p4_slop.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_DDIC_DEMURA_PXPX_SLOPE_ADDR(ddic_base, 2), g_common_coe.p4p5p6_slop.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_DDIC_DEMURA_PXPX_SLOPE_ADDR(ddic_base, 3), g_common_coe.p6p7_slop.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_DDIC_DEMURA_PXPX_SLOPE_ADDR(ddic_base, 4), coe->p0p1p2_slop.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_DDIC_DEMURA_PXPX_SLOPE_ADDR(ddic_base, 5), coe->p2p3p4_slop.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_DDIC_DEMURA_PXPX_SLOPE_ADDR(ddic_base, 6), g_common_coe.p4p5p6_slop.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_DDIC_DEMURA_PXPX_SLOPE_ADDR(ddic_base, 7), g_common_coe.p6p7_slop.value);
		hisi_module_set_reg(&operator->module_desc,
		DPU_DDIC_DEMURA_PXPX_SLOPE_ADDR(ddic_base, 8), coe->p0p1p2_slop.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_DDIC_DEMURA_PXPX_SLOPE_ADDR(ddic_base, 9), coe->p2p3p4_slop.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_DDIC_DEMURA_PXPX_SLOPE_ADDR(ddic_base, 10), g_common_coe.p4p5p6_slop.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_DDIC_DEMURA_PXPX_SLOPE_ADDR(ddic_base, 11), g_common_coe.p6p7_slop.value);
	// DEMURA_DBV_ROI_WIN1_STA defalut is 0
	hisi_module_set_reg(&operator->module_desc,
		DPU_DDIC_DEMURA_WIN1_DBV_SLOPE_ADDR(ddic_base), g_gcbc_dbv_regiona);
	hisi_module_set_reg(&operator->module_desc,
		DPU_DDIC_DEMURA_WIN2_DBV_SLOPE_ADDR(ddic_base), g_gcbc_dbv_regionb);
	hisi_module_set_reg(&operator->module_desc,
		DPU_DDIC_DEMURA_WIN3_DBV_SLOPE_ADDR(ddic_base), g_gcbc_dbv_regionc);

	hisi_module_set_reg(&operator->module_desc,
		DPU_DDIC_DEMURA_CUT_OFF_R_ADDR(ddic_base), g_demura_upcut_pt << 12 | g_demura_lcut_pt);
	hisi_module_set_reg(&operator->module_desc,
		DPU_DDIC_DEMURA_CUT_OFF_G_ADDR(ddic_base), g_demura_upcut_pt << 12 | g_demura_lcut_pt);
	hisi_module_set_reg(&operator->module_desc,
		DPU_DDIC_DEMURA_CUT_OFF_B_ADDR(ddic_base), g_demura_upcut_pt << 12 | g_demura_lcut_pt);
	// DEMURA_CUT_OFF_COEF_R/G/B default is 0
	if (g_demura_idx == 2) { // 8 plane
		hisi_module_set_reg(&operator->module_desc,
			DPU_DDIC_DEMURA_CUT_OFF_SLOPE_R_ADDR(ddic_base), g_demura_p7uc_slope << 16 | g_demura_lcp0_slope);
		hisi_module_set_reg(&operator->module_desc,
			DPU_DDIC_DEMURA_CUT_OFF_SLOPE_G_ADDR(ddic_base), g_demura_p7uc_slope << 16 | g_demura_lcp0_slope);
		hisi_module_set_reg(&operator->module_desc,
			DPU_DDIC_DEMURA_CUT_OFF_SLOPE_B_ADDR(ddic_base), g_demura_p7uc_slope << 16 | g_demura_lcp0_slope);
	} else {
		hisi_module_set_reg(&operator->module_desc,
			DPU_DDIC_DEMURA_CUT_OFF_SLOPE_R_ADDR(ddic_base), g_common_coe.p0lcucp7_slop.value);
		hisi_module_set_reg(&operator->module_desc,
			DPU_DDIC_DEMURA_CUT_OFF_SLOPE_G_ADDR(ddic_base), g_common_coe.p0lcucp7_slop.value);
		hisi_module_set_reg(&operator->module_desc,
			DPU_DDIC_DEMURA_CUT_OFF_SLOPE_B_ADDR(ddic_base), g_common_coe.p0lcucp7_slop.value);
	}

	// DEMURA_DP_COEF_R/G/B default is 0
	hisi_module_set_reg(&operator->module_desc,
		DPU_DDIC_DEMURA_DP_SLOPE_R_ADDR(ddic_base), g_demura_ucdp4095_slope << 16 | g_demura_dp0lc_slope);
	hisi_module_set_reg(&operator->module_desc,
		DPU_DDIC_DEMURA_DP_SLOPE_G_ADDR(ddic_base), g_demura_ucdp4095_slope << 16 | g_demura_dp0lc_slope);
	hisi_module_set_reg(&operator->module_desc,
		DPU_DDIC_DEMURA_DP_SLOPE_B_ADDR(ddic_base), g_demura_ucdp4095_slope << 16 | g_demura_dp0lc_slope);

	hisi_module_set_reg(&operator->module_desc,
		DPU_DDIC_DEMURA_DBV_WIN_MB_ADDR(ddic_base),
		win3_mb1 << 10 | win3_mb0 << 8 | win2_mb1 << 6 | win2_mb0 << 4 | win1_mb1 << 2 | win1_mb0);
}

static void hisi_ddic_set_dm_dppinfo(struct hisi_composer_data *ov_data, struct hisi_dm_ddic_info *ddic_info)
{
	disp_pr_info(" ++++ \n");

	ddic_info->ddic_input_img_width_union.value = 0;
	ddic_info->ddic_input_img_width_union.reg.ddic_input_img_width = ov_data->fix_panel_info->xres - 1;
	ddic_info->ddic_input_img_width_union.reg.ddic_input_img_height =  ov_data->fix_panel_info->yres - 1;

	ddic_info->ddic_output_img_width_union.value = 0;
	ddic_info->ddic_output_img_width_union.reg.ddic_output_img_width = ov_data->fix_panel_info->xres - 1;
	ddic_info->ddic_output_img_width_union.reg.ddic_output_img_height = ov_data->fix_panel_info->yres - 1;

	ddic_info->ddic_input_fmt_union.value = 0;
	ddic_info->ddic_input_fmt_union.reg.ddic_layer_id = 0x1F;
	ddic_info->ddic_input_fmt_union.reg.ddic_order0 = DPU_DSC_OP_ID;
	ddic_info->ddic_input_fmt_union.reg.ddic_sel = 1;
	ddic_info->ddic_input_fmt_union.reg.ddic_input_fmt = RGB_10BIT;

	ddic_info->ddic_deburnin_wb_en_union.value= 0;
	if (g_debug_en_demura) {
		ddic_info->ddic_deburnin_wb_en_union.reg.ddic_demura_type = 0x1;
		if (g_demura_plane_num == 8)
			ddic_info->ddic_deburnin_wb_en_union.reg.ddic_demura_type = 0x3;
		disp_pr_debug(" set demura type 0x%x \n", ddic_info->ddic_deburnin_wb_en_union.reg.ddic_demura_type);
	} else {
		ddic_info->ddic_deburnin_wb_en_union.reg.ddic_demura_type = 0;
	}

	ddic_info->ddic_demura_fmt_union.value= 0;
	ddic_info->ddic_demura_fmt_union.reg.ddic_output_fmt = RGB_10BIT;
	ddic_info->ddic_demura_fmt_union.reg.ddic_demura_fmt = g_demura_lut_fmt;

	ddic_info->ddic_demura_lut_width_union.value= 0;
	ddic_info->ddic_demura_lut_width_union.reg.ddic_demura_lut_width = g_r_demura_w - 1;
	ddic_info->ddic_demura_lut_width_union.reg.ddic_demura_lut_height = g_r_demura_h - 1;
	disp_pr_info(" set g_r_demura_w = %d, g_r_demura_h = %d\n", g_r_demura_w, g_r_demura_h);

	ddic_info->ddic_reserved_2_union.value= 0;
	if (g_debug_en_demura) {
		if (g_demura_plane_num == 4) {
			ddic_info->ddic_reserved_2_union.reg.ddic_demura_lut_layer_id0 = 0;
			ddic_info->ddic_reserved_2_union.reg.ddic_demura_lut_layer_id1 = 31;
		} else if  (g_demura_plane_num == 8) {
			ddic_info->ddic_reserved_2_union.reg.ddic_demura_lut_layer_id0 = 0;
			ddic_info->ddic_reserved_2_union.reg.ddic_demura_lut_layer_id1 = 1;
		}
	} else {
		ddic_info->ddic_reserved_2_union.reg.ddic_demura_lut_layer_id0 = 31;
		ddic_info->ddic_reserved_2_union.reg.ddic_demura_lut_layer_id1 = 31;
	}
}

static void ddic_dm_set_regs(struct hisi_comp_operator *operator,
	struct hisi_dm_param *dm_param,
	char __iomem *dpu_base_addr)
{
	struct hisi_dm_ddic_info *ddic_info = &(dm_param->ddic_info);
	char __iomem *dm_base = dpu_base_addr + dpu_get_dm_offset(operator->scene_id);

	disp_pr_info(" ++++ \n");

	hisi_module_set_reg(&operator->module_desc,
		DPU_DM_DDIC_INPUT_IMG_WIDTH_ADDR(dm_base), ddic_info->ddic_input_img_width_union.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_DM_DDIC_OUTPUT_IMG_WIDTH_ADDR(dm_base), ddic_info->ddic_output_img_width_union.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_DM_DDIC_INPUT_FMT_ADDR(dm_base), ddic_info->ddic_input_fmt_union.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_DM_DDIC_DEBURNIN_WB_EN_ADDR(dm_base), ddic_info->ddic_deburnin_wb_en_union.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_DM_DDIC_DEMURA_FMT_ADDR(dm_base), ddic_info->ddic_demura_fmt_union.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_DM_DDIC_DEMURA_LUT_WIDTH_ADDR(dm_base), ddic_info->ddic_demura_lut_width_union.value);
	hisi_module_set_reg(&operator->module_desc,
		DPU_DM_DDIC_RESERVED_2_ADDR(dm_base), ddic_info->ddic_reserved_2_union.value);
}
static void hisi_ddic_set_infos(struct pipeline_src_layer *src_layer,
	struct dpu_ddic *ddic_info)
{
	ddic_info->dither_ctr1.value = 0;
	ddic_info->dither_ctr1.reg.dither_mode = 0x1; // 12bit->10bit

	ddic_info->dither_ctr0.value = 0;
	ddic_info->dither_ctr0.reg.dither_en = 0x1;
}

static void hisi_ddic_flush_en(struct hisi_comp_operator *operator, char __iomem *dpu_base_addr)
{
	hisi_module_set_reg(&operator->module_desc, DPU_DDIC_REG_CTRL_FLUSH_EN_ADDR(dpu_base_addr + DSS_DDIC0_OFFSET), 1);
}

static int hisi_ddic_set_cmd_item(struct hisi_comp_operator *operator, struct hisi_composer_device *composer_device,
		void *layer)
{
	int ret;
	struct hisi_dm_param *dm_param = NULL;
	struct pipeline_src_layer *src_layer = (struct pipeline_src_layer *)layer; // src_layer ??
	struct dpu_ddic ddic_info = {0};
	disp_pr_info(" ++++ \n");
	dm_param = composer_device->ov_data.dm_param;

	hisi_ddic_set_dm_dppinfo(&(composer_device->ov_data), &(dm_param->ddic_info));
	ddic_dm_set_regs(operator, dm_param, composer_device->ov_data.ip_base_addrs[DISP_IP_BASE_DPU]);
	hisi_ddic_set_infos(src_layer, &ddic_info);
	if (g_debug_en_demura)
		hisi_ddic_demura_set_luts(operator, composer_device->ov_data.ip_base_addrs[DISP_IP_BASE_DPU]);

	hisi_ddic_flush_en(operator, composer_device->ov_data.ip_base_addrs[DISP_IP_BASE_DPU]);

	disp_pr_info(" ---- \n");
	return 0;
}

void hisi_ddic_init(struct hisi_operator_type *type_operator, struct dpu_module_ops ops, void *cookie)
{
	struct hisi_operator_ddic **ddic = NULL;
	struct hisi_comp_operator *base = NULL;
	uint32_t i;

	disp_pr_info("dpp ++++ ,type_operator->operator_count = %d\n",type_operator->operator_count);

	ddic = kzalloc(sizeof(*ddic) * type_operator->operator_count, GFP_KERNEL);
	if (!ddic)
		return;

	disp_pr_info(" type_operator->operator_count:%u \n", type_operator->operator_count);
	for (i = 0; i < type_operator->operator_count; i++) {
		disp_pr_info("size of struct hisi_operator_dpp:%u \n", sizeof(*(ddic[i])));
		ddic[i] = kzalloc(sizeof(*(ddic[i])), GFP_KERNEL);
		if (!ddic[i])
			continue;

		/* TODO: error check */
		hisi_operator_init(&ddic[i]->base, ops, cookie);

		base = &ddic[i]->base;
		base->id_desc.info.idx = i;
		base->id_desc.info.type = type_operator->type;
		base->set_cmd_item = hisi_ddic_set_cmd_item;
		sema_init(&ddic[i]->base.operator_sem, 1);

		base->out_data = kzalloc(sizeof(struct hisi_ddic_out_data), GFP_KERNEL);
		if (!base->out_data) {
			kfree(ddic[i]);
			ddic[i] = NULL;
			continue;
		}

		base->in_data = kzalloc(sizeof(struct hisi_ddic_in_data), GFP_KERNEL);
		if (!base->in_data) {
			kfree(base->out_data);
			base->out_data = NULL;

			kfree(ddic[i]);
			ddic[i] = NULL;
			continue;
		}
		disp_pr_info("dpp base->out_data:%p,base->in_data %p \n",base->out_data, base->in_data);
		ddic[i]->base.be_dm_counted = false;
	}
	type_operator->operators = (struct hisi_comp_operator **)(ddic);
}
