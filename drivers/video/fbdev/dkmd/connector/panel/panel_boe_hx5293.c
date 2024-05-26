 /* Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
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

#include "dpu_conn_mgr.h"
#include "panel_dev.h"
#include "dsc_output_calc.h"

/*
 * attention:
 * 1. hx5293 udp panel default has no TE pulse, need pull down TE pin first manually, then TE pin can output pulse.
 * 2. hx5293 udp power(AVDD, AVEE) connect to panel through backlight chip(ktz8864),
 *    so need init ktz8864 first before power on panel.
 */
#define BOE_INIT 1

static char g_sleep_out[] = {
	0x11,
};

static char g_display_on[] = {
	0x29,
};

#if BOE_INIT
static char g_cmd_1[] = {
	0xb9,
	0xFF,  0x52, 0x93,
};

static char g_cmd_2[] = {
	0xc2,
	0x08, 0xFF, 0x00, 0x02, 0x4A, 0x14, 0x00,
};

static char g_cmd_3[] = {
	0xC6,
	0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x26, 0x1B,
};

static char g_cmd_4[] = {
	0xC7,
	0x00, 0x14,
};

static char g_cmd_5[] = {
	0xBB,
	0x01, 0x02, 0x0A,  0x01, 0x01,
};

static char g_cmd_6[] = {
	0xB1,
	0x38, 0x23, 0x23, 0x68, 0x68, 0x4A, 0x4F,
};

static char g_cmd_7[] = {
	0xB2,
	0x45, 0xC0, 0x7C, 0x06, 0x2F, 0x64, 0x00, 0x00, 0x00, 0x05, 0x20, 0x70, 0x03,
};

static char g_cmd_8[] = {
	0xB4,
	0x00, 0xFF, 0x04, 0x04, 0x03, 0x38, 0x10, 0x10, 0x04, 0x76, 0x02, 0x04, 0x13,
	0x14, 0x15, 0x01, 0x01, 0x00,
};

static char g_cmd_9[] = {
	0xB6,
	0x7B, 0x7B,
};

static char g_cmd_10[] = {
	0xBA,
	0x13, 0x23, 0x00, 0x83, 0x92, 0x80, 0x00, 0x01, 0x10, 0x00, 0x00, 0x00, 0x08,
	0x3D, 0x0A, 0xF3, 0x04, 0x00, 0x06, 0x40, 0x00, 0x83, 0x92, 0x80, 0x00, 0x00,
	0x06,
};

static char g_cmd_11[] = {
	0xBC,
	0x07,
};

static char g_cmd_12[] = {
	0xBF,
	0x18,
};

static char g_cmd_13[] = {
	0xC0,
	0x60, 0x60,
};

static char g_cmd_14[] = {
	0xCB,
	0x00, 0x44,
};

static char g_cmd_15[] = {
	0xCC,
	0x02, 0x02,
};

static char g_cmd_16[] = {
	0xD5,
	0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x23, 0x22, 0x19, 0x19,
	0x18, 0x18, 0x18, 0x18, 0x01, 0x00, 0x03, 0x02, 0x3A, 0x3A, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
};


static char g_cmd_18[] = {
	0xD3,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x13, 0x13, 0x13, 0x1B, 0x2B, 0x0D, 0x0D, 0x0D, 0x21,
	0x10, 0x2A, 0x00, 0x09, 0x32, 0x10, 0x09, 0x00, 0x09, 0x32, 0x17, 0x70, 0x07, 0x70, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x0D, 0x00, 0x13, 0x00, 0xFF, 0x54, 0x01, 0x80, 0x0D, 0x00, 0x01, 0x80, 0x0D, 0x0D, 0x00, 0x01, 0x80,
	0x01, 0x80, 0x80, 0x00, 0x00, 0x38, 0x77, 0x88, 0x88,
};


static char g_cmd_19[] = {
	0xE7,
	0x00, 0x00, 0x00, 0x14, 0x14, 0x28, 0x29, 0x18, 0x58, 0x01, 0x01, 0x00, 0x00, 0x02, 0x02, 0x05, 0x10, 0x10,
	0x1D, 0xB9, 0x23, 0xB9, 0x00, 0x02, 0x07, 0xC0, 0x01, 0xC0, 0x13, 0xC0, 0x14, 0x05, 0x00, 0x00, 0xC1, 0x00,
	0x00, 0x00, 0x00, 0x02, 0x08, 0x17, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A, 0x70, 0x01,
	0x70, 0x68, 0x00, 0x47, 0x00, 0x70, 0x00, 0x00,
};

/* YYG ON */
static char g_cmd_ef[] = {
	0xEF,
	0x00, 0x40, 0x30, 0x06,
};


static char g_cmd_21[] = {
	0xF9,
	0x00, 0x5A, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x0E, 0x00, 0x00, 0x6F, 0x08, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};


static char g_cmd_22[] = {
	0xFB,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x88, 0x90, 0x00, 0x12,
};


static char g_cmd_bd01[] = {
	0xBD,
	0x01,
};

static char g_cmd_25[] = {
	0xB4,
	0x0A, 0x38, 0x03, 0x38, 0x04, 0x76, 0x04, 0x76, 0x02, 0x04, 0x13, 0x14, 0x15, 0x01, 0x01, 0x00,
};


