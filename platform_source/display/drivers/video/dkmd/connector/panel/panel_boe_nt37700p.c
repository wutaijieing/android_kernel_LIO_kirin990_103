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


static struct dsi_cmd_desc lcd_display_on_cmds[] = {
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

static struct dsi_cmd_desc lcd_display_off_cmds[] = {
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

static struct gpio_desc g_fpga_lcd_gpio_lowpower_cmds[] = {
	/* vcc1v8 disable */
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 2,
		GPIO_AMOLED_VCC1V8_NAME, &g_gpio_amoled_vcc1v8, 0 },
	/* vcc3v1 disable */
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 15,
		GPIO_AMOLED_VCC3V1_NAME, &g_gpio_amoled_vcc3v1, 0 },
	/* reset */
	{ DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 0 },
	/* backlight disable */
};

static struct gpio_desc g_fpga_lcd_gpio_free_cmds[] = {
	/* vcc3v1 */
	{ DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0,
		GPIO_AMOLED_VCC3V1_NAME, &g_gpio_amoled_vcc3v1, 0 },
	/* vcc1v8 */
	{ DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0,
		GPIO_AMOLED_VCC1V8_NAME, &g_gpio_amoled_vcc1v8, 0 },
	/* reset */
	{ DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0,
		GPIO_AMOLED_RESET_NAME, &g_gpio_amoled_reset, 0 },
};

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
 * LCD VCC
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

static struct pinctrl_cmd_desc g_lcd_pinctrl_lowpower_cmds[] = {
	{DTYPE_PINCTRL_SET, &g_pctrl, DTYPE_PINCTRL_STATE_IDLE},
};

static struct pinctrl_cmd_desc g_lcd_pinctrl_finit_cmds[] = {
	{ DTYPE_PINCTRL_PUT, &g_pctrl, 0 },
};

/*******************************************************************************
 */
static void panel_drv_private_data_setup(struct panel_drv_private *priv, struct device_node *np)
{
	if (priv->connector_info.base.fpga_flag == 1) {
		g_gpio_amoled_reset = (uint32_t)of_get_named_gpio(np, "gpios", 0);
		g_gpio_amoled_vcc3v1 = (uint32_t)of_get_named_gpio(np, "gpios", 1);
		g_gpio_amoled_vcc1v8 = (uint32_t)of_get_named_gpio(np, "gpios", 2);

		dpu_pr_info("used gpio:[rst: %d, vcc3v1: %d, tp1v8: %d]\n",
			g_gpio_amoled_reset, g_gpio_amoled_vcc3v1, g_gpio_amoled_vcc1v8);

		priv->gpio_request_cmds = g_fpga_lcd_gpio_request_cmds;
		priv->gpio_request_cmds_len = (uint32_t)ARRAY_SIZE(g_fpga_lcd_gpio_request_cmds);
		priv->gpio_free_cmds = g_fpga_lcd_gpio_free_cmds;
		priv->gpio_free_cmds_len = (uint32_t)ARRAY_SIZE(g_fpga_lcd_gpio_free_cmds);

		priv->gpio_normal_cmds = g_fpga_lcd_gpio_normal_cmds;
		priv->gpio_normal_cmds_len = (uint32_t)ARRAY_SIZE(g_fpga_lcd_gpio_normal_cmds);
		priv->gpio_lowpower_cmds = g_fpga_lcd_gpio_lowpower_cmds;
		priv->gpio_lowpower_cmds_len = (uint32_t)ARRAY_SIZE(g_fpga_lcd_gpio_lowpower_cmds);
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

/* dsi param initialized value from panel spec */
static void mipi_lcd_init_dsi_param(struct dkmd_connector_info *pinfo, struct mipi_panel_info *mipi)
{
	if (pinfo->base.fpga_flag == 1) {
		mipi->hsa = 7;
		mipi->hbp = 12;
		mipi->dpi_hsize = 812;
		mipi->hline_time = 840;
		mipi->vsa = 10;
		mipi->vbp = 58;
		mipi->vfp = 80;

		mipi->pxl_clk_rate = 20 * 1000000UL;
		mipi->dsi_bit_clk = 120;
	} else {
		mipi->hsa = 7;
		mipi->hbp = 12;
		mipi->dpi_hsize = 812;
		mipi->hline_time = 840;
		mipi->vsa = 10;
		mipi->vbp = 58;
		mipi->vfp = 80;

		mipi->pxl_clk_rate = 192 * 1000000UL;
		mipi->dsi_bit_clk = 550;
	}

	mipi->dsi_bit_clk_upt = mipi->dsi_bit_clk;
	mipi->dsi_bit_clk_default = mipi->dsi_bit_clk;

	mipi->pxl_clk_rate_div = 1;
	mipi->dsi_bit_clk_upt_support = 0;

	mipi->clk_post_adjust = 215;
	mipi->lane_nums = DSI_4_LANES;
	mipi->color_mode = DSI_24BITS_1;

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
	priv->dirty_region_info.top_align = 60;
	priv->dirty_region_info.bottom_align = 60;
	priv->dirty_region_info.w_align = -1;
	priv->dirty_region_info.h_align = 120;
	priv->dirty_region_info.w_min = 1080;
	priv->dirty_region_info.h_min = 120;
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
	pinfo->base.xres = 1080; // FIXME: Modified for new panel device
	pinfo->base.yres = 2340; // FIXME: Modified for new panel device

	/* When calculating DPI needs the following parameters */
	pinfo->base.width = 67; // FIXME: Modified for new panel device
	pinfo->base.height = 139; // FIXME: Modified for new panel device

	// TODO: caculate fps by mipi timing para
	pinfo->base.fps = 60;

	/* 2. config connector info
	 * would be used for dsi & composer setup
	 */
	mipi_lcd_init_dsi_param(pinfo, &get_primary_connector(pinfo)->mipi);

	/* dsi or composer need this param */
	pinfo->dirty_region_updt_support = 0;
	pinfo->vsync_ctrl_type = VSYNC_IDLE_MIPI_ULPS | VSYNC_IDLE_CLK_OFF;

	/* 3. config panel private info
	 * would be used for panel setup
	 */
	priv->bl_info.bl_min = 12;
	priv->bl_info.bl_max = 2047;
	priv->bl_info.bl_default = 600;

	lcd_init_dirty_region(priv);

	priv->disp_on_cmds = lcd_display_on_cmds;
	priv->disp_on_cmds_len = ARRAY_SIZE(lcd_display_on_cmds);
	priv->disp_off_cmds = lcd_display_off_cmds;
	priv->disp_off_cmds_len = ARRAY_SIZE(lcd_display_off_cmds);

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

panel_device_match_data(nt37700p_panel_info, PANEL_NT37700P_ID, panel_of_device_setup, panel_of_device_release);

MODULE_LICENSE("GPL");
