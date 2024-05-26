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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"

#include <asm/sysreg.h>
#include "dpu_fb.h"
#include "dpu_display_effect.h"
#include "dpu_dpe_utils.h"
#include "overlay/init/dpu_init.h"
#include "performance/dpu_performance.h"
#include "dpu_mipi_dsi.h"
#include "dpu_fb_sysfs.h"
#include "overlay/dump/dpu_dump.h"
#include "dpu_disp_recorder.h"

#ifdef CONFIG_INPUTHUB_20
#include "sensor_feima.h"
#endif
#include "chrdev/dpu_chrdev.h"

dpu_fb_fix_var_screeninfo_t g_dpu_fb_fix_var_screeninfo[DPU_FB_PIXEL_FORMAT_MAX] = {
	{0}, {0}, {0}, {0}, {0}, {0}, {0},
	/* for DPU_FB_PIXEL_FORMAT_BGR_565 */
	{ FB_TYPE_PACKED_PIXELS, 1, 1, FB_VMODE_NONINTERLACED, 0, 5, 11, 0, 5, 6, 5, 0, 0, 0, 0, 0, 2 },
	{ FB_TYPE_PACKED_PIXELS, 1, 1, FB_VMODE_NONINTERLACED, 0, 4, 8, 0, 4, 4, 4, 0, 0, 0, 0, 0, 2 },
	{ FB_TYPE_PACKED_PIXELS, 1, 1, FB_VMODE_NONINTERLACED, 0, 4, 8, 12, 4, 4, 4, 4, 0, 0, 0, 0, 2 },
	{ FB_TYPE_PACKED_PIXELS, 1, 1, FB_VMODE_NONINTERLACED, 0, 5, 10, 0, 5, 5, 5, 0, 0, 0, 0, 0, 2 },
	{ FB_TYPE_PACKED_PIXELS, 1, 1, FB_VMODE_NONINTERLACED, 0, 5, 10, 15, 5, 5, 5, 1, 0, 0, 0, 0, 2 },
	{0},
	/* DPU_FB_PIXEL_FORMAT_BGRA_8888 */
	{ FB_TYPE_PACKED_PIXELS, 1, 1, FB_VMODE_NONINTERLACED, 0, 8, 16, 24, 8, 8, 8, 8, 0, 0, 0, 0, 4 },
	{ FB_TYPE_INTERLEAVED_PLANES, 2, 1, FB_VMODE_NONINTERLACED, 0, 5, 11, 0, 5, 6, 5, 0, 0, 0, 0, 0, 2 },
	{0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0},
	{0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}
};

static bool check_panel_status(struct dpu_fb_data_type *dpufd)
{
	if (dpufd->index != AUXILIARY_PANEL_IDX && !dpufd->panel_power_on) {
		DPU_FB_DEBUG("fb%d, panel power off!\n", dpufd->index);
		return false;
	}

	return true;
}

void set_reg(char __iomem *addr, uint32_t val, uint8_t bw, uint8_t bs)
{
	uint32_t mask = (1UL << bw) - 1UL;
	uint32_t temp = 0;

	temp = inp32(addr);
	temp &= ~(mask << bs);

	outp32(addr, temp | ((val & mask) << bs));

	if (g_debug_set_reg_val)
		DPU_FB_DEBUG("writel: [%pK] = 0x%x\n", addr, temp | ((val & mask) << bs));
}

uint32_t set_bits32(uint32_t old_val, uint32_t val, uint8_t bw, uint8_t bs)
{
	uint32_t mask = (1UL << bw) - 1UL;
	uint32_t tmp;

	tmp = old_val;
	tmp &= ~(mask << bs);

	return (tmp | ((val & mask) << bs));
}

void dpufb_set_reg(struct dpu_fb_data_type *dpufd,
	char __iomem *addr, uint32_t val, uint8_t bw, uint8_t bs)
{
	void_unused(dpufd);
	set_reg(addr, val, bw, bs);
}

bool is_fastboot_display_enable(void)
{
	return ((g_fastboot_enable_flag == 1) ? true : false);
}