static char g_cmd_26[] = {
	0xD3,
	0x01,
};


static char g_cmd_27[] = {
	0xDA,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0E, 0x0F, 0x00,
};

static char g_cmd_28[] = {
	0xB1,
	0x00,
};

static char g_cmd_29[] = {
	0xBF,
	0x52,
};

static char g_cmd_bd02[] = {
	0xBD,
	0x02,
};

static char g_cmd_31[] = {
	0xB1,
	0x2C,
};

static char g_cmd_32[] = {
	0xB4,
	0x9B, 0x9B, 0x00,
};

/* NOTE:PPS must set basic_para_8bpc_8bpp[4] = 1079 !! */
static char g_cmd_33[] = {
	0xF9,
	0x00, 0x00, 0x00, 0xF4, 0x6B, 0x74, 0x3B, 0x74, 0x2B, 0x34, 0x2B, 0xF6, 0x2A, 0xB6, 0x1A, 0x78,
	0x1A, 0x38, 0x1A, 0xF8, 0x19, 0xFA, 0x19, 0xFC, 0x19, 0xBE, 0x09, 0x40, 0x09, 0x00, 0x01, 0x02,
	0x01, 0x7E, 0x7D, 0x7B, 0x79, 0x77, 0x70, 0x69, 0x62, 0x54, 0x46, 0x38, 0x2A, 0x1C, 0x0E, 0x33,
	0x0B, 0x0B, 0x06, 0x00, 0x20, 0x0C, 0x03, 0xF0, 0x10, 0x00, 0x18, 0x5C, 0x06, 0xB7, 0x0D, 0x0C,
	0x00, 0x0F, 0x00, 0xF6, 0x00, 0x20, 0x00, 0x1C, 0x03, 0x00, 0x02, 0x38, 0x04, 0x38, 0x04, 0x08,
	0x00, 0x38, 0x04, 0x00, 0x0F, 0x80, 0x30, 0x89, 0x00, 0x00, 0x11,
};


static char g_cmd_bd03[] = {
	0xBD,
	0x03,
};

static char g_cmd_35[] = {
	0xB2,
	0xC0, 0x10, 0x32, 0x54, 0x76, 0x98, 0xBA, 0xDC, 0xFE, 0xFF, 0x80, 0x00, 0xFF, 0x45,
};

static char g_cmd_36[] = {
	0xF9,
	0x0C,
};

/* GAMBA+ */
static char g_cmd_bd00[] = {
	0xBD,
	0x00,
};

static char g_cmd_c1[] = {
	0xC1,
	0x01, 0x00, 0x00, 0x05, 0x0B, 0x10, 0x16, 0x1B, 0x1F, 0x23, 0x30, 0x3C, 0x4B, 0x56, 0x5E,
	0x65, 0x6C, 0x73, 0x74, 0x7B, 0x82, 0x8B, 0x94, 0x9E, 0xAB, 0xB4, 0xBE, 0xC1, 0xC4, 0xC6,
	0xCA, 0xCD, 0xD1, 0xD6, 0xD9, 0x35, 0x67, 0xF1, 0x5D, 0xC7, 0x0B, 0x04, 0xCB, 0x40,
};


static char g_cmd_c1_1[] = {
	0xC1,
	0x00, 0x00, 0x05, 0x0A, 0x0F, 0x14, 0x1A, 0x1E, 0x22, 0x2F, 0x3A, 0x49, 0x54, 0x5C, 0x64, 0x6A,
	0x71, 0x71, 0x78, 0x7F, 0x87, 0x8F, 0x98, 0xA3, 0xA9, 0xB0, 0xB2, 0xB5, 0xB7, 0xB9, 0xBC, 0xBE,
	0xC1, 0xC3, 0x36, 0xB1, 0x06, 0xB1, 0xBA, 0x04, 0xBC, 0xB7, 0x90,
};

static char g_cmd_setdgclut[] = {
	0xC1,
	0x00, 0x00, 0x05, 0x0A, 0x0F, 0x14, 0x1A, 0x1E, 0x22, 0x2E, 0x3B, 0x4A, 0x56, 0x5E, 0x65, 0x6C,
	0x73, 0x74, 0x7B, 0x83, 0x8B, 0x95, 0xA0, 0xB0, 0xBA, 0xC6, 0xCA, 0xCE, 0xD3, 0xD8, 0xDF, 0xE8,
	0xF5, 0xFF, 0x36, 0xB1, 0x27, 0x08, 0x85, 0x9C, 0x10, 0x11, 0x40,
};

/* YYG+ */
static char g_cmd_getyyg_en[] = {
	0xEF,
	0x77, 0x40, 0x30, 0x06,
};

static char g_cmd_yyg[] = {
	0xB1,
	0x0E, 0x00, 0x8C,
};

static char g_cmd_yyg2[] = {
	0xB2,
	0xF0, 0x20, 0x00, 0x58, 0x03, 0x07,
};

static char g_cmd_yyg3[] = {
	0xB3,
	0x3A, 0x38, 0xF8, 0x08, 0x20,
};

static char g_cmd_yyg4[] = {
	0xB4,
	0x04, 0x38, 0x04, 0x1E, 0x1E, 0x04, 0x04, 0x1E, 0x1E, 0x04, 0x38, 0x04,
};

