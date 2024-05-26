/* Copyright (c) Huawei Technologies Co., Ltd. 2013-2021. All rights reserved.
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

#include "dpu_fb.h"
#include "dpu_fb_dts.h"
#include "dpu_mmbuf_manager.h"
#if defined(CONFIG_HWLOG_KERNEL)
#include <huawei_platform/log/log_jank.h>
#endif
#include "dpu_display_effect.h"
#include <linux/module.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include <linux/device.h>
#if defined(CONFIG_DPU_FB_V320)
#include <platform_include/basicplatform/linux/power/hisi_hi6521_charger_power.h>
#endif
#ifdef CONFIG_DPU_DISPLAY_DFR
#include "dpu_frame_rate_ctrl.h"
#endif
#include "dpu_mipi_dsi.h"
#ifdef CONFIG_POWER_DUBAI_RGB_STATS
#include "product/rgb_stats/dpu_fb_rgb_stats.h"
#endif

#if defined(CONFIG_TEE_TUI)
#include "tui.h"
#endif

#if defined(CONFIG_LCDKIT_DRIVER)
#include "lcdkit_fb_util.h"
#endif

#ifdef CONFIG_LCD_KIT_DRIVER
#include "lcd_kit_core.h"
#endif

#if defined(CONFIG_VIDEO_IDLE)
#include "dpu_fb_video_idle.h"
#endif

#ifdef CONFIG_DPU_EFFECT_HIHDR
#include "dpu_hihdr.h"
#endif

#define set_flag_bits_if_cond(cond, flag, bits) \
	do { if (cond) (flag) |= (bits); } while (0)

uint8_t color_temp_cal_buf[32] = {0};

static int dpu_fb_resource_initialized;
static struct platform_device *pdev_list[DPU_FB_MAX_DEV_LIST] = {0};

static int pdev_list_cnt;
struct fb_info *fbi_list[DPU_FB_MAX_FBI_LIST] = {0};
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

#define MAX_DPE_NUM 3
static struct regulator_bulk_data g_dpe_regulator[MAX_DPE_NUM];

/*lint -e552*/
int g_primary_lcd_xres;
int g_primary_lcd_yres;
uint64_t g_pxl_clk_rate;
uint8_t g_prefix_ce_support;
uint8_t g_prefix_sharpness1d_support;
uint8_t g_prefix_sharpness2d_support;
/*lint +e552*/

int g_debug_enable_lcd_sleep_in;
uint32_t g_err_status;  //lint !e552

struct dpu_fb_data_type *g_dpufd_fb0;
struct fb_info *g_info_fb0;  //lint !e552

/******************************************************************************
 * FUNCTIONS PROTOTYPES
 */
static int dpu_fb_register(struct dpu_fb_data_type *dpufd);

static int dpu_fb_open(struct fb_info *info, int user);
static int dpu_fb_release(struct fb_info *info, int user);
static int dpu_fb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info);
static int dpu_fb_check_var(struct fb_var_screeninfo *var, struct fb_info *info);
static int dpu_fb_set_par(struct fb_info *info);
static int dpu_fb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg);
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
	fbi = framebuffer_alloc(sizeof(struct dpu_fb_data_type), NULL);
	if (!fbi) {
		DPU_FB_ERR("can't alloc framebuffer info data!\n");
		platform_device_put(this_dev);
		return NULL;
	}

	dpufd = (struct dpu_fb_data_type *)fbi->par;
	memset(dpufd, 0, sizeof(*dpufd));
	dpufd->fbi = fbi;
	dpufd->fb_imgType = DPU_FB_PIXEL_FORMAT_BGRA_8888;
	dpufd->index = (uint32_t)fbi_list_index;

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
	fbi_list[fbi_list_index++] = fbi;

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

#ifdef CONFIG_DPU_FB_V501
static void dpufb_fb1_blank_sem0_lock(struct dpu_fb_data_type *dpufd)
{
	if (dpufd->index == PRIMARY_PANEL_IDX) {
		if (dpufd_list[EXTERNAL_PANEL_IDX])
			down(&dpufd_list[EXTERNAL_PANEL_IDX]->blank_sem0);
	}
}

static void dpufb_fb1_blank_sem0_unlock(struct dpu_fb_data_type *dpufd)
{
	if (dpufd->index == PRIMARY_PANEL_IDX) {
		if (dpufd_list[EXTERNAL_PANEL_IDX])
			up(&dpufd_list[EXTERNAL_PANEL_IDX]->blank_sem0);
	}
}
#endif

static void dpu_fb_displayeffect_update(struct dpu_fb_data_type *dpufd)
{
	int disp_panel_id = dpufd->panel_info.disp_panel_id;

	if (dpufd->index == AUXILIARY_PANEL_IDX)
		return;

	DPU_FB_INFO("[effect]+\n");

#if defined(CONFIG_DPU_FB_V410)
	dpufd->effect_updated_flag[disp_panel_id].gmp_effect_updated = true;
	dpufd->effect_updated_flag[disp_panel_id].igm_effect_updated = true;
	dpufd->effect_updated_flag[disp_panel_id].xcc_effect_updated = true;
	dpufd->effect_updated_flag[disp_panel_id].gamma_effect_updated = true;
	dpufd->effect_updated_flag[disp_panel_id].acm_effect_updated = true;
	dpufd->effect_updated_flag[disp_panel_id].hiace_effect_updated = true;
	dpufd->hiace_info[dpufd->panel_info.disp_panel_id].algorithm_result = 0;
#elif defined(CONFIG_DPU_FB_V501) || defined(CONFIG_DPU_FB_V330)
	if (!(dpufd->effect_gmp_copy_status & BIT(0)))
		dpufd->effect_updated_flag[disp_panel_id].gmp_effect_updated = true;
	dpufd->effect_updated_flag[disp_panel_id].igm_effect_updated = true;
	dpufd->effect_updated_flag[disp_panel_id].xcc_effect_updated = true;
	dpufd->effect_updated_flag[disp_panel_id].gamma_effect_updated = true;
	dpufd->effect_updated_flag[disp_panel_id].hiace_effect_updated = true;
	dpufd->hiace_info[dpufd->panel_info.disp_panel_id].algorithm_result = 0;
	DPU_FB_INFO("[effect]copy_status=%d\n", dpufd->effect_gmp_copy_status);
	dpufd->effect_gmp_copy_status = 0x0;
#elif defined(CONFIG_DPU_FB_V320)
	dpufd->effect_updated_flag[disp_panel_id].gmp_effect_updated = true;
	dpufd->effect_updated_flag[disp_panel_id].igm_effect_updated = true;
	dpufd->effect_updated_flag[disp_panel_id].xcc_effect_updated = true;
	dpufd->effect_updated_flag[disp_panel_id].gamma_effect_updated = true;
	dpufd->effect_updated_flag[disp_panel_id].acm_effect_updated = true;
#else
	dpufd->effect_updated_flag[disp_panel_id].post_xcc_effect_updated = true;
	dpufd->effect_updated_flag[disp_panel_id].hiace_effect_updated = true;
	dpufd->hiace_info[dpufd->panel_info.disp_panel_id].algorithm_result = 0;
#endif
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

#if defined(CONFIG_FOLD_DISPLAY)
static void dpufb_config_panel_power_on_status(struct dpu_fb_data_type *dpufd)
{
	if (dpufd->panel_info.disp_panel_id == DISPLAY_INNEL_PANEL_ID)
		dpufd->panel_power_status |= EN_INNER_PANEL_ON;

	if (dpufd->panel_info.disp_panel_id == DISPLAY_OUTER_PANEL_ID)
		dpufd->panel_power_status |= EN_OUTER_PANEL_ON;
}

static void dpufb_config_panel_power_off_status(struct dpu_fb_data_type *dpufd)
{
	if (dpufd->panel_power_status == EN_INNER_OUTER_PANEL_ON) {
		if (dpufd->panel_info.disp_panel_id == DISPLAY_INNEL_PANEL_ID)
			dpufd->panel_power_status = EN_OUTER_PANEL_ON;
		if (dpufd->panel_info.disp_panel_id == DISPLAY_OUTER_PANEL_ID)
			dpufd->panel_power_status = EN_INNER_PANEL_ON;
	} else {
		dpufd->panel_power_status = EN_INNER_OUTER_PANEL_OFF;
	}
}

void dpufb_set_panel_power_status(struct dpu_fb_data_type *dpufd,
	bool power_on)
{
	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL\n");
		return;
	}

	if (dpufd->index != PRIMARY_PANEL_IDX)
		return;

	if (dpu_check_panel_product_type(dpufd->panel_info.product_type)) {
		if (power_on)
			dpufb_config_panel_power_on_status(dpufd);
		else
			dpufb_config_panel_power_off_status(dpufd);

		DPU_FB_INFO("[fold]disp_panel_id = %d, panel_power_status = %d\n",
			dpufd->panel_info.disp_panel_id, dpufd->panel_power_status);
	}
}
#endif

