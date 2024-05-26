/* Copyright (c) Hisilicon Technologies Co., Ltd. 2021-2021. All rights reserved.
  * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
 /*lint -e542 -e570*/
#include "dpu_fb.h"
#include <linux/types.h>

#include "mipi_lcd_utils.h"
#include "../spr/dpu_spr.h"
#include "../dsc/dsc_algorithm_manager.h"
#define DTS_COMP_BOE_NT37800A "hisilicon,mipi_boe_nt37800a"

#define AP_SPR_DSC1_2_EN 1
#define TXIP_RXIP_ON 1

static int g_lcd_fpga_flag;

/* Power ON Sequence */
static char g_on_cmd0[] = { /* page0 */
	0xF0, 0x55, 0xAA, 0x52, 0x08, 0x00,
};

static char g_on_cmd1[] = { /* ddic gpio (ex: TE) */
	0xBE, 0x0E, 0x94, 0x93, 0x1F,
};

static char g_on_cmd2[] = { /* shift index, change to next register offset */
	0x6F, 0x0B,
};

static char g_on_cmd3[] = { /* pmic setting, follow anna pulse(74,20,34) */
	0xB5, 0x4A, 0x14, 0x22
};

static char g_on_cmd4[] = { /* page1 */
	0xF0, 0x55, 0xAA, 0x52, 0x08, 0x01,
};

static char g_on_cmd5[] = {
	0xD1, 0x07, 0x02, 0x10,
};

static char g_on_cmd6[] = { /* aod control */
	0xD2, 0x00, 0x40, 0x11,
};

static char g_on_cmd7[] = { /* voltage setting */
	0xB7, 0x1E, 0x1E, 0x14, 0x14, 0x14, 0x14, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E,
};

static char g_on_cmd8[] = { /* voltage setting */
	0xB1, 0x00, 0x00,
};

static char g_on_cmd9[] = { /* voltage setting */
	0xB4, 0x03, 0x03,
};

static char g_on_cmd10[] = { /* voltage setting */
	0xB8, 0x43, 0x43, 0x41, 0x41, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43,
};

static char g_on_cmd11[] = { /* voltage setting */
	0xB5, 0x00, 0xB7, 0x00, 0x87, 0x00, 0x87, 0x00, 0xB7, 0x00, 0x87,
};

static char g_on_cmd12[] = { /* change to page3 */
	0xF0, 0x55, 0xAA, 0x52, 0x08, 0x03,
};

static char g_on_cmd13[] = { /* shift index */
	0x6F, 0x07,
};

static char g_on_cmd14[] = { /* gate eq setting */
	0xB0, 0x00,
};

#ifdef AP_SPR_DSC1_2_EN
/* this code bypass DDIC spr, when enable ap spr + dsc1.2 */
static char g_on_cmd_sprdsc1p2_01[] = {
	0xDE, 0x43,
};

static char g_on_cmd_sprdsc1p2_02[] = {
	0x6F, 0x08,
};

#ifdef TXIP_RXIP_ON
static char g_on_cmd_sprdsc1p2_03[] = {
	0xDE, 0x80,
};

static char g_on_cmd_sprdsc1p2_04[] = {
	0x6F, 0x09,
};

static char g_on_cmd_sprdsc1p2_05[] = {
	0xDE, 0xE4, 0x6C,
};
#else
static char g_on_cmd_sprdsc1p2_03[] = {
	0xDE, 0x00,/* close rxip */
};

static char g_on_cmd_sprdsc1p2_04[] = {
	0x6F, 0x09,
};

static char g_on_cmd_sprdsc1p2_05[] = {
	0xDE, 0xB4, 0x78,
};
#endif


static char g_on_cmd_sprdsc1p2_06[] = {
	0xF0, 0x55, 0xAA, 0x52, 0x08, 0x07,
};

static char g_on_cmd_sprdsc1p2_07[] = {
	0xB0, 0x24,/* close ddic spr */
};

static char g_on_cmd_sprdsc1p2_08[] = {
	0xF0, 0x55, 0xAA, 0x52, 0x08, 0x08,
};

static char g_on_cmd_sprdsc1p2_09[] = {
	0xBE, 0x00,
};
/* this code bypass DDIC spr, when enable ap spr + dsc1.2 */
#endif

/* black screen function: always display even didn't receive complete picture
 * cmd1:0xF0, 0x55, 0xAA, 0x52, 0x08, 0x00
 * cmd2:0xC0, 0xF6
 */

static char g_on_cmd_page1[] = {
	0xF0, 0x55, 0xAA, 0x52, 0x08, 0x01
};


static char g_on_cmd15[] = { /* disable cmd2 */
	0xF0, 0x55, 0xAA, 0x52, 0x00, 0x00
};

static char g_on_cmd16[] = { /* not access */
	0xFF, 0xAA, 0x55, 0xA5, 0x81,
};

static char g_on_cmd17[] = {
	0x6F, 0x0D,
};

static char g_on_cmd18[] = {
	0xF3, 0xCB,
};

static char g_on_cmd19[] = {
	0xFB, 0x20,
};

static char g_on_cmd20[] = {
	0x6F, 0x44,
};

static char g_on_cmd21[] = {
	0xFB, 0x04, 0x04,
};

static char g_on_cmd22[] = {
	0x6F, 0x48,
};

static char g_on_cmd23[] = {
	0xFB, 0x04, 0x04,
};

static char g_on_cmd24[] = {
	0xFF, 0xAA, 0x55, 0xA5, 0x00,
};

static char g_on_cmd25[] = { /* te on */
	0x35, 0x00,
};

static char g_on_cmd26[] = { /* video mode porch setting */
	0x3B, 0x00, 0x10, 0x00, 0x10,
};

static char g_on_cmd27[] = { /* dimming setting */
	0x53, 0x20,
};

static char g_on_cmd28[] = { /* display ram x */
	0x2A, 0x00, 0x00, 0x05, 0x3F,
};

static char g_on_cmd29[] = { /* display ram y */
	0x2B, 0x00, 0x00, 0x0A, 0xD3,
};

static char g_on_cmd30[] = {
	0x82, 0xAB,
};

/* dsc setting */
#ifdef AP_SPR_DSC1_2_EN
static char g_on_cmd31[] = {
	0x91, 0x89, 0xF0, 0x00, 0x2C, 0xF1, 0x55, 0x01, 0xEA, 0x02, 0x61, 0x00, 0x38, 0x02, 0xCB, 0x0A, 0x72, 0x0B, 0xD0,
};

#else
static char g_on_cmd31[] = {
	0x91, 0x89, 0x28, 0x00, 0x2C, 0xC2, 0x00, 0x02, 0x50, 0x04, 0xBA, 0x00, 0x09, 0x02, 0x3C, 0x01, 0xDC, 0x10, 0xF0,
};
#endif

static char g_on_cmd32[] = { /* dsc enable */
	0x90, 0x01,
};

static char g_on_cmd33[] = {
	0x03, 0x01,
};

/* close dsc */
#ifdef CLOSE_DSC
static char g_on_cmd32[] = {
	0x90, 0x02,
};

static char g_on_cmd33[] = {
	0x03, 0x00,
};
#endif


static char g_on_cmd34[] = {
	0x2C,0x00,
};

static char g_on_cmd35[] = { /* frame rate control ,90hz start, then change to 60hz */
	0x2f, 0x02,
};

static char g_on_cmd36[] = { /* sleep out */
	0x11,
};

static char g_on_cmd37[] = { /* frame rate control, 60hz */
	0x2f, 0x01,
};

static char g_on_cmd38[] = {
	0x29,
};


/* bitmode not send 0x11(sleep out), 0x29(display on) command */
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
#endif

static struct dsi_cmd_desc g_change_page1_cmds[] = {
	{ DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd_page1), g_on_cmd_page1},
};

static struct dsi_cmd_desc g_display_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd0), g_on_cmd0},
	{DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd1), g_on_cmd1},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd2), g_on_cmd2},
	{DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd3), g_on_cmd3},
	{DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd4), g_on_cmd4},
	{DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd5), g_on_cmd5},
	{DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd6), g_on_cmd6},
	{DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd7), g_on_cmd7},
	{DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd8), g_on_cmd8},
	{DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd9), g_on_cmd9},
	{DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd10), g_on_cmd10},
	{DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd11), g_on_cmd11},
	{DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd12), g_on_cmd12},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd13), g_on_cmd13},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd14), g_on_cmd14},

#ifdef AP_SPR_DSC1_2_EN
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd_sprdsc1p2_01), g_on_cmd_sprdsc1p2_01},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd_sprdsc1p2_02), g_on_cmd_sprdsc1p2_02},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd_sprdsc1p2_03), g_on_cmd_sprdsc1p2_03},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd_sprdsc1p2_04), g_on_cmd_sprdsc1p2_04},
	{DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd_sprdsc1p2_05), g_on_cmd_sprdsc1p2_05},
	{DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd_sprdsc1p2_06), g_on_cmd_sprdsc1p2_06},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd_sprdsc1p2_07), g_on_cmd_sprdsc1p2_07},
	{DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd_sprdsc1p2_08), g_on_cmd_sprdsc1p2_08},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd_sprdsc1p2_09), g_on_cmd_sprdsc1p2_09},
#endif

	{DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd15), g_on_cmd15},
	{DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd16), g_on_cmd16},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd17), g_on_cmd17},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd18), g_on_cmd18},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd19), g_on_cmd19},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd20), g_on_cmd20},
	{DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd21), g_on_cmd21},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd22), g_on_cmd22},
	{DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd23), g_on_cmd23},
	{DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd24), g_on_cmd24},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd25), g_on_cmd25},
	{DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd26), g_on_cmd26},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd27), g_on_cmd27},
	{DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd28), g_on_cmd28},
	{DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd29), g_on_cmd29},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd30), g_on_cmd30},
	{DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd31), g_on_cmd31},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd32), g_on_cmd32},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd33), g_on_cmd33},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd34), g_on_cmd34},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd35), g_on_cmd35},