static char g_cmd_yyg5[] = {
	0xB5,
	0x04, 0x1E, 0x1E, 0x04, 0x38, 0x04, 0x04, 0x1E, 0x1E,
};

static char g_cmd_yyg6[] = {
	0xBA,
	0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xF0,
};

static char g_cmd_yyg7[] = {
	0xBB,
	0xFF,  0x00,  0x00,  0x00,  0x00,  0x03,
};

static char g_cmd_yyg8[] = {
	0xBD,
	0x00,  0x10,  0x20,  0x30,  0x40,  0x50,  0x60,  0x70,  0x80,  0x90,  0xA0,  0xB0,  0xC0,  0xD0,  0xE0,  0xF0,
};

static char g_cmd_yyg9[] = {
	0xBE,
	0xFF, 0x00, 0x00, 0x00, 0x00, 0x03,
};

static char g_cmd_yyg10[] = {
	0xC0,
	0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xF0,
};

static char g_cmd_yyg11[] = {
	0xC1,
	0xFF, 0x00, 0x00, 0x00, 0x00, 0x03,
};

static char g_cmd_yyg12[] = {
	0xC3,
	0x8B, 0x59, 0xDB, 0xFA, 0xBD, 0x08, 0x3D, 0xAA, 0xD5, 0x23, 0x39, 0xA5,
};

static char g_cmd_yyg13[] = {
	0xC4,
	0x17, 0xA7, 0x6F, 0xC3, 0x1D, 0x64, 0xB6, 0x44, 0x73, 0x45, 0x3F, 0xF3,
};

static char g_cmd_yyg14[] = {
	0xC5,
	0xA1, 0x7B, 0x56, 0x9C, 0x82, 0x89, 0x54, 0x94, 0x5D, 0x22, 0xCD, 0xFE,
};

static char g_cmd_yyg15[] = {
	0xC6,
	0x79, 0x3B, 0xD3, 0x98, 0xE6, 0xE1, 0x23, 0x7F, 0x00, 0x12, 0xA9, 0xD7,
};

static char g_cmd_yyg16[] = {
	0xC7,
	0xF0, 0x06, 0xAB, 0xC4, 0xFC, 0xEE, 0x48, 0x6A, 0xC2, 0x5B, 0x3A, 0x44,
};

static char g_cmd_yyg17[] = {
	0xC8,
	0x40, 0x24, 0x13, 0x2E, 0x25, 0x77, 0x4D, 0xF1, 0xA1, 0x70, 0x0A, 0x14,
};

static char g_cmd_yyg18[] = {
	0xCF,
	0x06, 0x00, 0x01, 0x04,
};

static char g_cmd_yyg19[] = {
	0xD0,
	0x01, 0x00, 0x00, 0x03, 0x10,
};

/* YYG- */

static char g_cmd_51[] = {
	0xB2,
	0xC0, 0x10, 0x32, 0x54, 0x76, 0x98, 0xBA, 0xDC, 0xFE, 0xFF, 0x80, 0x00, 0xFF, 0x65,
};

static char g_cmd_53[] = {
	0xB4,
	0x00, 0xFF, 0x19, 0x18, 0x03, 0x38, 0x19, 0x18, 0x04, 0x76, 0x02, 0x01, 0x10, 0x14, 0x15, 0x01, 0x01, 0x01,
};


/* DEBUG PORT */
static char g_cmd_55[] = {
	0xBE,
	0x00, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
};

static char g_cmd_57[] = {
	0xBE,
	0x1F, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x02,
};

static char g_cmd_59[] = {
	0x35,
};

static char pwm_out_0x51[] = {
	0x51,
	0xFE,
};
#endif

