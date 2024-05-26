/* Copyright (c) 2019-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/compiler.h>
#include "hisi_disp_debug.h"
#include "hisi_disp_gadgets.h"
#include "display_effect_alsc.h"

bool g_reg_inited = false;
bool g_noise_got = false;
int g_enable_alsc = 1;
bool g_is_alsc_init = false;
static u32 g_addr_block[ALSC_BLOCK_LEN] = {
	0x2e2e2b2b, 0x2e2a2929, 0x2a30292b, 0x30302b2f, 0x302e2f29,
	0x30282525, 0x28342529, 0x34342931, 0x34303125, 0x34242020,
	0x24392025, 0x39392535, 0x39343520, 0x391f1919, 0x1f401920,
	0x4040203a, 0x40393a19, 0x40181111, 0x18481119, 0x48481941,
	0x48404111, 0x48100000, 0x10590011, 0x59591149, 0x59484900,
	0, 0, 0, 0, 0
};

static u32 g_ave_block[ALSC_BLOCK_LEN] = {
	4096, 5461, 5461, 5461, 5461, 1365, 1365, 1365, 1365, 624, 624, 624, 624,
	284, 284, 284, 284, 171, 171, 171, 171, 53, 53, 53, 53, 0, 0, 0, 0, 0
};

static u32 g_alsc_coef_block_r[ALSC_BLOCK_LEN] = {
	3265, 2099, 2177, 2410, 2099, 4276, 4898, 5442, 4587, 3498,
	4120, 4587, 3965, 2410, 2721, 3187, 2721, 1244, 1011, 1322,
	1166, 544, 544, 700, 544, 0, 0, 0, 0, 0
};

static u32 g_alsc_coef_block_g[ALSC_BLOCK_LEN] = {
	3298, 1979, 2039, 2338, 2039, 4497, 4617, 5876, 4677, 3657,
	3657, 5097, 3837, 2518, 2338, 3238, 2458, 1319, 959, 1439,
	1079, 600, 600, 839, 540, 0, 0, 0, 0, 0
};

static u32 g_alsc_coef_block_bc[ALSC_BLOCK_LEN] = {
	4638, 2722, 2823, 3428, 2823, 5041, 5243, 6856, 5545, 3126,
	3327, 4335, 3529, 1613, 1714, 2218, 1815, 807, 605, 907,
	706, 403, 403, 504, 403, 0, 0, 0, 0, 0
};

static u32 g_gamma_lut_table[ALSC_DEGAMMA_COEF_LEN] = {
	0x0000, 0x0001, 0x0002, 0x0004, 0x0005, 0x0006, 0x0007, 0x0009,
	0x000a, 0x000b, 0x000c, 0x000e, 0x000f, 0x0010, 0x0012, 0x0013,
	0x0015, 0x0017, 0x0019, 0x001b, 0x001d, 0x001f, 0x0021, 0x0023,
	0x0025, 0x0028, 0x002a, 0x002d, 0x002f, 0x0032, 0x0035, 0x0038,
	0x003b, 0x003e, 0x0041, 0x0045, 0x0048, 0x004b, 0x004f, 0x0053,
	0x0057, 0x005a, 0x005e, 0x0063, 0x0067, 0x006b, 0x006f, 0x0074,
	0x0079, 0x007d, 0x0082, 0x0087, 0x008c, 0x0091, 0x0097, 0x009c,
	0x00a1, 0x00a7, 0x00ad, 0x00b2, 0x00b8, 0x00be, 0x00c5, 0x00cb,
	0x00d1, 0x00d8, 0x00de, 0x00e5, 0x00ec, 0x00f3, 0x00fa, 0x0101,
	0x0108, 0x0110, 0x0117, 0x011f, 0x0127, 0x012f, 0x0137, 0x013f,
	0x0147, 0x0150, 0x0158, 0x0161, 0x016a, 0x0173, 0x017c, 0x0185,
	0x018e, 0x0198, 0x01a1, 0x01ab, 0x01b5, 0x01bf, 0x01c9, 0x01d3,
	0x01dd, 0x01e8, 0x01f2, 0x01fd, 0x0208, 0x0213, 0x021e, 0x0229,
	0x0235, 0x0240, 0x024c, 0x0258, 0x0264, 0x0270, 0x027c, 0x0289,
	0x0295, 0x02a2, 0x02af, 0x02bb, 0x02c9, 0x02d6, 0x02e3, 0x02f1,
	0x02fe, 0x030c, 0x031a, 0x0328, 0x0336, 0x0345, 0x0353, 0x0362,
	0x0371, 0x0380, 0x038f, 0x039e, 0x03ad, 0x03bd, 0x03cd, 0x03dd,
	0x03ed, 0x03fd, 0x040d, 0x041d, 0x042e, 0x043f, 0x0450, 0x0461,
	0x0472, 0x0483, 0x0495, 0x04a6, 0x04b8, 0x04ca, 0x04dc, 0x04ef,
	0x0501, 0x0514, 0x0526, 0x0539, 0x054c, 0x0560, 0x0573, 0x0587,
	0x059a, 0x05ae, 0x05c2, 0x05d6, 0x05eb, 0x05ff, 0x0614, 0x0629,
	0x063e, 0x0653, 0x0668, 0x067e, 0x0693, 0x06a9, 0x06bf, 0x06d5,
	0x06eb, 0x0702, 0x0718, 0x072f, 0x0746, 0x075d, 0x0775, 0x078c,
	0x07a4, 0x07bb, 0x07d3, 0x07eb, 0x0804, 0x081c, 0x0835, 0x084e,
	0x0867, 0x0880, 0x0899, 0x08b3, 0x08cc, 0x08e6, 0x0900, 0x091a,
	0x0935, 0x094f, 0x096a, 0x0985, 0x09a0, 0x09bb, 0x09d6, 0x09f2,
	0x0a0d, 0x0a29, 0x0a45, 0x0a62, 0x0a7e, 0x0a9b, 0x0ab8, 0x0ad5,
	0x0af2, 0x0b0f, 0x0b2c, 0x0b4a, 0x0b68, 0x0b86, 0x0ba4, 0x0bc3,
	0x0be1, 0x0c00, 0x0c1f, 0x0c3e, 0x0c5d, 0x0c7d, 0x0c9c, 0x0cbc,
	0x0cdc, 0x0cfd, 0x0d1d, 0x0d3e, 0x0d5e, 0x0d7f, 0x0da0, 0x0dc2,
	0x0de3, 0x0e05, 0x0e27, 0x0e49, 0x0e6b, 0x0e8d, 0x0eb0, 0x0ed3,
	0x0ef6, 0x0f19, 0x0f3c, 0x0f60, 0x0f84, 0x0fa8, 0x0fcc, 0x0ff0,
	0x1fff,
};

static void alsc_block_param_config(struct dss_alsc *alsc, const struct hisi_dss_alsc* const in_alsc)
{
	int block_cnt = 0;

	for (; block_cnt < ALSC_BLOCK_LEN; ++block_cnt) {
		alsc->addr_block[block_cnt]   = in_alsc->addr_block[block_cnt];
		alsc->ave_block[block_cnt]    = in_alsc->ave_block[block_cnt];
		alsc->coef_block_r[block_cnt] = in_alsc->coef_block_r[block_cnt];
		alsc->coef_block_g[block_cnt] = in_alsc->coef_block_g[block_cnt];
		alsc->coef_block_b[block_cnt] = in_alsc->coef_block_b[block_cnt];
		alsc->coef_block_c[block_cnt] = in_alsc->coef_block_c[block_cnt];
	}
}

static void alsc_degamma_coef_config(struct dss_alsc *alsc)
{
	int block_cnt = 0;

	for (; block_cnt < ALSC_DEGAMMA_COEF_LEN; ++block_cnt) {
		alsc->degamma_r_coef[block_cnt] = g_gamma_lut_table[block_cnt];
		alsc->degamma_g_coef[block_cnt] = g_gamma_lut_table[block_cnt];
		alsc->degamma_b_coef[block_cnt] = g_gamma_lut_table[block_cnt];
	}
}

static void alsc_param_debug_print(const struct dss_alsc* const alsc)
{
	disp_pr_info("action=%u "
		"alsc_en_by_dirty_region_limit=%u "
		"degamma_en=%u "
		"degamma_lut_sel=%u "
		"alsc_en=%u "
		"sensor_channel=%u "
		"pic_size=0x%x "
		"addr=0x%x "
		"size=0x%x "
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
		"bl_param.coef_b2b=%u"
		"bl_param.coef_b2c=%u\n",
		alsc->action,
		alsc->alsc_en_by_dirty_region_limit,
		alsc->degamma_en,
		alsc->degamma_lut_sel,
		alsc->alsc_en,
		alsc->sensor_channel,
		alsc->pic_size,
		alsc->addr,
		alsc->size,
		alsc->bl_param.coef_r2r,
		alsc->bl_param.coef_r2g,
		alsc->bl_param.coef_r2b,
		alsc->bl_param.coef_r2c,
		alsc->bl_param.coef_g2r,
		alsc->bl_param.coef_g2g,
		alsc->bl_param.coef_g2b,
		alsc->bl_param.coef_g2c,
		alsc->bl_param.coef_b2r,
		alsc->bl_param.coef_b2g,
		alsc->bl_param.coef_b2b,
		alsc->bl_param.coef_b2c);
}

static void alsc_reg_param_config(struct hisi_operator_dpp *hisi_opert,
	const struct hisi_dss_alsc* const in_alsc)
{
	struct dss_alsc *alsc = &(hisi_opert->alsc);
	alsc->action |= BIT(ALSC_ENABLE);

	alsc->action            |= in_alsc->action;
	alsc->degamma_en		= in_alsc->alsc_en;
	alsc->degamma_lut_sel   = 0;
	alsc->alsc_en           = in_alsc->alsc_en;
	alsc->alsc_en_by_dirty_region_limit = in_alsc->alsc_en;
	alsc->sensor_channel    = in_alsc->sensor_channel;
	alsc->pic_size			= 0x86e77f;

	alsc->addr              = in_alsc->addr;
	alsc->addr_default      = in_alsc->addr;
	alsc->size              = in_alsc->size;
	alsc->size_default      = in_alsc->size;
	alsc->bl_param.coef_r2r = in_alsc->bl_param.coef_r2r;
	alsc->bl_param.coef_r2g = in_alsc->bl_param.coef_r2g;
	alsc->bl_param.coef_r2b = in_alsc->bl_param.coef_r2b;
	alsc->bl_param.coef_r2c = in_alsc->bl_param.coef_r2c;
	alsc->bl_param.coef_g2r = in_alsc->bl_param.coef_g2r;
	alsc->bl_param.coef_g2g = in_alsc->bl_param.coef_g2g;
	alsc->bl_param.coef_g2b = in_alsc->bl_param.coef_g2b;
	alsc->bl_param.coef_g2c = in_alsc->bl_param.coef_g2c;
	alsc->bl_param.coef_b2r = in_alsc->bl_param.coef_b2r;
	alsc->bl_param.coef_b2g = in_alsc->bl_param.coef_b2g;
	alsc->bl_param.coef_b2b = in_alsc->bl_param.coef_b2b;
	alsc->bl_param.coef_b2c = in_alsc->bl_param.coef_b2c;

	alsc_block_param_config(alsc, in_alsc);
	alsc_degamma_coef_config(alsc);
	disp_pr_info("[ALSC]ALSC init or update param loaded\n");
}


static void alsc_block_param_set_reg(const struct dss_alsc *alsc, char __iomem *alsc_base)
{
	int block_cnt = 0;
	int block_offset = 0;

	for (; block_cnt < ALSC_BLOCK_LEN; ++block_cnt) {
		block_offset = 0x4 * block_cnt;
		outp32(alsc_base + ALSC_ADDR_BLOCK + block_offset, g_addr_block[block_cnt]);
		outp32(alsc_base + ALSC_AVE_BLOCK + block_offset, g_ave_block[block_cnt]);
		outp32(alsc_base + ALSC_COEF_BLOCK_R_OFFSET + block_offset, g_alsc_coef_block_r[block_cnt]);
		outp32(alsc_base + ALSC_COEF_BLOCK_G_OFFSET + block_offset, g_alsc_coef_block_g[block_cnt]);
		outp32(alsc_base + ALSC_COEF_BLOCK_B_OFFSET + block_offset, g_alsc_coef_block_bc[block_cnt]);
		outp32(alsc_base + ALSC_COEF_BLOCK_C_OFFSET + block_offset, g_alsc_coef_block_bc[block_cnt]);
	}
}

static void alsc_degamma_coef_init_set_reg(const struct dss_alsc *alsc, char __iomem *alsc_base)
{
	int block_cnt = 0;
	int block_offset = 0;
	uint32_t val = 0;

	for (; block_cnt < ALSC_DEGAMMA_COEF_LEN - 1; block_cnt += 2) {
		block_offset = 0x2 * block_cnt;
		val = (g_gamma_lut_table[block_cnt] & ALSC_DEGAMMA_VAL_MASK);
		val |= ((g_gamma_lut_table[block_cnt + 1] & ALSC_DEGAMMA_VAL_MASK) << 16);
		outp32(alsc_base + U_ALSC_DEGAMMA_R_COEF + block_offset, val);

		val = (g_gamma_lut_table[block_cnt] & ALSC_DEGAMMA_VAL_MASK);
		val |= ((g_gamma_lut_table[block_cnt + 1] & ALSC_DEGAMMA_VAL_MASK) << 16);
		outp32(alsc_base + U_ALSC_DEGAMMA_G_COEF + block_offset, val);

		val = (g_gamma_lut_table[block_cnt] & ALSC_DEGAMMA_VAL_MASK);
		val |= ((g_gamma_lut_table[block_cnt + 1] & ALSC_DEGAMMA_VAL_MASK) << 16);
		outp32(alsc_base + U_ALSC_DEGAMMA_B_COEF + block_offset, val);
	}

	block_offset += 0x4;
	outp32(alsc_base + U_ALSC_DEGAMMA_R_COEF + block_offset,
		g_gamma_lut_table[block_cnt] & ALSC_DEGAMMA_VAL_MASK);
	outp32(alsc_base + U_ALSC_DEGAMMA_G_COEF + block_offset,
		g_gamma_lut_table[block_cnt] & ALSC_DEGAMMA_VAL_MASK);
	outp32(alsc_base + U_ALSC_DEGAMMA_B_COEF + block_offset,
		g_gamma_lut_table[block_cnt] & ALSC_DEGAMMA_VAL_MASK);
}

static void alsc_degamma_coef_init_set_reg_debug(const struct dss_alsc *alsc, char __iomem *alsc_base)
{
	int block_cnt = 0;
	int block_offset = 0;
	uint32_t val = 0;

	for (; block_cnt < ALSC_DEGAMMA_COEF_LEN - 1; block_cnt += 2) {
		block_offset = 0x2 * block_cnt;
		val = 0x00001fff;
		val |= 0x00001fff << 16;
		outp32(alsc_base + U_ALSC_DEGAMMA_R_COEF + block_offset, val);

		val = 0x00001fff;
		val |= 0x00001fff << 16;
		outp32(alsc_base + U_ALSC_DEGAMMA_G_COEF + block_offset, val);

		val = 0x00001fff;
		val |= 0x00001fff << 16;
		outp32(alsc_base + U_ALSC_DEGAMMA_B_COEF + block_offset, val);
	}

	block_offset += 0x4;
	disp_pr_info("wry: g_gamma_lut_table[block_cnt] & ALSC_DEGAMMA_VAL_MASK = 0x%x\n",
		g_gamma_lut_table[block_cnt] & ALSC_DEGAMMA_VAL_MASK);
	disp_pr_info("wry: g_gamma_lut_table[block_cnt] & ALSC_DEGAMMA_VAL_MASK = 0x%x\n",
		g_gamma_lut_table[block_cnt] & ALSC_DEGAMMA_VAL_MASK);
	disp_pr_info("wry: g_gamma_lut_table[block_cnt] & ALSC_DEGAMMA_VAL_MASK = 0x%x\n",
		g_gamma_lut_table[block_cnt] & ALSC_DEGAMMA_VAL_MASK);

	outp32(alsc_base + U_ALSC_DEGAMMA_R_COEF + block_offset, 0x00001fff);
	outp32(alsc_base + U_ALSC_DEGAMMA_G_COEF + block_offset, 0x00001fff);
	outp32(alsc_base + U_ALSC_DEGAMMA_B_COEF + block_offset, 0x00001fff);
}

static void alsc_bl_param_set_reg(struct hisi_operator_dpp *hisi_opert, char __iomem *dpu_base_addr)
{
	char __iomem *alsc_base = NULL;
	struct dss_alsc *alsc = NULL;

	alsc_base = dpu_base_addr + ALSC_OFFSET;
	alsc = &(hisi_opert->alsc);

	disp_pr_info("wry: alsc + ALSC_COEF_R2R addr is 0x%p, alsc->bl_param.coef_r2r = %u\n",
		alsc_base + ALSC_COEF_R2R, alsc->bl_param.coef_r2r);
	outp32(alsc_base + ALSC_COEF_R2R, alsc->bl_param.coef_r2r);
	outp32(alsc_base + ALSC_COEF_R2G, alsc->bl_param.coef_r2g);
	outp32(alsc_base + ALSC_COEF_R2B, alsc->bl_param.coef_r2b);
	outp32(alsc_base + ALSC_COEF_R2C, alsc->bl_param.coef_r2c);
	outp32(alsc_base + ALSC_COEF_G2R, alsc->bl_param.coef_g2r);
	outp32(alsc_base + ALSC_COEF_G2G, alsc->bl_param.coef_g2g);
	outp32(alsc_base + ALSC_COEF_G2B, alsc->bl_param.coef_g2b);
	outp32(alsc_base + ALSC_COEF_G2C, alsc->bl_param.coef_g2c);
	outp32(alsc_base + ALSC_COEF_B2R, alsc->bl_param.coef_b2r);
	outp32(alsc_base + ALSC_COEF_B2G, alsc->bl_param.coef_b2g);
	outp32(alsc_base + ALSC_COEF_B2B, alsc->bl_param.coef_b2b);
	outp32(alsc_base + ALSC_COEF_B2C, alsc->bl_param.coef_b2c);
}

static void alsc_init_set_reg(struct hisi_operator_dpp *hisi_opert, char __iomem *dpu_base_addr)
{
	char __iomem *alsc_base = dpu_base_addr + ALSC_OFFSET;
	struct dss_alsc *alsc = &(hisi_opert->alsc);

	disp_pr_info("wry: alsc addr is 0x%p\n", alsc_base);
	outp32(alsc_base + ALSC_DEGAMMA_EN, 0);
	outp32(alsc_base + ALSC_EN, 0);
	outp32(alsc_base + ALSC_DEGAMMA_MEM_CTRL, 0x8);
	outp32(alsc_base + ALSC_SENSOR_CHANNEL, alsc->sensor_channel);
	outp32(alsc_base + ALSC_PIC_SIZE, alsc->pic_size);
	outp32(alsc_base + ALSC_ADDR, alsc->addr);
	outp32(alsc_base + ALSC_SIZE, alsc->size);
	alsc_bl_param_set_reg(hisi_opert, dpu_base_addr);
	alsc_block_param_set_reg(alsc, alsc_base);
	alsc_degamma_coef_init_set_reg(alsc, alsc_base);
	outp32(alsc_base + 0x9a0, 0x2);
	outp32(alsc_base + ALSC_DEGAMMA_EN, alsc->alsc_en);
	outp32(alsc_base + ALSC_EN, alsc->alsc_en);
}

static void alsc_frame_set_reg(struct hisi_operator_dpp *hisi_opert, char __iomem *dpu_base_addr)
{
	char __iomem *alsc_base = dpu_base_addr + ALSC_OFFSET;
	struct dss_alsc *alsc = &(hisi_opert->alsc);
	uint32_t val = 0;

	val = alsc->alsc_en & alsc->alsc_en_by_dirty_region_limit;
	if (val)
		alsc->status = fb_enable_bit(alsc->status, ALSC_WORKING);
	else
		alsc->status = fb_disable_bit(alsc->status, ALSC_WORKING);

	outp32(alsc_base + ALSC_PIC_SIZE, alsc->pic_size);
	outp32(alsc_base + ALSC_ADDR, alsc->addr);
	alsc_bl_param_set_reg(hisi_opert, dpu_base_addr);
}

void hisi_alsc_set_reg(struct hisi_operator_dpp *hisi_opert, char __iomem *dpu_base_addr)
{
	char __iomem *alsc_base = NULL;
	struct dss_alsc *alsc = NULL;

	alsc_base = dpu_base_addr + ALSC_OFFSET;
	alsc = &(hisi_opert->alsc);

	if (!g_enable_alsc) {
		outp32(alsc_base + ALSC_DEGAMMA_EN, 0);
		outp32(alsc_base + ALSC_EN, 0);
		return;
	}

	if (likely(!is_bit_enable(alsc->action, ALSC_ENABLE)))
		alsc_frame_set_reg(hisi_opert, dpu_base_addr);
	else
		alsc_init_set_reg(hisi_opert, dpu_base_addr);

	alsc->action = BIT(ALSC_NO_ACTION);

	g_reg_inited = true;
	g_noise_got = false;
	alsc_param_debug_print(alsc);
}

static void alsc_free_list_node(struct dss_alsc_data *node)
{
	struct dss_alsc_data *p = node;
	struct dss_alsc_data *q = node;

	if (!node)
		return;

	while (p) {
		q = p->next;
		kfree(p);
		p = q;
	}
}

static void alsc_free_data_storage(struct hisi_operator_dpp *hisi_opert)
{
	if (!hisi_opert->alsc.data_head)
		return;

	/* alsc_data hasn't been a cycle */
	/* head-->node1...-->tail
	 * head<--node1...<--tail
	 *  |                 |
	 *  V                 V
	 * NULL              NULL
	 */
	if (!(hisi_opert->alsc.data_head->prev)) {
		alsc_free_list_node(hisi_opert->alsc.data_head);
		return;
	}

	/* alsc_data has been a cycle */
	/* head-->node1...-->tail
	 * head<--node1...<--tail
	 *  |^                |^
	 *  ||________________||
	 *  |__________________|
	 */
	hisi_opert->alsc.data_head->prev->next = NULL;
	alsc_free_list_node(hisi_opert->alsc.data_head);
}