#ifdef BIST_MODE
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd37), g_on_cmd37},
	{ DTYPE_DCS_LWRITE, 0, 20, WAIT_TYPE_US, sizeof(g_bist_mode_1), g_bist_mode_1},
	{ DTYPE_DCS_LWRITE, 0, 20, WAIT_TYPE_US, sizeof(g_bist_mode_2), g_bist_mode_2},
	{ DTYPE_DCS_LWRITE, 0, 20, WAIT_TYPE_US, sizeof(g_bist_mode_3), g_bist_mode_3},
#else
	{ DTYPE_DCS_WRITE, 0, 120, WAIT_TYPE_MS, sizeof(g_on_cmd36), g_on_cmd36},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd37), g_on_cmd37},
	{ DTYPE_DCS_WRITE, 0, 120, WAIT_TYPE_MS, sizeof(g_on_cmd38), g_on_cmd38},
#endif
};

/* Power OFF Sequence */
static char g_off_cmd0[] = {
	0x28
};

static char g_off_cmd1[] = {
	0x10
};

static struct dsi_cmd_desc g_lcd_display_off_cmd[] = {
	{ DTYPE_DCS_WRITE, 0, 10, WAIT_TYPE_US, sizeof(g_off_cmd0), g_off_cmd0 },
	{ DTYPE_DCS_WRITE, 0, 100, WAIT_TYPE_MS, sizeof(g_off_cmd1), g_off_cmd1 },
};

/*******************************************************************************
 ** LCD GPIO
 */
#define GPIO_AMOLED_RESET_NAME  "gpio_amoled_reset"
#define GPIO_AMOLED_VCC1V8_NAME "gpio_amoled_vcc1v8"
#define GPIO_AMOLED_VCC3V1_NAME "gpio_amoled_vcc3v1"
#define GPIO_AMOLED_TE0_NAME    "gpio_amoled_te0"

static uint32_t g_gpio_amoled_te0;
static uint32_t g_gpio_amoled_vcc1v8;
static uint32_t g_gpio_amoled_reset;
static uint32_t g_gpio_amoled_vcc3v1;
/*******************************************************************************
 * LCD VCC
 */
#define VCC_LCDIO_NAME     "lcdio-vcc"
#define VCC_LCDANALOG_NAME "lcdanalog-vcc"

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

