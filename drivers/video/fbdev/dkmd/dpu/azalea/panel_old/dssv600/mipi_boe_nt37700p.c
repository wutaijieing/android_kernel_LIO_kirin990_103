/* Copyright (c) 2019-2019, Hisilicon Tech. Co., Ltd. All rights reserved.
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

#include "dpu_fb.h"
#include <linux/types.h>

#include "../../panel/mipi_lcd_utils.h"
#include "../../spr/dpu_spr.h"
#include "../../dsc/dsc_algorithm_manager.h"
#define DTS_COMP_BOE_NT37700P "hisilicon,mipi_boe_nt37700p"
static int g_lcd_fpga_flag;

/* 8bit or 10bit out */
#define SPR_RGB2UYUV_8BITEN

/* need close AP side SPR dsc1.2 */
#define DSC_1_2_DECOMPRESS

/* lint -save -e569, -e574, -e527, -e572 */
/*
 * Power ON/OFF Sequence(sleep mode to Normal mode) begin
 */
static char g_display_off[] = {
	0x28,
};
static char g_enter_sleep[] = {
	0x10,
};

static char g_sleep_out[] = {
	0x11,
	0x00,
};

static char g_display_on[] = {
	0x29,
	0x00,
};

static char g_dimming_off[] = {
	0x53,
	0x28,
};

static char g_enter_nor1[] = {
	0x51,
	0x06, 0x46,
};

static char g_reg_35[] = {
	0x35,
	0x00,
};

static char g_panel_porch[] = {
	0x3B,
	0x00, 0x08, 0x00, 0x08,
};

static char g_compress_mode[] = {
	0x90,
	0x02,
};

static char g_xderction[] = {
	0x2A,
	0x00, 0x00, 0x04, 0x37,
};

static char g_yderction[] = {
	0x2B,
	0x00, 0x00, 0x09, 0x23,
};

#ifdef BIST_MODE
static char g_bist_mode_4[] = {
	0x28,
};

static char g_bist_mode_0[] = {
	0x10,
};
static char g_bist_mode_1[] = {
	0xF0,
	0x55, 0xAA, 0x52, 0x08, 0x00,
};
static char g_bist_mode_2[] = {
	0xEF,
	0x01, 0x00, 0xFF, 0xFF, 0xFF, 0x16, 0xFF,
};
static char g_bist_mode_3[] = {
	0xEE,
	0x87, 0x78, 0x02, 0x40,
};
static char g_em_adjust[] = {
	0xB4,
	0x00, 0x9C,
};
#endif

static char g_bl_level[] = {
	0x51,
	0x7, 0xff,
};
static char g_sram_sync[] = {
	0x2C,
	0x00,
};

static char g_page0[] = {
	0xF0,
	0x55, 0xAA, 0x52, 0x08, 0x00,
};

/* IC SPR OFF, DSC1.2 8bit ON */
/* 0x90 reg :0x01 dsc1.2;0x02 no compress */
#ifdef DSC_1_2_DECOMPRESS
static char g_vesa_mode_on[] = {
	0x90,
	0x01,
};
#else
static char g_vesa_mode_on[] = {
	0x90,
	0x02,
};
#endif

/* PS_MODE = 4 for 1.2 YUV 8bit case */
static char g_ps_mode[] = {
	0xDD,
	0x04,
};

/* RXIP enable */
static char g_page3[] = {
	0xF0,
	0x55, 0xAA, 0x52, 0x08, 0x03,
};

#ifdef SPR_RGB2UYUV_8BITEN
static char g_rxip_enable_2[] = {
	0xDE,
	0x43, 0x3c, 0x06, 0xe4, 0xc3, 0x3d, 0x1e, 0x64, 0x80,
};
static char g_vesa_setting_4[] = {
	0x91,
	0x80, 0xEC, 0x00, 0x3C, 0xF2, 0x00, 0x01, 0x8D, 0x09, 0xB1, 0x00, 0x07,
	0x02, 0x09, 0x01, 0xBB, 0x09, 0x20,
};
#else
static char g_rxip_enable_2[] = {
	0xDE,
	0x42, 0x3c, 0x06, 0xe4, 0xc3, 0x3d, 0x1e, 0x64, 0x80,
};
static char g_vesa_setting_4[] = {
	0x91,
	0xA0, 0xF0, 0x00, 0x3C, 0xF1, 0x55, 0x02, 0x28, 0x04, 0xAE, 0x00, 0x5A,
	0x02, 0x09, 0x04, 0xCD, 0x0C, 0x00,
};
#endif

static char g_vesa_setting_1[] = {
	0x03,
	0x01,
};
static char g_vesa_setting_2[] = {
	0x2B,
	0x00, 0x00, 0x09, 0x23,
};
static char g_vesa_setting_3[] = {
	0x90,
	0x01,
};


/* SPR function control */
static char g_page7[] = {
	0xF0,
	0x55, 0xAA, 0x52, 0x08, 0x07,
};

