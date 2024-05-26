/* Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 */
#ifndef DISPLAY_EFFECT_ALSC_INTERFACE_H
#define DISPLAY_EFFECT_ALSC_INTERFACE_H

#define BLOCK_LEN 30
#define DEGAMMA_COEF_LEN 257

/* Data struct exposed to input_hub for alsc initialization and updating */
enum alsc_action {
	ALSC_NO_ACTION,
	ALSC_ENABLE,
	ALSC_UPDATE,
	ALSC_BL_UPDATE,
	ALSC_GAMMA_UPDATE,
	ALSC_LIMIT_UPDATE,
	ALSC_ACTION_MAX_BIT
};

struct alsc_bl_param {
	uint32_t coef_r2r;
	uint32_t coef_r2g;
	uint32_t coef_r2b;
	uint32_t coef_r2c;
	uint32_t coef_g2r;
	uint32_t coef_g2g;
	uint32_t coef_g2b;
	uint32_t coef_g2c;
	uint32_t coef_b2r;
	uint32_t coef_b2g;
	uint32_t coef_b2b;
	uint32_t coef_b2c;
};

struct alsc_noise {
	uint32_t status;
	uint32_t noise1;
	uint32_t noise2;
	uint32_t noise3;
	uint32_t noise4;
	uint64_t timestamp;
};

struct dpu_alsc {
	uint32_t action;
	uint32_t alsc_en;
	uint32_t sensor_channel;
	uint32_t addr;
	uint32_t size;
	struct alsc_bl_param bl_param;
	uint32_t addr_block[BLOCK_LEN];   // ADDR_BLOCK address range 0x1A00-0x1A74, which contains 30 blocks.
	uint32_t ave_block[BLOCK_LEN];    // AVE_BLOCK address range 0x1A80-0x1AF4, which contains 30 blocks.
	uint32_t coef_block_r[BLOCK_LEN]; // COEF_BLOCK_R address range 0x1B00-0x1B74, which contains 30 blocks.
	uint32_t coef_block_g[BLOCK_LEN]; // COEF_BLOCK_G address range 0x1B80-0x1BF4, which contains 30 blocks.
	uint32_t coef_block_b[BLOCK_LEN]; // COEF_BLOCK_B address range 0x1C00-0x1C74, which contains 30 blocks.
	uint32_t coef_block_c[BLOCK_LEN]; // COEF_BLOCK_C address range 0x1C80-0x1CF4, which contains 30 blocks.
	uint32_t degamma_lut_r[DEGAMMA_COEF_LEN];
	uint32_t degamma_lut_g[DEGAMMA_COEF_LEN];
	uint32_t degamma_lut_b[DEGAMMA_COEF_LEN];
};

/* Interfaces exposed to input_hub and these will be implemented
 * in display_effect_alsc.c
 */
int dss_alsc_init(const struct dpu_alsc* const in_alsc);
int dss_alsc_update_bl_param(const struct alsc_bl_param* const bl_param);
int dss_alsc_update_degamma_coef(const struct dpu_alsc* const in_alsc);
int dss_alsc_register_cb_func(void (*func)(struct alsc_noise*), void (*send_ddic_alpha)(uint32_t));
#endif
