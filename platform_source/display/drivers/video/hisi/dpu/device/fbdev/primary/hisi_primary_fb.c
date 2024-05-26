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
#include <linux/module.h>

#include "hisi_fb.h"
#include "hisi_disp.h"
#include "primary/hisi_primary_fb_base.h"
#include "hisi_fb_pm.h"
#include "hisi_drv_test.h"
#include "hisi_comp_utils.h"
#ifdef CONFIG_DKMD_OLD_ARCH
#include "adaptor/hisi_fb_adaptor.h"
#endif
#include "hisi_drv_kthread.h"
#include "hisi_disp_pm.h"
#include "hisi_composer_core.h"
#include "hisi_disp_config.h"
#include "hisi_disp_iommu.h"
#include "hisi_fb_sysfs.h"

#include "dpu_fpga_debug.h"

struct hisi_fb_info *hfb_info_list[HISI_FB_MAX_FBI_LIST] = {0};
static int hfb_info_list_index;

hisi_fb_fix_var_screeninfo_t g_hisi_fb_fix_var_screeninfo[HISI_FB_PIXEL_FORMAT_MAX] = {
	{0}, {0}, {0}, {0}, {0}, {0}, {0},
	/* for HISI_FB_PIXEL_FORMAT_BGR_565 */
	{FB_TYPE_PACKED_PIXELS, 1, 1, FB_VMODE_NONINTERLACED, 0, 5, 11, 0, 5, 6, 5, 0, 0, 0, 0, 0, 2},
	{FB_TYPE_PACKED_PIXELS, 1, 1, FB_VMODE_NONINTERLACED, 0, 4, 8, 0, 4, 4, 4, 0, 0, 0, 0, 0, 2},
	{FB_TYPE_PACKED_PIXELS, 1, 1, FB_VMODE_NONINTERLACED, 0, 4, 8, 12, 4, 4, 4, 4, 0, 0, 0, 0, 2},
	{FB_TYPE_PACKED_PIXELS, 1, 1, FB_VMODE_NONINTERLACED, 0, 5, 10, 0, 5, 5, 5, 0, 0, 0, 0, 0, 2},
	{FB_TYPE_PACKED_PIXELS, 1, 1, FB_VMODE_NONINTERLACED, 0, 5, 10, 15, 5, 5, 5, 1, 0, 0, 0, 0, 2},
	{0},
	/* HISI_FB_PIXEL_FORMAT_BGRA_8888 */
	{FB_TYPE_PACKED_PIXELS, 1, 1, FB_VMODE_NONINTERLACED, 0, 8, 16, 24, 8, 8, 8, 8, 0, 0, 0, 0, 4},
	{FB_TYPE_INTERLEAVED_PLANES, 2, 1, FB_VMODE_NONINTERLACED, 0, 5, 11, 0, 5, 6, 5, 0, 0, 0, 0, 0, 2},
	{0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0},
	{0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}
};

static hisi_register_cb g_fb_register_priv_ops_tbl[ONLINE_OVERLAY_ID_MAX] = {
	hisi_fb_base_register_private_ops,
	NULL,
	hisi_external_fb_base_register_private_ops,
	hisi_external_fb_base_register_private_ops,
};

static int hisi_dss_check_existence(void)
{
	if (hisi_config_get_fpga_flag() == 1) {
		unsigned int v6_v7_tag = hisi_config_get_v7_tag();
		if (v6_v7_tag == 1)
			return -1; /* V7 logic has no DSS onboard */
		else
			return 0; /* V6 logic has DSS onboard */
	} else {
		/* EMU and ASIC default have DSS onboard */
		return 0;
	}
}

static bool hisi_fb_build_frame(struct hisi_present_data *present_data, void __user *argp)
{
	disp_pr_info(" ++++ ");
	return true;
}

