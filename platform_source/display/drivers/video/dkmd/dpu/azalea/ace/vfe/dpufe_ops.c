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
#include <linux/uaccess.h>

#include "dpufe_iommu.h"
#include "dpufe.h"
#include "dpufe_ov_play.h"
#include "dpufe_async_play.h"
#include "../include/dpu_communi_def.h"
#include "dpufe_ioctl.h"
#include "dpufe_dts.h"
#include "dpufe_vsync.h"
#include "dpufe_dbg.h"
#include "dpufe_dev.h"
#include "dpufe_virtio_fb.h"
#include "dpufe_dts.h"
#include "dpufe_pan_display.h"
#include "dpufe_ion.h"
#include "dpufe_mmbuf_manager.h"
#include "dpufe_ov_utils.h"
#include "dpufe_disp_recorder.h"

#define DPUFE_PLATFORM_TYPE  (DPUFE_ACCEL_DSSV510 | DPUFE_ACCEL_PLATFORM_TYPE_ASIC)
#define DPUFE_ACCEL_DSSV510 0x200
#define DPUFE_ACCEL_PLATFORM_TYPE_ASIC 0x20000000

static int dpufe_get_platform_panel_info(void __user *argp)
{
	struct platform_panel_info get_platform_panel_info;
	uint32_t panel_num_one_chn = 0;
	struct panel_base_info *pinfo = NULL;

	memset(&get_platform_panel_info, 0, sizeof(struct platform_panel_info));

	pinfo = get_panel_info_for_one_disp(PRIMARY_PANEL_IDX);
	dpufe_check_and_return(!pinfo, -1, "get panel info failed\n");
	panel_num_one_chn = get_panel_num_for_one_disp(PRIMARY_PANEL_IDX);
	dpufe_check_and_return(panel_num_one_chn == 0, -1, "get panel num failed\n");

	get_platform_panel_info.disp_phy_pri_pres = pinfo[0];

	if (panel_num_one_chn > 1) {
		get_platform_panel_info.disp_logical_pri_cnnt = 1;
		get_platform_panel_info.disp_logical_pri_pres = pinfo[1];
	}

	if (get_disp_chn_num() > 1) {
		get_platform_panel_info.disp_phy_ext_cnnt = 1;
		pinfo = get_panel_info_for_one_disp(EXTERNAL_PANEL_IDX);
		dpufe_check_and_return(!pinfo, -1, "get panel info failed\n");

		panel_num_one_chn = get_panel_num_for_one_disp(EXTERNAL_PANEL_IDX);
		dpufe_check_and_return(panel_num_one_chn == 0, -1, "get panel num failed\n");

		get_platform_panel_info.disp_phy_ext_pres = pinfo[0];

		if (panel_num_one_chn > 1) {
			get_platform_panel_info.disp_logical_ext_cnnt = 1;
			get_platform_panel_info.disp_logical_ext_pres = pinfo[1];
		}
	}

	if (copy_to_user(argp, &get_platform_panel_info, sizeof(struct platform_panel_info))) {
		DPUFE_ERR("copy to user fail!\n");
		return -EINVAL;
	}

	return 0;
}

static int dpufe_lcd_dirty_region_info_get(void __user *argp)
{
	struct lcd_dirty_region_info linfo;

	memset(&linfo, 0, sizeof(struct lcd_dirty_region_info));

	if (copy_to_user(argp, &linfo, sizeof(struct lcd_dirty_region_info))) {
		DPUFE_ERR("copy to user fail");
		return -EFAULT;
	}

	return 0;
}

static int dpufe_get_platform_product_info(void __user *argp)
{
	struct platform_product_info get_platform_product_info;

	memset(&get_platform_product_info, 0, sizeof(struct platform_product_info));
	if (copy_to_user(argp, &get_platform_product_info, sizeof(struct platform_product_info))) {
		DPUFE_ERR("copy to user fail!\n");
		return -EINVAL;
	}

	return 0;
}

static int dpufe_dss_get_platform_type(void __user *argp)
{
	int type = DPUFE_PLATFORM_TYPE;
	int ret;

	if (!argp) {
		DPUFE_ERR("NULL Pointer!\n");
		return -EINVAL;
	}
	ret = copy_to_user(argp, &type, sizeof(type));
	if (ret) {
		DPUFE_ERR("copy to user failed! ret=%d\n", ret);
		ret = -EFAULT;
	}

	return ret;
}

