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

#include <linux/fb.h>
#include <linux/semaphore.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mm_virtio_iommu.h>
#include "dpufe.h"
#include "../include/dpu_communi_def.h"
#include "dpufe_dts.h"
#include "dpufe_vsync.h"
#include "dpufe_ops.h"
#include "dpufe_dbg.h"
#include "dpufe_vactive.h"
#include "dpufe_panel_def.h"
#include "dpufe_fb_buf_sync.h"
#include "dpufe_iommu.h"
#include "dpufe_async_play.h"
#include "dpufe_ov_play.h"
#include "dpufe_ion.h"
#include "dpufe_pan_display.h"
#include "dpufe_enum.h"

static struct dpufe_data_type *dpufe_list[MAX_DISP_CHN_NUM] = {0};

struct dpufe_data_type *dpufe_get_dpufd(uint32_t fb_idx)
{
	dpufe_check_and_return(fb_idx >= MAX_DISP_CHN_NUM, NULL, "fb[%u] is null\n", fb_idx);
	return dpufe_list[fb_idx];
}

int dpufe_get_pinfo(int fb_idx, struct panel_base_info *phy_pinfo)
{
	struct panel_base_info *pinfo = NULL;

	pinfo = get_panel_info_for_one_disp(fb_idx);
	if (pinfo == NULL) {
		DPUFE_ERR("pinfo is NULL\n");
		return -1;
	}

	phy_pinfo->xres = pinfo[0].xres;
	phy_pinfo->yres = pinfo[0].yres;
	phy_pinfo->width = pinfo[0].width;
	phy_pinfo->height = pinfo[0].height;
	phy_pinfo->panel_type = pinfo[0].panel_type;
	if (phy_pinfo->xres == 0) {
		DPUFE_ERR("get phy panel res failed\n");
		return -1;
	}

	return 0;
}

static int dpufe_register_callback(struct dpufe_data_type *dfd)
{
	// add special func
	switch (dfd->index) {
	case PRIMARY_PANEL_IDX:
		// dpufe pri panel func regist
		dfd->buf_sync_suspend = dpufe_reset_all_buf_fence;
		dfd->sysfs_create_fnc = dpufe_sysfs_create;
		dfd->sysfs_remove_fnc = dpufe_sysfs_remove;
		dfd->vsync_register = dpufe_vsync_register;
		dfd->vsync_unregister = dpufe_vsync_unregister;
		dfd->vstart_isr_register = dpufe_vstart_isr_register;
		dfd->vstart_isr_unregister = NULL;
		dfd->buf_sync_register = dpufe_buf_sync_register;
		dfd->buf_sync_unregister = dpufe_buf_sync_unregister;
		break;
	case EXTERNAL_PANEL_IDX:
		// dpufe ext panel func regist
		dfd->buf_sync_suspend = dpufe_reset_all_buf_fence;
		dfd->sysfs_create_fnc = NULL;
		dfd->sysfs_remove_fnc = NULL;
		dfd->vsync_register = dpufe_vsync_register;
		dfd->vsync_unregister = dpufe_vsync_unregister;
		dfd->vstart_isr_register = dpufe_vstart_isr_register;
		dfd->vstart_isr_unregister = NULL;
		dfd->buf_sync_register = dpufe_buf_sync_register;
		dfd->buf_sync_unregister = dpufe_buf_sync_unregister;
		break;
	case AUXILIARY_PANEL_IDX:
		// dpufe aux panel func regist
		break;
	default:
		DPUFE_ERR("unsupport index %d", dfd->index);
		return -1;
	}
	return 0;
}

static void dpufe_fbi_data_init(struct fb_info *fbi)
{
	fbi->screen_base = 0;
	fbi->flags = FBINFO_FLAG_DEFAULT;
	fbi->pseudo_palette = NULL;
	dpufe_register_fb_ops(fbi);
}

