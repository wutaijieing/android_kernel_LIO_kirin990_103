/* Copyright (c) 2008-2014, Hisilicon Tech. Co., Ltd. All rights reserved.
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#ifdef CONFIG_HUAWEI_TS
#include <huawei_platform/touthscreen/huawei_touchscreen.h>
#endif

#include "hisi_fb.h"
#include "hisi_mipi_dsi.h"
#include "hisi_disp_debug.h"
#include "hisi_disp_panel.h"
#include "hisi_connector_utils.h"
#include "hisi_mipi_core.h"
#include "hisi_mipi_utils.h"

#define DTS_COMP_LG_TD4322   "dkmd,td4322_panel"


static int g_lcd_fpga_flag;


/*******************************************************************************
** Display ON/OFF Sequence begin
*/
/*lint -save -e569, -e574, -e527, -e572*/
static char te_enable[] = {
	0x35,
	0x00,
};

static char bl_enable[] = {
	0x53,
	0x24,
};

static char exit_sleep[] = {
	0x11,
};

static char display_on[] = {
	0x29,
};

static char display_PHY1[] = {
	0xB0,
	0x04,
};
static char display_PHY2[] = {
	0xB6,
	0x38,
	0x93,
	0x00,
};
static char lcd_power_on_cmd1[] = {
	0xD6,
	0x01,
};

static char color_enhancement[] = {
	0x30,
	0x00,0x00,0x02,0xA7,
};

static char lcd_disp_x[] = {
	0x2A,
	0x00, 0x00,0x04,0x37
};

static char lcd_disp_y[] = {
	0x2B,
	0x00, 0x00,0x07,0x7F
};

static struct dsi_cmd_desc set_display_address[] = {
	{DTYPE_DCS_LWRITE, 0, 5, WAIT_TYPE_US,
		sizeof(lcd_disp_x), lcd_disp_x},
	{DTYPE_DCS_LWRITE, 0, 5, WAIT_TYPE_US,
		sizeof(lcd_disp_y), lcd_disp_y},
};

static char display_off[] = {
	0x28,
};

static char enter_sleep[] = {
	0x10,
};

static struct dsi_cmd_desc lcd_display_on_cmds[] = {
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(bl_enable), bl_enable},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(te_enable), te_enable},
	{DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement), color_enhancement},
	{DTYPE_DCS_WRITE, 0, 120, WAIT_TYPE_MS,
		sizeof(exit_sleep), exit_sleep},

	{DTYPE_DCS_WRITE, 0, 120, WAIT_TYPE_MS,
		sizeof(display_on), display_on},

};

static struct dsi_cmd_desc lg_fpga_modify_phyclk_config_cmds[] = {
	{DTYPE_GEN_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(lcd_power_on_cmd1), lcd_power_on_cmd1},
	{DTYPE_GEN_LWRITE, 0, 120, WAIT_TYPE_MS,
		sizeof(display_PHY1), display_PHY1},
	{DTYPE_GEN_LWRITE, 0, 120, WAIT_TYPE_MS,
		sizeof(display_PHY2), display_PHY2},
};

static struct dsi_cmd_desc lcd_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 0, 52, WAIT_TYPE_MS,sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 0, 102, WAIT_TYPE_MS,sizeof(enter_sleep), enter_sleep},
};

/******************************************************************************
*
** Display Effect Sequence(smart color, edge enhancement, smart contrast, cabc)
*/
static char PWM_OUT_0x51[] = {
	0x51,
	0xFE,
};

static struct dsi_cmd_desc pwm_out_on_cmds[] = {
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(PWM_OUT_0x51), PWM_OUT_0x51},
};


/*******************************************************************************
** LCD VCC
*/
#define VCC_LCDIO_NAME		"lcdio-vcc"
#define VCC_LCDANALOG_NAME	"lcdanalog-vcc"

static struct regulator *vcc_lcdio;
static struct regulator *vcc_lcdanalog;

static struct vcc_desc lcd_vcc_init_cmds[] = {
	/* vcc get */
	{DTYPE_VCC_GET, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0, WAIT_TYPE_MS, 0},
	{DTYPE_VCC_GET, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0, WAIT_TYPE_MS, 0},

	/* vcc set voltage */
	{DTYPE_VCC_SET_VOLTAGE, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 3100000, 3100000, WAIT_TYPE_MS, 0},
	/* io set voltage */
	{DTYPE_VCC_SET_VOLTAGE, VCC_LCDIO_NAME, &vcc_lcdio, 1800000, 1800000, WAIT_TYPE_MS, 0},
};

static struct vcc_desc lcd_vcc_finit_cmds[] = {
	/* vcc put */
	{DTYPE_VCC_PUT, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0, WAIT_TYPE_MS, 0},
	{DTYPE_VCC_PUT, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0, WAIT_TYPE_MS, 0},
};

static struct vcc_desc lcd_vcc_enable_cmds[] = {
	/* vcc enable */
	{DTYPE_VCC_ENABLE, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0, WAIT_TYPE_MS, 3},
	{DTYPE_VCC_ENABLE, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0, WAIT_TYPE_MS, 3},
};

static struct vcc_desc lcd_vcc_disable_cmds[] = {
	/* vcc disable */
	{DTYPE_VCC_DISABLE, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0, WAIT_TYPE_MS, 3},
	{DTYPE_VCC_DISABLE, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0, WAIT_TYPE_MS, 3},
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

#define GPIO_LCD_P5V5_ENABLE_NAME	"gpio_lcd_p5v5_enable"
#define GPIO_LCD_N5V5_ENABLE_NAME "gpio_lcd_n5v5_enable"
#define GPIO_LCD_RESET_NAME	"gpio_lcd_reset"
#define GPIO_LCD_BL_ENABLE_NAME	"gpio_lcd_bl_enable"

#define GPIO_LCD_TP1V8_NAME	"gpio_lcd_tp1v8"
#define GPIO_LCD_TP2V85_NAME	"gpio_lcd_tp2v85"
//#define GPIO_LCD_ID0_NAME	"gpio_lcd_id0"

static uint32_t gpio_lcd_p5v5_enable;
static uint32_t gpio_lcd_n5v5_enable;
static uint32_t gpio_lcd_reset;
static uint32_t gpio_lcd_bl_enable;

static uint32_t gpio_lcd_tp1v8;
static uint32_t g_gpio_lcd_tp2v85;
//static uint32_t gpio_lcd_id0;


static struct gpio_desc fpga_lcd_gpio_request_cmds[] = {
	/* AVDD_5.5V */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_p5v5_enable, 0},
	/* AVEE_-5.5V */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_n5v5_enable, 0},
	/* reset */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},
	/* backlight enable */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_BL_ENABLE_NAME, &gpio_lcd_bl_enable, 0},
	/* TP_1.8V */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_TP1V8_NAME, &gpio_lcd_tp1v8, 0},
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_TP2V85_NAME, &g_gpio_lcd_tp2v85, 0},
};

static struct gpio_desc fpga_lcd_gpio_normal_cmds[] = {
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 12,
		GPIO_LCD_TP2V85_NAME, &g_gpio_lcd_tp2v85, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20,
		GPIO_LCD_TP1V8_NAME, &gpio_lcd_tp1v8, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 12,
		GPIO_LCD_TP1V8_NAME, &gpio_lcd_tp1v8, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20,
		GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_p5v5_enable, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20,
		GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_n5v5_enable, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 15,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_US, 10,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 50,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
		GPIO_LCD_BL_ENABLE_NAME, &gpio_lcd_bl_enable, 1},
};

static struct gpio_desc fpga_lcd_gpio_lowpower_cmds[] = {
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
		GPIO_LCD_BL_ENABLE_NAME, &gpio_lcd_bl_enable, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 3,
		GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_n5v5_enable, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 3,
		GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_p5v5_enable, 0},

	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 3,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},

	{DTYPE_GPIO_INPUT, WAIT_TYPE_US, 0,
		GPIO_LCD_BL_ENABLE_NAME, &gpio_lcd_bl_enable, 0},
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 0,
		GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_n5v5_enable, 0},
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 0,
		GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_p5v5_enable, 0},

	{DTYPE_GPIO_INPUT, WAIT_TYPE_US, 0,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},
};

static struct gpio_desc fpga_lcd_gpio_free_cmds[] = {
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 0,
		GPIO_LCD_BL_ENABLE_NAME, &gpio_lcd_bl_enable, 0},
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 0,
		GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_n5v5_enable, 0},
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 0,
		GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_p5v5_enable, 0},
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 0,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},
};


static struct gpio_desc asic_lcd_gpio_request_cmds[] = {
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_p5v5_enable, 0},
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_n5v5_enable, 0},
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_BL_ENABLE_NAME, &gpio_lcd_bl_enable, 0},
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},
};

static struct gpio_desc asic_lcd_gpio_normal_cmds[] = {
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 10,
		GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_p5v5_enable, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 10,
		GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_n5v5_enable, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 15,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_BL_ENABLE_NAME, &gpio_lcd_bl_enable, 1},
};

static struct gpio_desc asic_lcd_gpio_free_cmds[] = {
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 50,
		GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_n5v5_enable, 0},
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 50,
		GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_p5v5_enable, 0},
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 0,
		GPIO_LCD_BL_ENABLE_NAME, &gpio_lcd_bl_enable, 0},
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 50,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},
};

