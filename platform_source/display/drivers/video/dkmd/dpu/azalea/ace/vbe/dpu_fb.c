/* Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#include <linux/module.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include <linux/device.h>
#include "dpu_fb.h"
#include "dpu_fb_dts.h"
#include "dpu_display_effect.h"
#include "dpu_mmbuf_manager.h"
#include "dpu_mipi_dsi.h"
#include "dpu_fb_panel.h"
#include "dpu_fb_sysfs.h"
#include "dpu_dpe_utils.h"
#include "dpu_iommu.h"
#include "overlay/dpu_overlay_utils_platform.h"
#include "overlay/init/dpu_init.h"
#include "overlay/dump/dpu_dump.h"
#include "chrdev/dpu_chrdev.h"
#include "merger_mgr/dpu_disp_merger_mgr.h"
#include "dpu_disp_recorder.h"

#define set_flag_bits_if_cond(cond, flag, bits) \
	do { if (cond) (flag) |= (bits); } while (0)

uint8_t color_temp_cal_buf[32] = {0};

static int dpu_fb_resource_initialized;
static struct platform_device *pdev_list[DPU_FB_MAX_DEV_LIST] = {0};

static int pdev_list_cnt;
static int fbi_list_index;

struct dpu_fb_data_type *dpufd_list[DPU_FB_MAX_FBI_LIST] = {0};
static int dpufd_list_index;
struct semaphore g_dpufb_dss_clk_vote_sem;

uint32_t g_dts_resouce_ready;
uint32_t g_fastboot_enable_flag;
uint32_t g_fake_lcd_flag;
uint32_t g_dss_version_tag;
uint32_t g_dss_module_resource_initialized;
uint32_t g_logo_buffer_base;
uint32_t g_logo_buffer_size;
uint32_t g_mipi_dphy_version;
uint32_t g_mipi_dphy_opt;
uint32_t g_chip_id;

uint32_t g_fastboot_already_set;

unsigned int g_esd_recover_disable;  //lint !e552
int g_fastboot_set_needed;

struct dpu_iova_info g_dpu_iova_info;
struct dpu_iova_info g_dpu_vm_iova_info;

#define MAX_DPE_NUM 3
static struct regulator_bulk_data g_dpe_regulator[MAX_DPE_NUM];

/*lint -e552*/
unsigned int g_primary_lcd_xres;
unsigned int g_primary_lcd_yres;
uint64_t g_pxl_clk_rate;
uint8_t g_prefix_ce_support;
uint8_t g_prefix_sharpness1d_support;
uint8_t g_prefix_sharpness2d_support;
/*lint +e552*/

int g_debug_enable_lcd_sleep_in;
uint32_t g_err_status;  //lint !e552

struct dpu_fb_data_type *g_dpufd_fb0;

static int dpu_fb_suspend_sub(struct dpu_fb_data_type *dpufd);
static int dpu_fb_resume_sub(struct dpu_fb_data_type *dpufd);
#ifdef CONFIG_HAS_EARLYSUSPEND
static void dpufb_early_suspend(struct early_suspend *h);
static void dpufb_early_resume(struct early_suspend *h);
#endif

#ifdef CONFIG_PM_RUNTIME
static void dpufb_pm_runtime_get(struct dpu_fb_data_type *dpufd);
static void dpufb_pm_runtime_put(struct dpu_fb_data_type *dpufd);
static void dpufb_pm_runtime_register(struct platform_device *pdev);
static void dpufb_pm_runtime_unregister(struct platform_device *pdev);
#endif

/*******************************************************************************
 *
 */
static void dpufb_init_regulator(struct dpu_fb_data_type *dpufd)
{
	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is NULL\n");

	dpufd->dpe_regulator = &(g_dpe_regulator[0]);
	dpufd->smmu_tcu_regulator = &(g_dpe_regulator[1]);
	dpufd->mediacrg_regulator = &(g_dpe_regulator[2]);
}

struct dpu_fb_data_type *dpufb_get_dpufd(uint32_t fb_idx)
{
	dpu_check_and_return(fb_idx > (uint32_t)dpufd_list_index, NULL, ERR, "fb[%u] is null\n", fb_idx);
	return dpufd_list[fb_idx];
}

struct platform_device *dpu_fb_add_device(struct platform_device *pdev)
{
	struct dpu_fb_panel_data *pdata = NULL;
	struct platform_device *this_dev = NULL;
	struct fb_info *fbi = NULL;
	struct dpu_fb_data_type *dpufd = NULL;
	uint32_t type;
	uint32_t id;

	dpu_check_and_return(!pdev, NULL, ERR, "pdev is NULL\n");
	pdata = dev_get_platdata(&pdev->dev);
	dpu_check_and_return(!pdata, NULL, ERR, "pdata is NULL\n");

	dpu_check_and_return((fbi_list_index >= DPU_FB_MAX_FBI_LIST), NULL, ERR, "no more framebuffer info list!\n");

	id = pdev->id;
	type = pdata->panel_info->type;
	/* alloc panel device data */
	this_dev = dpu_fb_device_alloc(pdata, type, id);
	dpu_check_and_return(!this_dev, NULL, ERR, "failed to dpu_fb_device_alloc!\n");

	/* alloc framebuffer info + par data */
	fbi = framebuffer_alloc(sizeof(struct dpu_fb_data_type));
	if (!fbi) {
		DPU_FB_ERR("can't alloc framebuffer info data!\n");
		platform_device_put(this_dev);
		return NULL;
	}

	dpufd = (struct dpu_fb_data_type *)fbi->par;
	memset(dpufd, 0, sizeof(*dpufd));
	dpufd->fbi = fbi;
	dpufd->fb_imgType = DPU_FB_PIXEL_FORMAT_BGRA_8888;
	dpufd->index = dpufd_list_index;

