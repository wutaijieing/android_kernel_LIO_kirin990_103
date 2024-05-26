/* Copyright (c) 2020-2020,Tech. Co., Ltd. All rights reserved.
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

#ifndef MIPI_SAMSUNG_EA8076_H
#define MIPI_SAMSUNG_EA8076_H
#include "dpu_fb.h"
#define DTS_COMP_SAMSUNG_EA8076 "hisilicon,mipi_samsung_EA8076"
static uint32_t read_value[4] = {0};
static uint32_t expected_value[4] = {0x9c, 0x00, 0x00, 0x80};
static uint32_t read_mask[4] = {0xFF, 0xFF, 0xFF, 0xFF};
static char* reg_name[4] = {"power mode", "MADCTR", "image mode", "dsi error"};
static char lcd_reg_0a[] = {0x0a};
static char lcd_reg_0b[] = {0x0b};
static char lcd_reg_0d[] = {0x0d};
static char lcd_reg_0e[] = {0x0e};

static struct dsi_cmd_desc lcd_check_reg[] = {
	{DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
		sizeof(lcd_reg_0a), lcd_reg_0a},
	{DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
		sizeof(lcd_reg_0b), lcd_reg_0b},
	{DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
		sizeof(lcd_reg_0d), lcd_reg_0d},
	{DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
		sizeof(lcd_reg_0e), lcd_reg_0e},
};

static struct mipi_dsi_read_compare_data data = {
	.read_value = read_value,
	.expected_value = expected_value,
	.read_mask = read_mask,
	.reg_name = reg_name,
	.log_on = 1,
	.cmds = lcd_check_reg,
	.cnt = ARRAY_SIZE(lcd_check_reg),
};

static char display_off[] = {
	0x28,
};
static char enter_sleep[] = {
	0x10,
};

static char g_cmd0[] = {
	0x11,
	0x00,
};

static char g_cmd1[] = {
	0xF0,
	0x5A, 0x5A,
};

static char g_cmd2[] = {
	0x35,
	0x00,
};

static char g_cmd3[] = {
	0xF0,
	0xA5, 0xA5,
};

static char g_cmd4[] = {
	0x2B,
	0x00, 0x00, 0x09, 0x23,
};

static char g_cmd5[] = {
	0xF0,
	0x5A, 0x5A,
};

static char g_cmd6[] = {
	0xE1,
	0x00, 0x00, 0x02, 0x02,
	0x42, 0x02,
};

static char g_cmd7[] = {
	0xB0,
	0x0C,
};

static char g_cmd8[] = {
	0xE1,
	0x08,
};

static char g_cmd9[] = {
	0xF0,
	0xA5, 0xA5,
};

static char g_cmd10[] = {
	0x55,
	0x00,
};

static char g_cmd11[] = {
	0xF0,
	0x5A, 0x5A,
};

static char g_cmd12[] = {
	0x53,
	0x20,
};

static char g_cmd13[] = {
	0xF0,
	0xA5, 0xA5,
};

static char g_cmd14[] = {
	0x29,
	0x00,
};

static char lcd_disp_x[] = {
	0x2A,
	0x00, 0x00, 0x04, 0x37
};

static char lcd_disp_y[] = {
	0x2B,
	0x00, 0x00, 0x08, 0xBF
};

/*******************************************************************************
** Power ON/OFF Sequence(sleep mode to Normal mode) begin
*/
static struct dsi_cmd_desc display_on_cmds[] = {
	{DTYPE_DCS_WRITE, 0,10, WAIT_TYPE_MS,
		sizeof(g_cmd0), g_cmd0},
	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US,
		sizeof(g_cmd1), g_cmd1},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US,
		sizeof(g_cmd2), g_cmd2},
	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US,
		sizeof(g_cmd3), g_cmd3},
	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US,
		sizeof(g_cmd4), g_cmd4},
	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US,
		sizeof(g_cmd5), g_cmd5},
	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US,
		sizeof(g_cmd6), g_cmd6},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US,
		sizeof(g_cmd7), g_cmd7},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US,
		sizeof(g_cmd8), g_cmd8},
	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US,
		sizeof(g_cmd9), g_cmd9},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US,
		sizeof(g_cmd10), g_cmd10},
	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US,
		sizeof(g_cmd11), g_cmd11},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US,
		sizeof(g_cmd12), g_cmd12},
	{DTYPE_DCS_LWRITE, 0,110, WAIT_TYPE_MS,
		sizeof(g_cmd13), g_cmd13},

	{DTYPE_DCS_WRITE, 0,10, WAIT_TYPE_US,
		sizeof(g_cmd14), g_cmd14},
};

static struct dsi_cmd_desc lcd_display_off_cmd[] = {
	{DTYPE_DCS_WRITE, 0, 20, WAIT_TYPE_MS, sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 0, 120, WAIT_TYPE_MS, sizeof(enter_sleep), enter_sleep},
};

/*******************************************************************************
** LCD VCC
*/
#define VCC_LCDIO_NAME  "lcdio-vcc"
#define VCC_LCDDIG_NAME "lcddig-vcc"

static struct regulator *vcc_lcdio;
static struct regulator *vcc_lcddig;

static struct vcc_desc lcd_vcc_init_cmds[] = {
	/* vcc get */
	{DTYPE_VCC_GET, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0, WAIT_TYPE_MS, 0},

	/* io set voltage */
	{DTYPE_VCC_SET_VOLTAGE, VCC_LCDIO_NAME, &vcc_lcdio, 1800000, 1800000, WAIT_TYPE_MS, 0},
};

static struct vcc_desc lcd_vcc_finit_cmds[] = {
	/* vcc put */
	{DTYPE_VCC_PUT, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0, WAIT_TYPE_MS, 0},
};