static int alsc_init_data_storage(struct hisi_operator_dpp *hisi_opert)
{
	struct dss_alsc_data *p = NULL;
	struct dss_alsc_data *head = NULL;
	int i = 1;

	head = (struct dss_alsc_data*)kzalloc(sizeof(struct dss_alsc_data), GFP_ATOMIC);
	if (!head) {
		disp_pr_info("[ALSC]alsc_init_data_storage failed\n");
		return -1;
	}
	/* First node of alsc data cycle */
	hisi_opert->alsc.data_head = head;
	hisi_opert->alsc.data_tail = head;

	/* rest node of alsc data cycle except for last node */
	for (; i < ALSC_MAX_DATA_LEN - 1; ++i) {
		p = (struct dss_alsc_data*)kzalloc(sizeof(struct dss_alsc_data), GFP_ATOMIC);
		if (!p) {
			alsc_free_data_storage(hisi_opert);
			disp_pr_info("[ALSC]alsc_init_data_storage failed\n");
			return -1;
		}
		head->next = p;
		p->prev = head;
		p->next = NULL;
		head = p;
	}

	/* last node of alsc data cycle */
	p = (struct dss_alsc_data*)kzalloc(sizeof(struct dss_alsc_data), GFP_ATOMIC);
	if (!p) {
		alsc_free_data_storage(hisi_opert);
		disp_pr_info("[ALSC]alsc_init_data_storage failed\n");
		return -1;
	}
	head->next = p;
	p->prev = head;
	p->next = hisi_opert->alsc.data_head;
	hisi_opert->alsc.data_head->prev = p;
	disp_pr_info("[ALSC]alsc_init_data_storage success\n");

	return 0;
}