#if BOE_INIT
static struct dsi_cmd_desc lcd_display_on_cmds[] = {
	{DTYPE_DCS_WRITE1, 0, 200, WAIT_TYPE_MS, sizeof(pwm_out_0x51), pwm_out_0x51},
	{DTYPE_DCS_WRITE, 0,200, WAIT_TYPE_MS, sizeof(g_sleep_out), g_sleep_out},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_1), g_cmd_1},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_2), g_cmd_2},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_3), g_cmd_3},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_4), g_cmd_4},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_5), g_cmd_5},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_6), g_cmd_6},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_7), g_cmd_7},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_8), g_cmd_8},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_9), g_cmd_9},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_10), g_cmd_10},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_11), g_cmd_11},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_12), g_cmd_12},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_13), g_cmd_13},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_14), g_cmd_14},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_15), g_cmd_15},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_16), g_cmd_16},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_18), g_cmd_18},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_19), g_cmd_19},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_ef), g_cmd_ef},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_21), g_cmd_21},

	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_22), g_cmd_22},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_bd01), g_cmd_bd01},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_25), g_cmd_25},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_26), g_cmd_26},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_27), g_cmd_27},

	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_28), g_cmd_28},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_29), g_cmd_29},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_bd02), g_cmd_bd02},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_31), g_cmd_31},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_32), g_cmd_32},

	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_33), g_cmd_33},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_bd03), g_cmd_bd03},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_35), g_cmd_35},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_36), g_cmd_36},

	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_bd00), g_cmd_bd00},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_c1), g_cmd_c1},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_bd01), g_cmd_bd01},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_c1_1), g_cmd_c1_1},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_bd02), g_cmd_bd02},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_setdgclut), g_cmd_setdgclut},

	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_bd00), g_cmd_bd00},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_getyyg_en), g_cmd_getyyg_en},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_yyg), g_cmd_yyg},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_yyg2), g_cmd_yyg2},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_yyg3), g_cmd_yyg3},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_yyg4), g_cmd_yyg4},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_yyg5), g_cmd_yyg5},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_yyg6), g_cmd_yyg6},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_yyg7), g_cmd_yyg7},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_yyg8), g_cmd_yyg8},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_yyg9), g_cmd_yyg9},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_yyg10), g_cmd_yyg10},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_yyg11), g_cmd_yyg11},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_yyg12), g_cmd_yyg12},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_yyg13), g_cmd_yyg13},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_yyg14), g_cmd_yyg14},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_yyg15), g_cmd_yyg15},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_yyg16), g_cmd_yyg16},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_yyg17), g_cmd_yyg17},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_yyg18), g_cmd_yyg18},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_yyg19), g_cmd_yyg19},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_ef), g_cmd_ef},

	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_bd03), g_cmd_bd03},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_51), g_cmd_51},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_bd00), g_cmd_bd00},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_53), g_cmd_53},

	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_bd00), g_cmd_bd00},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_55), g_cmd_55},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_bd01), g_cmd_bd01},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_57), g_cmd_57},
	{DTYPE_DCS_LWRITE, 0,100, WAIT_TYPE_US, sizeof(g_cmd_bd00), g_cmd_bd00},

	{DTYPE_DCS_WRITE, 0,8, WAIT_TYPE_MS, sizeof(g_cmd_59), g_cmd_59},
	{DTYPE_DCS_WRITE, 0,100, WAIT_TYPE_US, sizeof(g_display_on), g_display_on},
};
#else
static struct dsi_cmd_desc lcd_display_on_cmds[] = {
	{DTYPE_DCS_WRITE, 0,200, WAIT_TYPE_MS, sizeof(g_sleep_out), g_sleep_out},
	{DTYPE_DCS_WRITE, 0,40, WAIT_TYPE_MS, sizeof(g_display_on), g_display_on},
};
#endif

static char g_display_off[] = {
	0x28,
};
static char g_sleep_in[] = {
	0x10,
};

static struct dsi_cmd_desc lcd_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 0, 60, WAIT_TYPE_MS, sizeof(g_display_off), g_display_off},
	{DTYPE_DCS_WRITE, 0, 120, WAIT_TYPE_MS, sizeof(g_sleep_in), g_sleep_in},
};

/*******************************************************************************
** LCD VCC
*/
#define VCC_LCDIO_NAME "lcdio-vcc"

static struct regulator *vcc_lcdio;

static struct vcc_desc lcd_vcc_init_cmds[] = {
	/* vcc get */
	{DTYPE_VCC_GET, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0, WAIT_TYPE_MS, 0},

	/* io set voltage */
	{DTYPE_VCC_SET_VOLTAGE, VCC_LCDIO_NAME, &vcc_lcdio, 1850000, 1850000, WAIT_TYPE_MS, 0},
};

static struct vcc_desc lcd_vcc_finit_cmds[] = {
	/* vcc put */
	{DTYPE_VCC_PUT, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0, WAIT_TYPE_MS, 0},
};

static struct vcc_desc lcd_vcc_enable_cmds[] = {
	/* vcc enable */
	{DTYPE_VCC_ENABLE, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0, WAIT_TYPE_MS, 3},
};

static struct vcc_desc lcd_vcc_disable_cmds[] = {
	/* vcc disable */
	{DTYPE_VCC_DISABLE, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0, WAIT_TYPE_MS, 3},
};

/*******************************************************************************
** LCD IOMUX
*/
static struct pinctrl_data pctrl;

static struct pinctrl_cmd_desc lcd_pinctrl_init_cmds[] = {
	{DTYPE_PINCTRL_GET, &pctrl, 0},
	{DTYPE_PINCTRL_STATE_GET, &pctrl, DTYPE_PINCTRL_STATE_DEFAULT},
	{DTYPE_PINCTRL_STATE_GET, &pctrl, DTYPE_PINCTRL_STATE_IDLE},
};

static struct pinctrl_cmd_desc lcd_pinctrl_normal_cmds[] = {
	{DTYPE_PINCTRL_SET, &pctrl, DTYPE_PINCTRL_STATE_DEFAULT},
};

static struct pinctrl_cmd_desc lcd_pinctrl_lowpower_cmds[] = {
	{DTYPE_PINCTRL_SET, &pctrl, DTYPE_PINCTRL_STATE_IDLE},
};

static struct pinctrl_cmd_desc lcd_pinctrl_finit_cmds[] = {
	{DTYPE_PINCTRL_PUT, &pctrl, 0},
};