static uint32_t g_spr_gama_degama_table[SPR_GAMMA_LUT_SIZE] = {
	/* spr gamma r */
	0,   72,  144,  216,  288,  360,  426,  486,  541,  592,  641,  686,  730,  772,  812,  850,  887,  923,  957,  991,
	1024, 1055, 1086, 1116, 1146, 1174, 1203, 1230, 1257, 1284, 1310, 1335, 1360, 1385, 1409, 1433, 1456, 1479, 1502, 1524,
	1547, 1568, 1590, 1611, 1632, 1653, 1673, 1693, 1713, 1733, 1753, 1772, 1791, 1810, 1829, 1847, 1866, 1884, 1902, 1920,
	1937, 1955, 1972, 1989, 2006, 2023, 2040, 2057, 2073, 2089, 2106, 2122, 2138, 2153, 2169, 2185, 2200, 2216, 2231, 2246,
	2261, 2276, 2291, 2306, 2320, 2335, 2349, 2364, 2378, 2392, 2406, 2420, 2434, 2448, 2462, 2475, 2489, 2503, 2516, 2529,
	2543, 2556, 2569, 2582, 2595, 2608, 2621, 2634, 2647, 2659, 2672, 2684, 2697, 2709, 2722, 2734, 2746, 2759, 2771, 2783,
	2795, 2807, 2819, 2831, 2842, 2854, 2866, 2877, 2889, 2901, 2912, 2924, 2935, 2946, 2958, 2969, 2980, 2991, 3003, 3014,
	3025, 3036, 3047, 3058, 3068, 3079, 3090, 3101, 3112, 3122, 3133, 3143, 3154, 3164, 3175, 3185, 3196, 3206, 3217, 3227,
	3237, 3247, 3257, 3268, 3278, 3288, 3298, 3308, 3318, 3328, 3338, 3348, 3358, 3367, 3377, 3387, 3397, 3406, 3416, 3426,
	3435, 3445, 3454, 3464, 3474, 3483, 3492, 3502, 3511, 3521, 3530, 3539, 3549, 3558, 3567, 3576, 3585, 3595, 3604, 3613,
	3622, 3631, 3640, 3649, 3658, 3667, 3676, 3685, 3694, 3702, 3711, 3720, 3729, 3738, 3746, 3755, 3764, 3772, 3781, 3790,
	3798, 3807, 3815, 3824, 3833, 3841, 3850, 3858, 3866, 3875, 3883, 3892, 3900, 3908, 3917, 3925, 3933, 3942, 3950, 3958,
	3966, 3974, 3983, 3991, 3999, 4007, 4015, 4023, 4031, 4039, 4047, 4055, 4063, 4071, 4079, 4087, 4096, 0,

	/* spr gamma g */
	0,   72,  144,  216,  288,  360,  426,  486,  541,  592,  641,  686,  730,  772,  812,  850,  887,  923,  957,  991,
	1024, 1055, 1086, 1116, 1146, 1174, 1203, 1230, 1257, 1284, 1310, 1335, 1360, 1385, 1409, 1433, 1456, 1479, 1502, 1524,
	1547, 1568, 1590, 1611, 1632, 1653, 1673, 1693, 1713, 1733, 1753, 1772, 1791, 1810, 1829, 1847, 1866, 1884, 1902, 1920,
	1937, 1955, 1972, 1989, 2006, 2023, 2040, 2057, 2073, 2089, 2106, 2122, 2138, 2153, 2169, 2185, 2200, 2216, 2231, 2246,
	2261, 2276, 2291, 2306, 2320, 2335, 2349, 2364, 2378, 2392, 2406, 2420, 2434, 2448, 2462, 2475, 2489, 2503, 2516, 2529,
	2543, 2556, 2569, 2582, 2595, 2608, 2621, 2634, 2647, 2659, 2672, 2684, 2697, 2709, 2722, 2734, 2746, 2759, 2771, 2783,
	2795, 2807, 2819, 2831, 2842, 2854, 2866, 2877, 2889, 2901, 2912, 2924, 2935, 2946, 2958, 2969, 2980, 2991, 3003, 3014,
	3025, 3036, 3047, 3058, 3068, 3079, 3090, 3101, 3112, 3122, 3133, 3143, 3154, 3164, 3175, 3185, 3196, 3206, 3217, 3227,
	3237, 3247, 3257, 3268, 3278, 3288, 3298, 3308, 3318, 3328, 3338, 3348, 3358, 3367, 3377, 3387, 3397, 3406, 3416, 3426,
	3435, 3445, 3454, 3464, 3474, 3483, 3492, 3502, 3511, 3521, 3530, 3539, 3549, 3558, 3567, 3576, 3585, 3595, 3604, 3613,
	3622, 3631, 3640, 3649, 3658, 3667, 3676, 3685, 3694, 3702, 3711, 3720, 3729, 3738, 3746, 3755, 3764, 3772, 3781, 3790,
	3798, 3807, 3815, 3824, 3833, 3841, 3850, 3858, 3866, 3875, 3883, 3892, 3900, 3908, 3917, 3925, 3933, 3942, 3950, 3958,
	3966, 3974, 3983, 3991, 3999, 4007, 4015, 4023, 4031, 4039, 4047, 4055, 4063, 4071, 4079, 4087, 4096, 0,

	/* spr gamma b */
	0,   72,  144,  216,  288,  360,  426,  486,  541,  592,  641,  686,  730,  772,  812,  850,  887,  923,  957,  991,
	1024, 1055, 1086, 1116, 1146, 1174, 1203, 1230, 1257, 1284, 1310, 1335, 1360, 1385, 1409, 1433, 1456, 1479, 1502, 1524,
	1547, 1568, 1590, 1611, 1632, 1653, 1673, 1693, 1713, 1733, 1753, 1772, 1791, 1810, 1829, 1847, 1866, 1884, 1902, 1920,
	1937, 1955, 1972, 1989, 2006, 2023, 2040, 2057, 2073, 2089, 2106, 2122, 2138, 2153, 2169, 2185, 2200, 2216, 2231, 2246,
	2261, 2276, 2291, 2306, 2320, 2335, 2349, 2364, 2378, 2392, 2406, 2420, 2434, 2448, 2462, 2475, 2489, 2503, 2516, 2529,
	2543, 2556, 2569, 2582, 2595, 2608, 2621, 2634, 2647, 2659, 2672, 2684, 2697, 2709, 2722, 2734, 2746, 2759, 2771, 2783,
	2795, 2807, 2819, 2831, 2842, 2854, 2866, 2877, 2889, 2901, 2912, 2924, 2935, 2946, 2958, 2969, 2980, 2991, 3003, 3014,
	3025, 3036, 3047, 3058, 3068, 3079, 3090, 3101, 3112, 3122, 3133, 3143, 3154, 3164, 3175, 3185, 3196, 3206, 3217, 3227,
	3237, 3247, 3257, 3268, 3278, 3288, 3298, 3308, 3318, 3328, 3338, 3348, 3358, 3367, 3377, 3387, 3397, 3406, 3416, 3426,
	3435, 3445, 3454, 3464, 3474, 3483, 3492, 3502, 3511, 3521, 3530, 3539, 3549, 3558, 3567, 3576, 3585, 3595, 3604, 3613,
	3622, 3631, 3640, 3649, 3658, 3667, 3676, 3685, 3694, 3702, 3711, 3720, 3729, 3738, 3746, 3755, 3764, 3772, 3781, 3790,
	3798, 3807, 3815, 3824, 3833, 3841, 3850, 3858, 3866, 3875, 3883, 3892, 3900, 3908, 3917, 3925, 3933, 3942, 3950, 3958,
	3966, 3974, 3983, 3991, 3999, 4007, 4015, 4023, 4031, 4039, 4047, 4055, 4063, 4071, 4079, 4087, 4096, 0,

	/* spr degamma r */
	0,    4,    7,   11,   14,   18,   21,   25,   28,   32,   36,   39,   43,   46,   50,   53,   57,   60,   64,   68,
	71,   74,   78,   82,   86,   89,   93,   98,  102,  106,  110,  115,  119,  124,  129,  134,  139,  144,  149,  154,
	160,  165,  171,  176,  182,  188,  194,  200,  206,  213,  219,  226,  232,  239,  246,  253,  260,  267,  274,  282,
	289,  297,  304,  312,  320,  328,  336,  344,  353,  361,  370,  378,  387,  396,  405,  414,  423,  433,  442,  452,
	462,  471,  481,  491,  501,  512,  522,  532,  543,  554,  565,  576,  587,  598,  609,  620,  632,  644,  655,  667,
	679,  691,  703,  716,  728,  741,  753,  766,  779,  792,  805,  819,  832,  846,  859,  873,  887,  901,  915,  929,
	944,  958,  973,  988, 1002, 1017, 1032, 1048, 1063, 1078, 1094, 1110, 1126, 1142, 1158, 1174, 1190, 1207, 1223, 1240,
	1257, 1274, 1291, 1308, 1325, 1343, 1360, 1378, 1396, 1414, 1432, 1450, 1468, 1487, 1506, 1524, 1543, 1562, 1581, 1600,
	1620, 1639, 1659, 1679, 1698, 1718, 1739, 1759, 1779, 1800, 1820, 1841, 1862, 1883, 1904, 1926, 1947, 1969, 1990, 2012,
	2034, 2056, 2078, 2101, 2123, 2146, 2168, 2191, 2214, 2237, 2261, 2284, 2308, 2331, 2355, 2379, 2403, 2427, 2452, 2476,
	2501, 2525, 2550, 2575, 2600, 2626, 2651, 2677, 2702, 2728, 2754, 2780, 2806, 2833, 2859, 2886, 2912, 2939, 2966, 2993,
	3021, 3048, 3076, 3103, 3131, 3159, 3187, 3216, 3244, 3273, 3301, 3330, 3359, 3388, 3417, 3447, 3476, 3506, 3535, 3565,
	3595, 3626, 3656, 3686, 3717, 3748, 3779, 3810, 3841, 3872, 3903, 3935, 3967, 3999, 4031, 4063, 4096, 0,

	/* spr degamma g */
	0,    4,    7,   11,   14,   18,   21,   25,   28,   32,   36,   39,   43,   46,   50,   53,   57,   60,   64,   68,
	71,   74,   78,   82,   86,   89,   93,   98,  102,  106,  110,  115,  119,  124,  129,  134,  139,  144,  149,  154,
	160,  165,  171,  176,  182,  188,  194,  200,  206,  213,  219,  226,  232,  239,  246,  253,  260,  267,  274,  282,
	289,  297,  304,  312,  320,  328,  336,  344,  353,  361,  370,  378,  387,  396,  405,  414,  423,  433,  442,  452,
	462,  471,  481,  491,  501,  512,  522,  532,  543,  554,  565,  576,  587,  598,  609,  620,  632,  644,  655,  667,
	679,  691,  703,  716,  728,  741,  753,  766,  779,  792,  805,  819,  832,  846,  859,  873,  887,  901,  915,  929,
	944,  958,  973,  988, 1002, 1017, 1032, 1048, 1063, 1078, 1094, 1110, 1126, 1142, 1158, 1174, 1190, 1207, 1223, 1240,
	1257, 1274, 1291, 1308, 1325, 1343, 1360, 1378, 1396, 1414, 1432, 1450, 1468, 1487, 1506, 1524, 1543, 1562, 1581, 1600,
	1620, 1639, 1659, 1679, 1698, 1718, 1739, 1759, 1779, 1800, 1820, 1841, 1862, 1883, 1904, 1926, 1947, 1969, 1990, 2012,
	2034, 2056, 2078, 2101, 2123, 2146, 2168, 2191, 2214, 2237, 2261, 2284, 2308, 2331, 2355, 2379, 2403, 2427, 2452, 2476,
	2501, 2525, 2550, 2575, 2600, 2626, 2651, 2677, 2702, 2728, 2754, 2780, 2806, 2833, 2859, 2886, 2912, 2939, 2966, 2993,
	3021, 3048, 3076, 3103, 3131, 3159, 3187, 3216, 3244, 3273, 3301, 3330, 3359, 3388, 3417, 3447, 3476, 3506, 3535, 3565,
	3595, 3626, 3656, 3686, 3717, 3748, 3779, 3810, 3841, 3872, 3903, 3935, 3967, 3999, 4031, 4063, 4096, 0,

	/* spr degamma b */
	0,    4,    7,   11,   14,   18,   21,   25,   28,   32,   36,   39,   43,   46,   50,   53,   57,   60,   64,   68,
	71,   74,   78,   82,   86,   89,   93,   98,  102,  106,  110,  115,  119,  124,  129,  134,  139,  144,  149,  154,
	160,  165,  171,  176,  182,  188,  194,  200,  206,  213,  219,  226,  232,  239,  246,  253,  260,  267,  274,  282,
	289,  297,  304,  312,  320,  328,  336,  344,  353,  361,  370,  378,  387,  396,  405,  414,  423,  433,  442,  452,
	462,  471,  481,  491,  501,  512,  522,  532,  543,  554,  565,  576,  587,  598,  609,  620,  632,  644,  655,  667,
	679,  691,  703,  716,  728,  741,  753,  766,  779,  792,  805,  819,  832,  846,  859,  873,  887,  901,  915,  929,
	944,  958,  973,  988, 1002, 1017, 1032, 1048, 1063, 1078, 1094, 1110, 1126, 1142, 1158, 1174, 1190, 1207, 1223, 1240,
	1257, 1274, 1291, 1308, 1325, 1343, 1360, 1378, 1396, 1414, 1432, 1450, 1468, 1487, 1506, 1524, 1543, 1562, 1581, 1600,
	1620, 1639, 1659, 1679, 1698, 1718, 1739, 1759, 1779, 1800, 1820, 1841, 1862, 1883, 1904, 1926, 1947, 1969, 1990, 2012,
	2034, 2056, 2078, 2101, 2123, 2146, 2168, 2191, 2214, 2237, 2261, 2284, 2308, 2331, 2355, 2379, 2403, 2427, 2452, 2476,
	2501, 2525, 2550, 2575, 2600, 2626, 2651, 2677, 2702, 2728, 2754, 2780, 2806, 2833, 2859, 2886, 2912, 2939, 2966, 2993,
	3021, 3048, 3076, 3103, 3131, 3159, 3187, 3216, 3244, 3273, 3301, 3330, 3359, 3388, 3417, 3447, 3476, 3506, 3535, 3565,
	3595, 3626, 3656, 3686, 3717, 3748, 3779, 3810, 3841, 3872, 3903, 3935, 3967, 3999, 4031, 4063, 4096, 0,
};

static void spr_core_ctl_config(struct dpu_panel_info *pinfo)
{
	pinfo->spr.spr_size.value = 0;
	pinfo->spr.spr_size.reg.spr_width = pinfo->xres - 1;
	pinfo->spr.spr_size.reg.spr_height = pinfo->yres - 1;

	pinfo->spr.spr_ctrl.value = 0;
	pinfo->spr.spr_ctrl.reg.spr_hpartial_mode = 0;
	pinfo->spr.spr_ctrl.reg.spr_partial_mode = 0;
	pinfo->spr.spr_ctrl.reg.spr_ppro_bypass = 0;
	pinfo->spr.spr_ctrl.reg.spr_dirweightmode = 1;
	pinfo->spr.spr_ctrl.reg.spr_borderdet_en = 0;
	pinfo->spr.spr_ctrl.reg.spr_subdir_en = 1;
	pinfo->spr.spr_ctrl.reg.spr_maskprocess_en = 0;
	pinfo->spr.spr_ctrl.reg.spr_multifac_en = 0;
	pinfo->spr.spr_ctrl.reg.spr_bordertb_dummymode = 0;
	pinfo->spr.spr_ctrl.reg.spr_borderr_dummymode = 1;
	pinfo->spr.spr_ctrl.reg.spr_borderl_dummymode = 1;
	pinfo->spr.spr_ctrl.reg.spr_larea_en = 0;
	pinfo->spr.spr_ctrl.reg.spr_linebuf_1dmode = 0;
	pinfo->spr.spr_ctrl.reg.spr_dummymode = 0;
	pinfo->spr.spr_ctrl.reg.spr_subpxl_layout = 1;/* RGBG mode */
	pinfo->spr.spr_ctrl.reg.spr_en = 1;
}

