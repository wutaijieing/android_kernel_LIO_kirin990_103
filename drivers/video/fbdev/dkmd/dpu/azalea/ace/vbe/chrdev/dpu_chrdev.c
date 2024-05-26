/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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
#include <linux/fs.h>
#include <linux/semaphore.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/console.h>
#include <linux/uaccess.h>
#include "dpu_fb_def.h"
#include "chrdev/dpu_chrdev.h"

#define FB_MAJOR 29
#define FB_MAX 3

static struct semaphore fb_lock;
static struct class *fb_class = NULL;
static struct fb_info *registered_fb[FB_MAX];
static int num_registered_fb;

struct class *get_fb_class(void)
{
	return fb_class;
}

static struct fb_info *get_fb_info(unsigned int idx)
{
	struct fb_info *fb_info = NULL;

	if (idx >= FB_MAX)
		return ERR_PTR(-ENODEV);

	down(&fb_lock);
	fb_info = registered_fb[idx];
	up(&fb_lock);

	return fb_info;
}

static int lock_fb_info(struct fb_info *info)
{
	mutex_lock(&info->lock);
	if (!info->fbops) {
		mutex_unlock(&info->lock);
		return 0;
	}
	return 1;
}

static void unlock_fb_info(struct fb_info *info)
{
	mutex_unlock(&info->lock);
}

struct fb_info *framebuffer_alloc(size_t size)
{
	unsigned int fb_info_size = sizeof(struct fb_info);
	struct fb_info *info = NULL;
	char *p = NULL;

	DPU_FB_INFO("+\n");
	p = kzalloc(fb_info_size + size, GFP_KERNEL);
	if (!p)
		return NULL;
	info = (struct fb_info *)p;
	if (size)
		info->par = p + fb_info_size;

	DPU_FB_INFO("-\n");
	return info;
}

void framebuffer_release(struct fb_info *info)
{
	DPU_FB_INFO("+\n");
	if (!info)
		return;
	kfree(info);
	DPU_FB_INFO("-\n");
}

int register_framebuffer(struct fb_info *fb_info)
{
	DPU_FB_DEBUG("+\n");
	mutex_init(&fb_info->lock);

	down(&fb_lock);

	if (num_registered_fb >= FB_MAX) {
		DPU_FB_ERR("exceed max fb support[%d]\n", num_registered_fb);
		up(&fb_lock);
		return -1;
	}

	fb_info->node = num_registered_fb;
	fb_info->dev = device_create(fb_class, NULL, MKDEV(FB_MAJOR, (unsigned int)num_registered_fb),
			fb_info, "fb%d", num_registered_fb);
	if (IS_ERR(fb_info->dev)) {
		DPU_FB_ERR(KERN_WARNING "Unable to create device for framebuffer %d; errno = %ld\n",
			num_registered_fb, PTR_ERR(fb_info->dev));
		fb_info->dev = NULL;
		up(&fb_lock);
		return -1;
	}

	registered_fb[num_registered_fb++] = fb_info;
	up(&fb_lock);

	DPU_FB_DEBUG("-\n");
	return 0;
}

int unregister_framebuffer(struct fb_info *fb_info)
{
	DPU_FB_DEBUG("+\n");

	down(&fb_lock);
	registered_fb[fb_info->node] = NULL;
	num_registered_fb--;
	up(&fb_lock);

	DPU_FB_DEBUG("-\n");
	return 0;
}

static int fb_blank(struct fb_info *info, unsigned long blank)
{
	int ret = -EINVAL;

	DPU_FB_DEBUG("+\n");
	dpu_check_and_return((blank > FB_BLANK_POWERDOWN), -1, ERR, "wrong blank mode!\n");

	if (info->fbops->fb_blank)
		ret = info->fbops->fb_blank(blank, info);

	DPU_FB_DEBUG("-\n");
	return ret;
}

static struct fb_info *file_fb_info(struct file *file)
{
	struct inode *inode = file_inode(file);
	unsigned int fbidx = iminor(inode);
	struct fb_info *info = registered_fb[fbidx];

	if (info != file->private_data)
		info = NULL;
	return info;
}

static long fb_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long ret = 0;
	struct fb_info *info = file_fb_info(file);
	dpu_check_and_return(!info, -ENODEV, ERR, "info is null\n");

	switch (cmd) {
	case FBIOBLANK:
		if (lock_fb_info(info) == 0)
			return -ENODEV;
		ret = fb_blank(info, arg);
		unlock_fb_info(info);
		break;
	default:
		if (info->fbops->fb_ioctl)
			ret = info->fbops->fb_ioctl(info, cmd, arg);
		else
			ret = -ENOTTY;
	}

	return ret;
}

static int fb_open(struct inode *inode, struct file *file)
{
	int fbidx = iminor(inode);
	struct fb_info *info = NULL;
	int ret = 0;

	DPU_FB_DEBUG("+\n");
	info = get_fb_info(fbidx);
	if (!info) {
		DPU_FB_ERR("fb info is null\n");
		return -ENODEV;
	}

	mutex_lock(&info->lock);
	file->private_data = info;
	if (info->fbops->fb_open)
		info->fbops->fb_open(info);
	mutex_unlock(&info->lock);

	DPU_FB_DEBUG("-\n");
	return ret;
}

static int fb_release(struct inode *inode, struct file *file)
{
	struct fb_info *info = file->private_data;
	void_unused(inode);

	mutex_lock(&info->lock);
	if (info->fbops->fb_release)
		info->fbops->fb_release(info);
	module_put(info->fbops->owner);
	mutex_unlock(&info->lock);

	return 0;
}

static const struct file_operations fb_fops = {
	.owner = THIS_MODULE,
	.read = NULL,
	.write = NULL,
	.unlocked_ioctl = fb_ioctl,
	.compat_ioctl = fb_ioctl,
	.mmap = NULL,
	.open = fb_open,
	.release = fb_release,
};

int dpu_fb_create_cdev(void)
{
	int ret;
	static bool dev_init = false;
	DPU_FB_DEBUG("+\n");

	if (dev_init) {
		DPU_FB_INFO("cdev has inited\n");
		return 0;
	}
	ret = register_chrdev(FB_MAJOR, "fb", &fb_fops);
	if (ret) {
		DPU_FB_ERR("unable to get major %d for fb devs\n", FB_MAJOR);
		return -1;
	}

	fb_class = class_create(THIS_MODULE, "graphics");
	if (IS_ERR(fb_class)) {
		ret = PTR_ERR(fb_class);
		DPU_FB_ERR("Unable to create fb class; errno = %d\n", ret);
		fb_class = NULL;
		unregister_chrdev(FB_MAJOR, "fb");
	}

	sema_init(&fb_lock, 1);
	dev_init = true;

	DPU_FB_DEBUG("+\n");
	return ret;
}
