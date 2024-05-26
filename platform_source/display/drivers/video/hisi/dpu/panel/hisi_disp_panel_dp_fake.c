/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/module.h>

#include "hisi_disp_debug.h"
#include "hisi_disp_panel.h"
#include "hisi_connector_utils.h"
#include "hisi_disp_config.h"
#include "hisi_dp_core.h"
#include "hisi_dp_utils.h"

#define DTS_COMP_DP0_FAKE_PANEL      "hisilicon,disp_dp0_fake_panel"
#define DTS_COMP_DP1_FAKE_PANEL      "hisilicon,disp_dp1_fake_panel"

static int hisi_dp_fake_read_dtsi(struct device_node *np, uint8_t *dp_id, uint8_t *dp_mode)
{
	int ret;

	ret = of_property_read_u8(np, "channel_id", dp_id);
	if (ret) {
		disp_pr_err("[DP] read id fail");
		return -1;
	}

	ret = of_property_read_u8(np, "mode", dp_mode);
	if (ret) {
		disp_pr_err("[DP] read mode fail");
		return -1;
	}

	return 0;
}

static void hisi_dp_fake_init_panel(struct hisi_panel_info *info)
{
	memset(info, 0, sizeof(struct hisi_panel_info));
	info->width = 16000;
	info->height = 9000;

	info->type = PANEL_DP;
	/*info->orientation = LCD_LANDSCAPE;
	info->bpp = LCD_RGB888;
	info->bgr_fmt = LCD_RGB;*/
	info->bl_info.bl_type = BL_SET_BY_NONE;

	info->bl_info.bl_min = 1;
	info->bl_info.bl_max = 255;
	info->bl_info.bl_default = 102;
	info->ifbc_type = IFBC_TYPE_NONE;

	info->vsync_ctrl_type = 0;

	info->ldi.hsync_plr = 1;
	info->ldi.vsync_plr = 1;
	info->ldi.pixelclk_plr = 0;
	info->ldi.data_en_plr = 0;

	info->xres = 1920;
	info->yres = 1080;
	info->ldi.h_back_porch = 148;
	info->ldi.h_front_porch = 88;
	info->ldi.h_pulse_width = 44;
	info->ldi.v_back_porch = 36;
	info->ldi.v_front_porch = 4;
	info->ldi.v_pulse_width = 5;
	info->timing_info.pxl_clk_rate = 148500000UL;
	info->timing_info.pxl_clk_rate_div = 1;
}

static void hisi_dp_fake_init_connector(struct hisi_connector_info **out_info, uint8_t dp_id, uint8_t dp_mode)
{
	struct hisi_dp_info *dp_info = NULL;

	dp_info = kzalloc(sizeof(*dp_info), GFP_KERNEL);
	if (!dp_info)
		return;

	/* other dp infomation */
	dp_info->base.power_step_cnt = 1;
	dp_info->base.connector_type = HISI_CONNECTOR_TYPE_DP;
	dp_info->dp.mode = dp_mode;
	dp_info->dp.id = dp_id;

	*out_info = (struct hisi_connector_info *)dp_info;
}

static struct hisi_panel_ops g_hisi_fake_dp_panel_ops = {
	.base = {
		.turn_on = NULL,
		.turn_off = NULL,
		.present = NULL,
	},
	.bl_ops = {
		.set_backlight = NULL,
	}
};

static int hisi_fake_dp_panel_probe(struct platform_device *pdev)
{
	struct hisi_panel_device *panel_data = NULL;
	uint8_t dp_mode;
	uint8_t dp_id;
	int ret;

	disp_pr_info("[DP] enter ++++++\n");

	panel_data = devm_kzalloc(&pdev->dev, sizeof(*panel_data), GFP_KERNEL);
	if (!panel_data)
		return -1;

	panel_data->pdev = pdev;

	/* read dtsi */
	ret = hisi_dp_fake_read_dtsi(pdev->dev.of_node, &dp_id, &dp_mode);
	if (ret) {
		disp_pr_err("[DP] parse dts fail\n");
		return -1;
	}
	disp_pr_info("[DP] id: %u, mode: %u\n", dp_id, dp_mode);

	/* init panel and dp information */
	hisi_dp_fake_init_panel(&panel_data->panel_info);
	hisi_dp_fake_init_connector(&panel_data->connector_info, dp_id, dp_mode);
	platform_set_drvdata(pdev, panel_data);

	hisi_dp_create_platform_device(panel_data, &g_hisi_fake_dp_panel_ops);

	disp_pr_info("[DP] exit ------\n");
	return 0;
}

static void hisi_fake_dp_panel_shutdown(struct platform_device *pdev)
{

}

static const struct of_device_id hisi_fake_dp_panel_match_table[] = {
	{
		.compatible = DTS_COMP_DP0_FAKE_PANEL,
		.data = NULL,
	},
	{
		.compatible = DTS_COMP_DP1_FAKE_PANEL,
		.data = NULL,
	},
	{},
};
MODULE_DEVICE_TABLE(of, hisi_fake_dp_panel_match_table);

static struct platform_driver g_hisi_fake_dp_panel_driver = {
	.probe = hisi_fake_dp_panel_probe,
	.remove = NULL,
	.suspend = NULL,
	.resume = NULL,
	.shutdown = hisi_fake_dp_panel_shutdown,
	.driver = {
		.name = HISI_FAKE_DP_PANEL_DEV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(hisi_fake_dp_panel_match_table),
	},
};

static int __init hisi_fake_dp_panel_init(void)
{
	int ret = 0;

	disp_pr_info("[DP] enter ++++++");

	ret = platform_driver_register(&g_hisi_fake_dp_panel_driver);
	if (ret) {
		disp_pr_err("[DP] platform_driver_register failed, error=%d!\n", ret);
		return ret;
	}

	disp_pr_info("[DP] exit ------");
	return ret;
}

module_init(hisi_fake_dp_panel_init);

MODULE_DESCRIPTION("Hisilicon Dp Fake Panel Driver");
MODULE_LICENSE("GPL v2");

