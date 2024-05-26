/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include "hisi_chrdev.h"
#include "hisi_disp_debug.h"
#include "hisi_disp_rm.h"
#include "hisi_disp_rm_operator.h"
#include "hisi_dpu_module.h"
#include "hisi_operators_manager.h"

/* debug for alloc resource from kernel, not from hwc */
#ifdef DEBUG_ALLOC_RESOURCE_FROM_KERNEL
struct hisi_disp_rm *g_dpu_resource_manager = NULL;
#endif

static void hisi_rm_init_data(struct platform_device *rm_dev, void *dev_data, void *input, void *panel_ops)
{
	struct hisi_disp_rm *rm_data = (struct hisi_disp_rm *)dev_data;

	rm_data->pdev = rm_dev;
}

struct hisi_disp_rm *hisi_disp_rm_platform_device(void)
{
	struct hisi_disp_rm *rm_dev = NULL;

	/* create parent device, such as mipi_dsi device */
	rm_dev = hisi_drv_create_device(HISI_DISP_RM_DEV_NAME, sizeof(*rm_dev), hisi_rm_init_data, NULL, NULL);
	if (!rm_dev) {
		disp_pr_err("create %s device fail", HISI_DISP_RM_DEV_NAME);
		return NULL;
	}

	return rm_dev;
}

static ssize_t hisi_disp_rm_debug_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return strlen(buf);
}

static ssize_t hisi_disp_rm_debug_store(struct device *device,
			struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static struct device_attribute rm_attrs[] = {
	__ATTR(rm_debug, S_IRUGO | S_IWUSR, hisi_disp_rm_debug_show, hisi_disp_rm_debug_store),

	/* TODO: other attrs */
};

static int hisi_disp_rm_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int hisi_disp_rm_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static long hisi_disp_rm_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	return 0;
}

static struct file_operations dte_rm_fops = {
	.owner = THIS_MODULE,
	.open = hisi_disp_rm_open,
	.release = hisi_disp_rm_release,
	.unlocked_ioctl = hisi_disp_rm_ioctl,
	.compat_ioctl =  hisi_disp_rm_ioctl,
};

static void hisi_rm_init_operator_manager(struct hisi_disp_rm *rm_dev)
{
	hisi_disp_init_operators(hisi_disp_config_get_operators(), COMP_OPS_TYPE_MAX, NULL);
}

static void hisi_rm_init_operator_resource(struct hisi_disp_rm *rm_dev)
{
	struct list_head *operator_list = NULL;
	struct hisi_comp_operator *operator = NULL;
	struct hisi_operator_type *node = NULL;
	struct hisi_operator_type *_node_ = NULL;
	struct hisi_disp_resource *resource = NULL;
	struct hisi_disp_resource_type *resource_type;
	uint32_t i;

	disp_pr_info(" ++++ ");

	operator_list = hisi_disp_get_operator_list();
	if (!operator_list)
		return;

	list_for_each_entry_safe(node, _node_, operator_list, list_node) {
		if (!node)
			continue;

		resource_type = &rm_dev->operator_resource[node->type];
		resource_type->type = node->type;
		resource_type->request_resource_ops = hisi_disp_rm_get_request_func(resource_type->type);
		INIT_LIST_HEAD(&resource_type->resource_list);

		disp_pr_debug("init operator type = %u, operator_count=%u", resource_type->type, node->operator_count);

		for (i = 0; i < node->operator_count; i++) {
			if (!node->operators) {
				disp_pr_err("node->operators is null ptr i = %u", i);
				break;
			}

			operator = node->operators[i];
			resource = hisi_disp_rm_create_operator(operator->id_desc, operator);
			disp_pr_debug("&operator=0x%p", operator);
			if (!resource) {
				disp_pr_err("create operator fail, id=%u", operator->id_desc.id);
				continue;
			}

			list_add_tail(&resource->list_node, &resource_type->resource_list);
			disp_pr_debug("create resource id = 0x%x", resource->id.id);
			disp_pr_debug("resource_type->resource_list = 0x%p", &resource_type->resource_list);
			disp_pr_debug("resource->list_node = 0x%p", &resource->list_node);
		}
	}

	disp_pr_info(" ---- ");
}

static void hisi_disp_rm_create_chrdev(struct hisi_disp_rm *rm_dev)
{
	/* init chrdev info */
	rm_dev->rm_chrdev.name = HISI_DISP_RM_DEV_NAME;
	rm_dev->rm_chrdev.id = 1;
	rm_dev->rm_chrdev.chrdev_fops = &dte_rm_fops;

	hisi_disp_create_chrdev(&rm_dev->rm_chrdev, rm_dev);
	hisi_disp_create_attrs(rm_dev->rm_chrdev.dte_cdevice, rm_attrs, ARRAY_SIZE(rm_attrs));
}

static int hisi_disp_rm_probe(struct platform_device *pdev)
{
	struct hisi_disp_rm *rm_dev = NULL;
	struct device_node *np = NULL;
	int ret;

	disp_pr_info("resource manager enter ++");

	rm_dev = devm_kzalloc(&pdev->dev, sizeof(*rm_dev), GFP_KERNEL);
	if (!rm_dev) {
		disp_pr_err("alloc rm device data fail");
		return -1;
	}
	rm_dev->pdev = pdev;

	platform_set_drvdata(pdev, (void*)rm_dev);

	/* 1, read config from dtsi */
	ret = hisi_disp_init_dpu_config(pdev);
	if (ret) {
		disp_pr_err("init dpu config fail");
		return -1;
	}

	/* 2, init operator manager */
	hisi_rm_init_operator_manager(rm_dev);

	/* 3, init all resource, include buffer for lreg and operators */
	hisi_rm_init_operator_resource(rm_dev);

	/* TODO: init rm priv ops */
	// hisi_disp_rm_init_priv_ops(&rm_dev->priv_ops);

	/* create chrdev */
	hisi_disp_rm_create_chrdev(rm_dev);

#ifdef DEBUG_ALLOC_RESOURCE_FROM_KERNEL
	g_dpu_resource_manager = rm_dev;
#endif

	disp_pr_info("resource manager enter --");
	return 0;
}

static int hisi_disp_rm_remove(struct platform_device *pdev)
{
	/* TODO: remove device */

	return 0;
}

static const struct of_device_id hisi_disp_rm_match_table[] = {
	{
		.compatible = HISI_DTS_COMP_DPU_RM,
		.data = NULL,
	},
	{},
};
MODULE_DEVICE_TABLE(of, hisi_disp_rm_match_table);

static struct platform_driver g_hisi_disp_rm_driver = {
	.probe = hisi_disp_rm_probe,
	.remove = hisi_disp_rm_remove,
	.suspend = NULL,
	.resume = NULL,
	.shutdown = NULL,
	.driver = {
		.name = HISI_DISP_RM_DEV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(hisi_disp_rm_match_table),
	},
};


static int hisi_drv_rm_init(void)
{
	int ret;

	disp_pr_info("enter +++++++ \n");

	ret = platform_driver_register(&g_hisi_disp_rm_driver);
	if (ret) {
		disp_pr_info("platform_driver_register failed, error=%d!\n", ret);
		return ret;
	}

	disp_pr_info("enter -------- \n");
	return ret;
}


module_init(hisi_drv_rm_init);

MODULE_DESCRIPTION("Hisilicon Resource Manager Driver");
MODULE_LICENSE("GPL v2");

