/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
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

#include "dpu_conn_mgr.h"
#include "panel_dev.h"

#define USE_FOR_FPGA 1
#define USE_FOR_UDP 0

/*******************************************************************************
** Display ON/OFF Sequence begin
*/
/* Power ON Sequence(sleep mode to Normal mode) */
static char boe_poweron_param1[] = {
	0xfe,
	0x00,
};

static char boe_poweron_param2[] = {
	0x35,
	0x00,
};

static char boe_poweron_param3[] = {
	0x53,
	0x20,
};

static char boe_poweron_param4[] = {
	0x2a,
	0x00, 0x06, 0x01, 0xd7,
};

static char boe_poweron_param5[] = {
	0x2b,
	0x00, 0x00, 0x01, 0xd1,
};

static char boe_poweron_param6[] = {
	0x11,
	0x00,
};

static char boe_poweron_param7[] = {
	0x29,
	0x00,
};

/* Power OFF Sequence(Normal to power off) */
static char g_off_cmd0[] = {
	0x28,
	0x00,
};

static char g_off_cmd1[] = {
	0x10,
	0x00,
};

/* power on sequence */
static struct dsi_cmd_desc lcd_display_on_cmds[] = {
	{ DTYPE_DCS_WRITE1, 0, 0, WAIT_TYPE_US,
		sizeof(boe_poweron_param1), boe_poweron_param1 },
	{ DTYPE_DCS_WRITE1, 0, 0, WAIT_TYPE_US,
		sizeof(boe_poweron_param2), boe_poweron_param2 },
	{ DTYPE_DCS_WRITE1, 0, 0, WAIT_TYPE_US,
		sizeof(boe_poweron_param3), boe_poweron_param3 },
	{ DTYPE_DCS_LWRITE, 0, 0, WAIT_TYPE_US,
		sizeof(boe_poweron_param4), boe_poweron_param4 },
	{ DTYPE_DCS_LWRITE, 0, 0, WAIT_TYPE_US,
		sizeof(boe_poweron_param5), boe_poweron_param5 },
	{ DTYPE_DCS_WRITE, 0, 5, WAIT_TYPE_MS,
		sizeof(boe_poweron_param6), boe_poweron_param6 },
	{ DTYPE_DCS_WRITE, 0, 0, WAIT_TYPE_US,
		sizeof(boe_poweron_param7), boe_poweron_param7 },
};

/* power off sequence */
static struct dsi_cmd_desc lcd_display_off_cmds[] = {
	{ DTYPE_DCS_WRITE, 0, 10, WAIT_TYPE_US,
		sizeof(g_off_cmd0), g_off_cmd0 },
	{ DTYPE_DCS_WRITE, 0, 100, WAIT_TYPE_MS,
		sizeof(g_off_cmd1), g_off_cmd1 },
};

/*******************************************************************************
** LCD GPIO
*/
#define GPIO_AMOLED_RESET_NAME  "g_gpio_amoled_reset"
#define GPIO_AMOLED_VCC1V8_NAME "g_gpio_amoled_vcc1v8"
#define GPIO_AMOLED_VCC3V1_NAME "g_gpio_amoled_vcc3v1"

static uint32_t g_gpio_amoled_vcc1v8;
static uint32_t g_gpio_amoled_reset;
static uint32_t g_gpio_amoled_vcc3v1;

static struct gpio_desc g_fpga_lcd_gpio_request_cmds[] = {
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_AMOLED_VCC3V1_NAME, &g_gpio_amoled_vcc3v1, 0},
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_AMOLED_VCC1V8_NAME, &g_gpio_amoled_vcc1v8, 0},
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 0},
};

static struct gpio_desc g_fpga_lcd_gpio_free_cmds[] = {
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 0,
		GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 0},
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 0,
		GPIO_AMOLED_VCC1V8_NAME, &g_gpio_amoled_vcc1v8, 0},
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 0,
		GPIO_AMOLED_VCC3V1_NAME, &g_gpio_amoled_vcc3v1, 0},
};

