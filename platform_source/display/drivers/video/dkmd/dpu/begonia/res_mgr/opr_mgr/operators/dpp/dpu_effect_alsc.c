/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/slab.h>
#include <linux/module.h>
#include <dpu/soc_dpu_define.h>
#include "dpu_effect_alsc.h"
#include "dkmd_object.h"
#include "dkmd_log.h"
#include "opr_cmd_data.h"
#include "dpp_cmd_data.h"
#include "cmdlist_interface.h"
#include "isr/dkmd_isr.h"

int32_t g_enable_alsc = 1;
module_param_named(enable_alsc, g_enable_alsc, int, 0600);
MODULE_PARM_DESC(enable_alsc, "enable alsc");

// TODO: temporarily add here for chip verify
struct dpu_alsc *g_alsc;
struct dkmd_alsc g_in_alsc = {
	.alsc_en = 1,
	.sensor_channel = 1,
	.addr = 0,
	.size = 0x5959,
	.bl_param = {
		.coef_r2r = 5214481,
		.coef_r2g = 1151249,
		.coef_r2b = 203161,
		.coef_r2c = 5214481,
		.coef_g2r = 1354410,
		.coef_g2g = 5620804,
		.coef_g2b = 677205,
		.coef_g2c = 1354410,
		.coef_b2r = 67720,
		.coef_b2g = 1015808,
		.coef_b2b = 3250585,
		.coef_b2c = 67720,
	},

	.addr_block = {
		0x2e2e2b2b, 0x2e2a2929, 0x2a30292b, 0x30302b2f, 0x302e2f29,
		0x30282525, 0x28342529, 0x34342931, 0x34303125, 0x34242020,
		0x24392025, 0x39392535, 0x39343520, 0x391f1919, 0x1f401920,
		0x4040203a, 0x40393a19, 0x40181111, 0x18481119, 0x48481941,
		0x48404111, 0x48100000, 0x10590011, 0x59591149, 0x59484900,
		0, 0, 0, 0, 0
	},

	.ave_block = {
		4096, 5461, 5461, 5461, 5461, 1365, 1365, 1365, 1365, 624, 624, 624, 624,
		284, 284, 284, 284, 171, 171, 171, 171, 53, 53, 53, 53, 0, 0, 0, 0, 0
	},

	.coef_block_r = {
		3265, 2099, 2177, 2410, 2099, 4276, 4898, 5442, 4587, 3498,
		4120, 4587, 3965, 2410, 2721, 3187, 2721, 1244, 1011, 1322,
		1166, 544, 544, 700, 544, 0, 0, 0, 0, 0
	},

	.coef_block_g = {
		3298, 1979, 2039, 2338, 2039, 4497, 4617, 5876, 4677, 3657,
		3657, 5097, 3837, 2518, 2338, 3238, 2458, 1319, 959, 1439,
		1079, 600, 600, 839, 540, 0, 0, 0, 0, 0
	},

	.coef_block_b = {
		4638, 2722, 2823, 3428, 2823, 5041, 5243, 6856, 5545, 3126,
		3327, 4335, 3529, 1613, 1714, 2218, 1815, 807, 605, 907,
		706, 403, 403, 504, 403, 0, 0, 0, 0, 0
	},

	.coef_block_c = {
		4638, 2722, 2823, 3428, 2823, 5041, 5243, 6856, 5545, 3126,
		3327, 4335, 3529, 1613, 1714, 2218, 1815, 807, 605, 907,
		706, 403, 403, 504, 403, 0, 0, 0, 0, 0
	},

