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
#include "dkmd_backlight.h"

#define PANEL_UNIFORM_NAME "gfx_panel"

static void panel_base_connector_dts_parse(struct dkmd_connector_info *pinfo, struct device_node *np)
{
	uint32_t connector_count = 0;
	uint32_t value = 0;
	const __be32 *p = NULL;
	struct property *prop = NULL;

	if (of_property_read_u32(np, ACTIVE_FLAG_NAME, &pinfo->active_flag) != 0)
		dpu_pr_info("get active flag failed");

	of_property_for_each_u32(np, "pipe_sw_post_chn_idx", prop, p, value) {
		if (connector_count >= MAX_CONNECT_CHN_NUM)
			break;

		dpu_pr_info("connector sw_post_chn_idx=%#x is active_flag=%u", value, pinfo->active_flag);
		pinfo->sw_post_chn_idx[connector_count] = value;
		dpu_assert(!g_conn_manager->connector[value % PIPE_SW_POST_CH_MAX]);
		pinfo->sw_post_chn_num = ++connector_count;
	}

	if (pinfo->sw_post_chn_num > EXTERNAL_CONNECT_CHN_IDX) {
		dpu_pr_info("cphy 1+1 or dual-mipi mode!\n");
		get_primary_connector(pinfo)->bind_connector = get_external_connector(pinfo);
	}
}

int32_t panel_base_of_device_setup(struct panel_drv_private *priv)
{
	int32_t ret;
	struct dkmd_connector_info *pinfo = &priv->connector_info;
	struct device_node *np = priv->pdev->dev.of_node;

	/* 1. config base object info
	 * would be used for framebuffer setup
	 */
	pinfo->base.xres = 1080;
	pinfo->base.yres = 1920;

	/* When calculating DPI needs the following parameters */
	pinfo->base.width = 75;
	pinfo->base.height = 133;

	ret = of_property_read_u32(np, FPGA_FLAG_NAME, &pinfo->base.fpga_flag);
	if (ret) {
		dpu_pr_info("get fpga_flag failed!\n");
		pinfo->base.fpga_flag = 0;
	}
	dpu_pr_info("fpga_flag=%#x", pinfo->base.fpga_flag);

	ret = of_property_read_u32(np, LCD_DISP_TYPE_NAME, &pinfo->base.type);
	if (ret) {
		dpu_pr_info("get lcd_display_type failed!\n");
		pinfo->base.type = PANEL_MIPI_VIDEO;
	}
	dpu_pr_info("panel_type=%#x", pinfo->base.type);

	ret = of_property_read_u32(np, PIPE_SW_ITFSW_IDX_NAME, &pinfo->base.pipe_sw_itfch_idx);
	if (ret) {
		dpu_pr_info("get pipe_sw_itfch_idx failed!\n");
		pinfo->base.pipe_sw_itfch_idx = PIPE_SW_PRE_ITFCH0;
	}
	dpu_pr_info("pipe_sw_pre_itfch_idx=%#x", pinfo->base.pipe_sw_itfch_idx);

	ret = of_property_read_u32(np, FAKE_PANEL_FLAG_NAME, &pinfo->base.fake_panel_flag);
	if (ret)
		pinfo->base.fake_panel_flag = 0;
	dpu_pr_info("fake_panel_flag=%#x", pinfo->base.fake_panel_flag);

	panel_base_connector_dts_parse(pinfo, np);

	/* 2. config connector info
	 * would be used for dsi & composer setup
	 */
	pinfo->bpp = LCD_RGB888;
	pinfo->bgr_fmt = LCD_RGB;

	/* not be used by now */
	pinfo->orientation = LCD_PORTRAIT;

	ret = of_property_read_u32(np, LCD_IFBC_TYPE_NAME, &pinfo->ifbc_type);
	if (ret) {
		dpu_pr_info("get ifbc_type failed!\n");
		pinfo->ifbc_type = IFBC_TYPE_NONE;
	}
	dpu_pr_info("ifbc_type=%#x", pinfo->ifbc_type);
	pinfo->vsync_ctrl_type = 0; /* default disable */

	priv->bl_info.bl_min = 1;
	priv->bl_info.bl_max = 255;
	priv->bl_info.bl_default = 102;

	ret = of_property_read_u32(np, LCD_BL_TYPE_NAME, &priv->bl_info.bl_type);
	if (ret) {
		dpu_pr_info("get lcd_bl_type failed!\n");
		priv->bl_info.bl_type = BL_SET_BY_MIPI;
	}
	dpu_pr_info("bl_type=%#x", priv->bl_info.bl_type);

	ret = of_property_read_string_index(np, LCD_BLPWM_DEV_NAME, 0, &priv->bl_ctrl.bl_dev_name);
	if (ret)
		priv->bl_ctrl.bl_dev_name = NULL;
	dpu_pr_info("get bl dev name %s", priv->bl_ctrl.bl_dev_name);

	if (!is_fake_panel(&pinfo->base))
		panel_drv_data_setup(priv, np);

	return 0;
}

