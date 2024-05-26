/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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


#include <linux/types.h>

#include "dpu_csc.h"
#include "hisi_disp_debug.h"
#include "hisi_disp_gadgets.h"

#define init_csc_coe(coe) {.value = coe,}
struct csc_coe {
	DPU_ARSR_CSC_IDC0_UNION idc0;
	DPU_ARSR_CSC_IDC2_UNION idc2;

	DPU_ARSR_CSC_ODC0_UNION odc0;
	DPU_ARSR_CSC_ODC2_UNION odc2;

	DPU_ARSR_CSC_P00_UNION p00;
	DPU_ARSR_CSC_P01_UNION p01;
	DPU_ARSR_CSC_P02_UNION p02;
	DPU_ARSR_CSC_P10_UNION p10;
	DPU_ARSR_CSC_P11_UNION p11;
	DPU_ARSR_CSC_P12_UNION p12;
	DPU_ARSR_CSC_P20_UNION p20;
	DPU_ARSR_CSC_P21_UNION p21;
	DPU_ARSR_CSC_P22_UNION p22;
};

static struct csc_coe g_yuv2rgb_601_wide = {
	.idc0 = {
		.reg = {
			.csc_idc0 = 0x600,
			.csc_idc1 = 0x600,
		},
	},
	.idc2 = init_csc_coe(0),
	.odc0 = init_csc_coe(0x000),
	.odc2 = init_csc_coe(0x000),
	.p00 = init_csc_coe(0x4000),
	.p01 = init_csc_coe(0x0000),
	.p02 = init_csc_coe(0x059ba),
	.p10 = init_csc_coe(0x4000),
	.p11 = init_csc_coe(0x1e9fa),
	.p12 = init_csc_coe(0x1d24c),
	.p20 = init_csc_coe(0x4000),
	.p21 = init_csc_coe(0x07168),
	.p22 = init_csc_coe(0x00000),
};

static struct csc_coe g_rgb2yuv_601_wide = {
	.idc0 = init_csc_coe(0x000),
	.idc2 = init_csc_coe(0),
	.odc0 = {
		.reg = {
			.csc_odc0 = 0x200,
			.csc_odc1 = 0x200,
		},
	},
	.odc2 = init_csc_coe(0x000),
	.p00 = init_csc_coe(0x01323),
	.p01 = init_csc_coe(0x02591),
	.p02 = init_csc_coe(0x0074c),
	.p10 = init_csc_coe(0x1f533),
	.p11 = init_csc_coe(0x1eacd),
	.p12 = init_csc_coe(0x02000),
	.p20 = init_csc_coe(0x02000),
	.p21 = init_csc_coe(0x1e534),
	.p22 = init_csc_coe(0x1facc),
};

static struct csc_coe g_yuv2rgb_601_narrow = {
	.idc0 = {
		.reg = {
			.csc_idc0 = 0x600,
			.csc_idc1 = 0x600,
		},
	},
	.idc2 = init_csc_coe(0x7c0),
	.odc0 = init_csc_coe(0x000),
	.odc2 = init_csc_coe(0x000),
	.p00 = init_csc_coe(0x4a85),
	.p01 = init_csc_coe(0x00000),
	.p02 = init_csc_coe(0x06625),
	.p10 = init_csc_coe(0x4a85),
	.p11 = init_csc_coe(0x1e6ed),
	.p12 = init_csc_coe(0x1cbf8),
	.p20 = init_csc_coe(0x4a85),
	.p21 = init_csc_coe(0x0811a),
	.p22 = init_csc_coe(0x00000),
};

static struct csc_coe g_rgb2yuv_601_narrow = {
	.idc0 = init_csc_coe(0x000),
	.idc2 = init_csc_coe(0x000),
	.odc0 = {
		.reg = {
			.csc_odc0 = 0x200,
			.csc_odc1 = 0x200,
		},
	},
	.odc2 = init_csc_coe(0x040),
	.p00 = init_csc_coe(0x0106f),
	.p01 = init_csc_coe(0x02044),
	.p02 = init_csc_coe(0x00644),
	.p10 = init_csc_coe(0x1f684),
	.p11 = init_csc_coe(0x1ed60),
	.p12 = init_csc_coe(0x01c1c),
	.p20 = init_csc_coe(0x01c1c),
	.p21 = init_csc_coe(0x1e876),
	.p22 = init_csc_coe(0x1fb6e),
};