static struct gpio_desc asic_lcd_gpio_lowpower_cmds[] = {
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_BL_ENABLE_NAME, &gpio_lcd_bl_enable, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_n5v5_enable, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_p5v5_enable, 0},

	{DTYPE_GPIO_INPUT, WAIT_TYPE_US, 100,
		GPIO_LCD_BL_ENABLE_NAME, &gpio_lcd_bl_enable, 0},
	{DTYPE_GPIO_INPUT, WAIT_TYPE_US, 100,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_n5v5_enable, 0},
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_p5v5_enable, 0},
};

/*******************************************************************************
** ACM
*/
static uint32_t acm_r0_hh = 0x7f;
static uint32_t acm_r0_lh = 0x0;
static uint32_t acm_r1_hh = 0xff;
static uint32_t acm_r1_lh = 0x80;
static uint32_t acm_r2_hh = 0x17f;
static uint32_t acm_r2_lh = 0x100;
static uint32_t acm_r3_hh = 0x1ff;
static uint32_t acm_r3_lh = 0x180;
static uint32_t acm_r4_hh = 0x27f;
static uint32_t acm_r4_lh = 0x200;
static uint32_t acm_r5_hh = 0x2ff;
static uint32_t acm_r5_lh = 0x280;
static uint32_t acm_r6_hh = 0x37f;
static uint32_t acm_r6_lh = 0x300;

static uint32_t acm_hue_param01 = 0x200 << 16 | 0x200;
static uint32_t acm_hue_param23 = 0x1FC << 16 | 0x200;
static uint32_t acm_hue_param45 = 0x204 << 16 | 0x200;
static uint32_t acm_hue_param67 = 0x200 << 16 | 0x200;
static uint32_t acm_hue_smooth0 = 0x0040003F;
static uint32_t acm_hue_smooth1 = 0x00C000BF;
static uint32_t acm_hue_smooth2 = 0x0140013F;
static uint32_t acm_hue_smooth3 = 0x01C001BF;
static uint32_t acm_hue_smooth4 = 0x02410240;
static uint32_t acm_hue_smooth5 = 0x02C102C0;
static uint32_t acm_hue_smooth6 = 0x0340033F;
static uint32_t acm_hue_smooth7 = 0x03C003BF;
static uint32_t acm_color_choose = 1;
static uint32_t acm_l_cont_en = 0;
static uint32_t acm_lc_param01 = 0x020401FC;
static uint32_t acm_lc_param23 = 0x02000200;
static uint32_t acm_lc_param45 = 0x020801F8;
static uint32_t acm_lc_param67 = 0x020401FC;
static uint32_t acm_l_adj_ctrl = 0;
static uint32_t acm_capture_ctrl = 0;
static uint32_t acm_capture_in = 0;
static uint32_t acm_capture_out = 0;
static uint32_t acm_ink_ctrl = 0;
static uint32_t acm_ink_out = 0;