bool is_dss_idle_enable(void)
{
	return ((g_enable_dss_idle == 1) ? true : false);
}

uint32_t get_panel_xres(struct dpu_fb_data_type *dpufd)
{
	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL");
		return 0;
	}

	return ((dpufd->resolution_rect.w > 0) ? (uint32_t)(dpufd->resolution_rect.w) : dpufd->panel_info.xres);
}

uint32_t get_panel_yres(struct dpu_fb_data_type *dpufd)
{
	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL");
		return 0;
	}

	return ((dpufd->resolution_rect.h > 0) ? (uint32_t)(dpufd->resolution_rect.h) : dpufd->panel_info.yres);
}

uint32_t dpufb_line_length(int index, uint32_t xres, int bpp)
{
	void_unused(index);
	return ALIGN_UP(xres * (uint32_t)bpp, DMA_STRIDE_ALIGN);
}

void dpufb_get_timestamp(struct timeval *tv)
{
	struct timespec ts;

	if (!tv) {
		DPU_FB_ERR("tv is NULL\n");
		return;
	}

	ktime_get_ts(&ts);
	tv->tv_sec = ts.tv_sec;
	tv->tv_usec = ts.tv_nsec / NSEC_PER_USEC;
}

uint32_t dpufb_timestamp_diff(struct timeval *lasttime, struct timeval *curtime)
{
	uint32_t ret;

	if (!lasttime || !curtime) {
		DPU_FB_ERR("lasttime or curtime is NULL\n");
		return 0;
	}

	ret = (uint32_t)(1000000 * (curtime->tv_sec - lasttime->tv_sec) +
		curtime->tv_usec - lasttime->tv_usec);
	return ret;
}

extern uint32_t g_fastboot_already_set;
int dpufb_ctrl_fastboot(struct dpu_fb_data_type *dpufd)
{
	struct dpu_fb_panel_data *pdata = NULL;
	int ret = 0;

	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL\n");
		return -EINVAL;
	}
	pdata = dev_get_platdata(&dpufd->pdev->dev);
	if (!pdata) {
		DPU_FB_ERR("pdata is NULL\n");
		return -EINVAL;
	}

	if (pdata->set_fastboot && !g_fastboot_already_set)
		ret = pdata->set_fastboot(dpufd->pdev);

	dpufb_vsync_resume(dpufd);

	dpu_overlay_on(dpufd, true);

	if (dpufd->panel_info.esd_enable && dpufd->esd_ctrl.esd_inited)
		hrtimer_restart(&dpufd->esd_ctrl.esd_hrtimer);

	return ret;
}

int dpufb_ctrl_on(struct dpu_fb_data_type *dpufd)
{
	struct dpu_fb_panel_data *pdata = NULL;
	int ret = 0;

	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL\n");
		return -EINVAL;
	}

	pdata = dev_get_platdata(&dpufd->pdev->dev);
	if (!pdata) {
		DPU_FB_ERR("pdata is NULL\n");
		return -EINVAL;
	}

	if (pdata->on != NULL) {
		ret = pdata->on(dpufd->pdev);
		if (ret < 0) {
			DPU_FB_ERR("regulator/clk on fail\n");
			return ret;
		}
	}

	dpufb_vsync_resume(dpufd);

	dpu_overlay_on(dpufd, false);

	if (dpufd->panel_info.esd_enable && dpufd->esd_ctrl.esd_inited) {
		if (dpufd->panel_info.esd_check_time_period) {
			hrtimer_start(&dpufd->esd_ctrl.esd_hrtimer, ktime_set(dpufd->panel_info.esd_check_time_period / 1000,
				(dpufd->panel_info.esd_check_time_period % 1000) * 1000000), HRTIMER_MODE_REL);
		} else {
			hrtimer_start(&dpufd->esd_ctrl.esd_hrtimer, ktime_set(ESD_CHECK_TIME_PERIOD / 1000,
				(ESD_CHECK_TIME_PERIOD % 1000) * 1000000), HRTIMER_MODE_REL);
		}
	}

	enable_disp_recorder(&dpufd->disp_recorder);
	return ret;
}