static struct csc_coe g_yuv2rgb_709_wide = {
	.idc0 = {
		.reg = {
			.csc_idc0 = 0x600,
			.csc_idc1 = 0x600,
		},
	},
	.idc2 = init_csc_coe(0x000),
	.odc0 = init_csc_coe(0x000),
	.odc2 = init_csc_coe(0x000),
	.p00 = init_csc_coe(0x4000),
	.p01 = init_csc_coe(0x00000),
	.p02 = init_csc_coe(0x064ca),
	.p10 = init_csc_coe(0x4000),
	.p11 = init_csc_coe(0x1f403),
	.p12 = init_csc_coe(0x1e20a),
	.p20 = init_csc_coe(0x4000),
	.p21 = init_csc_coe(0x076c2),
	.p22 = init_csc_coe(0x00000),
};

static struct csc_coe g_rgb2yuv_709_wide = {
	.idc0 = init_csc_coe(0x000),
	.idc2 = init_csc_coe(0x000),
	.odc0 = {
		.reg = {
			.csc_odc0 = 0x200,
			.csc_odc1 = 0x200,
		},
	},
	.odc2 = init_csc_coe(0x000),

	.p00 = init_csc_coe(0x00d9b),
	.p01 = init_csc_coe(0x02dc6),
	.p02 = init_csc_coe(0x0049f),
	.p10 = init_csc_coe(0x1f8ab),
	.p11 = init_csc_coe(0x1e755),
	.p12 = init_csc_coe(0x02000),
	.p20 = init_csc_coe(0x02000),
	.p21 = init_csc_coe(0x1e2ef),
	.p22 = init_csc_coe(0x1fd11),
};

static struct csc_coe g_yuv2rgb_709_narrow = {
	.idc0 = {
		.reg = {
			.csc_idc0 = 0x600,
			.csc_idc1 = 0x600,
		},
	},
	.idc2 = init_csc_coe(0x7c0),
	.odc0 = init_csc_coe(0x000),
	.odc2 = init_csc_coe(0x000),

	.p00 = init_csc_coe(0x4a85),
	.p01 = init_csc_coe(0x00000),
	.p02 = init_csc_coe(0x072bc),
	.p10 = init_csc_coe(0x4a85),
	.p11 = init_csc_coe(0x1f25a),
	.p12 = init_csc_coe(0x1dde5),
	.p20 = init_csc_coe(0x4a85),
	.p21 = init_csc_coe(0x08732),
	.p22 = init_csc_coe(0x00000),
};

static struct csc_coe g_rgb2yuv_709_narrow = {
	.idc0 = init_csc_coe(0x000),
	.idc2 = init_csc_coe(0x000),
	.odc0 = {
		.reg = {
			.csc_odc0 = 0x200,
			.csc_odc1 = 0x200,
		},
	},
	.odc2 = init_csc_coe(0x040),

	.p00 = init_csc_coe(0x00baf),
	.p01 = init_csc_coe(0x02750),
	.p02 = init_csc_coe(0x003f8),
	.p10 = init_csc_coe(0x1f98f),
	.p11 = init_csc_coe(0x1ea55),
	.p12 = init_csc_coe(0x01c1c),
	.p20 = init_csc_coe(0x01c1c),
	.p21 = init_csc_coe(0x1e678),
	.p22 = init_csc_coe(0x1fd6c),
};

static struct csc_coe g_rgb2yuv_2020 = {
	.idc0 = {
		.reg = {
			.csc_idc0 = 0x00600,
			.csc_idc1 = 0x00600,
		},
	},
	.idc2 = init_csc_coe(0x00000),
	.odc0 = init_csc_coe(0x00000),
	.odc2 = init_csc_coe(0x00000),

	.p00 = init_csc_coe(0x04000),
	.p01 = init_csc_coe(0x00000),
	.p02 = init_csc_coe(0x00000),
	.p10 = init_csc_coe(0x00000),
	.p11 = init_csc_coe(0x04000),
	.p12 = init_csc_coe(0x00000),
	.p20 = init_csc_coe(0x00000),
	.p21 = init_csc_coe(0x00000),
	.p22 = init_csc_coe(0x04000),
};