static struct gpio_desc g_fpga_lcd_gpio_lowpower_cmds[] = {
	/* backlight enable */
	/* reset */
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 0},
	/* vcc3v1 disable */
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 10,
		GPIO_AMOLED_VCC3V1_NAME, &g_gpio_amoled_vcc3v1, 0},
	/* vcc1v8 disable */
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 10,
		GPIO_AMOLED_VCC1V8_NAME, &g_gpio_amoled_vcc1v8, 0},

	/* backlight enable input */
	/* reset input */
	{ DTYPE_GPIO_INPUT, WAIT_TYPE_US, 100,
		GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 0},
	/* vcc3v1 disable */
	{ DTYPE_GPIO_INPUT, WAIT_TYPE_US, 100,
		GPIO_AMOLED_VCC3V1_NAME, &g_gpio_amoled_vcc3v1, 0},
	/* vcc1v8 disable */
	{ DTYPE_GPIO_INPUT, WAIT_TYPE_US, 100,
		GPIO_AMOLED_VCC1V8_NAME, &g_gpio_amoled_vcc1v8, 0},
};

static struct gpio_desc g_fpga_lcd_gpio_normal_cmds[] = {
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 10,
		GPIO_AMOLED_VCC3V1_NAME, &g_gpio_amoled_vcc3v1, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20,
		GPIO_AMOLED_VCC1V8_NAME, &g_gpio_amoled_vcc1v8, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 15,
		GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_US, 10,
		GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 50,
		GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 1},
};

static struct gpio_desc g_asic_lcd_gpio_request_cmds[] = {
	{ DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 0},
};

static struct gpio_desc g_asic_lcd_gpio_free_cmds[] = {
	{ DTYPE_GPIO_FREE, WAIT_TYPE_US, 50,
		GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 0},
};

static struct gpio_desc g_asic_lcd_gpio_normal_cmds[] = {
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_US, 50,
		GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 1},
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_US, 50,
		GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 0},
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 10,
		GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 1},
};

static struct gpio_desc g_asic_lcd_gpio_lowpower_cmds[] = {
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_US, 1,
		GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 0},
	/* reset input */
	{ DTYPE_GPIO_INPUT, WAIT_TYPE_US, 1,
		GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 0 },
};

/*******************************************************************************
** LCD VCC
*/
#define VCC_LCDIO_NAME     "lcdio-vcc"
#define VCC_LCDDIC_NAME    "lcddic-vcc"
#define VCC_LCDANALOG_NAME "lcdanalog-vcc"

static struct regulator *vcc_lcdio;
static struct regulator *vcc_lcddic;
static struct regulator *vcc_lcdanalog;

static struct vcc_desc lcd_vcc_init_cmds[] = {
	/* vcc get */
	{DTYPE_VCC_GET, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0, WAIT_TYPE_MS, 0},
	{DTYPE_VCC_GET, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0, WAIT_TYPE_MS, 0},
	{DTYPE_VCC_GET, VCC_LCDDIC_NAME, &vcc_lcddic, 0, 0, WAIT_TYPE_MS, 0},

	/* io set voltage */
	{DTYPE_VCC_SET_VOLTAGE, VCC_LCDIO_NAME, &vcc_lcdio, 1850000, 1850000, WAIT_TYPE_MS, 0},
	/* analog set voltage */
	{DTYPE_VCC_SET_VOLTAGE, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 3100000, 3100000, WAIT_TYPE_MS, 0},
	/* vcc set voltage */
	{DTYPE_VCC_SET_VOLTAGE, VCC_LCDDIC_NAME, &vcc_lcddic, 1220000, 1220000, WAIT_TYPE_MS, 0},
};

