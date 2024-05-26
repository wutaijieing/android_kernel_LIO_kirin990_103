/* Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
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
#include "dsc/dsc_output_calc.h"
#include "dsc/dsc_algorithm_manager.h"

/* if not defined, means dsc1.1 and ic spr */
#define AP_SPR_DSC1_2_EN 1
#define TXIP_RXIP_ON 1

#ifdef DFR90
#define MIPI_HLINE_TIME 487 /* 90hz */
#else
#define MIPI_HLINE_TIME 710 /* 60hz */
#endif

/* Power ON Sequence */
static char g_on_cmd0[] = { /* page0 */
	0xF0, 0x55, 0xAA, 0x52, 0x08, 0x00
};

static char g_on_cmd1[] = { /* ddic gpio (ex: TE) */
	0xBE, 0x0E, 0x94, 0x93, 0x1F
};

static char g_on_cmd2[] = { /* shift index, change to next register offset */
	0x6F, 0x0B
};

static char g_on_cmd3[] = { /* pmic setting, follow anna pulse(74,20,34) */
	0xB5, 0x4A, 0x14, 0x22
};

static char g_on_cmd4[] = { /* page1 */
	0xF0, 0x55, 0xAA, 0x52, 0x08, 0x01
};

static char g_on_cmd5[] = {
	0xD1, 0x07, 0x02, 0x10
};

static char g_on_cmd6[] = { /* aod control */
	0xD2, 0x00, 0x40, 0x11
};

static char g_on_cmd7[] = { /* voltage setting */
	0xB7, 0x1E, 0x1E, 0x14, 0x14, 0x14, 0x14, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E
};

static char g_on_cmd8[] = { /* voltage setting */
	0xB1, 0x00, 0x00
};

static char g_on_cmd9[] = { /* voltage setting */
	0xB4, 0x03, 0x03
};

static char g_on_cmd10[] = { /* voltage setting */
	0xB8, 0x43, 0x43, 0x41, 0x41, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43
};

static char g_on_cmd11[] = { /* voltage setting */
	0xB5, 0x00, 0xB7, 0x00, 0x87, 0x00, 0x87, 0x00, 0xB7, 0x00, 0x87
};

static char g_on_cmd12[] = { /* change to page3 */
	0xF0, 0x55, 0xAA, 0x52, 0x08, 0x03
};

static char g_on_cmd13[] = { /* shift index */
	0x6F, 0x07
};

static char g_on_cmd14[] = { /* gate eq setting */
	0xB0, 0x00
};

#ifdef AP_SPR_DSC1_2_EN
/* this code bypass DDIC spr, when enable ap spr + dsc1.2 */
static char g_on_cmd_sprdsc1p2_01[] = {
	0xDE, 0x43
};

static char g_on_cmd_sprdsc1p2_02[] = {
	0x6F, 0x08
};

#ifdef TXIP_RXIP_ON
static char g_on_cmd_sprdsc1p2_03[] = {
	0xDE, 0x80
};

static char g_on_cmd_sprdsc1p2_04[] = {
	0x6F, 0x09
};

static char g_on_cmd_sprdsc1p2_05[] = {
	0xDE, 0xE4, 0x6C
};
#else /* txip/rxip off */
static char g_on_cmd_sprdsc1p2_03[] = { /* close rxip */
	0xDE, 0x00
};

static char g_on_cmd_sprdsc1p2_04[] = {
	0x6F, 0x09
};

static char g_on_cmd_sprdsc1p2_05[] = {
	0xDE, 0xB4, 0x78
};
#endif /* TXIP_RXIP_ON */

static char g_on_cmd_sprdsc1p2_06[] = {
	0xF0, 0x55, 0xAA, 0x52, 0x08, 0x07
};

static char g_on_cmd_sprdsc1p2_07[] = {
	0xB0, 0x24 /* close ddic spr */
};

static char g_on_cmd_sprdsc1p2_08[] = {
	0xF0, 0x55, 0xAA, 0x52, 0x08, 0x08
};

static char g_on_cmd_sprdsc1p2_09[] = {
	0xBE, 0x00
};
/* this code bypass DDIC spr, when enable ap spr + dsc1.2 */
#endif /* AP_SPR_DSC1_2_EN */

static char g_on_cmd15[] = { /* disable cmd2 */
	0xF0, 0x55, 0xAA, 0x52, 0x00, 0x00
};

static char g_on_cmd16[] = { /* not access */
	0xFF, 0xAA, 0x55, 0xA5, 0x81
};

static char g_on_cmd17[] = {
	0x6F, 0x0D
};

static char g_on_cmd18[] = {
	0xF3, 0xCB
};

static char g_on_cmd19[] = {
	0xFB, 0x20
};

static char g_on_cmd20[] = {
	0x6F, 0x44
};

static char g_on_cmd21[] = {
	0xFB, 0x04, 0x04
};

static char g_on_cmd22[] = {
	0x6F, 0x48
};

static char g_on_cmd23[] = {
	0xFB, 0x04, 0x04
};

static char g_on_cmd24[] = {
	0xFF, 0xAA, 0x55, 0xA5, 0x00
};

static char g_on_cmd25[] = { /* te on */
	0x35, 0x00
};

static char g_on_cmd26[] = { /* video mode porch setting */
	0x3B, 0x00, 0x10, 0x00, 0x10
};

static char g_on_cmd27[] = { /* dimming setting */
	0x53, 0x20
};

static char g_on_cmd28[] = { /* display ram x */
	0x2A, 0x00, 0x00, 0x05, 0x3F
};

static char g_on_cmd29[] = { /* display ram y */
	0x2B, 0x00, 0x00, 0x0A, 0xD3
};

static char g_on_cmd30[] = {
	0x82, 0xAB
};

/* dsc setting */
#ifdef AP_SPR_DSC1_2_EN
static char g_on_cmd31[] = {
	0x91, 0x89, 0xF0, 0x00, 0x2C, 0xF1, 0x55, 0x01, 0xEA, 0x02,
	0x61, 0x00, 0x38, 0x02, 0xCB, 0x0A, 0x72, 0x0B, 0xD0
};

#else
static char g_on_cmd31[] = {
	0x91, 0x89, 0x28, 0x00, 0x2C, 0xC2, 0x00, 0x02, 0x50, 0x04,
	0xBA, 0x00, 0x09, 0x02, 0x3C, 0x01, 0xDC, 0x10, 0xF0,
};
#endif

/* close dsc */
#ifdef CLOSE_DSC
static char g_on_cmd32[] = {
	0x90, 0x02
};

static char g_on_cmd33[] = {
	0x03, 0x00
};
#else
static char g_on_cmd32[] = { /* dsc enable */
	0x90, 0x01
};

static char g_on_cmd33[] = {
	0x03, 0x01
};
#endif

static char g_on_cmd34[] = {
	0x2C, 0x00
};

#ifdef DFR90
static char g_on_cmd35[] = { /* frame rate control ,90hz */
	0x2F, 0x02
};
#else
static char g_on_cmd35[] = { /* frame rate control ,60hz */
	0x2F, 0x01
};
#endif

static char g_on_cmd36[] = { /* sleep out */
	0x11
};

static char g_on_cmd38[] = { /* display on */
	0x29
};

static char g_bl_level[] = { /* set backlight */
	0x51, 0xff
};

/* bitmode not send 0x11(sleep out), 0x29(display on) command */
#ifdef BIST_MODE
static char g_bist_mode_4[] = {
	0x28
};

static char g_bist_mode_0[] = {
	0x10
};
static char g_bist_mode_1[] = {
	0xF0, 0x55, 0xAA, 0x52, 0x08, 0x00
};
static char g_bist_mode_2[] = {
	0xEF, 0x01, 0x00, 0xFF, 0xFF, 0xFF, 0x16, 0xFF
};
static char g_bist_mode_3[] = {
	0xEE, 0x87, 0x78, 0x02, 0x40
};
#endif