static void spr_core_pixsel_config(struct dpu_panel_info *pinfo)
{
	pinfo->spr.spr_pix_even.value = 0;
	pinfo->spr.spr_pix_even.reg.spr_coeffsel_even00 = 0;
	pinfo->spr.spr_pix_even.reg.spr_coeffsel_even01 = 1;
	pinfo->spr.spr_pix_even.reg.spr_coeffsel_even10 = 4;
	pinfo->spr.spr_pix_even.reg.spr_coeffsel_even11 = 5;
	pinfo->spr.spr_pix_even.reg.spr_coeffsel_even20 = 0;
	pinfo->spr.spr_pix_even.reg.spr_coeffsel_even21 = 0;

	pinfo->spr.spr_pix_odd.value = 0;
	pinfo->spr.spr_pix_odd.reg.spr_coeffsel_odd00 = 1;
	pinfo->spr.spr_pix_odd.reg.spr_coeffsel_odd01 = 2;
	pinfo->spr.spr_pix_odd.reg.spr_coeffsel_odd10 = 3;
	pinfo->spr.spr_pix_odd.reg.spr_coeffsel_odd11 = 4;
	pinfo->spr.spr_pix_odd.reg.spr_coeffsel_odd20 = 0;
	pinfo->spr.spr_pix_odd.reg.spr_coeffsel_odd21 = 0;

	pinfo->spr.spr_pix_panel.value = 0;
	pinfo->spr.spr_pix_panel.reg.spr_pxlsel_even0 = 0;
	pinfo->spr.spr_pix_panel.reg.spr_pxlsel_even1 = 2;
	pinfo->spr.spr_pix_panel.reg.spr_pxlsel_even2 = 0;
	pinfo->spr.spr_pix_panel.reg.spr_pxlsel_odd0 = 2;
	pinfo->spr.spr_pix_panel.reg.spr_pxlsel_odd1 = 0;
	pinfo->spr.spr_pix_panel.reg.spr_pxlsel_odd2 = 0;
}

static void spr_core_coeffs_r_config(struct dpu_panel_info *pinfo)
{
	pinfo->spr.spr_coeff_v0h0_r0.value = 0;
	pinfo->spr.spr_coeff_v0h0_r0.reg.spr_coeff_v0h0_horz_r0 = 0;
	pinfo->spr.spr_coeff_v0h0_r0.reg.spr_coeff_v0h0_horz_r1 = 16;
	pinfo->spr.spr_coeff_v0h0_r0.reg.spr_coeff_v0h0_horz_r2 = 16;
	pinfo->spr.spr_coeff_v0h0_r0.reg.spr_coeff_v0h0_horz_r3 = 0;

	pinfo->spr.spr_coeff_v0h0_r1.value = 0;
	pinfo->spr.spr_coeff_v0h0_r1.reg.spr_coeff_v0h0_horz_r4 = 0;
	pinfo->spr.spr_coeff_v0h0_r1.reg.spr_coeff_v0h0_vert_r0 = 12;
	pinfo->spr.spr_coeff_v0h0_r1.reg.spr_coeff_v0h0_vert_r1 = 20;
	pinfo->spr.spr_coeff_v0h0_r1.reg.spr_coeff_v0h0_vert_r2 = 0;

	pinfo->spr.spr_coeff_v0h1_r0.value = 0;
	pinfo->spr.spr_coeff_v0h1_r0.reg.spr_coeff_v0h1_horz_r0 = 0;
	pinfo->spr.spr_coeff_v0h1_r0.reg.spr_coeff_v0h1_horz_r1 = 0;
	pinfo->spr.spr_coeff_v0h1_r0.reg.spr_coeff_v0h1_horz_r2 = 0;
	pinfo->spr.spr_coeff_v0h1_r0.reg.spr_coeff_v0h1_horz_r3 = 0;

	pinfo->spr.spr_coeff_v0h1_r1.value = 0;
	pinfo->spr.spr_coeff_v0h1_r1.reg.spr_coeff_v0h1_horz_r4 = 0;
	pinfo->spr.spr_coeff_v0h1_r1.reg.spr_coeff_v0h1_vert_r0 = 0;
	pinfo->spr.spr_coeff_v0h1_r1.reg.spr_coeff_v0h1_vert_r1 = 0;
	pinfo->spr.spr_coeff_v0h1_r1.reg.spr_coeff_v0h1_vert_r2 = 0;

	pinfo->spr.spr_coeff_v1h0_r0.value = 0;
	pinfo->spr.spr_coeff_v1h0_r0.reg.spr_coeff_v1h0_horz_r0 = 0;
	pinfo->spr.spr_coeff_v1h0_r0.reg.spr_coeff_v1h0_horz_r1 = 0;
	pinfo->spr.spr_coeff_v1h0_r0.reg.spr_coeff_v1h0_horz_r2 = 0;
	pinfo->spr.spr_coeff_v1h0_r0.reg.spr_coeff_v1h0_horz_r3 = 0;

	pinfo->spr.spr_coeff_v1h0_r1.value = 0;
	pinfo->spr.spr_coeff_v1h0_r1.reg.spr_coeff_v1h0_horz_r4 = 0;
	pinfo->spr.spr_coeff_v1h0_r1.reg.spr_coeff_v1h0_vert_r0 = 0;
	pinfo->spr.spr_coeff_v1h0_r1.reg.spr_coeff_v1h0_vert_r1 = 0;
	pinfo->spr.spr_coeff_v1h0_r1.reg.spr_coeff_v1h0_vert_r2 = 0;

	pinfo->spr.spr_coeff_v1h1_r0.value = 0;
	pinfo->spr.spr_coeff_v1h1_r0.reg.spr_coeff_v1h1_horz_r0 = 0;
	pinfo->spr.spr_coeff_v1h1_r0.reg.spr_coeff_v1h1_horz_r1 = 16;
	pinfo->spr.spr_coeff_v1h1_r0.reg.spr_coeff_v1h1_horz_r2 = 16;
	pinfo->spr.spr_coeff_v1h1_r0.reg.spr_coeff_v1h1_horz_r3 = 0;

	pinfo->spr.spr_coeff_v1h1_r1.value = 0;
	pinfo->spr.spr_coeff_v1h1_r1.reg.spr_coeff_v1h1_horz_r4 = 0;
	pinfo->spr.spr_coeff_v1h1_r1.reg.spr_coeff_v1h1_vert_r0 = 12;
	pinfo->spr.spr_coeff_v1h1_r1.reg.spr_coeff_v1h1_vert_r1 = 20;
	pinfo->spr.spr_coeff_v1h1_r1.reg.spr_coeff_v1h1_vert_r2 = 0;
}

static void spr_core_coeffs_g_config(struct dpu_panel_info *pinfo)
{
	pinfo->spr.spr_coeff_v0h0_g0.value = 0;
	pinfo->spr.spr_coeff_v0h0_g0.reg.spr_coeff_v0h0_horz_g0 = 0;
	pinfo->spr.spr_coeff_v0h0_g0.reg.spr_coeff_v0h0_horz_g1 = 0;
	pinfo->spr.spr_coeff_v0h0_g0.reg.spr_coeff_v0h0_horz_g2 = 18;
	pinfo->spr.spr_coeff_v0h0_g0.reg.spr_coeff_v0h0_horz_g3 = 14;

	pinfo->spr.spr_coeff_v0h0_g1.value = 0;
	pinfo->spr.spr_coeff_v0h0_g1.reg.spr_coeff_v0h0_horz_g4 = 0;
	pinfo->spr.spr_coeff_v0h0_g1.reg.spr_coeff_v0h0_vert_g0 = 0;
	pinfo->spr.spr_coeff_v0h0_g1.reg.spr_coeff_v0h0_vert_g1 = 31;
	pinfo->spr.spr_coeff_v0h0_g1.reg.spr_coeff_v0h0_vert_g2 = 0;

	pinfo->spr.spr_coeff_v0h1_g0.value = 0;
	pinfo->spr.spr_coeff_v0h1_g0.reg.spr_coeff_v0h1_horz_g0 = 0;
	pinfo->spr.spr_coeff_v0h1_g0.reg.spr_coeff_v0h1_horz_g1 = 0;
	pinfo->spr.spr_coeff_v0h1_g0.reg.spr_coeff_v0h1_horz_g2 = 18;
	pinfo->spr.spr_coeff_v0h1_g0.reg.spr_coeff_v0h1_horz_g3 = 14;

	pinfo->spr.spr_coeff_v0h1_g1.value = 0;
	pinfo->spr.spr_coeff_v0h1_g1.reg.spr_coeff_v0h1_horz_g4 = 0;
	pinfo->spr.spr_coeff_v0h1_g1.reg.spr_coeff_v0h1_vert_g0 = 0;
	pinfo->spr.spr_coeff_v0h1_g1.reg.spr_coeff_v0h1_vert_g1 = 31;
	pinfo->spr.spr_coeff_v0h1_g1.reg.spr_coeff_v0h1_vert_g2 = 0;

	pinfo->spr.spr_coeff_v1h0_g0.value = 0;
	pinfo->spr.spr_coeff_v1h0_g0.reg.spr_coeff_v1h0_horz_g0 = 0;
	pinfo->spr.spr_coeff_v1h0_g0.reg.spr_coeff_v1h0_horz_g1 = 0;
	pinfo->spr.spr_coeff_v1h0_g0.reg.spr_coeff_v1h0_horz_g2 = 18;
	pinfo->spr.spr_coeff_v1h0_g0.reg.spr_coeff_v1h0_horz_g3 = 14;

	pinfo->spr.spr_coeff_v1h0_g1.value = 0;
	pinfo->spr.spr_coeff_v1h0_g1.reg.spr_coeff_v1h0_horz_g4 = 0;
	pinfo->spr.spr_coeff_v1h0_g1.reg.spr_coeff_v1h0_vert_g0 = 0;
	pinfo->spr.spr_coeff_v1h0_g1.reg.spr_coeff_v1h0_vert_g1 = 31;
	pinfo->spr.spr_coeff_v1h0_g1.reg.spr_coeff_v1h0_vert_g2 = 0;

	pinfo->spr.spr_coeff_v1h1_g0.value = 0;
	pinfo->spr.spr_coeff_v1h1_g0.reg.spr_coeff_v1h1_horz_g0 = 0;
	pinfo->spr.spr_coeff_v1h1_g0.reg.spr_coeff_v1h1_horz_g1 = 0;
	pinfo->spr.spr_coeff_v1h1_g0.reg.spr_coeff_v1h1_horz_g2 = 18;
	pinfo->spr.spr_coeff_v1h1_g0.reg.spr_coeff_v1h1_horz_g3 = 14;

	pinfo->spr.spr_coeff_v1h1_g1.value = 0;
	pinfo->spr.spr_coeff_v1h1_g1.reg.spr_coeff_v1h1_horz_g4 = 0;
	pinfo->spr.spr_coeff_v1h1_g1.reg.spr_coeff_v1h1_vert_g0 = 0;
	pinfo->spr.spr_coeff_v1h1_g1.reg.spr_coeff_v1h1_vert_g1 = 31;
	pinfo->spr.spr_coeff_v1h1_g1.reg.spr_coeff_v1h1_vert_g2 = 0;
}