static u32 acm_lut_hue_table[] = {
	  0,    4,    8,   12,   16,   20,   24,   28,   32,   36,   40,
	 44,   48,   52,   56,   60,   64,   68,   72,   76,   80,   84,
	 88,   92,   96,  100,  104,  108,  112,  116,  120,  124,  128,
	132,  136,  140,  144,  148,  152,  156,  160,  164,  168,  172,
	176,  180,  184,  188,  192,  196,  200,  204,  208,  212,  216,
	220,  224,  228,  232,  236,  240,  244,  248,  252,  256,  260,
	264,  268,  272,  276,  280,  284,  288,  292,  296,  300,  304,
	308,  312,  316,  320,  324,  328,  332,  336,  340,  344,  348,
	352,  356,  360,  364,  368,  372,  376,  380,  384,  388,  392,
	396,  400,  404,  408,  412,  416,  420,  424,  428,  432,  436,
	440,  444,  448,  452,  456,  460,  464,  468,  472,  476,  480,
	484,  488,  492,  496,  500,  504,  508,  512,  516,  520,  524,
	528,  532,  536,  540,  544,  548,  552,  556,  560,  564,  568,
	572,  576,  580,  584,  588,  592,  596,  600,  604,  608,  612,
	616,  620,  624,  628,  632,  636,  640,  644,  648,  652,  656,
	660,  664,  668,  672,  676,  680,  684,  688,  692,  696,  700,
	704,  708,  712,  716,  720,  724,  728,  732,  736,  740,  744,
	748,  752,  756,  760,  764,  768,  772,  776,  780,  784,  788,
	792,  796,  800,  804,  808,  812,  816,  820,  824,  828,  832,
	836,  840,  844,  848,  852,  856,  860,  864,  868,  872,  876,
	880,  884,  888,  892,  896,  900,  904,  908,  912,  916,  920,
	924,  928,  932,  936,  940,  944,  948,  952,  956,  960,  964,
	968,  972,  976,  980,  984,  988,  992,  996, 1000, 1004, 1008,
	1012, 1016, 1020
};
static u32 acm_lut_sata_table[] = {
	 5,  4,  4,  4,  3,  2,  2,  2,  1,  1,  1,  1,  0,  0,  0,  0,  0,  0,  1,  2,
	 2,  2,  3,  4,  4,  6,  7,  8, 10, 12, 13, 14, 16, 18, 21, 23, 26, 28, 30, 33,
	35, 37, 38, 40, 42, 43, 45, 46, 48, 49, 50, 51, 52, 53, 54, 55, 56, 56, 57, 57,
	58, 58, 58, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59,
	59, 59, 59, 59, 58, 58, 58, 58, 58, 57, 56, 55, 54, 52, 51, 50, 49, 48, 48, 47,
	46, 45, 44, 44, 43, 43, 42, 42, 42, 41, 41, 40, 40, 40, 40, 41, 41, 41, 42, 42,
	42, 42, 42, 41, 41, 41, 40, 40, 40, 41, 42, 42, 43, 44, 44, 45, 46, 47, 48, 49,
	50, 50, 51, 52, 53, 54, 54, 55, 56, 56, 57, 57, 58, 57, 56, 56, 55, 54, 54, 53,
	52, 52, 51, 50, 50, 50, 49, 48, 48, 47, 46, 45, 44, 44, 43, 42, 41, 40, 40, 39,
	38, 37, 36, 36, 35, 35, 35, 35, 34, 34, 34, 34, 34, 34, 34, 33, 33, 33, 32, 32,
	32, 32, 31, 31, 30, 30, 30, 29, 29, 29, 28, 28, 28, 28, 28, 27, 27, 26, 26, 25,
	24, 23, 22, 22, 21, 21, 20, 20, 20, 19, 19, 18, 18, 18, 17, 17, 16, 16, 16, 15,
	15, 15, 14, 14, 14, 13, 13, 12, 12, 11, 10,  9,  8,  8,  7,  6
};
static u32 acm_lut_satr0_table[] = {
	 0,  2,  3,  5,  6,  8, 10, 11, 13, 14,
	16, 16, 17, 17, 18, 18, 18, 19, 19, 20,
	20, 21, 21, 22, 22, 23, 23, 23, 24, 24,
	25, 25, 25, 25, 24, 24, 24, 24, 23, 23,
	23, 23, 22, 22, 22, 22, 21, 21, 21, 21,
	20, 20, 20, 20, 18, 16, 14, 12, 10,  8,
	 6,  4,  2,  0
};
static u32 acm_lut_satr1_table[] = {
	 0,  0,  0,  4,  7, 11, 15, 19, 25, 30,
	35, 39, 41, 44, 46, 48, 50, 53, 55, 57,
	59, 62, 64, 66, 68, 71, 73, 75, 77, 80,
	82, 84, 83, 81, 80, 78, 77, 75, 74, 72,
	71, 69, 68, 67, 65, 64, 62, 61, 59, 58,
	56, 49, 42, 35, 30, 25, 20, 15, 11,  7,
	 4,  0,  0,  0
};
static u32 acm_lut_satr2_table[] = {
	 0,  0,  0,  0,  0,  3,  9, 14, 18, 23,
	27, 32, 36, 41, 44, 48, 51, 56, 59, 62,
	65, 66, 69, 71, 72, 74, 75, 77, 77, 77,
	77, 77, 75, 75, 74, 72, 71, 68, 66, 63,
	60, 57, 54, 50, 47, 42, 38, 35, 30, 26,
	21, 15, 11,  6,  2,  0,  0,  0,  0,  0,
	 0,  0,  0,  0
};
static u32 acm_lut_satr3_table[] = {
	 0,  0,  0,  0,  0,  0,  0,  3,  7,  9,
	13, 16, 19, 22, 25, 28, 30, 34, 36, 39,
	41, 43, 45, 46, 48, 49, 50, 51, 53, 54,
	54, 54, 54, 54, 53, 53, 51, 50, 49, 47,
	46, 44, 42, 40, 38, 35, 33, 29, 26, 24,
	21, 18, 15, 11, 9, 7, 5, 3, 0, 0,
	 0, 0, 0, 0
};
static u32 acm_lut_satr4_table[] = {
	 0,  0,  0,  0,  0,  0,  0,  3,  7,  9,
	13, 16, 19, 22, 25, 28, 30, 34, 36, 39,
	41, 43, 45, 46, 48, 49, 50, 51, 53, 54,
	54, 54, 54, 54, 53, 53, 51, 50, 49, 47,
	46, 44, 42, 40, 38, 35, 33, 29, 26, 24,
	21, 18, 15, 11,  9,  7,  5,  3,  0,  0,
	 0,  0,  0,  0
};
static u32 acm_lut_satr5_table[] = {
	 0,  0,  0,  0,  0,  0,  0,  2,  6,  9,
	13, 16, 19, 22, 25, 28, 30, 34, 36, 39,
	41, 43, 45, 46, 48, 49, 50, 51, 53, 54,
	54, 54, 54, 54, 53, 53, 51, 50, 49, 47,
	46, 44, 42, 40, 38, 35, 33, 29, 26, 24,
	21, 18, 15, 11,  7,  4,  1,  0,  0,  0,
	 0,  0,  0,  0
};
static u32 acm_lut_satr6_table[] = {
	 0,  0,  0,  5, 10, 16, 22, 28, 33, 39,
	46, 50, 51, 52, 53, 54, 54, 55, 56, 56,
	57, 58, 59, 59, 60, 61, 61, 62, 63, 64,
	64, 65, 64, 63, 63, 62, 61, 60, 59, 58,
	58, 57, 56, 55, 54, 54, 53, 52, 51, 50,
	49, 48, 47, 43, 38, 33, 28, 21, 15, 10,
	 5,  0,  0,  0
};
static u32 acm_lut_satr7_table[] = {
	 0,  4,  7, 11, 15, 18, 22, 26, 30, 33,
	37, 38, 38, 39, 40, 40, 41, 42, 42, 43,
	44, 44, 45, 46, 46, 47, 48, 48, 49, 50,
	50, 51, 50, 50, 49, 48, 48, 47, 47, 46,
	45, 45, 44, 43, 43, 42, 41, 41, 40, 40,
	39, 38, 38, 37, 33, 30, 26, 22, 18, 15,
	11,  7,  4,  0
};
static u32 gmp_lut_table_low32bit[9][9][9] = {
    {
        {0x00000000, 0x00000200, 0x00000400, 0x00000600, 0x00000800, 0x00000a00, 0x00000c00, 0x00000e00, 0x00000ff0, },
        {0x00200000, 0x00200200, 0x00200400, 0x00200600, 0x00200800, 0x00200a00, 0x00200c00, 0x00200e00, 0x00200ff0, },
        {0x00400000, 0x00400200, 0x00400400, 0x00400600, 0x00400800, 0x00400a00, 0x00400c00, 0x00400e00, 0x00400ff0, },
        {0x00600000, 0x00600200, 0x00600400, 0x00600600, 0x00600800, 0x00600a00, 0x00600c00, 0x00600e00, 0x00600ff0, },
        {0x00800000, 0x00800200, 0x00800400, 0x00800600, 0x00800800, 0x00800a00, 0x00800c00, 0x00800e00, 0x00800ff0, },
        {0x00a00000, 0x00a00200, 0x00a00400, 0x00a00600, 0x00a00800, 0x00a00a00, 0x00a00c00, 0x00a00e00, 0x00a00ff0, },
        {0x00c00000, 0x00c00200, 0x00c00400, 0x00c00600, 0x00c00800, 0x00c00a00, 0x00c00c00, 0x00c00e00, 0x00c00ff0, },
        {0x00e00000, 0x00e00200, 0x00e00400, 0x00e00600, 0x00e00800, 0x00e00a00, 0x00e00c00, 0x00e00e00, 0x00e00ff0, },
        {0x00ff0000, 0x00ff0200, 0x00ff0400, 0x00ff0600, 0x00ff0800, 0x00ff0a00, 0x00ff0c00, 0x00ff0e00, 0x00ff0ff0, },
    },
    {
        {0x00000000, 0x00000200, 0x00000400, 0x00000600, 0x00000800, 0x00000a00, 0x00000c00, 0x00000e00, 0x00000ff0, },
        {0x00200000, 0x00200200, 0x00200400, 0x00200600, 0x00200800, 0x00200a00, 0x00200c00, 0x00200e00, 0x00200ff0, },
        {0x00400000, 0x00400200, 0x00400400, 0x00400600, 0x00400800, 0x00400a00, 0x00400c00, 0x00400e00, 0x00400ff0, },
        {0x00600000, 0x00600200, 0x00600400, 0x00600600, 0x00600800, 0x00600a00, 0x00600c00, 0x00600e00, 0x00600ff0, },
        {0x00800000, 0x00800200, 0x00800400, 0x00800600, 0x00800800, 0x00800a00, 0x00800c00, 0x00800e00, 0x00800ff0, },
        {0x00a00000, 0x00a00200, 0x00a00400, 0x00a00600, 0x00a00800, 0x00a00a00, 0x00a00c00, 0x00a00e00, 0x00a00ff0, },
        {0x00c00000, 0x00c00200, 0x00c00400, 0x00c00600, 0x00c00800, 0x00c00a00, 0x00c00c00, 0x00c00e00, 0x00c00ff0, },
        {0x00e00000, 0x00e00200, 0x00e00400, 0x00e00600, 0x00e00800, 0x00e00a00, 0x00e00c00, 0x00e00e00, 0x00e00ff0, },
        {0x00ff0000, 0x00ff0200, 0x00ff0400, 0x00ff0600, 0x00ff0800, 0x00ff0a00, 0x00ff0c00, 0x00ff0e00, 0x00ff0ff0, },
    },
    {
        {0x00000000, 0x00000200, 0x00000400, 0x00000600, 0x00000800, 0x00000a00, 0x00000c00, 0x00000e00, 0x00000ff0, },
        {0x00200000, 0x00200200, 0x00200400, 0x00200600, 0x00200800, 0x00200a00, 0x00200c00, 0x00200e00, 0x00200ff0, },
        {0x00400000, 0x00400200, 0x00400400, 0x00400600, 0x00400800, 0x00400a00, 0x00400c00, 0x00400e00, 0x00400ff0, },
        {0x00600000, 0x00600200, 0x00600400, 0x00600600, 0x00600800, 0x00600a00, 0x00600c00, 0x00600e00, 0x00600ff0, },
        {0x00800000, 0x00800200, 0x00800400, 0x00800600, 0x00800800, 0x00800a00, 0x00800c00, 0x00800e00, 0x00800ff0, },
        {0x00a00000, 0x00a00200, 0x00a00400, 0x00a00600, 0x00a00800, 0x00a00a00, 0x00a00c00, 0x00a00e00, 0x00a00ff0, },
        {0x00c00000, 0x00c00200, 0x00c00400, 0x00c00600, 0x00c00800, 0x00c00a00, 0x00c00c00, 0x00c00e00, 0x00c00ff0, },
        {0x00e00000, 0x00e00200, 0x00e00400, 0x00e00600, 0x00e00800, 0x00e00a00, 0x00e00c00, 0x00e00e00, 0x00e00ff0, },
        {0x00ff0000, 0x00ff0200, 0x00ff0400, 0x00ff0600, 0x00ff0800, 0x00ff0a00, 0x00ff0c00, 0x00ff0e00, 0x00ff0ff0, },
    },
    {
        {0x00000000, 0x00000200, 0x00000400, 0x00000600, 0x00000800, 0x00000a00, 0x00000c00, 0x00000e00, 0x00000ff0, },
        {0x00200000, 0x00200200, 0x00200400, 0x00200600, 0x00200800, 0x00200a00, 0x00200c00, 0x00200e00, 0x00200ff0, },
        {0x00400000, 0x00400200, 0x00400400, 0x00400600, 0x00400800, 0x00400a00, 0x00400c00, 0x00400e00, 0x00400ff0, },
        {0x00600000, 0x00600200, 0x00600400, 0x00600600, 0x00600800, 0x00600a00, 0x00600c00, 0x00600e00, 0x00600ff0, },
        {0x00800000, 0x00800200, 0x00800400, 0x00800600, 0x00800800, 0x00800a00, 0x00800c00, 0x00800e00, 0x00800ff0, },
        {0x00a00000, 0x00a00200, 0x00a00400, 0x00a00600, 0x00a00800, 0x00a00a00, 0x00a00c00, 0x00a00e00, 0x00a00ff0, },
        {0x00c00000, 0x00c00200, 0x00c00400, 0x00c00600, 0x00c00800, 0x00c00a00, 0x00c00c00, 0x00c00e00, 0x00c00ff0, },
        {0x00e00000, 0x00e00200, 0x00e00400, 0x00e00600, 0x00e00800, 0x00e00a00, 0x00e00c00, 0x00e00e00, 0x00e00ff0, },
        {0x00ff0000, 0x00ff0200, 0x00ff0400, 0x00ff0600, 0x00ff0800, 0x00ff0a00, 0x00ff0c00, 0x00ff0e00, 0x00ff0ff0, },
    },
    {
        {0x00000000, 0x00000200, 0x00000400, 0x00000600, 0x00000800, 0x00000a00, 0x00000c00, 0x00000e00, 0x00000ff0, },
        {0x00200000, 0x00200200, 0x00200400, 0x00200600, 0x00200800, 0x00200a00, 0x00200c00, 0x00200e00, 0x00200ff0, },
        {0x00400000, 0x00400200, 0x00400400, 0x00400600, 0x00400800, 0x00400a00, 0x00400c00, 0x00400e00, 0x00400ff0, },
        {0x00600000, 0x00600200, 0x00600400, 0x00600600, 0x00600800, 0x00600a00, 0x00600c00, 0x00600e00, 0x00600ff0, },
        {0x00800000, 0x00800200, 0x00800400, 0x00800600, 0x00800800, 0x00800a00, 0x00800c00, 0x00800e00, 0x00800ff0, },
        {0x00a00000, 0x00a00200, 0x00a00400, 0x00a00600, 0x00a00800, 0x00a00a00, 0x00a00c00, 0x00a00e00, 0x00a00ff0, },
        {0x00c00000, 0x00c00200, 0x00c00400, 0x00c00600, 0x00c00800, 0x00c00a00, 0x00c00c00, 0x00c00e00, 0x00c00ff0, },
        {0x00e00000, 0x00e00200, 0x00e00400, 0x00e00600, 0x00e00800, 0x00e00a00, 0x00e00c00, 0x00e00e00, 0x00e00ff0, },
        {0x00ff0000, 0x00ff0200, 0x00ff0400, 0x00ff0600, 0x00ff0800, 0x00ff0a00, 0x00ff0c00, 0x00ff0e00, 0x00ff0ff0, },
    },
    {
        {0x00000000, 0x00000200, 0x00000400, 0x00000600, 0x00000800, 0x00000a00, 0x00000c00, 0x00000e00, 0x00000ff0, },
        {0x00200000, 0x00200200, 0x00200400, 0x00200600, 0x00200800, 0x00200a00, 0x00200c00, 0x00200e00, 0x00200ff0, },
        {0x00400000, 0x00400200, 0x00400400, 0x00400600, 0x00400800, 0x00400a00, 0x00400c00, 0x00400e00, 0x00400ff0, },
        {0x00600000, 0x00600200, 0x00600400, 0x00600600, 0x00600800, 0x00600a00, 0x00600c00, 0x00600e00, 0x00600ff0, },
        {0x00800000, 0x00800200, 0x00800400, 0x00800600, 0x00800800, 0x00800a00, 0x00800c00, 0x00800e00, 0x00800ff0, },
        {0x00a00000, 0x00a00200, 0x00a00400, 0x00a00600, 0x00a00800, 0x00a00a00, 0x00a00c00, 0x00a00e00, 0x00a00ff0, },
        {0x00c00000, 0x00c00200, 0x00c00400, 0x00c00600, 0x00c00800, 0x00c00a00, 0x00c00c00, 0x00c00e00, 0x00c00ff0, },
        {0x00e00000, 0x00e00200, 0x00e00400, 0x00e00600, 0x00e00800, 0x00e00a00, 0x00e00c00, 0x00e00e00, 0x00e00ff0, },
        {0x00ff0000, 0x00ff0200, 0x00ff0400, 0x00ff0600, 0x00ff0800, 0x00ff0a00, 0x00ff0c00, 0x00ff0e00, 0x00ff0ff0, },
    },
    {
        {0x00000000, 0x00000200, 0x00000400, 0x00000600, 0x00000800, 0x00000a00, 0x00000c00, 0x00000e00, 0x00000ff0, },
        {0x00200000, 0x00200200, 0x00200400, 0x00200600, 0x00200800, 0x00200a00, 0x00200c00, 0x00200e00, 0x00200ff0, },
        {0x00400000, 0x00400200, 0x00400400, 0x00400600, 0x00400800, 0x00400a00, 0x00400c00, 0x00400e00, 0x00400ff0, },
        {0x00600000, 0x00600200, 0x00600400, 0x00600600, 0x00600800, 0x00600a00, 0x00600c00, 0x00600e00, 0x00600ff0, },
        {0x00800000, 0x00800200, 0x00800400, 0x00800600, 0x00800800, 0x00800a00, 0x00800c00, 0x00800e00, 0x00800ff0, },
        {0x00a00000, 0x00a00200, 0x00a00400, 0x00a00600, 0x00a00800, 0x00a00a00, 0x00a00c00, 0x00a00e00, 0x00a00ff0, },
        {0x00c00000, 0x00c00200, 0x00c00400, 0x00c00600, 0x00c00800, 0x00c00a00, 0x00c00c00, 0x00c00e00, 0x00c00ff0, },
        {0x00e00000, 0x00e00200, 0x00e00400, 0x00e00600, 0x00e00800, 0x00e00a00, 0x00e00c00, 0x00e00e00, 0x00e00ff0, },
        {0x00ff0000, 0x00ff0200, 0x00ff0400, 0x00ff0600, 0x00ff0800, 0x00ff0a00, 0x00ff0c00, 0x00ff0e00, 0x00ff0ff0, },
    },
    {
        {0x00000000, 0x00000200, 0x00000400, 0x00000600, 0x00000800, 0x00000a00, 0x00000c00, 0x00000e00, 0x00000ff0, },
        {0x00200000, 0x00200200, 0x00200400, 0x00200600, 0x00200800, 0x00200a00, 0x00200c00, 0x00200e00, 0x00200ff0, },
        {0x00400000, 0x00400200, 0x00400400, 0x00400600, 0x00400800, 0x00400a00, 0x00400c00, 0x00400e00, 0x00400ff0, },
        {0x00600000, 0x00600200, 0x00600400, 0x00600600, 0x00600800, 0x00600a00, 0x00600c00, 0x00600e00, 0x00600ff0, },
        {0x00800000, 0x00800200, 0x00800400, 0x00800600, 0x00800800, 0x00800a00, 0x00800c00, 0x00800e00, 0x00800ff0, },
        {0x00a00000, 0x00a00200, 0x00a00400, 0x00a00600, 0x00a00800, 0x00a00a00, 0x00a00c00, 0x00a00e00, 0x00a00ff0, },
        {0x00c00000, 0x00c00200, 0x00c00400, 0x00c00600, 0x00c00800, 0x00c00a00, 0x00c00c00, 0x00c00e00, 0x00c00ff0, },
        {0x00e00000, 0x00e00200, 0x00e00400, 0x00e00600, 0x00e00800, 0x00e00a00, 0x00e00c00, 0x00e00e00, 0x00e00ff0, },
        {0x00ff0000, 0x00ff0200, 0x00ff0400, 0x00ff0600, 0x00ff0800, 0x00ff0a00, 0x00ff0c00, 0x00ff0e00, 0x00ff0ff0, },
    },
    {
        {0xf0000000, 0xf0000200, 0xf0000400, 0xf0000600, 0xf0000800, 0xf0000a00, 0xf0000c00, 0xf0000e00, 0xf0000ff0, },
        {0xf0200000, 0xf0200200, 0xf0200400, 0xf0200600, 0xf0200800, 0xf0200a00, 0xf0200c00, 0xf0200e00, 0xf0200ff0, },
        {0xf0400000, 0xf0400200, 0xf0400400, 0xf0400600, 0xf0400800, 0xf0400a00, 0xf0400c00, 0xf0400e00, 0xf0400ff0, },
        {0xf0600000, 0xf0600200, 0xf0600400, 0xf0600600, 0xf0600800, 0xf0600a00, 0xf0600c00, 0xf0600e00, 0xf0600ff0, },
        {0xf0800000, 0xf0800200, 0xf0800400, 0xf0800600, 0xf0800800, 0xf0800a00, 0xf0800c00, 0xf0800e00, 0xf0800ff0, },
        {0xf0a00000, 0xf0a00200, 0xf0a00400, 0xf0a00600, 0xf0a00800, 0xf0a00a00, 0xf0a00c00, 0xf0a00e00, 0xf0a00ff0, },
        {0xf0c00000, 0xf0c00200, 0xf0c00400, 0xf0c00600, 0xf0c00800, 0xf0c00a00, 0xf0c00c00, 0xf0c00e00, 0xf0c00ff0, },
        {0xf0e00000, 0xf0e00200, 0xf0e00400, 0xf0e00600, 0xf0e00800, 0xf0e00a00, 0xf0e00c00, 0xf0e00e00, 0xf0e00ff0, },
        {0xf0ff0000, 0xf0ff0200, 0xf0ff0400, 0xf0ff0600, 0xf0ff0800, 0xf0ff0a00, 0xf0ff0c00, 0xf0ff0e00, 0xf0ff0ff0, },
    },
};
static u32 gmp_lut_table_high4bit[9][9][9] = {
    {
        {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, },
        {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, },
        {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, },
        {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, },
        {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, },
        {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, },
        {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, },
        {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, },
        {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, },
    },
    {
        {0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, },
        {0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, },
        {0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, },
        {0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, },
        {0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, },
        {0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, },
        {0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, },
        {0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, },
        {0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, },
    },
    {
        {0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, },
        {0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, },
        {0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, },
        {0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, },
        {0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, },
        {0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, },
        {0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, },
        {0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, },
        {0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, },
    },
    {
        {0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, },
        {0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, },
        {0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, },
        {0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, },
        {0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, },
        {0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, },
        {0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, },
        {0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, },
        {0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, },
    },
    {
        {0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, },
        {0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, },
        {0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, },
        {0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, },
        {0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, },
        {0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, },
        {0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, },
        {0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, },
        {0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, },
    },
    {
        {0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, },
        {0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, },
        {0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, },
        {0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, },
        {0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, },
        {0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, },
        {0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, },
        {0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, },
        {0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, },
    },
    {
        {0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, },
        {0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, },
        {0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, },
        {0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, },
        {0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, },
        {0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, },
        {0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, },
        {0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, },
        {0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, },
    },
    {
        {0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, },
        {0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, },
        {0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, },
        {0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, },
        {0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, },
        {0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, },
        {0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, },
        {0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, },
        {0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, },
    },
    {
        {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, },
        {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, },
        {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, },
        {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, },
        {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, },
        {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, },
        {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, },
        {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, },
        {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, },
    },
};