int dpufb_ctrl_off(struct dpu_fb_data_type *dpufd)
{
	struct dpu_fb_panel_data *pdata = NULL;
	int ret = 0;

	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL\n");
		return -EINVAL;
	}
	pdata = dev_get_platdata(&dpufd->pdev->dev);
	if (!pdata) {
		DPU_FB_ERR("pdata is NULL\n");
		return -EINVAL;
	}

	if (dpufd->panel_info.esd_enable)
		hrtimer_cancel(&dpufd->esd_ctrl.esd_hrtimer);

	dpu_overlay_off(dpufd);

	if (pdata->off != NULL)
		ret = pdata->off(dpufd->pdev);

	if (dpufd->merger_mgr.ops && dpufd->merger_mgr.ops->suspend)
		dpufd->merger_mgr.ops->suspend(&dpufd->merger_mgr);

	disable_disp_recorder(&dpufd->disp_recorder);

	return ret;
}

int dpufb_ctrl_lp(struct dpu_fb_data_type *dpufd, bool lp_enter)
{
	struct dpu_fb_panel_data *pdata = NULL;
	int ret = 0;

	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL\n");
		return -EINVAL;
	}
	pdata = dev_get_platdata(&dpufd->pdev->dev);
	if (!pdata) {
		DPU_FB_ERR("pdata is NULL\n");
		return -EINVAL;
	}

	if (lp_enter) {
		dpu_overlay_off_lp(dpufd);

		if (pdata->lp_ctrl != NULL)
			ret = pdata->lp_ctrl(dpufd->pdev, lp_enter);
	} else {
		if (pdata->lp_ctrl != NULL)
			ret = pdata->lp_ctrl(dpufd->pdev, lp_enter);

		dpu_overlay_on_lp(dpufd);
	}

	return ret;
}

#define TIME_RANGE_TO_NEXT_VSYNC 3000000
#define DELAY_TIME_AFTER_TE 1000
#define DELAY_TIME_RANGE 500
static void dpu_esd_timing_ctrl(struct dpu_fb_data_type *dpufd)
{
	uint64_t vsync_period;
	ktime_t curr_time;
	uint64_t time_diff = 0;
	uint64_t delay_time = 0;
	uint32_t current_fps;

	if (!dpufd->panel_info.esd_timing_ctrl)
		return;

	current_fps = (dpufd->panel_info.fps == FPS_HIGH_60HZ) ? FPS_60HZ : dpufd->panel_info.fps;
	if (current_fps == 0) {
		DPU_FB_ERR("error fps %d\n", current_fps);
		return;
	}

	vsync_period = 1000000000UL / (uint64_t)current_fps; /* convert to ns from s */
	curr_time = ktime_get();
	if (ktime_to_ns(curr_time) > ktime_to_ns(dpufd->vsync_ctrl.vsync_timestamp)) {
		time_diff = ktime_to_ns(curr_time) - ktime_to_ns(dpufd->vsync_ctrl.vsync_timestamp);
		if ((vsync_period > time_diff) &&
			((vsync_period - time_diff) < TIME_RANGE_TO_NEXT_VSYNC)) {
			delay_time = (vsync_period - time_diff) / 1000 +
				DELAY_TIME_AFTER_TE; /* convert to us from ns */

			usleep_range(delay_time, delay_time + DELAY_TIME_RANGE);
			DPU_FB_INFO("vsync %llu ns, timediff %llu ns, delay %llu us",
				vsync_period, time_diff, delay_time);
		}
	}
}

static void dpu_ctrl_esd_exit(struct dpu_fb_data_type *dpufd)
{
	dpufd->esd_check_is_doing = 0;
	dpufb_vsync_disable_enter_idle(dpufd, false);
	dpufb_deactivate_vsync(dpufd);
}