static int hisi_fb_present_kthread_handle(void *data)
{
	struct hisi_present_data *present_data = (struct hisi_present_data *)data;
	struct hisi_fb_priv_ops *fb_ops = NULL;
	struct hisi_fb_info *hfb_info = NULL;
	int32_t ret;

	hfb_info = present_data->hfb_info;
	fb_ops = &hfb_info->fb_priv_ops;

	if (!fb_ops->base.present)
		return 0;

	down(&present_data->frames_sem);
	ret = fb_ops->base.present(hfb_info->id, hfb_info->pdev, hisi_fb_get_using_frame(present_data));
	up(&present_data->frames_sem);

	return ret;
}

static int hisi_fb_blank(int blank_mode, struct fb_info *info)
{
	struct hisi_fb_info *hfb_info = NULL;
	int ret;

	hfb_info = (struct hisi_fb_info *)info->par;

	/* TODO:
	 * other modules listen to the blank event, FB_EARLY_EVENT_BLANK or FB_EVENT_BLANK
	 * such as, AOD. those event had been sended at fbmen.c
	 */
	ret = hisi_fb_pm_blank(blank_mode, hfb_info);
	if (ret) {
		disp_pr_info("blank sub fail");
		return ret;
	}

	return 0;
}

static int hisi_fb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
	disp_pr_info(" ++++ ");
	return 0;
}

static int hisi_fb_online_play(struct hisi_fb_info *hfb_info, void __user *argp)
{
	struct hisi_fb_priv_ops *fb_ops = NULL;
	struct hisi_present_data *present_data = NULL;
	int ret;
	disp_pr_info(" ++++ ");
	fb_ops = &hfb_info->fb_priv_ops;
	present_data = &hfb_info->present_data;

	if (!hisi_fb_build_frame(present_data, argp))
		return -EINVAL;

	if (!fb_ops->base.present)
		return 0;

	ret = fb_ops->base.present(hfb_info->id, hfb_info->pdev, hisi_fb_get_using_frame(present_data));
	if (ret)
		return -1;

	/* TODO: get release and retire fence */
	// hisi_fence_get_fence();
	hisi_fb_switch_frame_index(present_data);

	return 0;
}

static int hisi_fb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
	struct hisi_fb_info *hfb_info = NULL;
	void __user *argp = (void __user *)(uintptr_t)arg;
	int ret = 0;

	hfb_info = (struct hisi_fb_info *)info->par;

	disp_pr_info("enter ++++");
	switch (cmd) {
	case DISP_IOCTL_CMD_ONLINE_PLAY:
		ret = hisi_fb_online_play(hfb_info, argp); // ybr
		break;
	default:
		return 0;
	}

	disp_pr_info("exit ----");

	return ret;
}

static void init_isr(struct hisi_fb_info *hfb_info)
{
	struct platform_device *dev = hfb_info->pdev;
	struct hisi_fb_info *info = platform_get_drvdata(dev);
	struct platform_device *dev2 = info->next_pdev;
	struct hisi_composer_device *overlay_dev =platform_get_drvdata(dev2);
	struct hisi_composer_data *ov_data = &overlay_dev->ov_data;

	disp_pr_info(" dev1:%s ", dev->name);
	disp_pr_info(" dev2:%s ", dev2->name);

	hisi_online_init(ov_data);
}