/*******************************************************************************
** LCD GPIO
*/
#define GPIO_LCD_P5V5_ENABLE_NAME "gpio_lcd_p5v5_enable"
#define GPIO_LCD_N5V5_ENABLE_NAME "gpio_lcd_n5v5_enable"
#define GPIO_LCD_BL_ENABLE_NAME "gpio_lcd_bl_enable"
#define GPIO_LCD_RESET_NAME "gpio_lcd_reset"
#define GPIO_LCD_TE0_NAME "gpio_lcd_te0"
static uint32_t gpio_lcd_p5v5_enable;
static uint32_t gpio_lcd_n5v5_enable;
static uint32_t gpio_lcd_bl_enable;
static uint32_t gpio_lcd_reset;
static uint32_t gpio_lcd_te0;

static struct gpio_desc asic_lcd_gpio_request_cmds[] = {
	/* AVDD_5.5V */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0, GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_p5v5_enable, 0},
	/* AVEE_-5.5V */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0, GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_n5v5_enable, 0},

	/* backlight enable */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0, GPIO_LCD_BL_ENABLE_NAME, &gpio_lcd_bl_enable, 0},
	/* reset */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0, GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},
};

static struct gpio_desc asic_lcd_gpio_normal_cmds[] = {
	/* AVDD_5.5V */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20, GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_p5v5_enable, 1},
	/* AVEE_-5.5V */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20, GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_n5v5_enable, 1},
	/* backlight enable */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5, GPIO_LCD_BL_ENABLE_NAME, &gpio_lcd_bl_enable, 1},
	/* reset */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5, GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_US, 50, GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20, GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 1},
};

static struct gpio_desc asic_lcd_gpio_lowpower_cmds[] = {
	/* backlight enable */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5, GPIO_LCD_BL_ENABLE_NAME, &gpio_lcd_bl_enable, 0},
	/* reset */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20, GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},
	/* AVEE_-5.5V */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20, GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_n5v5_enable, 0},
	/* AVDD_5.5V */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20, GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_p5v5_enable, 0},
};

static struct gpio_desc asic_lcd_gpio_free_cmds[] = {

	/* AVDD_5.5V */
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 0, GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_p5v5_enable, 0},
	/* AVEE_-5.5V */
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 0, GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_n5v5_enable, 0},

	/* backlight enable */
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 0, GPIO_LCD_BL_ENABLE_NAME, &gpio_lcd_bl_enable, 0},
	/* reset */
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 0, GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},
};

/*******************************************************************************
 */
static void panel_drv_private_data_setup(struct panel_drv_private *priv, struct device_node *np)
{
	gpio_lcd_p5v5_enable = (uint32_t)of_get_named_gpio(np, "gpios", 0);
	gpio_lcd_n5v5_enable = (uint32_t)of_get_named_gpio(np, "gpios", 1);
	gpio_lcd_reset = (uint32_t)of_get_named_gpio(np, "gpios", 2);
	gpio_lcd_bl_enable = (uint32_t)of_get_named_gpio(np, "gpios", 3);
	gpio_lcd_te0 = (uint32_t)of_get_named_gpio(np, "gpios", 4);

	dpu_pr_info("used gpio:[p5v5: %d, n5v5: %d, rst: %d, bl: %d, te0: %d]\n",
		gpio_lcd_p5v5_enable, gpio_lcd_n5v5_enable, gpio_lcd_reset, gpio_lcd_bl_enable, gpio_lcd_te0);

	priv->gpio_request_cmds = asic_lcd_gpio_request_cmds;
	priv->gpio_request_cmds_len = ARRAY_SIZE(asic_lcd_gpio_request_cmds);
	priv->gpio_free_cmds = asic_lcd_gpio_free_cmds;
	priv->gpio_free_cmds_len = ARRAY_SIZE(asic_lcd_gpio_free_cmds);

	priv->gpio_normal_cmds = asic_lcd_gpio_normal_cmds;
	priv->gpio_normal_cmds_len = ARRAY_SIZE(asic_lcd_gpio_normal_cmds);
	priv->gpio_lowpower_cmds = asic_lcd_gpio_lowpower_cmds;
	priv->gpio_lowpower_cmds_len = ARRAY_SIZE(asic_lcd_gpio_lowpower_cmds);

	priv->vcc_init_cmds = lcd_vcc_init_cmds;
	priv->vcc_init_cmds_len = ARRAY_SIZE(lcd_vcc_init_cmds);
	priv->vcc_finit_cmds = lcd_vcc_finit_cmds;
	priv->vcc_finit_cmds_len = ARRAY_SIZE(lcd_vcc_finit_cmds);

	priv->vcc_enable_cmds = lcd_vcc_enable_cmds;
	priv->vcc_enable_cmds_len = ARRAY_SIZE(lcd_vcc_enable_cmds);
	priv->vcc_disable_cmds = lcd_vcc_disable_cmds;
	priv->vcc_disable_cmds_len = ARRAY_SIZE(lcd_vcc_disable_cmds);

	priv->pinctrl_init_cmds = lcd_pinctrl_init_cmds;
	priv->pinctrl_init_cmds_len = ARRAY_SIZE(lcd_pinctrl_init_cmds);
	priv->pinctrl_finit_cmds = lcd_pinctrl_finit_cmds;
	priv->pinctrl_finit_cmds_len = ARRAY_SIZE(lcd_pinctrl_finit_cmds);

	priv->pinctrl_normal_cmds = lcd_pinctrl_normal_cmds;
	priv->pinctrl_normal_cmds_len = ARRAY_SIZE(lcd_pinctrl_normal_cmds);
	priv->pinctrl_lowpower_cmds = lcd_pinctrl_lowpower_cmds;
	priv->pinctrl_lowpower_cmds_len = ARRAY_SIZE(lcd_pinctrl_lowpower_cmds);
}

