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

#include <linux/fb.h>
#include <linux/atomic.h>
#include <linux/delay.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>
#include <dkmd_dpu.h>

#include "dkmd_object.h"
#include "gfxdev_mgr.h"
#include "dpu_fb.h"
#include "dpu_fb_pan_display.h"
#include "dpu_ion_mem.h"
#include "dkmd_comp.h"
#include "res_mgr.h"
#include "cmdlist_interface.h"

static bool dpu_fb_fastboot_display_enabled(struct composer *comp)
{
	dpu_pr_info("fastboot_display_enabled = %d", comp->fastboot_display_enabled);

	if (comp->fastboot_display_enabled && comp->set_fastboot) {
		comp->power_on = comp->set_fastboot(comp);
		comp->fastboot_display_enabled = 0;
	}

	return comp->power_on;
}

static int32_t dpu_fb_blank_sub(struct device_fb *dpu_fb, int32_t blank_mode)
{
	struct composer *comp = dpu_fb->composer;

	dpu_pr_info("%s enter, power_on=%d", comp->base.name, comp->power_on);

	/* TODO: other modules listen to the blank event, FB_EARLY_EVENT_BLANK or FB_EVENT_BLANK
	 * such as, AOD. those event had been sended at fbmem.c
	 */
	down(&comp->blank_sem);

	switch (blank_mode) {
	case FB_BLANK_UNBLANK:
		gfxdev_blank_power_on(comp);
		break;
	case FB_BLANK_VSYNC_SUSPEND:
	case FB_BLANK_HSYNC_SUSPEND:
	case FB_BLANK_NORMAL:
	case FB_BLANK_POWERDOWN:
	default:
		gfxdev_blank_power_off(comp);
		break;
	}
	up(&comp->blank_sem);

	dpu_pr_info("%s exit, power_on=%d", comp->base.name, comp->power_on);

	return 0;
}

static int32_t dpu_fb_create_fence(struct device_fb *dpu_fb, void __user *argp)
{
	int32_t fence_fd = -1;
	struct composer *comp = dpu_fb->composer;

	if (comp->create_fence)
		fence_fd = comp->create_fence(comp);

	if (copy_to_user(argp, &fence_fd, sizeof(fence_fd)) != 0) {
		dpu_pr_err("%s copy fence to user fail, fence_fd=%d", comp->base.name, fence_fd);
		if (fence_fd > 0)
			ksys_close(fence_fd);
		return -1;
	}

	return 0;
}

static int32_t dpu_fb_present(struct device_fb *dpu_fb, const void __user* argp)
{
	int32_t ret = -1;
	struct disp_frame frame;
	struct composer *comp = dpu_fb->composer;

	if (copy_from_user(&frame, argp, sizeof(frame)) != 0) {
		dpu_pr_err("copy from user frame info fail");
		return -1;
	}

	down(&comp->blank_sem);
	if (!comp->power_on) {
		dpu_pr_warn("%s is power off", comp->base.name);
		up(&comp->blank_sem);
		dkmd_cmdlist_release_locked(frame.scene_id, frame.cmdlist_id);
		return -1;
	}
	dpu_pr_debug("%s present info: scene_id=%d cmdlist_id=%u layer_count=%u frame_index=%u",
		comp->base.name, frame.scene_id, frame.cmdlist_id, frame.layer_count, frame.frame_index);

	/* comp_frame_index may differ from frame.frame_index when UT invoke to kernel */
	comp->comp_frame_index++;

	ret = comp->present(comp, (void *)&frame);
	if (unlikely(ret))
		dpu_pr_err("%s present fail: scene_id=%d cmdlist_id=%u layer_count=%u frame_index=%u",
			comp->base.name, frame.scene_id, frame.cmdlist_id, frame.layer_count, frame.frame_index);

	up(&comp->blank_sem);

	return ret;
}

