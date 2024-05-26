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

#define BIT_VACTIVE0_START  BIT(7)
#define BIT_VACTIVE0_END    BIT(8)

#define ALSC_MAX_DATA_LEN 3

#define ALSC_CLK_FREQ 192
#define ALSC_CLK_FREQ_DIVISOR 100

#define ALSC_PIC_WIDTH_SHIFT 13
#define ALSC_PIC_HEIGHT_MASK ((1UL << ALSC_PIC_WIDTH_SHIFT) - 1)

#define ALSC_ADDR_Y_SHIFT 13
#define ALSC_ADDR_X_MASK ((1UL << ALSC_ADDR_Y_SHIFT) - 1)

#define ALSC_SIZE_Y_SHIFT 8
#define ALSC_SIZE_X_MASK ((1UL << ALSC_SIZE_Y_SHIFT) - 1)

#define ALSC_DEGAMMA_VAL_MASK ((1UL << 13) - 1)

#define fb_enable_bit(val, i) ((val) | (1 << (i)))

#define fb_disable_bit(val, i) ((val) & (~(1 << (i))))

#define is_bit_enable(val, i) (!!((val) & (1 << (i))))

#define ALSC_OFFSET                 (0x000B4000)
#define ALSC_DEGAMMA_EN             (0x1900)
#define ALSC_DEGAMMA_MEM_CTRL       (0x1904)
#define ALSC_DEGAMMA_LUT_SEL        (0x1908)
#define ALSC_DEGAMMA_DBG0           (0x190c)
#define ALSC_DEGAMMA_DBG1           (0x1910)
#define ALSC_EN                     (0x1914)
#define ALSC_PPRO_BYPASS            (0x1918)
#define ALSC_SENSOR_CHANNEL         (0x191C)
#define ALSC_PIC_SIZE               (0x1920)
#define ALSC_ADDR                   (0x1924)
#define ALSC_SIZE                   (0x1928)
#define ALSC_NOISE1                 (0x192C)
#define ALSC_NOISE2                 (0x1930)
#define ALSC_NOISE3                 (0x1934)
#define ALSC_NOISE4                 (0x1938)
#define ALSC_COEF_R2R               (0x193C)
#define ALSC_COEF_R2G               (0x1940)
#define ALSC_COEF_R2B               (0x1944)
#define ALSC_COEF_R2C               (0x1948)
#define ALSC_COEF_G2R               (0x194C)
#define ALSC_COEF_G2G               (0x1950)
#define ALSC_COEF_G2B               (0x1954)
#define ALSC_COEF_G2C               (0x1958)
#define ALSC_COEF_B2R               (0x195C)
#define ALSC_COEF_B2G               (0x1960)
#define ALSC_COEF_B2B               (0x1964)
#define ALSC_COEF_B2C               (0x1968)
#define ALSC_TIMESTAMP_L            (0x196C)
#define ALSC_TIMESTAMP_H            (0x1970)
#define ALSC_DBG0                   (0x1974)
#define ALSC_DBG1                   (0x1978)
#define ALSC_ADDR_BLOCK             (0x1A00)
#define ALSC_AVE_BLOCK              (0x1A80)
#define ALSC_COEF_BLOCK_R_OFFSET    (0x1B00)
#define ALSC_COEF_BLOCK_G_OFFSET    (0x1B80)
#define ALSC_COEF_BLOCK_B_OFFSET    (0x1C00)
#define ALSC_COEF_BLOCK_C_OFFSET    (0x1C80)
#define U_DEGAMMA_R_COEF            (0x5000)
#define U_DEGAMMA_G_COEF            (0x5400)
#define U_DEGAMMA_B_COEF            (0x5800)


#define U_ALSC_DEGAMMA_R_COEF            (0x1C000)
#define U_ALSC_DEGAMMA_G_COEF            (0x1C400)
#define U_ALSC_DEGAMMA_B_COEF            (0x1C800)

#define ALSC_BLOCK_LEN              (30)
#define ALSC_DEGAMMA_COEF_LEN       (257)

#define BLOCK_LEN 30
#define DEGAMMA_COEF_LEN 257

enum alsc_action {
	ALSC_NO_ACTION,
	ALSC_ENABLE,
	ALSC_UPDATE,
	ALSC_BL_UPDATE,
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

struct hisi_dss_alsc {
	uint32_t action;
	uint32_t alsc_en;
	uint32_t sensor_channel;
	uint32_t addr;
	uint32_t size;
	struct alsc_bl_param bl_param;
	uint32_t addr_block[BLOCK_LEN];
	uint32_t ave_block[BLOCK_LEN];
	uint32_t coef_block_r[BLOCK_LEN];
	uint32_t coef_block_g[BLOCK_LEN];
	uint32_t coef_block_b[BLOCK_LEN];
	uint32_t coef_block_c[BLOCK_LEN];
	uint32_t degamma_lut_r[DEGAMMA_COEF_LEN];
	uint32_t degamma_lut_g[DEGAMMA_COEF_LEN];
	uint32_t degamma_lut_b[DEGAMMA_COEF_LEN];
};

enum alsc_status {
	ALSC_UNINIT,
	ALSC_WORKING,
	ALSC_BLANK,
	ALSC_STATUS_MAX_NUM
};

struct rect {
	uint32_t x1;
	uint32_t y1;
	uint32_t x2;
	uint32_t y2;
};


struct dss_alsc_cb_func {
	void (*send_data_func)(struct alsc_noise*);
};

struct dss_alsc {
	uint32_t status;
	uint32_t action;
	uint32_t degamma_en;
	uint32_t degamma_lut_sel;
	uint32_t alsc_en;
	uint32_t alsc_en_by_dirty_region_limit;
	uint32_t sensor_channel;
	uint32_t pic_size;
	uint32_t addr;
	uint32_t addr_default;
	uint32_t size;
	uint32_t size_default;
	struct alsc_bl_param bl_param;
	uint32_t bl_update_to_hwc;
	uint32_t addr_block[ALSC_BLOCK_LEN];
	uint32_t ave_block[ALSC_BLOCK_LEN];
	uint32_t coef_block_r[ALSC_BLOCK_LEN];
	uint32_t coef_block_g[ALSC_BLOCK_LEN];
	uint32_t coef_block_b[ALSC_BLOCK_LEN];
	uint32_t coef_block_c[ALSC_BLOCK_LEN];
	uint32_t degamma_r_coef[ALSC_DEGAMMA_COEF_LEN];
	uint32_t degamma_g_coef[ALSC_DEGAMMA_COEF_LEN];
	uint32_t degamma_b_coef[ALSC_DEGAMMA_COEF_LEN];
	struct mutex alsc_lock;
	struct dss_alsc_data* data_head;
	struct dss_alsc_data* data_tail;
	struct dss_alsc_cb_func cb_func;
};

struct dss_alsc_data {
	uint32_t status;
	uint32_t noise1;
	uint32_t noise2;
	uint32_t noise3;
	uint32_t noise4;
	uint32_t timestamp_l;
	uint32_t timestamp_h;
	uint64_t ktimestamp;
	struct dss_alsc_data* prev;
	struct dss_alsc_data* next;
};
#endif