/* black screen function: always display even didn't receive complete picture
 * cmd1:0xF0, 0x55, 0xAA, 0x52, 0x08, 0x00
 * cmd2:0xC0, 0xF6
 */

static struct dsi_cmd_desc g_lcd_display_on_cmds[] = {
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
	{DTYPE_DCS_LWRITE, 0, 20, WAIT_TYPE_US, sizeof(g_bist_mode_1), g_bist_mode_1},
	{DTYPE_DCS_LWRITE, 0, 20, WAIT_TYPE_US, sizeof(g_bist_mode_2), g_bist_mode_2},
	{DTYPE_DCS_LWRITE, 0, 20, WAIT_TYPE_US, sizeof(g_bist_mode_3), g_bist_mode_3},
#else
	{DTYPE_DCS_WRITE, 0, 120, WAIT_TYPE_MS, sizeof(g_on_cmd36), g_on_cmd36},
	{DTYPE_DCS_WRITE, 0, 120, WAIT_TYPE_MS, sizeof(g_on_cmd38), g_on_cmd38},
	{DTYPE_DCS_WRITE1, 0, 100, WAIT_TYPE_US, sizeof(g_bl_level), g_bl_level},
#endif
};

/* Power OFF Sequence */
static char g_off_cmd0[] = {
	0x28
};

static char g_off_cmd1[] = {
	0x10
};

static struct dsi_cmd_desc g_lcd_display_off_cmds[] = {
	{ DTYPE_DCS_WRITE, 0, 10, WAIT_TYPE_US, sizeof(g_off_cmd0), g_off_cmd0 },
	{ DTYPE_DCS_WRITE, 0, 100, WAIT_TYPE_MS, sizeof(g_off_cmd1), g_off_cmd1 },
};

/*************************************************************************
 * LCD GPIO for fpga
 */
#define GPIO_AMOLED_RESET_NAME  "gpio_amoled_reset"
#define GPIO_AMOLED_VCC1V8_NAME "gpio_amoled_vcc1v8"
#define GPIO_AMOLED_VCC3V1_NAME "gpio_amoled_vcc3v1"
#define GPIO_AMOLED_TE0_NAME    "gpio_amoled_te0"

static uint32_t g_gpio_amoled_reset;
static uint32_t g_gpio_amoled_vcc1v8;
static uint32_t g_gpio_amoled_vcc3v1;
static uint32_t g_gpio_amoled_te0;

static struct gpio_desc g_fpga_lcd_gpio_request_cmds[] = {
	/* vcc3v1 */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0, GPIO_AMOLED_VCC3V1_NAME, &g_gpio_amoled_vcc3v1, 0},
	/* vcc1v8 */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0, GPIO_AMOLED_VCC1V8_NAME, &g_gpio_amoled_vcc1v8, 0},
	/* reset */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0, GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 0},
};

/* spec define: vcc1v8=1 --> vcc3v1=1 --> reset=1 to 0 to 1
 * spec not define delay time, so use nt37700p value
 */
static struct gpio_desc g_fpga_lcd_gpio_normal_cmds[] = {
	/* vcc1v8 enable */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 2, GPIO_AMOLED_VCC1V8_NAME, &g_gpio_amoled_vcc1v8, 1},
	/* vcc3v1 enable */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 15, GPIO_AMOLED_VCC3V1_NAME, &g_gpio_amoled_vcc3v1, 1},
	/* reset */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5, GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_US, 50, GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 15, GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 1},
	/* backlight enable */
};

/* follow nt37700p */
static struct gpio_desc g_fpga_lcd_gpio_free_cmds[] = {
	/* backlight enable */
	/* reset */
	{DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0, GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 0},
	/* vcc3v1 */
	{DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0, GPIO_AMOLED_VCC3V1_NAME, &g_gpio_amoled_vcc3v1, 0},
	/* vcc1v8 */
	{DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0, GPIO_AMOLED_VCC1V8_NAME, &g_gpio_amoled_vcc1v8, 0},
};

/* follow nt37700p */
static struct gpio_desc g_fpga_lcd_gpio_lowpower_cmds[] = {
	/* backlight enable */
	/* reset */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5, GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 0},
	/* vcc3v1 disable */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 10, GPIO_AMOLED_VCC3V1_NAME, &g_gpio_amoled_vcc3v1, 0},
	/* vcc1v8 disable */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 10, GPIO_AMOLED_VCC1V8_NAME, &g_gpio_amoled_vcc1v8, 0},

	/* backlight enable input */
	/* reset input */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_US, 100, GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 0},
	/* vcc3v1 disable */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_US, 100, GPIO_AMOLED_VCC3V1_NAME, &g_gpio_amoled_vcc3v1, 0},
	/* vcc1v8 disable */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_US, 100, GPIO_AMOLED_VCC1V8_NAME, &g_gpio_amoled_vcc1v8, 0},
};

/*************************************************************************
 * LCD GPIO for udp, current follow nt37700p
 */
static struct gpio_desc g_asic_lcd_gpio_request_cmds[] = {
	{ DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0, GPIO_AMOLED_VCC3V1_NAME, &g_gpio_amoled_vcc3v1, 0 },
	{ DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0, GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 0 },
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

static struct gpio_desc g_asic_lcd_gpio_lowpower_cmds[] = {
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20,
		GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 0 },
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20,
		GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_vcc3v1, 0 },
};

static struct gpio_desc g_asic_lcd_gpio_free_cmds[] = {
	{ DTYPE_GPIO_FREE, WAIT_TYPE_US, 50,
		GPIO_AMOLED_VCC3V1_NAME, &g_gpio_amoled_vcc3v1, 0 },
	{ DTYPE_GPIO_FREE, WAIT_TYPE_US, 50,
		GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 0 },
};

/*******************************************************************************
 * LCD VCC for udp, current follow nt37700p
 */
#define VCC_LCDIO_NAME	"lcdio-vcc"
#define VCC_LCDDIG_NAME	"lcddig-vcc"

static struct regulator *g_vcc_lcdio;
static struct regulator *g_vcc_lcddig;

static struct vcc_desc g_lcd_vcc_init_cmds[] = {
	/* vcc get */
	{ DTYPE_VCC_GET, VCC_LCDIO_NAME,
		&g_vcc_lcdio, 0, 0, WAIT_TYPE_MS, 0 },
	{ DTYPE_VCC_GET, VCC_LCDDIG_NAME,
		&g_vcc_lcddig, 0, 0, WAIT_TYPE_MS, 0 },

	/* vcc set voltage */
	{ DTYPE_VCC_SET_VOLTAGE, VCC_LCDDIG_NAME,
		&g_vcc_lcddig, 1250000, 1250000, WAIT_TYPE_MS, 0 },
	/* io set voltage */
	{ DTYPE_VCC_SET_VOLTAGE, VCC_LCDIO_NAME,
		&g_vcc_lcdio, 1850000, 1850000, WAIT_TYPE_MS, 0 },
};

static struct vcc_desc g_lcd_vcc_finit_cmds[] = {
	/* vcc put */
	{ DTYPE_VCC_PUT, VCC_LCDIO_NAME,
		&g_vcc_lcdio, 0, 0, WAIT_TYPE_MS, 0 },
	{ DTYPE_VCC_PUT, VCC_LCDDIG_NAME,
		&g_vcc_lcddig, 0, 0, WAIT_TYPE_MS, 0 },
};

static struct vcc_desc g_lcd_vcc_enable_cmds[] = {
	/* vcc enable */
	{ DTYPE_VCC_ENABLE, VCC_LCDIO_NAME,
		&g_vcc_lcdio, 0, 0, WAIT_TYPE_MS, 3 },
	{ DTYPE_VCC_ENABLE, VCC_LCDDIG_NAME,
		&g_vcc_lcddig, 0, 0, WAIT_TYPE_MS, 3 },
};