static int dpufe_init_fbi_info(struct dpufe_data_type *dfd, struct fb_info *fbi)
{
	uint32_t bpp = 0;
	struct fb_fix_screeninfo *fix = &fbi->fix;
	struct fb_var_screeninfo *var = &fbi->var;
	struct panel_base_info panel_info;

	dpufe_fb_init_screeninfo_base(fix, var);

	dpufe_fb_init_sreeninfo_by_img_type(fix, var, dfd->fb_img_type, &bpp);

	if(dpufe_get_pinfo(dfd->index, &panel_info) != 0) {
		DPUFE_ERR("dpufe get pinfo failed\n");
		return -1;
	}

	dpufe_fb_init_sreeninfo_by_panel_info(var, panel_info, dfd->fb_buf_num, bpp);

	(void)snprintf(fix->id, sizeof(fix->id), "dpufe-fb%d", dfd->index);
	fix->line_length = dpufe_line_length(dfd->index, var->xres_virtual, bpp);
	fix->smem_len = roundup(fix->line_length * var->yres_virtual, PAGE_SIZE);
	fix->smem_start = 0;
	fix->reserved[0] = 0;

	dpufe_fbi_data_init(fbi);

	return 0;
}

static void dpufe_init_sema(struct dpufe_data_type *dfd)
{
	sema_init(&dfd->blank_sem, 1);
}

static void dpufe_common_register(struct dpufe_data_type *dfd)
{
	if (dfd->vsync_register != NULL)
		dpufe_vsync_register(&dfd->vsync_ctrl, &dfd->attrs);
	if (dfd->vstart_isr_register != NULL)
		dpufe_vstart_isr_register(&dfd->vstart_ctl);
	if (dfd->sysfs_create_fnc != NULL)
		dpufe_sysfs_create(dfd->fbi->dev, &dfd->attrs);
	if (dfd->buf_sync_register != NULL)
		dpufe_buf_sync_register(dfd);
}

static int dpufe_framebuffer_register(struct dpufe_data_type *dfd)
{
	struct fb_info *fbi = NULL;

	fbi = dfd->fbi;
	if (dpufe_init_fbi_info(dfd, fbi) != 0) {
		DPUFE_DEBUG("init fbi info failed\n");
		return -1;
	}

	if (dpufe_register_callback(dfd) < 0) {
		DPUFE_DEBUG("init callback failed\n");
		return -1;
	}

	dpufe_init_sema(dfd);
	dpufe_sysfs_init(&dfd->attrs);
	dpufe_overlay_init(dfd);

	if (register_framebuffer(fbi) != 0) {
		DPUFE_ERR("fb%d failed to register_framebuffer!\n", dfd->index);
		return -1;
	}

	dpufe_common_register(dfd);
	DPUFE_INFO("FrameBuffer[%d] %dx%d size=%d bytes is registered successfully!\n",
		dfd->index, fbi->var.xres, fbi->var.yres, fbi->fix.smem_len);
	return 0;
}

static int dpufe_create_framebuffer(void)
{
	struct fb_info *fbi = NULL;
	struct dpufe_data_type *dfd = NULL;
	static int dpufe_list_idx = 0;

	/* alloc framebuffer info + par data */
	fbi = framebuffer_alloc(sizeof(struct dpufe_data_type), NULL);
	if (!fbi) {
		DPUFE_ERR("can't alloc framebuffer info data!\n");
		return -1;
	}

	dfd = (struct dpufe_data_type *)fbi->par;
	memset(dfd, 0, sizeof(*dfd));
	dfd->fbi = fbi;

	dfd->fb_img_type = DPU_FB_PIXEL_FORMAT_BGRA_8888;
	dfd->fb_buf_num = 0;
	dfd->fb_mem_free_flag = false;

	dfd->index = dpufe_list_idx;
	dpufe_list[dpufe_list_idx++] = dfd;
	DPUFE_INFO("index is %d\n", dfd->index);

	if (dfd->index == PRIMARY_PANEL_IDX) {
		dfd->fb_buf_num = DPU_FB0_BUF_NUM;
		dfd->fb_mem_free_flag = true;
	}

	dfd->disp_data = (struct dpu_core_disp_data *)kmalloc(sizeof(struct dpu_core_disp_data), GFP_KERNEL);
	if (!dfd->disp_data) {
		DPUFE_ERR("fb%u alloc disp data failed\n", dfd->index);
		framebuffer_release(fbi);
		return -1;
	}

	if (dpufe_framebuffer_register(dfd) < 0) {
		framebuffer_release(fbi);
		kfree(dfd->disp_data);
		DPUFE_DEBUG("framebuffer regist failed\n");
		return -1;
	}
	return 0;
}

