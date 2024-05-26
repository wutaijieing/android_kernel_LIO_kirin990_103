/**
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

/*******************************************************************************
** Display ON/OFF Sequence begin
*/
static char pwm_out_0x51[] = {
	0x51,
	0xFE,
};

static char partial_setting_1[] = {
	0xFF,
	0x10,
};

static char partial_setting_2[] = {
	0xFB,
	0x01,
};

static char partial_setting_3[] = {
	0xC9,
	0x4B, 0x04,0x21,0x00,0x0F,0x03,0x19,0x01,0x97,0x10,0xF0,
};

static char lcd_disp_x[] = {
	0x2A,
	0x00, 0x00,0x04,0x37
};

static char lcd_disp_y[] = {
	0x2B,
	0x00, 0x00,0x07,0x7F
};

static char te_enable[] = {
	0x35,
	0x00,
};

static char bl_enable[] = {
	0x53,
	0x24,
};

static char color_enhancement[] = {
	0x30,
	0x00, 0x00,0x02,0xA7,
};

static char exit_sleep[] = {
	0x11,
};

static char display_on[] = {
	0x29,
};

static char lcd_power_on_cmd1[] = {
	0xD6,
	0x01,
};

static char display_phy1[] = {
	0xB0,
	0x04,
};

static char display_phy2[] = {
	0xB6,
	0x38, 0x93,0x00,
};

static struct dsi_cmd_desc lcd_display_on_cmds[] = {
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(pwm_out_0x51), pwm_out_0x51},

	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(partial_setting_1), partial_setting_1},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(partial_setting_2), partial_setting_2},
	{DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(partial_setting_3), partial_setting_3},

	{DTYPE_DCS_LWRITE, 0, 5, WAIT_TYPE_US,
		sizeof(lcd_disp_x), lcd_disp_x},
	{DTYPE_DCS_LWRITE, 0, 5, WAIT_TYPE_US,
		sizeof(lcd_disp_y), lcd_disp_y},

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

	// for fpga phyclk config
	{DTYPE_GEN_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(lcd_power_on_cmd1), lcd_power_on_cmd1},
	{DTYPE_GEN_LWRITE, 0, 120, WAIT_TYPE_MS,
		sizeof(display_phy1), display_phy1},
	{DTYPE_GEN_LWRITE, 0, 120, WAIT_TYPE_MS,
		sizeof(display_phy2), display_phy2},
};

static char display_off[] = {
	0x28,
};

static char enter_sleep[] = {
	0x10,
};

static struct dsi_cmd_desc lcd_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 0, 52, WAIT_TYPE_MS,sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 0, 102, WAIT_TYPE_MS,sizeof(enter_sleep), enter_sleep},
};

/* dsi param initialized value from panel spec */
static void mipi_lcd_init_dsi_param(struct mipi_panel_info *mipi)
{
	mipi->hsa = 11;
	mipi->hbp = 37;
	mipi->dpi_hsize = 812;
	mipi->hline_time = 936;
	mipi->vsa = 10;
	mipi->vbp = 54;
	mipi->vfp = 36;

	mipi->dsi_bit_clk = 120;
	mipi->dsi_bit_clk_upt = mipi->dsi_bit_clk;
	mipi->dsi_bit_clk_default = mipi->dsi_bit_clk;

	mipi->pxl_clk_rate = 20 * 1000000UL;
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
	priv->dirty_region_info.top_align = 32;
	priv->dirty_region_info.bottom_align = 32;
	priv->dirty_region_info.w_align = -1;
	priv->dirty_region_info.h_align = -1;
	priv->dirty_region_info.w_min = 1080;
	priv->dirty_region_info.h_min = -1;
	priv->dirty_region_info.top_start = -1;
	priv->dirty_region_info.bottom_start = -1;
}

static int32_t panel_of_device_setup(struct panel_drv_private *priv)
{
	int32_t ret;
	struct dkmd_connector_info *pinfo = &priv->connector_info;

	dpu_pr_info("enter!\n");

	/* Inheritance based processing */
	panel_base_of_device_setup(priv);

	/* 1. config base object info
	 * would be used for framebuffer setup
	 */
	pinfo->base.xres = 1080; // FIXME: Modified for new panel device
	pinfo->base.yres = 1920; // FIXME: Modified for new panel device

	/* When calculating DPI needs the following parameters */
	pinfo->base.width = 75; // FIXME: Modified for new panel device
	pinfo->base.height = 133; // FIXME: Modified for new panel device

	// TODO: caculate fps by mipi timing para
	pinfo->base.fps = 60;

	/* 2. config connector info
	 * would be used for dsi & composer setup
	 */
	mipi_lcd_init_dsi_param(&get_primary_connector(pinfo)->mipi);

	/* dsi or composer need this param */
	pinfo->dirty_region_updt_support = 0;

	/* 3. config panel private info
	 * would be used for panel setup
	 */
	priv->bl_info.bl_min = 1;
	priv->bl_info.bl_max = 255;
	priv->bl_info.bl_default = 102;

	lcd_init_dirty_region(priv);

	priv->disp_on_cmds = lcd_display_on_cmds;
	priv->disp_on_cmds_len = (uint32_t)ARRAY_SIZE(lcd_display_on_cmds);
	priv->disp_off_cmds = lcd_display_off_cmds;
	priv->disp_off_cmds_len = (uint32_t)ARRAY_SIZE(lcd_display_off_cmds);

	/* Don't need to operate vcc and pinctrl for fpga */
	if (pinfo->base.fpga_flag == 1) {
		priv->vcc_enable_cmds_len = 0;
		priv->vcc_disable_cmds_len = 0;
		priv->pinctrl_normal_cmds_len = 0;
		priv->pinctrl_lowpower_cmds_len = 0;
		priv->pinctrl_init_cmds_len = 0;
		priv->pinctrl_finit_cmds_len = 0;
		priv->vcc_init_cmds_len = 0;
		priv->vcc_finit_cmds_len = 0;
	}

	if (pinfo->base.fpga_flag == 0) {
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

panel_device_match_data(td4322_panel_info, PANEL_TD4322_ID, panel_of_device_setup, panel_of_device_release);

MODULE_LICENSE("GPL");