static struct vcc_desc g_lcd_vcc_disable_cmds[] = {
	/* vcc enable */
	{ DTYPE_VCC_DISABLE, VCC_LCDDIG_NAME,
		&g_vcc_lcddig, 0, 0, WAIT_TYPE_MS, 3 },
	{ DTYPE_VCC_DISABLE, VCC_LCDIO_NAME,
		&g_vcc_lcdio, 0, 0, WAIT_TYPE_MS, 3 },
};

/*******************************************************************************
 ** LCD IOMUX for udp, current follow nt37700p
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

static struct pinctrl_cmd_desc g_lcd_pinctrl_lowpower_cmds[] = {
	{DTYPE_PINCTRL_SET, &g_pctrl, DTYPE_PINCTRL_STATE_IDLE},
};

static struct pinctrl_cmd_desc g_lcd_pinctrl_finit_cmds[] = {
	{ DTYPE_PINCTRL_PUT, &g_pctrl, 0 },
};

/*******************************************************************************
 ** spr gamma/degamma setting
 */
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

static void spr_core_ctl_set(struct dkmd_connector_info *pinfo, struct spr_info *spr)
{
	spr->spr_size.value = 0;
	spr->spr_size.reg.spr_width = pinfo->base.xres - 1;
	spr->spr_size.reg.spr_height = pinfo->base.yres - 1;

	spr->spr_ctrl.value = 0;
	spr->spr_ctrl.reg.spr_hpartial_mode = 0;
	spr->spr_ctrl.reg.spr_partial_mode = 0;
	spr->spr_ctrl.reg.spr_ppro_bypass = 0;
	spr->spr_ctrl.reg.spr_dirweightmode = 1;
	spr->spr_ctrl.reg.spr_borderdet_en = 0;
	spr->spr_ctrl.reg.spr_subdir_en = 1;
	spr->spr_ctrl.reg.spr_maskprocess_en = 0;
	spr->spr_ctrl.reg.spr_multifac_en = 0;
	spr->spr_ctrl.reg.spr_bordertb_dummymode = 0;
	spr->spr_ctrl.reg.spr_borderr_dummymode = 1;
	spr->spr_ctrl.reg.spr_borderl_dummymode = 1;
	spr->spr_ctrl.reg.spr_larea_en = 0;
	spr->spr_ctrl.reg.spr_linebuf_1dmode = 0;
	spr->spr_ctrl.reg.spr_dummymode = 0;
	spr->spr_ctrl.reg.spr_subpxl_layout = 1; /* RGBG mode */
#ifdef AP_SPR_DSC1_2_EN
	spr->spr_ctrl.reg.spr_en = 1;
#else
	spr->spr_ctrl.reg.spr_en = 0;
#endif
}

static void spr_core_pixsel_set(struct spr_info *spr)
{
	spr->spr_pix_even.value = 0;
	spr->spr_pix_even.reg.spr_coeffsel_even00 = 0;
	spr->spr_pix_even.reg.spr_coeffsel_even01 = 1;
	spr->spr_pix_even.reg.spr_coeffsel_even10 = 4;
	spr->spr_pix_even.reg.spr_coeffsel_even11 = 5;
	spr->spr_pix_even.reg.spr_coeffsel_even20 = 0;
	spr->spr_pix_even.reg.spr_coeffsel_even21 = 0;

	spr->spr_pix_odd.value = 0;
	spr->spr_pix_odd.reg.spr_coeffsel_odd00 = 1;
	spr->spr_pix_odd.reg.spr_coeffsel_odd01 = 2;
	spr->spr_pix_odd.reg.spr_coeffsel_odd10 = 3;
	spr->spr_pix_odd.reg.spr_coeffsel_odd11 = 4;
	spr->spr_pix_odd.reg.spr_coeffsel_odd20 = 0;
	spr->spr_pix_odd.reg.spr_coeffsel_odd21 = 0;

	spr->spr_pix_panel.value = 0;
	spr->spr_pix_panel.reg.spr_pxlsel_even0 = 0;
	spr->spr_pix_panel.reg.spr_pxlsel_even1 = 2;
	spr->spr_pix_panel.reg.spr_pxlsel_even2 = 0;
	spr->spr_pix_panel.reg.spr_pxlsel_odd0 = 2;
	spr->spr_pix_panel.reg.spr_pxlsel_odd1 = 0;
	spr->spr_pix_panel.reg.spr_pxlsel_odd2 = 0;
}

static void spr_core_coeffs_r_set(struct spr_info *spr)
{
	spr->spr_coeff_v0h0_r0.value = 0;
	spr->spr_coeff_v0h0_r0.reg.spr_coeff_v0h0_horz_r0 = 0;
	spr->spr_coeff_v0h0_r0.reg.spr_coeff_v0h0_horz_r1 = 16;
	spr->spr_coeff_v0h0_r0.reg.spr_coeff_v0h0_horz_r2 = 16;
	spr->spr_coeff_v0h0_r0.reg.spr_coeff_v0h0_horz_r3 = 0;

	spr->spr_coeff_v0h0_r1.value = 0;
	spr->spr_coeff_v0h0_r1.reg.spr_coeff_v0h0_horz_r4 = 0;
	spr->spr_coeff_v0h0_r1.reg.spr_coeff_v0h0_vert_r0 = 12;
	spr->spr_coeff_v0h0_r1.reg.spr_coeff_v0h0_vert_r1 = 20;
	spr->spr_coeff_v0h0_r1.reg.spr_coeff_v0h0_vert_r2 = 0;

	spr->spr_coeff_v0h1_r0.value = 0;
	spr->spr_coeff_v0h1_r0.reg.spr_coeff_v0h1_horz_r0 = 0;
	spr->spr_coeff_v0h1_r0.reg.spr_coeff_v0h1_horz_r1 = 0;
	spr->spr_coeff_v0h1_r0.reg.spr_coeff_v0h1_horz_r2 = 0;
	spr->spr_coeff_v0h1_r0.reg.spr_coeff_v0h1_horz_r3 = 0;

	spr->spr_coeff_v0h1_r1.value = 0;
	spr->spr_coeff_v0h1_r1.reg.spr_coeff_v0h1_horz_r4 = 0;
	spr->spr_coeff_v0h1_r1.reg.spr_coeff_v0h1_vert_r0 = 0;
	spr->spr_coeff_v0h1_r1.reg.spr_coeff_v0h1_vert_r1 = 0;
	spr->spr_coeff_v0h1_r1.reg.spr_coeff_v0h1_vert_r2 = 0;

	spr->spr_coeff_v1h0_r0.value = 0;
	spr->spr_coeff_v1h0_r0.reg.spr_coeff_v1h0_horz_r0 = 0;
	spr->spr_coeff_v1h0_r0.reg.spr_coeff_v1h0_horz_r1 = 0;
	spr->spr_coeff_v1h0_r0.reg.spr_coeff_v1h0_horz_r2 = 0;
	spr->spr_coeff_v1h0_r0.reg.spr_coeff_v1h0_horz_r3 = 0;

	spr->spr_coeff_v1h0_r1.value = 0;
	spr->spr_coeff_v1h0_r1.reg.spr_coeff_v1h0_horz_r4 = 0;
	spr->spr_coeff_v1h0_r1.reg.spr_coeff_v1h0_vert_r0 = 0;
	spr->spr_coeff_v1h0_r1.reg.spr_coeff_v1h0_vert_r1 = 0;
	spr->spr_coeff_v1h0_r1.reg.spr_coeff_v1h0_vert_r2 = 0;

	spr->spr_coeff_v1h1_r0.value = 0;
	spr->spr_coeff_v1h1_r0.reg.spr_coeff_v1h1_horz_r0 = 0;
	spr->spr_coeff_v1h1_r0.reg.spr_coeff_v1h1_horz_r1 = 0;
	spr->spr_coeff_v1h1_r0.reg.spr_coeff_v1h1_horz_r2 = 31;
	spr->spr_coeff_v1h1_r0.reg.spr_coeff_v1h1_horz_r3 = 0;

	spr->spr_coeff_v1h1_r1.value = 0;
	spr->spr_coeff_v1h1_r1.reg.spr_coeff_v1h1_horz_r4 = 0;
	spr->spr_coeff_v1h1_r1.reg.spr_coeff_v1h1_vert_r0 = 0;
	spr->spr_coeff_v1h1_r1.reg.spr_coeff_v1h1_vert_r1 = 31;
	spr->spr_coeff_v1h1_r1.reg.spr_coeff_v1h1_vert_r2 = 0;
}