int hisi_fb_open(struct fb_info *info, int user)
{
	struct hisi_fb_info *hfb_info = NULL;
	struct hisi_fb_priv_ops *fb_ops = NULL;
	int ret = 0;
	bool skip_blank = false;

	hfb_info = (struct hisi_fb_info *)info->par;
	fb_ops = &hfb_info->fb_priv_ops;

	disp_pr_info(" ++++ ");
	disp_pr_info(" open fb%d, ov%d\n", hfb_info->index, hfb_info->id);

	if (!dss_is_system_on()) {
		disp_pr_info("dpu system had been manually powered down\n");
		return -EINVAL;
	}

	/* if fb panel is disconnected, open fail */
	if (!hfb_info->be_connected) {
		disp_pr_info("fb panel is disconnected, id=%d", hfb_info->id);
		return 0;
	}

	/* fb have been opened */
	if (atomic_read(&hfb_info->ref_cnt) > 0) {
		disp_pr_info("hfb_info[%d] ref count %d\n", hfb_info->id, hfb_info->ref_cnt);
		return 0;
	}

	dpu_check_and_call_func(ret, fb_ops->open.init_cmdlist_cb, hfb_info);
	if (ret) {
		disp_pr_err("init cmdlist fail, ret=%d", ret);
		return -EINVAL;
	}

	dpu_check_and_call_func(skip_blank, fb_ops->open.set_fastboot_cb, info);
	if (!skip_blank && hfb_info == ONLINE_OVERLAY_ID_PRIMARY) {
		// for cases involved external, it should be powered up by debug-command-line
		disp_pr_info("fb pm blank for %d\n", hfb_info->id);
		ret = hisi_fb_pm_blank(FB_BLANK_UNBLANK, hfb_info);
		// init_isr(hfb_info);
	}

	atomic_inc(&hfb_info->ref_cnt);

	disp_pr_info("----");

	return 0;
}

int hisi_fb_release(struct fb_info *info, int user)
{
	struct hisi_fb_info *hfb_info = NULL;
	struct hisi_fb_priv_ops *fb_ops = NULL;
	int ret = 0;
	disp_pr_info(" ++++ ");

	hfb_info = (struct hisi_fb_info *)info->par;
	fb_ops = &hfb_info->fb_priv_ops;

	disp_pr_info(" release fb%d, ov%d \n", hfb_info->index, hfb_info->id);
	if (hfb_info->id == 0)
		return 0;

	/* if fb panel is disconnected, open fail */
	if (!hfb_info->be_connected) {
		disp_pr_info("fb panel is disconnected, id=%d", hfb_info->id);
		return 0;
	}

	/* fb have not been opened, return */
	if (atomic_read(&hfb_info->ref_cnt) == 0)
		return 0;

	/* ref_cnt is not 0 */
	if (!atomic_sub_and_test(1, &hfb_info->ref_cnt))
		return 0;

	disp_pr_info("----");

	return 0;

	ret = hisi_fb_pm_blank(FB_BLANK_POWERDOWN, hfb_info);
	if (ret) {
		disp_pr_err("blank pown down error, ret=%d", ret);
	}

	dpu_check_and_call_func(ret, fb_ops->open.deinit_cmdlist_cb, hfb_info);

	return ret;
}

static void hisi_fb_read_dtsi(struct hisi_fb_info *fb_info)
{
	/* TODO: read enable async from dtsi */
	fb_info->enable_present_async = false;
}

static struct fb_ops hisi_fb_ops = {
	.owner = THIS_MODULE,
	.fb_open = hisi_fb_open,
	.fb_release = hisi_fb_release,
	.fb_read = NULL,
	.fb_write = NULL,
	.fb_cursor = NULL,
	.fb_check_var = NULL,
	.fb_set_par = NULL,
	.fb_setcolreg = NULL,
	.fb_blank = hisi_fb_blank,
	.fb_pan_display = hisi_fb_pan_display,
	.fb_fillrect = NULL,
	.fb_copyarea = NULL,
	.fb_imageblit = NULL,
	.fb_sync = NULL,
#ifdef CONFIG_DKMD_OLD_ARCH
	.fb_ioctl = hisi_fb_adaptor_ioctl,
	.fb_compat_ioctl = hisi_fb_adaptor_ioctl,
#else
	.fb_ioctl = hisi_fb_ioctl,
	.fb_compat_ioctl = hisi_fb_ioctl,
#endif
	.fb_mmap = NULL,
};

int dpufb_esd_recover_disable(int value)
{
	//TODO
	disp_pr_info("++++");
	return 0;
}
EXPORT_SYMBOL(dpufb_esd_recover_disable);