/*******************************************************************************
*/
static void dsc_config_initial(struct dkmd_connector_info *pinfo, struct dsc_calc_info *dsc)
{
	dpu_pr_info("+\n");

	pinfo->ifbc_type = IFBC_TYPE_VESA3X_DUAL;
	dsc->dsc_en = 1;

	dsc->dsc_info.dsc_version_major = 1;
	dsc->dsc_info.dsc_version_minor = 1;
	dsc->dsc_info.pic_height = pinfo->base.yres;
	dsc->dsc_info.pic_width = pinfo->base.xres;
	dsc->dsc_info.slice_height = 7 + 1;
	dsc->dsc_info.slice_width = 1079 + 1;
	dsc->dsc_info.chunk_size = 0x438;
	dsc->dsc_info.initial_dec_delay = 0x031c;
	dsc->dsc_info.initial_scale_value = 0x20;
	dsc->dsc_info.scale_increment_interval = 0xf6;
	dsc->dsc_info.scale_decrement_interval = 15;
	dsc->dsc_info.nfl_bpg_offset = 0x0db7;
	dsc->dsc_info.slice_bpg_offset = 0x065c;
	dsc->dsc_info.final_offset = 0x10f0;

	// dsc parameter info
	dsc->dsc_info.dsc_bpc = 8;
	dsc->dsc_info.dsc_bpp = 8;
	dsc->dsc_info.initial_xmit_delay = 512;
	dsc->dsc_info.first_line_bpg_offset = 12;

	// DSC_CTRL
	dsc->dsc_info.block_pred_enable = 1;
	dsc->dsc_info.linebuf_depth = 9;
	// RC_PARAM3
	dsc->dsc_info.initial_offset = 6144;
	// FLATNESS_QP_TH
	dsc->dsc_info.flatness_min_qp = 3;
	dsc->dsc_info.flatness_max_qp = 12;
	// DSC_PARAM4
	dsc->dsc_info.rc_edge_factor = 0x6;
	dsc->dsc_info.rc_model_size = 8192;
	// DSC_RC_PARAM5: 0x330b0b
	dsc->dsc_info.rc_tgt_offset_lo = (0x330b0b >> 20) & 0xF;
	dsc->dsc_info.rc_tgt_offset_hi = (0x330b0b >> 16) & 0xF;
	dsc->dsc_info.rc_quant_incr_limit1 = (0x330b0b >> 8) & 0x1F;
	dsc->dsc_info.rc_quant_incr_limit0 = (0x330b0b >> 0) & 0x1F;
	// DSC_RC_BUF_THRESH0: 0xe1c2a38
	dsc->dsc_info.rc_buf_thresh[0] = (0xe1c2a38 >> 24) & 0xFF;
	dsc->dsc_info.rc_buf_thresh[1] = (0xe1c2a38 >> 16) & 0xFF;
	dsc->dsc_info.rc_buf_thresh[2] = (0xe1c2a38 >> 8) & 0xFF;
	dsc->dsc_info.rc_buf_thresh[3] = (0xe1c2a38 >> 0) & 0xFF;
	// DSC_RC_BUF_THRESH1: 0x46546269
	dsc->dsc_info.rc_buf_thresh[4] = (0x46546269 >> 24) & 0xFF;
	dsc->dsc_info.rc_buf_thresh[5] = (0x46546269 >> 16) & 0xFF;
	dsc->dsc_info.rc_buf_thresh[6] = (0x46546269 >> 8) & 0xFF;
	dsc->dsc_info.rc_buf_thresh[7] = (0x46546269 >> 0) & 0xFF;
	// DSC_RC_BUF_THRESH2: 0x7077797b
	dsc->dsc_info.rc_buf_thresh[8] = (0x7077797b >> 24) & 0xFF;
	dsc->dsc_info.rc_buf_thresh[9] = (0x7077797b >> 16) & 0xFF;
	dsc->dsc_info.rc_buf_thresh[10] = (0x7077797b >> 8) & 0xFF;
	dsc->dsc_info.rc_buf_thresh[11] = (0x7077797b >> 0) & 0xFF;
	// DSC_RC_BUF_THRESH3: 0x7d7e0000
	dsc->dsc_info.rc_buf_thresh[12] = (0x7d7e0000 >> 24) & 0xFF;
	dsc->dsc_info.rc_buf_thresh[13] = (0x7d7e0000 >> 16) & 0xFF;
	// DSC_RC_RANGE_PARAM0: 0x1020100
	dsc->dsc_info.rc_range[0].min_qp = (0x1020100 >> 27) & 0x1F;
	dsc->dsc_info.rc_range[0].max_qp = (0x1020100 >> 22) & 0x1F;
	dsc->dsc_info.rc_range[0].offset = (0x1020100 >> 16) & 0x3F;
	dsc->dsc_info.rc_range[1].min_qp = (0x1020100 >> 11) & 0x1F;
	dsc->dsc_info.rc_range[1].max_qp = (0x1020100 >> 6) & 0x1F;
	dsc->dsc_info.rc_range[1].offset = (0x1020100 >> 0) & 0x3F;
	// DSC_RC_RANGE_PARAM1: 0x94009be
	dsc->dsc_info.rc_range[2].min_qp = (0x94009be >> 27) & 0x1F;
	dsc->dsc_info.rc_range[2].max_qp = (0x94009be >> 22) & 0x1F;
	dsc->dsc_info.rc_range[2].offset = (0x94009be >> 16) & 0x3F;
	dsc->dsc_info.rc_range[3].min_qp = (0x94009be >> 11) & 0x1F;
	dsc->dsc_info.rc_range[3].max_qp = (0x94009be >> 6) & 0x1F;
	dsc->dsc_info.rc_range[3].offset = (0x94009be >> 0) & 0x3F;
	// DSC_RC_RANGE_PARAM2, 0x19fc19fa
	dsc->dsc_info.rc_range[4].min_qp = (0x19fc19fa >> 27) & 0x1F;
	dsc->dsc_info.rc_range[4].max_qp = (0x19fc19fa >> 22) & 0x1F;
	dsc->dsc_info.rc_range[4].offset = (0x19fc19fa >> 16) & 0x3F;
	dsc->dsc_info.rc_range[5].min_qp = (0x19fc19fa >> 11) & 0x1F;
	dsc->dsc_info.rc_range[5].max_qp = (0x19fc19fa >> 6) & 0x1F;
	dsc->dsc_info.rc_range[5].offset = (0x19fc19fa >> 0) & 0x3F;
	// DSC_RC_RANGE_PARAM3, 0x19f81a38
	dsc->dsc_info.rc_range[6].min_qp = (0x19f81a38 >> 27) & 0x1F;
	dsc->dsc_info.rc_range[6].max_qp = (0x19f81a38 >> 22) & 0x1F;
	dsc->dsc_info.rc_range[6].offset = (0x19f81a38 >> 16) & 0x3F;
	dsc->dsc_info.rc_range[7].min_qp = (0x19f81a38 >> 11) & 0x1F;
	dsc->dsc_info.rc_range[7].max_qp = (0x19f81a38 >> 6) & 0x1F;
	dsc->dsc_info.rc_range[7].offset = (0x19f81a38 >> 0) & 0x3F;
	// DSC_RC_RANGE_PARAM4, 0x1a781ab6
	dsc->dsc_info.rc_range[8].min_qp = (0x1a781ab6 >> 27) & 0x1F;
	dsc->dsc_info.rc_range[8].max_qp = (0x1a781ab6 >> 22) & 0x1F;
	dsc->dsc_info.rc_range[8].offset = (0x1a781ab6 >> 16) & 0x3F;
	dsc->dsc_info.rc_range[9].min_qp = (0x1a781ab6 >> 11) & 0x1F;
	dsc->dsc_info.rc_range[9].max_qp = (0x1a781ab6 >> 6) & 0x1F;
	dsc->dsc_info.rc_range[9].offset = (0x1a781ab6 >> 0) & 0x3F;
	// DSC_RC_RANGE_PARAM5, 0x2af62b34
	dsc->dsc_info.rc_range[10].min_qp = (0x2af62b34 >> 27) & 0x1F;
	dsc->dsc_info.rc_range[10].max_qp = (0x2af62b34 >> 22) & 0x1F;
	dsc->dsc_info.rc_range[10].offset = (0x2af62b34 >> 16) & 0x3F;
	dsc->dsc_info.rc_range[11].min_qp = (0x2af62b34 >> 11) & 0x1F;
	dsc->dsc_info.rc_range[11].max_qp = (0x2af62b34 >> 6) & 0x1F;
	dsc->dsc_info.rc_range[11].offset = (0x2af62b34 >> 0) & 0x3F;
	// DSC_RC_RANGE_PARAM6, 0x2b743b74
	dsc->dsc_info.rc_range[12].min_qp = (0x2b743b74 >> 27) & 0x1F;
	dsc->dsc_info.rc_range[12].max_qp = (0x2b743b74 >> 22) & 0x1F;
	dsc->dsc_info.rc_range[12].offset = (0x2b743b74 >> 16) & 0x3F;
	dsc->dsc_info.rc_range[13].min_qp = (0x2b743b74 >> 11) & 0x1F;
	dsc->dsc_info.rc_range[13].max_qp = (0x2b743b74 >> 6) & 0x1F;
	dsc->dsc_info.rc_range[13].offset = (0x2b743b74 >> 0) & 0x3F;
	// DSC_RC_RANGE_PARAM7, 0x6bf40000
	dsc->dsc_info.rc_range[14].min_qp = (0x6bf40000 >> 27) & 0x1F;
	dsc->dsc_info.rc_range[14].max_qp = (0x6bf40000 >> 22) & 0x1F;
	dsc->dsc_info.rc_range[14].offset = (0x6bf40000 >> 16) & 0x3F;

	dpu_pr_info("-\n");
}