static char g_spr_func_ctrl_1[] = {
	0xB0,
	/* SPR off 0x00, 0x00, 0x70, 0x60, 0x00, 0x00, */
	0xA0, 0x00, 0x70, 0x60, 0x00, 0x00,
};
static char g_spr_func_ctrl_2[] = {
	0xB1,
	/* SPR off 0x00, 0x10, 0x00, 0x10, 0x00, 0x00, */
	0x0C, 0x2C, 0x00, 0x2C, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static char g_spr_func_ctrl_3[] = {
	0xB2,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x10,
	0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static char g_spr_func_ctrl_4[] = {
	0xB4,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x60, 0x80, 0x60, 0x80, 0x60, 0x80,
	0x40, 0x80,
};

#ifdef CONFIG_DPU_FB_V510
static u32 g_spr_gama_degama_table[] = {
	/* gama_r */
	0, 288, 541, 730, 887, 1024, 1146, 1257, 1360, 1456, 1547, 1632, 1713,
	1791, 1866, 1937, 2006, 2073, 2138, 2200, 2261, 2320, 2378, 2434, 2489,
	2543, 2595, 2647, 2697, 2746, 2795, 2842, 2889, 2935, 2980, 3025, 3068,
	3112, 3154, 3196, 3237, 3278, 3318, 3358, 3397, 3435, 3474, 3511, 3549,
	3585, 3622, 3658, 3694, 3729, 3764, 3798, 3833, 3866, 3900, 3933, 3966,
	3999, 4031, 4063, 4095,
	/* gama_g */
	0, 288, 541, 730, 887, 1024, 1146, 1257, 1360, 1456, 1547, 1632, 1713,
	1791, 1866, 1937, 2006, 2073, 2138, 2200, 2261, 2320, 2378, 2434, 2489,
	2543, 2595, 2647, 2697, 2746, 2795, 2842, 2889, 2935, 2980, 3025, 3068,
	3112, 3154, 3196, 3237, 3278, 3318, 3358, 3397, 3435, 3474, 3511, 3549,
	3585, 3622, 3658, 3694, 3729, 3764, 3798, 3833, 3866, 3900, 3933, 3966,
	3999, 4031, 4063, 4095,
	/* gama_b */
	0, 288, 541, 730, 887, 1024, 1146, 1257, 1360, 1456, 1547, 1632, 1713,
	1791, 1866, 1937, 2006, 2073, 2138, 2200, 2261, 2320, 2378, 2434, 2489,
	2543, 2595, 2647, 2697, 2746, 2795, 2842, 2889, 2935, 2980, 3025, 3068,
	3112, 3154, 3196, 3237, 3278, 3318, 3358, 3397, 3435, 3474, 3511, 3549,
	3585, 3622, 3658, 3694, 3729, 3764, 3798, 3833, 3866, 3900, 3933, 3966,
	3999, 4031, 4063, 4095,

	/* degama_r */
	0, 14, 28, 43, 57, 71, 86, 102, 119, 139, 160, 182, 206, 232, 260, 289,
	320, 353, 387, 423, 462, 501, 543, 587, 632, 679, 728, 779, 832, 887,
	944, 1002, 1063, 1126, 1190, 1257, 1325, 1396, 1468, 1543, 1620, 1698,
	1779, 1862, 1947, 2034, 2123, 2214, 2308, 2403, 2501, 2600, 2702, 2806,
	2912, 3021, 3131, 3244, 3359, 3476, 3595, 3717, 3841, 3967, 4095,
	/* degama_g */
	0, 14, 28, 43, 57, 71, 86, 102, 119, 139, 160, 182, 206, 232, 260, 289,
	320, 353, 387, 423, 462, 501, 543, 587, 632, 679, 728, 779, 832, 887,
	944, 1002, 1063, 1126, 1190, 1257, 1325, 1396, 1468, 1543, 1620, 1698,
	1779, 1862, 1947, 2034, 2123, 2214, 2308, 2403, 2501, 2600, 2702, 2806,
	2912, 3021, 3131, 3244, 3359, 3476, 3595, 3717, 3841, 3967, 4095,
	/* degama_b */
	0, 14, 28, 43, 57, 71, 86, 102, 119, 139, 160, 182, 206, 232, 260, 289,
	320, 353, 387, 423, 462, 501, 543, 587, 632, 679, 728, 779, 832, 887,
	944, 1002, 1063, 1126, 1190, 1257, 1325, 1396, 1468, 1543, 1620, 1698,
	1779, 1862, 1947, 2034, 2123, 2214, 2308, 2403, 2501, 2600, 2702, 2806,
	2912, 3021, 3131, 3244, 3359, 3476, 3595, 3717, 3841, 3967, 4095,
};
#else
static u32 g_spr_gama_degama_table[] = {
	/* gamma */
	0, 288, 541, 730, 887, 1024, 1146, 1257, 1360, 1456, 1547, 1632, 1713,
		1791, 1866, 1937, 2006, 2073, 2138, 2200, 2261, 2320, 2378, 2434, 2489,
		2543, 2595, 2647, 2697, 2746, 2795, 2842, 2889, 2935, 2980, 3025, 3068,
		3112, 3154, 3196, 3237, 3278, 3318, 3358, 3397, 3435, 3474, 3511, 3549,
		3585, 3622, 3658, 3694, 3729, 3764, 3798, 3833, 3866, 3900, 3933, 3966,
		3999, 4031, 4063, 4095, 4095,
	/* degamma */
	0, 14, 28, 43, 57, 71, 86, 102, 119, 139, 160, 182, 206, 232, 260, 289,
		320, 353, 387, 423, 462, 501, 543, 587, 632, 679, 728, 779, 832, 887,
		944, 1002, 1063, 1126, 1190, 1257, 1325, 1396, 1468, 1543, 1620, 1698,
		1779, 1862, 1947, 2034, 2123, 2214, 2308, 2403, 2501, 2600, 2702, 2806,
		2912, 3021, 3131, 3244, 3359, 3476, 3595, 3717, 3841, 3967, 4095, 4095,
};
#endif

static struct dsi_cmd_desc g_display_on_cmds[] = {
	{ DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(g_dimming_off), g_dimming_off },
	{ DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(g_enter_nor1), g_enter_nor1 },
	{ DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(g_reg_35), g_reg_35 },
	{ DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(g_panel_porch), g_panel_porch },
	{ DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(g_compress_mode), g_compress_mode },
	{ DTYPE_DCS_LWRITE, 0, 20, WAIT_TYPE_US,
		sizeof(g_xderction), g_xderction },
	{ DTYPE_DCS_LWRITE, 0, 20, WAIT_TYPE_US,
		sizeof(g_yderction), g_yderction },
	/* ic spr off, dsc1.2 enable */
	{ DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(g_vesa_mode_on), g_vesa_mode_on },
	{ DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(g_page0), g_page0 },
	{ DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(g_ps_mode), g_ps_mode },

#ifdef DSC_1_2_DECOMPRESS
	{ DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(g_page3), g_page3 },
	{ DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(g_rxip_enable_2), g_rxip_enable_2 },
	{ DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(g_vesa_setting_1), g_vesa_setting_1 },
	{ DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(g_vesa_setting_2), g_vesa_setting_2 },
	{ DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(g_vesa_setting_3), g_vesa_setting_3 },
	{ DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(g_vesa_setting_4), g_vesa_setting_4 },
#endif
	{ DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(g_page7), g_page7 },
	{ DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(g_spr_func_ctrl_1), g_spr_func_ctrl_1 },
	{ DTYPE_DCS_LWRITE, 0, 20, WAIT_TYPE_US,
		sizeof(g_spr_func_ctrl_2), g_spr_func_ctrl_2 },
	{ DTYPE_DCS_LWRITE, 0, 20, WAIT_TYPE_US,
		sizeof(g_spr_func_ctrl_3), g_spr_func_ctrl_3 },
	{ DTYPE_DCS_LWRITE, 0, 20, WAIT_TYPE_US,
		sizeof(g_spr_func_ctrl_4), g_spr_func_ctrl_4 },
	/* end */

#ifdef BIST_MODE
	{ DTYPE_DCS_LWRITE, 0, 20, WAIT_TYPE_US,
		sizeof(g_bist_mode_1), g_bist_mode_1 },
	{ DTYPE_DCS_LWRITE, 0, 20, WAIT_TYPE_US,
		sizeof(g_bist_mode_2), g_bist_mode_2 },
	{ DTYPE_DCS_LWRITE, 0, 20, WAIT_TYPE_US,
		sizeof(g_bist_mode_3), g_bist_mode_3 },
#else
	{ DTYPE_DCS_WRITE, 0, 120, WAIT_TYPE_MS,
		sizeof(g_sleep_out), g_sleep_out },
	{ DTYPE_DCS_WRITE, 0, 120, WAIT_TYPE_MS,
		sizeof(g_display_on), g_display_on },
	{ DTYPE_DCS_LWRITE, 0, 20, WAIT_TYPE_US,
		sizeof(g_bl_level), g_bl_level },
	{ DTYPE_DCS_WRITE1, 0, 20, WAIT_TYPE_US,
		sizeof(g_sram_sync), g_sram_sync },
#endif
};

static struct dsi_cmd_desc g_lcd_display_off_cmd[] = {
	{ DTYPE_DCS_WRITE, 0, 1, WAIT_TYPE_MS,
		sizeof(g_display_off), g_display_off },
	{ DTYPE_DCS_WRITE, 0, 120, WAIT_TYPE_MS,
		sizeof(g_enter_sleep), g_enter_sleep },
};

/*******************************************************************************
 ** LCD GPIO
 */
#define GPIO_AMOLED_RESET_NAME	"gpio_amoled_reset"
#define GPIO_AMOLED_VCC1V8_NAME	"gpio_amoled_vcc1v8"
#define GPIO_AMOLED_VCC3V1_NAME	"gpio_amoled_vcc3v1"
#define GPIO_AMOLED_TE0_NAME	"gpio_amoled_te0"

static uint32_t g_gpio_amoled_te0;
static uint32_t g_gpio_amoled_vcc1v8;
static uint32_t g_gpio_amoled_reset;
static uint32_t g_gpio_amoled_vcc3v1;
/*******************************************************************************
 * LCD VCC
 */
#define VCC_LCDIO_NAME	"lcdio-vcc"
#define VCC_LCDANALOG_NAME	"lcdanalog-vcc"

static struct regulator *g_vcc_lcdio;

#ifdef CONFIG_DPU_FB_V510
static struct regulator *g_vcc_lcdanalog;
#endif

static struct gpio_desc g_fpga_lcd_gpio_request_cmds[] = {
	/* vcc3v1 */
	{ DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_AMOLED_VCC3V1_NAME, &g_gpio_amoled_vcc3v1, 0 },
	/* vcc1v8 */
	{ DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_AMOLED_VCC1V8_NAME, &g_gpio_amoled_vcc1v8, 0 },

	/* reset */
	{ DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 0 },
};

static struct gpio_desc g_fpga_lcd_gpio_normal_cmds[] = {
	/* vcc1v8 enable */
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 2,
		GPIO_AMOLED_VCC1V8_NAME, &g_gpio_amoled_vcc1v8, 1 },
	/* vcc3v1 enable */
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 15,
		GPIO_AMOLED_VCC3V1_NAME, &g_gpio_amoled_vcc3v1, 1 },
	/* reset */
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 1 },
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_US, 50,
		GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 0 },
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 15,
		GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 1 },
	/* backlight enable */
};