int dpufe_probe(struct platform_device *pdev)
{
	int fb_idx;

	DPUFE_INFO("+\n");

	dpufe_iommu_enable();
	if (init_panel_info_by_dts() != 0) {
		DPUFE_ERR("init_panel_info_by_dts failed\n");
		return -1;
	}

	for (fb_idx = 0; fb_idx < MAX_DISP_CHN_NUM; fb_idx++) {
		if (dpufe_create_framebuffer() != 0) {
			DPUFE_ERR("framebuffer[%d] create fail!\n", fb_idx);
			return -1;
		}
	}

	DPUFE_INFO("-\n");
	return 0;
}

static int dpufe_suspend_sub(struct dpufe_data_type *dfd)
{
	int ret;

	if (!dfd) {
		DPUFE_ERR("dpufd is NULL");
		return -EINVAL;
	}

	DPUFE_INFO("[fb%d]+\n", dfd->index);

	ret = dpufe_blank_sub(dfd, FB_BLANK_POWERDOWN);
	if (ret) {
		DPUFE_ERR("fb%d can't turn off display, error=%d!\n", dfd->index, ret);
		return ret;
	}

	return 0;
}

static int dpufe_resume_sub(struct dpufe_data_type *dfd)
{
	int ret;

	if (!dfd) {
		DPUFE_ERR("dpufd is NULL");
		return -EINVAL;
	}

	ret = dpufe_blank_sub(dfd, FB_BLANK_UNBLANK);
	if (ret)
		DPUFE_ERR("fb%d can't turn on display, error=%d!\n", dfd->index, ret);

	return ret;
}

static int dpufe_suspend(struct platform_device *pdev, pm_message_t state)
{
	int i;
	int ret;
	struct dpufe_data_type *dfd = NULL;

	dpufe_check_and_return(!pdev, -EINVAL,"NULL Pointer\n");

	for (i = 0; i < MAX_DISP_CHN_NUM; ++i) {
		dfd = dpufe_list[i];
		DPUFE_INFO("fb%d, +\n", dfd->index);

		fb_set_suspend(dfd->fbi, FBINFO_STATE_SUSPENDED);
		ret = dpufe_suspend_sub(dfd);
		if (ret != 0) {
			DPUFE_ERR("fb%d dpu_fb_suspend_sub failed, error=%d!\n", dfd->index, ret);
			fb_set_suspend(dfd->fbi, FBINFO_STATE_RUNNING);
		} else {
			pdev->dev.power.power_state = state;
		}

		DPUFE_INFO("fb%d, -\n", dfd->index);
	}

	return ret;
}

static int dpufe_resume(struct platform_device *pdev)
{
	int i;
	int ret;
	struct dpufe_data_type *dfd = NULL;

	dpufe_check_and_return(!pdev, -EINVAL,"NULL Pointer\n");

	for (i = 0; i < MAX_DISP_CHN_NUM; ++i) {
		dfd = dpufe_list[i];
		DPUFE_INFO("fb%d, +\n", dfd->index);

		ret = dpufe_resume_sub(dfd);
		pdev->dev.power.power_state = PMSG_ON;
		fb_set_suspend(dfd->fbi, FBINFO_STATE_RUNNING);

		DPUFE_INFO("fb%d, -\n", dfd->index);
	}
	return ret;
}