/* dsi param initialized value from panel spec */
static void mipi_lcd_init_dsi_param(struct dkmd_connector_info *pinfo, struct mipi_panel_info *mipi)
{
	mipi->hsa = 21;
	mipi->hbp = 42;
	mipi->dpi_hsize = 375;
	mipi->hline_time = 500;
	mipi->vsa = 10;
	mipi->vbp = 54;
	mipi->vfp = 10;

	mipi->pxl_clk_rate = 720 * 1000000UL;
	mipi->dsi_bit_clk = 500;

	mipi->dsi_bit_clk_upt = mipi->dsi_bit_clk;
	mipi->dsi_bit_clk_default = mipi->dsi_bit_clk;

	mipi->pxl_clk_rate_div = 6;
	mipi->dsi_bit_clk_upt_support = 0;

	mipi->clk_post_adjust = 100;
	mipi->lane_nums = DSI_4_LANES;
	mipi->color_mode = DSI_24BITS_1;

	mipi->dsi_version = DSI_1_1_VERSION;
	mipi->phy_mode = DPHY_MODE;

	mipi->vc = 0;
	mipi->max_tx_esc_clk = 10 * 1000000;
	mipi->burst_mode = DSI_BURST_SYNC_PULSES_1;
	mipi->non_continue_en = 1;
}

