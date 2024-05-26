/* Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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

#ifdef DFR120
#define MIPI_HLINE_TIME 385 /* 120hz */
#else
#define MIPI_HLINE_TIME 740 /* 60hz */
#endif

/*******************************************************************************
 ** Power ON/OFF Sequence(sleep mode to Normal mode) begin
 */
static char g_display_off0[] = {
	0x22,
};

static char g_display_off1[] = {
	0x28,
};

static char g_enter_sleep[] = {
	0x10,
};

static char g_on_cmd0[] = {
	0xF0,
	0x55, 0xAA, 0x52, 0x08, 0x00,
};

static char g_on_cmd1[] = {
	0x6F,
	0x02
};

static char g_on_cmd2[] = {
	0xB5,
	0x46,
};

static char g_on_cmd3[] = {
	0xBE,
	0x0E, 0x09, 0x14, 0x13,
};

static char g_on_cmd4[] = {
	0xCA,
	0x12,
};

static char g_on_cmd5[] = {
	0xF0,
	0x55, 0xAA, 0x52, 0x08, 0x01,
};

static char g_on_cmd6[] = {
	0xC3,
	0x97, 0x01, 0x5A, 0xD0, 0x20, 0x81, 0x00,
};

static char g_on_cmd7[] = {
	0xCD,
	0x05, 0x31,
};

static char g_on_cmd8[] = {
	0xD1,
	0x07, 0x02, 0x18,
};

static char g_on_cmd9[] = {
	0xD2,
	0x00, 0x40, 0x11,
};

static char g_on_cmd10[] = {
	0xCE,
	0x04, 0x00, 0x01, 0x00, 0x07,
};

static char g_on_cmd11[] = {
	0xF0,
	0x55, 0xAA, 0x52, 0x08, 0x03,
};

static char g_on_cmd12[] = {
	0x6F,
	0x0F,
};

static char g_on_cmd13[] = {
	0xB6,
	0x1F,
};

static char g_on_cmd14[] = {
	0x6F,
	0x18,
};

static char g_on_cmd15[] = {
	0xB6,
	0x0F,
};

static char g_on_cmd16[] = {
	0x6F,
	0x21,
};

static char g_on_cmd17[] = {
	0xB6,
	0x0F,
};

static char g_on_cmd18[] = {
	0x6F,
	0x2A,
};

static char g_on_cmd19[] = {
	0xB6,
	0x0F,
};

#ifdef DFR120
static char g_on_cmd_120hz[] = {
	0x2F,
	0x02,
};
#endif

static char g_on_cmd20[] = {
	0xF0,
	0x55, 0xAA, 0x52, 0x08, 0x05,
};

static char g_on_cmd21[] = {
	0xB5,
	0x86, 0x81,
};

static char g_on_cmd22[] = {
	0xB7,
	0x86, 0x00, 0x00, 0x81,
};

static char g_on_cmd23[] = {
	0xB8,
	0x85, 0x00, 0x00, 0x81,
};

static char g_on_cmd24[] = {
	0x35,
	0x00,
};

static char g_on_cmd25[] = {
	0x53,
	0x20,
};

static char g_on_cmd26[] = {
	0x82,
	0xAC,
};

static char g_on_cmd27[] = {
	0x2A,
	0x00, 0x00, 0x04, 0xD3,
};

static char g_on_cmd28[] = {
	0x2B,
	0x00, 0x00, 0x0A, 0x73,
};

#ifdef CLOSE_DSC
static char g_on_cmd29[] = {
	0x03,
	0x00,
};

static char g_on_cmd30[] = {
	0x90,
	0x02,
};
#else
static char g_on_cmd29[] = {
	0x03,
	0x01,
};

static char g_on_cmd30[] = {
	0x90,
	0x01,
};
#endif

static char g_on_cmd31[] = {
	0x91,
	0xAB, 0x28, 0x00, 0x0C, 0xC2, 0x00, 0x03, 0x6A, 0x01, 0x8E, 0x00, 0x11, 0x08, 0xBB, 0x03, 0xB4, 0x10, 0xF0,
};