int dpufb_ctrl_esd(struct dpu_fb_data_type *dpufd)
{
	struct dpu_fb_panel_data *pdata = NULL;
	int ret = 0;

	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL\n");

	pdata = dev_get_platdata(&dpufd->pdev->dev);
	dpu_check_and_return(!pdata, -EINVAL, ERR, "pdata is NULL\n");

	down(&dpufd->power_sem);

	if (!dpufd->panel_power_on) {
		DPU_FB_DEBUG("fb%d, panel power off!\n", dpufd->index);
		goto err_out;
	}

	if (!pdata->esd_handle) {
		DPU_FB_DEBUG("pdata->esd_handle is null\n");
		goto err_out;
	}

	dpufb_vsync_disable_enter_idle(dpufd, true);
	dpufb_activate_vsync(dpufd);

	dpu_esd_timing_ctrl(dpufd);
	ret = pdata->esd_handle(dpufd->pdev);
	dpu_ctrl_esd_exit(dpufd);

err_out:
	up(&dpufd->power_sem);

	return ret;
}

int dpufb_fps_upt_isr_handler(struct dpu_fb_data_type *dpufd)
{
	struct dpu_fb_panel_data *pdata = NULL;
	int ret = 0;

	if (!dpufd) {
		DPU_FB_ERR("dpufd is NULL\n");
		return -EINVAL;
	}
	pdata = dev_get_platdata(&dpufd->pdev->dev);
	if (!pdata) {
		DPU_FB_ERR("pdata is NULL\n");
		return -EINVAL;
	}

	if (!dpufd->panel_power_on) {
		DPU_FB_DEBUG("fb%d, panel power off!\n", dpufd->index);
		goto err_out;
	}

	if (pdata->lcd_fps_updt_handle != NULL)
		ret = pdata->lcd_fps_updt_handle(dpufd->pdev);

#ifdef CONFIG_INPUTHUB_20
	send_lcd_freq_to_sensorhub(dpufd->panel_info.frm_rate_ctrl.target_frame_rate);
#endif

err_out:
	return ret;
}

int dpufb_ctrl_dss_voltage_get(struct dpu_fb_data_type *dpufd, void __user *argp)
{
	int voltage_value = 0;
	dss_vote_cmd_t dss_vote_cmd;

	if (!dpufd) {
		DPU_FB_ERR("dss voltage get dpufd NULL Pointer!\n");
		return -EINVAL;
	}

	if (!argp) {
		DPU_FB_ERR("dss voltage get argp NULL Pointer!\n");
		return -EINVAL;
	}

	if (dpufd->index == EXTERNAL_PANEL_IDX) {
		DPU_FB_ERR("fb%d, dss voltage get not supported!\n", dpufd->index);
		return -EINVAL;
	}

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		if (dpufd->core_clk_upt_support == 0) {
			DPU_FB_DEBUG("no support core_clk_upt\n");
			return 0;
		}
	}
	memset(&dss_vote_cmd, 0, sizeof(dss_vote_cmd_t));

	dss_get_peri_volt(&voltage_value);

	dss_vote_cmd.dss_voltage_level = dpe_get_voltage_level(voltage_value);

	if (copy_to_user(argp, &dss_vote_cmd, sizeof(dss_vote_cmd_t))) {
		DPU_FB_ERR("copy to user fail\n");
		return -EFAULT;
	}
	DPU_FB_DEBUG("fb%d, current_peri_voltage_level = %d\n", dpufd->index, dss_vote_cmd.dss_voltage_level);

	return 0;
}

bool check_primary_panel_power_status(struct dpu_fb_data_type *dpufd)
{
	if ((dpufd->index == AUXILIARY_PANEL_IDX) && (!dpufd_list[PRIMARY_PANEL_IDX]->panel_power_on)) {
		DPU_FB_INFO("fb%d, primary_panel is power off!\n", dpufd->index);
		return false;
	}

	return true;
}