static struct csc_coe g_yuv2rgb_2020 = {
	.idc0 = {
		.reg = {
			.csc_idc0 = 0x00600,
			.csc_idc1 = 0x00600,
		},
	},
	.idc2 = init_csc_coe(0x007C0),
	.odc0 = init_csc_coe(0x00000),
	.odc2 = init_csc_coe(0x00000),

	.p00 = init_csc_coe(0x04A85),
	.p01 = init_csc_coe(0x00000),
	.p02 = init_csc_coe(0x06B6F),
	.p10 = init_csc_coe(0x04A85),
	.p11 = init_csc_coe(0x1F402),
	.p12 = init_csc_coe(0x1D65F),
	.p20 = init_csc_coe(0x04A85),
	.p21 = init_csc_coe(0x08912),
	.p22 = init_csc_coe(0x00000),
};

static struct csc_coe g_srgb2p3 = {
	.idc0 = {
		.reg = {
			.csc_idc0 = 0x00000,
			.csc_idc1 = 0x00000,
		},
	},
	.idc2 = init_csc_coe(0x00000),
	.odc0 = init_csc_coe(0x00000),
	.odc2 = init_csc_coe(0x00000),

	.p00 = init_csc_coe(0x034A3),
	.p01 = init_csc_coe(0x00B5D),
	.p02 = init_csc_coe(0x00000),
	.p10 = init_csc_coe(0x00220),
	.p11 = init_csc_coe(0x03DE0),
	.p12 = init_csc_coe(0x00000),
	.p20 = init_csc_coe(0x00118),
	.p21 = init_csc_coe(0x004A2),
	.p22 = init_csc_coe(0x03A46),
};

static struct csc_coe g_yuv2p3_601_narrow = {
	.idc0 = {
		.reg = {
			.csc_idc0 = 0x600,
			.csc_idc1 = 0x600,
		},
	},
	.idc2 = init_csc_coe(0x7c0),
	.odc0 = init_csc_coe(0x00000),
	.odc2 = init_csc_coe(0x00000),

	.p00 = init_csc_coe(0x4a85),
	.p01 = init_csc_coe(0x1fb8c),
	.p02 = init_csc_coe(0x04ac6),
	.p10 = init_csc_coe(0x4a85),
	.p11 = init_csc_coe(0x1e7c2),
	.p12 = init_csc_coe(0x1d116),
	.p20 = init_csc_coe(0x4a85),
	.p21 = init_csc_coe(0x073bc),
	.p22 = init_csc_coe(0x1fdfa),
};

static struct csc_coe g_yuv2p3_601_wide = {
	.idc0 = {
		.reg = {
			.csc_idc0 = 0x600,
			.csc_idc1 = 0x600,
		},
	},
	.idc2 = init_csc_coe(0x000),
	.odc0 = init_csc_coe(0x000),
	.odc2 = init_csc_coe(0x000),

	.p00 = init_csc_coe(0x4000),
	.p01 = init_csc_coe(0x1fc17),
	.p02 = init_csc_coe(0x041af),
	.p10 = init_csc_coe(0x4000),
	.p11 = init_csc_coe(0x1e6cc),
	.p12 = init_csc_coe(0x1d6cb),
	.p20 = init_csc_coe(0x4000),
	.p21 = init_csc_coe(0x065aa),
	.p22 = init_csc_coe(0x1fe39),
};

static struct csc_coe g_yuv2p3_709_narrow = {
	.idc0 = {
		.reg = {
			.csc_idc0 = 0x600,
			.csc_idc1 = 0x600,
		},
	},
	.idc2 = init_csc_coe(0x7c0),
	.odc0 = init_csc_coe(0x000),
	.odc2 = init_csc_coe(0x000),

	.p00 = init_csc_coe(0x4a85),
	.p01 = init_csc_coe(0x1fd94),
	.p02 = init_csc_coe(0x0584f),
	.p10 = init_csc_coe(0x4a85),
	.p11 = init_csc_coe(0x1f2ce),
	.p12 = init_csc_coe(0x1e2d6),
	.p20 = init_csc_coe(0x4a85),
	.p21 = init_csc_coe(0x07a1c),
	.p22 = init_csc_coe(0x1ff7e),
};