static int dpufe_blank_power_on(struct dpufe_data_type *dfd)
{
	int ret;

	DPUFE_INFO("fb[%u] +\n", dfd->index);

	if (dfd->panel_power_on) {
		DPUFE_INFO("fb[%d] already power on\n", dfd->index);
		return 0;
	}

	ret = vfb_send_blank_ctl(dfd->index, FB_BLANK_UNBLANK);
	dpufe_check_and_return(ret != 0, ret, "send unblank ctl fail\n");

	dpufe_resume_async_state(dfd);
	dfd->panel_power_on = true;
	enable_disp_recorder(&dfd->disp_recorder);

	DPUFE_INFO("fb[%u] -\n", dfd->index);
	return 0;
}

static int dpufe_blank_power_off(struct dpufe_data_type *dfd)
{
	int ret;

	DPUFE_INFO("fb[%u] +\n", dfd->index);

	if (!dfd->panel_power_on) {
		DPUFE_INFO("fb[%u] already power down\n", dfd->index);
		return 0;
	}

	ret = vfb_send_blank_ctl(dfd->index, FB_BLANK_POWERDOWN);
	dpufe_check_and_return(ret != 0, ret, "send power down ctl fail\n");

	if (dfd->buf_sync_suspend != NULL)
		dfd->buf_sync_suspend(dfd);

	dfd->panel_power_on = false;
	dfd->online_play_count = 0;
	disable_disp_recorder(&dfd->disp_recorder);
	empty_dbg_queues(dfd->index);

	DPUFE_INFO("fb[%u] -\n", dfd->index);

	return 0;
}

int dpufe_blank_sub(struct dpufe_data_type *dfd, int blank_mode)
{
	int ret = -1;

	if (dfd->index >= get_registered_fb_nums()) {
		DPUFE_DEBUG("fb%d unexist\n", dfd->index);
		return 0;
	}

	down(&dfd->blank_sem);
	switch (blank_mode) {
	case FB_BLANK_UNBLANK:
		ret = dpufe_blank_power_on(dfd);
		break;
	case FB_BLANK_POWERDOWN:
		ret = dpufe_blank_power_off(dfd);
		break;
	default:
		DPUFE_ERR("unsupport blank mode[%d]\n", blank_mode);
		break;
	}

	up(&dfd->blank_sem);
	return ret;
}

static int dpufe_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
	int ret = -1;
	struct dpufe_data_type *dfd = NULL;
	void __user *argp = (void __user *)(uintptr_t)arg;

	dpufe_check_and_return(!info, -EINVAL, "info NULL Pointer!\n");
	dpufe_check_and_return(!argp, -EINVAL, "argp NULL Pointer!\n");

	dfd = (struct dpufe_data_type *)info->par;
	dpufe_check_and_return(!dfd, -EINVAL, "dpufd is NULL!\n");

	if (dfd->index >= get_registered_fb_nums()) {
		DPUFE_DEBUG("fb%d unexist\n", dfd->index);
		return 0;
	}

	switch (cmd) {
	case DPUFE_OV_ONLINE_PLAY:
		ret = dpufe_ov_online_play(dfd, argp);
		break;
	case DPUFE_OV_ASYNC_PLAY:
		ret = dpufe_ov_async_play(dfd, argp);
		break;
	case DPUFE_GET_RELEASE_AND_RETIRE_FENCE:
		ret = dpufe_get_release_and_retire_fence(dfd, argp);
		break;
	case DPUFE_VSYNC_CTRL:
		ret = dpufe_vsync_ctrl(&dfd->vsync_ctrl, argp);
		break;
	case DPUFE_PLATFORM_PRODUCT_INFO_GET:
		ret = dpufe_get_platform_product_info(argp);
		break;
	case DPUFE_PLATFORM_TYPE_GET:
		ret = dpufe_dss_get_platform_type(argp);
		break;
	case DPUFE_PLATFORM_PANEL_INFO_GET:
		ret = dpufe_get_platform_panel_info(argp);
		break;
	case DPUFE_LCD_DIRTY_REGION_INFO_GET:
		ret = dpufe_lcd_dirty_region_info_get(argp);
		break;
	case DPUFE_GRALLOC_MAP_IOVA:
		ret = dpufe_buffer_map_iova(argp);
		break;
	case DPUFE_GRALLOC_UNMAP_IOVA:
		ret = dpufe_buffer_unmap_iova(argp);
		break;
	case DPUFE_EVS_SWITCH:
		ret = dpufe_evs_switch(dfd, argp);
		break;
	case DPUFE_MMBUF_ALLOC:
		ret = dpufe_mmbuf_request(info, argp);
		break;
	case DPUFE_MMBUF_FREE:
		ret = dpufe_mmbuf_release(info, argp);
		break;
	case DPUFE_MMBUF_FREE_ALL:
		ret = dpufe_mmbuf_free_all(info, argp);
		break;
	default:
		DPUFE_ERR("ioctl fb[%u]: not support cmd 0x%x\n", dfd->index, cmd);
		break;
	}

	return ret;
}