int dpufb_ctrl_dss_voltage_set(struct dpu_fb_data_type *dpufd, void __user *argp)
{
	int ret = 0;
	dss_vote_cmd_t dss_vote_cmd;
	int current_peri_voltage = 0;

	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dss voltage set dpufd NULL Pointer!\n");
	dpu_check_and_return(!argp, -EINVAL, ERR, "dss voltage set argp NULL Pointer!\n");
	dpu_check_and_return((dpufd->index == EXTERNAL_PANEL_IDX), -EINVAL, ERR,
		"fb%d, dss voltage get not supported!\n", dpufd->index);
	dpu_check_and_return((dpufd->index == MEDIACOMMON_PANEL_IDX), -EINVAL, ERR,
		"fb%d, dss voltage get not supported!\n", dpufd->index);

	if ((dpufd->index == PRIMARY_PANEL_IDX) && (dpufd->core_clk_upt_support == 0)) {
		DPU_FB_DEBUG("no support core_clk_upt\n");
		return ret;
	}

	if (!check_panel_status(dpufd))
		return -EINVAL;

	if (!check_primary_panel_power_status(dpufd))
		return -EINVAL;

	down(&g_dpufb_dss_clk_vote_sem);

	ret = copy_from_user(&dss_vote_cmd, argp, sizeof(dss_vote_cmd_t));
	if (ret) {
		DPU_FB_ERR("copy_from_user failed!ret=%d!\n", ret);
		goto volt_vote_out;
	}

	if (dss_vote_cmd.dss_voltage_level == dpufd->dss_vote_cmd.dss_voltage_level) {
		DPU_FB_DEBUG("fb%d same voltage level %d\n", dpufd->index, dss_vote_cmd.dss_voltage_level);
		goto volt_vote_out;
	}

	ret = dpufb_set_dss_vote_voltage(dpufd, dss_vote_cmd.dss_voltage_level, &current_peri_voltage);
	if (ret < 0)
		goto volt_vote_out;

	dss_vote_cmd.dss_voltage_level = dpe_get_voltage_level(current_peri_voltage);

	if (copy_to_user(argp, &dss_vote_cmd, sizeof(dss_vote_cmd_t))) {
		DPU_FB_ERR("copy to user fail\n");
		ret = -EFAULT;
		goto volt_vote_out;
	}

volt_vote_out:
	up(&g_dpufb_dss_clk_vote_sem);
	return ret;
}

int dpufb_ctrl_dss_vote_cmd_set(struct dpu_fb_data_type *dpufd, const void __user *argp)
{
	int ret = 0;
	bool status = true;
	dss_vote_cmd_t vote_cmd;

	if (!dpufd) {
		DPU_FB_ERR("dss clk rate set dpufd NULL Pointer!\n");
		return -EINVAL;
	}

	if (!argp) {
		DPU_FB_ERR("dss clk rate set argp NULL Pointer!\n");
		return -EINVAL;
	}

	if (dpufd->index == PRIMARY_PANEL_IDX) {
		if (dpufd->core_clk_upt_support == 0) {
			DPU_FB_DEBUG("no support core_clk_upt\n");
			return ret;
		}
	}

	ret = copy_from_user(&vote_cmd, argp, sizeof(dss_vote_cmd_t));
	if (ret) {
		DPU_FB_ERR("copy_from_user failed!ret=%d", ret);
		return ret;
	}

	down(&dpufd->blank_sem);
	down(&g_dpufb_dss_clk_vote_sem);

	status = check_panel_status(dpufd);
	if (!status) {
		ret = -EPERM;
		goto err_out;
	}

	ret = set_dss_vote_cmd(dpufd, vote_cmd);

err_out:
	up(&g_dpufb_dss_clk_vote_sem);
	up(&dpufd->blank_sem);

	return ret;
}

int dpu_fb_blank_panel_power_on(struct dpu_fb_data_type *dpufd)
{
	int ret;

	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL!\n");

	ret = dpufd->on_fnc(dpufd);
	if (ret == 0)
		dpufd->panel_power_on = true;

	return ret;
}

