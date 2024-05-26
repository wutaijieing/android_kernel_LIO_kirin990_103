/*
 *  ISP driver, isp_fstcma.c
 *
 * Copyright (c) 2013 ISP Technologies CO., Ltd.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/printk.h>
#include <linux/remoteproc.h>
#include <platform_include/isp/linux/hisp_remoteproc.h>
#include <linux/of_reserved_mem.h>
#include <linux/version.h>

#define DTS_COMP_FSTCMA_NAME    "hisilicon,isp-fastboot-cma"

struct hisp_fstcma_struct {
	struct device *device;
};

struct hisp_fstcma_struct fstcma_dev;

void *hisp_fstcma_alloc(dma_addr_t *dma_handle, size_t size, gfp_t flag)
{
	struct hisp_fstcma_struct *dev = &fstcma_dev;
	void *va = NULL;

	pr_info("%s: +\n", __func__);
	if (dev->device == NULL) {
		pr_err("%s: failed.\n", __func__);
		return NULL;
	}

	va = dma_alloc_coherent(dev->device, size, dma_handle, flag);
	if (va == NULL) {
		pr_err("%s: alloc failed.\n", __func__);
		return NULL;
	}

	pr_info("%s: -\n", __func__);
	return va;
}

void hisp_fstcma_free(void *va, dma_addr_t dma_handle, size_t size)
{
	struct hisp_fstcma_struct *dev = &fstcma_dev;

	pr_info("%s: +\n", __func__);

	if (va == NULL || dma_handle == 0) {
		pr_info("%s: cma_va.%pK\n", __func__, va);
		return;
	}

	dma_free_coherent(dev->device, size, va, dma_handle);

	pr_info("%s: -\n", __func__);
}

static int hisp_fstcma_probe(struct platform_device *pdev)
{
	struct hisp_fstcma_struct *dev = &fstcma_dev;
	int ret;

	pr_info("%s: +\n", __func__);

	dev->device = &(pdev->dev);

	ret = of_reserved_mem_device_init(dev->device);
	if (ret != 0) {
		pr_err("%s: init failed, ret.%d\n", __func__, ret);
		goto out;
	}

	pr_info("%s: -\n", __func__);
	return 0;

out:
	dev->device = NULL;

	pr_err("%s: error-\n", __func__);
	return ret;
}

static int hisp_fstcma_remove(struct platform_device *pdev)
{
	struct hisp_fstcma_struct *dev = &fstcma_dev;

	if (dev->device == NULL) {
		pr_err("%s: failed.\n", __func__);
		return -EINVAL;
	}

	of_reserved_mem_device_release(dev->device);

	return 0;
}

static const struct of_device_id hisp_fstcma_of_match[] = {
	{ .compatible = DTS_COMP_FSTCMA_NAME},
	{ },
};
MODULE_DEVICE_TABLE(of, hisp_fstcma_of_match);

static struct platform_driver hisp_fstcma_driver = {
	.driver = {
		.owner      = THIS_MODULE,
		.name       = "fastboot_cma",
		.of_match_table = of_match_ptr(hisp_fstcma_of_match),
	},
	.probe  = hisp_fstcma_probe,
	.remove = hisp_fstcma_remove,
};

static int __init hisp_fstcma_init(void)
{
	pr_info("%s: +\n", __func__);
	return platform_driver_register(&hisp_fstcma_driver);
}
subsys_initcall(hisp_fstcma_init);

static void __exit hisp_fstcma_exit(void)
{
	platform_driver_unregister(&hisp_fstcma_driver);
}
module_exit(hisp_fstcma_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ISP hispcma module");