static void spr_core_coeffs_g_set(struct spr_info *spr)
{
	spr->spr_coeff_v0h0_g0.value = 0;
	spr->spr_coeff_v0h0_g0.reg.spr_coeff_v0h0_horz_g0 = 0;
	spr->spr_coeff_v0h0_g0.reg.spr_coeff_v0h0_horz_g1 = 0;
	spr->spr_coeff_v0h0_g0.reg.spr_coeff_v0h0_horz_g2 = 31;
	spr->spr_coeff_v0h0_g0.reg.spr_coeff_v0h0_horz_g3 = 0;

	spr->spr_coeff_v0h0_g1.value = 0;
	spr->spr_coeff_v0h0_g1.reg.spr_coeff_v0h0_horz_g4 = 0;
	spr->spr_coeff_v0h0_g1.reg.spr_coeff_v0h0_vert_g0 = 0;
	spr->spr_coeff_v0h0_g1.reg.spr_coeff_v0h0_vert_g1 = 31;
	spr->spr_coeff_v0h0_g1.reg.spr_coeff_v0h0_vert_g2 = 0;

	spr->spr_coeff_v0h1_g0.value = 0;
	spr->spr_coeff_v0h1_g0.reg.spr_coeff_v0h1_horz_g0 = 0;
	spr->spr_coeff_v0h1_g0.reg.spr_coeff_v0h1_horz_g1 = 0;
	spr->spr_coeff_v0h1_g0.reg.spr_coeff_v0h1_horz_g2 = 31;
	spr->spr_coeff_v0h1_g0.reg.spr_coeff_v0h1_horz_g3 = 0;

	spr->spr_coeff_v0h1_g1.value = 0;
	spr->spr_coeff_v0h1_g1.reg.spr_coeff_v0h1_horz_g4 = 0;
	spr->spr_coeff_v0h1_g1.reg.spr_coeff_v0h1_vert_g0 = 0;
	spr->spr_coeff_v0h1_g1.reg.spr_coeff_v0h1_vert_g1 = 31;
	spr->spr_coeff_v0h1_g1.reg.spr_coeff_v0h1_vert_g2 = 0;

	spr->spr_coeff_v1h0_g0.value = 0;
	spr->spr_coeff_v1h0_g0.reg.spr_coeff_v1h0_horz_g0 = 0;
	spr->spr_coeff_v1h0_g0.reg.spr_coeff_v1h0_horz_g1 = 0;
	spr->spr_coeff_v1h0_g0.reg.spr_coeff_v1h0_horz_g2 = 31;
	spr->spr_coeff_v1h0_g0.reg.spr_coeff_v1h0_horz_g3 = 0;

	spr->spr_coeff_v1h0_g1.value = 0;
	spr->spr_coeff_v1h0_g1.reg.spr_coeff_v1h0_horz_g4 = 0;
	spr->spr_coeff_v1h0_g1.reg.spr_coeff_v1h0_vert_g0 = 0;
	spr->spr_coeff_v1h0_g1.reg.spr_coeff_v1h0_vert_g1 = 31;
	spr->spr_coeff_v1h0_g1.reg.spr_coeff_v1h0_vert_g2 = 0;

	spr->spr_coeff_v1h1_g0.value = 0;
	spr->spr_coeff_v1h1_g0.reg.spr_coeff_v1h1_horz_g0 = 0;
	spr->spr_coeff_v1h1_g0.reg.spr_coeff_v1h1_horz_g1 = 0;
	spr->spr_coeff_v1h1_g0.reg.spr_coeff_v1h1_horz_g2 = 31;
	spr->spr_coeff_v1h1_g0.reg.spr_coeff_v1h1_horz_g3 = 0;

	spr->spr_coeff_v1h1_g1.value = 0;
	spr->spr_coeff_v1h1_g1.reg.spr_coeff_v1h1_horz_g4 = 0;
	spr->spr_coeff_v1h1_g1.reg.spr_coeff_v1h1_vert_g0 = 0;
	spr->spr_coeff_v1h1_g1.reg.spr_coeff_v1h1_vert_g1 = 31;
	spr->spr_coeff_v1h1_g1.reg.spr_coeff_v1h1_vert_g2 = 0;
}

static void spr_core_coeffs_b_set(struct spr_info *spr)
{
	spr->spr_coeff_v0h0_b0.value = 0;
	spr->spr_coeff_v0h0_b0.reg.spr_coeff_v0h0_horz_b0 = 0;
	spr->spr_coeff_v0h0_b0.reg.spr_coeff_v0h0_horz_b1 = 0;
	spr->spr_coeff_v0h0_b0.reg.spr_coeff_v0h0_horz_b2 = 0;
	spr->spr_coeff_v0h0_b0.reg.spr_coeff_v0h0_horz_b3 = 0;

	spr->spr_coeff_v0h0_b1.value = 0;
	spr->spr_coeff_v0h0_b1.reg.spr_coeff_v0h0_horz_b4 = 0;
	spr->spr_coeff_v0h0_b1.reg.spr_coeff_v0h0_vert_b0 = 0;
	spr->spr_coeff_v0h0_b1.reg.spr_coeff_v0h0_vert_b1 = 0;
	spr->spr_coeff_v0h0_b1.reg.spr_coeff_v0h0_vert_b2 = 0;

	spr->spr_coeff_v0h1_b0.value = 0;
	spr->spr_coeff_v0h1_b0.reg.spr_coeff_v0h1_horz_b0 = 0;
	spr->spr_coeff_v0h1_b0.reg.spr_coeff_v0h1_horz_b1 = 0;
	spr->spr_coeff_v0h1_b0.reg.spr_coeff_v0h1_horz_b2 = 16;
	spr->spr_coeff_v0h1_b0.reg.spr_coeff_v0h1_horz_b3 = 16;

	spr->spr_coeff_v0h1_b1.value = 0;
	spr->spr_coeff_v0h1_b1.reg.spr_coeff_v0h1_horz_b4 = 0;
	spr->spr_coeff_v0h1_b1.reg.spr_coeff_v0h1_vert_b0 = 12;
	spr->spr_coeff_v0h1_b1.reg.spr_coeff_v0h1_vert_b1 = 20;
	spr->spr_coeff_v0h1_b1.reg.spr_coeff_v0h1_vert_b2 = 0;

	spr->spr_coeff_v1h0_b0.value = 0;
	spr->spr_coeff_v1h0_b0.reg.spr_coeff_v1h0_horz_b0 = 0;
	spr->spr_coeff_v1h0_b0.reg.spr_coeff_v1h0_horz_b1 = 0;
	spr->spr_coeff_v1h0_b0.reg.spr_coeff_v1h0_horz_b2 = 16;
	spr->spr_coeff_v1h0_b0.reg.spr_coeff_v1h0_horz_b3 = 16;

	spr->spr_coeff_v1h0_b1.value = 0;
	spr->spr_coeff_v1h0_b1.reg.spr_coeff_v1h0_horz_b4 = 0;
	spr->spr_coeff_v1h0_b1.reg.spr_coeff_v1h0_vert_b0 = 12;
	spr->spr_coeff_v1h0_b1.reg.spr_coeff_v1h0_vert_b1 = 20;
	spr->spr_coeff_v1h0_b1.reg.spr_coeff_v1h0_vert_b2 = 0;

	spr->spr_coeff_v1h1_b0.value = 0;
	spr->spr_coeff_v1h1_b0.reg.spr_coeff_v1h1_horz_b0 = 0;
	spr->spr_coeff_v1h1_b0.reg.spr_coeff_v1h1_horz_b1 = 0;
	spr->spr_coeff_v1h1_b0.reg.spr_coeff_v1h1_horz_b2 = 0;
	spr->spr_coeff_v1h1_b0.reg.spr_coeff_v1h1_horz_b3 = 0;

	spr->spr_coeff_v1h1_b1.value = 0;
	spr->spr_coeff_v1h1_b1.reg.spr_coeff_v1h1_horz_b4 = 0;
	spr->spr_coeff_v1h1_b1.reg.spr_coeff_v1h1_vert_b0 = 0;
	spr->spr_coeff_v1h1_b1.reg.spr_coeff_v1h1_vert_b1 = 0;
	spr->spr_coeff_v1h1_b1.reg.spr_coeff_v1h1_vert_b2 = 0;
}