static struct vcc_desc lcd_vcc_enable_cmds[] = {
	/* vcc enable */
	{DTYPE_VCC_ENABLE, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0, WAIT_TYPE_MS, 10},
};

static struct vcc_desc lcd_vcc_disable_cmds[] = {
	/* vcc disable */
	{DTYPE_VCC_DISABLE, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0, WAIT_TYPE_MS, 10},
};

static struct vcc_desc lcd_vcc_init_dig_cmds[] = {
	/* vci1v2 get */
	{DTYPE_VCC_GET, VCC_LCDDIG_NAME, &vcc_lcddig, 0, 0, WAIT_TYPE_MS, 0},

	/* vci1v2 set voltage */
	{DTYPE_VCC_SET_VOLTAGE, VCC_LCDDIG_NAME, &vcc_lcddig, 1200000, 1200000, WAIT_TYPE_MS, 0},
};

static struct vcc_desc lcd_vcc_finit_dig_cmds[] = {
	/* vci1v2 put */
	{DTYPE_VCC_PUT, VCC_LCDDIG_NAME, &vcc_lcddig, 0, 0, WAIT_TYPE_MS, 0},
};

static struct vcc_desc lcd_vcc_enable_dig_cmds[] = {
	/* vci1v2 enable */
	{DTYPE_VCC_ENABLE, VCC_LCDDIG_NAME, &vcc_lcddig, 0, 0, WAIT_TYPE_MS, 10},
};

static struct vcc_desc lcd_vcc_disable_dig_cmds[] = {
	/* vci1v2 disable */
	{DTYPE_VCC_DISABLE, VCC_LCDDIG_NAME, &vcc_lcddig, 0, 0, WAIT_TYPE_MS, 10},
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
#define GPIO_AMOLED_RESET_NAME  "gpio_amoled_reset"
#define GPIO_AMOLED_TE0_NAME    "gpio_amoled_te0"
#define GPIO_AMOLED_VCI3V1_NAME "gpio_amoled_vci3v1"
#define GPIO_AMOLED_VCC1V8_NAME "gpio_amoled_vcc1v8"
static uint32_t gpio_amoled_reset;
static uint32_t gpio_amoled_te0;
static uint32_t gpio_amoled_vci3v1;
static uint32_t gpio_amoled_vcc1v8;

static struct gpio_desc fpga_lcd_gpio_request_cmds[] = {
	/* vcc1v8 */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_AMOLED_VCC1V8_NAME, &gpio_amoled_vcc1v8, 0},
	/* vcc3v1 */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_AMOLED_VCI3V1_NAME, &gpio_amoled_vci3v1, 0},
	/* reset */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_AMOLED_RESET_NAME, &gpio_amoled_reset, 0},
};

static struct gpio_desc fpga_lcd_gpio_normal_cmds[] = {
	/* vcc3v1 enable */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 15,
		GPIO_AMOLED_VCI3V1_NAME, &gpio_amoled_vci3v1, 1},
	/* vcc1v8 enable */
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 2,
		GPIO_AMOLED_VCC1V8_NAME, &gpio_amoled_vcc1v8, 0 },
	/* reset */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_AMOLED_RESET_NAME, &gpio_amoled_reset, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_US, 50,
		GPIO_AMOLED_RESET_NAME, &gpio_amoled_reset, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 15,
		GPIO_AMOLED_RESET_NAME, &gpio_amoled_reset, 1},
	/* backlight enable */
};

static struct gpio_desc lcd_gpio_off_cmds[] = {
	/* vcc3v1 disable */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 15,
		GPIO_AMOLED_VCI3V1_NAME, &gpio_amoled_vci3v1, 0},
	/* reset */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20,
		GPIO_AMOLED_RESET_NAME, &gpio_amoled_reset, 0},
};

static struct gpio_desc lcd_gpio_free_cmds[] = {
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 50,
		GPIO_AMOLED_VCI3V1_NAME, &gpio_amoled_vci3v1, 0},

	/* reset */
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 50,
		GPIO_AMOLED_RESET_NAME, &gpio_amoled_reset, 0},
};

static struct gpio_desc asic_lcd_gpio_enable_analog_cmds[] = {
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 10,
		GPIO_AMOLED_VCI3V1_NAME, &gpio_amoled_vci3v1, 1},
};

static struct gpio_desc asic_lcd_gpio_disable_analog_cmds[] = {
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 10,
		GPIO_AMOLED_VCI3V1_NAME, &gpio_amoled_vci3v1, 0},
};

static struct gpio_desc asic_lcd_gpio_request_cmds[] = {
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_AMOLED_VCI3V1_NAME, &gpio_amoled_vci3v1, 0},
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_AMOLED_RESET_NAME, &gpio_amoled_reset, 0},
};

static struct gpio_desc asic_lcd_gpio_normal_cmds[] = {
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_US, 50,
		GPIO_AMOLED_RESET_NAME, &gpio_amoled_reset, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_US, 20,
		GPIO_AMOLED_RESET_NAME, &gpio_amoled_reset, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_AMOLED_RESET_NAME, &gpio_amoled_reset, 1},
};

static struct gpio_desc asic_lcd_gpio_lowpower_cmds[] = {
	/* reset */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20,
		GPIO_AMOLED_RESET_NAME, &gpio_amoled_reset, 0},
};

static struct gpio_desc asic_lcd_gpio_free_cmds[] = {
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 50,
		GPIO_AMOLED_VCI3V1_NAME, &gpio_amoled_vci3v1, 0},

	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 50,
		GPIO_AMOLED_RESET_NAME, &gpio_amoled_reset, 0},
};
#endif