static char g_on_cmd32[] = {
	0x44,
	0x0A, 0x6C,
};

static char g_on_cmd33[] = {
	0x11
};

static char g_on_cmd34[] = {
	0x29
};

static char g_bl_level[] = { /* set backlight */
	0x51, 0xff
};

static struct dsi_cmd_desc g_lcd_display_on_cmds[] = {
	{ DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd0),   g_on_cmd0 },
	{ DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd1),   g_on_cmd1 },
	{ DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd2),   g_on_cmd2 },
	{ DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd3),   g_on_cmd3 },
	{ DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd4),   g_on_cmd4 },
	{ DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd5),   g_on_cmd5 },
	{ DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd6),   g_on_cmd6 },
	{ DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd7),   g_on_cmd7 },
	{ DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd8),   g_on_cmd8 },
	{ DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd9),   g_on_cmd9 },
	{ DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd10), g_on_cmd10 },
	{ DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd11), g_on_cmd11 },
	{ DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd12), g_on_cmd12 },
	{ DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd13), g_on_cmd13 },
	{ DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd14), g_on_cmd14 },
	{ DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd15), g_on_cmd15 },
	{ DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd16), g_on_cmd16 },
	{ DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd17), g_on_cmd17 },
	{ DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd18), g_on_cmd18 },
	{ DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd19), g_on_cmd19 },
	// switch 120hz cmd  from xml
#ifdef DFR120
	{ DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd_120hz), g_on_cmd_120hz},
#endif
	{ DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd20), g_on_cmd20 },
	{ DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd21), g_on_cmd21 },
	{ DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd22), g_on_cmd22 },
	{ DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd23), g_on_cmd23 },
	{ DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd24), g_on_cmd24 },
	{ DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd25), g_on_cmd25 },
	{ DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd26), g_on_cmd26 },
	{ DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd27), g_on_cmd27 },
	{ DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd28), g_on_cmd28 },
	{ DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd29), g_on_cmd29 },
	{ DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd30), g_on_cmd30 },
	{ DTYPE_DCS_LWRITE, 0, 19, WAIT_TYPE_US, sizeof(g_on_cmd31), g_on_cmd31 },
	{ DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US, sizeof(g_on_cmd32), g_on_cmd32 },
	{ DTYPE_DCS_WRITE,  0, 70, WAIT_TYPE_MS, sizeof(g_on_cmd33), g_on_cmd33 },
	{ DTYPE_DCS_WRITE,  0, 10, WAIT_TYPE_US, sizeof(g_on_cmd34), g_on_cmd34 },
	{ DTYPE_DCS_WRITE1, 0, 100, WAIT_TYPE_US, sizeof(g_bl_level), g_bl_level},

};

static struct dsi_cmd_desc g_lcd_display_off_cmds[] = {
	{ DTYPE_DCS_WRITE, 0, 22, WAIT_TYPE_MS, sizeof(g_display_off0), g_display_off0 },
	{ DTYPE_DCS_WRITE, 0, 10, WAIT_TYPE_MS, sizeof(g_display_off1), g_display_off1 },
	{ DTYPE_DCS_WRITE, 0, 120, WAIT_TYPE_MS, sizeof(g_enter_sleep), g_enter_sleep },
};

/*******************************************************************************
 ** LCD GPIO
 */
#define GPIO_AMOLED_RESET_NAME "gpio_amoled_reset"
#define GPIO_AMOLED_VCC1V8_NAME "gpio_amoled_vcc1v8"
#define GPIO_AMOLED_VCC3V1_NAME "gpio_amoled_vcc3v1"
#define GPIO_AMOLED_TE0_NAME "gpio_amoled_te0"

static uint32_t g_gpio_amoled_reset;
static uint32_t g_gpio_amoled_vcc1v8;
static uint32_t g_gpio_amoled_vcc3v1;
static uint32_t g_gpio_amoled_te0;