int dpu_fb_blank_sub(int blank_mode, struct fb_info *info)
{
	struct dpu_fb_data_type *dpufd = NULL;
	int ret = 0;

	dpu_check_and_return(!info, -EINVAL, ERR, "info is NULL\n");

	dpufd = (struct dpu_fb_data_type *)info->par;
	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL\n");

	DPU_FB_DEBUG("[fb%d]blank_mode = %d, panel_power_on = %d\n", dpufd->index, blank_mode, (int)dpufd->panel_power_on);
	down(&dpufd->blank_sem);
	down(&dpufd->blank_sem0);
	dpu_fb_down_effect_sem(dpufd);
	switch (blank_mode) {
	case FB_BLANK_UNBLANK:
		dpu_fb_displayeffect_update(dpufd);
#ifdef CONFIG_DPU_FB_V501
		dpufb_fb1_blank_sem0_lock(dpufd);
#endif
		if (!dpufd->panel_power_on) {
			ret = dpu_fb_blank_panel_power_on(dpufd);
#if defined(CONFIG_FOLD_DISPLAY)
			dpufb_set_panel_power_status(dpufd, true);
#endif
		}
#ifdef CONFIG_DPU_FB_V501
		dpufb_fb1_blank_sem0_unlock(dpufd);
#endif
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

#ifdef CONFIG_DPU_DISPLAY_DFR
	if (blank_mode == FB_BLANK_UNBLANK)
		dfr_power_on_notification(dpufd);
#endif

#ifdef CONFIG_DPU_FB_ALSC
	dpu_alsc_blank(dpufd, blank_mode);
#endif
	DPU_FB_DEBUG("[fb%d]-\n", dpufd->index);
	return ret;
}

bool dpu_fb_set_fastboot_needed(struct fb_info *info)
{
	struct dpu_fb_data_type *dpufd = NULL;

	if (!info) {
		DPU_FB_ERR("info is NULL");
		return false;
	}
	dpufd = (struct dpu_fb_data_type *)info->par;
	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL");
		return false;
	}

	if (g_fastboot_set_needed > 0) {
		dpufb_ctrl_fastboot(dpufd);

		dpufd->panel_power_on = true;
		if (info->screen_base && (info->fix.smem_len > 0))
			memset(info->screen_base, 0x0, info->fix.smem_len);

		g_fastboot_set_needed--;
		return true;
	}

	return false;
}

int dpu_fb_open_sub(struct fb_info *info)
{
	struct dpu_fb_data_type *dpufd = NULL;
	int ret = 0;
	bool needed = false;

	if (!info) {
		DPU_FB_ERR("info is NULL");
		return -EINVAL;
	}

	dpufd = (struct dpu_fb_data_type *)info->par;
	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL");
		return -EINVAL;
	}

	DPU_FB_DEBUG("fb%d +!\n", dpufd->index);
	if (dpufd->index == EXTERNAL_PANEL_IDX && !dpufd->panel_info.fake_external) {
		if (dpu_cmdlist_init(dpufd)) {
			DPU_FB_ERR("fb%d dpu_cmdlist_init failed!\n", dpufd->index);
			return -EINVAL;
		}
	}

	if (dpufd->set_fastboot_fnc != NULL)
		needed = dpufd->set_fastboot_fnc(info);

	if (!needed) {
		ret = dpu_fb_blank_sub(FB_BLANK_UNBLANK, info);
		if (ret != 0) {
			if (dpufd->index == EXTERNAL_PANEL_IDX && !dpufd->panel_info.fake_external)
				dpu_cmdlist_deinit(dpufd);
			DPU_FB_ERR("can't turn on display!\n");
			return ret;
		}
	}

	return 0;
}

int dpu_fb_release_sub(struct fb_info *info)
{
	int ret;
	struct dpu_fb_data_type *dpufd = NULL;

	if (!info) {
		DPU_FB_ERR("info is NULL\n");
		return -EINVAL;
	}

	dpufd = (struct dpu_fb_data_type *)info->par;
	if (!dpufd) {
		DPU_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	ret = dpu_fb_blank_sub(FB_BLANK_POWERDOWN, info);
	if (ret != 0) {
		DPU_FB_ERR("can't turn off display!\n");
		return ret;
	}

	if (dpufd->index == EXTERNAL_PANEL_IDX && !dpufd->panel_info.fake_external)
		dpu_cmdlist_deinit(dpufd);

	return 0;
}

uint32_t dpu_get_panel_product_type(void)
{
	struct dpu_fb_data_type *dpufd = g_dpufd_fb0;

	return dpufd->panel_info.product_type;
}

bool dpu_check_panel_product_type(uint32_t product_type)
{
#if defined(CONFIG_FOLD_DISPLAY)
	if (product_type & PANEL_SUPPORT_TWO_PANEL_DISPLAY_TYPE)
		return true;
	return false;
#else
	void_unused(product_type);
	return false;
#endif
}

bool dpu_check_dual_lcd_support(uint32_t dual_lcd_support)
{
	if (dual_lcd_support != 0)
		return true;
	return false;
}

uint8_t dpu_check_panel_feature_support(struct dpu_panel_info *panel_info)
{
#if defined(CONFIG_FOLD_DISPLAY)
	if (!panel_info) {
		DPU_FB_ERR("panel_info is NULL\n");
		return 0;
	}
	return panel_info->cascadeic_support;
#else
	void_unused(panel_info);
	return 0;
#endif
}

void dpu_fb_frame_refresh(struct dpu_fb_data_type *dpufd, char *trigger)
{
	char *envp[2];  /* uevent num for environment pointer */
	char buf[64] = {0};  /* buf len for fb_frame_refresh flag */

	snprintf(buf, sizeof(buf), "Refresh=1");
	envp[0] = buf;
	envp[1] = NULL;

	if (!dpufd || !trigger) {
		DPU_FB_ERR("NULL Pointer\n");
		return;
	}

	kobject_uevent_env(&(dpufd->fbi->dev->kobj), KOBJ_CHANGE, envp);
	DPU_FB_INFO("fb%d, %s frame refresh\n", dpufd->index, trigger);
}

#if defined(CONFIG_DPU_FB_AOD)
void dpu_aod_dec_atomic(struct dpu_fb_data_type *dpufd)
{
	int temp;

	if (!dpufd)
		return;

	temp = atomic_read(&(dpufd->atomic_v));
	DPU_FB_INFO("atomic_v = %d\n", temp);
	if (temp <= 0)
		return;

	atomic_dec(&(dpufd->atomic_v));
}

int dpu_aod_inc_atomic(struct dpu_fb_data_type *dpufd)
{
	int temp;

	if (!dpufd)
		return 0;

	temp = atomic_inc_return(&(dpufd->atomic_v));
	DPU_FB_INFO("atomic_v increased to %d\n", temp);
	if (temp == 1)
		return 1;

	DPU_FB_INFO("no need reget dss, atomic_v = %d\n", temp);
	dpu_aod_dec_atomic(dpufd);

	return 0;
}

void dpu_aod_schedule_wq(void)
{
	struct dpu_fb_data_type *dpufd = g_dpufd_fb0;
	if (!dpufd) {
		DPU_FB_ERR("NULL Pointer\n");
		return;
	}

#if defined(CONFIG_FOLD_DISPLAY)
	if (dpu_check_panel_product_type(dpufd->panel_info.product_type)) {
		if (dpufd->enable_fast_unblank_for_multi_panel == false) {
			DPU_FB_INFO("[fold] power up with fb blank!\n");
			dpufd->enable_fast_unblank = false;
			return;
		}
	}
#endif

	dpufd->enable_fast_unblank = true;
	queue_work(dpufd->aod_ud_fast_unblank_workqueue, &dpufd->aod_ud_fast_unblank_work);
}

static void ramless_aod_wait_for_offline_blank(struct dpu_fb_data_type *dpufd)
{
	int try_times = 0;

	if (!is_mipi_video_panel(dpufd))
		return;

	if (dpufd_list[AUXILIARY_PANEL_IDX] == NULL)
		return;

	while (dpufd_list[AUXILIARY_PANEL_IDX]->panel_power_on) {
		mdelay(1);
		if (++try_times > 1000) {  /* wait times */
			DPU_FB_ERR("wait for offline blank timeout!\n");
			break;
		}
	}
	DPU_FB_DEBUG(" %d ms\n", try_times);
}

static void aod_wait_for_offline_blank(struct dpu_fb_data_type *dpufd, int blank_mode)
{
	if (dpufd->index != PRIMARY_PANEL_IDX)
		return;

	if (blank_mode != FB_BLANK_UNBLANK)
		return;

	/* for ramless aod panel, now just dpuv330 platform aqm product use */
	if (!dpufd_list[PRIMARY_PANEL_IDX]->panel_info.ramless_aod)
		return;
	ramless_aod_wait_for_offline_blank(dpufd);
}

static void aod_wait_for_blank(struct dpu_fb_data_type *dpufd, int blank_mode)
{
	uint32_t try_times;

	aod_wait_for_offline_blank(dpufd, blank_mode);

	if (blank_mode == FB_BLANK_UNBLANK) {
		if ((dpufd->panel_power_on) && (dpufd->secure_ctrl.secure_blank_flag)) {
			/* wait for blank */
			DPU_FB_INFO("wait for tui blank!\n");
			while (dpufd->panel_power_on)
				mdelay(1);
		}
		while (dpufd->enable_fast_unblank)
			mdelay(1);  /* delay 1ms */
	}

	if ((blank_mode == FB_BLANK_POWERDOWN) && (dpufd->enable_fast_unblank)) {
		DPU_FB_INFO("need to wait for aod fast unblank wq end!\n");
		try_times = 0;
		while (dpufd->enable_fast_unblank) {
			mdelay(1);
			if (++try_times > 1000) {  /* wait times */
				DPU_FB_ERR("wait for aod fast unblank wq end timeout!\n");
				break;
			}
		}
	}
}

static void restore_fast_unblank_status(struct dpu_fb_data_type *dpufd)
{
	if (dpufd->index != PRIMARY_PANEL_IDX)
		return;
#if defined(CONFIG_FOLD_DISPLAY)
	dpufd->enable_fast_unblank_for_multi_panel = false;
#endif
}

static bool dpu_fb_need_fast_unblank(struct dpu_fb_data_type *dpufd)
{
	if (dpufd->index != PRIMARY_PANEL_IDX)
		return false;

	DPU_FB_INFO("+\n");
	if (dpu_aod_need_fast_unblank()) {
#if defined(CONFIG_FOLD_DISPLAY)
		dpufd->enable_fast_unblank_for_multi_panel = true;
#endif
		return true;
	}

	DPU_FB_INFO("-\n");
	return false;
}

static void dpu_create_aod_wq(struct dpu_fb_data_type *dpufd)
{
	if (!dpufd) {
		DPU_FB_ERR("dpufd NULL Pointer!\n");
		return;
	}
	/* creat aod workqueue */
	if (dpufd->index == PRIMARY_PANEL_IDX) {
		dpufd->aod_ud_fast_unblank_workqueue = alloc_ordered_workqueue("aod_ud_fast_unblank", WQ_HIGHPRI | WQ_MEM_RECLAIM);
		if (dpufd->aod_ud_fast_unblank_workqueue == NULL) {
			DPU_FB_ERR("creat aod work queue failed!\n");
			return;
		}
		INIT_WORK(&dpufd->aod_ud_fast_unblank_work, dpu_fb_unblank_wq_handle);

		g_dpufd_fb0 = dpufd;
	}
}

#endif

static int dpu_fb_blank(int blank_mode, struct fb_info *info)
{
	int ret;
	struct dpu_fb_data_type *dpufd = NULL;
#if defined(CONFIG_DPU_FB_AOD)
	bool sensorhub_aod_hwlock_succ = false;
#endif

	dpu_check_and_return(!info, -EINVAL, ERR, "info is NULL\n");

	dpufd = (struct dpu_fb_data_type *)info->par;
	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL\n");

	dpu_check_and_return((dpufd->panel_info.fake_external && (dpufd->index == EXTERNAL_PANEL_IDX)),
		-EINVAL, INFO, "it is fake, blank it fail\n");

	if (dpufd->index == PRIMARY_PANEL_IDX)
		g_info_fb0 = info;

	dpu_log_enable_if((dpufd->index != AUXILIARY_PANEL_IDX), "fb%d, blank_mode[%d] +\n",
		dpufd->index, blank_mode);

#if defined(CONFIG_DPU_FB_AOD)
	sensorhub_aod_hwlock_succ = dpu_fb_request_aod_hw_lock(dpufd);

	if (blank_mode == FB_BLANK_UNBLANK) {
		dpu_check_and_return((dpu_fb_need_fast_unblank(dpufd)), 0, INFO, "[aod] power up with fast unblank\n");
		wait_for_aod_stop(dpufd);
		ret = dpu_fb_aod_blank(dpufd, blank_mode);
		dpu_check_and_return((ret == -3), 0, INFO, "AOD in fast unlock mode\n");
	}

	aod_wait_for_blank(dpufd, blank_mode);

	if (blank_mode == FB_BLANK_POWERDOWN)
		down(&dpufd->blank_aod_sem);
#endif

	ret = dpu_fb_blank_device(dpufd, blank_mode, info);
	if (ret != 0)
		goto hw_unlock;

	dpu_log_enable_if((dpufd->index != AUXILIARY_PANEL_IDX), "fb%d, blank_mode[%d] -\n",
		dpufd->index, blank_mode);

	g_err_status = 0;

#if defined(CONFIG_DPU_FB_AOD)
	if (blank_mode == FB_BLANK_POWERDOWN) {
		(void)dpu_fb_aod_blank(dpufd, blank_mode);
		dpu_fb_release_aod_hw_lock(dpufd, sensorhub_aod_hwlock_succ);
		restore_fast_unblank_status(dpufd);
		up(&dpufd->blank_aod_sem);
	} else {
		dpu_fb_release_aod_hw_lock(dpufd, sensorhub_aod_hwlock_succ);
	}
#endif

	return 0;

hw_unlock:

#if defined(CONFIG_DPU_FB_AOD)
	dpu_fb_release_aod_hw_lock(dpufd, sensorhub_aod_hwlock_succ);
	if (blank_mode == FB_BLANK_POWERDOWN)
		up(&dpufd->blank_aod_sem);
#endif

	return ret;
}

static int dpu_fb_open(struct fb_info *info, int user)
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

	if (dpufd->panel_info.fake_external && (dpufd->index == EXTERNAL_PANEL_IDX)) {
		DPU_FB_INFO("fb%d, is fake, open it fail\n", dpufd->index);
		return -EINVAL;
	}

	if (!dpufd->ref_cnt) {
		DPU_FB_DEBUG("fb%d, +!\n", dpufd->index);
		if (dpufd->open_sub_fnc != NULL) {
#if defined(CONFIG_HWLOG_KERNEL)
			LOG_JANK_D(JLID_KERNEL_LCD_OPEN, "%s", "JL_KERNEL_LCD_OPEN 3650");
#endif
			ret = dpufd->open_sub_fnc(info);
		}
		DPU_FB_DEBUG("fb%d, -! ret = %d\n", dpufd->index, ret);
	}

	dpufd->ref_cnt++;

	return ret;
}