static struct csc_coe g_yuv2p3_709_wide = {
	.idc0 = {
		.reg = {
			.csc_idc0 = 0x600,
			.csc_idc1 = 0x600,
		},
	},
	.idc2 = init_csc_coe(0x000),
	.odc0 = init_csc_coe(0x000),
	.odc2 = init_csc_coe(0x000),

	.p00 = init_csc_coe(0x4000),
	.p01 = init_csc_coe(0x1fddf),
	.p02 = init_csc_coe(0x04d93),
	.p10 = init_csc_coe(0x4000),
	.p11 = init_csc_coe(0x1f469),
	.p12 = init_csc_coe(0x1E661),
	.p20 = init_csc_coe(0x4000),
	.p21 = init_csc_coe(0x06b43),
	.p22 = init_csc_coe(0x1ff8d),
};

static struct csc_coe* g_csc_coe_arry[CSC_DIRECTION_MAX][CSC_MODE_MAX] = {
	/*  NONE */
	{NULL, NULL, NULL, NULL, NULL, NULL},

	/* RGB2YUV */
	{NULL, &g_rgb2yuv_601_wide, &g_rgb2yuv_601_narrow, &g_rgb2yuv_709_wide, &g_rgb2yuv_709_narrow, &g_rgb2yuv_2020},

	/* YUV2RGB */
	{NULL, &g_yuv2rgb_601_wide, &g_yuv2rgb_601_narrow, &g_yuv2rgb_709_wide, &g_yuv2rgb_709_narrow, &g_yuv2rgb_2020},

	/* YUV2P3 */
	{NULL, &g_yuv2p3_601_wide, &g_yuv2p3_601_narrow, &g_yuv2p3_709_wide, &g_yuv2p3_709_narrow, NULL},

	/* SRGB2P3 */
	{NULL, &g_srgb2p3, NULL, NULL, NULL, NULL},
};

void dpu_csc_set_cmd_item(struct dpu_module_desc *module, struct dpu_csc_data *input)
{
	char __iomem *base = NULL;

	if (!input) {
		disp_pr_err("input is null");
		return;
	}

	base = input->base;
	disp_pr_info("++++ csc_mode = %d", input->csc_mode);

	if ((input->csc_mode == CSC_MODE_NONE) || (input->direction == CSC_DIRECTION_NONE)) {
		disp_pr_info("++++ csc_mode = %d dirction = %d", input->csc_mode, input->direction);
		hisi_module_set_reg(module, DPU_ARSR_CSC_ICG_MODULE_ADDR(base), 0);
		return;
	}

	if (input->csc_mode == CSC_MODE_NONE) // for scene txt has not set
		input->csc_mode = DEFAULT_CSC_MODE;

	dpu_csc_set_coef(module, base, input->direction, input->csc_mode);
	hisi_module_set_reg(module, DPU_ARSR_CSC_ICG_MODULE_ADDR(base), 1); // enable csc

	disp_pr_info("---- csc_mode = %d, input->direction %d", input->csc_mode, input->direction);
}

void dpu_post_csc_set_cmd_item(struct dpu_module_desc *module, struct dpu_csc_data *input)
{
	char __iomem *base = NULL;

	if (!input) {
		disp_pr_err("input is null");
		return;
	}

	base = input->base;
	disp_pr_info("++++ csc_mode = %d direction = %d\n", input->csc_mode, input->direction);

	if ((input->csc_mode == CSC_MODE_NONE) || (input->direction == CSC_DIRECTION_NONE)) {
		hisi_module_set_reg(module, DPU_ARSR_POST_CSC_MODULE_EN_ADDR(base), 0);
		return;
	}

	dpu_post_csc_set_coef(module, base, input->direction, input->csc_mode);
	hisi_module_set_reg(module, DPU_ARSR_POST_CSC_MODULE_EN_ADDR(base), 1); // enable csc

	disp_pr_info("---- post csc_mode = %d, input->direction %d", input->csc_mode, input->direction);
}

enum CSC_DIRECTION dpu_csc_get_direction(uint32_t in_format, uint32_t out_format)
{
	if (is_rgb(in_format) && is_yuv(out_format))
		return CSC_RGB2YUV;

	if (is_yuv(in_format) && is_rgb(out_format))
		return CSC_YUV2RGB;

	return CSC_DIRECTION_NONE;
}