	.degamma_coef = {
		.r_coef = {
			0x0000, 0x0001, 0x0002, 0x0004, 0x0005, 0x0006, 0x0007, 0x0009, 0x000a, 0x000b, 0x000c, 0x000e, 0x000f, 0x0010, 0x0012, 0x0013,
			0x0015, 0x0017, 0x0019, 0x001b, 0x001d, 0x001f, 0x0021, 0x0023, 0x0025, 0x0028, 0x002a, 0x002d, 0x002f, 0x0032, 0x0035, 0x0038,
			0x003b, 0x003e, 0x0041, 0x0045, 0x0048, 0x004b, 0x004f, 0x0053, 0x0057, 0x005a, 0x005e, 0x0063, 0x0067, 0x006b, 0x006f, 0x0074,
			0x0079, 0x007d, 0x0082, 0x0087, 0x008c, 0x0091, 0x0097, 0x009c, 0x00a1, 0x00a7, 0x00ad, 0x00b2, 0x00b8, 0x00be, 0x00c5, 0x00cb,
			0x00d1, 0x00d8, 0x00de, 0x00e5, 0x00ec, 0x00f3, 0x00fa, 0x0101, 0x0108, 0x0110, 0x0117, 0x011f, 0x0127, 0x012f, 0x0137, 0x013f,
			0x0147, 0x0150, 0x0158, 0x0161, 0x016a, 0x0173, 0x017c, 0x0185, 0x018e, 0x0198, 0x01a1, 0x01ab, 0x01b5, 0x01bf, 0x01c9, 0x01d3,
			0x01dd, 0x01e8, 0x01f2, 0x01fd, 0x0208, 0x0213, 0x021e, 0x0229, 0x0235, 0x0240, 0x024c, 0x0258, 0x0264, 0x0270, 0x027c, 0x0289,
			0x0295, 0x02a2, 0x02af, 0x02bb, 0x02c9, 0x02d6, 0x02e3, 0x02f1, 0x02fe, 0x030c, 0x031a, 0x0328, 0x0336, 0x0345, 0x0353, 0x0362,
			0x0371, 0x0380, 0x038f, 0x039e, 0x03ad, 0x03bd, 0x03cd, 0x03dd, 0x03ed, 0x03fd, 0x040d, 0x041d, 0x042e, 0x043f, 0x0450, 0x0461,
			0x0472, 0x0483, 0x0495, 0x04a6, 0x04b8, 0x04ca, 0x04dc, 0x04ef, 0x0501, 0x0514, 0x0526, 0x0539, 0x054c, 0x0560, 0x0573, 0x0587,
			0x059a, 0x05ae, 0x05c2, 0x05d6, 0x05eb, 0x05ff, 0x0614, 0x0629, 0x063e, 0x0653, 0x0668, 0x067e, 0x0693, 0x06a9, 0x06bf, 0x06d5,
			0x06eb, 0x0702, 0x0718, 0x072f, 0x0746, 0x075d, 0x0775, 0x078c, 0x07a4, 0x07bb, 0x07d3, 0x07eb, 0x0804, 0x081c, 0x0835, 0x084e,
			0x0867, 0x0880, 0x0899, 0x08b3, 0x08cc, 0x08e6, 0x0900, 0x091a, 0x0935, 0x094f, 0x096a, 0x0985, 0x09a0, 0x09bb, 0x09d6, 0x09f2,
			0x0a0d, 0x0a29, 0x0a45, 0x0a62, 0x0a7e, 0x0a9b, 0x0ab8, 0x0ad5, 0x0af2, 0x0b0f, 0x0b2c, 0x0b4a, 0x0b68, 0x0b86, 0x0ba4, 0x0bc3,
			0x0be1, 0x0c00, 0x0c1f, 0x0c3e, 0x0c5d, 0x0c7d, 0x0c9c, 0x0cbc, 0x0cdc, 0x0cfd, 0x0d1d, 0x0d3e, 0x0d5e, 0x0d7f, 0x0da0, 0x0dc2,
			0x0de3, 0x0e05, 0x0e27, 0x0e49, 0x0e6b, 0x0e8d, 0x0eb0, 0x0ed3, 0x0ef6, 0x0f19, 0x0f3c, 0x0f60, 0x0f84, 0x0fa8, 0x0fcc, 0x0ff0,
			0x1fff,
		},

		.g_coef = {
			0x0000, 0x0001, 0x0002, 0x0004, 0x0005, 0x0006, 0x0007, 0x0009, 0x000a, 0x000b, 0x000c, 0x000e, 0x000f, 0x0010, 0x0012, 0x0013,
			0x0015, 0x0017, 0x0019, 0x001b, 0x001d, 0x001f, 0x0021, 0x0023, 0x0025, 0x0028, 0x002a, 0x002d, 0x002f, 0x0032, 0x0035, 0x0038,
			0x003b, 0x003e, 0x0041, 0x0045, 0x0048, 0x004b, 0x004f, 0x0053, 0x0057, 0x005a, 0x005e, 0x0063, 0x0067, 0x006b, 0x006f, 0x0074,
			0x0079, 0x007d, 0x0082, 0x0087, 0x008c, 0x0091, 0x0097, 0x009c, 0x00a1, 0x00a7, 0x00ad, 0x00b2, 0x00b8, 0x00be, 0x00c5, 0x00cb,
			0x00d1, 0x00d8, 0x00de, 0x00e5, 0x00ec, 0x00f3, 0x00fa, 0x0101, 0x0108, 0x0110, 0x0117, 0x011f, 0x0127, 0x012f, 0x0137, 0x013f,
			0x0147, 0x0150, 0x0158, 0x0161, 0x016a, 0x0173, 0x017c, 0x0185, 0x018e, 0x0198, 0x01a1, 0x01ab, 0x01b5, 0x01bf, 0x01c9, 0x01d3,
			0x01dd, 0x01e8, 0x01f2, 0x01fd, 0x0208, 0x0213, 0x021e, 0x0229, 0x0235, 0x0240, 0x024c, 0x0258, 0x0264, 0x0270, 0x027c, 0x0289,
			0x0295, 0x02a2, 0x02af, 0x02bb, 0x02c9, 0x02d6, 0x02e3, 0x02f1, 0x02fe, 0x030c, 0x031a, 0x0328, 0x0336, 0x0345, 0x0353, 0x0362,
			0x0371, 0x0380, 0x038f, 0x039e, 0x03ad, 0x03bd, 0x03cd, 0x03dd, 0x03ed, 0x03fd, 0x040d, 0x041d, 0x042e, 0x043f, 0x0450, 0x0461,
			0x0472, 0x0483, 0x0495, 0x04a6, 0x04b8, 0x04ca, 0x04dc, 0x04ef, 0x0501, 0x0514, 0x0526, 0x0539, 0x054c, 0x0560, 0x0573, 0x0587,
			0x059a, 0x05ae, 0x05c2, 0x05d6, 0x05eb, 0x05ff, 0x0614, 0x0629, 0x063e, 0x0653, 0x0668, 0x067e, 0x0693, 0x06a9, 0x06bf, 0x06d5,
			0x06eb, 0x0702, 0x0718, 0x072f, 0x0746, 0x075d, 0x0775, 0x078c, 0x07a4, 0x07bb, 0x07d3, 0x07eb, 0x0804, 0x081c, 0x0835, 0x084e,
			0x0867, 0x0880, 0x0899, 0x08b3, 0x08cc, 0x08e6, 0x0900, 0x091a, 0x0935, 0x094f, 0x096a, 0x0985, 0x09a0, 0x09bb, 0x09d6, 0x09f2,
			0x0a0d, 0x0a29, 0x0a45, 0x0a62, 0x0a7e, 0x0a9b, 0x0ab8, 0x0ad5, 0x0af2, 0x0b0f, 0x0b2c, 0x0b4a, 0x0b68, 0x0b86, 0x0ba4, 0x0bc3,
			0x0be1, 0x0c00, 0x0c1f, 0x0c3e, 0x0c5d, 0x0c7d, 0x0c9c, 0x0cbc, 0x0cdc, 0x0cfd, 0x0d1d, 0x0d3e, 0x0d5e, 0x0d7f, 0x0da0, 0x0dc2,
			0x0de3, 0x0e05, 0x0e27, 0x0e49, 0x0e6b, 0x0e8d, 0x0eb0, 0x0ed3, 0x0ef6, 0x0f19, 0x0f3c, 0x0f60, 0x0f84, 0x0fa8, 0x0fcc, 0x0ff0,
			0x1fff,
		},

		.b_coef = {
			0x0000, 0x0001, 0x0002, 0x0004, 0x0005, 0x0006, 0x0007, 0x0009, 0x000a, 0x000b, 0x000c, 0x000e, 0x000f, 0x0010, 0x0012, 0x0013,
			0x0015, 0x0017, 0x0019, 0x001b, 0x001d, 0x001f, 0x0021, 0x0023, 0x0025, 0x0028, 0x002a, 0x002d, 0x002f, 0x0032, 0x0035, 0x0038,
			0x003b, 0x003e, 0x0041, 0x0045, 0x0048, 0x004b, 0x004f, 0x0053, 0x0057, 0x005a, 0x005e, 0x0063, 0x0067, 0x006b, 0x006f, 0x0074,
			0x0079, 0x007d, 0x0082, 0x0087, 0x008c, 0x0091, 0x0097, 0x009c, 0x00a1, 0x00a7, 0x00ad, 0x00b2, 0x00b8, 0x00be, 0x00c5, 0x00cb,
			0x00d1, 0x00d8, 0x00de, 0x00e5, 0x00ec, 0x00f3, 0x00fa, 0x0101, 0x0108, 0x0110, 0x0117, 0x011f, 0x0127, 0x012f, 0x0137, 0x013f,
			0x0147, 0x0150, 0x0158, 0x0161, 0x016a, 0x0173, 0x017c, 0x0185, 0x018e, 0x0198, 0x01a1, 0x01ab, 0x01b5, 0x01bf, 0x01c9, 0x01d3,
			0x01dd, 0x01e8, 0x01f2, 0x01fd, 0x0208, 0x0213, 0x021e, 0x0229, 0x0235, 0x0240, 0x024c, 0x0258, 0x0264, 0x0270, 0x027c, 0x0289,
			0x0295, 0x02a2, 0x02af, 0x02bb, 0x02c9, 0x02d6, 0x02e3, 0x02f1, 0x02fe, 0x030c, 0x031a, 0x0328, 0x0336, 0x0345, 0x0353, 0x0362,
			0x0371, 0x0380, 0x038f, 0x039e, 0x03ad, 0x03bd, 0x03cd, 0x03dd, 0x03ed, 0x03fd, 0x040d, 0x041d, 0x042e, 0x043f, 0x0450, 0x0461,
			0x0472, 0x0483, 0x0495, 0x04a6, 0x04b8, 0x04ca, 0x04dc, 0x04ef, 0x0501, 0x0514, 0x0526, 0x0539, 0x054c, 0x0560, 0x0573, 0x0587,
			0x059a, 0x05ae, 0x05c2, 0x05d6, 0x05eb, 0x05ff, 0x0614, 0x0629, 0x063e, 0x0653, 0x0668, 0x067e, 0x0693, 0x06a9, 0x06bf, 0x06d5,
			0x06eb, 0x0702, 0x0718, 0x072f, 0x0746, 0x075d, 0x0775, 0x078c, 0x07a4, 0x07bb, 0x07d3, 0x07eb, 0x0804, 0x081c, 0x0835, 0x084e,
			0x0867, 0x0880, 0x0899, 0x08b3, 0x08cc, 0x08e6, 0x0900, 0x091a, 0x0935, 0x094f, 0x096a, 0x0985, 0x09a0, 0x09bb, 0x09d6, 0x09f2,
			0x0a0d, 0x0a29, 0x0a45, 0x0a62, 0x0a7e, 0x0a9b, 0x0ab8, 0x0ad5, 0x0af2, 0x0b0f, 0x0b2c, 0x0b4a, 0x0b68, 0x0b86, 0x0ba4, 0x0bc3,
			0x0be1, 0x0c00, 0x0c1f, 0x0c3e, 0x0c5d, 0x0c7d, 0x0c9c, 0x0cbc, 0x0cdc, 0x0cfd, 0x0d1d, 0x0d3e, 0x0d5e, 0x0d7f, 0x0da0, 0x0dc2,
			0x0de3, 0x0e05, 0x0e27, 0x0e49, 0x0e6b, 0x0e8d, 0x0eb0, 0x0ed3, 0x0ef6, 0x0f19, 0x0f3c, 0x0f60, 0x0f84, 0x0fa8, 0x0fcc, 0x0ff0,
			0x1fff,
		}
	}
};