static struct gpio_desc g_fpga_lcd_gpio_request_cmds[] = {
	/* vcc3v1 */
	{ DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0, GPIO_AMOLED_VCC3V1_NAME, &g_gpio_amoled_vcc3v1, 0 },
	/* vcc1v8 */
	{ DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0, GPIO_AMOLED_VCC1V8_NAME, &g_gpio_amoled_vcc1v8, 0 },

	/* reset */
	{ DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0, GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 0 },
};

static struct gpio_desc g_fpga_lcd_gpio_normal_cmds[] = {
	/* vcc1v8 enable */
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 2, GPIO_AMOLED_VCC1V8_NAME, &g_gpio_amoled_vcc1v8, 1 },
	/* vcc3v1 enable */
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 15, GPIO_AMOLED_VCC3V1_NAME, &g_gpio_amoled_vcc3v1, 1 },
	/* reset */
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5, GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 1 },
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_US, 50, GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 0 },
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 15, GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 1 },
	/* backlight enable */
};

static struct gpio_desc g_fpga_lcd_gpio_lowpower_cmds[] = {
	/* vcc1v8 disable */
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 2, GPIO_AMOLED_VCC1V8_NAME, &g_gpio_amoled_vcc1v8, 0 },
	/* vcc3v1 disable */
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 15, GPIO_AMOLED_VCC3V1_NAME, &g_gpio_amoled_vcc3v1, 0 },
	/* reset */
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5, GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 0 },
};

static struct gpio_desc g_fpga_lcd_gpio_free_cmds[] = {
	/* vcc3v1 */
	{ DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0, GPIO_AMOLED_VCC3V1_NAME, &g_gpio_amoled_vcc3v1, 0 },
	/* vcc1v8 */
	{ DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0, GPIO_AMOLED_VCC1V8_NAME, &g_gpio_amoled_vcc1v8, 0 },
	/* reset */
	{ DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0, GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 0 },
};

static struct gpio_desc g_asic_lcd_gpio_request_cmds[] = {
	{ DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0, GPIO_AMOLED_VCC3V1_NAME, &g_gpio_amoled_vcc3v1, 0 },
	{ DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0, GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 0 },
};

static struct gpio_desc g_asic_lcd_gpio_free_cmds[] = {
	{ DTYPE_GPIO_FREE, WAIT_TYPE_US, 50, GPIO_AMOLED_VCC3V1_NAME, &g_gpio_amoled_vcc3v1, 0 },
	{ DTYPE_GPIO_FREE, WAIT_TYPE_US, 50, GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 0 },
};

static struct gpio_desc g_asic_lcd_gpio_normal_cmds[] = {
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20, GPIO_AMOLED_VCC3V1_NAME, &g_gpio_amoled_vcc3v1, 1 },
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_US, 50, GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 1 },
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_US, 50, GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 0 },
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 15, GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 1 },
};

static struct gpio_desc g_asic_lcd_gpio_lowpower_cmds[] = {
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20, GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 0 },
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20, GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_vcc3v1, 0 },
};

/*******************************************************************************
 ** LCD VCC
 */
#define VCC_LCDIO_NAME	"lcdio-vcc"
#define VCC_LCDDIG_NAME	"lcddig-vcc"

static struct regulator *g_vcc_lcdio;
static struct regulator *g_vcc_lcddig;

static struct vcc_desc g_lcd_vcc_init_cmds[] = {
	/* vcc get */
	{DTYPE_VCC_GET, VCC_LCDIO_NAME, &g_vcc_lcdio, 0, 0, WAIT_TYPE_MS, 0},
	{ DTYPE_VCC_GET, VCC_LCDDIG_NAME, &g_vcc_lcddig, 0, 0, WAIT_TYPE_MS, 0 },

	/* vcc set voltage */
	{ DTYPE_VCC_SET_VOLTAGE, VCC_LCDDIG_NAME, &g_vcc_lcddig, 1250000, 1250000, WAIT_TYPE_MS, 0 },
	/* io set voltage */
	{ DTYPE_VCC_SET_VOLTAGE, VCC_LCDIO_NAME, &g_vcc_lcdio, 1850000, 1850000, WAIT_TYPE_MS, 0 },
};