static int dpu_fb_release(struct fb_info *info, int user)
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

	if (dpufd->panel_info.fake_external && (dpufd->index == EXTERNAL_PANEL_IDX)) {
		DPU_FB_INFO("fb%d, is fake, release it fail\n", dpufd->index);
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
			ret = dpufd->release_sub_fnc(info);

		DPU_FB_DEBUG("fb%d, -\n", dpufd->index);

		if (dpufd->index == PRIMARY_PANEL_IDX ||
			(dpu_check_dual_lcd_support(dpufd->panel_info.dual_lcd_support)
			&& dpufd->index == EXTERNAL_PANEL_IDX)) {
			if (!dpufd->fb_mem_free_flag) {
				dpufb_free_fb_buffer(dpufd);
				dpufd->fb_mem_free_flag = true;
			}
#if defined(CONFIG_HUAWEI_DSM)
			if (lcd_dclient && !dsm_client_ocuppy(lcd_dclient)) {
				DPU_FB_INFO("fb%d, ref_cnt = %d\n", dpufd->index, dpufd->ref_cnt);
				dsm_client_record(lcd_dclient, "No fb0 device can use\n");
				dsm_client_notify(lcd_dclient, DSM_LCD_FB0_CLOSE_ERROR_NO);
			}
#endif
		}
	}

	return ret;
}