static void spr_core_coeffs_b_config(struct dpu_panel_info *pinfo)
{
	pinfo->spr.spr_coeff_v0h0_b0.value = 0;
	pinfo->spr.spr_coeff_v0h0_b0.reg.spr_coeff_v0h0_horz_b0 = 0;
	pinfo->spr.spr_coeff_v0h0_b0.reg.spr_coeff_v0h0_horz_b1 = 0;
	pinfo->spr.spr_coeff_v0h0_b0.reg.spr_coeff_v0h0_horz_b2 = 0;
	pinfo->spr.spr_coeff_v0h0_b0.reg.spr_coeff_v0h0_horz_b3 = 0;

	pinfo->spr.spr_coeff_v0h0_b1.value = 0;
	pinfo->spr.spr_coeff_v0h0_b1.reg.spr_coeff_v0h0_horz_b4 = 0;
	pinfo->spr.spr_coeff_v0h0_b1.reg.spr_coeff_v0h0_vert_b0 = 0;
	pinfo->spr.spr_coeff_v0h0_b1.reg.spr_coeff_v0h0_vert_b1 = 0;
	pinfo->spr.spr_coeff_v0h0_b1.reg.spr_coeff_v0h0_vert_b2 = 0;

	pinfo->spr.spr_coeff_v0h1_b0.value = 0;
	pinfo->spr.spr_coeff_v0h1_b0.reg.spr_coeff_v0h1_horz_b0 = 0;
	pinfo->spr.spr_coeff_v0h1_b0.reg.spr_coeff_v0h1_horz_b1 = 0;
	pinfo->spr.spr_coeff_v0h1_b0.reg.spr_coeff_v0h1_horz_b2 = 16;
	pinfo->spr.spr_coeff_v0h1_b0.reg.spr_coeff_v0h1_horz_b3 = 16;

	pinfo->spr.spr_coeff_v0h1_b1.value = 0;
	pinfo->spr.spr_coeff_v0h1_b1.reg.spr_coeff_v0h1_horz_b4 = 0;
	pinfo->spr.spr_coeff_v0h1_b1.reg.spr_coeff_v0h1_vert_b0 = 12;
	pinfo->spr.spr_coeff_v0h1_b1.reg.spr_coeff_v0h1_vert_b1 = 20;
	pinfo->spr.spr_coeff_v0h1_b1.reg.spr_coeff_v0h1_vert_b2 = 0;

	pinfo->spr.spr_coeff_v1h0_b0.value = 0;
	pinfo->spr.spr_coeff_v1h0_b0.reg.spr_coeff_v1h0_horz_b0 = 0;
	pinfo->spr.spr_coeff_v1h0_b0.reg.spr_coeff_v1h0_horz_b1 = 0;
	pinfo->spr.spr_coeff_v1h0_b0.reg.spr_coeff_v1h0_horz_b2 = 16;
	pinfo->spr.spr_coeff_v1h0_b0.reg.spr_coeff_v1h0_horz_b3 = 16;

	pinfo->spr.spr_coeff_v1h0_b1.value = 0;
	pinfo->spr.spr_coeff_v1h0_b1.reg.spr_coeff_v1h0_horz_b4 = 0;
	pinfo->spr.spr_coeff_v1h0_b1.reg.spr_coeff_v1h0_vert_b0 = 12;
	pinfo->spr.spr_coeff_v1h0_b1.reg.spr_coeff_v1h0_vert_b1 = 20;
	pinfo->spr.spr_coeff_v1h0_b1.reg.spr_coeff_v1h0_vert_b2 = 0;

	pinfo->spr.spr_coeff_v1h1_b0.value = 0;
	pinfo->spr.spr_coeff_v1h1_b0.reg.spr_coeff_v1h1_horz_b0 = 0;
	pinfo->spr.spr_coeff_v1h1_b0.reg.spr_coeff_v1h1_horz_b1 = 0;
	pinfo->spr.spr_coeff_v1h1_b0.reg.spr_coeff_v1h1_horz_b2 = 0;
	pinfo->spr.spr_coeff_v1h1_b0.reg.spr_coeff_v1h1_horz_b3 = 0;

	pinfo->spr.spr_coeff_v1h1_b1.value = 0;
	pinfo->spr.spr_coeff_v1h1_b1.reg.spr_coeff_v1h1_horz_b4 = 0;
	pinfo->spr.spr_coeff_v1h1_b1.reg.spr_coeff_v1h1_vert_b0 = 0;
	pinfo->spr.spr_coeff_v1h1_b1.reg.spr_coeff_v1h1_vert_b1 = 0;
	pinfo->spr.spr_coeff_v1h1_b1.reg.spr_coeff_v1h1_vert_b2 = 0;
}

static void spr_core_larea_config(struct dpu_panel_info *pinfo)
{
	pinfo->spr.spr_larea_start.value = 0;
	pinfo->spr.spr_larea_start.reg.spr_larea_sx = 400;
	pinfo->spr.spr_larea_start.reg.spr_larea_sy = 200;

	pinfo->spr.spr_larea_end.value = 0;
	pinfo->spr.spr_larea_end.reg.spr_larea_ex = 600;
	pinfo->spr.spr_larea_end.reg.spr_larea_ey = 400;

	pinfo->spr.spr_larea_offset.value = 0;
	pinfo->spr.spr_larea_offset.reg.spr_larea_x_offset = 1;
	pinfo->spr.spr_larea_offset.reg.spr_larea_y_offset = 1;

	pinfo->spr.spr_larea_gain.value = 0;
	pinfo->spr.spr_larea_gain.reg.spr_larea_gain_r = 128;
	pinfo->spr.spr_larea_gain.reg.spr_larea_gain_g = 128;
	pinfo->spr.spr_larea_gain.reg.spr_larea_gain_b = 128;

	pinfo->spr.spr_larea_border_r.value = 0;
	pinfo->spr.spr_larea_border_r.reg.spr_larea_border_gainl_r = 128;
	pinfo->spr.spr_larea_border_r.reg.spr_larea_border_gainr_r = 128;
	pinfo->spr.spr_larea_border_r.reg.spr_larea_border_gaint_r = 128;
	pinfo->spr.spr_larea_border_r.reg.spr_larea_border_gainb_r = 128;

	pinfo->spr.spr_larea_border_g.value = 0;
	pinfo->spr.spr_larea_border_g.reg.spr_larea_border_gainl_g = 128;
	pinfo->spr.spr_larea_border_g.reg.spr_larea_border_gainr_g = 128;
	pinfo->spr.spr_larea_border_g.reg.spr_larea_border_gaint_g = 128;
	pinfo->spr.spr_larea_border_g.reg.spr_larea_border_gainb_g = 128;

	pinfo->spr.spr_larea_border_b.value = 0;
	pinfo->spr.spr_larea_border_b.reg.spr_larea_border_gainl_b = 128;
	pinfo->spr.spr_larea_border_b.reg.spr_larea_border_gainr_b = 128;
	pinfo->spr.spr_larea_border_b.reg.spr_larea_border_gaint_b = 128;
	pinfo->spr.spr_larea_border_b.reg.spr_larea_border_gainb_b = 128;
}