int dpu_fb_blank_panel_power_off(struct dpu_fb_data_type *dpufd)
{
	int curr_pwr_state;
	int ret;

	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL!\n");

	curr_pwr_state = dpufd->panel_power_on;
	down(&dpufd->power_sem);
	dpufd->panel_power_on = false;
	up(&dpufd->power_sem);

	dpufd->mask_layer_xcc_flag = 0;
	dpufd->hbm_is_opened = 0;
	dpufd->online_play_count = 0;

	if (dpufd->bl_cancel != NULL)
		dpufd->bl_cancel(dpufd);

	ret = dpufd->off_fnc(dpufd);
	if (ret)
		dpufd->panel_power_on = curr_pwr_state;

	return ret;
}

int dpu_fb_blank_update_tui_status(struct dpu_fb_data_type *dpufd)
{
	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL!\n");

	/* 1. if tui is running, dss should not powerdown,
	 *    because register will be writen in tui mode.
	 * 2. if tui is enable, but not running, then tui should not be ok,
	 *    send the msg that tui config fail.
	 */

	if (dpufd->secure_ctrl.secure_status == DSS_SEC_RUNNING) {
		dpufd->secure_ctrl.secure_blank_flag = 1;

		DPU_FB_INFO("wait for tui quit\n");
		return -EINVAL;
	} else if (dpufd->secure_ctrl.secure_event == DSS_SEC_ENABLE) {
		dpufd->secure_ctrl.secure_event = DSS_SEC_DISABLE;

		DPU_FB_INFO("In power down, secure event will not be handled\n");
	}

	return 0;
}

int dpu_fb_blank_device(struct dpu_fb_data_type *dpufd, int blank_mode)
{
	int ret = 0;

	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL!\n");

	if (dpufd->dp_device_srs != NULL) {
		dpufd->dp_device_srs(dpufd, (blank_mode == FB_BLANK_UNBLANK) ? true : false);
	} else {
		ret = dpu_fb_blank_sub(dpufd, blank_mode);
		if (ret != 0) {
			DPU_FB_ERR("fb%d, blank_mode %d failed!\n", dpufd->index, blank_mode);
			return ret;
		}
	}

	return ret;
}

void dpu_fb_pm_runtime_register(struct dpu_fb_data_type *dpufd)
{
	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is NULL!\n");

#ifdef CONFIG_PM_RUNTIME
	dpufd->pm_runtime_register = dpufb_pm_runtime_register;
	dpufd->pm_runtime_unregister = dpufb_pm_runtime_unregister;
	dpufd->pm_runtime_get = dpufb_pm_runtime_get;
	dpufd->pm_runtime_put = dpufb_pm_runtime_put;
#else
	dpufd->pm_runtime_register = NULL;
	dpufd->pm_runtime_unregister = NULL;
	dpufd->pm_runtime_get = NULL;
	dpufd->pm_runtime_put = NULL;
#endif
}

void dpu_fb_fnc_register_base(struct dpu_fb_data_type *dpufd)
{
	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is NULL!\n");

	dpufd->set_fastboot_fnc = NULL;
	dpufd->open_sub_fnc = NULL;
	dpufd->release_sub_fnc = NULL;
	dpufd->hpd_open_sub_fnc = NULL;
	dpufd->hpd_release_sub_fnc = NULL;
	dpufd->lp_fnc = NULL;
	dpufd->esd_fnc = NULL;
	dpufd->fps_upt_isr_handler = NULL;
	dpufd->mipi_dsi_bit_clk_upt_isr_handler = NULL;
	dpufd->sysfs_attrs_add_fnc = NULL;
	dpufd->sysfs_attrs_append_fnc = NULL;
	dpufd->sysfs_create_fnc = NULL;
	dpufd->sysfs_remove_fnc = NULL;

	dpufd->pm_runtime_register = NULL;
	dpufd->pm_runtime_unregister = NULL;
	dpufd->pm_runtime_get = NULL;
	dpufd->pm_runtime_put = NULL;
	dpufd->bl_register = NULL;
	dpufd->bl_unregister = NULL;
	dpufd->bl_update = NULL;
	dpufd->bl_cancel = NULL;
	dpufd->vsync_register = NULL;
	dpufd->vsync_unregister = NULL;
	dpufd->vsync_ctrl_fnc = NULL;
	dpufd->vsync_isr_handler = NULL;
	dpufd->buf_sync_register = NULL;
	dpufd->buf_sync_unregister = NULL;
	dpufd->buf_sync_signal = NULL;
	dpufd->buf_sync_suspend = NULL;
	dpufd->secure_register = NULL;
	dpufd->secure_unregister = NULL;
}