void dpu_post_csc_set_coef(struct dpu_module_desc *module, char __iomem *base, enum CSC_DIRECTION direction,
	enum CSC_MODE csc_mode)
{
	struct csc_coe *coe = NULL;

	disp_assert_if_cond(csc_mode == CSC_MODE_MAX);
	disp_assert_if_cond(direction == CSC_DIRECTION_MAX);

	coe = g_csc_coe_arry[direction][csc_mode];
	if (!coe) {
		disp_pr_info("coe is null, direction=%u,csc_mode=%u", direction, csc_mode);
		return;
	}

	hisi_module_set_reg(module, DPU_ARSR_POST_CSC_IDC0_ADDR(base), coe->idc0.value);
	hisi_module_set_reg(module, DPU_ARSR_POST_CSC_IDC2_ADDR(base), coe->idc2.value);

	hisi_module_set_reg(module, DPU_ARSR_POST_CSC_ODC0_ADDR(base), coe->odc0.value);
	hisi_module_set_reg(module, DPU_ARSR_POST_CSC_ODC2_ADDR(base), coe->odc2.value);

	hisi_module_set_reg(module, DPU_ARSR_POST_CSC_P00_ADDR(base), coe->p00.value);
	hisi_module_set_reg(module, DPU_ARSR_POST_CSC_P01_ADDR(base), coe->p01.value);
	hisi_module_set_reg(module, DPU_ARSR_POST_CSC_P02_ADDR(base), coe->p02.value);

	hisi_module_set_reg(module, DPU_ARSR_POST_CSC_P10_ADDR(base), coe->p10.value);
	hisi_module_set_reg(module, DPU_ARSR_POST_CSC_P11_ADDR(base), coe->p11.value);
	hisi_module_set_reg(module, DPU_ARSR_POST_CSC_P12_ADDR(base), coe->p12.value);

	hisi_module_set_reg(module, DPU_ARSR_POST_CSC_P20_ADDR(base), coe->p20.value);
	hisi_module_set_reg(module, DPU_ARSR_POST_CSC_P21_ADDR(base), coe->p21.value);
	hisi_module_set_reg(module, DPU_ARSR_POST_CSC_P22_ADDR(base), coe->p22.value);
}

void dpu_csc_set_coef(struct dpu_module_desc *module, char __iomem *base, enum CSC_DIRECTION direction,
	enum CSC_MODE csc_mode)
{

	struct csc_coe *coe = NULL;

	disp_assert_if_cond(csc_mode == CSC_MODE_MAX);
	disp_assert_if_cond(direction == CSC_DIRECTION_MAX);

	coe = g_csc_coe_arry[direction][csc_mode];
	if (!coe) {
		disp_pr_info("coe is null, direction=%u,csc_mode=%u", direction, csc_mode);
		return;
	}

	hisi_module_set_reg(module, DPU_ARSR_CSC_IDC0_ADDR(base), coe->idc0.value);
	hisi_module_set_reg(module, DPU_ARSR_CSC_IDC2_ADDR(base), coe->idc2.value);

	hisi_module_set_reg(module, DPU_ARSR_CSC_ODC0_ADDR(base), coe->odc0.value);
	hisi_module_set_reg(module, DPU_ARSR_CSC_ODC2_ADDR(base), coe->odc2.value);

	hisi_module_set_reg(module, DPU_ARSR_CSC_P00_ADDR(base), coe->p00.value);
	hisi_module_set_reg(module, DPU_ARSR_CSC_P01_ADDR(base), coe->p01.value);
	hisi_module_set_reg(module, DPU_ARSR_CSC_P02_ADDR(base), coe->p02.value);

	hisi_module_set_reg(module, DPU_ARSR_CSC_P10_ADDR(base), coe->p10.value);
	hisi_module_set_reg(module, DPU_ARSR_CSC_P11_ADDR(base), coe->p11.value);
	hisi_module_set_reg(module, DPU_ARSR_CSC_P12_ADDR(base), coe->p12.value);

	hisi_module_set_reg(module, DPU_ARSR_CSC_P20_ADDR(base), coe->p20.value);
	hisi_module_set_reg(module, DPU_ARSR_CSC_P21_ADDR(base), coe->p21.value);
	hisi_module_set_reg(module, DPU_ARSR_CSC_P22_ADDR(base), coe->p22.value);
}

