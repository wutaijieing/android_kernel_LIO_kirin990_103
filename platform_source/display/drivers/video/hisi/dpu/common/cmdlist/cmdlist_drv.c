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
#include "cmdlist.h"
#include "cmdlist_drv.h"

struct cmdlist_private g_dpu_cmdlist_priv;
EXPORT_SYMBOL(g_dpu_cmdlist_priv);

bool dpu_cmdlist_enable = true;
MODULE_PARM_DESC(dpu_cmdlist_enable, "cmdlist module config enable");
module_param(dpu_cmdlist_enable, bool, 0600);

#ifdef CMDLIST_DEBUG
static int cmdlist_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int cmdlist_close(struct inode *inode, struct file *filp)
{
	return 0;
}

static const struct file_operations cmdlist_fops = {
	.owner = THIS_MODULE,
	.open = cmdlist_open,
	.release = cmdlist_close,
};

static int cmdlist_debug_setup(struct cmdlist_private *priv)
{
	int ret = 0;

	priv->chr_major = register_chrdev(0/*UNNAMED_MAJOR*/, DEV_NAME_CMDLIST, &cmdlist_fops);
	if (priv->chr_major < 0 ) {
		pr_err("error %d registering cmdlist chrdev!\n", ret);
		return -ENXIO;
	}

	priv->chr_class = class_create(THIS_MODULE, "cmdlist");
	if (IS_ERR(priv->chr_class)) {
		pr_err("create cmdlist class fail!\n");
		ret = PTR_ERR(priv->chr_class);
		goto err_chr_class;
	}

	priv->chr_dev = device_create(priv->chr_class, 0, MKDEV(priv->chr_major, 0/*minor*/), NULL, DEV_NAME_CMDLIST);
	if (IS_ERR(priv->chr_dev)) {
		pr_err("create cmdlist char device fail!\n");
		ret = PTR_ERR(priv->chr_dev);
		goto err_chr_device;
	}
	dev_set_drvdata(priv->chr_dev, priv);

	pr_debug("add cmdlist chr_dev device succ!\n");
	return ret;

err_chr_device:
	class_destroy(priv->chr_class);
err_chr_class:
	unregister_chrdev(priv->chr_major, DEV_NAME_CMDLIST);
	return ret;
}
#endif

static void cmdlist_of_device_setup(struct platform_device *pdev)
{
	int ret = 0;
	dma_addr_t dma_handle;
	struct cmdlist_private *priv = &g_dpu_cmdlist_priv;

	priv->of_dev = &pdev->dev;
	priv->dev_reg_base = of_iomap(priv->of_dev->of_node, 0);
	pr_info("dev_reg_base: 0x%x\n", priv->dev_reg_base);

	ret = dma_set_mask_and_coherent(priv->of_dev, DMA_BIT_MASK(64));
	if (ret != 0) {
		pr_err("dma set mask and coherent failed %d!\n",  ret);
		return;
	}

	/* One cmdlist node is inlcude cmd_header(size: 16Byte) and cmd_item[0..N](size: 16Byte)
	 * N: tatal_items[13:0] = 0x3FFF = 16383
	 */
	// FIXME,add 3x memory,to temporarily resolved allocate memeroy faild
	priv->sum_pool_size = roundup(ITEMS_MAX_NUM * ONE_ITEM_SIZE, PAGE_SIZE) * DPU_SCENE_MAX * 3;
	priv->pool_vir_addr = dma_alloc_coherent(priv->of_dev, priv->sum_pool_size, &dma_handle, GFP_KERNEL);
	if (!priv->pool_vir_addr) {
		pr_err("dma alloc 0x%zx Byte coherent failed %d!\n", priv->sum_pool_size, PTR_ERR(priv->pool_vir_addr));
		return;
	}
	priv->pool_phy_addr = dma_handle;
	memset(priv->pool_vir_addr, 0, priv->sum_pool_size);

	/**
	 * devm_gen_pool_create: The pool will be automatically destroyed by the device management code
	 */
	priv->memory_pool = devm_gen_pool_create(priv->of_dev, 4/*order*/, -1, DEV_NAME_CMDLIST);
	if (IS_ERR_OR_NULL(priv->memory_pool)) {
		pr_err("create memory pool failed %d!\n", PTR_ERR(priv->memory_pool));
		goto err_pool_create;
	}

	ret = gen_pool_add_virt(priv->memory_pool, (unsigned long)(uintptr_t)priv->pool_vir_addr, priv->pool_phy_addr, priv->sum_pool_size, -1);
	if (ret != 0) {
		pr_err("memory pool add failed %d!\n",  ret);
		goto err_pool_add;
	}
	pr_info("alloc pool[%p].size[0x%x] vir_addr=0x%x, phy_addr=0x%x\n",
		priv->memory_pool, priv->sum_pool_size, priv->pool_vir_addr, priv->pool_phy_addr);

	return;

err_pool_add:
	gen_pool_destroy(priv->memory_pool);
	priv->memory_pool = NULL;

err_pool_create:
	dma_free_coherent(priv->of_dev, priv->sum_pool_size, priv->pool_vir_addr, priv->pool_phy_addr);
	priv->pool_vir_addr = 0;
	priv->pool_phy_addr = 0;

	return;
}