static void spr_core_larea_set(struct spr_info *spr)
{
	spr->spr_larea_start.value = 0;
	spr->spr_larea_start.reg.spr_larea_sx = 400;
	spr->spr_larea_start.reg.spr_larea_sy = 200;

	spr->spr_larea_end.value = 0;
	spr->spr_larea_end.reg.spr_larea_ex = 600;
	spr->spr_larea_end.reg.spr_larea_ey = 400;

	spr->spr_larea_offset.value = 0;
	spr->spr_larea_offset.reg.spr_larea_x_offset = 1;
	spr->spr_larea_offset.reg.spr_larea_y_offset = 1;

	spr->spr_larea_gain.value = 0;
	spr->spr_larea_gain.reg.spr_larea_gain_r = 128;
	spr->spr_larea_gain.reg.spr_larea_gain_g = 128;
	spr->spr_larea_gain.reg.spr_larea_gain_b = 128;

	spr->spr_larea_border_r.value = 0;
	spr->spr_larea_border_r.reg.spr_larea_border_gainl_r = 128;
	spr->spr_larea_border_r.reg.spr_larea_border_gainr_r = 128;
	spr->spr_larea_border_r.reg.spr_larea_border_gaint_r = 128;
	spr->spr_larea_border_r.reg.spr_larea_border_gainb_r = 128;

	spr->spr_larea_border_g.value = 0;
	spr->spr_larea_border_g.reg.spr_larea_border_gainl_g = 128;
	spr->spr_larea_border_g.reg.spr_larea_border_gainr_g = 128;
	spr->spr_larea_border_g.reg.spr_larea_border_gaint_g = 128;
	spr->spr_larea_border_g.reg.spr_larea_border_gainb_g = 128;

	spr->spr_larea_border_b.value = 0;
	spr->spr_larea_border_b.reg.spr_larea_border_gainl_b = 128;
	spr->spr_larea_border_b.reg.spr_larea_border_gainr_b = 128;
	spr->spr_larea_border_b.reg.spr_larea_border_gaint_b = 128;
	spr->spr_larea_border_b.reg.spr_larea_border_gainb_b = 128;
}

static void spr_core_border_set(struct spr_info *spr)
{
	spr->spr_r_borderlr.value = 0;
	spr->spr_r_borderlr.reg.spr_borderlr_gainl_r = 128;
	spr->spr_r_borderlr.reg.spr_borderlr_offsetl_r = 0;
	spr->spr_r_borderlr.reg.spr_borderlr_gainr_r = 128;
	spr->spr_r_borderlr.reg.spr_borderlr_offsetr_r = 0;

	spr->spr_r_bordertb.value = 0;
	spr->spr_r_bordertb.reg.spr_bordertb_gaint_r = 128;
	spr->spr_r_bordertb.reg.spr_bordertb_offsett_r = 0;
	spr->spr_r_bordertb.reg.spr_bordertb_gainb_r = 128;
	spr->spr_r_bordertb.reg.spr_bordertb_offsetb_r = 0;

	spr->spr_g_borderlr.value = 0;
	spr->spr_g_borderlr.reg.spr_borderlr_gainl_g = 128;
	spr->spr_g_borderlr.reg.spr_borderlr_offsetl_g = 0;
	spr->spr_g_borderlr.reg.spr_borderlr_gainr_g = 128;
	spr->spr_g_borderlr.reg.spr_borderlr_offsetr_g = 0;

	spr->spr_g_bordertb.value = 0;
	spr->spr_g_bordertb.reg.spr_bordertb_gaint_g = 128;
	spr->spr_g_bordertb.reg.spr_bordertb_offsett_g = 0;
	spr->spr_g_bordertb.reg.spr_bordertb_gainb_g = 128;
	spr->spr_g_bordertb.reg.spr_bordertb_offsetb_g = 0;

	spr->spr_b_borderlr.value = 0;
	spr->spr_b_borderlr.reg.spr_borderlr_gainl_b = 128;
	spr->spr_b_borderlr.reg.spr_borderlr_offsetl_b = 0;
	spr->spr_b_borderlr.reg.spr_borderlr_gainr_b = 128;
	spr->spr_b_borderlr.reg.spr_borderlr_offsetr_b = 0;

	spr->spr_b_bordertb.value = 0;
	spr->spr_b_bordertb.reg.spr_bordertb_gaint_b = 128;
	spr->spr_b_bordertb.reg.spr_bordertb_offsett_b = 0;
	spr->spr_b_bordertb.reg.spr_bordertb_gainb_b = 128;
	spr->spr_b_bordertb.reg.spr_bordertb_offsetb_b = 0;

	spr->spr_pixgain_reg.value = 0;
	spr->spr_pixgain_reg.reg.spr_finalgain_r = 128;
	spr->spr_pixgain_reg.reg.spr_finaloffset_r = 0;
	spr->spr_pixgain_reg.reg.spr_finalgain_g = 128;
	spr->spr_pixgain_reg.reg.spr_finaloffset_g = 0;

	spr->spr_pixgain_reg1.value = 0;
	spr->spr_pixgain_reg1.reg.spr_finalgain_b = 128;
	spr->spr_pixgain_reg1.reg.spr_finaloffset_b = 0;

	spr->spr_border_p0.value = 0;
	spr->spr_border_p0.reg.spr_borderl_sx = 0;
	spr->spr_border_p0.reg.spr_borderl_ex = 0;

	spr->spr_border_p1.value = 0;
	spr->spr_border_p1.reg.spr_borderr_sx =1079;
	spr->spr_border_p1.reg.spr_borderr_ex = 1080;

	spr->spr_border_p2.value = 0;
	spr->spr_border_p2.reg.spr_bordert_sy = 0;
	spr->spr_border_p2.reg.spr_bordert_ey = 0;

	spr->spr_border_p3.value = 0;
	spr->spr_border_p3.reg.spr_borderb_sy = 0;
	spr->spr_border_p3.reg.spr_borderb_ey = 0;
}