static struct vcc_desc g_lcd_vcc_init_cmds[] = {
	/* vcc get */
	{ DTYPE_VCC_GET, VCC_LCDIO_NAME,
		&g_vcc_lcdio, 0, 0, WAIT_TYPE_MS, 0 },
#ifdef CONFIG_DPU_FB_V510
	{ DTYPE_VCC_GET, VCC_LCDANALOG_NAME,
		&g_vcc_lcdanalog, 0, 0, WAIT_TYPE_MS, 0 },

	/* vcc set voltage */
	{ DTYPE_VCC_SET_VOLTAGE, VCC_LCDANALOG_NAME,
		&g_vcc_lcdanalog, 3100000, 3100000, WAIT_TYPE_MS, 0 },
#endif
	/* io set voltage */
	{ DTYPE_VCC_SET_VOLTAGE, VCC_LCDIO_NAME,
		&g_vcc_lcdio, 1800000, 1800000, WAIT_TYPE_MS, 0 },
};

static struct vcc_desc g_lcd_vcc_finit_cmds[] = {
	/* vcc put */
	{ DTYPE_VCC_PUT, VCC_LCDIO_NAME,
		&g_vcc_lcdio, 0, 0, WAIT_TYPE_MS, 0 },
#ifdef CONFIG_DPU_FB_V510
	{ DTYPE_VCC_PUT, VCC_LCDANALOG_NAME,
		&g_vcc_lcdanalog, 0, 0, WAIT_TYPE_MS, 0 },
#endif
};

static struct vcc_desc g_lcd_vcc_enable_cmds[] = {
	/* vcc enable */
#ifdef CONFIG_DPU_FB_V510
	{ DTYPE_VCC_ENABLE, VCC_LCDANALOG_NAME,
		&g_vcc_lcdanalog, 0, 0, WAIT_TYPE_MS, 3 },
#endif
	{ DTYPE_VCC_ENABLE, VCC_LCDIO_NAME,
		&g_vcc_lcdio, 0, 0, WAIT_TYPE_MS, 3 },
};

/*******************************************************************************
 ** LCD IOMUX
 */
static struct pinctrl_data g_pctrl;

static struct pinctrl_cmd_desc g_lcd_pinctrl_init_cmds[] = {
	{ DTYPE_PINCTRL_GET, &g_pctrl, 0 },
	{ DTYPE_PINCTRL_STATE_GET, &g_pctrl, DTYPE_PINCTRL_STATE_DEFAULT },
	{ DTYPE_PINCTRL_STATE_GET, &g_pctrl, DTYPE_PINCTRL_STATE_IDLE },
};

static struct pinctrl_cmd_desc g_lcd_pinctrl_normal_cmds[] = {
	{ DTYPE_PINCTRL_SET, &g_pctrl, DTYPE_PINCTRL_STATE_DEFAULT },
};

static struct pinctrl_cmd_desc g_lcd_pinctrl_finit_cmds[] = {
	{ DTYPE_PINCTRL_PUT, &g_pctrl, 0 },
};

/*******************************************************************************
 */

static struct gpio_desc g_asic_lcd_gpio_request_cmds[] = {
	{ DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_AMOLED_VCC3V1_NAME, &g_gpio_amoled_vcc3v1, 0 },
	{ DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 0 },
};

static struct gpio_desc g_asic_lcd_gpio_normal_cmds[] = {
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20,
		GPIO_AMOLED_VCC3V1_NAME, &g_gpio_amoled_vcc3v1, 1 },
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_US, 50,
		GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 1 },
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_US, 50,
		GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 0 },
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 15,
		GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 1 },
};

static int mipi_panel_set_fastboot(struct platform_device *pdev)
{
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL)
		return -EINVAL;

	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL)
		return -EINVAL;

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);

	if (g_lcd_fpga_flag == 0) {
		/* lcd pinctrl normal */
		pinctrl_cmds_tx(pdev, g_lcd_pinctrl_normal_cmds,
			ARRAY_SIZE(g_lcd_pinctrl_normal_cmds));
		/* lcd gpio request */
		gpio_cmds_tx(g_asic_lcd_gpio_request_cmds,
			ARRAY_SIZE(g_asic_lcd_gpio_request_cmds));
	}

	/* backlight on */
	dpu_lcd_backlight_on(pdev);

	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return 0;
}

static uint32_t mipi_read_ic_regs(char __iomem *mipi_dsi0_base, uint32_t rd_cmd)
{
	uint32_t status;
	uint32_t try_times = 0;

	outp32(mipi_dsi0_base + MIPIDSI_GEN_HDR_OFFSET, rd_cmd);
	status = inp32(mipi_dsi0_base + MIPIDSI_CMD_PKT_STATUS_OFFSET);
	while (status & 0x10) {
		udelay(50);
		if (++try_times > 100) {
			DPU_FB_ERR("Read AMOLED reg:0x%x timeout!\n",
				rd_cmd >> 8);
			break;
		}
		status = inp32(mipi_dsi0_base + MIPIDSI_CMD_PKT_STATUS_OFFSET);
	}
	status = inp32(mipi_dsi0_base + MIPIDSI_GEN_PLD_DATA_OFFSET);
	DPU_FB_INFO("read NT37700P reg:0x%x = 0x%x\n", rd_cmd >> 8, status);

	return 0;
}

static void mipi_panel_power_on(struct platform_device *pdev, int fpga_flag)
{
	if (fpga_flag == 0) {
		vcc_cmds_tx(pdev, g_lcd_vcc_enable_cmds,
			ARRAY_SIZE(g_lcd_vcc_enable_cmds));
		pinctrl_cmds_tx(pdev, g_lcd_pinctrl_normal_cmds,
			ARRAY_SIZE(g_lcd_pinctrl_normal_cmds));
		gpio_cmds_tx(g_asic_lcd_gpio_request_cmds,
			ARRAY_SIZE(g_asic_lcd_gpio_request_cmds));
		gpio_cmds_tx(g_asic_lcd_gpio_normal_cmds,
			ARRAY_SIZE(g_asic_lcd_gpio_normal_cmds));
	} else {
		// lcd gpio request
		gpio_cmds_tx(g_fpga_lcd_gpio_request_cmds,
			ARRAY_SIZE(g_fpga_lcd_gpio_request_cmds));

		// lcd gpio normal
		gpio_cmds_tx(g_fpga_lcd_gpio_normal_cmds,
			ARRAY_SIZE(g_fpga_lcd_gpio_normal_cmds));
	}
}

static int mipi_panel_on(struct platform_device *pdev)
{
	struct dpu_fb_data_type *dpufd = NULL;
	struct dpu_panel_info *pinfo = NULL;
	char __iomem *mipi_dsi0_base = NULL;

	if (pdev == NULL)
		return -EINVAL;

	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL)
		return -EINVAL;

	pinfo = &(dpufd->panel_info);
	if (pinfo == NULL)
		return -EINVAL;

	DPU_FB_INFO("fb%d, +\n", dpufd->index);

	pinfo = &(dpufd->panel_info);
	mipi_dsi0_base = dpufd->mipi_dsi0_base;

	if (pinfo->lcd_init_step == LCD_INIT_POWER_ON) {
		mipi_panel_power_on(pdev, g_lcd_fpga_flag);

		pinfo->lcd_init_step = LCD_INIT_MIPI_LP_SEND_SEQUENCE;
	} else if (pinfo->lcd_init_step == LCD_INIT_MIPI_LP_SEND_SEQUENCE) {
		mdelay(10); /* time constraints */
		mipi_dsi_cmds_tx(g_display_on_cmds,
			ARRAY_SIZE(g_display_on_cmds), mipi_dsi0_base);

		/* read panel power state reg */
		mipi_read_ic_regs(mipi_dsi0_base, 0x0a06);
		pinfo->lcd_init_step = LCD_INIT_MIPI_HS_SEND_SEQUENCE;
	} else if (pinfo->lcd_init_step == LCD_INIT_MIPI_HS_SEND_SEQUENCE) {
		;
	} else {
		DPU_FB_ERR("failed to init lcd!\n");
	}

	dpu_lcd_backlight_on(pdev);
	DPU_FB_DEBUG("fb%d, -!\n", dpufd->index);
	return 0;
}

