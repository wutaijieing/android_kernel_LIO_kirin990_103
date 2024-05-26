/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
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
#include "hdmitx_drv.h"

struct platform_device *g_dkmd_hdmitx_devive = NULL;

static void hdmitx_panel_connector_dts_parse(struct dkmd_connector_info *pinfo, struct device_node *np)
{
	uint32_t connector_count = 0;
	uint32_t value = 0;
	const __be32 *p = NULL;
	struct property *prop = NULL;

	if (of_property_read_u32(np, "active_flag", &pinfo->active_flag) != 0)
		dpu_pr_info("get active flag failed");

	of_property_for_each_u32(np, "pipe_sw_post_chn_idx", prop, p, value) {
		if (connector_count >= MAX_CONNECT_CHN_NUM)
			break;

		dpu_pr_info("connector sw_post_chn_idx=%#x is active_flag=%u", value, pinfo->active_flag);
		pinfo->sw_post_chn_idx[connector_count] = value;
		dpu_assert(!g_conn_manager->connector[value % PIPE_SW_POST_CH_MAX]);
		pinfo->sw_post_chn_num = ++connector_count;
	}

	if (pinfo->sw_post_chn_num > EXTERNAL_CONNECT_CHN_IDX)
		get_primary_connector(pinfo)->bind_connector = get_external_connector(pinfo);
}

static int32_t hdmitx_panel_parse_dt(struct hdmitx_private *priv, struct device_node *np)
{
	int ret;
	struct dkmd_connector_info *pinfo = &priv->connector_info;

	priv->hdmitx_base = of_iomap(np, 0);
	if (!priv->hdmitx_base) {
		dpu_pr_err("failed to get hidptx_base!\n");
		return -ENXIO;
	}

	priv->hsdt1_crg_base = of_iomap(np, 1);
	if (!priv->hsdt1_crg_base) {
		dpu_pr_err("failed to get hsdt1_crg_base!\n");
		return -ENXIO;
	}

	priv->hsdt1_sub_harden_base = of_iomap(np, 2);
	if (!priv->hsdt1_sub_harden_base) {
		dpu_pr_err("failed to get hsdt1_sub_harden_base!\n");
		return -ENXIO;
	}

	ret = of_property_read_u32(np, "fpga_flag", &pinfo->base.fpga_flag);
	if (ret) {
		dpu_pr_info("get fpga_flag failed!\n");
		pinfo->base.fpga_flag = 0;
	}
	dpu_pr_info("fpga_flag=%#x", pinfo->base.fpga_flag);

	ret = of_property_read_u32(np, "pipe_sw_itfch_idx", &pinfo->base.pipe_sw_itfch_idx);
	if (ret) {
		dpu_pr_info("get pipe_sw_itfch_idx failed!\n");
		pinfo->base.pipe_sw_itfch_idx = PIPE_SW_PRE_ITFCH1;
	}
	dpu_pr_info("pipe_sw_pre_itfch_idx=%#x", pinfo->base.pipe_sw_itfch_idx);

	ret = of_property_read_u32(np, "fake_panel_flag", &pinfo->base.fake_panel_flag);
	if (ret)
		pinfo->base.fake_panel_flag = 0;
	dpu_pr_info("fake_panel_flag=%#x", pinfo->base.fake_panel_flag);

	hdmitx_panel_connector_dts_parse(pinfo, np);

	return 0;
}

static const struct of_device_id hdmitx_device_match_table[] = {
	{
		.compatible = DTS_COMP_HDMI,
		.data = NULL,
	},
	{},
};
MODULE_DEVICE_TABLE(of, hdmitx_device_match_table);

static int32_t hdmitx_probe(struct platform_device *pdev)
{
	struct hdmitx_private *priv = NULL;

	dpu_pr_info("+\n");

	if (!is_connector_manager_available())
		return -EPROBE_DEFER;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		dpu_pr_err("alloc offline driver private data failed!\n");
		return -EINVAL;
	}

	priv->device = pdev;
	priv->dev = &pdev->dev;
	if (hdmitx_panel_parse_dt(priv, pdev->dev.of_node)){
		dpu_pr_err("parse dts error!\n");
		return -EINVAL;
	}

	/* this is the tail device */
	priv->connector_info.base.peri_device = pdev;
	priv->connector_info.base.name = DEV_NAME_HDMI;
	priv->connector_info.base.type = PANEL_HDMI;

	/* add private data to device */
	platform_set_drvdata(pdev, &priv->connector_info);

	/* will create chrdev: /dev/gfx_hdmi  */
	if (register_connector(&priv->connector_info) != 0) {
		dpu_pr_warn("device register failed, need retry!\n");
		devm_kfree(&pdev->dev, priv);
		return -EPROBE_DEFER;
	}
	g_dkmd_hdmitx_devive = pdev;
	dpu_pr_info("-\n");

	return 0;
}

/**
 * Clear resource when device removed but not for devicetree device
 */
static int32_t hdmitx_remove(struct platform_device *pdev)
{
	struct hdmitx_private *priv = platform_get_drvdata(pdev);

	if (!priv) {
		dpu_pr_err("get dsi private data failed!\n");
		return -EINVAL;
	}
	unregister_connector(&priv->connector_info);

	dpu_pr_info("%s remove complete!\n", priv->connector_info.base.name);

	return 0;
}

static struct platform_driver hdmitx_platform_driver = {
	.probe  = hdmitx_probe,
	.remove = hdmitx_remove,
	.driver = {
		.name  = DEV_NAME_HDMI,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(hdmitx_device_match_table),
	}
};

static int32_t __init hdmitx_register(void)
{
	return platform_driver_register(&hdmitx_platform_driver);
}

static void __exit hdmitx_unregister(void)
{
	platform_driver_unregister(&hdmitx_platform_driver);
}

late_initcall(hdmitx_register);
module_exit(hdmitx_unregister);

MODULE_AUTHOR("Graphics Display");
MODULE_DESCRIPTION("HDMITX Module Driver");
MODULE_LICENSE("GPL");