/* dirty region initialized value from panel spec */
static void lcd_init_dirty_region(struct panel_drv_private *priv)
{
	priv->dirty_region_info.left_align = -1;
	priv->dirty_region_info.right_align = -1;
	priv->dirty_region_info.top_align = 32;
	priv->dirty_region_info.bottom_align = 32;
	priv->dirty_region_info.w_align = -1;
	priv->dirty_region_info.h_align = -1;
	priv->dirty_region_info.w_min = 2160;
	priv->dirty_region_info.h_min = -1;
	priv->dirty_region_info.top_start = -1;
	priv->dirty_region_info.bottom_start = -1;
}

static int32_t panel_of_device_setup(struct panel_drv_private *priv)
{
	int32_t ret;
	struct dkmd_connector_info *pinfo = &priv->connector_info;
	struct device_node *np = priv->pdev->dev.of_node;

	dpu_pr_info("enter!\n");

	/* Inheritance based processing */
	panel_base_of_device_setup(priv);
	panel_drv_private_data_setup(priv, np);

	/* 1. config base object info
	 * would be used for framebuffer setup
	 */
	pinfo->base.xres = 2160; // FIXME: Modified for new panel device
	pinfo->base.yres = 3840; // FIXME: Modified for new panel device

	/* When calculating DPI needs the following parameters */
	pinfo->base.width = 68; // FIXME: Modified for new panel device
	pinfo->base.height = 121; // FIXME: Modified for new panel device

	// TODO: caculate fps by mipi timing para
	pinfo->base.fps = 60;

	pinfo->ifbc_type = IFBC_TYPE_VESA3X_DUAL;

	/* 2. config connector info
	 * would be used for dsi & composer setup
	 */
	mipi_lcd_init_dsi_param(pinfo, &get_primary_connector(pinfo)->mipi);

	dsc_config_initial(pinfo, &get_primary_connector(pinfo)->dsc);

	/* dsi or composer need this param */
	pinfo->dirty_region_updt_support = 0;

	/* 3. config panel private info
	 * would be used for panel setup
	 */
	priv->bl_info.bl_min = 12;
	priv->bl_info.bl_max = 2047;
	priv->bl_info.bl_default = 1000;

	lcd_init_dirty_region(priv);

	priv->disp_on_cmds = lcd_display_on_cmds;
	priv->disp_on_cmds_len = ARRAY_SIZE(lcd_display_on_cmds);
	priv->disp_off_cmds = lcd_display_off_cmds;
	priv->disp_off_cmds_len = (uint32_t)ARRAY_SIZE(lcd_display_off_cmds);

	if (pinfo->base.fpga_flag == 0) {
		ret = peri_vcc_cmds_tx(priv->pdev, priv->vcc_init_cmds, priv->vcc_init_cmds_len);
		if (ret != 0)
			dpu_pr_info("vcc init failed!\n");

		ret = peri_pinctrl_cmds_tx(priv->pdev, priv->pinctrl_init_cmds, priv->pinctrl_init_cmds_len);
		if (ret != 0)
			dpu_pr_info("pinctrl init failed\n");

		ret = peri_vcc_cmds_tx(priv->pdev, priv->vcc_enable_cmds, priv->vcc_enable_cmds_len);
		if (ret)
			dpu_pr_warn("vcc enable cmds handle fail!\n");
	}

	dpu_pr_info("exit!\n");

	return 0;
}

static void panel_of_device_release(struct panel_drv_private *priv)
{
	int32_t ret = 0;

	panel_base_of_device_release(priv);

	if (priv->gpio_free_cmds && (priv->gpio_free_cmds_len > 0)) {
		ret = peri_gpio_cmds_tx(priv->gpio_free_cmds, priv->gpio_free_cmds_len);
		if (ret)
			dpu_pr_info("gpio free handle err!\n");
	}

	dpu_pr_info("exit!\n");
}

panel_device_match_data(hx5293_panel_info, PANEL_HX5293_ID, panel_of_device_setup, panel_of_device_release);

MODULE_LICENSE("GPL");
