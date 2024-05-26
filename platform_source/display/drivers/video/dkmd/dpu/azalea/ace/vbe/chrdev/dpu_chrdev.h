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

#ifndef DPU_CHRDEV_H
#define DPU_CHRDEV_H

#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>

#define FB_TYPE_PACKED_PIXELS 0
#define FB_VMODE_NONINTERLACED 0
#define FB_TYPE_INTERLEAVED_PLANES 2
#define FBIOBLANK 0x4611
#define FBINFO_STATE_RUNNING 0
#define FBINFO_STATE_SUSPENDED 1

enum {
	FB_BLANK_UNBLANK,
	FB_BLANK_NORMAL,
	FB_BLANK_VSYNC_SUSPEND,
	FB_BLANK_HSYNC_SUSPEND,
	FB_BLANK_POWERDOWN
};

struct fb_info;
struct device;
struct vm_area_struct;

struct fb_ops {
	struct module *owner;

	int (*fb_open)(struct fb_info *info);

	int (*fb_release)(struct fb_info *info);

	/* blank display */
	int (*fb_blank)(int blank, struct fb_info *info);

	/* perform fb specific ioctl (optional) */
	int (*fb_ioctl)(struct fb_info *info, unsigned int cmd, unsigned long arg);

	/* Handle 32bit compat ioctl (optional) */
	int (*fb_compat_ioctl)(struct fb_info *info, unsigned cmd, unsigned long arg);

	/* perform fb specific mmap */
	int (*fb_mmap)(struct fb_info *info, struct vm_area_struct *vma);
};

struct fb_info {
	int node;
	struct mutex lock; /* Lock for open/release/ioctl funcs */
	struct fb_ops *fbops;
	struct device *dev; /* This is this fb device */
	void *par;
};

struct fb_info *framebuffer_alloc(size_t size);
void framebuffer_release(struct fb_info *info);
int register_framebuffer(struct fb_info *fb_info);
int unregister_framebuffer(struct fb_info *fb_info);
int dpu_fb_create_cdev(void);
struct class *get_fb_class(void);

#endif /* DPU_CHRDEV_H */