static void hisi_fb_init_screeninfo_base(struct fb_fix_screeninfo *fix, struct fb_var_screeninfo *var)
{
	disp_check_and_no_retval((!fix || !var), err, "fix or var is NULL\n");

	fix->type_aux = 0;
	fix->visual = FB_VISUAL_TRUECOLOR;
	fix->ywrapstep = 0;
	fix->mmio_start = 0;
	fix->mmio_len = 0;
	fix->accel = FB_ACCEL_NONE;

	var->xoffset = 0;
	var->yoffset = 0;
	var->grayscale = 0;
	var->nonstd = 0;
	var->activate = FB_ACTIVATE_VBL;
	var->accel_flags = 0;
	var->sync = 0;
	var->rotate = 0;
}

static void hisi_fb_init_sreeninfo_by_img_type(struct fb_fix_screeninfo *fix, struct fb_var_screeninfo *var,
	uint32_t fb_img_type, int *bpp)
{
	disp_check_and_no_retval((!fix || !var), err, "fix or var is NULL\n");
	disp_pr_info("++++");
	fix->type = g_hisi_fb_fix_var_screeninfo[fb_img_type].fix_type;
	fix->xpanstep = g_hisi_fb_fix_var_screeninfo[fb_img_type].fix_xpanstep;
	fix->ypanstep = g_hisi_fb_fix_var_screeninfo[fb_img_type].fix_ypanstep;
	var->vmode = g_hisi_fb_fix_var_screeninfo[fb_img_type].var_vmode;

	var->blue.offset = g_hisi_fb_fix_var_screeninfo[fb_img_type].var_blue_offset;
	var->green.offset = g_hisi_fb_fix_var_screeninfo[fb_img_type].var_green_offset;
	var->red.offset = g_hisi_fb_fix_var_screeninfo[fb_img_type].var_red_offset;
	var->transp.offset = g_hisi_fb_fix_var_screeninfo[fb_img_type].var_transp_offset;

	var->blue.length = g_hisi_fb_fix_var_screeninfo[fb_img_type].var_blue_length;
	var->green.length = g_hisi_fb_fix_var_screeninfo[fb_img_type].var_green_length;
	var->red.length = g_hisi_fb_fix_var_screeninfo[fb_img_type].var_red_length;
	var->transp.length = g_hisi_fb_fix_var_screeninfo[fb_img_type].var_transp_length;

	var->blue.msb_right = g_hisi_fb_fix_var_screeninfo[fb_img_type].var_blue_msb_right;
	var->green.msb_right = g_hisi_fb_fix_var_screeninfo[fb_img_type].var_green_msb_right;
	var->red.msb_right = g_hisi_fb_fix_var_screeninfo[fb_img_type].var_red_msb_right;
	var->transp.msb_right = g_hisi_fb_fix_var_screeninfo[fb_img_type].var_transp_msb_right;
	*bpp = g_hisi_fb_fix_var_screeninfo[fb_img_type].bpp;
	disp_pr_info("----");
}

static void hisi_fb_init_sreeninfo_by_panel_info(struct fb_var_screeninfo *var, struct hisi_panel_info *panel_info,
	uint32_t fb_num, int bpp)
{
	disp_check_and_no_retval((!var || !panel_info), err, "var or panel_info is NULL\n");
	disp_pr_info("++++");

	var->xres = panel_info->xres;
	var->yres = panel_info->yres;

	var->xres_virtual = var->xres;
	var->yres_virtual = var->yres * fb_num;
	var->bits_per_pixel = bpp * 8;

	disp_pr_info("wxh:%dx%d", panel_info->xres, panel_info->yres);
	disp_pr_info("hbp:%d, hfp:%d, hpw:%d", panel_info->ldi.h_back_porch, panel_info->ldi.h_front_porch, panel_info->ldi.h_pulse_width);
	disp_pr_info("vbp:%d, vfp:%d, vpw:%d", panel_info->ldi.v_back_porch, panel_info->ldi.v_front_porch, panel_info->ldi.v_pulse_width);

	var->pixclock = panel_info->timing_info.pxl_clk_rate;
	var->left_margin = panel_info->ldi.h_back_porch;
	var->right_margin = panel_info->ldi.h_front_porch;
	var->upper_margin = panel_info->ldi.v_back_porch;
	var->lower_margin = panel_info->ldi.v_front_porch;
	var->hsync_len = panel_info->ldi.h_pulse_width;
	var->vsync_len = panel_info->ldi.v_pulse_width;
	var->height = panel_info->height;
	var->width = panel_info->width;

	disp_pr_info("----");
}