void hisi_alsc_store_data(struct hisi_operator_dpp *hisi_opert, uint32_t isr_flag, char __iomem *dpu_base_addr)
{
	static uint32_t control_status;
	struct dss_alsc_data *p_data = NULL;
	char __iomem *alsc_base = NULL;
	static uint32_t read_noise_cnt = 0;

	if (!g_enable_alsc || !g_is_alsc_init) {
		disp_pr_info("g_is_alsc_init = %d \n", g_is_alsc_init);
		return;
	}

	disp_pr_info("read noise cnt = %d", read_noise_cnt);
	if (g_reg_inited)
		disp_pr_info("[ALSC]hisi_alsc_store_data enter!!!\n");
	else
		return;

	disp_pr_err("[wry] BIT_VACTIVE0_END come and store log for test\n");

	p_data = hisi_opert->alsc.data_head;
	alsc_base = dpu_base_addr + ALSC_OFFSET;

	if (!p_data) {
		disp_pr_info("[ALSC]alsc data storage is NULL\n");
		return;
	}

	if (isr_flag & BIT_VACTIVE0_START) {
		/* we don't have control_flag here cause Timestamp must
		 * occur first
		 */
		p_data->timestamp_l = inp32(alsc_base + ALSC_TIMESTAMP_L);
		p_data->timestamp_h = inp32(alsc_base + ALSC_TIMESTAMP_H);
		control_status = hisi_opert->alsc.status;
		disp_pr_info("[ALSC]store timestamp [0x%x 0x%x]\n", p_data->timestamp_l, p_data->timestamp_h);
	}

	if ((isr_flag & BIT_VACTIVE0_END)) {
		if (!is_bit_enable(control_status, ALSC_WORKING) || is_bit_enable(control_status, ALSC_BLANK))
			disp_pr_info("control_status is 0x%x\n", control_status);

		/* we need control_status here cause Noise should send or not must judge after
		 * VACTIVE0_START occurs
		 */
		p_data->noise1 = inp32(alsc_base + ALSC_NOISE1);
		p_data->noise2 = inp32(alsc_base + ALSC_NOISE2);
		p_data->noise3 = inp32(alsc_base + ALSC_NOISE3);
		p_data->noise4 = inp32(alsc_base + ALSC_NOISE4);
		p_data->status = 1;

		/* Move to next node of the alsc data ring cycle */
		hisi_opert->alsc.data_head = p_data->next;


		if (p_data->noise1 * p_data->noise2 * p_data->noise3 * p_data->noise4 == 0){
			disp_pr_info("read noise +++\n");
			return;
		} else {
			read_noise_cnt++;

			if (read_noise_cnt == 5) {
				g_noise_got = true;
				read_noise_cnt = 0;
				disp_pr_info("[ALSC]store noise [%u %u %u %u]\n",
					p_data->noise1, p_data->noise2, p_data->noise3, p_data->noise4);
			}
		}
	}
}