static int32_t dpu_fb_get_product_config(struct device_fb *dpu_fb, void __user *argp)
{
	int32_t ret = 0;
	struct product_config config = {0};
	struct composer *comp = dpu_fb->composer;

	if (!comp->get_product_config) {
		dpu_pr_err("comp get product config function is nullptr");
		return -1;
	}

	ret = comp->get_product_config(comp, &config);
	if (unlikely(ret != 0)) {
		dpu_pr_err("%s get product config fail", comp->base.name);
		return -1;
	}
	config.drv_feature.bits.drv_framework = DRV_FB;
	config.drv_feature.bits.is_pluggable = 0;

	if (copy_to_user(argp, &config, sizeof(config)) != 0) {
		dpu_pr_err("copy product config to user fail");
		return -1;
	}

	return 0;
}

// ============================================================================
static struct fb_fix_var_screeninfo g_fb_fix_var_screeninfo[FB_FOARMT_MAX] = {
	 /* FB_FORMAT_BGRA8888, */
	{ FB_TYPE_PACKED_PIXELS, 1, 1, FB_VMODE_NONINTERLACED, 0, 8, 16, 24, 8, 8, 8, 8, 0, 0, 0, 0, 4 },

	/* FB_FORMAT_RGB565 */
	{FB_TYPE_PACKED_PIXELS, 1, 1, FB_VMODE_NONINTERLACED, 0, 5, 11, 0, 5, 6, 5, 0, 0, 0, 0, 0, 2},

	/* FB_FORMAT_YUV_422_I */
	{FB_TYPE_INTERLEAVED_PLANES, 2, 1, FB_VMODE_NONINTERLACED, 0, 5, 11, 0, 5, 6, 5, 0, 0, 0, 0, 0, 2},
};

static int32_t dpu_fb_open(struct fb_info *info, int32_t user)
{
	int32_t ret = 0;
	struct composer *comp = NULL;
	struct device_fb *dpu_fb = NULL;

	if (unlikely(!info))
		return -ENODEV;

	comp = (struct composer *)info->par;
	if (unlikely(!comp))
		return -EINVAL;

	dpu_fb = (struct device_fb *)comp->device_data;
	if (unlikely(!dpu_fb)) {
		dpu_pr_err("dpu_fb is null");
		return -EINVAL;
	}

	dpu_pr_info("ref_cnt=%d enter", atomic_read(&dpu_fb->ref_cnt));

	/* fb have been opened */
	if (atomic_read(&dpu_fb->ref_cnt) == 0) {
		/* if fastboot display enabled, skip blank
		 * else, need unblank dpu
		 */
		if (!dpu_fb_fastboot_display_enabled(dpu_fb->composer))
			ret = dpu_fb_blank_sub(dpu_fb, FB_BLANK_UNBLANK);
	}
	atomic_inc(&dpu_fb->ref_cnt);

	dpu_pr_info("ref_cnt=%d exit", atomic_read(&dpu_fb->ref_cnt));

	return ret;
}

static int32_t dpu_fb_release(struct fb_info *info, int32_t user)
{
	int32_t ret = 0;
	struct composer *comp = NULL;
	struct device_fb *dpu_fb = NULL;

	if (unlikely(!info))
		return -ENODEV;

	comp = (struct composer *)info->par;
	if (unlikely(!comp))
		return -EINVAL;

	dpu_fb = (struct device_fb *)comp->device_data;
	if (unlikely(!dpu_fb)) {
		dpu_pr_err("dpu_fb is null");
		return -EINVAL;
	}
	dpu_pr_info("ref_cnt=%d enter", atomic_read(&dpu_fb->ref_cnt));

	/* fb have not been opened, return */
	if (atomic_read(&dpu_fb->ref_cnt) == 0) {
		dpu_pr_warn("gfx%u is not opened, cannot release", dpu_fb->index);
		return 0;
	}

	if (atomic_sub_and_test(1, &dpu_fb->ref_cnt)) {
		/* ref_cnt is 0, need blank */
		ret = dpu_fb_blank_sub(dpu_fb, FB_BLANK_POWERDOWN);
		if (ret)
			dpu_pr_err("gfx%u, blank pown down error, ret=%d", dpu_fb->index, ret);
	}
	dpu_pr_info("ref_cnt=%d exit", atomic_read(&dpu_fb->ref_cnt));

	return ret;
}