static void hisi_fbi_data_init(struct fb_info *fbi)
{
	fbi->screen_base = 0;
	//fbi->fbops = &hisi_fb_ops;
	fbi->flags = FBINFO_FLAG_DEFAULT;
	fbi->pseudo_palette = NULL;
}

static uint32_t get_initial_fps(struct hisi_panel_info *panel_info, struct fb_var_screeninfo *var)
{
	uint64_t lane_byte_clock;
	uint32_t hsize;
	uint32_t vsize;
	uint32_t fps;

	disp_pr_info("dsi_timing_support:%d", panel_info->mipi.dsi_timing_support);
	if (panel_info->mipi.dsi_timing_support) {
		lane_byte_clock = (panel_info->mipi.phy_mode == 0/* DPHY_MODE */) ?
				(uint64_t)(panel_info->mipi.dsi_bit_clk * 2 / 8) : /* lane byte clock */
				(uint64_t)(panel_info->mipi.dsi_bit_clk / 7);
		lane_byte_clock = lane_byte_clock * 1000000UL;  /* 1MHz */
		hsize = panel_info->mipi.hline_time;
		vsize = panel_info->yres + panel_info->mipi.vsa +
			panel_info->mipi.vfp + panel_info->mipi.vbp;

		fps = (uint32_t)(lane_byte_clock / hsize / vsize);
	} else {
		hsize = var->upper_margin + var->lower_margin + var->vsync_len + panel_info->yres;
		vsize = var->left_margin + var->right_margin + var->hsync_len + panel_info->xres;
		fps = (uint32_t)(var->pixclock / hsize / vsize);
	}
	disp_pr_info("fps:%d fps", fps);
	return fps;
}

static uint32_t hisifb_line_length(uint32_t xres, int bpp)
{
	return ALIGN_UP(xres * (uint32_t)bpp, DMA_STRIDE_ALIGN);
}

static void hisi_fb_create_sysfs_attrs(struct hisi_fb_info *hfb_info)
{
	disp_pr_info("++++");

	hisifb_sysfs_init(hfb_info);

	if (hfb_info->fb_priv_ops.vsync_ops.vsync_register != NULL)
		hfb_info->fb_priv_ops.vsync_ops.vsync_register(hfb_info->pdev);

	if (hfb_info->fb_priv_ops.sysfs_ops.sysfs_create_fnc != NULL)
		hfb_info->fb_priv_ops.sysfs_ops.sysfs_create_fnc(hfb_info->pdev);

	disp_pr_info("----");
}