static int alsc_update_bl_param(struct hisi_operator_dpp *hisi_opert,
	const struct alsc_bl_param* const in_bl_param)
{
	struct dss_alsc *alsc = &(hisi_opert->alsc);

	alsc->action |= BIT(ALSC_BL_UPDATE);
	alsc->bl_update_to_hwc = 1;
	alsc->bl_param.coef_r2r = in_bl_param->coef_r2r;
	alsc->bl_param.coef_r2g = in_bl_param->coef_r2g;
	alsc->bl_param.coef_r2b = in_bl_param->coef_r2b;
	alsc->bl_param.coef_r2c = in_bl_param->coef_r2c;
	alsc->bl_param.coef_g2r = in_bl_param->coef_g2r;
	alsc->bl_param.coef_g2g = in_bl_param->coef_g2g;
	alsc->bl_param.coef_g2b = in_bl_param->coef_g2b;
	alsc->bl_param.coef_g2c = in_bl_param->coef_g2c;
	alsc->bl_param.coef_b2r = in_bl_param->coef_b2r;
	alsc->bl_param.coef_b2g = in_bl_param->coef_b2g;
	alsc->bl_param.coef_b2b = in_bl_param->coef_b2b;
	alsc->bl_param.coef_b2c = in_bl_param->coef_b2c;
	return 0;
}

