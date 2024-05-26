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

#include "dkmd_log.h"

#include <linux/slab.h>
#include <linux/atomic.h>
#include <linux/delay.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>
#include <dkmd_dpu.h>

#include "gfxdev_mgr.h"
#include "dkmd_object.h"
#include "dkmd_comp.h"
#include "dpu_gfx.h"
#include "cmdlist_interface.h"

struct gfx_devno_info {
	dev_t devno;
	struct device_gfx *gfx_dev;
};

static struct gfx_devno_info g_gfx_devno_info[DEVICE_COMP_MAX_COUNT - 1];
static uint32_t g_gfx_devno_info_index;
static int32_t dpu_gfx_blank(struct device_gfx *gfx_dev, int32_t blank_mode);

static ssize_t dpu_gfx_debug_show(struct device *dev, struct device_attribute *attr, char* buf)
{
	int32_t ret;
	struct composer *comp = NULL;
	struct device_gfx *gfx_dev = NULL;

	dpu_check_and_return((!dev || !buf), -1, err, "input is null pointer");
	dpu_pr_info("dev->kobj.name: %s", dev->kobj.name);
	gfx_dev = (struct device_gfx *)dev_get_drvdata(dev);
	dpu_check_and_return(!gfx_dev, -1, err, "device_gfx is null pointer");
	comp = gfx_dev->composer;

	ret = snprintf(buf, PAGE_SIZE, "%d\n", comp->power_on);
	buf[strlen(buf) + 1] = '\0';

	return ret;
}

static ssize_t dpu_gfx_debug_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int32_t ret;
	int32_t blank_mode;
	struct device_gfx *gfx_dev = NULL;

	dpu_check_and_return((!dev || !buf), -1, err, "input is null pointer");
	gfx_dev = (struct device_gfx *)dev_get_drvdata(dev);
	dpu_check_and_return(!gfx_dev, -1, err, "device_gfx is null pointer");

	ret = sscanf(buf, "%d", &blank_mode);
	if (!ret) {
		dpu_pr_err("get buf (%s) blank_mode fail\n", buf);
		return -1;
	}
	dpu_pr_info("blank_mode=%d", blank_mode);

	(void)dpu_gfx_blank(gfx_dev, blank_mode);

	return count;
}

static DEVICE_ATTR(gfx_debug, S_IRUGO | S_IWUSR, dpu_gfx_debug_show, dpu_gfx_debug_store);

static int32_t dpu_gfx_open(struct inode *inode, struct file *filp)
{
	uint32_t i;
	struct device_gfx *gfx_dev = NULL;

	dpu_check_and_return(unlikely(!inode), -EINVAL, err, "inode is null");
	dpu_check_and_return(unlikely(!filp), -EINVAL, err, "filp is null");

	if (!filp->private_data) {
		for (i = 0; i < DEVICE_COMP_MAX_COUNT - 1; ++i) {
			if (inode->i_rdev == g_gfx_devno_info[i].devno) {
				filp->private_data = g_gfx_devno_info[i].gfx_dev;
				break;
			}
		}
	}
	gfx_dev = (struct device_gfx *)filp->private_data;
	if (gfx_dev != NULL)
		atomic_inc(&gfx_dev->ref_cnt);

	return 0;
}

static int32_t dpu_gfx_release(struct inode *inode, struct file *filp)
{
	struct device_gfx *gfx_dev = NULL;

	dpu_check_and_return(unlikely(!filp), -EINVAL, err, "filp is null");
	dpu_check_and_return(unlikely(!filp->private_data), -EINVAL, err, "private_data is null");

	gfx_dev = (struct device_gfx *)filp->private_data;
	dpu_pr_info("gfx_dev: %pK, ref_cnt=%d", gfx_dev, atomic_read(&gfx_dev->ref_cnt));

	if (unlikely(atomic_read(&gfx_dev->ref_cnt) == 0)) {
		dpu_pr_warn("gfx%u is not opened, cannot release", gfx_dev->index);
		return 0;
	}
	atomic_dec(&gfx_dev->ref_cnt);

	return 0;
}