/*******************************************************************************
** GAMMA
*/
static u32 gamma_lut_table_R[] = {
	  0,	 16,	 32,	 48,	 64,	 80,	 96,	112,	128,	144,
	160,	176,	192,	208,	224,	240,	256,	272,	288,	304,
	320,	336,	352,	368,	384,	400,	416,	432,	448,	464,
	480,	496,	512,	528,	544,	560,	576,	592,	608,	624,
	640,	656,	672,	688,	704,	720,	736,	752,	768,	784,
	800,	816,	832,	848,	864,	880,	896,	912,	928,	944,
	960,	976,	992,	1008,	1024,	1040,	1056,	1072,	1088,	1104,
	1120,	1136,	1152,	1168,	1184,	1200,	1216,	1232,	1248,	1264,
	1280,	1296,	1312,	1328,	1344,	1360,	1376,	1392,	1408,	1424,
	1440,	1456,	1472,	1488,	1504,	1520,	1536,	1552,	1568,	1584,
	1600,	1616,	1632,	1648,	1664,	1680,	1696,	1712,	1728,	1744,
	1760,	1776,	1792,	1808,	1824,	1840,	1856,	1872,	1888,	1904,
	1920,	1936,	1952,	1968,	1984,	2000,	2016,	2032,	2048,	2064,
	2080,	2096,	2112,	2128,	2144,	2160,	2176,	2192,	2208,	2224,
	2240,	2256,	2272,	2288,	2304,	2320,	2336,	2352,	2368,	2384,
	2400,	2416,	2432,	2448,	2464,	2480,	2496,	2512,	2528,	2544,
	2560,	2576,	2592,	2608,	2624,	2640,	2656,	2672,	2688,	2704,
	2720,	2736,	2752,	2768,	2784,	2800,	2816,	2832,	2848,	2864,
	2880,	2896,	2912,	2928,	2944,	2960,	2976,	2992,	3008,	3024,
	3040,	3056,	3072,	3088,	3104,	3120,	3136,	3152,	3168,	3184,
	3200,	3216,	3232,	3248,	3264,	3280,	3296,	3312,	3328,	3344,
	3360,	3376,	3392,	3408,	3424,	3440,	3456,	3472,	3488,	3504,
	3520,	3536,	3552,	3568,	3584,	3600,	3616,	3632,	3648,	3664,
	3680,	3696,	3712,	3728,	3744,	3760,	3776,	3792,	3808,	3824,
	3840,	3856,	3872,	3888,	3904,	3920,	3936,	3952,	3968,	3984,
	4000,	4016,	4032,	4048,	4064,	4080,	4095
};
static u32 gamma_lut_table_G[] = {
	  0,	 16,	 32,	 48,	 64,	 80,	 96,	112,	128,	144,
	160,	176,	192,	208,	224,	240,	256,	272,	288,	304,
	320,	336,	352,	368,	384,	400,	416,	432,	448,	464,
	480,	496,	512,	528,	544,	560,	576,	592,	608,	624,
	640,	656,	672,	688,	704,	720,	736,	752,	768,	784,
	800,	816,	832,	848,	864,	880,	896,	912,	928,	944,
	960,	976,	992,	1008,	1024,	1040,	1056,	1072,	1088,	1104,
	1120,	1136,	1152,	1168,	1184,	1200,	1216,	1232,	1248,	1264,
	1280,	1296,	1312,	1328,	1344,	1360,	1376,	1392,	1408,	1424,
	1440,	1456,	1472,	1488,	1504,	1520,	1536,	1552,	1568,	1584,
	1600,	1616,	1632,	1648,	1664,	1680,	1696,	1712,	1728,	1744,
	1760,	1776,	1792,	1808,	1824,	1840,	1856,	1872,	1888,	1904,
	1920,	1936,	1952,	1968,	1984,	2000,	2016,	2032,	2048,	2064,
	2080,	2096,	2112,	2128,	2144,	2160,	2176,	2192,	2208,	2224,
	2240,	2256,	2272,	2288,	2304,	2320,	2336,	2352,	2368,	2384,
	2400,	2416,	2432,	2448,	2464,	2480,	2496,	2512,	2528,	2544,
	2560,	2576,	2592,	2608,	2624,	2640,	2656,	2672,	2688,	2704,
	2720,	2736,	2752,	2768,	2784,	2800,	2816,	2832,	2848,	2864,
	2880,	2896,	2912,	2928,	2944,	2960,	2976,	2992,	3008,	3024,
	3040,	3056,	3072,	3088,	3104,	3120,	3136,	3152,	3168,	3184,
	3200,	3216,	3232,	3248,	3264,	3280,	3296,	3312,	3328,	3344,
	3360,	3376,	3392,	3408,	3424,	3440,	3456,	3472,	3488,	3504,
	3520,	3536,	3552,	3568,	3584,	3600,	3616,	3632,	3648,	3664,
	3680,	3696,	3712,	3728,	3744,	3760,	3776,	3792,	3808,	3824,
	3840,	3856,	3872,	3888,	3904,	3920,	3936,	3952,	3968,	3984,
	4000,	4016,	4032,	4048,	4064,	4080,	4095
};
static u32 gamma_lut_table_B[] = {
	  0,	 16,	 32,	 48,	 64,	 80,	 96,	112,	128,	144,
	160,	176,	192,	208,	224,	240,	256,	272,	288,	304,
	320,	336,	352,	368,	384,	400,	416,	432,	448,	464,
	480,	496,	512,	528,	544,	560,	576,	592,	608,	624,
	640,	656,	672,	688,	704,	720,	736,	752,	768,	784,
	800,	816,	832,	848,	864,	880,	896,	912,	928,	944,
	960,	976,	992,	1008,	1024,	1040,	1056,	1072,	1088,	1104,
	1120,	1136,	1152,	1168,	1184,	1200,	1216,	1232,	1248,	1264,
	1280,	1296,	1312,	1328,	1344,	1360,	1376,	1392,	1408,	1424,
	1440,	1456,	1472,	1488,	1504,	1520,	1536,	1552,	1568,	1584,
	1600,	1616,	1632,	1648,	1664,	1680,	1696,	1712,	1728,	1744,
	1760,	1776,	1792,	1808,	1824,	1840,	1856,	1872,	1888,	1904,
	1920,	1936,	1952,	1968,	1984,	2000,	2016,	2032,	2048,	2064,
	2080,	2096,	2112,	2128,	2144,	2160,	2176,	2192,	2208,	2224,
	2240,	2256,	2272,	2288,	2304,	2320,	2336,	2352,	2368,	2384,
	2400,	2416,	2432,	2448,	2464,	2480,	2496,	2512,	2528,	2544,
	2560,	2576,	2592,	2608,	2624,	2640,	2656,	2672,	2688,	2704,
	2720,	2736,	2752,	2768,	2784,	2800,	2816,	2832,	2848,	2864,
	2880,	2896,	2912,	2928,	2944,	2960,	2976,	2992,	3008,	3024,
	3040,	3056,	3072,	3088,	3104,	3120,	3136,	3152,	3168,	3184,
	3200,	3216,	3232,	3248,	3264,	3280,	3296,	3312,	3328,	3344,
	3360,	3376,	3392,	3408,	3424,	3440,	3456,	3472,	3488,	3504,
	3520,	3536,	3552,	3568,	3584,	3600,	3616,	3632,	3648,	3664,
	3680,	3696,	3712,	3728,	3744,	3760,	3776,	3792,	3808,	3824,
	3840,	3856,	3872,	3888,	3904,	3920,	3936,	3952,	3968,	3984,
	4000,	4016,	4032,	4048,	4064,	4080,	4095
};