	dpufb_init_base_addr(dpufd);
	dpufb_init_clk_name(dpufd);
	if (dpufb_init_irq(dpufd, type)) {
		platform_device_put(this_dev);
		framebuffer_release(fbi);
		return NULL;
	}

	dpufb_init_regulator(dpufd);

	/* link to the latest pdev */
	dpufd->pdev = this_dev;
	dpufd_list[dpufd_list_index++] = dpufd;

	 /* get/set panel info */
	memcpy(&dpufd->panel_info, pdata->panel_info, sizeof(*pdata->panel_info));
	/* set driver data */
	platform_set_drvdata(this_dev, dpufd);

	if (platform_device_add(this_dev)) {
		DPU_FB_ERR("failed to platform_device_add!\n");
		framebuffer_release(fbi);
		platform_device_put(this_dev);
		dpufd_list_index--;
		fbi_list_index--;
		return NULL;
	}

	return this_dev;
}

static void dpu_fb_displayeffect_update(struct dpu_fb_data_type *dpufd)
{
	int disp_panel_id = dpufd->panel_info.disp_panel_id;

	if (dpufd->index == AUXILIARY_PANEL_IDX)
		return;

	dpufd->effect_updated_flag[disp_panel_id].post_xcc_effect_updated = true;
	dpufd->effect_updated_flag[disp_panel_id].hiace_effect_updated = true;
	dpufd->hiace_info[dpufd->panel_info.disp_panel_id].algorithm_result = 0;
}

static void dpu_fb_down_effect_sem(struct dpu_fb_data_type *dpufd)
{
	down(&dpufd->blank_sem_effect);
	down(&dpufd->blank_sem_effect_hiace);
	down(&dpufd->blank_sem_effect_gmp);
}

static void dpu_fb_up_effect_sem(struct dpu_fb_data_type *dpufd)
{
	up(&dpufd->blank_sem_effect_gmp);
	up(&dpufd->blank_sem_effect_hiace);
	up(&dpufd->blank_sem_effect);
}

int dpu_fb_blank_sub(struct dpu_fb_data_type *dpufd, int blank_mode)
{
	int ret = 0;

	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL\n");

	down(&dpufd->blank_sem);
	down(&dpufd->blank_sem0);
	dpu_fb_down_effect_sem(dpufd);
	switch (blank_mode) {
	case FB_BLANK_UNBLANK:
		dpu_fb_displayeffect_update(dpufd);

		if (!dpufd->panel_power_on)
			ret = dpu_fb_blank_panel_power_on(dpufd);

		break;

	case FB_BLANK_VSYNC_SUSPEND:
	case FB_BLANK_HSYNC_SUSPEND:
	case FB_BLANK_NORMAL:
	case FB_BLANK_POWERDOWN:
	default:
		if (dpufd->panel_power_on) {
			if (dpu_fb_blank_update_tui_status(dpufd))
				break;

			ret = dpu_fb_blank_panel_power_off(dpufd);

			if (dpufd->buf_sync_suspend != NULL)
				dpufd->buf_sync_suspend(dpufd);

			/* reset online play bypass state, ensure normal state when next power on */
			(void) dpu_online_play_bypass_set(dpufd, false);
		}
		break;
	}
	dpu_fb_up_effect_sem(dpufd);
	up(&dpufd->blank_sem0);
	up(&dpufd->blank_sem);

	return ret;
}

static bool dpu_fb_set_fastboot_needed(struct dpu_fb_data_type *dpufd)
{
	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL");
		return false;
	}

	if (g_fastboot_set_needed == 1) {
		dpufb_ctrl_fastboot(dpufd);
		dpufd->panel_power_on = true;
		g_fastboot_set_needed = 0;
		return true;
	}

	return false;
}

int dpu_fb_open_sub(struct dpu_fb_data_type *dpufd)
{
	int ret = 0;
	bool needed = false;

	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	if (dpufd->set_fastboot_fnc != NULL)
		needed = dpufd->set_fastboot_fnc(dpufd);

	if (!needed) {
		ret = dpu_fb_blank_sub(dpufd, FB_BLANK_UNBLANK);
		if (ret != 0) {
			DPU_FB_ERR("can't turn on display!\n");
			return ret;
		}
	}

	return 0;
}