static void alsc_param_debug_print(const struct dpu_alsc *alsc)
{
	dpu_pr_debug("[ALSC]status=%u action=%u "
		"alsc_en_by_dirty_region_limit=%u "
		"degamma_en=%u "
		"degamma_lut_sel=%u "
		"alsc_en=%u "
		"sensor_channel=%u "
		"pic_size=%#x "
		"addr=%#x "
		"size=%#x "
		"bl_param.coef_r2r=%u "
		"bl_param.coef_r2g=%u "
		"bl_param.coef_r2b=%u "
		"bl_param.coef_r2c=%u "
		"bl_param.coef_g2r=%u "
		"bl_param.coef_g2g=%u "
		"bl_param.coef_g2b=%u "
		"bl_param.coef_g2c=%u "
		"bl_param.coef_b2r=%u "
		"bl_param.coef_b2g=%u "
		"bl_param.coef_b2b=%u "
		"bl_param.coef_b2c=%u",
		alsc->status,
		alsc->action,
		alsc->alsc_en_by_dirty_region_limit,
		alsc->degamma_en,
		alsc->degamma_lut_sel,
		alsc->dkmd_alsc.alsc_en,
		alsc->dkmd_alsc.sensor_channel,
		alsc->pic_size,
		alsc->dkmd_alsc.addr,
		alsc->dkmd_alsc.size,
		alsc->dkmd_alsc.bl_param.coef_r2r,
		alsc->dkmd_alsc.bl_param.coef_r2g,
		alsc->dkmd_alsc.bl_param.coef_r2b,
		alsc->dkmd_alsc.bl_param.coef_r2c,
		alsc->dkmd_alsc.bl_param.coef_g2r,
		alsc->dkmd_alsc.bl_param.coef_g2g,
		alsc->dkmd_alsc.bl_param.coef_g2b,
		alsc->dkmd_alsc.bl_param.coef_g2c,
		alsc->dkmd_alsc.bl_param.coef_b2r,
		alsc->dkmd_alsc.bl_param.coef_b2g,
		alsc->dkmd_alsc.bl_param.coef_b2b,
		alsc->dkmd_alsc.bl_param.coef_b2c);
}