/*******************************************************************************
** IGM
*/
static u32 igm_lut_table_R[] = {
	  0,	 16,	 32,	 48,	 64,	 80,	 96,	112,	128,	144,
	160,	176,	192,	208,	224,	240,	256,	272,	288,	304,
	320,	336,	352,	368,	384,	400,	416,	432,	448,	464,
	480,	496,	512,	528,	544,	560,	576,	592,	608,	624,
	640,	656,	672,	688,	704,	720,	736,	752,	768,	784,
	800,	816,	832,	848,	864,	880,	896,	912,	928,	944,
	960,	976,	992,	1008,	1024,	1040,	1056,	1072,	1088,	1104,
	1120,	1136,	1152,	1168,	1184,	1200,	1216,	1232,	1248,	1264,
	1280,	1296,	1312,	1328,	1344,	1360,	1376,	1392,	1408,	1424,
	1440,	1456,	1472,	1488,	1504,	1520,	1536,	1552,	1568,	1584,
	1600,	1616,	1632,	1648,	1664,	1680,	1696,	1712,	1728,	1744,
	1760,	1776,	1792,	1808,	1824,	1840,	1856,	1872,	1888,	1904,
	1920,	1936,	1952,	1968,	1984,	2000,	2016,	2032,	2048,	2064,
	2080,	2096,	2112,	2128,	2144,	2160,	2176,	2192,	2208,	2224,
	2240,	2256,	2272,	2288,	2304,	2320,	2336,	2352,	2368,	2384,
	2400,	2416,	2432,	2448,	2464,	2480,	2496,	2512,	2528,	2544,
	2560,	2576,	2592,	2608,	2624,	2640,	2656,	2672,	2688,	2704,
	2720,	2736,	2752,	2768,	2784,	2800,	2816,	2832,	2848,	2864,
	2880,	2896,	2912,	2928,	2944,	2960,	2976,	2992,	3008,	3024,
	3040,	3056,	3072,	3088,	3104,	3120,	3136,	3152,	3168,	3184,
	3200,	3216,	3232,	3248,	3264,	3280,	3296,	3312,	3328,	3344,
	3360,	3376,	3392,	3408,	3424,	3440,	3456,	3472,	3488,	3504,
	3520,	3536,	3552,	3568,	3584,	3600,	3616,	3632,	3648,	3664,
	3680,	3696,	3712,	3728,	3744,	3760,	3776,	3792,	3808,	3824,
	3840,	3856,	3872,	3888,	3904,	3920,	3936,	3952,	3968,	3984,
	4000,	4016,	4032,	4048,	4064,	4080,	4095
};
static u32 igm_lut_table_G[] = {
	  0,	 16,	 32,	 48,	 64,	 80,	 96,	112,	128,	144,
	160,	176,	192,	208,	224,	240,	256,	272,	288,	304,
	320,	336,	352,	368,	384,	400,	416,	432,	448,	464,
	480,	496,	512,	528,	544,	560,	576,	592,	608,	624,
	640,	656,	672,	688,	704,	720,	736,	752,	768,	784,
	800,	816,	832,	848,	864,	880,	896,	912,	928,	944,
	960,	976,	992,	1008,	1024,	1040,	1056,	1072,	1088,	1104,
	1120,	1136,	1152,	1168,	1184,	1200,	1216,	1232,	1248,	1264,
	1280,	1296,	1312,	1328,	1344,	1360,	1376,	1392,	1408,	1424,
	1440,	1456,	1472,	1488,	1504,	1520,	1536,	1552,	1568,	1584,
	1600,	1616,	1632,	1648,	1664,	1680,	1696,	1712,	1728,	1744,
	1760,	1776,	1792,	1808,	1824,	1840,	1856,	1872,	1888,	1904,
	1920,	1936,	1952,	1968,	1984,	2000,	2016,	2032,	2048,	2064,
	2080,	2096,	2112,	2128,	2144,	2160,	2176,	2192,	2208,	2224,
	2240,	2256,	2272,	2288,	2304,	2320,	2336,	2352,	2368,	2384,
	2400,	2416,	2432,	2448,	2464,	2480,	2496,	2512,	2528,	2544,
	2560,	2576,	2592,	2608,	2624,	2640,	2656,	2672,	2688,	2704,
	2720,	2736,	2752,	2768,	2784,	2800,	2816,	2832,	2848,	2864,
	2880,	2896,	2912,	2928,	2944,	2960,	2976,	2992,	3008,	3024,
	3040,	3056,	3072,	3088,	3104,	3120,	3136,	3152,	3168,	3184,
	3200,	3216,	3232,	3248,	3264,	3280,	3296,	3312,	3328,	3344,
	3360,	3376,	3392,	3408,	3424,	3440,	3456,	3472,	3488,	3504,
	3520,	3536,	3552,	3568,	3584,	3600,	3616,	3632,	3648,	3664,
	3680,	3696,	3712,	3728,	3744,	3760,	3776,	3792,	3808,	3824,
	3840,	3856,	3872,	3888,	3904,	3920,	3936,	3952,	3968,	3984,
	4000,	4016,	4032,	4048,	4064,	4080,	4095
};
static u32 igm_lut_table_B[] = {
	  0,	 16,	 32,	 48,	 64,	 80,	 96,	112,	128,	144,
	160,	176,	192,	208,	224,	240,	256,	272,	288,	304,
	320,	336,	352,	368,	384,	400,	416,	432,	448,	464,
	480,	496,	512,	528,	544,	560,	576,	592,	608,	624,
	640,	656,	672,	688,	704,	720,	736,	752,	768,	784,
	800,	816,	832,	848,	864,	880,	896,	912,	928,	944,
	960,	976,	992,	1008,	1024,	1040,	1056,	1072,	1088,	1104,
	1120,	1136,	1152,	1168,	1184,	1200,	1216,	1232,	1248,	1264,
	1280,	1296,	1312,	1328,	1344,	1360,	1376,	1392,	1408,	1424,
	1440,	1456,	1472,	1488,	1504,	1520,	1536,	1552,	1568,	1584,
	1600,	1616,	1632,	1648,	1664,	1680,	1696,	1712,	1728,	1744,
	1760,	1776,	1792,	1808,	1824,	1840,	1856,	1872,	1888,	1904,
	1920,	1936,	1952,	1968,	1984,	2000,	2016,	2032,	2048,	2064,
	2080,	2096,	2112,	2128,	2144,	2160,	2176,	2192,	2208,	2224,
	2240,	2256,	2272,	2288,	2304,	2320,	2336,	2352,	2368,	2384,
	2400,	2416,	2432,	2448,	2464,	2480,	2496,	2512,	2528,	2544,
	2560,	2576,	2592,	2608,	2624,	2640,	2656,	2672,	2688,	2704,
	2720,	2736,	2752,	2768,	2784,	2800,	2816,	2832,	2848,	2864,
	2880,	2896,	2912,	2928,	2944,	2960,	2976,	2992,	3008,	3024,
	3040,	3056,	3072,	3088,	3104,	3120,	3136,	3152,	3168,	3184,
	3200,	3216,	3232,	3248,	3264,	3280,	3296,	3312,	3328,	3344,
	3360,	3376,	3392,	3408,	3424,	3440,	3456,	3472,	3488,	3504,
	3520,	3536,	3552,	3568,	3584,	3600,	3616,	3632,	3648,	3664,
	3680,	3696,	3712,	3728,	3744,	3760,	3776,	3792,	3808,	3824,
	3840,	3856,	3872,	3888,	3904,	3920,	3936,	3952,	3968,	3984,
	4000,	4016,	4032,	4048,	4064,	4080,	4095
};