static int32_t dpu_gfx_present(struct device_gfx *gfx_dev, const void __user* argp)
{
	int32_t ret = -1;
	struct disp_frame frame;
	struct composer *comp = gfx_dev->composer;

	dpu_check_and_return(unlikely(!argp), -EINVAL, err, "argp is null pointer");

	if (copy_from_user(&frame, argp, sizeof(frame)) != 0) {
		dpu_pr_err("copy from user frame info fail");
		return -EINVAL;
	}

	/* if there is no poweron, need to call poweron here, and then call poweroff at task end */
	down(&comp->blank_sem);
	if (!comp->power_on) {
		if (strncmp(gfx_dev->pinfo->name, "gfx_dp", 6) == 0) {
			dpu_pr_warn("%s is power off", comp->base.name);
			up(&comp->blank_sem);
			dkmd_cmdlist_release_locked(frame.scene_id, frame.cmdlist_id);
			return -1;
		}
		comp->on(comp);
	}
	dpu_pr_debug("%s present info: scene_id=%d cmdlist_id=%u layer_count=%u",
		comp->base.name, frame.scene_id, frame.cmdlist_id, frame.layer_count);

	/* comp_frame_index may differ from frame.frame_index when UT invoke to kernel */
	comp->comp_frame_index++;

	ret = comp->present(comp, (void *)&frame);
	if (unlikely(ret))
		dpu_pr_err("%s present fail", comp->base.name);

	up(&comp->blank_sem);

	return ret;
}

static int32_t dpu_gfx_create_fence(struct device_gfx *gfx_dev, void __user *argp)
{
	int32_t fence_fd = -1;
	struct composer *comp = gfx_dev->composer;

	dpu_check_and_return(unlikely(!argp), -EINVAL, err, "argp is null pointer");

	/* // NOTICE: return -1 by now
	 *	if (comp->create_fence)
	 *		fence_fd = comp->create_fence(comp);
	 */
	if (copy_to_user(argp, &fence_fd, sizeof(fence_fd)) != 0) {
		dpu_pr_err("%s copy fence to user fail, fence_fd=%d", comp->base.name, fence_fd);
		if (fence_fd > 0)
			ksys_close(fence_fd);
		return -1;
	}

	return 0;
}

static int32_t dpu_gfx_get_product_config(struct device_gfx *gfx_dev, void __user *argp)
{
	int32_t ret = 0;
	struct product_config config = {0};
	struct composer *comp = gfx_dev->composer;

	dpu_check_and_return(unlikely(!argp), -EINVAL, err, "argp is null pointer");

	config.drv_feature.bits.drv_framework = DRV_CHR;
	if (!comp->get_product_config) {
		dpu_pr_info("comp get product config function is nullptr");
		return 0;
	}

	ret = comp->get_product_config(comp, &config);
	if (unlikely(ret != 0)) {
		dpu_pr_err("%s get product config fail", comp->base.name);
		return -1;
	}

	if (strncmp(gfx_dev->pinfo->name, "gfx_dp", 6) == 0) {
		config.dim_info_count = 1;
		config.dim_info[0].width = (int32_t)(comp->base.xres);
		config.dim_info[0].height = (int32_t)(comp->base.yres);
		config.fps_info_count = 1;
		config.fps_info[0] = comp->base.fps;
		config.drv_feature.bits.is_pluggable = 1; // hotpluggable

		dpu_pr_info("comp get xres[%d] yres[%d] fps[%d]",
			comp->base.xres, comp->base.yres, comp->base.fps);
	}

	if (copy_to_user(argp, &config, sizeof(config)) != 0) {
		dpu_pr_err("copy product config to user fail");
		return -1;
	}
	device_mgr_primary_frame_refresh(comp, "dptx_connect_in");

	return 0;
}

static int32_t dpu_gfx_blank(struct device_gfx *gfx_dev, int32_t blank_mode)
{
	int32_t ret = 0;
	struct composer *comp = gfx_dev->composer;

	dpu_pr_info("blank_mode=%d", blank_mode);
	down(&comp->blank_sem);

	if (blank_mode == 1) {
		gfxdev_blank_power_on(comp);
	} else if (blank_mode == 0) {
		gfxdev_blank_power_off(comp);
	} else {
		dpu_pr_err("error power mode=%d", blank_mode);
	}

	up(&comp->blank_sem);

	return ret;
}