static ssize_t dpu_fb_read(struct fb_info *info, char __user *buf,
	size_t count, loff_t *ppos)
{
	return 0;
}

static ssize_t dpu_fb_write(struct fb_info *info, const char __user *buf,
	size_t count, loff_t *ppos)
{
	int err;

	if (!info)
		return -ENODEV;

	if (!lock_fb_info(info))
		return -ENODEV;

	if (!info->screen_base) {
		unlock_fb_info(info);
		return -ENODEV;
	}

	err = fb_sys_write(info, buf, count, ppos);

	unlock_fb_info(info);

	return err;
}

static int dpu_fb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	struct composer *comp = NULL;
	struct device_fb *dpu_fb = NULL;

	if (unlikely(!var))
		return -EINVAL;

	if (unlikely(!info))
		return -ENODEV;

	comp = (struct composer *)info->par;
	if (unlikely(!comp))
		return -EINVAL;

	dpu_fb = (struct device_fb *)comp->device_data;
	if (unlikely(!dpu_fb)) {
		dpu_pr_err("dpu_fb is null");
		return -EINVAL;
	}

	if (var->rotate != FB_ROTATE_UR) {
		dpu_pr_err("error rotate %u", var->rotate);
		return -EINVAL;
	}

	if (var->grayscale != info->var.grayscale) {
		dpu_pr_err("error grayscale %u", var->grayscale);
		return -EINVAL;
	}

	if ((var->xres_virtual <= 0) || (var->yres_virtual <= 0)) {
		dpu_pr_err("xres_virtual=%u yres_virtual=%u out of range", var->xres_virtual, var->yres_virtual);
		return -EINVAL;
	}

	if ((var->xres == 0) || (var->yres == 0)) {
		dpu_pr_err("xres=%u, yres=%u is invalid", var->xres, var->yres);
		return -EINVAL;
	}

	if (var->xoffset > (var->xres_virtual - var->xres)) {
		dpu_pr_err("xoffset=%u is invalid, xres_virtual=%u xres=%u", var->xoffset, var->xres_virtual, var->xres);
		return -EINVAL;
	}

	if (var->yoffset > (var->yres_virtual - var->yres)) {
		dpu_pr_err("yoffset=%u is invalid, yres_virtual=%u yres=%u", var->yoffset, var->yres_virtual, var->yres);
		return -EINVAL;
	}

	return 0;
}

static int dpu_fb_set_par(struct fb_info *info)
{
	if (!info)
		return -ENODEV;

	return 0;
}

static int32_t dpu_fb_blank(int32_t blank_mode, struct fb_info *info)
{
	int32_t ret = 0;
	struct composer *comp = NULL;
	struct device_fb *dpu_fb = NULL;

	if (unlikely(!info))
		return -ENODEV;

	comp = (struct composer *)info->par;
	if (unlikely(!comp))
		return -EINVAL;

	/* obtain dpu_fb to process private data if needed */
	dpu_fb = (struct device_fb *)comp->device_data;
	if (unlikely(!dpu_fb)) {
		dpu_pr_err("dpu_fb is null");
		return -EINVAL;
	}

	/* TODO:
	 * other modules listen to the blank event, FB_EARLY_EVENT_BLANK or FB_EVENT_BLANK
	 * such as, AOD. those event had been sended at fbmen.c
	 */
	ret = dpu_fb_blank_sub(dpu_fb, blank_mode);
	if (ret)
		dpu_pr_info("gfx%u blank sub fail", dpu_fb->index);

	return ret;
}