void panel_base_of_device_release(struct panel_drv_private *priv)
{
	dpu_pr_info("base release enter!\n");
	(void)priv;
}

panel_device_match_data(fake_panel_info, FAKE_PANEL_ID, panel_base_of_device_setup, panel_base_of_device_release);

struct panel_table {
	int32_t index;
	const char *panel_desc;
};

static struct panel_table s_lcd_tabel[] = {
	{ FAKE_PANEL_ID, DTS_COMP_FAKE_PANEL, },
	{ PANEL_TD4322_ID, DTS_COMP_PANEL_TD4322, },
	{ PANEL_NT37700P_ID, DTS_COMP_PANEL_NT37700P, },
	{ PANEL_NT37800A_ID, DTS_COMP_PANEL_NT37800A, },
	{ PANEL_NT37701_BRQ_ID, DTS_COMP_PANEL_NT37701_BRQ, },
	{ PANEL_RM69091_ID, DTS_COMP_PANEL_RM69091, },
	{ PANEL_HX5293_ID, DTS_COMP_PANEL_HX5293, },
	{ PANEL_NT36870_ID, DTS_COMP_PANEL_NT36870, },
};

static const struct of_device_id panel_device_match_table[] = {
	{
		.compatible = DTS_COMP_FAKE_PANEL,
		.data = &fake_panel_info,
	},
	{
		.compatible = DTS_COMP_PANEL_TD4322,
		.data = &td4322_panel_info,
	},
	{
		.compatible = DTS_COMP_PANEL_NT37700P,
		.data = &nt37700p_panel_info,
	},
	{
		.compatible = DTS_COMP_PANEL_NT37800A,
		.data = &nt37800a_panel_info,
	},
	{
		.compatible = DTS_COMP_PANEL_NT37701_BRQ,
		.data = &nt37701_brq_panel_info,
	},
	{
		.compatible = DTS_COMP_PANEL_RM69091,
		.data = &rm69091_panel_info,
	},
	{
		.compatible = DTS_COMP_PANEL_HX5293,
		.data = &hx5293_panel_info,
	},
	{
		.compatible = DTS_COMP_PANEL_NT36870,
		.data = &nt36870_panel_info,
	},
	{},
};
MODULE_DEVICE_TABLE(of, panel_device_match_table);