static struct vcc_desc g_lcd_vcc_finit_cmds[] = {
	/* vcc put */
	{ DTYPE_VCC_PUT, VCC_LCDIO_NAME, &g_vcc_lcdio, 0, 0, WAIT_TYPE_MS, 0 },
	{ DTYPE_VCC_PUT, VCC_LCDDIG_NAME, &g_vcc_lcddig, 0, 0, WAIT_TYPE_MS, 0 },
};

static struct vcc_desc g_lcd_vcc_enable_cmds[] = {
	/* vcc enable */
	{ DTYPE_VCC_ENABLE, VCC_LCDIO_NAME, &g_vcc_lcdio, 0, 0, WAIT_TYPE_MS, 3 },
	{ DTYPE_VCC_ENABLE, VCC_LCDDIG_NAME, &g_vcc_lcddig, 0, 0, WAIT_TYPE_MS, 3 },
};

static struct vcc_desc g_lcd_vcc_disable_cmds[] = {
	/* vcc enable */
	{ DTYPE_VCC_DISABLE, VCC_LCDDIG_NAME, &g_vcc_lcddig, 0, 0, WAIT_TYPE_MS, 3 },
	{ DTYPE_VCC_DISABLE, VCC_LCDIO_NAME, &g_vcc_lcdio, 0, 0, WAIT_TYPE_MS, 3 },
};

/*******************************************************************************
 ** LCD IOMUX
 */
static struct pinctrl_data g_pctrl;

static struct pinctrl_cmd_desc g_lcd_pinctrl_init_cmds[] = {
	{DTYPE_PINCTRL_GET, &g_pctrl, 0},
	{DTYPE_PINCTRL_STATE_GET, &g_pctrl, DTYPE_PINCTRL_STATE_DEFAULT},
	{DTYPE_PINCTRL_STATE_GET, &g_pctrl, DTYPE_PINCTRL_STATE_IDLE},
};

static struct pinctrl_cmd_desc g_lcd_pinctrl_normal_cmds[] = {
	{DTYPE_PINCTRL_SET, &g_pctrl, DTYPE_PINCTRL_STATE_DEFAULT},
};

static struct pinctrl_cmd_desc g_lcd_pinctrl_lowpower_cmds[] = {
	{DTYPE_PINCTRL_SET, &g_pctrl, DTYPE_PINCTRL_STATE_IDLE},
};

static struct pinctrl_cmd_desc g_lcd_pinctrl_finit_cmds[] = {
	{DTYPE_PINCTRL_PUT, &g_pctrl, 0},
};

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
		priv->gpio_lowpower_cmds_len = ARRAY_SIZE(g_asic_lcd_gpio_lowpower_cmds);

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