static int mipi_panel_off(struct platform_device *pdev)
{
	struct dpu_fb_data_type *dpufd = NULL;
	struct dpu_panel_info *pinfo = NULL;
	char __iomem *mipi_dsi0_base = NULL;

	if (pdev == NULL)
		return -EINVAL;
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL)
		return -EINVAL;

	pinfo = &(dpufd->panel_info);

	mipi_dsi0_base = dpufd->mipi_dsi0_base;

	DPU_FB_INFO("fb%d, +\n", dpufd->index);

	if (pinfo->lcd_uninit_step == LCD_UNINIT_MIPI_HS_SEND_SEQUENCE) {
		dpu_lcd_backlight_off(pdev);

		mipi_dsi_cmds_tx(g_lcd_display_off_cmd,
			ARRAY_SIZE(g_lcd_display_off_cmd), mipi_dsi0_base);

		pinfo->lcd_uninit_step = LCD_UNINIT_MIPI_LP_SEND_SEQUENCE;
	} else if (pinfo->lcd_uninit_step == LCD_UNINIT_MIPI_LP_SEND_SEQUENCE) {
		pinfo->lcd_uninit_step = LCD_UNINIT_POWER_OFF;
	} else if (pinfo->lcd_uninit_step == LCD_UNINIT_POWER_OFF) {
		mipi_panel_power_on(pdev, g_lcd_fpga_flag);
	} else {
		DPU_FB_ERR("failed to uninit lcd!\n");
	}

	DPU_FB_INFO("fb%d, -\n", dpufd->index);

	return 0;
}

static int mipi_panel_remove(struct platform_device *pdev)
{
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL)
		return -EINVAL;
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL)
		return -EINVAL;

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);

	if (g_lcd_fpga_flag == 0) {
		vcc_cmds_tx(pdev, g_lcd_vcc_finit_cmds,
			ARRAY_SIZE(g_lcd_vcc_finit_cmds));

		pinctrl_cmds_tx(pdev, g_lcd_pinctrl_finit_cmds,
			ARRAY_SIZE(g_lcd_pinctrl_finit_cmds));
	}

	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);
	return 0;
}

static int mipi_panel_set_backlight(struct platform_device *pdev,
	uint32_t bl_level)
{
	struct dpu_fb_data_type *dpufd = NULL;
	int ret = 0;
	static uint32_t last_bl_level;

	char bl_level_adjust[3] = {
		0x51,
		0x00,
		0x00,
	};
	struct dsi_cmd_desc lcd_bl_level_adjust[] = {
		{DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US,
			sizeof(bl_level_adjust), bl_level_adjust},
	};

	if (pdev == NULL)
		return -1;

	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL)
		return -1;

	if (dpufd->panel_info.bl_set_type & BL_SET_BY_PWM) {

		ret = dpu_pwm_set_backlight(dpufd, bl_level);

	} else if (dpufd->panel_info.bl_set_type & BL_SET_BY_BLPWM) {

		ret = dpu_blpwm_set_backlight(dpufd, bl_level);

	} else if (dpufd->panel_info.bl_set_type & BL_SET_BY_SH_BLPWM) {

		ret = dpu_sh_blpwm_set_backlight(dpufd, bl_level);

	} else if (dpufd->panel_info.bl_set_type & BL_SET_BY_MIPI) {
		/* set backlight level reg */
		bl_level_adjust[1] = (bl_level >> 8) & 0xff;
		bl_level_adjust[2] = bl_level & 0xff;

		DPU_FB_INFO("bl_level is %d,\n"
			"bl_level_adjust[1] = 0x%x,\n"
			"bl_level_adjust[2] = 0x%x\n",
			bl_level,
			bl_level_adjust[1],
			bl_level_adjust[2]);

		if (last_bl_level != bl_level) {
			last_bl_level = bl_level;
			mipi_dsi_cmds_tx(lcd_bl_level_adjust,
				ARRAY_SIZE(lcd_bl_level_adjust),
				dpufd->mipi_dsi0_base);
		}
	} else {
		DPU_FB_ERR("fb%d, not support this bl_set_type %d!\n",
			dpufd->index, dpufd->panel_info.bl_set_type);
	}

	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

static ssize_t mipi_lcd_model_show(struct platform_device *pdev, char *buf)
{
	struct dpu_fb_data_type *dpufd = NULL;
	ssize_t ret;

	if (!pdev || !buf) {
		DPU_FB_ERR("pdev or buf is NULL\n");
		return -EINVAL;
	}

	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);

	ret = snprintf(buf, PAGE_SIZE, "samsung_EA8074 CMD AMOLED PANEL\n");

	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

static ssize_t mipi_panel_lcd_check_reg_show(struct platform_device *pdev,
	char *buf)
{
	ssize_t ret = 0;
	struct dpu_fb_data_type *dpufd = NULL;
	char __iomem *mipi_dsi0_base = NULL;
	uint32_t read_value[4] = {0};
	uint32_t expected_value[4] = { 0x9c, 0x00, 0x00, 0x80 };
	uint32_t read_mask[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
	char *reg_name[4] = {"power mode", "MADCTR", "image mode", "dsi error"};
	char lcd_reg_0a[] = {0x0a};
	char lcd_reg_0b[] = {0x0b};
	char lcd_reg_0d[] = {0x0d};
	char lcd_reg_0e[] = {0x0e};

	struct dsi_cmd_desc lcd_check_reg[] = {
		{ DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg_0a), lcd_reg_0a },
		{ DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg_0b), lcd_reg_0b },
		{ DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg_0d), lcd_reg_0d },
		{ DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg_0e), lcd_reg_0e },
	};


	struct mipi_dsi_read_compare_data data = {
		.read_value = read_value,
		.expected_value = expected_value,
		.read_mask = read_mask,
		.reg_name = reg_name,
		.log_on = 1,
		.cmds = lcd_check_reg,
		.cnt = ARRAY_SIZE(lcd_check_reg),
	};

	if (!pdev || !buf) {
		DPU_FB_ERR("pdev or buf is NULL");
		return -EINVAL;
	}
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	mipi_dsi0_base = dpufd->mipi_dsi0_base;

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);

	if (!mipi_dsi_read_compare(&data, mipi_dsi0_base))
		ret = snprintf(buf, PAGE_SIZE, "OK\n");
	else
		ret = snprintf(buf, PAGE_SIZE, "ERROR\n");

	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return ret;
}

static char g_lcd_disp_x[] = {
	0x2A,
	0x00, 0x00, 0x04, 0x37,
};

static char g_lcd_disp_y[] = {
	0x2B,
	0x00, 0x00, 0x09, 0x23,
};

static struct dsi_cmd_desc g_set_display_address[] = {
	{ DTYPE_DCS_LWRITE, 0, 5, WAIT_TYPE_US,
		sizeof(g_lcd_disp_x), g_lcd_disp_x },
	{ DTYPE_DCS_LWRITE, 0, 5, WAIT_TYPE_US,
		sizeof(g_lcd_disp_y), g_lcd_disp_y },
};
static int mipi_panel_set_display_region(struct platform_device *pdev,
	struct dss_rect *dirty)
{
	uint32_t x;
	uint32_t y;
	uint32_t w;
	uint32_t h;
	struct dpu_fb_data_type *dpufd = NULL;

	if (pdev == NULL || dirty == NULL)
		return -1;
	dpufd = platform_get_drvdata(pdev);
	if (dpufd == NULL)
		return -1;

	x = (uint32_t)dirty->x;
	y = (uint32_t)dirty->y;
	w = (uint32_t)dirty->w;
	h = (uint32_t)dirty->h;

	g_lcd_disp_x[1] = (x >> 8) & 0xff;
	g_lcd_disp_x[2] = x & 0xff;
	g_lcd_disp_x[3] = ((x + w - 1) >> 8) & 0xff;
	g_lcd_disp_x[4] = (x + w - 1) & 0xff;

	g_lcd_disp_y[1] = (y >> 8) & 0xff;
	g_lcd_disp_y[2] = y & 0xff;
	g_lcd_disp_y[3] = ((y + h - 1) >> 8) & 0xff;
	g_lcd_disp_y[4] = (y + h - 1) & 0xff;