static int32_t alsc_enable(struct dpu_alsc *alsc, const void *data)
{
	const struct dkmd_alsc *in_alsc = (const struct dkmd_alsc *)data;

	dpu_pr_debug("[ALSC]+");
	alsc->dkmd_alsc = *in_alsc;
	alsc->action |= BIT(ALSC_ENABLE);
	alsc->degamma_en                    = in_alsc->alsc_en;
	alsc->degamma_lut_sel               = 0;
	alsc->alsc_en_by_dirty_region_limit = in_alsc->alsc_en;
	alsc->addr_default                  = in_alsc->addr;
	alsc->size_default                  = in_alsc->size;

	return 0;
}

static int32_t alsc_disable(struct dpu_alsc *alsc, const void *data)
{
	(void)data;

	dpu_pr_debug("[ALSC]+");
	alsc->action = ALSC_DISABLE;

	return 0;
}

static void alsc_block_param_set_reg(const struct dpu_alsc *alsc)
{
	int32_t block_cnt = 0;
	char __iomem *alsc_base = alsc->alsc_base;
	const struct dkmd_alsc *dkmd_alsc = &alsc->dkmd_alsc;

	for (; block_cnt < ALSC_BLOCK_LEN; ++block_cnt) {
		outp32(DPU_DPP_ADDR_BLOCK_ADDR(alsc_base, block_cnt), dkmd_alsc->addr_block[block_cnt]);
		outp32(DPU_DPP_AVE_BLOCK_ADDR(alsc_base, block_cnt), dkmd_alsc->ave_block[block_cnt]);
		outp32(DPU_DPP_COEF_BLOCK_R_ADDR(alsc_base, block_cnt), dkmd_alsc->coef_block_r[block_cnt]);
		outp32(DPU_DPP_COEF_BLOCK_G_ADDR(alsc_base, block_cnt), dkmd_alsc->coef_block_g[block_cnt]);
		outp32(DPU_DPP_COEF_BLOCK_B_ADDR(alsc_base, block_cnt), dkmd_alsc->coef_block_b[block_cnt]);
		outp32(DPU_DPP_COEF_BLOCK_C_ADDR(alsc_base, block_cnt), dkmd_alsc->coef_block_c[block_cnt]);
	}
}