static void hisi_fb_init_fbi(struct hisi_fb_info *hfb_info)
{
	disp_pr_info("++++");
	disp_pr_info("xres:%d, yres:%d", hfb_info->fix_display_info.fix_panel_info->xres, hfb_info->fix_display_info.fix_panel_info->yres);

	struct hisi_display_info *disp_info = &(hfb_info_list[PRIMARY_PANEL_IDX]->fix_display_info);
	struct fb_info *fbi_info = hfb_info->fbi_info;
	fbi_info->fbops = &hisi_fb_ops;

	int bpp = 0;
	struct fb_fix_screeninfo *fix = &fbi_info->fix;
	struct fb_var_screeninfo *var = &fbi_info->var;
	struct hisi_panel_info *panel_info = disp_info->fix_panel_info;

	hisi_fb_init_screeninfo_base(fix, var);

	uint32_t fb_imgType = HISI_FB_PIXEL_FORMAT_BGRA_8888;
	hisi_fb_init_sreeninfo_by_img_type(fix, var, fb_imgType, &bpp);

	uint32_t fb_num = 3;
	hisi_fb_init_sreeninfo_by_panel_info(var, panel_info, fb_num, bpp);

	if (hfb_info->index == PRIMARY_PANEL_IDX) {
		panel_info->fps = get_initial_fps(panel_info, var);
		var->reserved[0] = panel_info->fps;
	}

	disp_pr_info("fb%d", hfb_info->index);
	snprintf(fix->id, sizeof(fix->id), "hisifb%d", hfb_info->index);
	fix->line_length = hisifb_line_length(var->xres_virtual, bpp);
	fix->smem_len = roundup(fix->line_length * var->yres_virtual, PAGE_SIZE);
	fix->smem_start = 0;

	hisi_fbi_data_init(fbi_info);

	fix->reserved[0] = is_mipi_cmd_panel(panel_info->type) ? 1 : 0;

	disp_pr_info("----");
}

static void hisi_fb_init_priv_data(struct hisi_fb_info *fb_info)
{
	uint32_t i;
	char thread_name[128] = { 0 };

	fb_info->present_data.hfb_info = fb_info;
	fb_info->present_data.using_idx = 0;
	fb_info->present_data.displaying_idx = 0;
	fb_info->present_data.displayed_idx = 0;
	sema_init(&fb_info->present_data.frames_sem, 1);

	fb_info->present_kthread.handle = hisi_fb_present_kthread_handle;
	fb_info->present_kthread.data = &fb_info->present_data;

	snprintf(thread_name, sizeof(thread_name), "present_kthread%u", fb_info->id);
	hisi_disp_create_kthread(&fb_info->present_kthread, thread_name);
}

static int hisi_fb_register(struct platform_device *pdev, struct hisi_fb_info *fb_info)
{
	int ret;

	disp_pr_info(" enter ++++");

	/* info->par is a pointer that point to fb_info */
	fb_info->fbi_info = framebuffer_alloc(0, NULL);
	fb_info->index = hfb_info_list_index;
	hfb_info_list[hfb_info_list_index++] = fb_info;
	disp_pr_info("fb%d", fb_info->index);
	hisi_fb_init_fbi(fb_info);
	fb_info->fbi_info->par = fb_info;

	/* TODO: initial other fb information */
	hisi_fb_init_priv_data(fb_info);

	register_framebuffer(fb_info->fbi_info);

	return 0;
}

static int hisi_fb_probe(struct platform_device *pdev)
{
	int ret;
	struct hisi_fb_info *info = NULL;
	hisi_register_cb register_priv_ops_cb;

	if (hisi_dss_check_existence() != 0) {
		disp_pr_info("V7 logic has no DPU onboard");
		return -ENODEV;
	}

	disp_pr_info(" enter ++++");

	info = platform_get_drvdata(pdev);
	if (IS_ERR_OR_NULL(info))
		return -EINVAL;

	hisi_fb_read_dtsi(info);

	/* register fb device, generate /dev/graphic/fbxxx */
	ret = hisi_fb_register(pdev, info);
	if (ret) {
		disp_pr_err("fb register fail");
		return -1;
	}

	disp_pr_info(" ----fb%d, ov%d----\n", info->index, info->id);

	if (info->id >= ONLINE_OVERLAY_ID_MAX)
		return -1;

	dss_save_pdev(info->id, pdev);

	register_priv_ops_cb = g_fb_register_priv_ops_tbl[info->id];
	if (register_priv_ops_cb)
		register_priv_ops_cb(info);

	hisi_fb_create_sysfs_attrs(info);

	hisi_disp_test_register_test_module(DTE_TEST_MODULE_FB, info);

	disp_pr_info(" ---- ");
	return 0;
}