void hisi_alsc_init(struct hisi_operator_dpp *hisi_opert)
{
	struct hisi_dss_alsc local_alsc = {
		1,
		1,
		1,
		0,
		0x5959,
		{
			5214481,
			1151249,
			203161,
			5214481,

			1354410,
			5620804,
			677205,
			1354410,

			67720,
			1015808,
			3250585,
			67720,
		},
		g_addr_block,
		g_ave_block,
		g_alsc_coef_block_r,
		g_alsc_coef_block_g,
		g_alsc_coef_block_bc,
		g_alsc_coef_block_bc,
		g_gamma_lut_table,
		g_gamma_lut_table,
		g_gamma_lut_table,
	};

	disp_pr_info("[wry] enter hisi_alsc_init g_is_alsc_init = %d\n", g_is_alsc_init);

	if (!g_is_alsc_init) {
		disp_pr_info("[ALSC][first config] fb0, g_is_alsc_init = %d\n", g_is_alsc_init);
		if (alsc_init_data_storage(hisi_opert))
			disp_pr_info("data init fail!!!!!!\n");

		hisi_opert->alsc.status = BIT(ALSC_WORKING);
		g_is_alsc_init = true;
		disp_pr_info("[ALSC]fb0, resources init success\n");

		alsc_reg_param_config(hisi_opert, &local_alsc);
	}
}