static int dpu_fb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	struct dpu_fb_data_type *dpufd = NULL;

	if (!info) {
		DPU_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	if (!var) {
		DPU_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	dpufd = (struct dpu_fb_data_type *)info->par;
	if (!dpufd) {
		DPU_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	if (var->rotate != FB_ROTATE_UR) {
		DPU_FB_ERR("error rotate %d!\n", var->rotate);
		return -EINVAL;
	}

	if (var->grayscale != info->var.grayscale) {
		DPU_FB_DEBUG("error grayscale %d!\n", var->grayscale);
		return -EINVAL;
	}

	if ((var->xres_virtual <= 0) || (var->yres_virtual <= 0)) {
		DPU_FB_ERR("xres_virtual=%d yres_virtual=%d out of range!\n",
			var->xres_virtual, var->yres_virtual);
		return -EINVAL;
	}

	if ((var->xres == 0) || (var->yres == 0)) {
		DPU_FB_ERR("xres=%d, yres=%d is invalid!\n", var->xres, var->yres);
		return -EINVAL;
	}

	if (var->xoffset > (var->xres_virtual - var->xres)) {
		DPU_FB_ERR("xoffset=%d(xres_virtual=%d, xres=%d) out of range!\n",
			var->xoffset, var->xres_virtual, var->xres);
		return -EINVAL;
	}

	if (var->yoffset > (var->yres_virtual - var->yres)) {
		DPU_FB_ERR("yoffset=%d(yres_virtual=%d, yres=%d) out of range!\n",
			var->yoffset, var->yres_virtual, var->yres);
		return -EINVAL;
	}

	return 0;
}

static int dpu_fb_set_par(struct fb_info *info)
{
	uint32_t xres;
	struct dpu_fb_data_type *dpufd = NULL;
	struct fb_var_screeninfo *var = NULL;

	if (!info) {
		DPU_FB_ERR("set par info NULL Pointer\n");
		return -EINVAL;
	}

	dpufd = (struct dpu_fb_data_type *)info->par;
	if (!dpufd) {
		DPU_FB_ERR("set par dpufd NULL Pointer\n");
		return -EINVAL;
	}

	var = &info->var;

	if (dpu_check_panel_product_type(dpufd->panel_info.product_type))
		xres = dpufd->panel_info.fb_xres;
	else
		xres = var->xres_virtual;

	dpufd->fbi->fix.line_length = dpufb_line_length(dpufd->index, xres,
		var->bits_per_pixel >> 3);

	return 0;
}

static int dpu_fb_pan_display(struct fb_var_screeninfo *var,
	struct fb_info *info)
{
	int ret = 0;
	struct dpu_fb_data_type *dpufd = NULL;

	if (!var || !info) {
		DPU_FB_ERR("pan display var or info NULL Pointer!\n");
		return -EINVAL;
	}

	dpufd = (struct dpu_fb_data_type *)info->par;
	if (!dpufd) {
		DPU_FB_ERR("pan display dpufd NULL Pointer!\n");
		return -EINVAL;
	}

	down(&dpufd->blank_sem);

	if (!dpufd->panel_power_on) {
		DPU_FB_INFO("fb%d, panel power off!\n", dpufd->index);
		goto err_out;
	}

	if (var->xoffset > (info->var.xres_virtual - info->var.xres))
		goto err_out;

	if (var->yoffset > (info->var.yres_virtual - info->var.yres))
		goto err_out;

	if (info->fix.xpanstep)
		info->var.xoffset =
		(var->xoffset / info->fix.xpanstep) * info->fix.xpanstep;

	if (info->fix.ypanstep)
		info->var.yoffset =
		(var->yoffset / info->fix.ypanstep) * info->fix.ypanstep;

	if (dpufd->pan_display_fnc != NULL)
		dpufd->pan_display_fnc(dpufd);
	else
		DPU_FB_ERR("fb%d pan_display_fnc not set!\n", dpufd->index);

	up(&dpufd->blank_sem);

	if (dpufd->bl_update != NULL)
		dpufd->bl_update(dpufd);

	return ret;

err_out:
	up(&dpufd->blank_sem);
	return 0;
}

static int dpufb_lcd_dirty_region_info_get(struct fb_info *info, void __user *argp)
{
	struct dpu_fb_data_type *dpufd = NULL;

	if (!info) {
		DPU_FB_ERR("dirty region info get info NULL Pointer!\n");
		return -EINVAL;
	}

	dpufd = (struct dpu_fb_data_type *)info->par;
	if (!dpufd) {
		DPU_FB_ERR("dirty region info get dpufdNULL Pointer!\n");
		return -EINVAL;
	}

	if (!argp) {
		DPU_FB_ERR("dirty region info get argp NULL Pointer!\n");
		return -EINVAL;
	}

	if (copy_to_user(argp, &(dpufd->panel_info.dirty_region_info),
		sizeof(struct lcd_dirty_region_info))) {
		DPU_FB_ERR("copy to user fail");
		return -EFAULT;
	}

	return 0;
}

static int dpufb_alsc_enable_info_get(struct fb_info *info, void __user *argp)
{
	struct dpu_fb_data_type *dpufd = NULL;
	struct alsc_enable_info enable_info;

	if (!info) {
		DPU_FB_ERR("info is NULL\n");
		return -EINVAL;
	}

	dpufd = (struct dpu_fb_data_type *)info->par;
	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL\n");
		return -EINVAL;
	}

	if (!argp) {
		DPU_FB_ERR("argp is NULL\n");
		return -EINVAL;
	}

	memset(&enable_info, 0, sizeof(enable_info));
#ifdef CONFIG_DPU_FB_ALSC
	enable_info.alsc_en = dpufd->alsc.alsc_en;
	enable_info.alsc_en &= (uint32_t)g_enable_alsc;
	enable_info.alsc_addr = dpufd->panel_info.dirty_region_info.alsc.alsc_addr;
	enable_info.alsc_size = dpufd->panel_info.dirty_region_info.alsc.alsc_size;
	enable_info.bl_update = dpufd->alsc.bl_update_to_hwc;
	DPU_FB_DEBUG("[ALSC]en addr size bl_update=[%u %u %u %u]\n",
		enable_info.alsc_en, enable_info.alsc_addr, enable_info.alsc_size, enable_info.bl_update);
	dpufd->alsc.bl_update_to_hwc = 0;
#endif

	if (copy_to_user(argp, &enable_info, sizeof(enable_info))) {
		DPU_FB_ERR("copy to user fail\n");
		return -EFAULT;
	}

	return 0;
}

static int dpufb_dirty_updt_support_get(struct fb_info *info, void __user *argp)
{
	struct dpu_fb_data_type *dpufd = NULL;
	uint32_t flag = 0;

	if (!info) {
		DPU_FB_ERR("dirty region updt set info NULL Pointer!\n");
		return -EINVAL;
	}

	dpufd = (struct dpu_fb_data_type *)info->par;
	if (!dpufd) {
		DPU_FB_ERR("dirty region updt set dpufd NULL Pointer!\n");
		return -EINVAL;
	}

	if (dpufd->index != PRIMARY_PANEL_IDX) {
		DPU_FB_ERR("fb%d, not supported!\n", dpufd->index);
		return -EINVAL;
	}

	if (!argp) {
		DPU_FB_ERR("dirty region updt set argp NULL Pointer!\n");
		return -EINVAL;
	}

	dpufd->dirty_region_updt_enable = 0;

	/* check partial update diable reason and set bits */
	set_flag_bits_if_cond(!g_enable_dirty_region_updt, flag, DEBUG_DIRTY_UPDT_DISABLE);
	set_flag_bits_if_cond(!dpufd->panel_info.dirty_region_updt_support, flag, LCD_CTRL_DIRTY_UPDT_DISABLE);
	set_flag_bits_if_cond(dpufd->color_temperature_flag, flag, COLOR_TEMPERATURE_DIRTY_UPDT_DISABLE);
	set_flag_bits_if_cond(dpufd->display_effect_flag, flag, DISPLAY_EFFECT_DIRTY_UPDT_DISABLE);
	set_flag_bits_if_cond(dpufd->esd_happened, flag, ESD_HAPPENED_DIRTY_UPDT_DISABLE);
	set_flag_bits_if_cond(dpufd->secure_ctrl.secure_event != DSS_SEC_DISABLE, flag,
		SECURE_ENABLED_DIRTY_UPDT_DISABLE);
#if defined(CONFIG_DPU_FB_AOD) || defined(CONFIG_DPU_FB_AP_AOD)
	set_flag_bits_if_cond(dpufd->aod_mode, flag, AOD_ENABLED_DIRTY_UPDT_DISABLE);
#endif
	set_flag_bits_if_cond(dpufd->vr_mode, flag, VR_ENABLED_DIRTY_UPDT_DISABLE);
#ifdef CONFIG_DPU_FB_V501
	set_flag_bits_if_cond(dpufb_hiace_roi_is_disable(dpufd) &&
		dpufb_display_effect_is_need_ace(dpufd), flag, ACE_ENABLED_DIRTY_UPDT_DISABLE);
	set_flag_bits_if_cond(dpufd->pipe_clk_ctrl.dirty_region_updt_disable, flag, PIPE_ENABLED_DIRTY_UPDT_DISABLE);
#else
	set_flag_bits_if_cond(dpufb_display_effect_is_need_ace(dpufd), flag, ACE_ENABLED_DIRTY_UPDT_DISABLE);
#endif

#ifdef CONFIG_DPU_DPP_CMDLIST
	set_flag_bits_if_cond(dpu_dpp_cmdlist_active(dpufd), flag, DPP_CMDLIST_DISABLE_PARTIAL_UPDATE);
#endif

#ifdef CONFIG_DPU_EFFECT_HIHDR
	set_flag_bits_if_cond(dpu_check_hihdr_active(dpufd), flag, HIHDR_ENABLED_DIRTY_UPDT_DISABLE);
#endif

	if (is_lcd_dfr_support(dpufd) &&
		(dpufd->panel_info.fps_updt != dpufd->panel_info.fps ||
		dpufd->panel_info.frm_rate_ctrl.status != FRM_UPDT_DONE))
		flag = flag | BIT(10);

	if (flag == 0)
		dpufd->dirty_region_updt_enable = 1;

	if (copy_to_user(argp, &flag, sizeof(flag))) {
		DPU_FB_ERR("copy to user fail");
		return -EFAULT;
	}

	return 0;
}

static int dpufb_get_video_idle_mode(struct fb_info *info, void __user *argp)
{
	int is_video_idle = 0;
	struct dpu_fb_data_type *dpufd = NULL;

	if (!info)
		return -EINVAL;

	dpufd = (struct dpu_fb_data_type *)info->par;
	if (!dpufd)
		return -EINVAL;

	if (!argp)
		return -EINVAL;

	is_video_idle = 0;

	if (g_enable_video_idle_l3cache && is_video_idle_ctrl_mode(dpufd)) {
#if defined(CONFIG_VIDEO_IDLE)
		/* last idle frame use gpu compose */
		is_video_idle =
			dpufd->video_idle_ctrl.gpu_compose_idle_frame ? 0 : 1;
#else
		is_video_idle = 1;
#endif
	}

	if (copy_to_user(argp, &is_video_idle, sizeof(is_video_idle))) {
		DPU_FB_ERR("copy to user fail");
		return -EFAULT;
	}

	return 0;
}

static int dpufb_idle_is_allowed(struct fb_info *info, void __user *argp)
{
	int is_allowed = 0;
	struct dpu_fb_data_type *dpufd = NULL;

	if (!info) {
		DPU_FB_ERR("idle is allowed info NULL Pointer!\n");
		return -EINVAL;
	}

	dpufd = (struct dpu_fb_data_type *)info->par;
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

static int dpufb_debug_check_fence_timeline(struct fb_info *info)
{
	struct dpu_fb_data_type *dpufd = NULL;
	struct dpufb_buf_sync *buf_sync_ctrl = NULL;
	unsigned long flags = 0;
	int val = 0;

	if (!info) {
		DPU_FB_ERR("timeline info NULL Pointer!\n");
		return -EINVAL;
	}

	dpufd = (struct dpu_fb_data_type *)info->par;
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

	dpu_resync_timeline(buf_sync_ctrl->timeline);
	dpu_resync_timeline(buf_sync_ctrl->timeline_retire);

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

static int dpufb_dss_get_platform_type(struct fb_info *info, void __user *argp)
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
#if defined(CONFIG_FOLD_DISPLAY)
	get_platform_product_info.fold_display_support = dpufd->panel_info.cascadeic_support;
#endif
	get_platform_product_info.dummy_pixel_num = dpufd->panel_info.dummy_pixel_num;
	get_platform_product_info.dfr_support_value = dpufd->panel_info.dfr_support_value;
	get_platform_product_info.bypass_device_compose = dpufd->panel_info.bypass_device_compose;
	get_platform_product_info.ifbc_type = dpufd->panel_info.ifbc_type;
	get_platform_product_info.p3_support = dpufd->panel_info.p3_support;
	get_platform_product_info.hdr_flw_support = dpufd->panel_info.hdr_flw_support;
	get_platform_product_info.post_hihdr_support = dpufd->panel_info.post_hihdr_support;
	get_platform_product_info.spr_enable = dpufd->panel_info.spr.spr_en;
	get_platform_product_info.dfr_method = dpufd->panel_info.dfr_method;
	get_platform_product_info.support_tiny_porch_ratio = dpufd->panel_info.support_tiny_porch_ratio;
	get_platform_product_info.support_ddr_bw_adjust = dpufd->panel_info.support_ddr_bw_adjust;
	get_platform_product_info.actual_porch_ratio = dpufd->panel_info.mipi.porch_ratio;
	get_platform_product_info.dual_lcd_support = dpufd->panel_info.dual_lcd_support;

#if defined(CONFIG_FOLD_DISPLAY)
	if (dpu_check_panel_product_type(dpufd->panel_info.product_type)) {
		get_platform_product_info.need_two_panel_display = 1;
		get_platform_product_info.fold_display_support = 1;
		get_platform_product_info.virtual_fb_xres = dpufd->panel_info.fb_xres;
		get_platform_product_info.virtual_fb_yres = dpufd->panel_info.fb_yres;
		dpufb_get_lcd_panel_info(dpufd, &get_platform_product_info);
	}
#endif

	if (copy_to_user(argp, &get_platform_product_info, sizeof(struct platform_product_info))) {
		DPU_FB_ERR("copy to user fail!\n");
		return -EINVAL;
	}

	DPU_FB_INFO("max_hwc_mmbuf_size=%d, max_mdc_mmbuf_size=%d,"
		"dummy_pixel_num=%d, dfr_support_value=%d, p3_support = %d, post_hihdr_support = %d, hdr_flw_support = %d,"
		"ifbc_type = %d, support_tiny_porch_ratio=%u, support_ddr_bw_adjust = %d, actual_porch_ratio=%u,"
		"bypass_device_compose=%d, dual_lcd_support = %d\n",
		get_platform_product_info.max_hwc_mmbuf_size, get_platform_product_info.max_mdc_mmbuf_size,
		get_platform_product_info.dummy_pixel_num, get_platform_product_info.dfr_support_value,
		get_platform_product_info.p3_support, get_platform_product_info.post_hihdr_support,
		get_platform_product_info.hdr_flw_support, get_platform_product_info.ifbc_type,
		get_platform_product_info.support_tiny_porch_ratio, get_platform_product_info.support_ddr_bw_adjust,
		get_platform_product_info.actual_porch_ratio, get_platform_product_info.bypass_device_compose,
		get_platform_product_info.dual_lcd_support);

#if defined(CONFIG_FOLD_DISPLAY)
	DPU_FB_INFO("fold_display_support=%d\n", get_platform_product_info.fold_display_support);
#endif

	return 0;
}

#ifdef CONFIG_DPU_EFFECT_HIHDR
static int dpufb_hihdr_get_mean(struct dpu_fb_data_type *dpufd, void __user *argp)
{
	int ret = 0;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL\n");
		return -EINVAL;
	}

	if (argp == NULL) {
		DPU_FB_ERR("argp is NULL\n");
		return -EINVAL;
	}

	if (dpufd->index != PRIMARY_PANEL_IDX) {
		DPU_FB_ERR("fb%d is not supported!\n", dpufd->index);
		return -EINVAL;
	}

	ret = (int)copy_to_user(argp, &dpufd->hihdr_mean, sizeof(uint32_t));
	if (ret) {
		DPU_FB_ERR("[hihdr] copy_to_user failed(param)! ret=%d.\n", ret);
		ret = -1;
	}

	return ret;
}

static int dpufb_hihdr_basic_config_ready(struct dpu_fb_data_type *dpufd, void __user *argp)
{
	int ret = 0;

	if (dpufd == NULL) {
		DPU_FB_ERR("dpufd is NULL\n");
		return -EINVAL;
	}

	if (argp == NULL) {
		DPU_FB_ERR("argp is NULL\n");
		return -EINVAL;
	}

	if (dpufd->index != PRIMARY_PANEL_IDX) {
		DPU_FB_ERR("fb%d is not supported!\n", dpufd->index);
		return -EINVAL;
	}

	if (!dpufd->panel_power_on) {
		DPU_FB_INFO("fb%d, panel is power off!\n", dpufd->index);
		return -EINVAL;
	}

	if (g_debug_effect_hihdr)
		DPU_FB_INFO("[hihdr] hihdr_basic_ready = %d", dpufd->hihdr_basic_ready);

	ret = (int)copy_to_user(argp, &dpufd->hihdr_basic_ready, sizeof(uint32_t));
	if (ret) {
		DPU_FB_ERR("[hihdr] copy_to_user failed(param)! ret=%d.\n", ret);
		ret = -1;
	}

	return ret;
}

#endif

static int dpu_fb_get_source_mode(struct dpu_fb_data_type *dpufd, void __user *argp)
{
	bool same_source = false;

	dpu_check_and_return((argp == NULL) || (dpufd == NULL), -EINVAL, ERR, "argp or dpufd is NULL!\n");

	same_source = dpufd->panel_info.use_dsi1 == DUAL_DSI_HETEROLOGY_ENABLE ? false : true;

	if (copy_to_user(argp, &(same_source), sizeof(same_source))) {
		DPU_FB_ERR("copy_to_user failed!\n");
		return -EFAULT;
	}

	return 0;
}

static int dpu_fb_resource_ioctl(struct fb_info *info, struct dpu_fb_data_type *dpufd, unsigned int cmd,
	void __user *argp)
{
	int ret = -ENOSYS;

	switch (cmd) {
	case DPUFB_DSS_VOLTAGE_GET:
		ret = dpufb_ctrl_dss_voltage_get(info, argp);
		break;
	case DPUFB_DSS_VOLTAGE_SET:
		ret = dpufb_ctrl_dss_voltage_set(info, argp);
		break;
	case DPUFB_DSS_VOTE_CMD_SET:
		ret = dpufb_ctrl_dss_vote_cmd_set(info, argp);
		break;
	case DPUFB_DEBUG_CHECK_FENCE_TIMELINE:
		ret = dpufb_debug_check_fence_timeline(info);
		break;
	case DPUFB_IDLE_IS_ALLOWED:
		ret = dpufb_idle_is_allowed(info, argp);
		break;
	case DPUFB_LCD_DIRTY_REGION_INFO_GET:
		ret = dpufb_lcd_dirty_region_info_get(info, argp);
		break;
	case DPUFB_ALSC_ENABLE_INFO_GET:
		ret = dpufb_alsc_enable_info_get(info, argp);
		break;
	case DPUFB_DIRTY_UPDT_SUPPORT_GET:
		ret = dpufb_dirty_updt_support_get(info, argp);
		break;
	case DPUFB_VIDEO_IDLE_CTRL:
		ret = dpufb_get_video_idle_mode(info, argp);
		break;
	case DPUFB_PLATFORM_TYPE_GET:
		ret = dpufb_dss_get_platform_type(info, argp);
		break;
	case DPUFB_PLATFORM_PRODUCT_INFO_GET:
		ret = dpufb_dss_get_platform_product_info(dpufd, argp);
		break;
	case DPUFB_DPTX_GET_COLOR_BIT_MODE:
		if (dpufd->dp_get_color_bit_mode != NULL) {
			ret = dpufd->dp_get_color_bit_mode(dpufd, argp);
		} else {
			DPU_FB_DEBUG("dp_get_color_bit_mode not registered, bypass ioctl msg DPUFB_DPTX_GET_COLOR_BIT_MODE\n");
			ret = -ENODATA;
		}
		break;
	case DPUFB_DPTX_GET_SOURCE_MODE:
		if (dpufd->dp_get_source_mode != NULL) {
			ret = dpufd->dp_get_source_mode(dpufd, argp);
		} else if (dpufd->panel_info.use_dsi1 == DUAL_DSI_HETEROLOGY_ENABLE) {
			ret = dpu_fb_get_source_mode(dpufd, argp);
		} else {
			DPU_FB_DEBUG("dp_get_source_mode not registered, bypass ioctl msg DPUFB_DPTX_GET_SOURCE_MODE\n");
			ret = -ENODATA;
		}
		break;
#if defined(CONFIG_FOLD_DISPLAY)
	case DPUFB_PANEL_REGION_NOTIFY:
		if (dpufd->panel_set_display_region != NULL)
			ret = dpufd->panel_set_display_region(dpufd, argp);
		break;
	case DPUFB_DISPLAY_PANEL_FOLD_STATUS_NOTIFY:
		if (dpufd->set_display_panel_status != NULL)
			ret = dpufd->set_display_panel_status(dpufd, argp);
		break;
#endif

	default:
		break;
	}

	return ret;
}

static int dpu_fb_effect_ioctl(struct fb_info *info, struct dpu_fb_data_type *dpufd, unsigned int cmd,
	void __user *argp)
{
	int ret = -ENOSYS;

	switch (cmd) {
#if defined(CONFIG_EFFECT_HIACE)
	case DPUFB_CE_ENABLE:
		ret = dpufb_ce_service_enable_hiace(info, argp);
		break;
	case DPUFB_CE_SUPPORT_GET:
		ret = dpufb_ce_service_get_support(info, argp);
		break;
	case DPUFB_CE_SERVICE_LIMIT_GET:
		ret = dpufb_ce_service_get_limit(info, argp);
		break;
	case DPUFB_CE_PARAM_GET:
		ret = dpufb_ce_service_get_param(info, argp);
		break;
	case DPUFB_HIACE_PARAM_GET:
		ret = dpufb_ce_service_get_hiace_param(info, argp);
		break;
	case DPUFB_CE_PARAM_SET:
		ret = dpufb_ce_service_set_param(info, argp);
		break;
	case DPUFB_CE_HIST_GET:
		ret = dpufb_ce_service_get_hist(info, argp);
		break;
	case DPUFB_CE_LUT_SET:
		ret = dpufb_ce_service_set_lut(info, argp);
		break;
#ifdef HIACE_SINGLE_MODE_SUPPORT
	case DPUFB_HIACE_SINGLE_MODE_TRIGGER:
		ret = dpufb_hiace_single_mode_trigger(info, argp);
		break;
	case DPUFB_HIACE_BLOCK_ONCE_SET:
		ret = dpufb_hiace_single_mode_block_once_set(info, argp);
		break;
	case DPUFB_HIACE_HIST_GET:
		ret = dpufb_hiace_hist_get(info, argp);
		break;
	case DPUFB_HIACE_FNA_DATA_GET:
		ret = dpufb_hiace_fna_get(info, argp);
		break;
#endif
#if defined(CONFIG_DPU_FB_V501) || defined(CONFIG_DPU_FB_V510) || \
	defined(CONFIG_DPU_FB_V600) || defined(CONFIG_DPU_FB_V360)
	case DPUFB_GET_HIACE_ENABLE:
		ret = dpufb_get_hiace_enable(info, argp);
		break;
	case DPUFB_HIACE_ROI_GET:
		ret = dpufb_get_hiace_roi(info, argp);
		break;
#endif
#endif
	case DPUFB_DISPLAY_ENGINE_INIT:
		ret = dpufb_display_engine_init(info, argp);
		break;
	case DPUFB_DISPLAY_ENGINE_DEINIT:
		ret = dpufb_display_engine_deinit(info, argp);
		break;
	case DPUFB_DISPLAY_ENGINE_PARAM_GET:
		ret = dpufb_display_engine_param_get(info, argp);
		break;
	case DPUFB_DISPLAY_ENGINE_PARAM_SET:
		ret = dpufb_display_engine_param_set(info, argp);
		break;
	case DPUFB_GET_REG_VAL:
		ret = dpufb_get_reg_val(info, argp);
		break;
	case DPUFB_EFFECT_MODULE_INIT:
	case DPUFB_EFFECT_MODULE_DEINIT:
	case DPUFB_EFFECT_INFO_GET:
	case DPUFB_EFFECT_INFO_SET:
		if (dpufd->display_effect_ioctl_handler != NULL) {
			ret = dpufd->display_effect_ioctl_handler(dpufd, cmd, argp);
		} else {
			DPU_FB_DEBUG("display_effect_ioctl_handler not registered, bypass ioctl msg \
				DPUFB_EFFECT_INFO_SET/GET & DPUFB_EFFECT_MODULE_INIT/DEINIT\n");
			ret = -ENODATA;
		}
		break;

#ifdef CONFIG_DPU_EFFECT_HIHDR
	case DPUFB_HIHDR_MEAN_GET:
		ret = dpufb_hihdr_get_mean(dpufd, argp);
		break;

	case DPUFB_HIHDR_BASIC_CONFIG_GET:
		ret = dpufb_hihdr_basic_config_ready(dpufd, argp);
		break;
#endif
	default:
		break;
	}

	return ret;
}

static int dpu_fb_display_ioctl(struct fb_info *info, struct dpu_fb_data_type *dpufd, unsigned int cmd,
	void __user *argp)
{
	int ret = -ENOSYS;

	switch (cmd) {
	case DPUFB_VSYNC_CTRL:
		if (dpufd->vsync_ctrl_fnc != NULL) {
			ret = dpufd->vsync_ctrl_fnc(info, argp);
		} else {
			DPU_FB_DEBUG("vsync_ctrl_fnc not registered, bypass ioctl msg DPUFB_VSYNC_CTRL\n");
			ret = -ENODATA;
		}
		break;
	case DPUFB_GRALLOC_MAP_IOVA:
		ret = dpu_buffer_map_iova(info, argp);
		break;
	case DPUFB_GRALLOC_UNMAP_IOVA:
		ret = dpu_buffer_unmap_iova(info, argp);
		break;
	case DPUFB_MMBUF_SIZE_QUERY:
		ret = dpu_mmbuf_reserved_size_query(info, argp);
		break;
	case DPUFB_DSS_MMBUF_ALLOC:
		ret = dpu_mmbuf_request(info, argp);
		break;
	case DPUFB_DSS_MMBUF_FREE:
		ret = dpu_mmbuf_release(info, argp);
		break;
	case DPUFB_DSS_MMBUF_FREE_ALL:
		ret = dpu_mmbuf_free_all(info, argp);
		break;
	case DPUFB_MDC_CHANNEL_INFO_REQUEST:
		ret = dpu_mdc_chn_request(info, argp);
		break;
	case DPUFB_MDC_CHANNEL_INFO_RELEASE:
		ret = dpu_mdc_chn_release(info, argp);
		break;
	case DPUFB_DPTX_SEND_HDR_METADATA:
		if (dpufd->dptx_hdr_infoframe_sdp_send) {
			ret = dpufd->dptx_hdr_infoframe_sdp_send(&(dpufd->dp), argp);
		} else {
			DPU_FB_DEBUG("dptx_hdr_infoframe_sdp_send not registered, bypass ioctl msg DPUFB_DPTX_SEND_HDR_METADATA\n");
			ret = -ENODATA;
		}
		break;
#if defined(CONFIG_FOLD_DISPLAY)
	case DPUFB_CONFIG_PANEL_ESD_STATUS:
		ret = dpufb_config_panel_esd_status(dpufd, argp);
		break;
#endif

	default:
		if (dpufd->ov_ioctl_handler != NULL)
			ret = dpufd->ov_ioctl_handler(dpufd, cmd, argp);
		break;
	}

	return ret;
}

static int dpu_fb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
	int ret;
	struct dpu_fb_data_type *dpufd = NULL;
	void __user *argp = (void __user *)(uintptr_t)arg;

	dpu_check_and_return(!info, -EINVAL, ERR, "info is NULL!\n");

	dpufd = (struct dpu_fb_data_type *)info->par;
	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL!\n");

#ifdef CONFIG_DPU_FB_MEDIACOMMON
	if (dpufd->index == MEDIACOMMON_PANEL_IDX)
		return dpu_mediacommon_ioctl_handler(dpufd, cmd, argp);
#endif

	ret = dpu_fb_resource_ioctl(info, dpufd, cmd, argp);
	if (ret != -ENOSYS)
		return ret;

	ret = dpu_fb_effect_ioctl(info, dpufd, cmd, argp);
	if (ret != -ENOSYS)
		return ret;

	ret = dpu_fb_display_ioctl(info, dpufd, cmd, argp);
	if (ret != -ENOSYS)
		return ret;
	else
		DPU_FB_ERR("unsupported ioctl [%x]\n", cmd);

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

/*******************************************************************************
 *
 */
static struct fb_ops dpu_fb_ops = {
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

#if defined(CONFIG_DPU_FB_V320)
int dpufb_fastboot_power_on(struct notifier_block *nb,
		unsigned long event, void *data)
{
	(void *)nb;
	(void *)data;

	struct dpu_fb_panel_data *pdata = NULL;
	/* primary panel */
	struct dpu_fb_data_type *dpufd = dpufd_list[PRIMARY_PANEL_IDX];

	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL\n");
		return -EINVAL;
	}

	pdata = (struct dpu_fb_panel_data *)dpufd->pdev->dev.platform_data;
	if ((pdata != NULL) && pdata->set_fastboot && !g_fastboot_already_set) {
		pdata->set_fastboot(dpufd->pdev);
		g_fastboot_already_set = 1;
	}
	return 0;
}
#endif

int dpufb_esd_recover_disable(int value)
{
	struct dpu_fb_panel_data *pdata = NULL;
	/* primary panel */
	struct dpu_fb_data_type *dpufd = dpufd_list[PRIMARY_PANEL_IDX];

	if (!dpufd) {
		DPU_FB_INFO("esd_recover_disable fail\n");
		return 0;
	}

	pdata = (struct dpu_fb_panel_data *)dpufd->pdev->dev.platform_data;
	if (pdata != NULL && pdata->panel_info) {
		if (pdata->panel_info->esd_enable) {
			DPU_FB_INFO("esd_recover_disable=%d\n", value);
			g_esd_recover_disable = (unsigned int)value;
		}
	}

	return 0;
}
EXPORT_SYMBOL(dpufb_esd_recover_disable); /*lint !e580*/

int dpufb_check_ldi_porch(struct dpu_panel_info *pinfo)
{
	int vertical_porch_min_time;
	int pxl_clk_rate_div;
	int ifbc_v_porch_div = 1;

	dpu_check_and_return(!pinfo, -EINVAL, ERR, "pinfo is NULL\n");

	if (pinfo->mipi.dsi_timing_support)
		return 0;

	pxl_clk_rate_div = (pinfo->pxl_clk_rate_div == 0 ? 1 : pinfo->pxl_clk_rate_div);
	if (pinfo->ifbc_type == IFBC_TYPE_RSP3X) {
		pxl_clk_rate_div *= 3;  /* IFBC_TYPE_RSP3X */
		pxl_clk_rate_div /= 2;  /* half clk_rate_division */
		ifbc_v_porch_div = 2;  /* half ifbv v_porch_division */
	}

	if (g_fpga_flag == 1)
		return 0;  /* do not check ldi porch in fpga version */

	/* hbp+hfp+hsw time should be longer than 30 pixel clock cycel */
	if (pxl_clk_rate_div * (pinfo->ldi.h_back_porch + pinfo->ldi.h_front_porch
		+ pinfo->ldi.h_pulse_width) <= 30) {
		DPU_FB_ERR("ldi hbp+hfp+hsw is not larger than 30, return!\n");
		return -1;
	}

	/* check vbp+vsw */
	if (pinfo->xres * pinfo->yres >= RES_4K_PAD)
		vertical_porch_min_time = 50;
	else if (pinfo->xres * pinfo->yres >= RES_1600P)
		vertical_porch_min_time = 45;
	else if (pinfo->xres * pinfo->yres >= RES_1440P)
		vertical_porch_min_time = 40;
	else if (pinfo->xres * pinfo->yres >= RES_1200P)
		vertical_porch_min_time = 45;
	else
		vertical_porch_min_time = 35;

	if ((uint32_t)ifbc_v_porch_div * (pinfo->ldi.v_back_porch + pinfo->ldi.v_pulse_width) <
		(uint32_t)vertical_porch_min_time) {
		if ((uint32_t)pinfo->non_check_ldi_porch == 1) {
			DPU_FB_INFO("panel IC specification do not match this rule(vbp+vsw >= %d),"
				"we must skip it, Otherwise it will display unnormal!\n", vertical_porch_min_time);
		} else {
			DPU_FB_ERR("ldi vbp+vsw is less than %d, return!\n", vertical_porch_min_time);
			return -1;
		}
	}

	return 0;
}

static void dpufb_check_dummy_pixel_num(struct dpu_panel_info *pinfo)
{
	if (pinfo->dummy_pixel_num % 2 != 0) {
		DPU_FB_ERR("dummy_pixel_num should be even, so plus 1 !\n");
		pinfo->dummy_pixel_num += 1;
	}

	if (pinfo->dummy_pixel_num >= pinfo->xres) {
		DPU_FB_ERR("dummy_pixel_num is invalid, force set it to 0 !\n");
		pinfo->dummy_pixel_num = 0;
	}
}

static uint32_t get_initial_fps(struct dpu_fb_data_type *dpufd, struct fb_var_screeninfo *var)
{
	uint64_t lane_byte_clock;
	uint32_t hsize;
	uint32_t vsize;
	uint32_t fps;

	if (dpufd->panel_info.mipi.dsi_timing_support) {
		lane_byte_clock = (dpufd->panel_info.mipi.phy_mode == DPHY_MODE) ?
				(uint64_t)(dpufd->panel_info.mipi.dsi_bit_clk * 2 / 8) : /* lane byte clock */
				(uint64_t)(dpufd->panel_info.mipi.dsi_bit_clk / 7);
		lane_byte_clock = lane_byte_clock * 1000000UL;  /* 1MHz */
		hsize = dpufd->panel_info.mipi.hline_time;
		vsize = dpufd->panel_info.yres + dpufd->panel_info.mipi.vsa +
			dpufd->panel_info.mipi.vfp + dpufd->panel_info.mipi.vbp;

		fps = (uint32_t)(lane_byte_clock / hsize / vsize);
	} else {
		hsize = var->upper_margin + var->lower_margin + var->vsync_len + dpufd->panel_info.yres;
		vsize = var->left_margin + var->right_margin + var->hsync_len + dpufd->panel_info.xres;
		fps = (uint32_t)(var->pixclock / hsize / vsize);
	}

	return fps;
}

void dpu_fb_pdp_fnc_register(struct dpu_fb_data_type *dpufd)
{
	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is NULL\n");
	DPU_FB_DEBUG("[fb%d]+\n", dpufd->index);

	dpufd->fb_mem_free_flag = false;

	if (g_fastboot_enable_flag == 1) {
		dpufd->set_fastboot_fnc = dpu_fb_set_fastboot_needed;
		g_fastboot_set_needed++;
	} else {
		dpufd->set_fastboot_fnc = NULL;
	}

	dpufd->open_sub_fnc = dpu_fb_open_sub;
	dpufd->release_sub_fnc = dpu_fb_release_sub;
	dpufd->hpd_open_sub_fnc = NULL;
	dpufd->hpd_release_sub_fnc = NULL;
	dpufd->lp_fnc = dpufb_ctrl_lp;
	dpufd->esd_fnc = dpufb_ctrl_esd;
	dpufd->sbl_ctrl_fnc = NULL;
	dpufd->fps_upt_isr_handler = dpufb_fps_upt_isr_handler;
	dpufd->mipi_dsi_bit_clk_upt_isr_handler = mipi_dsi_bit_clk_upt_isr_handler;
	dpufd->panel_mode_switch_isr_handler = panel_mode_switch_isr_handler;
#if defined(CONFIG_DEVICE_ATTRS)
	dpufd->sysfs_attrs_add_fnc = dpufb_sysfs_attrs_add;
#else
	dpufd->sysfs_attrs_add_fnc = NULL;
#endif
	dpufd->sysfs_attrs_append_fnc = dpufb_sysfs_attrs_append;
	dpufd->sysfs_create_fnc = dpufb_sysfs_create;
	dpufd->sysfs_remove_fnc = dpufb_sysfs_remove;

	dpufd->bl_register = dpufb_backlight_register;
	dpufd->bl_unregister = dpufb_backlight_unregister;
	dpufd->bl_update = dpufb_backlight_update;
	dpufd->bl_cancel = dpufb_backlight_cancel;
	dpufd->vsync_register = dpufb_vsync_register;
	dpufd->vsync_unregister = dpufb_vsync_unregister;
	dpufd->vsync_ctrl_fnc = dpufb_vsync_ctrl;
	dpufd->vsync_isr_handler = dpufb_vsync_isr_handler;
	dpufd->buf_sync_register = dpufb_buf_sync_register;
	dpufd->buf_sync_unregister = dpufb_buf_sync_unregister;
	dpufd->buf_sync_signal = dpufb_buf_sync_signal;
	dpufd->buf_sync_suspend = dpufb_buf_sync_suspend;
	dpufd->secure_register = dpufb_secure_register;
	dpufd->secure_unregister = dpufb_secure_unregister;
	dpufd->esd_register = dpufb_esd_register;
	dpufd->esd_unregister = dpufb_esd_unregister;
	dpufd->debug_register = dpufb_debug_register;
	dpufd->debug_unregister = dpufb_debug_unregister;
	dpufd->cabc_update = update_cabc_pwm;
#if defined(CONFIG_FOLD_DISPLAY)
	dpufd->panel_set_display_region = panel_set_display_region;
	dpufd->set_display_panel_status = set_display_panel_status;
#endif

	dpu_fb_pm_runtime_register(dpufd);

#if defined(CONFIG_DPU_FB_V501)
#if defined(CONFIG_VIDEO_IDLE)
	dpufd->video_idle_ctrl_register = dpufb_video_idle_ctrl_register;
	dpufd->video_idle_ctrl_unregister = dpufb_video_idle_ctrl_unregister;
#endif
	dpufd->pipe_clk_updt_isr_handler = dpufb_pipe_clk_updt_isr_handler;
	dpufd->overlay_online_wb_register = dpufb_ovl_online_wb_register;
	dpufd->overlay_online_wb_unregister = dpufb_ovl_online_wb_unregister;
#elif defined(CONFIG_VIDEO_IDLE)
	dpufd->video_idle_ctrl_register = dpufb_video_idle_ctrl_register;
	dpufd->video_idle_ctrl_unregister = dpufb_video_idle_ctrl_unregister;
#else
	dpufd->video_idle_ctrl_register = NULL;
	dpufd->video_idle_ctrl_unregister = NULL;
#endif

#if !defined(CONFIG_DPU_FB_V501)
	dpufd->pipe_clk_updt_isr_handler = NULL;
	dpufd->overlay_online_wb_register = NULL;
	dpufd->overlay_online_wb_unregister = NULL;
#endif

	dpufd->dumpDss = dpufb_alloc_dumpdss();
	DPU_FB_DEBUG("[fb%d]-\n", dpufd->index);
}

static void dpu_fbi_data_init(struct fb_info *fbi)
{
	fbi->screen_base = 0;
	fbi->fbops = &dpu_fb_ops;
	fbi->flags = FBINFO_FLAG_DEFAULT;
	fbi->pseudo_palette = NULL;
}

void dpu_fb_offlinecomposer_init(struct fb_var_screeninfo *var, struct dpu_panel_info *panel_info)
{
	dpu_check_and_no_retval((!var || !panel_info), ERR, "panel_info is NULL\n");

	/* for offline composer */
	g_primary_lcd_xres = var->xres;
	g_primary_lcd_yres = var->yres;
	g_pxl_clk_rate = panel_info->pxl_clk_rate;
	g_prefix_ce_support = panel_info->prefix_ce_support;
	g_prefix_sharpness1d_support = panel_info->prefix_sharpness1D_support;
	g_prefix_sharpness2d_support = panel_info->prefix_sharpness2D_support;
}

static int dpu_fbi_info_init(struct dpu_fb_data_type *dpufd, struct fb_info *fbi)
{
	int bpp = 0;
	struct fb_fix_screeninfo *fix = &fbi->fix;
	struct fb_var_screeninfo *var = &fbi->var;
	struct dpu_panel_info *panel_info = &dpufd->panel_info;

	dpu_fb_init_screeninfo_base(fix, var);

	if (!dpu_fb_img_type_valid(dpufd->fb_imgType)) {
		DPU_FB_ERR("fb%d, unkown image type!\n", dpufd->index);
		return -EINVAL;
	}

	dpu_fb_init_sreeninfo_by_img_type(fix, var, dpufd->fb_imgType, &bpp);

	/* for resolution update */
	memset(&(dpufd->resolution_rect), 0, sizeof(dss_rect_t));

	dpu_fb_init_sreeninfo_by_panel_info(var, panel_info, dpufd->fb_num, bpp);

	if (dpu_check_dual_lcd_support(dpufd->panel_info.dual_lcd_support) &&
		dpufd->index == EXTERNAL_PANEL_IDX)
		dpu_fb_init_sreeninfo_by_panel_info(var, panel_info, DPU_FB0_NUM, bpp);

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		panel_info->fps = get_initial_fps(dpufd, var);
		var->reserved[0] = panel_info->fps;
	}

	var->reserved[1] = panel_info->split_support;
	var->reserved[1] |= panel_info->split_logical1_ratio << EXT_PANEL_SPLIT_RATIO_OFFSET;
	var->reserved[1] |= panel_info->use_dsi1 << EXT_PANEL_USE_DSI1_OFFSET;
	DPU_FB_INFO("fb%d, dsi_timing_support %d, fps = reserved[0] = %d, split_support = 0x%x, \
				split_logical1_ratio = 0x%x, use_dsi1 = 0x%x\n",
				dpufd->index, panel_info->mipi.dsi_timing_support, var->reserved[0],
				var->reserved[1] & 0xFF, panel_info->split_logical1_ratio, panel_info->use_dsi1);

	snprintf(fix->id, sizeof(fix->id), "dpufb%d", dpufd->index);
	fix->line_length = dpufb_line_length(dpufd->index, var->xres_virtual, bpp);
	fix->smem_len = roundup(fix->line_length * var->yres_virtual, PAGE_SIZE);
	fix->smem_start = 0;

	dpu_fbi_data_init(fbi);

	fix->reserved[0] = is_mipi_cmd_panel(dpufd) ? 1 : 0;

	return 0;
}