static void alsc_degamma_coef_set_reg(const struct dpu_alsc *alsc, bool enable_cmdlist)
{
	uint32_t block_cnt = 0;
	uint32_t val;
	char __iomem *alsc_base = alsc->alsc_base;
	const struct alsc_degamma_coef *degamma_coef = &alsc->dkmd_alsc.degamma_coef;

	(void)enable_cmdlist;

	for (; block_cnt < ALSC_DEGAMMA_COEF_LEN - 1; block_cnt += 0x2) {
		val = (degamma_coef->r_coef[block_cnt] & ALSC_DEGAMMA_VAL_MASK);
		val |= ((degamma_coef->r_coef[block_cnt + 1] & ALSC_DEGAMMA_VAL_MASK) << ALSC_DEGAMMA_ODD_COEF_START_BIT);
		outp32(DPU_DPP_U_ALSC_DEGAMA_R_COEF_ADDR(alsc_base, (block_cnt >> 1)), val);

		val = (degamma_coef->g_coef[block_cnt] & ALSC_DEGAMMA_VAL_MASK);
		val |= ((degamma_coef->g_coef[block_cnt + 1] & ALSC_DEGAMMA_VAL_MASK) << ALSC_DEGAMMA_ODD_COEF_START_BIT);
		outp32(DPU_DPP_U_ALSC_DEGAMA_G_COEF_ADDR(alsc_base, (block_cnt >> 1)), val);

		val = (degamma_coef->b_coef[block_cnt] & ALSC_DEGAMMA_VAL_MASK);
		val |= ((degamma_coef->b_coef[block_cnt + 1] & ALSC_DEGAMMA_VAL_MASK) << ALSC_DEGAMMA_ODD_COEF_START_BIT);
		outp32(DPU_DPP_U_ALSC_DEGAMA_B_COEF_ADDR(alsc_base, (block_cnt >> 1)), val);
	}

	/* The 129th odd coef */
	outp32(DPU_DPP_U_ALSC_DEGAMA_R_COEF_ADDR(alsc_base, (block_cnt >> 1) + 1),
		degamma_coef->r_coef[block_cnt] & ALSC_DEGAMMA_VAL_MASK);
	outp32(DPU_DPP_U_ALSC_DEGAMA_G_COEF_ADDR(alsc_base, (block_cnt >> 1) + 1),
		degamma_coef->g_coef[block_cnt] & ALSC_DEGAMMA_VAL_MASK);
	outp32(DPU_DPP_U_ALSC_DEGAMA_B_COEF_ADDR(alsc_base, (block_cnt >> 1) + 1),
		degamma_coef->b_coef[block_cnt] & ALSC_DEGAMMA_VAL_MASK);
}

static void alsc_bl_param_set_reg(const struct dpu_alsc *alsc)
{
	char __iomem *alsc_base = alsc->alsc_base;
	const struct alsc_bl_param *bl_param = &alsc->dkmd_alsc.bl_param;

	outp32(DPU_DPP_COEF_R2R_ADDR(alsc_base), bl_param->coef_r2r);
	outp32(DPU_DPP_COEF_R2G_ADDR(alsc_base), bl_param->coef_r2g);
	outp32(DPU_DPP_COEF_R2B_ADDR(alsc_base), bl_param->coef_r2b);
	outp32(DPU_DPP_COEF_R2C_ADDR(alsc_base), bl_param->coef_r2c);
	outp32(DPU_DPP_COEF_G2R_ADDR(alsc_base), bl_param->coef_g2r);
	outp32(DPU_DPP_COEF_G2G_ADDR(alsc_base), bl_param->coef_g2g);
	outp32(DPU_DPP_COEF_G2B_ADDR(alsc_base), bl_param->coef_g2b);
	outp32(DPU_DPP_COEF_G2C_ADDR(alsc_base), bl_param->coef_g2c);
	outp32(DPU_DPP_COEF_B2R_ADDR(alsc_base), bl_param->coef_b2r);
	outp32(DPU_DPP_COEF_B2G_ADDR(alsc_base), bl_param->coef_b2g);
	outp32(DPU_DPP_COEF_B2B_ADDR(alsc_base), bl_param->coef_b2b);
	outp32(DPU_DPP_COEF_B2C_ADDR(alsc_base), bl_param->coef_b2c);
}

static void alsc_init_set_reg(struct dpu_alsc *alsc)
{
	char __iomem *alsc_base = alsc->alsc_base;

	if (!is_bit_enable(alsc->action, ALSC_ENABLE))
		return;

	dpu_pr_debug("[ALSC]+");
	outp32(DPU_DPP_ALSC_DEGAMA_EN_ADDR(alsc_base), 0);
	/* CH0 ALSC DEGAMMA MEM */
	outp32(DPU_DPP_ALSC_DEGAMA_MEM_CTRL_ADDR(alsc_base), 0x00000008);
	outp32(DPU_DPP_ALSC_EN_ADDR(alsc_base), 0);
	outp32(DPU_DPP_SENSOR_CHANNEL_ADDR(alsc_base), alsc->dkmd_alsc.sensor_channel);
	outp32(DPU_DPP_PIC_SIZE_ADDR(alsc_base), alsc->pic_size);
	outp32(DPU_DPP_ALSC_ADDR_ADDR(alsc_base), alsc->dkmd_alsc.addr);
	outp32(DPU_DPP_ALSC_SIZE_ADDR(alsc_base), alsc->dkmd_alsc.size);
	alsc_bl_param_set_reg(alsc);

	alsc_block_param_set_reg(alsc);

	alsc_degamma_coef_set_reg(alsc, false);

	outp32(DPU_DPP_ALSC_DEGAMA_EN_ADDR(alsc_base), alsc->dkmd_alsc.alsc_en);
	outp32(DPU_DPP_ALSC_EN_ADDR(alsc_base), alsc->dkmd_alsc.alsc_en);

	alsc->status = ALSC_WORKING;
}

