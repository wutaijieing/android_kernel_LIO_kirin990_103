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
#include "hisi_mipi_core.h"
#include "hisi_mipi_utils.h"

#define DTS_COMP_MIPI_FAKE_PANEL      "hisilicon,disp_fake_mipi_panel"

static void hisi_mipi_fake_read_dtsi()
{

}

static void hisi_mipi_fake_init_panel(struct hisi_panel_info *info)
{
	memset(info, 0, sizeof(struct hisi_panel_info));
	info->xres = 1080;
	info->yres = 1920;
	info->width = 62;
	info->height = 110;
	info->type = PANEL_MIPI_VIDEO;

	/* info->orientation = LCD_PORTRAIT;
	info->bpp = LCD_RGB888;
	info->bgr_fmt = LCD_RGB; */

	info->bl_info.bl_type = BL_SET_BY_BLPWM;
	info->bl_info.bl_min = 63;
	info->bl_info.bl_max = 7992;
	info->bl_info.bl_default = 4000;
	info->bl_info.blpwm_precision_type = BLPWM_PRECISION_2048_TYPE;
	info->bl_info.bl_ic_ctrl_mode = REG_ONLY_MODE;

	info->ldi.h_back_porch = 40;
	info->ldi.h_front_porch = 90;
	info->ldi.h_pulse_width = 20;
	info->ldi.v_back_porch = 28;
	info->ldi.v_front_porch = 14;
	info->ldi.v_pulse_width = 8;
}

static void hisi_mipi_fake_init_connector(struct hisi_connector_info **out_info)
{
	struct hisi_mipi_info *mipi_info = NULL;

	mipi_info = kzalloc(sizeof(*mipi_info), GFP_KERNEL);
	if (!mipi_info)
		return;

	/* other mipi infomation */
	mipi_info->base.power_step_cnt = 3;

	*out_info = (struct hisi_connector_info *)mipi_info;
}

static int hisi_fake_panel_on(uint32_t panel_id, struct platform_device *pdev)
{
	disp_pr_info(" enter ++++");

	return 0;
}

static int hisi_fake_panel_off(uint32_t panel_id, struct platform_device *pdev)
{
	disp_pr_info(" enter ++++");
	return 0;
}

static struct hisi_panel_ops g_hisi_fake_mipi_panel_ops = {
	.base = {
		.turn_on = hisi_fake_panel_on,
		.turn_off = hisi_fake_panel_off,
		.present = NULL,
	},
	.bl_ops = {
		.set_backlight = NULL,
	}
};

static int hisi_fake_mipi_panel_probe(struct platform_device *pdev)
{
	struct hisi_panel_device *panel_data = NULL;

	disp_pr_info("enter ++++++");

	panel_data = devm_kzalloc(&pdev->dev, sizeof(*panel_data), GFP_KERNEL);
	if (!panel_data)
		return -1;

	panel_data->pdev = pdev;

	/* read dtsi */
	hisi_mipi_fake_read_dtsi();

	/* init panel and mipi information */
	hisi_mipi_fake_init_panel(&panel_data->panel_info);
	hisi_mipi_fake_init_connector(&panel_data->connector_info);
	platform_set_drvdata(pdev, panel_data);

	hisi_mipi_create_platform_device(panel_data, &g_hisi_fake_mipi_panel_ops);

	disp_pr_info("exit ------");
	return 0;
}

static void hisi_fake_mipi_panel_shutdown(struct platform_device *pdev)
{

}

static const struct of_device_id hisi_fake_mipi_panel_match_table[] = {
	{
		.compatible = DTS_COMP_MIPI_FAKE_PANEL,
		.data = NULL,
	},
	{},
};
MODULE_DEVICE_TABLE(of, hisi_fake_mipi_panel_match_table);

static struct platform_driver g_hisi_fake_mipi_panel_driver = {
	.probe = hisi_fake_mipi_panel_probe,
	.remove = NULL,
	.suspend = NULL,
	.resume = NULL,
	.shutdown = hisi_fake_mipi_panel_shutdown,
	.driver = {
		.name = HISI_FAKE_PANEL_DEV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(hisi_fake_mipi_panel_match_table),
	},
};

static int __init hisi_fake_mipi_panel_init(void)
{
	int ret = 0;

	disp_pr_info("enter ++++++");

	ret = platform_driver_register(&g_hisi_fake_mipi_panel_driver);
	if (ret) {
		disp_pr_err("platform_driver_register failed, error=%d!\n", ret);
		return ret;
	}

	disp_pr_info("exit ------");
	return ret;
}

module_init(hisi_fake_mipi_panel_init);

MODULE_DESCRIPTION("Hisilicon Fake Panel Driver");
MODULE_LICENSE("GPL v2");