	DPU_FB_DEBUG("x[1] = 0x%2x, x[2] = 0x%2x\n"
		"x[3] = 0x%2x, x[4] = 0x%2x\n"
		"y[1] = 0x%2x, y[2] = 0x%2x\n"
		"y[3] = 0x%2x, y[4] = 0x%2x\n",
		g_lcd_disp_x[1], g_lcd_disp_x[2],
		g_lcd_disp_x[3], g_lcd_disp_x[4],
		g_lcd_disp_y[1], g_lcd_disp_y[2],
		g_lcd_disp_y[3], g_lcd_disp_y[4]);

	mipi_dsi_cmds_tx(g_set_display_address,
		ARRAY_SIZE(g_set_display_address), dpufd->mipi_dsi0_base);

	return 0;
}

static struct dpu_panel_info g_panel_info = {0};
static struct dpu_fb_panel_data g_panel_data = {
	.panel_info = &g_panel_info,
	.set_fastboot = mipi_panel_set_fastboot,
	.on = mipi_panel_on,
	.off = mipi_panel_off,
	.remove = mipi_panel_remove,
	.set_backlight = mipi_panel_set_backlight,
	.lcd_model_show = mipi_lcd_model_show,
	.lcd_check_reg = mipi_panel_lcd_check_reg_show,
	.set_display_region = mipi_panel_set_display_region,
};

static int lcd_voltage_relevant_init(struct platform_device *pdev)
{
	int ret = 0;

	if (g_lcd_fpga_flag == 0) {
		ret = vcc_cmds_tx(pdev, g_lcd_vcc_init_cmds,
			ARRAY_SIZE(g_lcd_vcc_init_cmds));
		if (ret != 0) {
			DPU_FB_ERR("LCD vcc init failed!\n");
			return ret;
		}

		ret = pinctrl_cmds_tx(pdev, g_lcd_pinctrl_init_cmds,
			ARRAY_SIZE(g_lcd_pinctrl_init_cmds));
		if (ret != 0) {
			DPU_FB_ERR("Init pinctrl failed, defer\n");
			return ret;
		}

		if (is_fastboot_display_enable())
			vcc_cmds_tx(pdev, g_lcd_vcc_enable_cmds,
				ARRAY_SIZE(g_lcd_vcc_enable_cmds));
	}

	return ret;
}

static void spr_init_global(struct dpu_panel_info *pinfo)
{
	struct spr_dsc_panel_para *spr = NULL;

	if (pinfo == NULL)
		return;

	spr = &(pinfo->spr);

	/* spr global init */
#ifdef SPR_RGB2UYUV_8BITEN
	spr->spr_rgbg2uyvy_8biten = 1;
#else
	spr->spr_rgbg2uyvy_8biten = 0;
#endif
	spr->spr_hpartial_mode = 0;
	spr->spr_partial_mode = 0;
	spr->spr_rgbg2uyvy_en = 1;
	spr->spr_horzborderdect = 1;
	spr->spr_linebuf_1dmode = 0;
	spr->spr_bordertb_dummymode = 1;
	spr->spr_borderlr_dummymode = 3;
	spr->spr_pattern_mode = 1;
	spr->spr_pattern_en = 0;
	spr->spr_subpxl_layout = 1;
	spr->ck_gt_spr_en = 1;
	spr->spr_en = 1;
	spr->spr_lut_table = g_spr_gama_degama_table;
}

/* (<< 24, << 18, << 16, << 12 ... ) means reg offset.
 * As followed.
 */
static void spr_init_coef(struct dpu_panel_info *pinfo)
{
	struct spr_dsc_panel_para *spr = NULL;

	if (pinfo == NULL)
		return;

	spr = &(pinfo->spr);

	/* line coeff sel and output arrange sel */
	spr->spr_coeffsel_even = (0x0 << 20) | (0x0 << 16) | (0x5 << 12)
		| (0x4 << 8) | (0x1 << 4) | 0x0;
	spr->spr_coeffsel_odd = (0x0 << 20) | (0x0 << 16) | (0x4 << 12)
		| (0x3 << 8) | (0x2 << 4) | 0x1;
	spr->pix_panel_arrange_sel = (0x0 << 10) | (0x0 << 8) | (0x2 << 6)
		| (0x0 << 4) | (0x2 << 2) | 0x0;

	/* config Red's v0h0 v0h1 v1h0 v1h1 coefficients */
	spr->spr_r_v0h0_coef[0] = (0x0 << 24) | (0x0 << 18) | (0x0 << 12)
		| (0x0 << 6) | 0x0;
	spr->spr_r_v0h0_coef[1] = (0x0 << 24) | (0x0 << 18) | (0x10 << 12)
		| (0x10 << 6) | 0x0;
	spr->spr_r_v0h0_coef[2] = (0x0 << 24) | (0x0 << 18) | (0x0 << 12)
		| (0x0 << 6) | 0x0;

	spr->spr_r_v0h1_coef[0] = (0x0 << 24) | (0x0 << 18) | (0x0 << 12)
		| (0x0 << 6) | 0x0;
	spr->spr_r_v0h1_coef[1] = (0x0 << 24) | (0x0 << 18) | (0x0 << 12)
		| (0x0 << 6) | 0x0;
	spr->spr_r_v0h1_coef[2] = (0x0 << 24) | (0x0 << 18) | (0x0 << 12)
		| (0x0 << 6) | 0x0;

	spr->spr_r_v1h0_coef[0] = (0x0 << 24) | (0x0 << 18) | (0x0 << 12)
		| (0x0 << 6) | 0x0;
	spr->spr_r_v1h0_coef[1] = (0x0 << 24) | (0x0 << 18) | (0x0 << 12)
		| (0x0 << 6) | 0x0;
	spr->spr_r_v1h0_coef[2] = (0x0 << 24) | (0x0 << 18) | (0x0 << 12)
		| (0x0 << 6) | 0x0;

	spr->spr_r_v1h1_coef[0] = (0x0 << 24) | (0x0 << 18) | (0x0 << 12)
		| (0x0 << 6) | 0x0;
	spr->spr_r_v1h1_coef[1] = (0x0 << 24) | (0x0 << 18) | (0x10 << 12)
		| (0x10 << 6) | 0x0;
	spr->spr_r_v1h1_coef[2] = (0x0 << 24) | (0x0 << 18) | (0x0 << 12)
		| (0x0 << 6) | 0x0;

	/* config Green's v0h0 v0h1 v1h0 v1h1 coefficients */
	spr->spr_g_v0h0_coef[0] = (0x0 << 24) | (0x0 << 18) | (0x0 << 12)
		| (0x0 << 6) | 0x0;
	spr->spr_g_v0h0_coef[1] = (0x0 << 24) | (0x0 << 18) | (0x1f << 12)
		| (0x0 << 6) | 0x0;
	spr->spr_g_v0h0_coef[2] = (0x0 << 24) | (0x0 << 18) | (0x0 << 12)
		| (0x0 << 6) | 0x0;

	spr->spr_g_v0h1_coef[0] = (0x0 << 24) | (0x0 << 18) | (0x0 << 12)
		| (0x0 << 6) | 0x0;
	spr->spr_g_v0h1_coef[1] = (0x0 << 24) | (0x0 << 18) | (0x1f << 12)
		| (0x0 << 6) | 0x0;
	spr->spr_g_v0h1_coef[2] = (0x0 << 24) | (0x0 << 18) | (0x0 << 12)
		| (0x0 << 6) | 0x0;

	spr->spr_g_v1h0_coef[0] = (0x0 << 24) | (0x0 << 18) | (0x0 << 12)
		| (0x0 << 6) | 0x0;
	spr->spr_g_v1h0_coef[1] = (0x0 << 24) | (0x0 << 18) | (0x1f << 12)
		| (0x0 << 6) | 0x0;
	spr->spr_g_v1h0_coef[2] = (0x0 << 24) | (0x0 << 18) | (0x0 << 12)
		| (0x0 << 6) | 0x0;

	spr->spr_g_v1h1_coef[0] = (0x0 << 24) | (0x0 << 18) | (0x0 << 12)
		| (0x0 << 6) | 0x0;
	spr->spr_g_v1h1_coef[1] = (0x0 << 24) | (0x0 << 18) | (0x1f << 12)
		| (0x0 << 6) | 0x0;
	spr->spr_g_v1h1_coef[2] = (0x0 << 24) | (0x0 << 18) | (0x0 << 12)
		| (0x0 << 6) | 0x0;

	/* config Blue's v0h0 v0h1 v1h0 v1h1 coefficients */
	spr->spr_b_v0h0_coef[0] = (0x0 << 24) | (0x0 << 18) | (0x0 << 12)
		| (0x0 << 6) | 0x0;
	spr->spr_b_v0h0_coef[1] = (0x0 << 24) | (0x0 << 18) | (0x0 << 12)
		| (0x0 << 6) | 0x0;
	spr->spr_b_v0h0_coef[2] = (0x0 << 24) | (0x0 << 18) | (0x0 << 12)
		| (0x0 << 6) | 0x0;

	spr->spr_b_v0h1_coef[0] = (0x0 << 24) | (0x0 << 18) | (0x0 << 12)
		| (0x0 << 6) | 0x0;
	spr->spr_b_v0h1_coef[1] = (0x0 << 24) | (0x0 << 18) | (0x10 << 12)
		| (0x10 << 6) | 0x0;
	spr->spr_b_v0h1_coef[2] = (0x0 << 24) | (0x0 << 18) | (0x0 << 12)
		| (0x0 << 6) | 0x0;

	spr->spr_b_v1h0_coef[0] = (0x0 << 24) | (0x0 << 18) | (0x0 << 12)
		| (0x0 << 6) | 0x0;
	spr->spr_b_v1h0_coef[1] = (0x0 << 24) | (0x0 << 18) | (0x10 << 12)
		| (0x10 << 6) | 0x0;
	spr->spr_b_v1h0_coef[2] = (0x0 << 24) | (0x0 << 18) | (0x0 << 12)
		| (0x0 << 6) | 0x0;

	spr->spr_b_v1h1_coef[0] = (0x0 << 24) | (0x0 << 18) | (0x0 << 12)
		| (0x0 << 6) | 0x0;
	spr->spr_b_v1h1_coef[1] = (0x0 << 24) | (0x0 << 18) | (0x0 << 12)
		| (0x0 << 6) | 0x0;
	spr->spr_b_v1h1_coef[2] = (0x0 << 24) | (0x0 << 18) | (0x0 << 12)
		| (0x0 << 6) | 0x0;
}