static void alsc_frame_set_reg(struct dpu_alsc *alsc)
{
	char __iomem *alsc_base = alsc->alsc_base;

	dpu_pr_debug("[ALSC]+");

	if (g_enable_alsc)
		return; // TODO: for chip verify

	if (is_bit_enable(alsc->action, ALSC_DISABLE)) {
		outp32(DPU_DPP_ALSC_DEGAMA_EN_ADDR(alsc_base), alsc->dkmd_alsc.alsc_en);
		outp32(DPU_DPP_ALSC_EN_ADDR(alsc_base), alsc->dkmd_alsc.alsc_en);
		alsc->status = ALSC_UNINIT;
		return;
	}

	outp32(DPU_DPP_PIC_SIZE_ADDR(alsc_base), alsc->pic_size);
	outp32(DPU_DPP_ALSC_ADDR_ADDR(alsc_base), alsc->dkmd_alsc.addr);

	if (is_bit_enable(alsc->action, ALSC_UPDATE_BL) || is_bit_enable(alsc->action, ALSC_ENABLE))
		alsc_bl_param_set_reg(alsc);

	if (is_bit_enable(alsc->action, ALSC_UPDATE_DEGAMMA) || is_bit_enable(alsc->action, ALSC_ENABLE))
		alsc_degamma_coef_set_reg(alsc, true);
}

static int32_t alsc_set_reg(struct dpu_alsc *alsc, const void *data)
{
	struct opr_cmd_data_base *base_data = (struct opr_cmd_data_base *)data;
	struct dpp_cmd_data *dpp_data = (struct dpp_cmd_data *)data;
	uint32_t width = rect_width(&base_data->in_common_data.rect);
	uint32_t height = rect_height(&base_data->in_common_data.rect);

	// TODO: how to get alsc struct if we have 2 alsc(in dpp)
	alsc = &dpp_data->alsc;
	g_alsc = alsc;

	if (width == 0 || height == 0) {
		dpu_pr_err("width=%u or height=%u is zero", width, height);
		return -1;
	}
	alsc->pic_size = alsc_picture_size(width - 1, height - 1);

	if (likely(alsc->status != ALSC_UNINIT))
		alsc_frame_set_reg(alsc);
	else
		alsc_init_set_reg(alsc);

	alsc_param_debug_print(alsc);

	alsc->action = ALSC_NO_ACTION;

	return 0;
}

static int32_t alsc_update_bl_param(struct dpu_alsc *alsc, const void *data)
{
	const struct alsc_bl_param *bl_param = (const struct alsc_bl_param *)data;

	dpu_pr_debug("[ALSC]+");
	alsc->action |= BIT(ALSC_UPDATE_BL);
	alsc->dkmd_alsc.bl_param = *bl_param;
	alsc->bl_update_to_hwc = 1;
	alsc->dkmd_alsc.bl_param = *bl_param;

	return 0;
}

static int32_t alsc_update_degamma_coef(struct dpu_alsc *alsc, const void *data)
{
	const struct alsc_degamma_coef *degamma_coef = (const struct alsc_degamma_coef *)data;

	dpu_pr_debug("[ALSC]+");
	/* TETON only */
	// TODO: if (dpu_check_panel_product_type(dpufd->panel_info.product_type))
	alsc->action |= BIT(ALSC_UPDATE_DEGAMMA);
	alsc->dkmd_alsc.degamma_coef = *degamma_coef;

	return 0;
}

static int32_t alsc_update_dirty_region(struct dpu_alsc *alsc, const void *data)
{
	return 0;
}

static int32_t alsc_blank(struct dpu_alsc *alsc, const void *data)
{
	(void)data;

	dpu_pr_debug("[ALSC]+");
	alsc->status = ALSC_IDLE;
	return 0;
}

static int32_t alsc_unblank(struct dpu_alsc *alsc, const void *data)
{
	(void)data;

	dpu_pr_debug("[ALSC]+");
	alsc->status = ALSC_UNINIT; // all parameters should be init again
	alsc->action |= BIT(ALSC_ENABLE);
	return 0;
}

static int32_t alsc_store_data(struct dpu_alsc *alsc, const void *data)
{
	static uint32_t control_status;
	uint32_t isr_flag = *((const uint32_t *)data);
	struct dpu_alsc_data *p_data = alsc->data_head;
	char __iomem *alsc_base = alsc->alsc_base;
	struct timespec64 ts;

	dpu_assert(!p_data);

	if (isr_flag & DSI_INT_VACT0_START) {
		/* we don't have control_flag here cause Timestamp must
		 * occur first
		 */
		p_data->noise.timestamp = inp32(DPU_DPP_TIMESTAMP_L_ADDR(alsc_base));
		p_data->noise.timestamp |= ((uint64_t)inp32(DPU_DPP_TIMESTAMP_H_ADDR(alsc_base)) << 32);
		control_status = alsc->status;
		dpu_pr_debug("[ALSC]store timestamp [%llu]", p_data->noise.timestamp);
	}

	if ((isr_flag & DSI_INT_VACT0_END)) {
		if (control_status != ALSC_WORKING)
			return 0;

		/* we need control_status here cause Noise should send or not must judge after
		 * VACTIVE0_START occurs
		 */
		p_data->noise.noise1 = inp32(DPU_DPP_NOISE1_ADDR(alsc_base));
		p_data->noise.noise2 = inp32(DPU_DPP_NOISE2_ADDR(alsc_base));
		p_data->noise.noise3 = inp32(DPU_DPP_NOISE3_ADDR(alsc_base));
		p_data->noise.noise4 = inp32(DPU_DPP_NOISE4_ADDR(alsc_base));
		ktime_get_boottime_ts64(&ts);
		p_data->ktimestamp = (uint64_t)ts.tv_sec * NSEC_PER_SEC + (uint64_t)ts.tv_nsec;
		p_data->noise.status = 1;
		dpu_pr_debug("[ALSC]store noise [%u %u %u %u], ktimestamp=%llu", p_data->noise.noise1, p_data->noise.noise2,
			p_data->noise.noise3, p_data->noise.noise4, p_data->ktimestamp);

		/* Move to next node of the alsc data ring cycle */
		alsc->data_head = p_data->next;
	}

	return 0;
}