/*******************************************************************************
** XCC
*/
static u32 xcc_table[12] = {0x0, 0x8000, 0x0,0x0,0x0,0x0,0x8000,0x0,0x0,0x0,0x0,0x8000,};
/*******************************************************************************
**
*/

static uint32_t mipi_check_lcd_reg(char __iomem *mipi_dsi0_base)
{
	uint32_t status;
	uint32_t try_times = 0;

	outp32(DPU_DSI_APB_WR_LP_HDR_ADDR(mipi_dsi0_base), 0x0A06);
	status = inp32(DPU_DSI_CMD_PLD_BUF_STATUS_ADDR(mipi_dsi0_base));
	while (status & 0x10) {
		udelay(50);
		if (++try_times > 100) {
			disp_pr_err("Read lcd power status timeout!\n");
			break;
		}

		status = inp32(DPU_DSI_CMD_PLD_BUF_STATUS_ADDR(mipi_dsi0_base));
	}
	status = inp32(DPU_DSI_APB_WR_LP_PLD_DATA_ADDR(mipi_dsi0_base));

	return status;
}

static int hisi_lg_TD4322_panel_on(uint32_t panel_id, struct platform_device *pdev)
{
	struct hisi_panel_device *panel_data = NULL;
	struct hisi_panel_info *pinfo = NULL;
	struct hisi_connector_info *connector_info = NULL;
	char __iomem *mipi_dsi0_base = NULL;
	uint32_t status;
	uint32_t i;

	disp_check_and_return(!pdev, -EINVAL, err, "pdev is NULL!\n");

	panel_data = platform_get_drvdata(pdev);
	disp_check_and_return(!panel_data, -EINVAL, err, "panel_data is NULL!\n");

	pinfo = &(panel_data->panel_info);

	connector_info = panel_data->connector_info;
	disp_check_and_return(!connector_info, -EINVAL, err, "connector_info is NULL!\n");

	mipi_dsi0_base = connector_info->connector_base;
	disp_check_and_return(!mipi_dsi0_base, -EINVAL, err, "mipi_dsi0_base is NULL!\n");

	disp_pr_info(" ++++");
//	disp_pr_info("dsi, mipi_lg_TD4322_panel_on +.\n");
	//WARN_ON(1);
	if (connector_info->current_step == 0) {
		if (g_lcd_fpga_flag == 0) {
			// TODO:
			/*vcc_cmds_tx(pdev, lcd_vcc_enable_cmds,
				ARRAY_SIZE(lcd_vcc_enable_cmds));
			pinctrl_cmds_tx(pdev, lcd_pinctrl_normal_cmds,
				ARRAY_SIZE(lcd_pinctrl_normal_cmds));
			dpu_gpio_cmds_tx(asic_lcd_gpio_request_cmds, \
				ARRAY_SIZE(asic_lcd_gpio_request_cmds));
			dpu_gpio_cmds_tx(asic_lcd_gpio_normal_cmds, \
				ARRAY_SIZE(asic_lcd_gpio_normal_cmds));*/
		} else {
			disp_pr_info("1 ++++");
			dpu_gpio_cmds_tx(fpga_lcd_gpio_request_cmds, \
				ARRAY_SIZE(fpga_lcd_gpio_request_cmds));
			dpu_gpio_cmds_tx(fpga_lcd_gpio_normal_cmds, \
				ARRAY_SIZE(fpga_lcd_gpio_normal_cmds));
		}
	} else if (connector_info->current_step == 1) {
		disp_pr_info("2 ++++");

		/* Fix that JDI LCD can not enter normal mode */
		for (i = 0; i < 5; i++) {
			dpu_mipi_dsi_cmds_tx(set_display_address, \
				ARRAY_SIZE(set_display_address), mipi_dsi0_base);
			dpu_mipi_dsi_cmds_tx(lcd_display_on_cmds, \
				ARRAY_SIZE(lcd_display_on_cmds), mipi_dsi0_base);

			status = mipi_check_lcd_reg(mipi_dsi0_base);
			disp_pr_info("TD4322 LCD Power State = 0x%x.\n", status);

			if (status & 0x08)
				break;
			else
				mdelay(10);
		}

		if (g_lcd_fpga_flag == 1) {
			dpu_mipi_dsi_cmds_tx(lg_fpga_modify_phyclk_config_cmds, \
				ARRAY_SIZE(lg_fpga_modify_phyclk_config_cmds), mipi_dsi0_base);
		}

		if ((pinfo->bl_info.bl_type & BL_SET_BY_BLPWM) || (pinfo->bl_info.bl_type & BL_SET_BY_SH_BLPWM)) {
			dpu_mipi_dsi_cmds_tx(pwm_out_on_cmds, \
				ARRAY_SIZE(pwm_out_on_cmds), mipi_dsi0_base);
		}

	} else if (connector_info->current_step == 2) {
	} else {
		disp_pr_err("failed to init lcd!\n");
	}
	// TODO: backlight on
	// hisi_lcd_backlight_on(pdev);
	disp_pr_info(" ---- \n");

	return 0;
}