static void spr_init_border(struct dpu_panel_info *pinfo)
{
	struct spr_dsc_panel_para *spr = NULL;

	if (pinfo == NULL)
		return;

	spr = &(pinfo->spr);

	/* config RGB's border detect gain parameters */
	spr->spr_borderlr_detect_r = (0x0 << 24) | (0x80 << 16) | (0x0 << 8)
		| 0x80;
	spr->spr_bordertb_detect_r = (0x0 << 24) | (0x80 << 16) | (0x0 << 8)
		| 0x80;
	spr->spr_borderlr_detect_g = (0x0 << 24) | (0x60 << 16) | (0x0 << 8)
		| 0x80;
	spr->spr_bordertb_detect_g = (0x0 << 24) | (0x80 << 16) | (0x0 << 8)
		| 0x80;
	spr->spr_borderlr_detect_b = (0x0 << 24) | (0x80 << 16) | (0x0 << 8)
		| 0x80;
	spr->spr_bordertb_detect_b = (0x0 << 24) | (0x80 << 16) | (0x0 << 8)
		| 0x80;

	/* config RGB's final gain parameters */
	spr->spr_pixgain = ((uint64_t)0 << (24 + 32))
		| ((uint64_t)128 << (16 + 32)) | ((uint64_t)0 << (8 + 32))
		| ((uint64_t)128 << 32) | ((uint64_t)0x0 << 8) | 0x80;

	/* config boarder region */
	spr->spr_borderl_position = (1 << 12) | 0x0;
	spr->spr_borderr_position = (pinfo->xres << 12) | (pinfo->xres - 1);
	spr->spr_bordert_position = 0x0;
	spr->spr_borderb_position = 0x0;
}

static void spr_init_rgb(struct dpu_panel_info *pinfo)
{
	struct spr_dsc_panel_para *spr = NULL;

	if (pinfo == NULL)
		return;

	spr = &(pinfo->spr);

	/* config RGB's diff ,weight regs */
	spr->spr_r_diff = (64 << 20) | (64 << 12) | (32 << 4) | 4;
	spr->spr_r_weight = ((uint64_t)128 << (24 + 32))
		| ((uint64_t)64 << (16 + 32)) | ((uint64_t)128 << (8 + 32))
		| ((uint64_t)64 << 32) | ((uint64_t)1 << 16)
		| ((uint64_t)0 << 15) | ((uint64_t)1 << 14)
		| ((uint64_t)52 << 8) | (uint64_t)64;

	spr->spr_g_diff = ((uint64_t)64 << 20) | ((uint64_t)64 << 12)
		| ((uint64_t)32 << 4) | 4;
	spr->spr_g_weight = ((uint64_t)128 << (24 + 32))
		| ((uint64_t)64 << (16 + 32)) | ((uint64_t)128 << (8 + 32))
		| ((uint64_t)64 << 32) | ((uint64_t)1 << 16)
		| ((uint64_t)1 << 15) | ((uint64_t)0 << 14)
		| ((uint64_t)56 << 8) | (uint64_t)64;

	spr->spr_b_diff = ((uint64_t)64 << 20) | ((uint64_t)64 << 12)
		| ((uint64_t)32 << 4) | (uint64_t)4;
	spr->spr_b_weight = ((uint64_t)128 << (24 + 32))
		| ((uint64_t)64 << (16 + 32)) | ((uint64_t)128 << (8 + 32))
		| ((uint64_t)64 << 32) | ((uint64_t)1 << 18)
		| ((uint64_t)2 << 16) | ((uint64_t)2 << 14)
		| ((uint64_t)56 << 8) | 64;

	/* config rgbg2uyvy  filtering coeff */
	spr->spr_rgbg2uyvy_coeff[0] = (0x0 << 21) | (0x6 << 18) | (0x0 << 15)
		| (0x2 << 12) | (0x0 << 9) | (0x1 << 6) | (0x2 << 3) | 0x1;
	spr->spr_rgbg2uyvy_coeff[1] = (0x2 << 21) | (0x1 << 18) | (0x0 << 15)
		| (0x1 << 12) | (0x1 << 9) | (0x7 << 6) | (0x1 << 3) | 0x7;
	if (spr->spr_rgbg2uyvy_8biten) {
		spr->spr_rgbg2uyvy_coeff[2] = (0x80 << 12) | 0x0;
		spr->spr_rgbg2uyvy_coeff[3] = (0x0 << 12) | 0x80;
	} else {
		spr->spr_rgbg2uyvy_coeff[2] = (0x200 << 12) | 0x0;
		spr->spr_rgbg2uyvy_coeff[3] = (0x0 << 12) | 0x200;
	}
}