void dpu_fb_sdp_fnc_register(struct dpu_fb_data_type *dpufd)
{
	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is NULL\n");

	dpufd->fb_mem_free_flag = true;

	dpufd->set_fastboot_fnc = NULL;
	dpufd->open_sub_fnc = dpu_fb_open_sub;
	dpufd->release_sub_fnc = dpu_fb_release_sub;
	dpufd->hpd_open_sub_fnc = NULL;
	dpufd->hpd_release_sub_fnc = NULL;
	dpufd->lp_fnc = NULL;
	dpufd->esd_fnc = NULL;
	dpufd->fps_upt_isr_handler = NULL;
	dpufd->mipi_dsi_bit_clk_upt_isr_handler = NULL;
	dpufd->sysfs_attrs_add_fnc = NULL;

	dpufd->sysfs_attrs_append_fnc = dpufb_sysfs_attrs_append;
	dpufd->sysfs_create_fnc = dpufb_sysfs_create;
	dpufd->sysfs_remove_fnc = dpufb_sysfs_remove;

	dpu_fb_pm_runtime_register(dpufd);

	dpufd->bl_register = dpufb_backlight_register;
	dpufd->bl_unregister = dpufb_backlight_unregister;
	dpufd->bl_update = dpufb_backlight_update;
	dpufd->bl_cancel = dpufb_backlight_cancel;
	dpufd->vsync_register = dpufb_vsync_register;
	dpufd->vsync_unregister = dpufb_vsync_unregister;
	dpufd->vsync_ctrl_fnc = dpufb_vsync_ctrl;
	dpufd->vsync_isr_handler = dpufb_vsync_isr_handler;
	dpufd->vactive_start_register = dpufb_vactive_start_register;
}

void dpu_fb_mdc_fnc_register(struct dpu_fb_data_type *dpufd)
{
	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is NULL\n");

	mutex_init(&dpufd->work_lock);
	sema_init(&dpufd->media_common_sr_sem, 1);
	dpufd->media_common_composer_sr_refcount = 0;

	dpufd->fb_mem_free_flag = true;
	dpu_fb_fnc_register_base(dpufd);
}

void dpu_fb_aux_fnc_register(struct dpu_fb_data_type *dpufd)
{
	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is NULL\n");

	sema_init(&dpufd->offline_composer_sr_sem, 1);
	dpufd->offline_composer_sr_refcount = 0;

	dpufd->fb_mem_free_flag = true;

	dpu_fb_fnc_register_base(dpufd);
}

void dpu_fb_common_register(struct dpu_fb_data_type *dpufd)
{
	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is NULL\n");
	DPU_FB_INFO("fb%u +\n", dpufd->index);

	dpufd->on_fnc = dpufb_ctrl_on;
	dpufd->off_fnc = dpufb_ctrl_off;

	/* backlight register */
	if (dpufd->bl_register != NULL)
		dpufd->bl_register(dpufd->pdev);
	/* vsync register */
	if (dpufd->vsync_register != NULL)
		dpufd->vsync_register(dpufd->pdev);
	/* secure register */
	if (dpufd->secure_register != NULL)
		dpufd->secure_register(dpufd->pdev);
	/* buf_sync register */
	if (dpufd->buf_sync_register != NULL)
		dpufd->buf_sync_register(dpufd->pdev);
	/* pm runtime register */
	if (dpufd->pm_runtime_register != NULL)
		dpufd->pm_runtime_register(dpufd->pdev);
	if (dpufd->vactive_start_register != NULL)
		dpufd->vactive_start_register(dpufd->pdev);
	/* fb sysfs create */
	if (dpufd->sysfs_create_fnc != NULL)
		dpufd->sysfs_create_fnc(dpufd->pdev);

	DPU_FB_INFO("fb%u -\n", dpufd->index);
}