static void spr_core_border_config(struct dpu_panel_info *pinfo)
{
	pinfo->spr.spr_r_borderlr.value = 0;
	pinfo->spr.spr_r_borderlr.reg.spr_borderlr_gainl_r = 128;
	pinfo->spr.spr_r_borderlr.reg.spr_borderlr_offsetl_r = 0;
	pinfo->spr.spr_r_borderlr.reg.spr_borderlr_gainr_r = 128;
	pinfo->spr.spr_r_borderlr.reg.spr_borderlr_offsetr_r = 0;

	pinfo->spr.spr_r_bordertb.value = 0;
	pinfo->spr.spr_r_bordertb.reg.spr_bordertb_gaint_r = 128;
	pinfo->spr.spr_r_bordertb.reg.spr_bordertb_offsett_r = 0;
	pinfo->spr.spr_r_bordertb.reg.spr_bordertb_gainb_r = 128;
	pinfo->spr.spr_r_bordertb.reg.spr_bordertb_offsetb_r = 0;

	pinfo->spr.spr_g_borderlr.value = 0;
	pinfo->spr.spr_g_borderlr.reg.spr_borderlr_gainl_g = 128;
	pinfo->spr.spr_g_borderlr.reg.spr_borderlr_offsetl_g = 0;
	pinfo->spr.spr_g_borderlr.reg.spr_borderlr_gainr_g = 128;
	pinfo->spr.spr_g_borderlr.reg.spr_borderlr_offsetr_g = 0;

	pinfo->spr.spr_g_bordertb.value = 0;
	pinfo->spr.spr_g_bordertb.reg.spr_bordertb_gaint_g = 128;
	pinfo->spr.spr_g_bordertb.reg.spr_bordertb_offsett_g = 0;
	pinfo->spr.spr_g_bordertb.reg.spr_bordertb_gainb_g = 128;
	pinfo->spr.spr_g_bordertb.reg.spr_bordertb_offsetb_g = 0;

	pinfo->spr.spr_b_borderlr.value = 0;
	pinfo->spr.spr_b_borderlr.reg.spr_borderlr_gainl_b = 128;
	pinfo->spr.spr_b_borderlr.reg.spr_borderlr_offsetl_b = 0;
	pinfo->spr.spr_b_borderlr.reg.spr_borderlr_gainr_b = 128;
	pinfo->spr.spr_b_borderlr.reg.spr_borderlr_offsetr_b = 0;

	pinfo->spr.spr_b_bordertb.value = 0;
	pinfo->spr.spr_b_bordertb.reg.spr_bordertb_gaint_b = 128;
	pinfo->spr.spr_b_bordertb.reg.spr_bordertb_offsett_b = 0;
	pinfo->spr.spr_b_bordertb.reg.spr_bordertb_gainb_b = 128;
	pinfo->spr.spr_b_bordertb.reg.spr_bordertb_offsetb_b = 0;

	pinfo->spr.spr_pixgain_reg.value = 0;
	pinfo->spr.spr_pixgain_reg.reg.spr_finalgain_r = 128;
	pinfo->spr.spr_pixgain_reg.reg.spr_finaloffset_r = 0;
	pinfo->spr.spr_pixgain_reg.reg.spr_finalgain_g = 128;
	pinfo->spr.spr_pixgain_reg.reg.spr_finaloffset_g = 0;

	pinfo->spr.spr_pixgain_reg1.value = 0;
	pinfo->spr.spr_pixgain_reg1.reg.spr_finalgain_b = 128;
	pinfo->spr.spr_pixgain_reg1.reg.spr_finaloffset_b = 0;

	pinfo->spr.spr_border_p0.value = 0;
	pinfo->spr.spr_border_p0.reg.spr_borderl_sx = 0;
	pinfo->spr.spr_border_p0.reg.spr_borderl_ex = 0;

	pinfo->spr.spr_border_p1.value = 0;
	pinfo->spr.spr_border_p1.reg.spr_borderr_sx =1079;
	pinfo->spr.spr_border_p1.reg.spr_borderr_ex = 1080;

	pinfo->spr.spr_border_p2.value = 0;
	pinfo->spr.spr_border_p2.reg.spr_bordert_sy = 0;
	pinfo->spr.spr_border_p2.reg.spr_bordert_ey = 0;

	pinfo->spr.spr_border_p3.value = 0;
	pinfo->spr.spr_border_p3.reg.spr_borderb_sy = 0;
	pinfo->spr.spr_border_p3.reg.spr_borderb_ey = 0;
}

static void spr_core_blend_config(struct dpu_panel_info *pinfo)
{
	pinfo->spr.spr_blend.value = 0;
	pinfo->spr.spr_blend.reg.spr_gradorthblend = 31;
	pinfo->spr.spr_blend.reg.spr_subdirblend = 32;

	pinfo->spr.spr_weight.value = 0;
	pinfo->spr.spr_weight.reg.spr_weight2bldk = 16;
	pinfo->spr.spr_weight.reg.spr_weight2bldcor = 0;
	pinfo->spr.spr_weight.reg.spr_weight2bldmin = 0;
	pinfo->spr.spr_weight.reg.spr_weight2bldmax = 32;

	pinfo->spr.spr_edgestr_r.value = 0;
	pinfo->spr.spr_edgestr_r.reg.spr_edgestrbldk_r = 16;
	pinfo->spr.spr_edgestr_r.reg.spr_edgestrbldcor_r = 64;
	pinfo->spr.spr_edgestr_r.reg.spr_edgestrbldmin_r = 0;
	pinfo->spr.spr_edgestr_r.reg.spr_edgestrbldmax_r = 128;

	pinfo->spr.spr_edgestr_g.value = 0;
	pinfo->spr.spr_edgestr_g.reg.spr_edgestrbldk_g = 16;
	pinfo->spr.spr_edgestr_g.reg.spr_edgestrbldcor_g = 64;
	pinfo->spr.spr_edgestr_g.reg.spr_edgestrbldmin_g = 0;
	pinfo->spr.spr_edgestr_g.reg.spr_edgestrbldmax_g = 128;

	pinfo->spr.spr_edgestr_b.value = 0;
	pinfo->spr.spr_edgestr_b.reg.spr_edgestrbldk_b = 16;
	pinfo->spr.spr_edgestr_b.reg.spr_edgestrbldcor_b = 64;
	pinfo->spr.spr_edgestr_b.reg.spr_edgestrbldmin_b = 0;
	pinfo->spr.spr_edgestr_b.reg.spr_edgestrbldmax_b = 128;

	pinfo->spr.spr_dir_min.value = 0;
	pinfo->spr.spr_dir_min.reg.spr_dirweibldmin_r = 0;
	pinfo->spr.spr_dir_min.reg.spr_dirweibldmin_g = 0;
	pinfo->spr.spr_dir_min.reg.spr_dirweibldmin_b = 0;

	pinfo->spr.spr_dir_max.value = 0;
	pinfo->spr.spr_dir_max.reg.spr_dirweibldmax_r = 256;
	pinfo->spr.spr_dir_max.reg.spr_dirweibldmax_g = 256;
	pinfo->spr.spr_dir_max.reg.spr_dirweibldmax_b = 256;
}

static void spr_core_diffdirgain_config(struct dpu_panel_info *pinfo)
{
	pinfo->spr.spr_diff_r0.value = 0;
	pinfo->spr.spr_diff_r0.reg.spr_diffdirgain_r_0 = 0;
	pinfo->spr.spr_diff_r0.reg.spr_diffdirgain_r_1 = 0;
	pinfo->spr.spr_diff_r0.reg.spr_diffdirgain_r_2 = 0;
	pinfo->spr.spr_diff_r0.reg.spr_diffdirgain_r_3 = 0;

	pinfo->spr.spr_diff_r1.value = 0;
	pinfo->spr.spr_diff_r1.reg.spr_diffdirgain_r_4 = 41;
	pinfo->spr.spr_diff_r1.reg.spr_diffdirgain_r_5 = 0;
	pinfo->spr.spr_diff_r1.reg.spr_diffdirgain_r_6 = 0;
	pinfo->spr.spr_diff_r1.reg.spr_diffdirgain_r_7 = 0;

	pinfo->spr.spr_diff_g0.value = 0;
	pinfo->spr.spr_diff_g0.reg.spr_diffdirgain_g_0 = 41;
	pinfo->spr.spr_diff_g0.reg.spr_diffdirgain_g_1 = 0;
	pinfo->spr.spr_diff_g0.reg.spr_diffdirgain_g_2 = 0;
	pinfo->spr.spr_diff_g0.reg.spr_diffdirgain_g_3 = 0;

	pinfo->spr.spr_diff_g1.value = 0;
	pinfo->spr.spr_diff_g1.reg.spr_diffdirgain_g_4 = 0;
	pinfo->spr.spr_diff_g1.reg.spr_diffdirgain_g_5 = 0;
	pinfo->spr.spr_diff_g1.reg.spr_diffdirgain_g_6 = 0;
	pinfo->spr.spr_diff_g1.reg.spr_diffdirgain_g_7 = 0;

	pinfo->spr.spr_diff_b0.value = 0;
	pinfo->spr.spr_diff_b0.reg.spr_diffdirgain_b_0 = 0;
	pinfo->spr.spr_diff_b0.reg.spr_diffdirgain_b_1 = 0;
	pinfo->spr.spr_diff_b0.reg.spr_diffdirgain_b_2 = 0;
	pinfo->spr.spr_diff_b0.reg.spr_diffdirgain_b_3 = 0;

	pinfo->spr.spr_diff_b1.value = 0;
	pinfo->spr.spr_diff_b1.reg.spr_diffdirgain_b_4 = 41;
	pinfo->spr.spr_diff_b1.reg.spr_diffdirgain_b_5 = 0;
	pinfo->spr.spr_diff_b1.reg.spr_diffdirgain_b_6 = 0;
	pinfo->spr.spr_diff_b1.reg.spr_diffdirgain_b_7 = 0;
}