static int32_t dpu_fb_ioctl(struct fb_info *info, uint32_t cmd, unsigned long arg)
{
	int32_t ret = 0;
	struct composer *comp = NULL;
	struct device_fb *dpu_fb = NULL;
	void __user *argp = (void __user *)(uintptr_t)arg;

	if (unlikely(!argp))
		return -EINVAL;

	if (unlikely(!info))
		return -ENODEV;

	comp = (struct composer *)info->par;
	if (unlikely(!comp))
		return -EINVAL;

	dpu_fb = (struct device_fb *)comp->device_data;
	if (unlikely(!dpu_fb)) {
		dpu_pr_err("dpu_fb is null");
		return -EINVAL;
	}

	dpu_pr_debug("gfx%u ioctl, cmd=%#x", dpu_fb->index, cmd);
	switch (cmd) {
	case DISP_CREATE_FENCE:
		return dpu_fb_create_fence(dpu_fb, argp);
	case DISP_PRESENT:
		return dpu_fb_present(dpu_fb, argp);
	case DISP_GET_PRODUCT_CONFIG:
		return dpu_fb_get_product_config(dpu_fb, argp);
	default:
		dpu_pr_err("gfx%u ioctl fail unsupport cmd, cmd=%#x", dpu_fb->index, cmd);
		return -1;
	}

	return ret;
}

static void fb_init_fbi_fix_info(struct device_fb *fb,
	struct fb_fix_var_screeninfo *screen_info, struct fb_fix_screeninfo *fix)
{
	const struct dkmd_object_info *pinfo = fb->pinfo;

	fb->bpp = screen_info->bpp;
	fix->type_aux = 0;
	fix->visual = FB_VISUAL_TRUECOLOR;
	fix->ywrapstep = 0;
	fix->mmio_start = 0;
	fix->mmio_len = 0;
	fix->accel = FB_ACCEL_NONE;

	fix->type = screen_info->fix_type;
	fix->xpanstep = screen_info->fix_xpanstep;
	fix->ypanstep = screen_info->fix_ypanstep;

	snprintf(fix->id, sizeof(fix->id), "dpufb%u", fb->index);

	fix->line_length = roundup(pinfo->xres * (uint32_t)fb->bpp, DMA_STRIDE_ALIGN);
	fix->smem_len = roundup(fix->line_length * pinfo->yres * FB_BUFFER_MAX_COUNT, PAGE_SIZE);
	fix->smem_start = 0;
	fix->reserved[0] = (uint16_t)is_mipi_cmd_panel(fb->pinfo);

	dpu_res_register_screen_info(pinfo->xres, pinfo->yres);
}

static void fb_init_fbi_var_info(struct device_fb *fb,
	struct fb_fix_var_screeninfo *screen_info, struct fb_var_screeninfo *var)
{
	const struct dkmd_object_info *pinfo = fb->pinfo;

	var->xoffset = 0;
	var->yoffset = 0;
	var->grayscale = 0;
	var->nonstd = 0;
	var->activate = FB_ACTIVATE_VBL;
	var->accel_flags = 0;
	var->sync = 0;
	var->rotate = 0;

	var->vmode = screen_info->var_vmode;
	var->blue.offset = screen_info->var_blue_offset;
	var->green.offset = screen_info->var_green_offset;
	var->red.offset = screen_info->var_red_offset;
	var->transp.offset = screen_info->var_transp_offset;
	var->blue.length = screen_info->var_blue_length;
	var->green.length = screen_info->var_green_length;
	var->red.length = screen_info->var_red_length;
	var->transp.length = screen_info->var_transp_length;
	var->blue.msb_right = screen_info->var_blue_msb_right;
	var->green.msb_right = screen_info->var_green_msb_right;
	var->red.msb_right = screen_info->var_red_msb_right;
	var->transp.msb_right = screen_info->var_transp_msb_right;

	var->xres = pinfo->xres;
	var->yres = pinfo->yres;
	var->height = pinfo->height;
	var->width = pinfo->width;
	var->xres_virtual = var->xres;
	var->yres_virtual = var->yres * FB_BUFFER_MAX_COUNT;
	var->bits_per_pixel = fb->bpp * 8;

	var->reserved[0] = pinfo->fps;
}