static int panel_probe(struct platform_device *pdev)
{
	const struct panel_match_data *data = NULL;
	struct panel_drv_private *priv = NULL;

	if (!is_connector_manager_available())
		return -EPROBE_DEFER;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		dpu_pr_err("alloc panel driver private data failed!\n");
		return -EINVAL;
	}
	priv->pdev = pdev;
	/* this is the tail device */
	priv->connector_info.base.peri_device = pdev;

	/* add private data to device */
	platform_set_drvdata(pdev, priv);

	data = of_device_get_match_data(&pdev->dev);
	if (!data) {
		dpu_pr_err("get panel match data failed!\n");
		return -EINVAL;
	}
	dpu_pr_info("panel:%s !\n", s_lcd_tabel[data->lcd_id].panel_desc);
	priv->connector_info.base.name = PANEL_UNIFORM_NAME;

	if (data->of_device_setup(priv)) {
		dpu_pr_err("Device initialization is failed!\n");
		return -EINVAL;
	}

	/* Initialization function interface pointer */
	panel_dev_data_setup(priv);

	if (register_connector(&priv->connector_info) != 0) {
		dpu_pr_warn("device register failed, need retry!\n");
		data->of_device_release(priv);
		return -EPROBE_DEFER;
	}

	priv->bl_ctrl.bl_led_index = (uint32_t)priv->connector_info.base.pipe_sw_itfch_idx;
	priv->bl_ctrl.bl_info = &priv->bl_info;
	priv->bl_ctrl.connector = get_primary_connector(&priv->connector_info);
	dkmd_backlight_init_bl_ctrl(priv->bl_info.bl_type, &priv->bl_ctrl);
	dkmd_backlight_register_led(pdev, &priv->bl_ctrl);

	dpu_pr_info("exit\n");
	return 0;
}

/**
 * @brief clear resource when device removed but not for devicetree device
 *
 * @param pdev
 * @return int
 */
static int panel_remove(struct platform_device *pdev)
{
	const struct panel_match_data *data = NULL;
	struct panel_drv_private *priv = NULL;

	priv = (struct panel_drv_private *)platform_get_drvdata(pdev);
	if (!priv) {
		dpu_pr_err("get dsi private data failed!\n");
		return -EINVAL;
	}
	unregister_connector(&priv->connector_info);

	data = of_device_get_match_data(&pdev->dev);
	if (data != NULL)
		data->of_device_release(priv);

	dkmd_backlight_deinit_bl_ctrl(&priv->bl_ctrl);
	dkmd_backlight_unregister_led(&priv->bl_ctrl);

	dpu_pr_info("%s panel remove complete!\n", priv->connector_info.base.name);
	return 0;
}

static int panel_pm_suspend(struct device *dev)
{
	struct panel_drv_private *priv = NULL;

	priv = (struct panel_drv_private *)dev_get_drvdata(dev);
	if (!priv) {
		dpu_pr_err("get dsi private data failed!\n");
		return -EINVAL;
	}
	dpu_pr_info("enter +!\n");

	connector_device_shutdown(&priv->connector_info);

	dpu_pr_info("exit -!\n");
	return 0;
}

static int panel_pm_resume(struct device *dev)
{
	return 0;
}

static void panel_shutdown(struct platform_device *pdev)
{
	struct panel_drv_private *priv = NULL;

	priv = (struct panel_drv_private *)platform_get_drvdata(pdev);
	if (!priv) {
		dpu_pr_err("get dsi private data failed!\n");
		return;
	}
	dpu_pr_info("enter +!\n");

	connector_device_shutdown(&priv->connector_info);

	dpu_pr_info("exit -!\n");
}

static const struct dev_pm_ops panel_pm_ops = {
	.suspend = panel_pm_suspend,
	.resume = panel_pm_resume,
};

static struct platform_driver panel_platform_driver = {
	.probe  = panel_probe,
	.remove = panel_remove,
	.shutdown = panel_shutdown,
	.driver = {
		.name  = "dsi_panel",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(panel_device_match_table),
		.pm = &panel_pm_ops,
	}
};

static int __init panel_register(void)
{
	return platform_driver_register(&panel_platform_driver);
}

static void __exit panel_unregister(void)
{
	platform_driver_unregister(&panel_platform_driver);
}

module_init(panel_register);
module_exit(panel_unregister);

MODULE_AUTHOR("Graphics Display");
MODULE_DESCRIPTION("Panel Module Driver");
MODULE_LICENSE("GPL");