static int hisi_fb_remove(struct platform_device *dev)
{
	disp_pr_info(" enter ++++");
	struct hisi_fb_info *fb_info = NULL;

	if (!dev) {
		disp_pr_err("NULL Pointer\n");
		return -EINVAL;
	}

	fb_info = platform_get_drvdata(dev);
	if (!fb_info) {
		disp_pr_err("NULL Pointer\n");
		return -EINVAL;
	}

	if (fb_info->fb_priv_ops.vsync_ops.vsync_unregister != NULL)
		fb_info->fb_priv_ops.vsync_ops.vsync_unregister(fb_info->pdev);

	if (fb_info->fb_priv_ops.sysfs_ops.sysfs_remove_fnc != NULL)
		fb_info->fb_priv_ops.sysfs_ops.sysfs_remove_fnc(fb_info->pdev);

	disp_pr_info(" ---- ");
	return 0;
}

static void hisi_fb_shutdown(struct platform_device *dev)
{

}

struct dev_pm_ops hisi_primary_fb_dev_pm_ops = {
	.runtime_suspend = hisi_fb_runtime_suspend,
	.runtime_resume = hisi_fb_runtime_resume,
	.runtime_idle = hisi_fb_runtime_idle,
	.suspend = hisi_fb_pm_suspend,
	.resume = hisi_fb_pm_resume,
};

static const struct of_device_id hisi_fb_match_table[] = {
	{
		.compatible = "hisilicon,dpu_resource",
		.data = NULL,
	},
	{},
};
MODULE_DEVICE_TABLE(of, hisi_fb_match_table);

static struct platform_driver hisi_primary_fb_driver = {
	.probe = hisi_fb_probe,
	.remove = hisi_fb_remove,
	.suspend = hisi_fb_suspend,
	.resume = hisi_fb_resume,
	.shutdown = hisi_fb_shutdown,
	.driver = {
		.name = HISI_PRIMARY_FB_DEV_NAME,
		.owner  = THIS_MODULE,
		.of_match_table = of_match_ptr(hisi_fb_match_table),
		.pm = NULL,
	},
};

static int hisi_primary_fb_init(void)
{
	int ret = 0;

	disp_pr_info(" enter ++++");

	dss_init_fpgacontext();

	ret = platform_driver_register(&hisi_primary_fb_driver);
	if (ret) {
		disp_pr_err("platform_driver_register failed, error=%d!\n", ret);
		return ret;
	}

	disp_pr_info(" exit ----");

	return ret;
}

static void hisi_fb_init_info(struct platform_device *pdev, void *dev_data, void *input_data, void *input_ops)
{
	struct hisi_fb_info *fb_info = (struct hisi_fb_info *)dev_data;
	struct hisi_composer_device *overlay_dev = (struct hisi_composer_device *)input_data;

	disp_pr_info(" enter ++++");

	fb_info->pdev = pdev;

	fb_info->next_pdev = overlay_dev->pdev;
	fb_info->next_ops = input_ops;

	fb_info->fix_display_info.fix_panel_info = overlay_dev->ov_data.fix_panel_info;
	fb_info->id = overlay_dev->ov_data.ov_id;
	fb_info->pwr_on = false;
	fb_info->be_connected = true;
	sema_init(&fb_info->blank_sem, 1);
	atomic_set(&(fb_info->ref_cnt), 0);
}

void hisi_fb_create_platform_device(const char *name, void *ov_device, void *ov_ops)
{
	struct hisi_fb_info *fb_info = NULL;
	disp_pr_info(" ++++ ");
	fb_info = hisi_drv_create_device(name, sizeof(*fb_info), hisi_fb_init_info, ov_device, ov_ops);
	if (!fb_info) {
		disp_pr_err("create fb device fail");
		return;
	}
}

module_init(hisi_primary_fb_init);

MODULE_DESCRIPTION("Hisilicon Framebuffer Driver");
MODULE_LICENSE("GPL v2");

#ifdef CONFIG_HISI_DISP_TEST_ENABLE
EXPORT_SYMBOL(hisi_fb_blank);
EXPORT_SYMBOL(hisi_fb_ioctl);
#endif