static int hisi_lg_TD4322_panel_off(uint32_t panel_id, struct platform_device *pdev)
{
	struct hisi_panel_device *panel_data = NULL;
	struct hisi_panel_info *pinfo = NULL;
	struct hisi_connector_info *connector_info = NULL;
	char __iomem *mipi_dsi0_base = NULL;

	disp_check_and_return(!pdev, -EINVAL, err, "pdev is NULL!\n");

	panel_data = platform_get_drvdata(pdev);
	disp_check_and_return(!panel_data, -EINVAL, err, "panel_data is NULL!\n");

	pinfo = &(panel_data->panel_info);

	connector_info = panel_data->connector_info;
	disp_check_and_return(!connector_info, -EINVAL, err, "connector_info is NULL!\n");

	mipi_dsi0_base = connector_info->connector_base;
	disp_check_and_return(!mipi_dsi0_base, -EINVAL, err, "mipi_dsi0_base is NULL!\n");;

	disp_pr_info(" enter ++++");

	if (pinfo->lcd_uninit_step == LCD_UNINIT_MIPI_HS_SEND_SEQUENCE) {
		// TODO: baklight off
		//hisi_lcd_backlight_off(pdev);

		dpu_mipi_dsi_cmds_tx(lcd_display_off_cmds, \
			ARRAY_SIZE(lcd_display_off_cmds), mipi_dsi0_base);
		pinfo->lcd_uninit_step = LCD_UNINIT_MIPI_LP_SEND_SEQUENCE;
	} else if (pinfo->lcd_uninit_step == LCD_UNINIT_MIPI_LP_SEND_SEQUENCE) {
		pinfo->lcd_uninit_step = LCD_UNINIT_POWER_OFF;
	} else if (pinfo->lcd_uninit_step == LCD_UNINIT_POWER_OFF) {
		if (g_lcd_fpga_flag==0) {
			// TODO:
			/*dpu_gpio_cmds_tx(asic_lcd_gpio_lowpower_cmds, \
				ARRAY_SIZE(asic_lcd_gpio_lowpower_cmds));
			dpu_gpio_cmds_tx(asic_lcd_gpio_free_cmds, \
				ARRAY_SIZE(asic_lcd_gpio_free_cmds));
			pinctrl_cmds_tx(pdev,lcd_pinctrl_lowpower_cmds,
				ARRAY_SIZE(lcd_pinctrl_lowpower_cmds));
			mdelay(3);
			vcc_cmds_tx(pdev, lcd_vcc_disable_cmds,
				ARRAY_SIZE(lcd_vcc_disable_cmds));*/
		} else {
			dpu_gpio_cmds_tx(fpga_lcd_gpio_lowpower_cmds, \
				ARRAY_SIZE(fpga_lcd_gpio_lowpower_cmds));
			dpu_gpio_cmds_tx(fpga_lcd_gpio_free_cmds, \
				ARRAY_SIZE(fpga_lcd_gpio_free_cmds));
		}
	} else {
		disp_pr_err("failed to uninit lcd!\n");
	}

	disp_pr_debug("dsi -.\n");
	return 0;
}

static int hisi_mipi_lg_TD4322_set_backlight(struct platform_device *pdev, uint32_t bl_level)
{
	int ret = 0;
	static char last_bl_level;
	struct hisi_connector_info *connector_info = NULL;
	struct hisi_panel_device *panel_data = NULL;
	struct hisi_panel_info *pinfo = NULL;
	char __iomem *mipi_dsi0_base = NULL;
	char bl_level_adjust[2] = {
		0x51,
		0x00,
	};

	disp_pr_info(" ++++ \n");
	struct dsi_cmd_desc lcd_bl_level_adjust[] = {
		{DTYPE_DCS_WRITE1, 0, 100, WAIT_TYPE_US,
			sizeof(bl_level_adjust), bl_level_adjust},
	};

	disp_check_and_return(!pdev, -EINVAL, err, "pdev is NULL!\n");

	panel_data = platform_get_drvdata(pdev);
	disp_check_and_return(!panel_data, -EINVAL, err, "panel_data is NULL!\n");

	pinfo = &(panel_data->panel_info);
	connector_info = panel_data->connector_info;
	disp_check_and_return(!connector_info, -EINVAL, err, "connector_info is NULL!\n");

	mipi_dsi0_base = connector_info->connector_base;
	disp_check_and_return(!mipi_dsi0_base, -EINVAL, err, "mipi_dsi0_base is NULL!\n");

	if (pinfo->bl_info.bl_type & BL_SET_BY_MIPI) {
		bl_level_adjust[1] = bl_level * 255 / pinfo->bl_info.bl_max;

		if (last_bl_level != bl_level_adjust[1]) {
			last_bl_level = bl_level_adjust[1];
			dpu_mipi_dsi_cmds_tx(lcd_bl_level_adjust,
				ARRAY_SIZE(lcd_bl_level_adjust),
				mipi_dsi0_base);
		}
		disp_pr_info("bl_level_adjust[1] = %d\n", bl_level_adjust[1]);
	} else {
		disp_pr_info("not support this bl_type %d!\n", pinfo->bl_info.bl_type);
	}

	return 0;
}

static struct hisi_panel_ops g_hisi_lg_TD4322_mipi_panel_ops = {
	.base = {
		.turn_on = hisi_lg_TD4322_panel_on,
		.turn_off = hisi_lg_TD4322_panel_off,
		.present = NULL,
	},

	.bl_ops = {
		.set_backlight = hisi_mipi_lg_TD4322_set_backlight,
	}
};

/*******************************************************************************
**
*/