static void spr_init_dsc_8bit_en(struct dpu_panel_info *pinfo)
{
	struct spr_dsc_panel_para *spr = NULL;

	if (pinfo == NULL)
		return;

	spr = &(pinfo->spr);

	/* dsc global init.
	 * These parameters correspond with DDIC which is fixed in panel.
	 */
	spr->input_reso = ((pinfo->yres - 1) << 16) | (pinfo->xres - 1);
	spr->dsc_enable = 1;
	spr->rcb_bits = 10908;
	spr->dsc_alg_ctrl = 0x2;
	spr->bits_per_component = 8;
	spr->dsc_sample = 0x1;

	spr->bpp_chk = (810 << 16) | 192;
	spr->pic_reso = (pinfo->yres << 16) | pinfo->xres;
	spr->slc_reso = (60 << 16) | pinfo->xres;

	spr->initial_xmit_delay = 512;
	spr->initial_dec_delay = 397;
	spr->initial_scale_value = 32;
	spr->scale_interval = (2481 << 16) | 7;
	spr->first_bpg = (15 << 16) | 521;
	spr->second_bpg = (0 << 16) | 0;
	spr->second_adj = 0;
	spr->init_finl_ofs = (6144 << 16) | 2336;
	spr->slc_bpg = 443;
	spr->flat_range = (3 << 8) | 12;
	spr->rc_mode_edge = (8192 << 16) | 6;
	spr->rc_qua_tgt = (3 << 20) | (3 << 16) | (11 << 8) | 11;

	/* Specify thresholds in the "RC model" for the 15 ranges */
	spr->rc_buf_thresh[0] = (56 << 24) | (42 << 16) | (28 << 8) | 14;
	spr->rc_buf_thresh[1] = (105 << 24) | (98 << 16) | (84 << 8) | 70;
	spr->rc_buf_thresh[2] = (123 << 24) | (121 << 16) | (119 << 8) | 112;
	spr->rc_buf_thresh[3] = (126 << 8) | 125;

	/* RC params for 15 registers */
	spr->rc_para[0] = (0 << 16) | (4 << 8) | 2;
	spr->rc_para[1] = (0 << 16) | (4 << 8) | 0;
	spr->rc_para[2] = (1 << 16) | (5 << 8) | 0;
	spr->rc_para[3] = (1 << 16) | (6 << 8) | 62;
	spr->rc_para[4] = (3 << 16) | (7 << 8) | 60;
	spr->rc_para[5] = (3 << 16) | (7 << 8) | 58;
	spr->rc_para[6] = (3 << 16) | (7 << 8) | 56;
	spr->rc_para[7] = (3 << 16) | (8 << 8) | 56;
	spr->rc_para[8] = (3 << 16) | (9 << 8) | 56;
	spr->rc_para[9] = (3 << 16) | (10 << 8) | 54;
	spr->rc_para[10] = (5 << 16) | (10 << 8) | 54;
	spr->rc_para[11] = (5 << 16) | (11 << 8) | 52;
	spr->rc_para[12] = (5 << 16) | (11 << 8) | 52;
	spr->rc_para[13] = (9 << 16) | (12 << 8) | 52;
	spr->rc_para[14] = (12 << 16) | (13 << 8) | 52;
}

#ifndef SPR_RGB2UYUV_8BITEN
static void spr_init_dsc_8bit_disable(struct dpu_panel_info *pinfo)
{
	struct spr_dsc_panel_para *spr = NULL;

	if (pinfo == NULL)
		return;

	spr = &(pinfo->spr);

	/* dsc global init.
	 * These parameters correspond with DDIC which is fixed in panel.
	 */
	spr->input_reso = ((pinfo->yres - 1) << 16) | (pinfo->xres - 1);
	spr->dsc_enable = 1;

	/* dsc global init */
	spr->rcb_bits = 14288;
	spr->dsc_alg_ctrl = 0x2;
	spr->bits_per_component = 0xa;
	spr->dsc_sample = 0x1;

	spr->bpp_chk = (1080 << 16) | 256;
	spr->initial_xmit_delay = 341;
	spr->initial_dec_delay = 552;
	spr->initial_scale_value = 10;
	spr->scale_interval = (1198 << 16) | 90;
	spr->first_bpg = (15 << 16) | 521;
	spr->second_bpg = (0 << 16) | 0;
	spr->second_adj = 0;
	spr->init_finl_ofs = (2048 << 16) | 3072;
	spr->slc_bpg = 1229;
	spr->flat_range = (7 << 8) | 16;
	spr->rc_mode_edge = (8192 << 16) | 6;
	spr->rc_qua_tgt = (3 << 20) | (3 << 16) | (15 << 8) | 15;

	/* Specify thresholds in the "RC model" for the 15 ranges */
	spr->rc_buf_thresh[0] = (56 << 24) | (42 << 16)
		| (28 << 8) | 14;
	spr->rc_buf_thresh[1] = (105 << 24) | (98 << 16)
		| (84 << 8) | 70;
	spr->rc_buf_thresh[2] = (123 << 24) | (121 << 16)
		| (119 << 8) | 112;
	spr->rc_buf_thresh[3] = (126 << 8) | 125;

	/* RC params for 15 registers */
	spr->rc_para[0] = (0 << 16) | (2 << 8) | 2;
	spr->rc_para[1] = (2 << 16) | (5 << 8) | 0;
	spr->rc_para[2] = (3 << 16) | (7 << 8) | 0;
	spr->rc_para[3] = (4 << 16) | (8 << 8) | 62;
	spr->rc_para[4] = (6 << 16) | (9 << 8) | 60;
	spr->rc_para[5] = (7 << 16) | (10 << 8) | 58;
	spr->rc_para[6] = (7 << 16) | (11 << 8) | 56;
	spr->rc_para[7] = (7 << 16) | (12 << 8) | 56;
	spr->rc_para[8] = (7 << 16) | (12 << 8) | 56;
	spr->rc_para[9] = (7 << 16) | (13 << 8) | 54;
	spr->rc_para[10] = (9 << 16) | (13 << 8) | 54;
	spr->rc_para[11] = (9 << 16) | (13 << 8) | 52;
	spr->rc_para[12] = (9 << 16) | (13 << 8) | 52;
	spr->rc_para[13] = (11 << 16) | (14 << 8) | 52;
	spr->rc_para[14] = (14 << 16) | (15 << 8) | 52;
}
#endif

static void spr_dsc_panel_para_init(struct dpu_panel_info *pinfo)
{
	if (pinfo == NULL)
		return;

	spr_init_global(pinfo);
	spr_init_coef(pinfo);
	spr_init_border(pinfo);
	spr_init_rgb(pinfo);
#ifdef SPR_RGB2UYUV_8BITEN
	spr_init_dsc_8bit_en(pinfo);
#else
	spr_init_dsc_8bit_disable(pinfo);
#endif
}

static void dsc_config_initial(struct dpu_panel_info *pinfo)
{
	struct dsc_algorithm_manager *pt = get_dsc_algorithm_manager_instance();
	struct input_dsc_info input_dsc_info;

	dpu_check_and_no_retval((pt == NULL) || (pinfo == NULL), ERR, "[DSC] NULL Pointer");
	DPU_FB_DEBUG("+\n");
	pinfo->ifbc_type = IFBC_TYPE_VESA4X_SINGLE_SPR;
	input_dsc_info.dsc_version = DSC_VERSION_V_1_2;
	input_dsc_info.format = DSC_YUV422;
	input_dsc_info.pic_width = pinfo->xres;
	input_dsc_info.pic_height = pinfo->yres;
	input_dsc_info.slice_width = input_dsc_info.pic_width;
	// slice height is same with DDIC
	input_dsc_info.slice_height = 59 + 1;
	input_dsc_info.dsc_bpp = DSC_12BPP;
	input_dsc_info.dsc_bpc = DSC_8BPC;
	input_dsc_info.block_pred_enable = 1;
	input_dsc_info.linebuf_depth = LINEBUF_DEPTH_9;
	input_dsc_info.gen_rc_params = DSC_NOT_GENERATE_RC_PARAMETERS;
	pt->vesa_dsc_info_calc(&input_dsc_info, &(pinfo->panel_dsc_info.dsc_info), NULL);
	pinfo->vesa_dsc.bits_per_pixel = DSC_0BPP;
}