struct alsc_fsm {
	int32_t (*handle)(struct dpu_alsc *, const void *);
};

static struct alsc_fsm g_alsc_fsm_tbl[ALSC_STATUS_MAX][ALSC_ACTION_MAX] = {
	// [ALSC_UNINIT]
	{
		// [ALSC_NO_ACTION]
		{NULL},

		// [ALSC_ENABLE]
		{alsc_enable},

		// [ALSC_DISABLE]
		{NULL},

		// [ALSC_UPDATE_BL]
		{NULL},

		// [ALSC_UPDATE_DEGAMMA]
		{NULL},

		// [ALSC_SET_REG]
		{alsc_set_reg},

		// [ALSC_UPDATE_DIRTY_REGION]
		{NULL},

		// [ALSC_BLANK]
		{NULL},

		// [ALSC_UNBLANK]
		{NULL},

		// [ALSC_HANDLE_ISR]
		{NULL},
	},

	// [ALSC_WORKING]
	{
		// [ALSC_NO_ACTION]
		{NULL},

		// [ALSC_ENABLE]
		{alsc_enable},

		// [ALSC_DISABLE]
		{alsc_disable},

		// [ALSC_UPDATE_BL]
		{alsc_update_bl_param},

		// [ALSC_UPDATE_DEGAMMA]
		{alsc_update_degamma_coef},

		// [ALSC_SET_REG]
		{alsc_set_reg},

		// [ALSC_UPDATE_DIRTY_REGION]
		{NULL},

		// [ALSC_BLANK]
		{alsc_blank},

		// [ALSC_UNBLANK]
		{NULL},

		// [ALSC_HANDLE_ISR]
		{alsc_store_data},
	},

	// [ALSC_IDLE]
	{
		// [ALSC_NO_ACTION]
		{NULL},

		// [ALSC_ENABLE]
		{alsc_enable},

		// [ALSC_DISABLE]
		{alsc_disable},

		// [ALSC_UPDATE_BL]
		{alsc_update_bl_param},

		// [ALSC_UPDATE_DEGAMMA]
		{alsc_update_degamma_coef},

		// [ALSC_SET_REG]
		{NULL},

		// [ALSC_UPDATE_DIRTY_REGION]
		{NULL},

		// [ALSC_BLANK]
		{NULL},

		// [ALSC_UNBLANK]
		{alsc_unblank},

		// [ALSC_HANDLE_ISR]
		{NULL},
	},

	// [ALSC_NO_DATA]
	{
		// [ALSC_NO_ACTION]
		{NULL},

		// [ALSC_ENABLE]
		{alsc_enable},

		// [ALSC_DISABLE]
		{alsc_disable},

		// [ALSC_UPDATE_BL]
		{alsc_update_bl_param},

		// [ALSC_UPDATE_DEGAMMA]
		{alsc_update_degamma_coef},

		// [ALSC_SET_REG]
		{alsc_set_reg},

		// [ALSC_UPDATE_DIRTY_REGION]
		{alsc_update_dirty_region},

		// [ALSC_BLANK]
		{NULL},

		// [ALSC_UNBLANK]
		{alsc_unblank},

		// [ALSC_HANDLE_ISR]
		{NULL},
	}
};

static int32_t alsc_fsm_handle(const void *data, int32_t action)
{
	struct dpu_alsc *alsc = g_alsc;
	int32_t ret = 0;

	dpu_assert(!alsc);
	if (!g_enable_alsc || !alsc->is_inited)
		return -1;

	/* ISR action should not use mutex, and alsc status is not changed */
	if (action != ALSC_HANDLE_ISR)
		mutex_lock(&(alsc->alsc_lock));

	dpu_pr_debug("status=%d action=%d", alsc->status, action);
	if (g_alsc_fsm_tbl[alsc->status][action].handle)
		ret = g_alsc_fsm_tbl[alsc->status][action].handle(alsc, data);

	if (action != ALSC_HANDLE_ISR)
		mutex_unlock(&(alsc->alsc_lock));

	return ret;
}

void dpu_alsc_set_reg(const struct opr_cmd_data_base *data)
{
	dpu_assert(!data);
	alsc_fsm_handle(data, ALSC_SET_REG);
}

void dpu_alsc_store_data(uint32_t isr_flag)
{
	alsc_fsm_handle(&isr_flag, ALSC_HANDLE_ISR);
}

static void alsc_free_list_node(struct dpu_alsc_data *node)
{
	struct dpu_alsc_data *p = node;
	struct dpu_alsc_data *q = node;

	while (p) {
		q = p->next;
		kfree(p);
		p = q;
	}
}