static int dpufe_open(struct fb_info *info, int user)
{
	int ret;
	struct dpufe_data_type *dfd = NULL;

	dpufe_check_and_return(!info, -EINVAL, "info is NULL\n");

	dfd = (struct dpufe_data_type *)info->par;
	dpufe_check_and_return(!dfd, -EINVAL,"dfd is NULL\n");
	if (dfd->ref_cnt == 0) {
		ret = dpufe_blank_sub(dfd, FB_BLANK_UNBLANK);
		dpufe_check_and_return(ret != 0, ret, "dpufe_open fb[%u] failed\n", dfd->index);
	}
	dfd->ref_cnt++;
	return 0;
}

static int dpufe_release(struct fb_info *info, int user)
{
	int ret;
	struct dpufe_data_type *dfd = NULL;

	dpufe_check_and_return(!info, -EINVAL, "info is NULL\n");

	dfd = (struct dpufe_data_type *)info->par;
	dpufe_check_and_return(!dfd, -EINVAL, "dfd is NULL\n");

	dfd->ref_cnt--;
	if (dfd->ref_cnt == 0) {
		ret = dpufe_blank_sub(dfd, FB_BLANK_POWERDOWN);
		dpufe_check_and_return(ret != 0, ret, "dpufe_open fb[%u] failed\n", dfd->index);
		dpufe_dbg_queue_free();
		dpufe_free_reserve_buffer(dfd);
	}
	return 0;
}

static int dpufe_blank(int blank_mode, struct fb_info *info)
{
	int ret;
	struct dpufe_data_type *dfd = NULL;

	dpufe_check_and_return(!info, -EINVAL, "info is NULL\n");
	dpufe_check_and_return((blank_mode != FB_BLANK_UNBLANK) && (blank_mode != FB_BLANK_POWERDOWN), -EINVAL,
		"wrong blank mode[%d]\n", blank_mode);

	dfd = (struct dpufe_data_type *)info->par;
	dpufe_check_and_return(!dfd, -EINVAL, "dfd is NULL\n");

	ret = dpufe_blank_sub(dfd, blank_mode);
	dpufe_check_and_return(ret != 0, ret, "handle blank mode[%d] failed\n", blank_mode);

	return 0;
}

static struct fb_ops dpufe_ops = {
	.owner = THIS_MODULE,
	.fb_open = dpufe_open,
	.fb_release = dpufe_release,
	.fb_read = NULL,
	.fb_write = NULL,
	.fb_cursor = NULL,
	.fb_check_var = dpufe_fb_check_var,
	.fb_set_par = dpufe_fb_set_par,
	.fb_setcolreg = NULL,
	.fb_blank = dpufe_blank,
	.fb_pan_display = dpufe_fb_pan_display,
	.fb_fillrect = NULL,
	.fb_copyarea = NULL,
	.fb_imageblit = NULL,
	.fb_sync = NULL,
	.fb_ioctl = dpufe_ioctl,
	.fb_compat_ioctl = dpufe_ioctl,
	.fb_mmap = dpufe_fb_mmap,
};

void dpufe_register_fb_ops(struct fb_info *fbi)
{
	fbi->fbops = &dpufe_ops;
}