static void spr_core_blend_set(struct spr_info *spr)
{
	spr->spr_blend.value = 0;
	spr->spr_blend.reg.spr_gradorthblend = 31;
	spr->spr_blend.reg.spr_subdirblend = 32;

	spr->spr_weight.value = 0;
	spr->spr_weight.reg.spr_weight2bldk = 16;
	spr->spr_weight.reg.spr_weight2bldcor = 0;
	spr->spr_weight.reg.spr_weight2bldmin = 0;
	spr->spr_weight.reg.spr_weight2bldmax = 32;

	spr->spr_edgestr_r.value = 0;
	spr->spr_edgestr_r.reg.spr_edgestrbldk_r = 16;
	spr->spr_edgestr_r.reg.spr_edgestrbldcor_r = 64;
	spr->spr_edgestr_r.reg.spr_edgestrbldmin_r = 0;
	spr->spr_edgestr_r.reg.spr_edgestrbldmax_r = 128;

	spr->spr_edgestr_g.value = 0;
	spr->spr_edgestr_g.reg.spr_edgestrbldk_g = 16;
	spr->spr_edgestr_g.reg.spr_edgestrbldcor_g = 64;
	spr->spr_edgestr_g.reg.spr_edgestrbldmin_g = 0;
	spr->spr_edgestr_g.reg.spr_edgestrbldmax_g = 128;

	spr->spr_edgestr_b.value = 0;
	spr->spr_edgestr_b.reg.spr_edgestrbldk_b = 16;
	spr->spr_edgestr_b.reg.spr_edgestrbldcor_b = 64;
	spr->spr_edgestr_b.reg.spr_edgestrbldmin_b = 0;
	spr->spr_edgestr_b.reg.spr_edgestrbldmax_b = 128;

	spr->spr_dir_min.value = 0;
	spr->spr_dir_min.reg.spr_dirweibldmin_r = 0;
	spr->spr_dir_min.reg.spr_dirweibldmin_g = 0;
	spr->spr_dir_min.reg.spr_dirweibldmin_b = 0;

	spr->spr_dir_max.value = 0;
	spr->spr_dir_max.reg.spr_dirweibldmax_r = 256;
	spr->spr_dir_max.reg.spr_dirweibldmax_g = 256;
	spr->spr_dir_max.reg.spr_dirweibldmax_b = 256;
}

static void spr_core_diffdirgain_set(struct spr_info *spr)
{
	spr->spr_diff_r0.value = 0;
	spr->spr_diff_r0.reg.spr_diffdirgain_r_0 = 0;
	spr->spr_diff_r0.reg.spr_diffdirgain_r_1 = 0;
	spr->spr_diff_r0.reg.spr_diffdirgain_r_2 = 0;
	spr->spr_diff_r0.reg.spr_diffdirgain_r_3 = 0;

	spr->spr_diff_r1.value = 0;
	spr->spr_diff_r1.reg.spr_diffdirgain_r_4 = 41;
	spr->spr_diff_r1.reg.spr_diffdirgain_r_5 = 0;
	spr->spr_diff_r1.reg.spr_diffdirgain_r_6 = 0;
	spr->spr_diff_r1.reg.spr_diffdirgain_r_7 = 0;

	spr->spr_diff_g0.value = 0;
	spr->spr_diff_g0.reg.spr_diffdirgain_g_0 = 41;
	spr->spr_diff_g0.reg.spr_diffdirgain_g_1 = 0;
	spr->spr_diff_g0.reg.spr_diffdirgain_g_2 = 0;
	spr->spr_diff_g0.reg.spr_diffdirgain_g_3 = 0;

	spr->spr_diff_g1.value = 0;
	spr->spr_diff_g1.reg.spr_diffdirgain_g_4 = 0;
	spr->spr_diff_g1.reg.spr_diffdirgain_g_5 = 0;
	spr->spr_diff_g1.reg.spr_diffdirgain_g_6 = 0;
	spr->spr_diff_g1.reg.spr_diffdirgain_g_7 = 0;

	spr->spr_diff_b0.value = 0;
	spr->spr_diff_b0.reg.spr_diffdirgain_b_0 = 0;
	spr->spr_diff_b0.reg.spr_diffdirgain_b_1 = 0;
	spr->spr_diff_b0.reg.spr_diffdirgain_b_2 = 0;
	spr->spr_diff_b0.reg.spr_diffdirgain_b_3 = 0;

	spr->spr_diff_b1.value = 0;
	spr->spr_diff_b1.reg.spr_diffdirgain_b_4 = 41;
	spr->spr_diff_b1.reg.spr_diffdirgain_b_5 = 0;
	spr->spr_diff_b1.reg.spr_diffdirgain_b_6 = 0;
	spr->spr_diff_b1.reg.spr_diffdirgain_b_7 = 0;
}

static void spr_core_bd_set(struct spr_info *spr)
{
	spr->spr_horzgradblend.value = 0;
	spr->spr_horzgradblend.reg.spr_horzgradlblend = 16;
	spr->spr_horzgradblend.reg.spr_horzgradrblend = 16;

	spr->spr_horzbdbld.value = 0;
	spr->spr_horzbdbld.reg.spr_horzbdbldk = 16;
	spr->spr_horzbdbld.reg.spr_horzbdbldcor = 8;
	spr->spr_horzbdbld.reg.spr_horzbdbldmin = 0;
	spr->spr_horzbdbld.reg.spr_horzbdbldmax = 32;

	spr->spr_horzbdweight.value = 0;
	spr->spr_horzbdweight.reg.spr_horzbdweightmin = 0;
	spr->spr_horzbdweight.reg.spr_horzbdweightmax = 256;

	spr->spr_vertbdbld.value = 0;
	spr->spr_vertbdbld.reg.spr_vertbdbldk = 16;
	spr->spr_vertbdbld.reg.spr_vertbdbldcor = 8;
	spr->spr_vertbdbld.reg.spr_vertbdbldmin = 0;
	spr->spr_vertbdbld.reg.spr_vertbdbldmax = 32;

	spr->spr_vertbdweight.value = 0;
	spr->spr_vertbdweight.reg.spr_vertbdweightmin = 0;
	spr->spr_vertbdweight.reg.spr_vertbdweightmax = 256;

	spr->spr_vertbd_gain_r.value = 0;
	spr->spr_vertbd_gain_r.reg.spr_vertbd_gainlef_r = 0;
	spr->spr_vertbd_gain_r.reg.spr_vertbd_gainrig_r = 0;
	spr->spr_vertbd_gain_r.reg.spr_horzbd_gaintop_r = 50;
	spr->spr_vertbd_gain_r.reg.spr_horzbd_gainbot_r = 0;

	spr->spr_vertbd_gain_g.value = 0;
	spr->spr_vertbd_gain_g.reg.spr_vertbd_gainlef_g = 0;
	spr->spr_vertbd_gain_g.reg.spr_vertbd_gainrig_g = 0;
	spr->spr_vertbd_gain_g.reg.spr_horzbd_gaintop_g = 0;
	spr->spr_vertbd_gain_g.reg.spr_horzbd_gainbot_g = 40;

	spr->spr_vertbd_gain_b.value = 0;
	spr->spr_vertbd_gain_b.reg.spr_vertbd_gainlef_b = 0;
	spr->spr_vertbd_gain_b.reg.spr_vertbd_gainrig_b = 0;
	spr->spr_vertbd_gain_b.reg.spr_horzbd_gaintop_b = 50;
	spr->spr_vertbd_gain_b.reg.spr_horzbd_gainbot_b = 0;
}

static void spr_core_set(struct dkmd_connector_info *pinfo, struct spr_info *spr)
{
	spr_core_ctl_set(pinfo, spr);
	spr_core_pixsel_set(spr);
	spr_core_coeffs_r_set(spr);
	spr_core_coeffs_g_set(spr);
	spr_core_coeffs_b_set(spr);
	spr_core_larea_set(spr);
	spr_core_border_set(spr);
	spr_core_blend_set(spr);
	spr_core_diffdirgain_set(spr);
	spr_core_bd_set(spr);
}