static void panel_info_initial(struct dpu_fb_panel_data *pdata,
	uint32_t bl_type, uint32_t lcd_display_type)
{
	struct dpu_panel_info *pinfo = NULL;

	pinfo = pdata->panel_info;
	memset(pinfo, 0, sizeof(struct dpu_panel_info));
	pinfo->xres = 1080;
	pinfo->yres = 2340;
	pinfo->width = 67;
	pinfo->height = 139;
	pinfo->orientation = LCD_PORTRAIT;
	pinfo->bpp = LCD_RGB888;
	pinfo->bgr_fmt = LCD_RGB;
	pinfo->bl_set_type = bl_type;
	pinfo->bl_min = 12;
	pinfo->bl_max = 2047;
	pinfo->bl_default = 600;

	pinfo->spr_dsc_mode = SPR_DSC_MODE_SPR_AND_DSC;
	pinfo->frc_enable = 0;
	pinfo->esd_enable = 0;
	pinfo->esd_skip_mipi_check = 0;
	pinfo->lcd_uninit_step_support = 1;

	pinfo->type = lcd_display_type;

	/* lcd porch size init */
	if (g_lcd_fpga_flag == 1) {
		pinfo->ldi.h_back_porch = 23;
		pinfo->ldi.h_front_porch = 50;
		pinfo->ldi.h_pulse_width = 20;
		pinfo->ldi.v_back_porch = 12;
		pinfo->ldi.v_front_porch = 14;
		pinfo->ldi.v_pulse_width = 4;
		pinfo->pxl_clk_rate = 20 * 1000000UL;
		pinfo->mipi.dsi_bit_clk = 120;
	} else {
		pinfo->ldi.h_back_porch = 20;
		pinfo->ldi.h_front_porch = 30;
		pinfo->ldi.h_pulse_width = 10;
		pinfo->ldi.v_back_porch = 36;
		pinfo->ldi.v_front_porch = 20;
		pinfo->ldi.v_pulse_width = 30;
		pinfo->pxl_clk_rate = 166 * 1000000UL;
		pinfo->mipi.dsi_bit_clk = 480;
	}

	pinfo->mipi.dsi_bit_clk_val1 = 271;
	pinfo->mipi.dsi_bit_clk_val2 = 280;
	pinfo->mipi.dsi_bit_clk_val3 = 290;
	pinfo->mipi.dsi_bit_clk_val4 = 300;
	pinfo->dsi_bit_clk_upt_support = 0;
	pinfo->mipi.dsi_bit_clk_upt = pinfo->mipi.dsi_bit_clk;

	pinfo->mipi.lane_nums = DSI_4_LANES;
	pinfo->mipi.color_mode = DSI_24BITS_1;
	pinfo->mipi.vc = 0;
	pinfo->mipi.max_tx_esc_clk = 10 * 1000000;
	pinfo->mipi.burst_mode = DSI_BURST_SYNC_PULSES_1;
	pinfo->mipi.phy_mode = DPHY_MODE;
	pinfo->mipi.dsi_version = DSI_1_1_VERSION;
	pinfo->mipi.non_continue_en = 1;

	pinfo->ifbc_type = IFBC_TYPE_NONE;
	pinfo->pxl_clk_rate_div = 1;

#if defined(CONFIG_DPU_FB_V600) || defined(CONFIG_DPU_FB_V360)
	dsc_config_initial(pinfo);
#endif
	spr_dsc_panel_para_init(pinfo);

	if (pinfo->type == PANEL_MIPI_CMD) {
		pinfo->vsync_ctrl_type = VSYNC_CTRL_ISR_OFF |
			VSYNC_CTRL_MIPI_ULPS | VSYNC_CTRL_CLK_OFF;
		pinfo->dirty_region_updt_support = 0;
		pinfo->dirty_region_info.left_align = -1;
		pinfo->dirty_region_info.right_align = -1;
		pinfo->dirty_region_info.top_align = 60;
		pinfo->dirty_region_info.bottom_align = 60;
		pinfo->dirty_region_info.w_align = -1;
		pinfo->dirty_region_info.h_align = 120;
		pinfo->dirty_region_info.w_min = 1080;
		pinfo->dirty_region_info.h_min = 120;
		pinfo->dirty_region_info.top_start = -1;
		pinfo->dirty_region_info.bottom_start = -1;
	}

	pinfo->current_mode = MODE_8BIT;

#ifdef CONFIG_DPU_FB_V510
	if (pinfo->spr_dsc_mode != SPR_DSC_MODE_NONE) {
		pinfo->ifbc_type = IFBC_TYPE_NONE;
		pinfo->pxl_clk_rate_div = 1;
	} else	{
		get_vesa_dsc_para(pinfo, pinfo->current_mode);
	}
#endif

	if (g_lcd_fpga_flag == 0) {
		if (pinfo->pxl_clk_rate_div > 1) {
			pinfo->ldi.h_back_porch /= pinfo->pxl_clk_rate_div;
			pinfo->ldi.h_front_porch /= pinfo->pxl_clk_rate_div;
			pinfo->ldi.h_pulse_width /= pinfo->pxl_clk_rate_div;
		}
	}
}

static int mipi_panel_probe(struct platform_device *pdev)
{
	int ret = -1;

	struct device_node *np = NULL;
	uint32_t bl_type;
	uint32_t lcd_display_type;
	uint32_t lcd_ifbc_type;

	DPU_FB_INFO("%s +\n", DTS_COMP_BOE_NT37700P);

	np = of_find_compatible_node(NULL, NULL, DTS_COMP_BOE_NT37700P);
	if (np == NULL) {
		DPU_FB_ERR("not found device node %s!\n",
			DTS_COMP_BOE_NT37700P);
		goto err_return;
	}

	ret = of_property_read_u32(np, LCD_DISPLAY_TYPE_NAME,
		&lcd_display_type);
	if (ret) {
		DPU_FB_ERR("get lcd_display_type failed!\n");
		lcd_display_type = PANEL_MIPI_CMD;
	}

	ret = of_property_read_u32(np, LCD_IFBC_TYPE_NAME,
		&lcd_ifbc_type);
	if (ret) {
		DPU_FB_ERR("get ifbc_type failed!\n");
		lcd_ifbc_type = IFBC_TYPE_NONE;
	}

	ret = of_property_read_u32(np, LCD_BL_TYPE_NAME,
		&bl_type);
	if (ret) {
		DPU_FB_ERR("get lcd_bl_type failed!\n");
		bl_type = BL_SET_BY_MIPI;
	}
	DPU_FB_INFO("bl_type = 0x%x\n", bl_type);

	if (dpu_fb_device_probe_defer(lcd_display_type, bl_type))
		goto err_probe_defer;


	ret = of_property_read_u32(np, FPGA_FLAG_NAME,
		&g_lcd_fpga_flag);//lint !e64
	if (ret)
		DPU_FB_ERR("get g_lcd_fpga_flag failed!\n");

	if (g_lcd_fpga_flag == 1) {
		g_gpio_amoled_reset = of_get_named_gpio(np, "gpios", 0);
		g_gpio_amoled_vcc3v1 = of_get_named_gpio(np, "gpios", 1);
		g_gpio_amoled_vcc1v8 = of_get_named_gpio(np, "gpios", 2);
	} else {
		g_gpio_amoled_reset = of_get_named_gpio(np, "gpios", 0);
		g_gpio_amoled_te0 = of_get_named_gpio(np, "gpios", 1);
		g_gpio_amoled_vcc3v1 = of_get_named_gpio(np, "gpios", 2);
	}

	if (!pdev) {
		DPU_FB_ERR("pdev is NULL\n");
		return 0;
	}

	pdev->id = 1;

	panel_info_initial(&g_panel_data, bl_type, lcd_display_type);

	ret = lcd_voltage_relevant_init(pdev);
	if (ret != 0)
		goto err_return;

	ret = platform_device_add_data(pdev, &g_panel_data,
		sizeof(struct dpu_fb_panel_data));
	if (ret) {
		DPU_FB_ERR("platform_device_add_data failed!\n");
		goto err_device_put;
	}

	dpu_fb_add_device(pdev);

	DPU_FB_DEBUG("-\n");
	return 0;

err_device_put:
	platform_device_put(pdev);
err_return:
	return ret;
err_probe_defer:
	return -EPROBE_DEFER;

}

static const struct of_device_id g_dpu_panel_match_table[] = {
	{
		.compatible = DTS_COMP_BOE_NT37700P,
		.data = NULL,
	},
	{},
};
MODULE_DEVICE_TABLE(of, g_dpu_panel_match_table);

static struct platform_driver g_this_driver = {
	.probe = mipi_panel_probe,
	.remove = NULL,
	.suspend = NULL,
	.resume = NULL,
	.shutdown = NULL,
	.driver = {
		.name = "mipi_boe_NT37700P",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(g_dpu_panel_match_table),
	},
};

static int __init mipi_lcd_panel_init(void)
{
	int ret;

	ret = platform_driver_register(&g_this_driver);
	if (ret) {
		DPU_FB_ERR("platform_driver_register failed, error = %d!\n",
			ret);
		return ret;
	}

	return ret;
}

/* lint -restore */
module_init(mipi_lcd_panel_init);