static void cmdlist_of_device_release(struct platform_device *pdev)
{
	struct cmdlist_private *priv = NULL;

	priv = (struct cmdlist_private *)platform_get_drvdata(pdev);

	if (priv && (priv->pool_vir_addr != 0)) {
		pr_info("free dmabuf vir_addr=0x%x, phy_addr=0x%x\n", priv->pool_vir_addr, priv->pool_phy_addr);
		dma_free_coherent(priv->of_dev, priv->sum_pool_size, priv->pool_vir_addr, priv->pool_phy_addr);
		priv->pool_vir_addr = 0;
		priv->pool_phy_addr = 0;
		priv->memory_pool = NULL;
	}
}

#define HISI_MDC_MATCH_DATA(name, ver, imp1, imp2) \
	static struct cmdlist_match_data name = { .version = ver, .of_device_setup_impl = imp1, .of_device_release_impl = imp2 }

HISI_MDC_MATCH_DATA(cmdlist_basic, GENERIC_DEVICETREE_CMDLIST, cmdlist_of_device_setup, cmdlist_of_device_release);

static const struct of_device_id device_match_table[] = {
	{
		.compatible = DTS_COMP_CMDLIST,
		.data = &cmdlist_basic,
	},
	{},
};
MODULE_DEVICE_TABLE(of, device_match_table);

static int cmdlist_probe(struct platform_device *pdev)
{
	const struct cmdlist_match_data *data = NULL;
	struct cmdlist_private *priv = &g_dpu_cmdlist_priv;

	if (!pdev)
		return -EINVAL;

	data = of_device_get_match_data(&pdev->dev);
	if ((data != NULL) && (priv->device_initailed == 0)) {
		priv->pdev = pdev;
		data->of_device_setup_impl(pdev);
#ifdef CMDLIST_DEBUG
		cmdlist_debug_setup(priv);
#endif
		priv->device_initailed = 1;
		sema_init(&priv->sem, 1);
	}
	platform_set_drvdata(pdev, priv);

	pr_info("cmdlist_probe ---- \n");
	return 0;
}

/**
 * Clear resource when device removed but not for devicetree device.
 */
static int cmdlist_remove(struct platform_device *pdev)
{
	const struct cmdlist_match_data *data = NULL;
	struct cmdlist_private *priv = &g_dpu_cmdlist_priv;

#ifdef CMDLIST_DEBUG
	if (priv->chr_major > 0) {
		device_destroy(priv->chr_class, MKDEV(priv->chr_major, 0));
		class_destroy(priv->chr_class);
		unregister_chrdev(priv->chr_major, DEV_NAME_CMDLIST);
		priv->chr_major = 0;
		pr_debug("unregister cmdlist chrdev device succ!\n");
	}
#endif

	data = of_device_get_match_data(&pdev->dev);
	if (data != NULL) {
		data->of_device_release_impl(pdev);
	}
	priv->device_initailed = 0;

	pr_info("cmdlist_remove ---- \n");
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

static int __init cmdlist_register(void)
{
	return platform_driver_register(&cmdlist_platform_driver);
}

static void __exit cmdlist_unregister(void)
{
	platform_driver_unregister(&cmdlist_platform_driver);
}

module_init(cmdlist_register);
module_exit(cmdlist_unregister);

MODULE_AUTHOR("Kirin Graphics Display");
MODULE_DESCRIPTION("Cmdlist Function Driver");
MODULE_LICENSE("GPL");