static struct vcc_desc lcd_vcc_finit_cmds[] = {
	/* vcc put */
	{DTYPE_VCC_PUT, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0, WAIT_TYPE_MS, 0},
	{DTYPE_VCC_PUT, VCC_LCDDIC_NAME, &vcc_lcddic, 0, 0, WAIT_TYPE_MS, 0},
	{DTYPE_VCC_PUT, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0, WAIT_TYPE_MS, 0},
};

static struct vcc_desc g_lcd_vcc_enable_cmds[] = {
	/* vcc enable */
	{DTYPE_VCC_ENABLE, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0, WAIT_TYPE_MS, 3},
	{DTYPE_VCC_ENABLE, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0, WAIT_TYPE_MS, 15},
	{DTYPE_VCC_ENABLE, VCC_LCDDIC_NAME, &vcc_lcddic, 0, 0, WAIT_TYPE_MS, 3},
};

static struct vcc_desc g_lcd_vcc_disable_cmds[] = {
	/* vcc disable */
	{DTYPE_VCC_DISABLE, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0, WAIT_TYPE_MS, 3},
	{DTYPE_VCC_DISABLE, VCC_LCDDIC_NAME, &vcc_lcddic, 0, 0, WAIT_TYPE_MS, 3},
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

static struct pinctrl_cmd_desc lcd_pinctrl_finit_cmds[] = {
	{DTYPE_PINCTRL_PUT, &pctrl, 0},
};

static struct pinctrl_cmd_desc g_lcd_pinctrl_normal_cmds[] = {
	{ DTYPE_PINCTRL_SET, &pctrl, DTYPE_PINCTRL_STATE_DEFAULT },
};

static struct pinctrl_cmd_desc g_lcd_pinctrl_lowpower_cmds[] = {
	{DTYPE_PINCTRL_SET, &pctrl, DTYPE_PINCTRL_STATE_IDLE},
};

static void check_dts_for_gpio(struct dkmd_connector_info *pinfo, struct device_node *np)
{
	if (pinfo->base.fpga_flag == USE_FOR_UDP) {
		g_gpio_amoled_reset = (uint32_t)of_get_named_gpio(np, "gpios", 0);
		g_gpio_amoled_vcc3v1 = (uint32_t)of_get_named_gpio(np, "gpios", 1);
		g_gpio_amoled_vcc1v8 = (uint32_t)of_get_named_gpio(np, "gpios", 2);
	} else {
		g_gpio_amoled_reset = (uint32_t)of_get_named_gpio(np, "gpios", 0);
		g_gpio_amoled_vcc3v1 = (uint32_t)of_get_named_gpio(np, "gpios", 1);
		g_gpio_amoled_vcc1v8 = (uint32_t)of_get_named_gpio(np, "gpios", 2);
	}

	dpu_pr_info("lcd gpio:reset:%d, 3v1:%d, 1v8:%d\n",
		g_gpio_amoled_reset, g_gpio_amoled_vcc3v1, g_gpio_amoled_vcc1v8);
}

static void panel_cmd_data_setup(struct panel_drv_private *priv, struct device_node *np)
{
	struct dkmd_connector_info *pinfo = &priv->connector_info;

	check_dts_for_gpio(pinfo, np);

	if (pinfo->base.fpga_flag == USE_FOR_FPGA) {
		priv->gpio_request_cmds = g_fpga_lcd_gpio_request_cmds;
		priv->gpio_request_cmds_len = ARRAY_SIZE(g_fpga_lcd_gpio_request_cmds);
		priv->gpio_free_cmds = g_fpga_lcd_gpio_free_cmds;
		priv->gpio_free_cmds_len = ARRAY_SIZE(g_fpga_lcd_gpio_free_cmds);

		priv->gpio_lowpower_cmds = g_fpga_lcd_gpio_lowpower_cmds;
		priv->gpio_lowpower_cmds_len = ARRAY_SIZE(g_fpga_lcd_gpio_lowpower_cmds);
		priv->gpio_normal_cmds = g_fpga_lcd_gpio_normal_cmds;
		priv->gpio_normal_cmds_len = ARRAY_SIZE(g_fpga_lcd_gpio_normal_cmds);
	} else {
		priv->gpio_request_cmds = g_asic_lcd_gpio_request_cmds;
		priv->gpio_request_cmds_len = ARRAY_SIZE(g_asic_lcd_gpio_request_cmds);
		priv->gpio_free_cmds = g_asic_lcd_gpio_free_cmds;
		priv->gpio_free_cmds_len = ARRAY_SIZE(g_asic_lcd_gpio_free_cmds);

		priv->vcc_enable_cmds = g_lcd_vcc_enable_cmds;
		priv->vcc_enable_cmds_len = ARRAY_SIZE(g_lcd_vcc_enable_cmds);
		priv->vcc_disable_cmds = g_lcd_vcc_disable_cmds;
		priv->vcc_disable_cmds_len = ARRAY_SIZE(g_lcd_vcc_disable_cmds);

		priv->pinctrl_normal_cmds = g_lcd_pinctrl_normal_cmds;
		priv->pinctrl_normal_cmds_len = ARRAY_SIZE(g_lcd_pinctrl_normal_cmds);
		priv->pinctrl_lowpower_cmds = g_lcd_pinctrl_lowpower_cmds;
		priv->pinctrl_lowpower_cmds_len = ARRAY_SIZE(g_lcd_pinctrl_lowpower_cmds);

		priv->gpio_normal_cmds = g_asic_lcd_gpio_normal_cmds;
		priv->gpio_normal_cmds_len = ARRAY_SIZE(g_asic_lcd_gpio_normal_cmds);
		priv->gpio_lowpower_cmds = g_asic_lcd_gpio_lowpower_cmds;
		priv->gpio_lowpower_cmds_len = ARRAY_SIZE(g_asic_lcd_gpio_lowpower_cmds);

		priv->pinctrl_init_cmds = lcd_pinctrl_init_cmds;
		priv->pinctrl_init_cmds_len = ARRAY_SIZE(lcd_pinctrl_init_cmds);
		priv->pinctrl_finit_cmds = lcd_pinctrl_finit_cmds;
		priv->pinctrl_finit_cmds_len = ARRAY_SIZE(lcd_pinctrl_finit_cmds);

		priv->vcc_init_cmds = lcd_vcc_init_cmds;
		priv->vcc_init_cmds_len = ARRAY_SIZE(lcd_vcc_init_cmds);
		priv->vcc_finit_cmds = lcd_vcc_finit_cmds;
		priv->vcc_finit_cmds_len = ARRAY_SIZE(lcd_vcc_finit_cmds);
	}
}

/* dsi param initialized value from panel spec */
static void mipi_lcd_init_dsi_param(struct dkmd_connector_info *pinfo, struct mipi_panel_info *mipi)
{
	if (pinfo->base.fpga_flag == USE_FOR_FPGA) {
		/* ldi */
		dpu_pr_info("fpga!\n");
		mipi->hsa = 11;
		mipi->hbp = 37;
		mipi->dpi_hsize = 812;
		mipi->hline_time = 936;
		mipi->vsa = 10;
		mipi->vbp = 54;
		mipi->vfp = 36;

		/* mipi */
		mipi->dsi_bit_clk = 120;
		mipi->pxl_clk_rate = 20 * 1000000UL;
	} else {
		dpu_pr_info("udp!\n");
		/* ldi */
		mipi->hsa = 23;
		mipi->hbp = 50;
		mipi->dpi_hsize = 20;
		mipi->hline_time = 12;
		mipi->vsa = 14;
		mipi->vbp = 4;
		mipi->vfp = 36;

		mipi->pxl_clk_rate = 20 * 1000000UL;
		/* mipi */
		mipi->dsi_bit_clk = 240;
	}

	mipi->dsi_bit_clk_upt_support = 0;
	mipi->clk_post_adjust = 215;
	mipi->dsi_bit_clk_upt = mipi->dsi_bit_clk;
	mipi->dsi_bit_clk_default = mipi->dsi_bit_clk;

	mipi->lane_nums = DSI_1_LANES;
	mipi->color_mode = DSI_24BITS_1;
	mipi->vc = 0;
	mipi->max_tx_esc_clk = 6 * 1000000;
	mipi->burst_mode = DSI_BURST_SYNC_PULSES_1;
	mipi->non_continue_en = 0;
	mipi->pxl_clk_rate_div = 1;
}

static int32_t panel_of_device_setup(struct panel_drv_private *priv)
{
	int32_t ret;
	struct dkmd_connector_info *pinfo = &priv->connector_info;
	struct device_node *np = priv->pdev->dev.of_node;

	dpu_pr_info("enter!\n");

	/* Inheritance based processing */
	panel_base_of_device_setup(priv);

	panel_cmd_data_setup(priv, np);

	/* 1. config base object info
	 * would be used for framebuffer setup
	 */
	pinfo->base.xres = 466; // FIXME: Modified for new panel device
	pinfo->base.yres = 466; // FIXME: Modified for new panel device

	/* When calculating DPI needs the following parameters */
	pinfo->base.width = 36; // FIXME: Modified for new panel device
	pinfo->base.height = 36; // FIXME: Modified for new panel device

	pinfo->base.fps = 15;
	pinfo->frc_enable = 0;
	pinfo->esd_enable = 0;
	pinfo->lcd_uninit_step_support = 1;
	pinfo->color_temperature_support = 0;

	/* 2. config connector info
	 * would be used for dsi & composer setup
	 */
	mipi_lcd_init_dsi_param(pinfo, &get_primary_connector(pinfo)->mipi);

	/* dsi or composer need this param */
	pinfo->dirty_region_updt_support = 0;

	/* 3. config panel private info
	 * would be used for panel setup
	 */
	priv->bl_info.bl_min = 1;
	priv->bl_info.bl_max = 255;
	priv->bl_info.bl_default = 200;

	priv->disp_on_cmds = lcd_display_on_cmds;
	priv->disp_on_cmds_len = (uint32_t)ARRAY_SIZE(lcd_display_on_cmds);
	priv->disp_off_cmds = lcd_display_off_cmds;
	priv->disp_off_cmds_len = ARRAY_SIZE(lcd_display_off_cmds);

	/* Don't need to operate vcc and pinctrl for fpga */
	if (pinfo->base.fpga_flag == USE_FOR_FPGA) {
		priv->vcc_enable_cmds_len = 0;
		priv->vcc_disable_cmds_len = 0;
		priv->pinctrl_normal_cmds_len = 0;
		priv->pinctrl_lowpower_cmds_len = 0;
		priv->pinctrl_init_cmds_len = 0;
		priv->pinctrl_finit_cmds_len = 0;
		priv->vcc_init_cmds_len = 0;
		priv->vcc_finit_cmds_len = 0;
	}

	if (pinfo->base.fpga_flag == USE_FOR_UDP) {
		ret = peri_vcc_cmds_tx(priv->pdev, priv->vcc_init_cmds, priv->vcc_init_cmds_len);
		if (ret != 0)
			dpu_pr_info("vcc init failed!\n");

		ret = peri_pinctrl_cmds_tx(priv->pdev, priv->pinctrl_init_cmds, priv->pinctrl_init_cmds_len);
		if (ret != 0)
			dpu_pr_info("pinctrl init failed\n");
	}

	if (priv->gpio_request_cmds && (priv->gpio_request_cmds_len > 0)) {
		ret = peri_gpio_cmds_tx(priv->gpio_request_cmds, priv->gpio_request_cmds_len);
		if (ret)
			dpu_pr_info("gpio cmds request err: %d!\n", ret);
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

panel_device_match_data(rm69091_panel_info, PANEL_RM69091_ID, panel_of_device_setup, panel_of_device_release);

MODULE_LICENSE("GPL");