static int hisi_mipi_lg_TD4322_read_dtsi(struct hisi_panel_info *info)
{
	struct device_node *np = NULL;
	int ret = 0;
	uint32_t bl_type = 0;
	uint32_t lcd_display_type = 0;
	uint32_t lcd_ifbc_type = 0;

	np = of_find_compatible_node(NULL, NULL, DTS_COMP_LG_TD4322);
	if (!np) {
		disp_pr_err("not found device node %s!\n", DTS_COMP_LG_TD4322);
		return -1;
	}

	ret = of_property_read_u32(np, LCD_DISPLAY_TYPE_NAME, &lcd_display_type);
	if (ret) {
		disp_pr_err("get lcd_display_type failed!\n");
		lcd_display_type = PANEL_MIPI_CMD;
	}
#ifdef DSI_1_2_VESA3X_VIDEO
	lcd_display_type = PANEL_MIPI_VIDEO;
#endif

	ret = of_property_read_u32(np, LCD_IFBC_TYPE_NAME, &lcd_ifbc_type);
	if (ret) {
		disp_pr_err("get ifbc_type failed!\n");
		lcd_ifbc_type = IFBC_TYPE_NONE;
	}

	ret = of_property_read_u32(np, LCD_BL_TYPE_NAME, &bl_type);
	if (ret) {
		disp_pr_err("get lcd_bl_type failed!\n");
		bl_type = BL_SET_BY_MIPI;
	}
	disp_pr_info("bl_type=0x%x.", bl_type);

#if 0 /* TODO: probe dependence */
	if (hisi_fb_device_probe_defer(lcd_display_type, bl_type))
		return -EPROBE_DEFER;
#endif

	disp_pr_debug("+.\n");

	ret = of_property_read_u32(np, FPGA_FLAG_NAME, &g_lcd_fpga_flag);//lint !e64
	if (ret) {
		disp_pr_warn("need to get g_lcd_fpga_flag resource in fpga, not needed in asic!\n");
	}

	if (g_lcd_fpga_flag == 1) {
#ifdef CONFIG_DKMD_DPU_V720
		gpio_lcd_p5v5_enable = of_get_named_gpio(np, "gpios", 0);
		gpio_lcd_n5v5_enable = of_get_named_gpio(np, "gpios", 1);
		gpio_lcd_reset = of_get_named_gpio(np, "gpios", 2);
		gpio_lcd_bl_enable = of_get_named_gpio(np, "gpios", 3);
		gpio_lcd_tp1v8 = of_get_named_gpio(np, "gpios", 4);
		g_gpio_lcd_tp2v85 = 33; // for dpuv720, gpio_4[1], 4x8+1=33
#else
		// gpio_lcd_p5v5_enable = of_get_named_gpio(np, "gpios", 0);
		// gpio_lcd_n5v5_enable = of_get_named_gpio(np, "gpios", 1);
		// gpio_lcd_reset = of_get_named_gpio(np, "gpios", 2);
		// gpio_lcd_bl_enable = of_get_named_gpio(np, "gpios", 3);
		// gpio_lcd_tp1v8 = of_get_named_gpio(np, "gpios", 4);
		// 32 55 43 54 33
		gpio_lcd_p5v5_enable = 32;
		gpio_lcd_n5v5_enable = 55;
		gpio_lcd_reset = 43;
		gpio_lcd_bl_enable = 54;
		gpio_lcd_tp1v8 = 33;

		g_gpio_lcd_tp2v85 = 41;
#endif
	} else {
		/*gpio_nums = <+5v:12, -5v:34, rst:194, bl_en:11, ID0:10, ID1:9>;*/
		gpio_lcd_p5v5_enable = of_get_named_gpio(np, "gpios", 0);
		gpio_lcd_n5v5_enable = of_get_named_gpio(np, "gpios", 1);
		gpio_lcd_reset = of_get_named_gpio(np, "gpios", 2);
		gpio_lcd_bl_enable = of_get_named_gpio(np, "gpios", 3);
	}

	disp_pr_info("used gpio:[+5v: %d, -5v: %d, rst: %d, bl_en: %d, tp1v8:%d, tp2v85:%d].\n",gpio_lcd_p5v5_enable, gpio_lcd_n5v5_enable,
		gpio_lcd_reset, gpio_lcd_bl_enable, gpio_lcd_tp1v8, g_gpio_lcd_tp2v85);

	memset(info, 0, sizeof(struct hisi_panel_info));
	info->bl_info.bl_type = bl_type;
	info->type = lcd_display_type;

	return 0;
}

static void hisi_mipi_lg_TD4322_init_panel(struct hisi_panel_info *info)
{
	//init lcd panel info
	info->xres = 1080;
	info->yres = 1920;
	info->width = 75;
	info->height = 133;
	info->orientation = LCD_PORTRAIT;
	info->bpp = LCD_RGB888;
	info->bgr_fmt = LCD_RGB;

#ifdef CONFIG_BACKLIGHT_10000
	info->bl_info.bl_min = 157;
	info->bl_info.bl_max = 9960;
	info->bl_info.bl_default = 4000;
	info->bl_info.blpwm_precision_type = BLPWM_PRECISION_10000_TYPE;
#else
	info->bl_info.bl_min = 1;
	info->bl_info.bl_max = 255;
	info->bl_info.bl_default = 102;
#endif

	info->ifbc_type = IFBC_TYPE_NONE; /* lcd_ifbc_type */
	info->lcd_uninit_step_support = 1;

#if 0 /* TODO */
	info->frc_enable = 0;
	info->esd_enable = 0;
	info->esd_skip_mipi_check = 0;

	info->cinema_mode_support = 0;

	/* init GAMMA LCP info */
	info->gamma_support = 0;
#endif

	if (g_lcd_fpga_flag == 1) {
		//ldi
		info->ldi.h_back_porch = 23;//50;
		info->ldi.h_front_porch = 50;//132;
		info->ldi.h_pulse_width = 20;
		info->ldi.v_back_porch = 12;
		info->ldi.v_front_porch = 14;
		info->ldi.v_pulse_width = 4;
		//mipi
		info->mipi.dsi_bit_clk = 120;
		/* info->dsi_bit_clk_upt_support = 0; */
		info->mipi.dsi_bit_clk_upt = info->mipi.dsi_bit_clk;
		info->timing_info.pxl_clk_rate = 20 * 1000000UL;
	} else {
	/* TODO */
	}

#if 0
	info->gmp_support = 0;
	info->gmp_lut_table_low32bit = &gmp_lut_table_low32bit[0][0][0];
	info->gmp_lut_table_high4bit = &gmp_lut_table_high4bit[0][0][0];
	info->gmp_lut_table_len = ARRAY_SIZE(gmp_lut_table_low32bit);

	info->hiace_support = 0;

	pinfo->acm_support = 0;

	pinfo->arsr1p_sharpness_support = 0;
	//enable arsr2p sharpness
	pinfo->prefix_sharpness2D_support = 0;
#endif

	info->mipi.dsi_version = DSI_1_1_VERSION;

	//non_continue adjust : measured in UI
	//JDI requires clk_post >= 60ns + 252ui, so need a clk_post_adjust more than 200ui. Here 215 is used.
	info->mipi.clk_post_adjust = 215;
	info->mipi.clk_pre_adjust= 0;
	info->mipi.clk_t_hs_prepare_adjust= 0;
	info->mipi.clk_t_lpx_adjust= 0;
	info->mipi.clk_t_hs_trial_adjust= 0;
	info->mipi.clk_t_hs_exit_adjust= 0;
	info->mipi.clk_t_hs_zero_adjust= 0;
	info->mipi.non_continue_en = 1;

	//mipi
	info->mipi.lane_nums = DSI_4_LANES;

	info->mipi.color_mode = DSI_24BITS_1;

	info->mipi.vc = 0;
	info->mipi.max_tx_esc_clk = 10 * 1000000;
	info->mipi.burst_mode = DSI_BURST_SYNC_PULSES_1;
	info->mipi.non_continue_en = 1;

	info->timing_info.pxl_clk_rate_div = 1;

	if (info->type == PANEL_MIPI_CMD) {
		info->vsync_ctrl_type = VSYNC_CTRL_ISR_OFF | VSYNC_CTRL_MIPI_ULPS | VSYNC_CTRL_CLK_OFF;
	}
}

static void hisi_mipi_lg_TD4322_init_connector(struct hisi_connector_info **out_info)
{
	struct hisi_mipi_info *mipi_info = NULL;

	mipi_info = kzalloc(sizeof(*mipi_info), GFP_KERNEL);
	if (!mipi_info)
		return;

	mipi_info->base.power_step_cnt = 3;
	/* other mipi infomation */

	*out_info = (struct hisi_connector_info *)mipi_info;
}

static int mipi_lg_TD4322_probe(struct platform_device *pdev)
{
	struct hisi_panel_device *panel_data = NULL;
	struct hisi_connector_device *connector_dev = NULL;
	int ret = 0;
	disp_pr_err(" ++++ \n");
	panel_data = devm_kzalloc(&pdev->dev, sizeof(*panel_data), GFP_KERNEL);
	if (!panel_data)
		return -1;

	panel_data->pdev = pdev;

	/* read dtsi */
	ret = hisi_mipi_lg_TD4322_read_dtsi(&panel_data->panel_info);
	if (ret)
		return ret;

	/* init panel and mipi information */
	hisi_mipi_lg_TD4322_init_panel(&panel_data->panel_info);
	hisi_mipi_lg_TD4322_init_connector(&panel_data->connector_info);
	platform_set_drvdata(pdev, panel_data);

	hisi_mipi_create_platform_device(panel_data, &g_hisi_lg_TD4322_mipi_panel_ops);

	disp_pr_err(" ---- \n");
	return 0;
}

static const struct of_device_id hisi_panel_match_table[] = {
	{
		.compatible = DTS_COMP_LG_TD4322,
		.data = NULL,
	},
	{},
};
MODULE_DEVICE_TABLE(of, hisi_panel_match_table);

static struct platform_driver this_driver = {
	.probe = mipi_lg_TD4322_probe,
	.remove = NULL,
	.suspend = NULL,
	.resume = NULL,
	.shutdown = NULL,
	.driver = {
		.name = "td4322_panel",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(hisi_panel_match_table),
	},
};

static int __init mipi_lcd_panel_init(void)
{
	int ret = 0;

	disp_pr_err(" ++++ \n");
	ret = platform_driver_register(&this_driver);
	if (ret) {
		disp_pr_err("platform_driver_register failed, error=%d!\n", ret);
		return ret;
	}

	return ret;
}
/*lint -restore*/
module_init(mipi_lcd_panel_init);