int dpu_fb_release_sub(struct dpu_fb_data_type *dpufd)
{
	int ret;

	if (!dpufd) {
		DPU_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	ret = dpu_fb_blank_sub(dpufd, FB_BLANK_POWERDOWN);
	if (ret != 0) {
		DPU_FB_ERR("can't turn off display!\n");
		return ret;
	}

	return 0;
}

void dpu_fb_frame_refresh(struct dpu_fb_data_type *dpufd, char *trigger)
{
	char *envp[2];  /* uevent num for environment pointer */
	char buf[64] = {0};  /* buf len for fb_frame_refresh flag */

	(void)snprintf(buf, sizeof(buf), "Refresh=1");
	envp[0] = buf;
	envp[1] = NULL;

	if (!dpufd || !trigger) {
		DPU_FB_ERR("NULL Pointer\n");
		return;
	}

	kobject_uevent_env(&(dpufd->fbi->dev->kobj), KOBJ_CHANGE, envp);
	DPU_FB_INFO("fb%d, %s frame refresh\n", dpufd->index, trigger);
}

static int dpu_fb_blank(int blank_mode, struct fb_info *info)
{
	int ret;
	struct dpu_fb_data_type *dpufd = NULL;

	DPU_FB_DEBUG("+\n");

	dpu_check_and_return(!info, -EINVAL, ERR, "info is NULL\n");

	dpufd = (struct dpu_fb_data_type *)info->par;
	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL\n");

	dpu_check_and_return((dpufd->panel_info.fake_external && (dpufd->index == EXTERNAL_PANEL_IDX)),
		-EINVAL, INFO, "it is fake, blank it fail\n");

	dpu_log_enable_if((dpufd->index != AUXILIARY_PANEL_IDX), "fb%d, blank_mode[%d] +\n",
		dpufd->index, blank_mode);

	ret = dpu_fb_blank_device(dpufd, blank_mode);
	if (ret != 0)
		goto hw_unlock;

	dpu_log_enable_if((dpufd->index != AUXILIARY_PANEL_IDX), "fb%d, blank_mode[%d] -\n",
		dpufd->index, blank_mode);

	g_err_status = 0;

	DPU_FB_DEBUG("-\n");
	return 0;

hw_unlock:
	return ret;
}

static int dpu_fb_open(struct fb_info *info)
{
	int ret = 0;
	struct dpu_fb_data_type *dpufd = NULL;

	if (!info) {
		DPU_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	dpufd = (struct dpu_fb_data_type *)info->par;
	if (!dpufd) {
		DPU_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	if (!dpufd->ref_cnt) {
		DPU_FB_DEBUG("fb%d, +!\n", dpufd->index);
		if (dpufd->open_sub_fnc != NULL)
			ret = dpufd->open_sub_fnc(dpufd);
		DPU_FB_DEBUG("fb%d, -! ret = %d\n", dpufd->index, ret);
	}

	dpufd->ref_cnt++;

	return ret;
}

static int dpu_fb_release(struct fb_info *info)
{
	int ret = 0;
	struct dpu_fb_data_type *dpufd = NULL;

	if (!info) {
		DPU_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	dpufd = (struct dpu_fb_data_type *)info->par;
	if (!dpufd) {
		DPU_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	if (!dpufd->ref_cnt) {
		DPU_FB_INFO("try to close unopened fb%d!\n", dpufd->index);
		return -EINVAL;
	}

	dpufd->ref_cnt--;

	if (!dpufd->ref_cnt) {
		DPU_FB_DEBUG("fb%d, +\n", dpufd->index);
		if (dpufd->release_sub_fnc != NULL)
			ret = dpufd->release_sub_fnc(dpufd);
		DPU_FB_DEBUG("fb%d, -\n", dpufd->index);
	}

	return ret;
}

static int dpufb_idle_is_allowed(struct dpu_fb_data_type *dpufd, void __user *argp)
{
	int is_allowed = 0;

	if (!dpufd) {
		DPU_FB_ERR("idle is allowed dpufd NULL Pointer!\n");
		return -EINVAL;
	}

	if (!argp) {
		DPU_FB_ERR("idle is allowed argp NULL Pointer!\n");
		return -EINVAL;
	}

	is_allowed = (dpufd->frame_update_flag == 1) ? 0 : 1;

	if (copy_to_user(argp, &is_allowed, sizeof(is_allowed))) {
		DPU_FB_ERR("copy to user fail");
		return -EFAULT;
	}

	return 0;
}

static int dpufb_debug_check_fence_timeline(struct dpu_fb_data_type *dpufd)
{
	struct dpufb_buf_sync *buf_sync_ctrl = NULL;
	unsigned long flags = 0;
	int val = 0;

	if (!dpufd) {
		DPU_FB_ERR("timeline dpufd NULL Pointer!\n");
		return -EINVAL;
	}

	buf_sync_ctrl = &dpufd->buf_sync_ctrl;
	if (buf_sync_ctrl->timeline == NULL) {
		DPU_FB_ERR("timeline NULL Pointer!\n");
		return -EINVAL;
	}

	if (dpufd->ov_req.frame_no != 0)
		DPU_FB_INFO("fb%d frame_no[%d] timeline_max[%d], TL(Nxt %d , Crnt %d)!\n",
			dpufd->index, dpufd->ov_req.frame_no, buf_sync_ctrl->timeline_max,
			buf_sync_ctrl->timeline->next_value, buf_sync_ctrl->timeline->value);

	spin_lock_irqsave(&buf_sync_ctrl->refresh_lock, flags);

	if ((buf_sync_ctrl->timeline->next_value - buf_sync_ctrl->timeline->value) > 0)
		val = buf_sync_ctrl->timeline->next_value - buf_sync_ctrl->timeline->value;

	// TODO:dpu_resync_timeline(buf_sync_ctrl->timeline);
	// TODO:dpu_resync_timeline(buf_sync_ctrl->timeline_retire);

	buf_sync_ctrl->timeline_max = buf_sync_ctrl->timeline->next_value + 1;
	buf_sync_ctrl->refresh = 0;

	spin_unlock_irqrestore(&buf_sync_ctrl->refresh_lock, flags);

	if (dpufd->ov_req.frame_no != 0) {
		DPU_FB_INFO("fb%d frame_no[%d] timeline_max[%d], TL(Nxt %d , Crnt %d)!\n",
			dpufd->index, dpufd->ov_req.frame_no, buf_sync_ctrl->timeline_max,
			buf_sync_ctrl->timeline->next_value, buf_sync_ctrl->timeline->value);
#ifdef CONFIG_MALI_FENCE_DEBUG
		kbase_fence_dump_in_display();
#endif
	}

	return 0;
}

static int dpufb_dss_get_platform_type(void __user *argp)
{
	int type;
	int ret;

	type = DPUFB_PLATFORM_TYPE;
	if (g_fpga_flag == 1)
		type = DPUFB_PLATFORM_TYPE | FB_ACCEL_PLATFORM_TYPE_FPGA;

	if (!argp) {
		DPU_FB_ERR("NULL Pointer!\n");
		return -EINVAL;
	}
	ret = copy_to_user(argp, &type, sizeof(type));
	if (ret) {
		DPU_FB_ERR("copy to user failed! ret=%d\n", ret);
		ret = -EFAULT;
	}

	return ret;
}

static int dpufb_dss_get_platform_product_info(struct dpu_fb_data_type *dpufd, void __user *argp)
{
	struct platform_product_info get_platform_product_info;

	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL\n");
		return -EINVAL;
	}

	if (!argp) {
		DPU_FB_ERR("argp is NULL\n");
		return -EINVAL;
	}

	memset(&get_platform_product_info, 0, sizeof(struct platform_product_info));
	get_platform_product_info.max_hwc_mmbuf_size = mmbuf_max_size;
	get_platform_product_info.max_mdc_mmbuf_size = dss_mmbuf_reserved_info[SERVICE_MDC].size;

	get_platform_product_info.dummy_pixel_num = dpufd->panel_info.dummy_pixel_num;
	get_platform_product_info.dfr_support_value = dpufd->panel_info.dfr_support_value;
	get_platform_product_info.ifbc_type = dpufd->panel_info.ifbc_type;
	get_platform_product_info.p3_support = dpufd->panel_info.p3_support;
	get_platform_product_info.hdr_flw_support = dpufd->panel_info.hdr_flw_support;
	get_platform_product_info.post_hihdr_support = dpufd->panel_info.post_hihdr_support;
	get_platform_product_info.dfr_method = dpufd->panel_info.dfr_method;
	get_platform_product_info.support_tiny_porch_ratio = dpufd->panel_info.support_tiny_porch_ratio;
	get_platform_product_info.support_ddr_bw_adjust = dpufd->panel_info.support_ddr_bw_adjust;
	get_platform_product_info.actual_porch_ratio = dpufd->panel_info.mipi.porch_ratio;
	get_platform_product_info.dual_lcd_support = 0;

	if (copy_to_user(argp, &get_platform_product_info, sizeof(struct platform_product_info))) {
		DPU_FB_ERR("copy to user fail!\n");
		return -EINVAL;
	}

	DPU_FB_INFO("max_hwc_mmbuf_size=%d, max_mdc_mmbuf_size=%d,"
		"dummy_pixel_num=%d, dfr_support_value=%d, p3_support = %d, post_hihdr_support = %d, hdr_flw_support = %d,"
		"ifbc_type = %d, support_tiny_porch_ratio=%u, support_ddr_bw_adjust = %d, actual_porch_ratio=%u,"
		"dual_lcd_support = %d\n",
		get_platform_product_info.max_hwc_mmbuf_size, get_platform_product_info.max_mdc_mmbuf_size,
		get_platform_product_info.dummy_pixel_num, get_platform_product_info.dfr_support_value,
		get_platform_product_info.p3_support, get_platform_product_info.post_hihdr_support,
		get_platform_product_info.hdr_flw_support, get_platform_product_info.ifbc_type,
		get_platform_product_info.support_tiny_porch_ratio, get_platform_product_info.support_ddr_bw_adjust,
		get_platform_product_info.actual_porch_ratio, get_platform_product_info.dual_lcd_support);

	return 0;
}

static int dpu_fb_resource_ioctl(struct dpu_fb_data_type *dpufd, unsigned int cmd, void __user *argp)
{
	int ret = -ENOSYS;

	switch (cmd) {
	case DPUFB_DSS_VOLTAGE_GET:
		ret = dpufb_ctrl_dss_voltage_get(dpufd, argp);
		break;
	case DPUFB_DSS_VOLTAGE_SET:
		ret = dpufb_ctrl_dss_voltage_set(dpufd, argp);
		break;
	case DPUFB_DSS_VOTE_CMD_SET:
		ret = dpufb_ctrl_dss_vote_cmd_set(dpufd, argp);
		break;
	case DPUFB_DEBUG_CHECK_FENCE_TIMELINE:
		ret = dpufb_debug_check_fence_timeline(dpufd);
		break;
	case DPUFB_IDLE_IS_ALLOWED:
		ret = dpufb_idle_is_allowed(dpufd, argp);
		break;
	case DPUFB_PLATFORM_TYPE_GET:
		ret = dpufb_dss_get_platform_type(argp);
		break;
	case DPUFB_PLATFORM_PRODUCT_INFO_GET:
		ret = dpufb_dss_get_platform_product_info(dpufd, argp);
		break;
	case DPUFB_DPTX_GET_COLOR_BIT_MODE:
		if (dpufd->dp_get_color_bit_mode != NULL)
			ret = dpufd->dp_get_color_bit_mode(dpufd, argp);
		break;
	default:
		break;
	}

	return ret;
}

static int dpu_fb_effect_ioctl(struct dpu_fb_data_type *dpufd, unsigned int cmd, void __user *argp)
{
	int ret = -ENOSYS;

	switch (cmd) {
	case DPUFB_EFFECT_MODULE_INIT:
	case DPUFB_EFFECT_MODULE_DEINIT:
	case DPUFB_EFFECT_INFO_GET:
	case DPUFB_EFFECT_INFO_SET:
		if (dpufd->display_effect_ioctl_handler != NULL)
			ret = dpufd->display_effect_ioctl_handler(dpufd, cmd, argp);
		break;

	default:
		break;
	}

	return ret;
}

static int dpu_fb_display_ioctl(struct dpu_fb_data_type *dpufd, unsigned int cmd,
	void __user *argp)
{
	int ret = -ENOSYS;

	switch (cmd) {
	case DPUFB_VSYNC_CTRL:
		if (dpufd->vsync_ctrl_fnc != NULL)
			ret = dpufd->vsync_ctrl_fnc(dpufd, argp);
		break;
	case DPUFB_MMBUF_SIZE_QUERY:
		ret = dpu_mmbuf_reserved_size_query(dpufd, argp);
		break;
	case DPUFB_DSS_MMBUF_ALLOC:
		ret = dpu_mmbuf_request(dpufd, argp);
		break;
	case DPUFB_DSS_MMBUF_FREE:
		ret = dpu_mmbuf_release(dpufd, argp);
		break;
	case DPUFB_DSS_MMBUF_FREE_ALL:
		ret = dpu_mmbuf_free_all(dpufd, argp);
		break;
	default:
		if (dpufd->ov_ioctl_handler != NULL)
			ret = dpufd->ov_ioctl_handler(dpufd, cmd, argp);
		break;
	}

	return ret;
}

static int dpu_fb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
	int ret = -1;
	struct dpu_fb_data_type *dpufd = NULL;
	void __user *argp = (void __user *)(uintptr_t)arg;

	dpu_check_and_return(!info, -EINVAL, ERR, "info is NULL!\n");

	dpufd = (struct dpu_fb_data_type *)info->par;
	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL!\n");

	ret = dpu_fb_resource_ioctl(dpufd, cmd, argp);
	if (ret != -ENOSYS)
		return ret;

	ret = dpu_fb_effect_ioctl(dpufd, cmd, argp);
	if (ret != -ENOSYS)
		return ret;

	ret = dpu_fb_display_ioctl(dpufd, cmd, argp);
	if (ret != -ENOSYS)
		return ret;
	else
		DPU_FB_ERR("unsupported ioctl [%x]\n", cmd);

	return ret;
}

static struct fb_ops dpu_fb_ops = {
	.owner = THIS_MODULE,
	.fb_open = dpu_fb_open,
	.fb_release = dpu_fb_release,
	.fb_blank = dpu_fb_blank,
	.fb_ioctl = dpu_fb_ioctl,
	.fb_compat_ioctl = dpu_fb_ioctl,
	.fb_mmap = NULL,
};

static void dpufb_check_dummy_pixel_num(struct dpu_panel_info *pinfo)
{
	if (pinfo->dummy_pixel_num % 2 != 0) {
		DPU_FB_INFO("dummy_pixel_num should be even, so plus 1 !\n");
		pinfo->dummy_pixel_num += 1;
	}

	if (pinfo->dummy_pixel_num >= pinfo->xres) {
		DPU_FB_INFO("dummy_pixel_num is invalid, force set it to 0 !\n");
		pinfo->dummy_pixel_num = 0;
	}
}

void dpu_fb_pdp_fnc_register(struct dpu_fb_data_type *dpufd)
{
	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is NULL\n");

	dpufd->fb_mem_free_flag = false;

	if (g_fastboot_enable_flag == 1) {
		dpufd->set_fastboot_fnc = dpu_fb_set_fastboot_needed;
		g_fastboot_set_needed = 1;
	} else {
		dpufd->set_fastboot_fnc = NULL;
	}

	dpufd->open_sub_fnc = dpu_fb_open_sub;
	dpufd->release_sub_fnc = dpu_fb_release_sub;
	dpufd->hpd_open_sub_fnc = NULL;
	dpufd->hpd_release_sub_fnc = NULL;
	dpufd->lp_fnc = dpufb_ctrl_lp;
	dpufd->esd_fnc = dpufb_ctrl_esd;
	dpufd->fps_upt_isr_handler = dpufb_fps_upt_isr_handler;
	dpufd->mipi_dsi_bit_clk_upt_isr_handler = mipi_dsi_bit_clk_upt_isr_handler;
	dpufd->panel_mode_switch_isr_handler = NULL;

	dpufd->sysfs_attrs_add_fnc = NULL;

	dpufd->sysfs_attrs_append_fnc = dpufb_sysfs_attrs_append;
	DPU_FB_DEBUG("fb%d regist attr append fnc\n", dpufd->index);
	dpufd->sysfs_create_fnc = dpufb_sysfs_create;
	dpufd->sysfs_remove_fnc = dpufb_sysfs_remove;

	dpufd->bl_register = dpufb_backlight_register;
	dpufd->bl_unregister = dpufb_backlight_unregister;
	dpufd->bl_update = dpufb_backlight_update;
	dpufd->bl_cancel = dpufb_backlight_cancel;
	dpufd->vsync_register = dpufb_vsync_register;
	dpufd->vsync_unregister = dpufb_vsync_unregister;
	dpufd->vactive_start_register = dpufb_vactive_start_register;
	dpufd->vsync_ctrl_fnc = dpufb_vsync_ctrl;
	dpufd->vsync_isr_handler = dpufb_vsync_isr_handler;

	dpu_fb_pm_runtime_register(dpufd);
}

static int dpu_fbi_info_init(struct dpu_fb_data_type *dpufd, struct fb_info *fbi)
{
	if (!dpu_fb_img_type_valid(dpufd->fb_imgType)) {
		DPU_FB_ERR("fb%d, unkown image type!\n", dpufd->index);
		return -EINVAL;
	}

	fbi->fbops = &dpu_fb_ops;

	return 0;
}

static int dpu_fb_register(struct dpu_fb_data_type *dpufd)
{
	struct dpu_panel_info *panel_info = NULL;
	struct fb_info *fbi = NULL;

	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL\n");

	fbi = dpufd->fbi;
	panel_info = &dpufd->panel_info;

	dpufb_check_dummy_pixel_num(panel_info);

	if (dpu_fbi_info_init(dpufd, fbi) < 0)
		return -EINVAL;

	dpu_fb_data_init(dpufd);

	atomic_set(&(dpufd->atomic_v), 0);

	dpu_fb_init_sema(dpufd);

	dpu_fb_init_spin_lock(dpufd);

	dpufb_sysfs_init(dpufd);

	if (dpu_fb_registe_callback(dpufd) < 0)
		return -EINVAL;

	if (dpu_overlay_init(dpufd))
		return -EPERM;

	dpu_display_effect_init(dpufd);

	if (register_framebuffer(fbi) < 0) {
		DPU_FB_ERR("fb%d failed to register_framebuffer!\n", dpufd->index);
		return -EPERM;
	}

	if (dpufd->sysfs_attrs_add_fnc != NULL)
		dpufd->sysfs_attrs_add_fnc(dpufd);

	dpu_fb_common_register(dpufd);

	register_disp_merger_mgr(dpufd);

	if (dpufd->merger_mgr.ops && dpufd->merger_mgr.ops->init)
		dpu_check_and_return(dpufd->merger_mgr.ops->init(&dpufd->merger_mgr), -1, ERR, "merge mgr init failed\n");

	init_disp_recorder(&dpufd->disp_recorder, (void *)dpufd);

	DPU_FB_INFO("FrameBuffer[%d] is registered successfully!\n", dpufd->index);
	return 0;
}

/*******************************************************************************
 *
 */
static int dpu_fb_get_regulator_resource(struct platform_device *pdev)
{
	int ret;

	g_dpe_regulator[0].supply = REGULATOR_PDP_NAME;
	g_dpe_regulator[1].supply = REGULATOR_SMMU_TCU_NAME;
	g_dpe_regulator[2].supply = REGULATOR_MEDIA_NAME;
	ret = devm_regulator_bulk_get(&(pdev->dev),
		ARRAY_SIZE(g_dpe_regulator), g_dpe_regulator);

	return ret;
}

static int dpu_fb_init_resource(struct platform_device *pdev)
{
	int ret;
	struct device_node *np = NULL;
	struct device *dev = NULL;

	dev = &pdev->dev;

	dev_dbg(dev, "initialized=%d, +\n", dpu_fb_resource_initialized);

	dpu_mmbuf_info_init();

	pdev->id = 0;

	np = of_find_compatible_node(NULL, NULL, DTS_COMP_FB_NAME);
	dpu_check_and_return(!np, -ENXIO, ERR, "NOT FOUND device node %s!\n", DTS_COMP_FB_NAME);

	ret = dpu_fb_read_property_from_dts(np, dev);
	dev_check_and_return(dev, ret, ret, err, "fail to read property from dts!\n");

	ret = dpu_fb_get_irq_no_from_dts(np, dev);
	dev_check_and_return(dev, ret, ret, err, "fail to get irq from dts!\n");

	ret = dpu_fb_get_baseaddr_from_dts(np, dev);
	dev_check_and_return(dev, ret, ret, err, "fail to get base addr from dts!\n");

	ret = dpu_fb_get_regulator_resource(pdev);
	dev_check_and_return(dev, ret, -ENXIO, err, "failed to get regulator resource! ret = %d\n", ret);

	ret = dpu_fb_get_clk_name_from_dts(np, dev);
	dev_check_and_return(dev, ret, ret, err, "fail to get clk name from dts!\n");

	ret = dpu_fb_get_iommu_info_from_dts(dev);
	dev_check_and_return(dev, ret, -ENXIO, err, "failed to get iommu info from dts\n");

	ret = dpu_iommu_enable(pdev);
	dev_check_and_return(dev, ret, -ENXIO, err, "failed to dpu_iommu_enable! ret = %d\n", ret);

	sema_init(&g_dpufb_dss_clk_vote_sem, 1);

	dpu_fb_create_cdev();

	dpu_fb_resource_initialized = 1;

	dpu_fb_device_set_status0(DTS_FB_RESOURCE_INIT_READY);

	dev_dbg(dev, "initialized=%d, -\n", dpu_fb_resource_initialized);

	return ret;
}

static int dpu_fb_probe(struct platform_device *pdev)
{
	int ret;
	struct dpu_fb_data_type *dpufd = NULL;
	struct device *dev = NULL;

	dpu_check_and_return(!pdev, -1, ERR, "pdev is NULL!\n");
	dev = &pdev->dev;

	if (dpu_fb_resource_initialized == 0) {
		ret = dpu_fb_init_resource(pdev);
		return ret;
	}

	if (pdev->id < 0) {
		dev_err(dev, "WARNING: id=%d, name=%s!\n", pdev->id, pdev->name);
		return 0;
	}

	if (dpu_fb_resource_initialized == 0) {
		dev_err(dev, "fb resource not initialized!\n");
		return -EPERM;
	}

	if (pdev_list_cnt >= DPU_FB_MAX_DEV_LIST) {
		dev_err(dev, "too many fb devices, num=%d!\n", pdev_list_cnt);
		return -ENOMEM;
	}

	dpufd = platform_get_drvdata(pdev);
	dev_check_and_return(dev, !dpufd, -EINVAL, err, "dpufd is NULL\n");

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);

	ret = dpu_fb_register(dpufd);
	if (ret) {
		dev_err(dev, "fb%d dpu_fb_register failed, error=%d!\n", dpufd->index, ret);
		return ret;
	}

	/* config earlysuspend */
#ifdef CONFIG_HAS_EARLYSUSPEND
	dpufd->early_suspend.suspend = dpufb_early_suspend;
	dpufd->early_suspend.resume = dpufb_early_resume;
	dpufd->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB - 2;
	register_early_suspend(&dpufd->early_suspend);
#endif

	pdev_list[pdev_list_cnt++] = pdev;

	/* set device probe status */
	dpu_fb_device_set_status1(dpufd);

	ret = create_isr_query_dev();
	if (ret != 0) {
		DPU_FB_ERR("fb%u fail to create isr_query dev\n", dpufd->index);
		return -1;
	}

	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return 0;
}

static int dpu_fb_remove(struct platform_device *pdev)
{
	struct dpu_fb_data_type *dpufd = NULL;

	if (!pdev) {
		DPU_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	dpufd = platform_get_drvdata(pdev);
	if (!dpufd) {
		DPU_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d, +\n", dpufd->index);

	/* pm_runtime unregister */
	if (dpufd->pm_runtime_unregister != NULL)
		dpufd->pm_runtime_unregister(pdev);

	/* stop the device */
	if (dpu_fb_suspend_sub(dpufd) != 0)
		DPU_FB_ERR("fb%d dpu_fb_suspend_sub failed!\n", dpufd->index);

	/* overlay destroy */
	dpu_overlay_deinit(dpufd);

	if (dpufd->merger_mgr.ops && dpufd->merger_mgr.ops->deinit)
		dpufd->merger_mgr.ops->deinit(&dpufd->merger_mgr);

	/* remove /dev/fb* */
	unregister_framebuffer(dpufd->fbi);

	/* unregister buf_sync */
	if (dpufd->buf_sync_unregister != NULL)
		dpufd->buf_sync_unregister(pdev);
	/* unregister vsync */
	if (dpufd->vsync_unregister != NULL)
		dpufd->vsync_unregister(pdev);
	/* unregister backlight */
	if (dpufd->bl_unregister != NULL)
		dpufd->bl_unregister(pdev);
	/* fb sysfs remove */
	if (dpufd->sysfs_remove_fnc != NULL)
		dpufd->sysfs_remove_fnc(dpufd->pdev);

	DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

	return 0;
}

static int dpu_fb_suspend_sub(struct dpu_fb_data_type *dpufd)
{
	int ret;

	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	ret = dpu_fb_blank_sub(dpufd, FB_BLANK_POWERDOWN);
	if (ret) {
		DPU_FB_ERR("fb%d can't turn off display, error=%d!\n", dpufd->index, ret);
		return ret;
	}

	return 0;
}

static int dpu_fb_resume_sub(struct dpu_fb_data_type *dpufd)
{
	int ret;

	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	ret = dpu_fb_blank_sub(dpufd, FB_BLANK_UNBLANK);
	if (ret)
		DPU_FB_ERR("fb%d can't turn on display, error=%d!\n", dpufd->index, ret);

	return ret;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void dpufb_early_suspend(struct early_suspend *h)
{
	struct dpu_fb_data_type *dpufd = NULL;

	dpufd = container_of(h, struct dpu_fb_data_type, early_suspend);
	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL\n");
		return;
	}

	DPU_FB_INFO("fb%d, +\n", dpufd->index);

	dpu_fb_suspend_sub(dpufd);

	DPU_FB_INFO("fb%d, -\n", dpufd->index);
}

static void dpufb_early_resume(struct early_suspend *h)
{
	struct dpu_fb_data_type *dpufd = NULL;

	dpufd = container_of(h, struct dpu_fb_data_type, early_suspend);
	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL\n");
		return;
	}

	DPU_FB_INFO("fb%d, +\n", dpufd->index);

	dpu_fb_resume_sub(dpufd);

	DPU_FB_INFO("fb%d, -\n", dpufd->index);
}
#endif

#if defined(CONFIG_PM) && !defined(CONFIG_HAS_EARLYSUSPEND)
#else
#define dpu_fb_suspend NULL
#define dpu_fb_resume NULL
#endif

/*******************************************************************************
 * pm_runtime
 */
#ifdef CONFIG_PM_RUNTIME
static int dpu_fb_runtime_suspend(struct device *dev)
{
	struct dpu_fb_data_type *dpufd = NULL;
	int ret;

	if (!dev) {
		DPU_FB_ERR("NULL Poniter\n");
		return 0;
	}

	dpufd = dev_get_drvdata(dev);
	if (!dpufd) {
		DPU_FB_ERR("NULL Poniter\n");
		return 0;
	}

	DPU_FB_INFO("fb%d, +\n", dpufd->index);

	ret = dpu_fb_suspend_sub(dpufd);
	if (ret != 0)
		DPU_FB_ERR("fb%d, failed to dpu_fb_suspend_sub! ret=%d\n", dpufd->index, ret);

	dpufd->media_common_composer_sr_refcount = 0;
	DPU_FB_INFO("fb%d, -\n", dpufd->index);

	return 0;
}

static int dpu_fb_runtime_resume(struct device *dev)
{
	struct dpu_fb_data_type *dpufd = NULL;
	int ret;

	if (!dev) {
		DPU_FB_ERR("NULL Poniter\n");
		return 0;
	}

	dpufd = dev_get_drvdata(dev);
	if (!dpufd) {
		DPU_FB_ERR("NULL Poniter\n");
		return 0;
	}

	DPU_FB_INFO("fb%d, +\n", dpufd->index);

	ret = dpu_fb_resume_sub(dpufd);
	if (ret != 0)
		DPU_FB_ERR("fb%d, failed to dpu_fb_resume_sub! ret=%d\n", dpufd->index, ret);

	DPU_FB_INFO("fb%d, -\n", dpufd->index);

	return 0;
}

static int dpu_fb_runtime_idle(struct device *dev)
{
	struct dpu_fb_data_type *dpufd = NULL;

	if (!dev) {
		DPU_FB_ERR("NULL Poniter\n");
		return 0;
	}

	dpufd = dev_get_drvdata(dev);
	if (!dpufd) {
		DPU_FB_ERR("NULL Poniter\n");
		return 0;
	}

	return 0;
}

void dpufb_pm_runtime_get(struct dpu_fb_data_type *dpufd)
{
	int ret;

	if (!dpufd) {
		DPU_FB_ERR("NULL Pointer\n");
		return;
	}

	ret = pm_runtime_get_sync(dpufd->fbi->dev);
	if (ret < 0)
		DPU_FB_ERR("fb%d, failed to pm_runtime_get_sync! ret=%d\n", dpufd->index, ret);
}

void dpufb_pm_runtime_put(struct dpu_fb_data_type *dpufd)
{
	int ret;

	if (!dpufd) {
		DPU_FB_ERR("NULL Pointer\n");
		return;
	}

	ret = pm_runtime_put(dpufd->fbi->dev);
	if (ret < 0)
		DPU_FB_ERR("fb%d, failed to pm_runtime_put! ret=%d\n", dpufd->index, ret);
}

void dpufb_pm_runtime_register(struct platform_device *pdev)
{
	int ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (!pdev) {
		DPU_FB_ERR("NULL Pointer\n");
		return;
	}

	dpufd = platform_get_drvdata(pdev);
	if (!dpufd) {
		dev_err(&pdev->dev, "NULL Pointer\n");
		return;
	}

	ret = pm_runtime_set_active(dpufd->fbi->dev);
	if (ret < 0)
		dev_err(&pdev->dev, "fb%d failed to pm_runtime_set_active\n", dpufd->index);
	pm_runtime_enable(dpufd->fbi->dev);
}

void dpufb_pm_runtime_unregister(struct platform_device *pdev)
{
	struct dpu_fb_data_type *dpufd = NULL;

	if (!pdev) {
		DPU_FB_ERR("NULL Pointer\n");
		return;
	}

	dpufd = platform_get_drvdata(pdev);
	if (!dpufd) {
		dev_err(&pdev->dev, "NULL Pointer\n");
		return;
	}

	pm_runtime_disable(dpufd->fbi->dev);
}
#endif

#ifdef CONFIG_PM_SLEEP
static int dpu_fb_pm_suspend(struct device *dev)
{
	struct dpu_fb_data_type *dpufd = NULL;
	int ret;

	if (!dev) {
		DPU_FB_ERR("NULL Poniter\n");
		return 0;
	}

	dpufd = dev_get_drvdata(dev);
	if (!dpufd)
		return 0;

	if (dpufd->index == EXTERNAL_PANEL_IDX || dpufd->index == AUXILIARY_PANEL_IDX)
		return 0;

	DPU_FB_INFO("fb%d, +\n", dpufd->index);

	ret = dpu_fb_suspend_sub(dpufd);
	if (ret != 0)
		DPU_FB_ERR("fb%d, failed to dpu_fb_suspend_sub! ret=%d\n", dpufd->index, ret);

	dpufd->media_common_composer_sr_refcount = 0;
	DPU_FB_INFO("fb%d, -\n", dpufd->index);

	return 0;
}


#endif

static void dpu_fb_shutdown(struct platform_device *pdev)
{
	int ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (!pdev) {
		DPU_FB_ERR("pdev NULL Pointer\n");
		return;
	}

	dpufd = platform_get_drvdata(pdev);
	if (!dpufd) {
		if (pdev->id)
			DPU_FB_ERR("dpufd NULL Pointer,pdev->id=%d\n", pdev->id);

		return;
	}

	if ((dpufd->index != PRIMARY_PANEL_IDX) && dpufd->index != EXTERNAL_PANEL_IDX) {
		DPU_FB_DEBUG("fb%d do not shutdown\n", dpufd->index);
		return;
	}

	DPU_FB_INFO("fb%d shutdown +\n", dpufd->index);
	dpufd->fb_shutdown = true;

	ret = dpu_fb_blank_sub(dpufd, FB_BLANK_POWERDOWN);
	if (ret)
		DPU_FB_ERR("fb%d can't turn off display, error=%d!\n", dpufd->index, ret);

	DPU_FB_INFO("fb%d shutdown -\n", dpufd->index);
}

static const struct dev_pm_ops dpu_fb_dev_pm_ops = {
#ifdef CONFIG_PM_RUNTIME
	.runtime_suspend = dpu_fb_runtime_suspend,
	.runtime_resume = dpu_fb_runtime_resume,
	.runtime_idle = dpu_fb_runtime_idle,
#endif
#ifdef CONFIG_PM_SLEEP
	.suspend = dpu_fb_pm_suspend,
	.resume = NULL,
#endif
};

static const struct of_device_id dpu_fb_match_table[] = {
	{
		.compatible = DTS_COMP_FB_NAME,
		.data = NULL,
	},
	{},
};
MODULE_DEVICE_TABLE(of, dpu_fb_match_table);

static struct platform_driver dpu_fb_driver = {
	.probe = dpu_fb_probe,
	.remove = dpu_fb_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend = NULL,
	.resume = NULL,
#endif
	.shutdown = dpu_fb_shutdown,
	.driver = {
		.name = DEV_NAME_FB,
		.owner  = THIS_MODULE,
		.of_match_table = of_match_ptr(dpu_fb_match_table),
		.pm = &dpu_fb_dev_pm_ops,
	},
};

static int __init dpu_fb_init(void)
{
	int ret;
	DPU_FB_INFO("+\n");

	ret = platform_driver_register(&dpu_fb_driver);
	if (ret) {
		DPU_FB_ERR("platform_driver_register failed, error=%d!\n", ret);
		return ret;
	}

	return ret;
}

module_init(dpu_fb_init);

MODULE_DESCRIPTION("Framebuffer Driver");
MODULE_LICENSE("GPL v2");
#pragma GCC diagnostic pop