#ifndef CLOSE_DSC
/* IFBC_TYPE_VESA3_75X_SINGLE */
static void dsc_vesa3_75x_single_config(struct dkmd_connector_info *pinfo, struct dsc_calc_info *dsc)
{
	pinfo->ifbc_type = IFBC_TYPE_VESA3_75X_SINGLE;
	dsc->dsc_en = 1;

	dsc->dsc_info.dsc_version_major = 1;
	dsc->dsc_info.dsc_version_minor = 1;
	dsc->dsc_info.pic_height = pinfo->base.yres;
	dsc->dsc_info.pic_width = pinfo->base.xres;
	dsc->dsc_info.chunk_size = 0x4d4;
	dsc->dsc_info.dsc_bpc = 10;
	dsc->dsc_info.dsc_bpp = 8;
	dsc->dsc_info.slice_height = 12;
	dsc->dsc_info.slice_width = 1236;

	/* DSC_CTRL */
	dsc->dsc_info.block_pred_enable = 1;
	dsc->dsc_info.linebuf_depth = 11;
	/* INITIAL_DELAY */
	dsc->dsc_info.initial_dec_delay = 0x036a;
	dsc->dsc_info.initial_xmit_delay = 512;
	/* RC_PARAM0 */
	dsc->dsc_info.initial_scale_value = 0x20;
	dsc->dsc_info.scale_increment_interval = 0x018e;
	/* RC_PARAM1 */
	dsc->dsc_info.scale_decrement_interval = 0x11;
	dsc->dsc_info.first_line_bpg_offset = 12;
	/* RC_PARAM2 */
	dsc->dsc_info.nfl_bpg_offset = 0x8bb;
	dsc->dsc_info.slice_bpg_offset = 0x3b4;
	/* RC_PARAM3 */
	dsc->dsc_info.initial_offset = 6144;
	dsc->dsc_info.final_offset = 0x10f0;
	/* FLATNESS_QP_TH */
	dsc->dsc_info.flatness_min_qp = 7;
	dsc->dsc_info.flatness_max_qp = 16;
	/* DSC_PARAM4 */
	dsc->dsc_info.rc_edge_factor = 0x6;
	dsc->dsc_info.rc_model_size = 8192;
	/* DSC_RC_PARAM5: 0x330f0f */
	dsc->dsc_info.rc_tgt_offset_lo = (0x330f0f >> 20) & 0xF;
	dsc->dsc_info.rc_tgt_offset_hi = (0x330f0f >> 16) & 0xF;
	dsc->dsc_info.rc_quant_incr_limit1 = (0x330f0f >> 8) & 0x1F;
	dsc->dsc_info.rc_quant_incr_limit0 = (0x330f0f >> 0) & 0x1F;
	/* DSC_RC_BUF_THRESH0: 0xe1c2a38 */
	dsc->dsc_info.rc_buf_thresh[0] = (0xe1c2a38 >> 24) & 0xFF;
	dsc->dsc_info.rc_buf_thresh[1] = (0xe1c2a38 >> 16) & 0xFF;
	dsc->dsc_info.rc_buf_thresh[2] = (0xe1c2a38 >> 8) & 0xFF;
	dsc->dsc_info.rc_buf_thresh[3] = (0xe1c2a38 >> 0) & 0xFF;
	/* DSC_RC_BUF_THRESH1: 0x46546269 */
	dsc->dsc_info.rc_buf_thresh[4] = (0x46546269 >> 24) & 0xFF;
	dsc->dsc_info.rc_buf_thresh[5] = (0x46546269 >> 16) & 0xFF;
	dsc->dsc_info.rc_buf_thresh[6] = (0x46546269 >> 8) & 0xFF;
	dsc->dsc_info.rc_buf_thresh[7] = (0x46546269 >> 0) & 0xFF;
	/* DSC_RC_BUF_THRESH2: 0x7077797b */
	dsc->dsc_info.rc_buf_thresh[8] = (0x7077797b >> 24) & 0xFF;
	dsc->dsc_info.rc_buf_thresh[9] = (0x7077797b >> 16) & 0xFF;
	dsc->dsc_info.rc_buf_thresh[10] = (0x7077797b >> 8) & 0xFF;
	dsc->dsc_info.rc_buf_thresh[11] = (0x7077797b >> 0) & 0xFF;
	/* DSC_RC_BUF_THRESH3: 0x7d7e0000 */
	dsc->dsc_info.rc_buf_thresh[12] = (0x7d7e0000 >> 24) & 0xFF;
	dsc->dsc_info.rc_buf_thresh[13] = (0x7d7e0000 >> 16) & 0xFF;
	/* DSC_RC_RANGE_PARAM0: 0x2022200 */
	dsc->dsc_info.rc_range[0].min_qp = (0x2022200 >> 27) & 0x1F;
	dsc->dsc_info.rc_range[0].max_qp = (0x2022200 >> 22) & 0x1F;
	dsc->dsc_info.rc_range[0].offset = (0x2022200 >> 16) & 0x3F;
	dsc->dsc_info.rc_range[1].min_qp = (0x2022200 >> 11) & 0x1F;
	dsc->dsc_info.rc_range[1].max_qp = (0x2022200 >> 6) & 0x1F;
	dsc->dsc_info.rc_range[1].offset = (0x2022200 >> 0) & 0x3F;
	/* DSC_RC_RANGE_PARAM1: 0x2a402abe */
	dsc->dsc_info.rc_range[2].min_qp = (0x2a402abe >> 27) & 0x1F;
	dsc->dsc_info.rc_range[2].max_qp = (0x2a402abe >> 22) & 0x1F;
	dsc->dsc_info.rc_range[2].offset = (0x2a402abe >> 16) & 0x3F;
	dsc->dsc_info.rc_range[3].min_qp = (0x2a402abe >> 11) & 0x1F;
	dsc->dsc_info.rc_range[3].max_qp = (0x2a402abe >> 6) & 0x1F;
	dsc->dsc_info.rc_range[3].offset = (0x2a402abe >> 0) & 0x3F;
	/* DSC_RC_RANGE_PARAM2, 0x3afc3afa */
	dsc->dsc_info.rc_range[4].min_qp = (0x3afc3afa >> 27) & 0x1F;
	dsc->dsc_info.rc_range[4].max_qp = (0x3afc3afa >> 22) & 0x1F;
	dsc->dsc_info.rc_range[4].offset = (0x3afc3afa >> 16) & 0x3F;
	dsc->dsc_info.rc_range[5].min_qp = (0x3afc3afa >> 11) & 0x1F;
	dsc->dsc_info.rc_range[5].max_qp = (0x3afc3afa >> 6) & 0x1F;
	dsc->dsc_info.rc_range[5].offset = (0x3afc3afa >> 0) & 0x3F;
	/* DSC_RC_RANGE_PARAM3, 0x3af83b38 */
	dsc->dsc_info.rc_range[6].min_qp = (0x3af83b38 >> 27) & 0x1F;
	dsc->dsc_info.rc_range[6].max_qp = (0x3af83b38 >> 22) & 0x1F;
	dsc->dsc_info.rc_range[6].offset = (0x3af83b38 >> 16) & 0x3F;
	dsc->dsc_info.rc_range[7].min_qp = (0x3af83b38 >> 11) & 0x1F;
	dsc->dsc_info.rc_range[7].max_qp = (0x3af83b38 >> 6) & 0x1F;
	dsc->dsc_info.rc_range[7].offset = (0x3af83b38 >> 0) & 0x3F;
	/* DSC_RC_RANGE_PARAM4, 0x3b783bb6 */
	dsc->dsc_info.rc_range[8].min_qp = (0x3b783bb6 >> 27) & 0x1F;
	dsc->dsc_info.rc_range[8].max_qp = (0x3b783bb6 >> 22) & 0x1F;
	dsc->dsc_info.rc_range[8].offset = (0x3b783bb6 >> 16) & 0x3F;
	dsc->dsc_info.rc_range[9].min_qp = (0x3b783bb6 >> 11) & 0x1F;
	dsc->dsc_info.rc_range[9].max_qp = (0x3b783bb6 >> 6) & 0x1F;
	dsc->dsc_info.rc_range[9].offset = (0x3b783bb6 >> 0) & 0x3F;
	/* DSC_RC_RANGE_PARAM5, 0x4bf64c34 */
	dsc->dsc_info.rc_range[10].min_qp = (0x4bf64c34 >> 27) & 0x1F;
	dsc->dsc_info.rc_range[10].max_qp = (0x4bf64c34 >> 22) & 0x1F;
	dsc->dsc_info.rc_range[10].offset = (0x4bf64c34 >> 16) & 0x3F;
	dsc->dsc_info.rc_range[11].min_qp = (0x4bf64c34 >> 11) & 0x1F;
	dsc->dsc_info.rc_range[11].max_qp = (0x4bf64c34 >> 6) & 0x1F;
	dsc->dsc_info.rc_range[11].offset = (0x4bf64c34 >> 0) & 0x3F;
	/* DSC_RC_RANGE_PARAM6, 0x4c745c74 */
	dsc->dsc_info.rc_range[12].min_qp = (0x4c745c74 >> 27) & 0x1F;
	dsc->dsc_info.rc_range[12].max_qp = (0x4c745c74 >> 22) & 0x1F;
	dsc->dsc_info.rc_range[12].offset = (0x4c745c74 >> 16) & 0x3F;
	dsc->dsc_info.rc_range[13].min_qp = (0x4c745c74 >> 11) & 0x1F;
	dsc->dsc_info.rc_range[13].max_qp = (0x4c745c74 >> 6) & 0x1F;
	dsc->dsc_info.rc_range[13].offset = (0x4c745c74 >> 0) & 0x3F;
	/* DSC_RC_RANGE_PARAM7, 0x8cf40000 */
	dsc->dsc_info.rc_range[14].min_qp = (0x8cf40000 >> 27) & 0x1F;
	dsc->dsc_info.rc_range[14].max_qp = (0x8cf40000 >> 22) & 0x1F;
	dsc->dsc_info.rc_range[14].offset = (0x8cf40000 >> 16) & 0x3F;
}
#endif

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
		dpu_pr_info("[37701_brq probe] udp mipi param set\n");
		mipi->hsa = 20;
		mipi->hbp = 20;
		mipi->dpi_hsize = 320;
		mipi->hline_time = MIPI_HLINE_TIME;
		mipi->vsa = 40;
		mipi->vbp = 60;
		mipi->vfp = 80;

		mipi->dsi_bit_clk = 550;
		mipi->pxl_clk_rate = 192 * 1000000UL;
	}

	mipi->dsi_bit_clk_upt_support = 0;
	mipi->dsi_bit_clk_upt = mipi->dsi_bit_clk;
	mipi->dsi_bit_clk_default = mipi->dsi_bit_clk;
	mipi->pxl_clk_rate_div = 1;

	mipi->clk_post_adjust = 16;
	mipi->lane_nums = DSI_4_LANES;
	mipi->color_mode = DSI_24BITS_1;

	mipi->vc = 0;
	mipi->max_tx_esc_clk = 10 * 1000000;
	mipi->burst_mode = DSI_BURST_SYNC_PULSES_1;
	mipi->non_continue_en = 1;
	mipi->phy_mode = DPHY_MODE;
	mipi->dsi_version = DSI_1_1_VERSION;
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
	pinfo->ifbc_type = IFBC_TYPE_NONE;

	/* 1. config base object info
	 * would be used for framebuffer setup
	 */
	pinfo->base.xres = 1236;
	pinfo->base.yres = 2676;

	/* When calculating DPI needs the following parameters */
	pinfo->base.width = 72;
	pinfo->base.height = 155;

	/* report to surfaceFlinger, remain: caculate fps by mipi timing para */
#ifdef DFR120
	pinfo->base.fps = 120;
#else
	pinfo->base.fps = 60;
#endif

	/* 2. config connector info
	 * would be used for dsi & composer setup
	 */
#ifndef CLOSE_DSC
	dsc_vesa3_75x_single_config(pinfo, &connector->dsc);
#endif
	mipi_lcd_init_dsi_param(pinfo, &connector->mipi);

	/* dsi or composer need this param */
	pinfo->dirty_region_updt_support = 0;

	/* 3. config panel private info
	 * would be used for panel setup
	 */
	priv->bl_info.bl_min = 24;
	priv->bl_info.bl_max = 2047;
	priv->bl_info.bl_default = 818;

	priv->disp_on_cmds = g_lcd_display_on_cmds;
	priv->disp_on_cmds_len = (uint32_t)ARRAY_SIZE(g_lcd_display_on_cmds);
	priv->disp_off_cmds = g_lcd_display_off_cmds;
	priv->disp_off_cmds_len = (uint32_t)ARRAY_SIZE(g_lcd_display_off_cmds);

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

panel_device_match_data(nt37701_brq_panel_info, PANEL_NT37701_BRQ_ID, panel_of_device_setup, panel_of_device_release);

MODULE_LICENSE("GPL");
