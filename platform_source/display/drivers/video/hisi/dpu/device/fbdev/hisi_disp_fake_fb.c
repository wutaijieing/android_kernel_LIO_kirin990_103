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

#include "hisi_fb.h"
#include "hisi_disp_composer.h"
#include "hisi_disp_debug.h"


#define DTS_COMP_FAKE_FB "hisilicon,disp_fake_fb"


static void hisi_fake_fb_init_panel(struct hisi_panel_info *info)
{
	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info)
		return;

	info->xres = 1080;
	info->yres = 1920;
	info->width = 62;
	info->height = 110;
	info->type = PANEL_MIPI_VIDEO;

	info->orientation = LCD_PORTRAIT;
	info->bpp = LCD_RGB888;
	info->bgr_fmt = LCD_RGB;

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

static int hisi_fake_fb_on(uint32_t panel_id, struct platform_device *pdev)
{
	disp_pr_info(" enter ++++");

	return 0;
}

static int hisi_fake_fb_off(uint32_t panel_id, struct platform_device *pdev)
{
	disp_pr_info(" enter ++++");
	return 0;
}

static struct hisi_pipeline_ops g_hisi_fake_fb_ops = {
	.turn_on = hisi_fake_fb_on,
	.turn_off = hisi_fake_fb_off,
	.present = NULL,
};

static int hisi_disp_fake_fb_probe(struct platform_device *pdev)
{
	struct hisi_composer_device *fake_comp_dev = NULL;

	disp_pr_info("enter ++++++");

	fake_comp_dev = devm_kzalloc(&pdev->dev, sizeof(*fake_comp_dev), GFP_KERNEL);
	if (!fake_comp_dev)
		return -1;

	fake_comp_dev->pdev = pdev;
	fake_comp_dev->ov_data.ov_id = ONLINE_OVERLAY_ID_MAX; /* for fake fb, will not register private fb ops */

	/* init panel and mipi information */
	hisi_fake_fb_init_panel(fake_comp_dev->ov_data.fix_panel_info);
	platform_set_drvdata(pdev, fake_comp_dev);

	hisi_fb_create_platform_device("fake_fb", fake_comp_dev, &g_hisi_fake_fb_ops);

	disp_pr_info("exit ------");
	return 0;
}

static void hisi_disp_fake_fb_shutdown(struct platform_device *pdev)
{
	struct hisi_composer_device *fake_comp_dev = NULL;

	fake_comp_dev = platform_get_drvdata(pdev);
	if (!fake_comp_dev)
		return;

	if (fake_comp_dev->ov_data.fix_panel_info) {
		kfree(fake_comp_dev->ov_data.fix_panel_info);
		fake_comp_dev->ov_data.fix_panel_info = NULL;
	}
}

static const struct of_device_id hisi_disp_fake_fb_match_table[] = {
	{
		.compatible = DTS_COMP_FAKE_FB,
		.data = NULL,
	},
	{},
};
MODULE_DEVICE_TABLE(of, hisi_disp_fake_fb_match_table);

static struct platform_driver g_hisi_fake_fb_driver = {
	.probe = hisi_disp_fake_fb_probe,
	.remove = NULL,
	.suspend = NULL,
	.resume = NULL,
	.shutdown = hisi_disp_fake_fb_shutdown,
	.driver = {
		.name = "fake_fb",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(hisi_disp_fake_fb_match_table),
	},
};

static int __init hisi_disp_fake_fb_init(void)
{
	int ret = 0;

	disp_pr_info("enter ++++++");

	ret = platform_driver_register(&g_hisi_fake_fb_driver);
	if (ret) {
		disp_pr_err("platform_driver_register failed, error=%d!\n", ret);
		return ret;
	}

	disp_pr_info("exit ------");
	return ret;
}

module_init(hisi_disp_fake_fb_init);

MODULE_DESCRIPTION("Hisilicon Fake Panel Driver");
MODULE_LICENSE("GPL v2");