static void spr_txip_set(struct dkmd_connector_info *pinfo, struct spr_info *spr)
{
	spr->txip_ctrl.value = 0;
#ifdef TXIP_RXIP_ON
	spr->txip_ctrl.reg.txip_en = 1;
#else
	spr->txip_ctrl.reg.txip_en = 0;
#endif
	spr->txip_ctrl.reg.txip_ppro_bypass = 0;
	spr->txip_ctrl.reg.txip_shift = 4;
	spr->txip_ctrl.reg.txip_rdmode = 1;

	spr->txip_size.value = 0;
	spr->txip_size.reg.txip_height = pinfo->base.yres - 1;
	spr->txip_size.reg.txip_width = pinfo->base.xres - 1;

	spr->txip_coef0.value = 0;
	spr->txip_coef0.reg.txip_coeff00 = 4;
	spr->txip_coef0.reg.txip_coeff01 = 8;
	spr->txip_coef0.reg.txip_coeff02 = 4;
	spr->txip_coef0.reg.txip_coeff03 = 0;

	spr->txip_coef1.value = 0;
	spr->txip_coef1.reg.txip_coeff10 = 8;
	spr->txip_coef1.reg.txip_coeff11 = 0;
	spr->txip_coef1.reg.txip_coeff12 = -8;
	spr->txip_coef1.reg.txip_coeff13 = 0;

	spr->txip_coef2.value = 0;
	spr->txip_coef2.reg.txip_coeff20 = -4;
	spr->txip_coef2.reg.txip_coeff21 = 4;
	spr->txip_coef2.reg.txip_coeff22 = -4;
	spr->txip_coef2.reg.txip_coeff23 = 4;

	spr->txip_coef3.value = 0;
	spr->txip_coef3.reg.txip_coeff30 = 4;
	spr->txip_coef3.reg.txip_coeff31 = 0;
	spr->txip_coef3.reg.txip_coeff32 = 4;
	spr->txip_coef3.reg.txip_coeff33 = 8;

	spr->txip_offset0.value = 0;
	spr->txip_offset0.reg.txip_offset0 = 0;
	spr->txip_offset0.reg.txip_offset1 = 512;

	spr->txip_offset1.value = 0;
	spr->txip_offset1.reg.txip_offset2 = 512;
	spr->txip_offset1.reg.txip_offset3 = 0;

	spr->txip_clip.value = 0;
	spr->txip_clip.reg.txip_clipmax = 1023;
	spr->txip_clip.reg.txip_clipmin = 0;
}

static void spr_datapack_set(struct dkmd_connector_info *pinfo, struct spr_info *spr)
{
	spr->datapack_ctrl.value = 0;
	spr->datapack_ctrl.reg.datapack_en = 1;
	spr->datapack_ctrl.reg.datapack_subpxl_layout = 1;
#ifdef CLOSE_DSC
	spr->datapack_ctrl.reg.datapack_packmode = 1;
#else
	spr->datapack_ctrl.reg.datapack_packmode = 0;
#endif
	spr->datapack_size.value = 0;
	spr->datapack_size.reg.datapack_width = pinfo->base.xres - 1;
	spr->datapack_size.reg.datapack_height = pinfo->base.yres - 1;
}

static void spr_degamma_gamma_set(struct spr_info *spr)
{
	spr->spr_gamma_en.value = 0;
	spr->spr_gamma_en.reg.spr_gama_en = 1;

	spr->spr_gamma_shiften.value = 0;
	spr->spr_gamma_shiften.reg.gama_shift_en = 0;
	spr->spr_gamma_shiften.reg.gama_mode = 1;

	spr->degamma_en.value = 0;
	spr->degamma_en.reg.degama_en = 1;

	spr->spr_lut_table = g_spr_gama_degama_table;
	spr->spr_lut_table_len = SPR_GAMMA_LUT_SIZE;
}

static void spr_param_set(struct dkmd_connector_info *pinfo, struct spr_info *spr)
{
	dpu_pr_info("+\n");

	spr_core_set(pinfo, spr);
	spr_txip_set(pinfo, spr);
	spr_datapack_set(pinfo, spr);
	spr_degamma_gamma_set(spr);

	spr->panel_xres = pinfo->base.xres;
	spr->panel_yres = pinfo->base.yres;
	dpu_pr_info("-\n");
}

#if defined(AP_SPR_DSC1_2_EN)
static void dsc_param_set(struct dkmd_connector_info *pinfo, struct dsc_calc_info *dsc)
{
	struct dsc_algorithm_manager *pt = get_dsc_algorithm_manager_instance();
	struct input_dsc_info input_dsc_info;

	if (!pt) {
		dpu_pr_err("pt is null!\n");
		return;
	}

	dpu_pr_info("+\n");

	pinfo->ifbc_type = IFBC_TYPE_VESA2X_DUAL_SPR;
	input_dsc_info.dsc_version = DSC_VERSION_V_1_2;
	input_dsc_info.format = DSC_YUV422;
	input_dsc_info.pic_width = pinfo->base.xres;
	input_dsc_info.pic_height = pinfo->base.yres;
	input_dsc_info.slice_width = 671 + 1;
	input_dsc_info.slice_height = 43 + 1;
	input_dsc_info.dsc_bpp = 16;
	input_dsc_info.dsc_bpc = DSC_8BPC;
	input_dsc_info.block_pred_enable = 1;
	input_dsc_info.linebuf_depth = LINEBUF_DEPTH_9;
	input_dsc_info.gen_rc_params = DSC_NOT_GENERATE_RC_PARAMETERS;
	pt->vesa_dsc_info_calc(&input_dsc_info, &(dsc->dsc_info), NULL);
	dsc->dsc_en = 1;
	dsc->spr_en = 1;
	dpu_pr_info("-\n");
}
#else // dsc1.1
static void dsc_param_set(struct dkmd_connector_info *pinfo, struct dsc_calc_info *dsc)
{
	pinfo->ifbc_type = IFBC_TYPE_VESA3X_DUAL;
	dsc->dsc_en = 1;

	dsc->dsc_info.dsc_version_major = 1;
	dsc->dsc_info.dsc_version_minor = 1;
	dsc->dsc_info.pic_height = pinfo->base.yres;
	dsc->dsc_info.pic_width = pinfo->base.xres;
	dsc->dsc_info.slice_height = 43 + 1;
	dsc->dsc_info.slice_width = 671 + 1;
	dsc->dsc_info.chunk_size = 0x2a0;
	dsc->dsc_info.initial_dec_delay = 0x0250;
	dsc->dsc_info.initial_scale_value = 0x20;
	dsc->dsc_info.scale_increment_interval = 0x04ba;
	dsc->dsc_info.scale_decrement_interval = 9;
	dsc->dsc_info.nfl_bpg_offset = 0x23c;
	dsc->dsc_info.slice_bpg_offset = 0x1dc;
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
}
#endif

static void ldi_lcd_init_dsi_param(struct ldi_panel_info *ldi)
{
	dpu_pr_info("[37800a probe]ldi param set");
	/* lcd porch size init */
	ldi->h_back_porch = 23;
	ldi->h_front_porch = 50;
	ldi->h_pulse_width = 20;
	ldi->v_back_porch = 80;
	ldi->v_front_porch = 40;
	ldi->v_pulse_width = 20;
}

/* dsi param initialized value from panel spec */
static void mipi_lcd_init_dsi_param(struct dkmd_connector_info *pinfo, struct mipi_panel_info *mipi)
{
	if (pinfo->base.fpga_flag == 1) {
		mipi->hsa = 13;
		mipi->hbp = 5;
		mipi->dpi_hsize = 338;
		mipi->hline_time = 1000;
		mipi->vsa = 30;
		mipi->vbp = 70;
		mipi->vfp = 20;

		mipi->dsi_bit_clk = 160;
		mipi->pxl_clk_rate = 20 * 1000000UL;
	} else {
		dpu_pr_info("[37800a probe] udp mipi param set\n");
		mipi->hsa = 13;
		mipi->hbp = 5;
		mipi->dpi_hsize = 338;
		mipi->hline_time = MIPI_HLINE_TIME;
		mipi->vsa = 24;
		mipi->vbp = 58;
		mipi->vfp = 20;

		mipi->dsi_bit_clk = 550;
		mipi->pxl_clk_rate = 192 * 1000000UL;
	}

	mipi->dsi_bit_clk_upt_support = 0;
	mipi->dsi_bit_clk_upt = mipi->dsi_bit_clk;
	mipi->dsi_bit_clk_default = mipi->dsi_bit_clk;
	mipi->pxl_clk_rate_div = 1;

	mipi->clk_post_adjust = 215;
	mipi->lane_nums = DSI_4_LANES;
	mipi->color_mode = DSI_24BITS_1;

	mipi->vc = 0;
	mipi->max_tx_esc_clk = 10 * 1000000;
	mipi->burst_mode = DSI_BURST_SYNC_PULSES_1;
	mipi->non_continue_en = 1;
	mipi->phy_mode = DPHY_MODE;
	mipi->dsi_version = DSI_1_1_VERSION;
}