static long dpu_gfx_ioctl(struct file *filp, uint32_t cmd, unsigned long arg)
{
	void __user *argp = (void __user *)(uintptr_t)arg;
	struct device_gfx *gfx_dev = NULL;

	dpu_check_and_return(unlikely(!filp), -EINVAL, err, "filp is null");
	dpu_check_and_return(unlikely(!filp->private_data), -EINVAL, err, "private_data is null");

	gfx_dev = (struct device_gfx *)filp->private_data;
	dpu_pr_debug("gfx%u ioctl, cmd=%#x", gfx_dev->index, cmd);

	switch (cmd) {
	case DISP_IOBLANK:
		return dpu_gfx_blank(gfx_dev, (int32_t)arg);
	case DISP_PRESENT:
		return dpu_gfx_present(gfx_dev, argp);
	case DISP_CREATE_FENCE:
		return dpu_gfx_create_fence(gfx_dev, argp);
	case DISP_GET_PRODUCT_CONFIG:
		return dpu_gfx_get_product_config(gfx_dev, argp);
	default:
		dpu_pr_info("unsupported cmd=%#x", cmd);
		return -EINVAL;
	}

	return 0;
}

static struct file_operations dpu_gfx_fops = {
	.owner = THIS_MODULE,
	.open = dpu_gfx_open,
	.release = dpu_gfx_release,
	.unlocked_ioctl = dpu_gfx_ioctl,
	.compat_ioctl =  dpu_gfx_ioctl,
};

struct composer *get_comp_from_gfx_device(struct device *dev)
{
	struct device_gfx *gfx = (struct device_gfx *)dev_get_drvdata(dev);

	dpu_check_and_return(!gfx, NULL, err, "device_gfx is null pointer");

	return gfx->composer;
}

int32_t gfx_device_register(struct composer *comp)
{
	struct device_gfx *gfx = NULL;
	struct dkmd_attr *comp_attr = NULL;
	struct dkmd_object_info *pinfo = &comp->base;

	if (g_gfx_devno_info_index >= DEVICE_COMP_MAX_COUNT) {
		dpu_pr_err("g_gfx_devno_info_index=%u exceed max %d", g_gfx_devno_info_index, DEVICE_COMP_MAX_COUNT);
		return -EINVAL;
	}

	gfx = (struct device_gfx *)kzalloc(sizeof(*gfx), GFP_KERNEL);
	if (unlikely(!gfx)) {
		dpu_pr_err("alloc gfx device failed");
		return -EINVAL;
	}
	dpu_pr_info("%s register device: %s", comp->base.name, pinfo->name);

	gfx->index = comp->index;
	gfx->composer = comp;
	gfx->pinfo = pinfo;

	/* init chrdev info */
	gfx->chrdev.name = pinfo->name;
	gfx->chrdev.fops = &dpu_gfx_fops;
	gfx->chrdev.drv_data = gfx;
	if (unlikely(dkmd_create_chrdev(&gfx->chrdev) != 0)) {
		dpu_pr_err("create chr device failed");
		kfree(gfx);
		return -EINVAL;
	}

	if (comp->get_sysfs_attrs) {
		comp->get_sysfs_attrs(comp, &comp_attr);
		if (comp_attr) {
			dkmd_sysfs_attrs_append(comp_attr, &dev_attr_gfx_debug.attr);
			dkmd_sysfs_create(gfx->chrdev.chr_dev, comp_attr);
		}
	}

	comp->device_data = gfx;
	g_gfx_devno_info[g_gfx_devno_info_index].devno = gfx->chrdev.devno;
	g_gfx_devno_info[g_gfx_devno_info_index++].gfx_dev = gfx;

	return 0;
}

void gfx_device_unregister(struct composer *comp)
{
	struct device_gfx *gfx = NULL;
	struct dkmd_attr *comp_attr = NULL;

	if (unlikely(!comp))
		return;

	gfx = (struct device_gfx *)comp->device_data;
	if (unlikely(!gfx))
		return;

	if (comp->get_sysfs_attrs) {
		comp->get_sysfs_attrs(comp, &comp_attr);
		if (comp_attr)
			dkmd_sysfs_remove(gfx->chrdev.chr_dev, comp_attr);
	}
	dkmd_destroy_chrdev(&gfx->chrdev);

	kfree(gfx);
	comp->device_data = NULL;
}