static int dpufe_pm_suspend(struct device *dev)
{
	int i;
	int ret = 0;
	struct dpufe_data_type *dfd = NULL;

	dpufe_check_and_return(!dev, -EINVAL,"NULL Pointer\n");

	for (i = 0; i < MAX_DISP_CHN_NUM; ++i) {
		dfd = dpufe_list[i];
		DPUFE_INFO("fb%d, +\n", dfd->index);

		ret = dpufe_suspend_sub(dfd);
		if (ret != 0)
			DPUFE_ERR("fb%d dpufe_suspend_sub failed, error=%d.\n", dfd->index, ret);

		DPUFE_INFO("fb%d, -\n", dfd->index);
	}

	return ret;
}


static int dpufe_pm_resume(struct device *dev)
{
	int i;
	int ret = 0;
	struct dpufe_data_type *dfd = NULL;

	dpufe_check_and_return(!dev, -EINVAL,"NULL Pointer\n");

	for (i = 0; i < MAX_DISP_CHN_NUM; ++i) {
		dfd = dpufe_list[i];
		DPUFE_INFO("fb%d, +\n", dfd->index);

		ret = dpufe_resume_sub(dfd);
		if (ret != 0)
			DPUFE_ERR("fb%d dpufe_resume_sub failed, error=%d.\n", dfd->index, ret);

		DPUFE_INFO("fb%d, -\n", dfd->index);
	}
	return ret;
}

static int dpufe_remove(struct platform_device *pdev)
{
	int i;
	struct dpufe_data_type *dfd = NULL;

	if (!pdev) {
		DPUFE_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	for (i = 0; i < MAX_DISP_CHN_NUM; ++i) {
		dfd = dpufe_list[i];

		DPUFE_DEBUG("fb%d, +\n", dfd->index);

		/* stop the device */
		if (dpufe_suspend_sub(dfd) != 0)
			DPUFE_ERR("fb%d dpufe_suspend_sub failed!\n", dfd->index);

		if (dfd->disp_data)
			kfree(dfd->disp_data);

		/* remove /dev/fb* */
		unregister_framebuffer(dfd->fbi);

		/* unregister buf_sync */
		if (dfd->buf_sync_unregister != NULL)
			dfd->buf_sync_unregister(dfd);
		/* unregister vsync */
		if (dfd->vsync_unregister != NULL)
			dfd->vsync_unregister(&dfd->vsync_ctrl, &dfd->attrs);
		/* unregister vstart */
		if (dfd->vstart_isr_unregister != NULL)
			dfd->vstart_isr_unregister(&dfd->vstart_ctl);
		/* fb sysfs remove */
		if (dfd->sysfs_remove_fnc != NULL)
			dfd->sysfs_remove_fnc(&pdev->dev, &dfd->attrs);

		DPUFE_DEBUG("fb%d, -\n", dfd->index);
	}
	return 0;
}

static const struct dev_pm_ops dpufe_dev_pm_ops = {
	.suspend = dpufe_pm_suspend,
	.resume = dpufe_pm_resume,
};

static const struct of_device_id g_dpufe_panel_match_table[] = {
	{
		.compatible = DTS_ACE_PANEL_INFO,
		.data = NULL,
	},
	{},
};

static struct platform_driver this_driver = {
	.probe = dpufe_probe,
	.remove = dpufe_remove,
	.suspend = dpufe_suspend,
	.resume = dpufe_resume,
	.driver = {
		.name = "dpu_ace",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(g_dpufe_panel_match_table),
		.pm = &dpufe_dev_pm_ops,
	},
};

static int __init dpu_ace_init(void)
{
	int ret;
	DPUFE_DEBUG("+\n");

	ret = platform_driver_register(&this_driver);
	if (ret != 0) {
		DPUFE_ERR("platform_driver_register failed, error=%d!\n", ret);
		return ret;
	}

	DPUFE_DEBUG("-\n");

	return ret;
}

module_init(dpu_ace_init);
