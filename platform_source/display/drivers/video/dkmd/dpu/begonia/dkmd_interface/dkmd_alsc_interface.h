/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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
#ifndef DKMD_ALSC_INTERFACE_H
#define DKMD_ALSC_INTERFACE_H

#define ALSC_BLOCK_LEN 30
#define ALSC_DEGAMMA_COEF_LEN 257

/* Data struct exposed to input_hub for alsc initialization and updating */
enum alsc_action {
	ALSC_NO_ACTION,
	ALSC_ENABLE,
	ALSC_DISABLE,
	ALSC_UPDATE_BL,
	ALSC_UPDATE_DEGAMMA,

	/* ALSC internal action */
	ALSC_SET_REG,
	ALSC_UPDATE_DIRTY_REGION,
	ALSC_BLANK,
	ALSC_UNBLANK,
	ALSC_HANDLE_ISR,

	ALSC_ACTION_MAX
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

struct alsc_degamma_coef {
	uint32_t r_coef[ALSC_DEGAMMA_COEF_LEN]; // address range 0x5000-0x51FC, which contains 129blocks.
	uint32_t g_coef[ALSC_DEGAMMA_COEF_LEN]; // address range 0x5400-0x55FC, which contains 129blocks.
	uint32_t b_coef[ALSC_DEGAMMA_COEF_LEN]; // address range 0x5800-0x59FC, which contains 129blocks.
};

struct alsc_noise {
	uint32_t status;
	uint32_t noise1;
	uint32_t noise2;
	uint32_t noise3;
	uint32_t noise4;
	uint64_t timestamp;
};

struct dkmd_alsc {
	uint32_t alsc_en;
	uint32_t sensor_channel;
	uint32_t addr;
	uint32_t size;
	struct alsc_bl_param bl_param;
	uint32_t addr_block[ALSC_BLOCK_LEN];   // ADDR_BLOCK address range 0x1A00-0x1A74, which contains 30 blocks.
	uint32_t ave_block[ALSC_BLOCK_LEN];    // AVE_BLOCK address range 0x1A80-0x1AF4, which contains 30 blocks.
	uint32_t coef_block_r[ALSC_BLOCK_LEN]; // COEF_BLOCK_R address range 0x1B00-0x1B74, which contains 30 blocks.
	uint32_t coef_block_g[ALSC_BLOCK_LEN]; // COEF_BLOCK_G address range 0x1B80-0x1BF4, which contains 30 blocks.
	uint32_t coef_block_b[ALSC_BLOCK_LEN]; // COEF_BLOCK_B address range 0x1C00-0x1C74, which contains 30 blocks.
	uint32_t coef_block_c[ALSC_BLOCK_LEN]; // COEF_BLOCK_C address range 0x1C80-0x1CF4, which contains 30 blocks.
	struct alsc_degamma_coef degamma_coef;
};

/* Interfaces exposed to input_hub and these will be implemented
 * in dpu_effect_alsc.c
 */
int dkmd_alsc_enable(const struct dkmd_alsc *in_alsc);
int dkmd_alsc_disable(void);
int dkmd_alsc_update_bl_param(const struct alsc_bl_param *bl_param);
int dkmd_alsc_update_degamma_coef(const struct alsc_degamma_coef *degamma_coef);
int dkmd_alsc_register_cb_func(void (*func)(struct alsc_noise*), void (*send_ddic_alpha)(uint32_t));

#endif