static void panel_drv_private_data_setup(struct panel_drv_private *priv, struct device_node *np)
{
	if (priv->connector_info.base.fpga_flag == 1) {
		g_gpio_amoled_reset = (uint32_t)of_get_named_gpio(np, "gpios", 0);
		g_gpio_amoled_vcc3v1 = (uint32_t)of_get_named_gpio(np, "gpios", 1);
		g_gpio_amoled_vcc1v8 = (uint32_t)of_get_named_gpio(np, "gpios", 2);

		dpu_pr_info("used gpio:[rst: %d, vcc3v1: %d, tp1v8: %d]\n",
			g_gpio_amoled_reset, g_gpio_amoled_vcc3v1, g_gpio_amoled_vcc1v8);

		priv->gpio_request_cmds = g_fpga_lcd_gpio_request_cmds;
		priv->gpio_request_cmds_len = ARRAY_SIZE(g_fpga_lcd_gpio_request_cmds);
		priv->gpio_free_cmds = g_fpga_lcd_gpio_free_cmds;
		priv->gpio_free_cmds_len = ARRAY_SIZE(g_fpga_lcd_gpio_free_cmds);

		priv->gpio_normal_cmds = g_fpga_lcd_gpio_normal_cmds;
		priv->gpio_normal_cmds_len = ARRAY_SIZE(g_fpga_lcd_gpio_normal_cmds);
		priv->gpio_lowpower_cmds = g_fpga_lcd_gpio_lowpower_cmds;
		priv->gpio_lowpower_cmds_len = ARRAY_SIZE(g_fpga_lcd_gpio_lowpower_cmds);
	} else {
		g_gpio_amoled_reset = (uint32_t)of_get_named_gpio(np, "gpios", 0);
		g_gpio_amoled_vcc3v1 = (uint32_t)of_get_named_gpio(np, "gpios", 1);
		g_gpio_amoled_te0 = (uint32_t)of_get_named_gpio(np, "gpios", 2);

		dpu_pr_info("used gpio:[rst: %d, vcc3v1: %d, te0: %d]\n",
			g_gpio_amoled_reset, g_gpio_amoled_vcc3v1, g_gpio_amoled_te0);

		priv->gpio_request_cmds = g_asic_lcd_gpio_request_cmds;
		priv->gpio_request_cmds_len = ARRAY_SIZE(g_asic_lcd_gpio_request_cmds);
		priv->gpio_free_cmds = g_asic_lcd_gpio_free_cmds;
		priv->gpio_free_cmds_len = ARRAY_SIZE(g_asic_lcd_gpio_free_cmds);

		priv->gpio_normal_cmds = g_asic_lcd_gpio_normal_cmds;
		priv->gpio_normal_cmds_len = ARRAY_SIZE(g_asic_lcd_gpio_normal_cmds);
		priv->gpio_lowpower_cmds = g_asic_lcd_gpio_lowpower_cmds;
		priv->gpio_lowpower_cmds_len = (uint32_t)ARRAY_SIZE(g_asic_lcd_gpio_lowpower_cmds);

		priv->vcc_init_cmds = g_lcd_vcc_init_cmds;
		priv->vcc_init_cmds_len = ARRAY_SIZE(g_lcd_vcc_init_cmds);
		priv->vcc_finit_cmds = g_lcd_vcc_finit_cmds;
		priv->vcc_finit_cmds_len = ARRAY_SIZE(g_lcd_vcc_finit_cmds);

		priv->vcc_enable_cmds = g_lcd_vcc_enable_cmds;
		priv->vcc_enable_cmds_len = ARRAY_SIZE(g_lcd_vcc_enable_cmds);
		priv->vcc_disable_cmds = g_lcd_vcc_disable_cmds;
		priv->vcc_disable_cmds_len = ARRAY_SIZE(g_lcd_vcc_disable_cmds);

		priv->pinctrl_init_cmds = g_lcd_pinctrl_init_cmds;
		priv->pinctrl_init_cmds_len = ARRAY_SIZE(g_lcd_pinctrl_init_cmds);
		priv->pinctrl_finit_cmds = g_lcd_pinctrl_finit_cmds;
		priv->pinctrl_finit_cmds_len = ARRAY_SIZE(g_lcd_pinctrl_finit_cmds);

		priv->pinctrl_normal_cmds = g_lcd_pinctrl_normal_cmds;
		priv->pinctrl_normal_cmds_len = ARRAY_SIZE(g_lcd_pinctrl_normal_cmds);
		priv->pinctrl_lowpower_cmds = g_lcd_pinctrl_lowpower_cmds;
		priv->pinctrl_lowpower_cmds_len = ARRAY_SIZE(g_lcd_pinctrl_lowpower_cmds);
	}
}

static int32_t panel_of_device_setup(struct panel_drv_private *priv)
{
	int32_t ret;
	struct dkmd_connector_info *pinfo = &priv->connector_info;
	struct device_node *np = priv->pdev->dev.of_node;
	struct dpu_connector *connector = NULL;

	connector = get_primary_connector(pinfo);
	if (!connector) {
		dpu_pr_err("pipe_sw post_ch=%u is not available!\n", pinfo->sw_post_chn_idx[PRIMARY_CONNECT_CHN_IDX]);
		return -1;
	}

	dpu_pr_info("enter!\n");

	/* Inheritance based processing */
	panel_base_of_device_setup(priv);
	panel_drv_private_data_setup(priv, np);

	/* 1. config base object info
	 * would be used for framebuffer setup
	 */
	pinfo->base.xres = 1344; // FIXME: Modified for new panel device
	pinfo->base.yres = 2772; // FIXME: Modified for new panel device

	/* When calculating DPI needs the following parameters */
	pinfo->base.width = 74; // FIXME: Modified for new panel device
	pinfo->base.height = 154; // FIXME: Modified for new panel device

	/* report to surfaceFlinger, remain: caculate fps by mipi timing para */
#ifdef DFR90
	pinfo->base.fps = 90;
#else
	pinfo->base.fps = 60;
#endif

	/* 2. config connector info
	 * would be used for dsi & composer setup
	 */
	ldi_lcd_init_dsi_param(&connector->ldi);
	mipi_lcd_init_dsi_param(pinfo, &connector->mipi);
	spr_param_set(pinfo, &connector->spr);
	dsc_param_set(pinfo, &connector->dsc);

	/* dsi or composer need this param */
	pinfo->dirty_region_updt_support = 0;

	/* 3. config panel private info
	 * would be used for panel setup
	 */
	priv->bl_info.bl_min = 12;
	priv->bl_info.bl_max = 2047;
	priv->bl_info.bl_default = 2047;

	priv->disp_on_cmds = g_lcd_display_on_cmds;
	priv->disp_on_cmds_len = ARRAY_SIZE(g_lcd_display_on_cmds);
	priv->disp_off_cmds = g_lcd_display_off_cmds;
	priv->disp_off_cmds_len = ARRAY_SIZE(g_lcd_display_off_cmds);

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

panel_device_match_data(nt37800a_panel_info, PANEL_NT37800A_ID, panel_of_device_setup, panel_of_device_release);

MODULE_LICENSE("GPL");