bool dpu_fb_img_type_valid(uint32_t fb_img_type)
{
	if ((fb_img_type == DPU_FB_PIXEL_FORMAT_BGR_565) ||
		(fb_img_type == DPU_FB_PIXEL_FORMAT_BGRX_4444) ||
		(fb_img_type == DPU_FB_PIXEL_FORMAT_BGRA_4444) ||
		(fb_img_type == DPU_FB_PIXEL_FORMAT_BGRX_5551) ||
		(fb_img_type == DPU_FB_PIXEL_FORMAT_BGRA_5551) ||
		(fb_img_type == DPU_FB_PIXEL_FORMAT_BGRA_8888) ||
		(fb_img_type == DPU_FB_PIXEL_FORMAT_YUV_422_I))
		return true;

	return false;
}

void dpu_fb_data_init(struct dpu_fb_data_type *dpufd)
{
	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is NULL\n");

	dpufd->ref_cnt = 0;
	dpufd->panel_power_on = false;
	dpufd->vr_mode = 0;
	dpufd->mask_layer_xcc_flag = 0;
	dpufd->ud_fp_scene = 0;
	dpufd->ud_fp_hbm_level = UD_FP_HBM_LEVEL;
	dpufd->ud_fp_current_level = UD_FP_CURRENT_LEVEL;
	dpufd->fb_pan_display = false;
	dpufd->is_idle_display = false;
	dpufd->fb2_irq_on_flag = false;
	dpufd->fb2_irq_force_on_flag = false;
	dpufd->dfr_create_singlethread = false;
	dpufd->base_frame_mode = 0;
}

void dpu_fb_init_sema(struct dpu_fb_data_type *dpufd)
{
	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is NULL\n");

	sema_init(&dpufd->blank_sem, 1);
	sema_init(&dpufd->blank_sem0, 1);
	sema_init(&dpufd->blank_sem_effect, 1);
	sema_init(&dpufd->blank_sem_effect_hiace, 1);
	sema_init(&dpufd->blank_sem_effect_gmp, 1);
	sema_init(&dpufd->effect_gmp_sem, 1);
	sema_init(&dpufd->brightness_esd_sem, 1);
	sema_init(&dpufd->power_sem, 1);
	sema_init(&dpufd->hiace_hist_lock_sem, 1);
	sema_init(&dpufd->esd_panel_change_sem, 1);
}

void dpu_fb_init_spin_lock(struct dpu_fb_data_type *dpufd)
{
	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is NULL\n");

	spin_lock_init(&dpufd->underflow_lock);
}

int dpu_fb_registe_callback(struct dpu_fb_data_type *dpufd)
{
	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is NULL\n");

	if (dpufd->index == PRIMARY_PANEL_IDX)
		dpu_fb_pdp_fnc_register(dpufd);
	else if (dpufd->index == EXTERNAL_PANEL_IDX)
		dpu_fb_sdp_fnc_register(dpufd);
	else if (dpufd->index == MEDIACOMMON_PANEL_IDX)
		dpu_fb_mdc_fnc_register(dpufd);
	else
		dpu_fb_aux_fnc_register(dpufd);

	return 0;
}

#define NANOSECOND_PER_SECOND 1000000000
u64 systime_get()
{
	u64 system_counter = 0;
	u32 system_couter_rate = 1;
	u64 nanosecond_stamp = 0;

	system_counter = read_sysreg(cntvct_el0);
	system_couter_rate = read_sysreg(cntfrq_el0);
	if (system_couter_rate == 0) {
		DPU_FB_ERR("system_couter_rate is 0\n");
		return 0;
	}

	nanosecond_stamp = system_counter * NANOSECOND_PER_SECOND / system_couter_rate;

	return nanosecond_stamp;
}
#pragma GCC diagnostic pop
