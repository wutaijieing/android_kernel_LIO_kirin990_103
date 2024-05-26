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

#include <linux/module.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/of.h>

#include "res_mgr.h"
#include "opr_mgr.h"
#include "gr_dev.h"
#include "dvfs.h"
#include "scene_id_mgr.h"

#define DPU_RES_DEV_NAME "dpu_res"

struct dpu_res *g_rm_dev;
EXPORT_SYMBOL(g_rm_dev);

// class graphics be create by fbmem
extern struct class *fb_class;

static ssize_t dpu_res_debug_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return strlen(buf);
}

static ssize_t dpu_res_debug_store(struct device *device,
			struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static struct device_attribute rm_attrs[] = {
	__ATTR(rm_debug, S_IRUGO | S_IWUSR, dpu_res_debug_show, dpu_res_debug_store),

	/* TODO: other attrs */
};

static void dpu_res_reset_resource_list(uint64_t res_types)
{
	struct dpu_res_resouce_node *_node_ = NULL;
	struct dpu_res_resouce_node *res_node = NULL;

	list_for_each_entry_safe(res_node, _node_, &g_rm_dev->resource_list, list_node) {
		if (!res_node)
			continue;

		dpu_pr_info("res_node->res_types==%llx res_ref_cnt=%d", res_node->res_type, atomic_read(&res_node->res_ref_cnt));
		if ((res_types & res_node->res_type) == 0)
			continue;

		if (atomic_sub_and_test(1, &res_node->res_ref_cnt)) {
			/* only one thread hold this res_node */
			if (res_node->reset)
				res_node->reset(res_node->data);
		} else {
			/* release current thread's res_node */
			if (res_node->release)
				res_node->release(res_node->data);
		}
	}
}

static int32_t dpu_res_open(struct inode *inode, struct file *filp)
{
	struct res_process_data *process_data = NULL;

	if (!filp) {
		dpu_pr_info("rm_dev or filp is null");
		return -1;
	}

	// TODO: add mutex
	if (!filp->private_data) {
		process_data = kzalloc(sizeof(*process_data), GFP_KERNEL);
		if (!process_data) {
			dpu_pr_err("kzalloc res process data fail");
			return -1;
		}
		atomic_set(&(process_data->ref_cnt), 0);
		filp->private_data = process_data;
	} else {
		process_data = (struct res_process_data *)filp->private_data;
	}

	atomic_inc(&process_data->ref_cnt);

	dpu_pr_info("process_data: %pK, ref_cnt = %d tgid=%d",
		process_data, atomic_read(&process_data->ref_cnt), task_tgid_vnr(current));

	return 0;
}

static int32_t dpu_res_release(struct inode *inode, struct file *filp)
{
	struct res_process_data *process_data = NULL;

	if (!filp) {
		dpu_pr_err("filp is null");
		return -1;
	}

	process_data = (struct res_process_data *)filp->private_data;
	if (!process_data) {
		dpu_pr_err("process_data is null");
		return -1;
	}

	dpu_pr_info("process_data: %pK, ref_cnt=%d res_types=%llx tgid=%d",
		process_data, atomic_read(&process_data->ref_cnt), process_data->res_types, task_tgid_vnr(current));

	if (atomic_read(&process_data->ref_cnt) == 0) {
		dpu_pr_warn("res_types=%llx is not opened, cannot release", process_data->res_types);
		return 0;
	}

	if (atomic_sub_and_test(1, &process_data->ref_cnt)) {
		dpu_res_reset_resource_list(process_data->res_types);

		kfree(filp->private_data);
		filp->private_data = NULL;
	}

	return 0;
}

static int32_t rm_check_ioctl(struct res_process_data *process_data, uint32_t cmd, void __user *argp)
{
	if (!argp) {
		dpu_pr_err("argp is null");
		return -EINVAL;
	}

	if (!process_data) {
		dpu_pr_err("process_data is null");
		return -EINVAL;
	}

	return 0;
}

static void dpu_res_inc_res_ref_cnt(uint64_t res_types)
{
	struct dpu_res_resouce_node *_node_ = NULL;
	struct dpu_res_resouce_node *res_node = NULL;

	list_for_each_entry_safe(res_node, _node_, &g_rm_dev->resource_list, list_node) {
		if (!res_node)
			continue;

		if ((res_types & res_node->res_type) == 0)
			continue;

		atomic_inc(&res_node->res_ref_cnt);
	}
}

static int32_t dpu_res_register_types(struct res_process_data *process_data, const void __user *argp)
{
	uint64_t types = 0;

	if (copy_from_user(&types, argp, sizeof(types))) {
		dpu_pr_err("copy from user client types fail");
		return -1;
	}
	process_data->res_types |= types;

	dpu_res_inc_res_ref_cnt(types);

	dpu_pr_debug("register types 0x%llx", types);

	return 0;
}

static int32_t dpu_res_ioctl_resource(struct res_process_data *process_data, uint32_t cmd, void __user *argp)
{
	struct dpu_res_resouce_node *_node_ = NULL;
	struct dpu_res_resouce_node *res_node = NULL;
	int32_t ret = -2;

	// hanle each resource manager
	list_for_each_entry_safe(res_node, _node_, &g_rm_dev->resource_list, list_node) {
		if (!res_node)
			continue;

		if (!res_node->ioctl)
			continue;

		ret = res_node->ioctl(res_node, cmd, argp);
		if (ret <= 0) { // mean this cmd have been handled by this node
			dpu_pr_debug("res node res_type=0x%x hanle this cmd 0x%x", res_node->res_type, cmd);
			break;
		}
	}

	if (ret != 0)
		dpu_pr_err("dpu_res_ioctl failed! cmd 0x%x ret=%d", cmd, ret);

	return ret;
}

static long dpu_res_ioctl(struct file *filp, uint32_t cmd, unsigned long arg)
{
	struct res_process_data *process_data = NULL;
	void __user *argp = (void __user *)(uintptr_t)arg;

	if (!filp) {
		dpu_pr_err("filp is null");
		return -1;
	}

	process_data = (struct res_process_data *)filp->private_data;
	if (rm_check_ioctl(process_data, cmd, argp) != 0)
		return -EINVAL;

	dpu_pr_debug("process_data: %pK, cmd=%#x tgid=%d", process_data, cmd, task_tgid_vnr(current));
	switch (cmd) {
	case RES_REGISTER_TYPES: // register client for each process
		return dpu_res_register_types(process_data, argp);
	default:
		return dpu_res_ioctl_resource(process_data, cmd, argp);
	}
}

static struct file_operations dpu_res_fops = {
	.owner = THIS_MODULE,
	.open = dpu_res_open,
	.release = dpu_res_release,
	.unlocked_ioctl = dpu_res_ioctl,
	.compat_ioctl =  dpu_res_ioctl,
};

static void dpu_res_create_chrdev(struct dpu_res *rm_dev)
{
	int ret;

	if (fb_class != NULL) {
		rm_dev->rm_chrdev.chr_class = fb_class;
	} else { /* maybe not support fbmem */
		rm_dev->rm_chrdev.chr_class = class_create(THIS_MODULE, "graphics");
		if (IS_ERR(rm_dev->rm_chrdev.chr_class)) {
			ret = PTR_ERR(rm_dev->rm_chrdev.chr_class);
			dpu_pr_err("Unable to create fb class; errno = %d\n", ret);
			rm_dev->rm_chrdev.chr_class = NULL;
		}
	}
	rm_dev->rm_chrdev.name = DPU_RES_DEV_NAME;
	rm_dev->rm_chrdev.fops = &dpu_res_fops;

	dkmd_create_chrdev(&rm_dev->rm_chrdev);
	dkmd_create_attrs(rm_dev->rm_chrdev.chr_dev, rm_attrs, ARRAY_SIZE(rm_attrs));
}

static void dpu_res_init_data(struct dpu_res_data *data)
{
	data->lbuf_size = dpu_config_get_lbuf_size();
	if (data->lbuf_size == 0)
		dpu_pr_err("get lbuf size fail, is 0");

	data->offline_scene_ids = dpu_config_get_offline_scene_ids(&data->offline_scene_id_count);
}

static void dpu_res_init_resource_list(struct list_head *res_head, struct dpu_res_data *rm_data)
{
	struct dpu_res_resouce_node *_node_ = NULL;
	struct dpu_res_resouce_node *res_node = NULL;

	INIT_LIST_HEAD(res_head);

	dpu_res_register_opr_mgr(res_head);
	dpu_res_register_gr_dev(res_head);
	dpu_res_register_dvfs(res_head);
	dpu_res_register_scene_id_mgr(res_head);

	list_for_each_entry_safe(res_node, _node_, res_head, list_node) {
		if (!res_node)
			continue;

		if (res_node->init) {
			res_node->data = res_node->init(rm_data);
			if (!res_node->data)
				dpu_pr_warn("init resource node data fail, res_type=0x%llx", res_node->res_type);
		}
	}
}

static void dpu_res_deinit_resouce_list(struct list_head *res_head)
{
	struct dpu_res_resouce_node *_node_ = NULL;
	struct dpu_res_resouce_node *res_node = NULL;

	list_for_each_entry_safe(res_node, _node_, res_head, list_node) {
		if (!res_node)
			continue;

		list_del(&res_node->list_node);
		if (res_node->deinit)
			res_node->deinit(res_node->data);

		kfree(res_node);
		res_node = NULL;
	}
}

static int32_t dpu_res_probe(struct platform_device *pdev)
{
	struct dpu_res *rm_dev = NULL;
	int32_t ret;

	/* 1, read config from dtsi */
	ret = dpu_init_config(pdev);
	if (ret) {
		dpu_pr_err("init dpu config fail");
		return -1;
	}

	rm_dev = devm_kzalloc(&pdev->dev, sizeof(*rm_dev), GFP_KERNEL);
	if (!rm_dev) {
		dpu_pr_err("alloc rm device data fail");
		return -1;
	}
	rm_dev->pdev = pdev;
	dpu_pr_info("rm_dev: 0x%pK", rm_dev);

	dpu_res_init_data(&rm_dev->data);
	dpu_res_init_resource_list(&rm_dev->resource_list, &rm_dev->data);

	/* create chrdev */
	dpu_res_create_chrdev(rm_dev);

	dev_set_drvdata(&pdev->dev, rm_dev);
	g_rm_dev = rm_dev;

	return 0;
}

static int32_t dpu_res_remove(struct platform_device *pdev)
{
	/* TODO: remove device */
	struct dpu_res *rm_dev = NULL;

	rm_dev = platform_get_drvdata(pdev);
	if (!rm_dev)
		return -1;

	dpu_res_deinit_resouce_list(&rm_dev->resource_list);
	dkmd_destroy_chrdev(&rm_dev->rm_chrdev);
	g_rm_dev = NULL;

	return 0;
}

#define DTS_COMP_DISP_RM_NAME "dkmd,dpu_resource"

static const struct of_device_id dpu_res_match_table[] = {
	{
		.compatible = DTS_COMP_DISP_RM_NAME,
		.data = NULL,
	},
	{},
};
MODULE_DEVICE_TABLE(of, dpu_res_match_table);

static struct platform_driver g_dpu_res_driver = {
	.probe = dpu_res_probe,
	.remove = dpu_res_remove,
	.suspend = NULL,
	.resume = NULL,
	.shutdown = NULL,
	.driver = {
		.name = DPU_RES_DEV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(dpu_res_match_table),
	},
};

static int32_t __init dpu_res_register(void)
{
	return platform_driver_register(&g_dpu_res_driver);
}

static void __exit dpu_res_unregister(void)
{
	platform_driver_unregister(&g_dpu_res_driver);
}

module_init(dpu_res_register);
module_exit(dpu_res_unregister);

MODULE_AUTHOR("Graphics Display");
MODULE_DESCRIPTION("Display Resource Manager Driver");
MODULE_LICENSE("GPL");