static void spr_core_bd_config(struct dpu_panel_info *pinfo)
{
	pinfo->spr.spr_horzgradblend.value = 0;
	pinfo->spr.spr_horzgradblend.reg.spr_horzgradlblend = 16;
	pinfo->spr.spr_horzgradblend.reg.spr_horzgradrblend = 16;

	pinfo->spr.spr_horzbdbld.value = 0;
	pinfo->spr.spr_horzbdbld.reg.spr_horzbdbldk = 16;
	pinfo->spr.spr_horzbdbld.reg.spr_horzbdbldcor = 8;
	pinfo->spr.spr_horzbdbld.reg.spr_horzbdbldmin = 0;
	pinfo->spr.spr_horzbdbld.reg.spr_horzbdbldmax = 32;

	pinfo->spr.spr_horzbdweight.value = 0;
	pinfo->spr.spr_horzbdweight.reg.spr_horzbdweightmin = 0;
	pinfo->spr.spr_horzbdweight.reg.spr_horzbdweightmax = 256;

	pinfo->spr.spr_vertbdbld.value = 0;
	pinfo->spr.spr_vertbdbld.reg.spr_vertbdbldk = 16;
	pinfo->spr.spr_vertbdbld.reg.spr_vertbdbldcor = 8;
	pinfo->spr.spr_vertbdbld.reg.spr_vertbdbldmin = 0;
	pinfo->spr.spr_vertbdbld.reg.spr_vertbdbldmax = 32;

	pinfo->spr.spr_vertbdweight.value = 0;
	pinfo->spr.spr_vertbdweight.reg.spr_vertbdweightmin = 0;
	pinfo->spr.spr_vertbdweight.reg.spr_vertbdweightmax = 256;

	pinfo->spr.spr_vertbd_gain_r.value = 0;
	pinfo->spr.spr_vertbd_gain_r.reg.spr_vertbd_gainlef_r = 0;
	pinfo->spr.spr_vertbd_gain_r.reg.spr_vertbd_gainrig_r = 0;
	pinfo->spr.spr_vertbd_gain_r.reg.spr_horzbd_gaintop_r = 50;
	pinfo->spr.spr_vertbd_gain_r.reg.spr_horzbd_gainbot_r = 0;

	pinfo->spr.spr_vertbd_gain_g.value = 0;
	pinfo->spr.spr_vertbd_gain_g.reg.spr_vertbd_gainlef_g = 0;
	pinfo->spr.spr_vertbd_gain_g.reg.spr_vertbd_gainrig_g = 0;
	pinfo->spr.spr_vertbd_gain_g.reg.spr_horzbd_gaintop_g = 0;
	pinfo->spr.spr_vertbd_gain_g.reg.spr_horzbd_gainbot_g = 40;

	pinfo->spr.spr_vertbd_gain_b.value = 0;
	pinfo->spr.spr_vertbd_gain_b.reg.spr_vertbd_gainlef_b = 0;
	pinfo->spr.spr_vertbd_gain_b.reg.spr_vertbd_gainrig_b = 0;
	pinfo->spr.spr_vertbd_gain_b.reg.spr_horzbd_gaintop_b = 50;
	pinfo->spr.spr_vertbd_gain_b.reg.spr_horzbd_gainbot_b = 0;
}

static void spr_core_config(struct dpu_panel_info *pinfo)
{
	DPU_FB_INFO("+\n");

	spr_core_ctl_config(pinfo);
	spr_core_pixsel_config(pinfo);
	spr_core_coeffs_r_config(pinfo);
	spr_core_coeffs_g_config(pinfo);
	spr_core_coeffs_b_config(pinfo);
	spr_core_larea_config(pinfo);
	spr_core_border_config(pinfo);
	spr_core_blend_config(pinfo);
	spr_core_diffdirgain_config(pinfo);
	spr_core_bd_config(pinfo);

	DPU_FB_INFO("-\n");
}

static void spr_txip_cfg(struct dpu_panel_info *pinfo)
{
	DPU_FB_DEBUG("+\n");

	pinfo->spr.txip_ctrl.value = 0;
#ifdef TXIP_RXIP_ON
	pinfo->spr.txip_ctrl.reg.txip_en = 1;
#else
	pinfo->spr.txip_ctrl.reg.txip_en = 0;
#endif
	pinfo->spr.txip_ctrl.reg.txip_ppro_bypass = 0;
	pinfo->spr.txip_ctrl.reg.txip_shift = 4;
	pinfo->spr.txip_ctrl.reg.txip_rdmode = 1;

	pinfo->spr.txip_size.value = 0;
	pinfo->spr.txip_size.reg.txip_height = pinfo->yres - 1;
	pinfo->spr.txip_size.reg.txip_width = pinfo->xres - 1;

	pinfo->spr.txip_coef0.value = 0;
	pinfo->spr.txip_coef0.reg.txip_coeff00 = 4;
	pinfo->spr.txip_coef0.reg.txip_coeff01 = 8;
	pinfo->spr.txip_coef0.reg.txip_coeff02 = 4;
	pinfo->spr.txip_coef0.reg.txip_coeff03 = 0;

	pinfo->spr.txip_coef1.value = 0;
	pinfo->spr.txip_coef1.reg.txip_coeff10 = 8;
	pinfo->spr.txip_coef1.reg.txip_coeff11 = 0;
	pinfo->spr.txip_coef1.reg.txip_coeff12 = -8;
	pinfo->spr.txip_coef1.reg.txip_coeff13 = 0;

	pinfo->spr.txip_coef2.value = 0;
	pinfo->spr.txip_coef2.reg.txip_coeff20 = -4;
	pinfo->spr.txip_coef2.reg.txip_coeff21 = 4;
	pinfo->spr.txip_coef2.reg.txip_coeff22 = -4;
	pinfo->spr.txip_coef2.reg.txip_coeff23 = 4;

	pinfo->spr.txip_coef3.value = 0;
	pinfo->spr.txip_coef3.reg.txip_coeff30 = 4;
	pinfo->spr.txip_coef3.reg.txip_coeff31 = 0;
	pinfo->spr.txip_coef3.reg.txip_coeff32 = 4;
	pinfo->spr.txip_coef3.reg.txip_coeff33 = 8;

	pinfo->spr.txip_offset0.value = 0;
	pinfo->spr.txip_offset0.reg.txip_offset0 = 0;
	pinfo->spr.txip_offset0.reg.txip_offset1 = 512;

	pinfo->spr.txip_offset1.value = 0;
	pinfo->spr.txip_offset1.reg.txip_offset2 = 512;
	pinfo->spr.txip_offset1.reg.txip_offset3 = 0;

	pinfo->spr.txip_clip.value = 0;
	pinfo->spr.txip_clip.reg.txip_clipmax = 1023;
	pinfo->spr.txip_clip.reg.txip_clipmin = 0;
}

static void spr_datapack_cfg(struct dpu_panel_info *pinfo)
{
	DPU_FB_DEBUG("+\n");

	pinfo->spr.datapack_ctrl.value = 0;
	pinfo->spr.datapack_ctrl.reg.datapack_en = 1;
	pinfo->spr.datapack_ctrl.reg.datapack_subpxl_layout = 1;
	pinfo->spr.datapack_ctrl.reg.datapack_packmode = 0;

	pinfo->spr.datapack_size.value = 0;
	pinfo->spr.datapack_size.reg.datapack_width = pinfo->xres - 1;
	pinfo->spr.datapack_size.reg.datapack_height = pinfo->yres - 1;
}

static void spr_gamma_config(struct dpu_panel_info *pinfo)
{
	DPU_FB_INFO("++\n");

	pinfo->spr.spr_gamma_en.value = 0;
	pinfo->spr.spr_gamma_en.reg.spr_gama_en = 1;

	pinfo->spr.spr_gamma_shiften.value = 0;
	pinfo->spr.spr_gamma_shiften.reg.gama_shift_en = 0;
	pinfo->spr.spr_gamma_shiften.reg.gama_mode = 1;

	pinfo->spr.degamma_en.value = 0;
	pinfo->spr.degamma_en.reg.degama_en = 1;

	pinfo->spr.spr_lut_table = g_spr_gama_degama_table;
	pinfo->spr.spr_lut_table_len = SPR_GAMMA_LUT_SIZE;
}

static void spr_para_init(struct dpu_panel_info *pinfo)
{
	DPU_FB_INFO("+\n");

	spr_core_config(pinfo);
	spr_txip_cfg(pinfo);
	spr_datapack_cfg(pinfo);

	spr_gamma_config(pinfo);

	pinfo->spr.spr_en = 1;

	DPU_FB_INFO("-\n");
}

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
	DPU_FB_INFO("read NT37800A reg:0x%x = 0x%x\n", rd_cmd >> 8, status);

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
		gpio_cmds_tx(g_fpga_lcd_gpio_request_cmds,
			ARRAY_SIZE(g_fpga_lcd_gpio_request_cmds));

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
		mdelay(10); /* time constraints */

		/* read panel power state reg */
		mipi_read_ic_regs(mipi_dsi0_base, 0x0a06);
		/* read page1 0xc8 value. 0:cut1 1:cut2 */
		mipi_dsi_cmds_tx(g_change_page1_cmds, ARRAY_SIZE(g_change_page1_cmds), mipi_dsi0_base);
		mipi_read_ic_regs(mipi_dsi0_base, 0xc806);

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