static struct fb_ops g_fb_ops = {
	.owner = THIS_MODULE,
	.fb_open = dpu_fb_open,
	.fb_release = dpu_fb_release,
	.fb_read = dpu_fb_read,
	.fb_write = dpu_fb_write,
	.fb_cursor = NULL,
	.fb_check_var = dpu_fb_check_var,
	.fb_set_par = dpu_fb_set_par,
	.fb_setcolreg = NULL,
	.fb_blank = dpu_fb_blank,
	.fb_pan_display = dpu_fb_pan_display,
	.fb_fillrect = NULL,
	.fb_copyarea = NULL,
	.fb_imageblit = NULL,
	.fb_sync = NULL,
	.fb_ioctl = dpu_fb_ioctl,
	.fb_compat_ioctl = dpu_fb_ioctl,
	.fb_mmap = dpu_fb_mmap,
};

struct composer *get_comp_from_fb_device(struct device *dev)
{
	struct fb_info *fbi = (struct fb_info *)dev_get_drvdata(dev);

	dpu_check_and_return(!fbi, NULL, err, "fbi is null pointer");

	return (struct composer *)fbi->par;
}

int32_t fb_device_register(struct composer *comp)
{
	struct device_fb *fb = NULL;
	struct fb_info *fbi = NULL;
	struct dkmd_attr *comp_attr = NULL;
	struct dkmd_object_info *pinfo = &comp->base;

	dpu_pr_info("pdev register composer id:%d", comp->index);

	fbi = framebuffer_alloc(sizeof(*fb), NULL);
	if (!fbi) {
		dpu_pr_err("alloc fbi fail");
		return -EINVAL;
	}
	fb = (struct device_fb *)fbi->par;
	fb->fbi_info = fbi;
	fb->index = comp->index;
	fb->composer = comp;
	fb->pinfo = pinfo;
	comp->device_data = fb;

	/* Refresh the pointer to composer */
	fbi->par = comp;
	fbi->fbops = &g_fb_ops;
	fbi->flags = FBINFO_FLAG_DEFAULT;
	fbi->pseudo_palette = NULL;

	fb_init_fbi_fix_info(fb, &g_fb_fix_var_screeninfo[FB_FORMAT_BGRA8888], &fbi->fix);
	fb_init_fbi_var_info(fb, &g_fb_fix_var_screeninfo[FB_FORMAT_BGRA8888], &fbi->var);

	if (register_framebuffer(fbi) < 0) {
		dpu_pr_err("gfx%u failed to register_framebuffer!", fb->index);
		dpu_free_fb_buffer(fbi);
		framebuffer_release(fbi);
		return -EINVAL;
	}

	if (comp->get_sysfs_attrs) {
		comp->get_sysfs_attrs(comp, &comp_attr);
		if (comp_attr)
			dkmd_sysfs_create(fbi->dev, comp_attr);
	}

	dpu_pr_info("Primary FrameBuffer[%u] at scene_id=%d %dx%d size=%d bytes is registered successfully!\n",
		fb->index, pinfo->pipe_sw_itfch_idx, fbi->var.xres, fbi->var.yres, fbi->fix.smem_len);

	return 0;
}

void fb_device_unregister(struct composer *comp)
{
	struct device_fb *fb = NULL;
	struct dkmd_attr *comp_attr = NULL;

	if (!comp) {
		dpu_pr_err("comp is null!");
		return;
	}

	fb = (struct device_fb *)comp->device_data;
	if (!fb) {
		dpu_pr_err("fb is null!");
		return;
	}
	if (comp->get_sysfs_attrs) {
		comp->get_sysfs_attrs(comp, &comp_attr);
		if (comp_attr)
			dkmd_sysfs_remove(fb->fbi_info->dev, comp_attr);
	}
	unregister_framebuffer(fb->fbi_info);
	dpu_free_fb_buffer(fb->fbi_info);
	framebuffer_release(fb->fbi_info);
}

void fb_device_shutdown(struct composer *comp)
{
	struct device_fb *fb = NULL;

	if (!comp) {
		dpu_pr_err("comp is null!");
		return;
	}

	fb = (struct device_fb *)comp->device_data;
	if (!fb) {
		dpu_pr_err("fb is null!");
		return;
	}

	dpu_fb_blank_sub(fb, FB_BLANK_POWERDOWN);
}