static void alsc_free_data_storage(struct dpu_alsc *alsc)
{
	dpu_assert(!alsc->data_head);

	/* alsc_data hasn't been a cycle */
	/* head-->node1...-->tail
	 * head<--node1...<--tail
	 *  |                 |
	 *  V                 V
	 * NULL              NULL
	 */
	if (!alsc->data_head->prev) {
		alsc_free_list_node(alsc->data_head);
		return;
	}

	/* alsc_data has been a cycle */
	/* head-->node1...-->tail
	 * head<--node1...<--tail
	 *  |^                |^
	 *  ||________________||
	 *  |__________________|
	 */
	alsc->data_head->prev->next = NULL;
	alsc_free_list_node(alsc->data_head);
}

static int32_t alsc_init_data_storage(struct dpu_alsc *alsc)
{
	struct dpu_alsc_data *p = NULL;
	struct dpu_alsc_data *head = NULL;
	int32_t i = 1;

	head = (struct dpu_alsc_data*)kzalloc(sizeof(struct dpu_alsc_data), GFP_ATOMIC);
	if (!head) {
		dpu_pr_err("[ALSC]alsc_init_data_storage failed");
		return -1;
	}

	/* First node of alsc data cycle */
	alsc->data_head = head;
	alsc->data_tail = head;

	/* rest node of alsc data cycle except for last node */
	for (; i < ALSC_MAX_DATA_LEN - 1; ++i) {
		p = (struct dpu_alsc_data*)kzalloc(sizeof(struct dpu_alsc_data), GFP_ATOMIC);
		if (!p) {
			alsc_free_data_storage(alsc);
			dpu_pr_err("[ALSC]alsc_init_data_storage failed");
			return -1;
		}
		head->next = p;
		p->prev = head;
		p->next = NULL;
		head = p;
	}

	/* last node of alsc data cycle */
	p = (struct dpu_alsc_data*)kzalloc(sizeof(struct dpu_alsc_data), GFP_ATOMIC);
	if (!p) {
		alsc_free_data_storage(alsc);
		dpu_pr_err("[ALSC]alsc_init_data_storage failed");
		return -1;
	}
	head->next = p;
	p->prev = head;
	p->next = alsc->data_head;
	alsc->data_head->prev = p;
	dpu_pr_debug("[ALSC]alsc_init_data_storage success");

	return 0;
}

void dpu_alsc_init(struct dpu_alsc *alsc)
{
	if (!alsc) {
		dpu_pr_err("alsc is NULL");
		return;
	}

	if (!alsc->is_inited) {
		if (alsc_init_data_storage(alsc))
			return;

		mutex_init(&(alsc->alsc_lock));
		alsc->status = ALSC_UNINIT;
		alsc->is_inited = true;
		dpu_pr_debug("[ALSC]resources init success");
	}

	// TODO: temporarily add here for chip verify
	g_alsc = alsc;
	dkmd_alsc_enable(&g_in_alsc);
}

void dpu_alsc_deinit(struct dpu_alsc *alsc)
{
	if (!alsc) {
		dpu_pr_err("alsc is NULL");
		return;
	}

	if (alsc->is_inited) {
		alsc_free_data_storage(alsc);
		mutex_destroy(&(alsc->alsc_lock));
	}
	alsc->is_inited = false;
}

/* Interfaces' implementation exposed to input_hub */
int32_t dkmd_alsc_enable(const struct dkmd_alsc *in_alsc)
{
	if (!in_alsc) {
		dpu_pr_err("[ALSC]in_alsc is NULL");
		return -1;
	}

	return alsc_fsm_handle(in_alsc, ALSC_ENABLE);
}
EXPORT_SYMBOL(dkmd_alsc_enable);

int32_t dkmd_alsc_disable(void)
{
	return alsc_fsm_handle(NULL, ALSC_DISABLE);
}
EXPORT_SYMBOL(dkmd_alsc_disable);

int32_t dkmd_alsc_update_bl_param(const struct alsc_bl_param *bl_param)
{
	if (!bl_param) {
		dpu_pr_err("[ALSC]bl_param is NULL");
		return -1;
	}

	return alsc_fsm_handle(bl_param, ALSC_UPDATE_BL);
}
EXPORT_SYMBOL(dkmd_alsc_update_bl_param);

int dkmd_alsc_update_degamma_coef(const struct alsc_degamma_coef *degamma_coef)
{
	if (!degamma_coef) {
		dpu_pr_err("[ALSC]degamma_coef is NULL");
		return -1;
	}

	return alsc_fsm_handle(degamma_coef, ALSC_UPDATE_DEGAMMA);
}
EXPORT_SYMBOL(dkmd_alsc_update_degamma_coef);

int32_t dkmd_alsc_register_cb_func(void (*func)(struct alsc_noise*), void (*send_ddic_alpha)(uint32_t))
{
	struct dpu_alsc *alsc = g_alsc;

	if (!func || !send_ddic_alpha) {
		dpu_pr_err("[ALSC]ALSC hasn't been initialized, wrong operation from input_hub");
		return -1;
	}

	if (!g_enable_alsc)
		return -1;

	dpu_pr_debug("[ALSC]+");

	alsc->cb_func.send_data_func = func;
	alsc->cb_func.send_ddic_alpha = send_ddic_alpha;

	return 0;
}
EXPORT_SYMBOL(dkmd_alsc_register_cb_func);