static int mipi_panel_set_backlight(struct platform_device *pdev, uint32_t bl_level)
{
	struct dpu_fb_data_type *dpufd = NULL;
	int ret = 0;
	static uint32_t last_bl_level;
	char bl_level_adjust[3] = {
		0x51, 0x00, 0x00,
	};
	struct dsi_cmd_desc lcd_bl_level_adjust[] = {
		{DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(bl_level_adjust), bl_level_adjust},
	};

	DPU_FB_INFO("get from dss bl_level is %d\n", bl_level);
	bl_level = 2047;
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
			mipi_dsi_cmds_tx(lcd_bl_level_adjust, ARRAY_SIZE(lcd_bl_level_adjust),
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

	if (pdev == NULL) {
		DPU_FB_ERR("NULL Pointer\n");
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

static char *reg_name[4] = {"power mode", "MADCTR", "image mode", "dsi error"};
static char lcd_reg_0a[] = {0x0a};
static char lcd_reg_0b[] = {0x0b};
static char lcd_reg_0d[] = {0x0d};
static char lcd_reg_0e[] = {0x0e};

static struct dsi_cmd_desc lcd_check_reg[] = {
	{ DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US, sizeof(lcd_reg_0a), lcd_reg_0a },
	{ DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US, sizeof(lcd_reg_0b), lcd_reg_0b },
	{ DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US, sizeof(lcd_reg_0d), lcd_reg_0d },
	{ DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US, sizeof(lcd_reg_0e), lcd_reg_0e },
};

static ssize_t mipi_panel_lcd_check_reg_show(struct platform_device *pdev,
	char *buf)
{
	ssize_t ret = 0;
	struct dpu_fb_data_type *dpufd = NULL;
	char __iomem *mipi_dsi0_base = NULL;
	uint32_t read_value[4] = {0};
	uint32_t expected_value[4] = { 0x9c, 0x00, 0x00, 0x80 };
	uint32_t read_mask[4] = { 0xFF, 0xFF, 0xFF, 0xFF };

	struct mipi_dsi_read_compare_data data = {
		.read_value = read_value,
		.expected_value = expected_value,
		.read_mask = read_mask,
		.reg_name = reg_name,
		.log_on = 1,
		.cmds = lcd_check_reg,
		.cnt = ARRAY_SIZE(lcd_check_reg),
	};

	if (pdev == NULL) {
		DPU_FB_ERR("pdev is NULL");
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

static void dsc_config_initial(struct dpu_panel_info *pinfo)
{
	struct dsc_algorithm_manager *pt = get_dsc_algorithm_manager_instance();
	struct input_dsc_info input_dsc_info;

	dpu_check_and_no_retval((pt == NULL) || (pinfo == NULL), ERR, "[DSC] NULL Pointer");
	DPU_FB_DEBUG("+\n");
	pinfo->ifbc_type = IFBC_TYPE_VESA2X_DUAL_SPR;
	input_dsc_info.dsc_version = DSC_VERSION_V_1_2;
	input_dsc_info.format = DSC_YUV422;
	input_dsc_info.pic_width = pinfo->xres;
	input_dsc_info.pic_height = pinfo->yres;
	input_dsc_info.slice_width = 671 + 1;
	input_dsc_info.slice_height = 43 + 1;
	input_dsc_info.dsc_bpp = DSC_16BPP;
	input_dsc_info.dsc_bpc = DSC_8BPC;
	input_dsc_info.block_pred_enable = 1;
	input_dsc_info.linebuf_depth = LINEBUF_DEPTH_9;
	input_dsc_info.gen_rc_params = DSC_NOT_GENERATE_RC_PARAMETERS;
	pt->vesa_dsc_info_calc(&input_dsc_info, &(pinfo->panel_dsc_info.dsc_info), NULL);
	pinfo->vesa_dsc.bits_per_pixel = DSC_0BPP;
	DPU_FB_ERR("panel: dsc_bpc=%d\n", pinfo->panel_dsc_info.dsc_info.dsc_bpc);
}

/* panel info initialized value from panel spec */
static void mipi_lcd_init_panel_info(struct dpu_panel_info *pinfo,
	uint32_t bl_type, uint32_t lcd_display_type)
{
	pinfo->xres = 1344;
	pinfo->yres = 2772;
	pinfo->width = 74;
	pinfo->height = 154;
	pinfo->orientation = LCD_PORTRAIT;
	pinfo->bpp = LCD_RGB888;
	pinfo->bgr_fmt = LCD_RGB;
	pinfo->bl_set_type = bl_type;
	pinfo->bl_min = 12;
	pinfo->bl_max = 2047;
	pinfo->bl_default = 2047;
	pinfo->bl_ic_ctrl_mode = AMOLED_NO_BL_IC_MODE;

	#ifdef AP_SPR_DSC1_2_EN
		pinfo->spr_dsc_mode = SPR_DSC_MODE_SPR_AND_DSC;
	#else
		pinfo->spr_dsc_mode = SPR_DSC_MODE_NONE;
	#endif

	pinfo->frc_enable = 0;
	pinfo->esd_enable = 0;
	pinfo->esd_skip_mipi_check = 0;
	pinfo->lcd_uninit_step_support = 1;
	pinfo->type = lcd_display_type;
}

/* dsi param initialized value from panel spec */
static void mipi_lcd_init_dsi_param(struct dpu_panel_info *pinfo)
{
	/* lcd porch size init */
	if (g_lcd_fpga_flag == 1) {
		pinfo->mipi.hsa = 13;
		pinfo->mipi.hbp = 5;
		pinfo->mipi.dpi_hsize = 338;
		pinfo->mipi.hline_time = 1000;

		pinfo->mipi.vsa = 30;
		pinfo->mipi.vbp = 50;
		pinfo->mipi.vfp = 20;
		pinfo->mipi.dsi_timing_support = 1;

		pinfo->pxl_clk_rate = 20 * 1000000UL;
		pinfo->mipi.dsi_bit_clk = 140;
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

	pinfo->pxl_clk_rate_div = 1;
}

static void panel_info_initial(struct dpu_fb_panel_data *pdata,
	uint32_t bl_type, uint32_t lcd_display_type)
{
	struct dpu_panel_info *pinfo = NULL;

	pinfo = pdata->panel_info;
	memset(pinfo, 0, sizeof(struct dpu_panel_info));

	mipi_lcd_init_panel_info(pinfo, bl_type, lcd_display_type);
	mipi_lcd_init_dsi_param(pinfo);

#if defined(CONFIG_DPU_FB_V600) || defined(CONFIG_DPU_FB_V360)
	dsc_config_initial(pinfo);
#endif

#ifdef SUPPORT_SPR
	spr_para_init(pinfo);
#endif

	if (pinfo->type == PANEL_MIPI_CMD) {
		pinfo->vsync_ctrl_type = 0; /* VSYNC_CTRL_ISR_OFF | VSYNC_CTRL_MIPI_ULPS | VSYNC_CTRL_CLK_OFF; */
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

	if (g_lcd_fpga_flag == 0) {
		if (pinfo->pxl_clk_rate_div > 1) {
			pinfo->ldi.h_back_porch /= pinfo->pxl_clk_rate_div;
			pinfo->ldi.h_front_porch /= pinfo->pxl_clk_rate_div;
			pinfo->ldi.h_pulse_width /= pinfo->pxl_clk_rate_div;
		}
	}
}

static int mipi_lcd_get_info_from_dts(struct device_node *np, uint32_t *bl_type,
	uint32_t *lcd_display_type, uint32_t *lcd_ifbc_type)
{
	int ret = -1;

	DPU_FB_INFO("%s +\n", DTS_COMP_BOE_NT37800A);

	np = of_find_compatible_node(NULL, NULL, DTS_COMP_BOE_NT37800A);
	if (np == NULL) {
		DPU_FB_ERR("not found device node %s!\n",
			DTS_COMP_BOE_NT37800A);
		return ret;
	}

	ret = of_property_read_u32(np, LCD_DISPLAY_TYPE_NAME,
		lcd_display_type);
	if (ret) {
		DPU_FB_ERR("get lcd_display_type failed!\n");
		*lcd_display_type = PANEL_MIPI_CMD;
	}

	ret = of_property_read_u32(np, LCD_IFBC_TYPE_NAME,
		lcd_ifbc_type);
	if (ret) {
		DPU_FB_ERR("get ifbc_type failed!\n");
		*lcd_ifbc_type = IFBC_TYPE_NONE;
	}

	ret = of_property_read_u32(np, LCD_BL_TYPE_NAME,
		bl_type);
	if (ret) {
		DPU_FB_ERR("get lcd_bl_type failed!\n");
		*bl_type = BL_SET_BY_MIPI;
	}
	DPU_FB_INFO("bl_type = 0x%x\n", *bl_type);

	if (dpu_fb_device_probe_defer(*lcd_display_type, *bl_type))
		return -EPROBE_DEFER;


	ret = of_property_read_u32(np, FPGA_FLAG_NAME, &g_lcd_fpga_flag);//lint !e64
	if (ret)
		DPU_FB_ERR("get g_lcd_fpga_flag failed!\n");

	if (g_lcd_fpga_flag == 1) {
		g_gpio_amoled_reset = 139;
		g_gpio_amoled_vcc3v1 = 141;
		g_gpio_amoled_vcc1v8 = 132;
		g_gpio_amoled_te0 = 113;
	} else {
		g_gpio_amoled_reset = of_get_named_gpio(np, "gpios", 0);
		g_gpio_amoled_te0 = of_get_named_gpio(np, "gpios", 1);
		g_gpio_amoled_vcc3v1 = of_get_named_gpio(np, "gpios", 2);
	}

	return 0;
}


static int mipi_panel_probe(struct platform_device *pdev)
{
	int ret = -1;
	struct device_node *np = NULL;
	uint32_t bl_type;
	uint32_t lcd_display_type;
	uint32_t lcd_ifbc_type;

	DPU_FB_DEBUG("+\n");

	pdev->id = 1;
	ret = mipi_lcd_get_info_from_dts(np, &bl_type, &lcd_display_type, &lcd_ifbc_type);
	if (ret) {
		DPU_FB_ERR("dpu_fb_device_probe_defer fail!\n");
		return -EPROBE_DEFER;
	}

	panel_info_initial(&g_panel_data, bl_type, lcd_display_type);

	ret = lcd_voltage_relevant_init(pdev);
	if (ret != 0)
		return ret;

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
}

static const struct of_device_id g_dpu_panel_match_table[] = {
	{
		.compatible = DTS_COMP_BOE_NT37800A,
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
		.name = "mipi_boe_NT37800A",
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
/*lint +e542 +e570*/
module_init(mipi_lcd_panel_init);