static int dpu_fb_buffer_allocate(struct dpu_fb_data_type *dpufd)
{
	if (dpufd->index == PRIMARY_PANEL_IDX) {
#ifdef CONFIG_DPU_EFFECT_HIHDR
		if (!dpufd->hdr_info) {
			dpufd->hdr_info = (dss_hihdr_info_t *)kzalloc(sizeof(dss_hihdr_info_t), GFP_ATOMIC);
			if (!dpufd->hdr_info)
				dpufd->hdr_info = (dss_hihdr_info_t *)kzalloc(sizeof(dss_hihdr_info_t), GFP_KERNEL);

			if (!dpufd->hdr_info) {
				DPU_FB_ERR("hihdr buffer alloc fail!\n");
				return -EINVAL;
			}
		}
#endif
	}

	return 0;
}

static int dpu_fb_buffer_free(struct dpu_fb_data_type *dpufd)
{
	if (dpufd->index == PRIMARY_PANEL_IDX) {
#ifdef CONFIG_DPU_EFFECT_HIHDR
		if (dpufd->hdr_info) {
			kfree(dpufd->hdr_info);
			dpufd->hdr_info = NULL;
		}
#endif
	}
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

	dpu_check_and_return(dpufb_create_buffer_client(dpufd), -ENOMEM, ERR, "create buffer client failed!\n");

	if (fbi->fix.smem_len > 0)
		dpu_check_and_return((!dpufb_alloc_fb_buffer(dpufd)), -ENOMEM, ERR, "dpufb_alloc_buffer fail!\n");

	dpu_fb_data_init(dpufd);

	atomic_set(&(dpufd->atomic_v), 0);

	dpu_fb_buffer_allocate(dpufd);

	dpu_fb_init_sema(dpufd);

	dpu_fb_init_spin_lock(dpufd);

	dpufb_sysfs_init(dpufd);

	if (dpu_fb_registe_callback(dpufd, &fbi->var, panel_info) < 0)
		return -EINVAL;

#if defined(CONFIG_DPU_FB_V320)
	if (g_fastboot_enable_flag == 1) {
		dpufd->nb.notifier_call = dpufb_fastboot_power_on;
		scharger_register_notifier(&dpufd->nb);
	}
#endif

	if (dpu_overlay_init(dpufd))
		return -EPERM;

	dpu_display_effect_init(dpufd);
	dpufb_display_engine_register(dpufd);

#if defined(CONFIG_DPU_DISPLAY_DFR)
	mipi_dsi_frm_rate_ctrl_init(dpufd);
#endif

	if (register_framebuffer(fbi) < 0) {
		DPU_FB_ERR("fb%d failed to register_framebuffer!\n", dpufd->index);
		return -EPERM;
	}

	if (dpufd->sysfs_attrs_add_fnc != NULL)
		dpufd->sysfs_attrs_add_fnc(dpufd);

	dpu_fb_common_register(dpufd);

	DPU_FB_INFO("FrameBuffer[%d] %dx%d size=%d bytes is registered successfully!\n",
		dpufd->index, fbi->var.xres, fbi->var.yres, fbi->fix.smem_len);

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

	ret = dpu_iommu_enable(pdev);
	dev_check_and_return(dev, ret, -ENXIO, err, "failed to dpu_iommu_enable! ret = %d\n", ret);

	/* find and get logo-buffer base */
	np = of_find_node_by_path(DTS_PATH_LOGO_BUFFER);
	if (!np)
		dev_err(dev, "NOT FOUND dts path: %s!\n", DTS_PATH_LOGO_BUFFER);

	if (g_fastboot_enable_flag == 1)
		dpu_fb_read_logo_buffer_from_dts(np, dev);

	sema_init(&g_dpufb_dss_clk_vote_sem, 1);

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

	if (!dpu_fb_resource_initialized) {
		ret = dpu_fb_init_resource(pdev);
		return ret;
	}

	if (pdev->id < 0) {
		dev_err(dev, "WARNING: id=%d, name=%s!\n", pdev->id, pdev->name);
		return 0;
	}

	if (!dpu_fb_resource_initialized) {
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

#if defined(CONFIG_DPU_FB_AOD)
	dpu_create_aod_wq(dpufd);
	/* clear SCBAKDATA0 status */
	outp32(dpufd->sctrl_base + SCBAKDATA0, 0x0);
#endif

#ifdef CONFIG_POWER_DUBAI_RGB_STATS
	if (dpufd->index == PRIMARY_PANEL_IDX)
		dpufb_rgb_stats_register(dpufd);
#endif

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

	dpufb_display_engine_unregister(dpufd);

	/* overlay destroy */
	dpu_overlay_deinit(dpufd);

	/* free framebuffer */
	dpufb_free_fb_buffer(dpufd);

	dpufb_destroy_buffer_client(dpufd);

	dpu_fb_buffer_free(dpufd);

	/* remove /dev/fb* */
	unregister_framebuffer(dpufd->fbi);

	dpufb_free_dumpdss(dpufd);

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
	/* lcd check esd remove */
	if (dpufd->esd_unregister != NULL)
		dpufd->esd_unregister(dpufd->pdev);
	/* unregister debug */
	if (dpufd->debug_unregister != NULL)
		dpufd->debug_unregister(dpufd->pdev);
	/* remove video idle ctrl */
	if (dpufd->video_idle_ctrl_unregister != NULL)
		dpufd->video_idle_ctrl_unregister(dpufd->pdev);
	/* remove overlay online wirteback */
	if (dpufd->overlay_online_wb_unregister != NULL)
		dpufd->overlay_online_wb_unregister(dpufd->pdev);

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

	DPU_FB_INFO("[fb%d]+\n", dpufd->index);

	ret = dpu_fb_blank_sub(FB_BLANK_POWERDOWN, dpufd->fbi);
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

	ret = dpu_fb_blank_sub(FB_BLANK_UNBLANK, dpufd->fbi);
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
static int dpu_fb_suspend(struct platform_device *pdev, pm_message_t state)
{
	int ret;
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

	DPU_FB_INFO("fb%d, +\n", dpufd->index);

	console_lock();
	fb_set_suspend(dpufd->fbi, FBINFO_STATE_SUSPENDED);
	ret = dpu_fb_suspend_sub(dpufd);
	if (ret != 0) {
		DPU_FB_ERR("fb%d dpu_fb_suspend_sub failed, error=%d!\n", dpufd->index, ret);
		fb_set_suspend(dpufd->fbi, FBINFO_STATE_RUNNING);
	} else {
		pdev->dev.power.power_state = state;
	}
	console_unlock();

	DPU_FB_INFO("fb%d, -\n", dpufd->index);

	return ret;
}

static int dpu_fb_resume(struct platform_device *pdev)
{
	int ret;
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

	DPU_FB_INFO("fb%d, +\n", dpufd->index);

	console_lock();
	ret = dpu_fb_resume_sub(dpufd);
	pdev->dev.power.power_state = PMSG_ON;
	fb_set_suspend(dpufd->fbi, FBINFO_STATE_RUNNING);
	console_unlock();

	DPU_FB_INFO("fb%d, -\n", dpufd->index);

	return ret;
}
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

	DPU_FB_INFO("[fb%d]+\n", dpufd->index);

	if (dpufd->index == AUXILIARY_PANEL_IDX)
		return 0;

	ret = dpu_fb_suspend_sub(dpufd);
	if (ret != 0)
		DPU_FB_ERR("fb%d, failed to dpu_fb_suspend_sub! ret=%d\n", dpufd->index, ret);

	dpufd->media_common_composer_sr_refcount = 0;
	DPU_FB_INFO("[fb%d]-\n", dpufd->index);

	return 0;
}

#ifdef SUPPORT_FPGA_SUSPEND_RESUME
static int dpu_fb_pm_resume(struct device *dev)
{
	struct dpu_fb_data_type *dpufd = NULL;
	int ret;

	dpufd = dev_get_drvdata(dev);
	if (!dpufd)
		return 0;

	if (dpufd->index != PRIMARY_PANEL_IDX)
		return 0;

	DPU_FB_INFO("fb%d, +\n", dpufd->index);

	ret = dpu_fb_resume_sub(dpufd);
	if (ret != 0)
		DPU_FB_ERR("fb%d, failed to dpu_fb_resume_sub! ret=%d\n", dpufd->index, ret);

	DPU_FB_INFO("fb%d, -\n", dpufd->index);

	return 0;
}

/*
 * 1. to fix the resume to light the lcd on fpga platform.
 * 2. if not fpga platform,do nothing and return 0.
 */
static int dpu_fb_pm_resume_on_fpga(struct device *dev)
{
	int ret;

	if (!dev)
		return 0;

	if (g_fpga_flag == 1) { /* fpga platform */
		ret = dpu_fb_pm_resume(dev);
		if (ret != 0)
			DPU_FB_ERR("failed to dpu_fb_pm_resume! ret=%d\n", ret);
	}
	return 0;
}
#endif
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

	if (dpufd->index != PRIMARY_PANEL_IDX) {
		DPU_FB_DEBUG("fb%d do not shutdown\n", dpufd->index);
		return;
	}

	DPU_FB_INFO("fb%d shutdown +\n", dpufd->index);
	dpufd->fb_shutdown = true;

	ret = dpu_fb_blank_sub(FB_BLANK_POWERDOWN, dpufd->fbi);
	if (ret)
		DPU_FB_ERR("fb%d can't turn off display, error=%d!\n", dpufd->index, ret);

	DPU_FB_INFO("fb%d shutdown -\n", dpufd->index);
}


/*******************************************************************************
 *
 */
#ifdef SUPPORT_FPGA_SUSPEND_RESUME
static const struct dev_pm_ops dpu_fb_dev_pm_ops = {
#ifdef CONFIG_PM_RUNTIME
	.runtime_suspend = dpu_fb_runtime_suspend,
	.runtime_resume = dpu_fb_runtime_resume,
	.runtime_idle = dpu_fb_runtime_idle,
#endif
#ifdef CONFIG_PM_SLEEP
	.suspend = dpu_fb_pm_suspend,
	/* to fix the dpuv330 resume to light the lcd on fpga platform */
	.resume = dpu_fb_pm_resume_on_fpga,
#endif
};
#else
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
#endif

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
	.suspend = dpu_fb_suspend,
	.resume = dpu_fb_resume,
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

