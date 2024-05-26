/**
 * @file cmdlist_drv.c
 * @brief Cmdlist device driver
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
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
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/of_device.h>
#include <linux/module.h>
#include <linux/genalloc.h>

#include "dkmd_log.h"
#include "chash.h"
#include "cmdlist_node.h"
#include "cmdlist_client.h"
#include "cmdlist_dev.h"
#include "cmdlist_drv.h"

int32_t g_cmdlist_dump_enable = 0;
module_param_named(cmdlist_dump_enable, g_cmdlist_dump_enable, int, 0600);
MODULE_PARM_DESC(cmdlist_dump_enable, "enable cmdlist config dump");

struct cmdlist_private g_cmdlist_priv;

static int32_t cmdlist_of_device_setup(struct platform_device *pdev)
{
	int32_t ret = 0;
	dma_addr_t dma_handle;
	struct cmdlist_private *cmd_priv = (struct cmdlist_private *)platform_get_drvdata(pdev);

	if (!cmd_priv)
		return -1;

	cmd_priv->of_dev = &pdev->dev;
	(void)of_property_read_u32(cmd_priv->of_dev->of_node, "scene_num", &cmd_priv->cmd_table.scene_num);
	dpu_pr_info("get scene_num: %d", cmd_priv->cmd_table.scene_num);

	(void)of_property_read_u32(cmd_priv->of_dev->of_node, "cmdlist_max_size", (uint32_t *)&cmd_priv->sum_pool_size);
	dpu_pr_info("get cmdlist_max_size: %d", cmd_priv->sum_pool_size);

	ret = dma_set_mask_and_coherent(cmd_priv->of_dev, DMA_BIT_MASK(64));
	if (ret != 0) {
		dpu_pr_err("dma set mask and coherent failed %d!", ret);
		return -1;
	}

	/* One cmdlist node is inlcude cmd_header(size: 16Byte) and cmd_item[0..N](size: 16Byte)
	 * N: tatal_items[13:0] = 0x3FFF = 16383
	 */
	cmd_priv->sum_pool_size =
		min(roundup(ITEMS_MAX_NUM * ONE_ITEM_SIZE, PAGE_SIZE) * cmd_priv->cmd_table.scene_num, cmd_priv->sum_pool_size);
	cmd_priv->pool_vir_addr = dma_alloc_coherent(cmd_priv->of_dev, cmd_priv->sum_pool_size, &dma_handle, GFP_KERNEL);
	if (!cmd_priv->pool_vir_addr) {
		dpu_pr_err("dma alloc 0x%zx Byte coherent failed %d!",
			cmd_priv->sum_pool_size, PTR_ERR(cmd_priv->pool_vir_addr));
		return -1;
	}
	cmd_priv->pool_phy_addr = dma_handle;

	/**
	 * devm_gen_pool_create: The pool will be automatically destroyed by the device management code
	 */
	cmd_priv->memory_pool = devm_gen_pool_create(cmd_priv->of_dev, 4 /* order */, -1, DEV_NAME_CMDLIST);
	if (IS_ERR_OR_NULL(cmd_priv->memory_pool)) {
		dpu_pr_err("create memory pool failed %d!", PTR_ERR(cmd_priv->memory_pool));
		goto err_pool_create;
	}

	ret = gen_pool_add_virt(cmd_priv->memory_pool, (unsigned long)(uintptr_t)cmd_priv->pool_vir_addr,
		cmd_priv->pool_phy_addr, cmd_priv->sum_pool_size, -1);
	if (ret != 0) {
		dpu_pr_err("memory pool add failed %d!", ret);
		goto err_pool_create;
	}
	sema_init(&cmd_priv->table_sem, 1);

	return 0;

err_pool_create:
	dma_free_coherent(cmd_priv->of_dev, cmd_priv->sum_pool_size, cmd_priv->pool_vir_addr, cmd_priv->pool_phy_addr);
	cmd_priv->pool_vir_addr = NULL;
	cmd_priv->pool_phy_addr = 0;
	return -1;
}

static void cmdlist_of_device_release(struct platform_device *pdev)
{
	struct cmdlist_private *cmd_priv = (struct cmdlist_private *)platform_get_drvdata(pdev);

	if (!cmd_priv)
		return;

	if (!cmd_priv->pool_vir_addr)
		return;

	dma_free_coherent(cmd_priv->of_dev, cmd_priv->sum_pool_size, cmd_priv->pool_vir_addr, cmd_priv->pool_phy_addr);
	cmd_priv->pool_vir_addr = NULL;
	cmd_priv->pool_phy_addr = 0;

	/* The pool will be automatically destroyed by the device management code */
	cmd_priv->memory_pool = NULL;
}

static struct cmdlist_match_data cmdlist_basic = {
	.version = GENERIC_DEVICETREE_CMDLIST,
	.of_device_setup = cmdlist_of_device_setup,
	.of_device_release = cmdlist_of_device_release
};

static const struct of_device_id device_match_table[] = {
	{
		.compatible = DTS_COMP_CMDLIST,
		.data = &cmdlist_basic,
	},
	{},
};
MODULE_DEVICE_TABLE(of, device_match_table);

static int32_t cmdlist_probe(struct platform_device *pdev)
{
	const struct cmdlist_match_data *data = NULL;
	struct cmdlist_private *cmd_priv = &g_cmdlist_priv;

	if (!pdev)
		return -EINVAL;

	data = of_device_get_match_data(&pdev->dev);
	if (!data) {
		dpu_pr_err("get device data failed!");
		return -EINVAL;
	}

	if (cmd_priv->device_initialized != 0) {
		dpu_pr_info("already initailed!");
		return 0;
	}
	memset(cmd_priv, 0, sizeof(*cmd_priv));
	cmd_priv->pdev = pdev;
	platform_set_drvdata(pdev, cmd_priv);

	if (data->of_device_setup(pdev)) {
		dpu_pr_err("Device initialization is failed!");
		return -EINVAL;
	}

	if (cmdlist_device_setup(cmd_priv) != 0) {
		data->of_device_release(pdev);
		dpu_pr_err("Device setup failed!");
		return -EINVAL;
	}
	cmd_priv->device_initialized = 1;

	return 0;
}

/**
 * Clear resource when device removed but not for devicetree device
 */
static int32_t cmdlist_remove(struct platform_device *pdev)
{
	const struct cmdlist_match_data *data = NULL;
	struct cmdlist_private *cmd_priv = (struct cmdlist_private *)platform_get_drvdata(pdev);

	if (!cmd_priv) {
		dpu_pr_err("cmd_priv is null!");
		return -EINVAL;
	}

	if (cmd_priv->device_initialized == 0) {
		dpu_pr_err("has no initialization!");
		return 0;
	}
	cmdlist_device_release(cmd_priv);

	data = of_device_get_match_data(&pdev->dev);
	if (data != NULL)
		data->of_device_release(pdev);

	cmd_priv->device_initialized = 0;
	return 0;
}

static struct platform_driver cmdlist_platform_driver = {
	.probe  = cmdlist_probe,
	.remove = cmdlist_remove,
	.driver = {
		.name  = DEV_NAME_CMDLIST,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(device_match_table),
	}
};

static int32_t __init cmdlist_register(void)
{
	return platform_driver_register(&cmdlist_platform_driver);
}

static void __exit cmdlist_unregister(void)
{
	platform_driver_unregister(&cmdlist_platform_driver);
}

module_init(cmdlist_register);
module_exit(cmdlist_unregister);

MODULE_AUTHOR("Graphics Display");
MODULE_DESCRIPTION("Cmdlist Module Driver");
MODULE_LICENSE("GPL");
