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
#include "dpu_offline_drv.h"

static const struct of_device_id offline_device_match_table[] = {
	{
		.compatible = DTS_COMP_OFFLINE,
		.data = NULL,
	},
	{},
};
MODULE_DEVICE_TABLE(of, offline_device_match_table);

static int32_t offline_probe(struct platform_device *pdev)
{
	int32_t ret;
	struct offline_private *priv = NULL;
	struct dkmd_connector_info *pinfo = NULL;

	dpu_pr_info("enter");

	if (!is_connector_manager_available())
		return -EPROBE_DEFER;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		dpu_pr_err("alloc offline driver private data failed!\n");
		return -EINVAL;
	}
	priv->pdev = pdev;
	pinfo = &priv->connector_info;
	/* this is the tail device */
	pinfo->base.peri_device = pdev;
	pinfo->base.name = DEV_NAME_OFFLINE;
	pinfo->base.type = PANEL_OFFLINECOMPOSER;

	ret = of_property_read_u32(pdev->dev.of_node, "pipe_sw_itfch_idx", &pinfo->base.pipe_sw_itfch_idx);
	if (ret) {
		dpu_pr_info("get pipe_sw_itfch_idx failed!\n");
		pinfo->base.pipe_sw_itfch_idx = DPU_SCENE_OFFLINE_0;
	}
	dpu_pr_info("pipe_sw_pre_itfch_idx=%#x", pinfo->base.pipe_sw_itfch_idx);

	pinfo->active_flag = 1;
	pinfo->sw_post_chn_num = 1;
	pinfo->sw_post_chn_idx[PRIMARY_CONNECT_CHN_IDX] = PIPE_SW_POST_CH_OFFLINE;

	/* add private data to device */
	platform_set_drvdata(pdev, &priv->connector_info);

	if (register_connector(&priv->connector_info) != 0) {
		dpu_pr_warn("device register failed, need retry!\n");
		return -EPROBE_DEFER;
	}
	dpu_pr_info("exit\n");

	return 0;
}

/**
 * Clear resource when device removed but not for devicetree device
 */
static int32_t offline_remove(struct platform_device *pdev)
{
	struct offline_private *priv = platform_get_drvdata(pdev);
	if (!priv) {
		dpu_pr_err("get dsi private data failed!\n");
		return -EINVAL;
	}

	unregister_connector(&priv->connector_info);

	dpu_pr_info("%s remove complete!\n", priv->connector_info.base.name);

	return 0;
}

static struct platform_driver offline_platform_driver = {
	.probe  = offline_probe,
	.remove = offline_remove,
	.driver = {
		.name  = DEV_NAME_OFFLINE,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(offline_device_match_table),
	}
};

static int32_t __init offline_register(void)
{
	return platform_driver_register(&offline_platform_driver);
}

static void __exit offline_unregister(void)
{
	platform_driver_unregister(&offline_platform_driver);
}

late_initcall(offline_register);
module_exit(offline_unregister);

MODULE_AUTHOR("Graphics Display");
MODULE_DESCRIPTION("Offline Module Driver");
MODULE_LICENSE("GPL");
