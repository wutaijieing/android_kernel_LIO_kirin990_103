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

#include "dpu_fb.h"
#include <linux/leds.h>

#ifdef CONFIG_HW_ZEROHUNG
#include <chipset_common/hwzrhung/hung_wp_screen.h>
#endif

static int lcd_backlight_registered;
static unsigned int is_recovery_mode;
static int is_no_fastboot_bl_enable;

#define MSEC_PER_FRAME 17
void dpufb_set_backlight(struct dpu_fb_data_type *dpufd, uint32_t bkl_lvl, bool enforce)
{
	struct dpu_fb_panel_data *pdata = NULL;
	uint32_t temp = bkl_lvl;

	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is NULL\n");
	dpu_check_and_no_retval(!dpufd->pdev, ERR, "dpufd->pdev is NULL\n");

	pdata = dev_get_platdata(&dpufd->pdev->dev);
	dpu_check_and_no_retval(!pdata, ERR, "pdata is NULL\n");

	if (!dpufd->panel_power_on || !dpufd->backlight.bl_updated) {
		dpufd->bl_level = bkl_lvl;
		DPU_FB_INFO("set_bl return: panel_power_on = %d, backlight.bl_updated = %d! bkl_lvl = %d\n",
			dpufd->panel_power_on, dpufd->backlight.bl_updated, bkl_lvl);
		return;
	}

	if (pdata->set_backlight) {
		if (dpufd->backlight.bl_level_old == (int)temp && !enforce) {
			dpufd->bl_level = bkl_lvl;
			DPU_FB_INFO("set_bl return: bl_level_old = %d, current_bl_level = %d, enforce = %d!\n",
				dpufd->backlight.bl_level_old, temp, enforce);
			return;
		}

#ifdef CONFIG_HW_ZEROHUNG
		hung_wp_screen_setbl(temp);
#endif
		if (dpufd->backlight.bl_level_old == 0)
			DPU_FB_INFO("backlight level = %d\n", bkl_lvl);

		dpufd->bl_level = bkl_lvl;

		if (dpufd->panel_info.bl_set_type & BL_SET_BY_MIPI) {
			dpufb_set_vsync_activate_state(dpufd, true);
			dpufb_activate_vsync(dpufd);
		}

		pdata->set_backlight(dpufd->pdev, bkl_lvl);

		if (dpufd->panel_info.bl_set_type & BL_SET_BY_MIPI) {
			dpufb_set_vsync_activate_state(dpufd, false);
			dpufb_deactivate_vsync(dpufd);
			if (bkl_lvl > 0)
				dpufd->secure_ctrl.have_set_backlight = true;
			else
				dpufd->secure_ctrl.have_set_backlight = false;
		}
		dpufd->backlight.bl_timestamp = ktime_get();
		dpufd->backlight.bl_level_old = temp;
	}
}

#ifdef CONFIG_DPU_FB_BACKLIGHT_DELAY
static void dpufb_bl_workqueue_handler(struct work_struct *work)
#else
static void dpufb_bl_workqueue_handler(struct dpu_fb_data_type *dpufd)
#endif
{
	struct dpu_fb_panel_data *pdata = NULL;
#ifdef CONFIG_DPU_FB_BACKLIGHT_DELAY
	struct dpufb_backlight *pbacklight = NULL;
	struct dpu_fb_data_type *dpufd = NULL;

	pbacklight = container_of(to_delayed_work(work), struct dpufb_backlight, bl_worker);  /*lint !e666*/
	dpu_check_and_no_retval(!pbacklight, ERR, "pbacklight is NULL\n");

	dpufd = container_of(pbacklight, struct dpu_fb_data_type, backlight);
#endif

	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is NULL\n");

	pdata = dev_get_platdata(&dpufd->pdev->dev);
	if (!pdata) {
		DPU_FB_ERR("pdata is NULL\n");
		return;
	}

	if (dpufd->online_play_count < ONLINE_PLAY_LOG_PRINTF)
		DPU_FB_INFO("[online_play_count = %d] begin bl_updated = %d, frame_updated = %d.\n",
			dpufd->online_play_count, dpufd->backlight.bl_updated, dpufd->backlight.frame_updated);

	if (!dpufd->backlight.bl_updated) {
		down(&dpufd->blank_sem);

		if (dpufd->backlight.frame_updated == 0) {
			up(&dpufd->blank_sem);
			return;
		}

		dpufd->backlight.frame_updated = 0;
		down(&dpufd->brightness_esd_sem);

		dpufd->backlight.bl_updated = 1;
		if (dpufd->online_play_count < ONLINE_PLAY_LOG_PRINTF)
			DPU_FB_INFO("[online_play_count = %d] set bl_updated = 1.\n", dpufd->online_play_count);

		if (is_recovery_mode) {
			/*
			 * fix the bug:recovery interface app can not set backlight successfully
			 * if backlight not be set at recovery,driver will set default value
			 * otherwise driver not set anymore
			 */
			if (dpufd->bl_level == 0)
				dpufd->bl_level = dpufd->panel_info.bl_default;
		}

		if (is_recovery_mode == 0 && !is_fastboot_display_enable() &&
			(is_no_fastboot_bl_enable == 0)) {
			is_no_fastboot_bl_enable = 1;
			dpufd->bl_level = dpufd->panel_info.bl_default;
		}

		dpufb_set_backlight(dpufd, dpufd->bl_level, false);
		up(&dpufd->brightness_esd_sem);
		up(&dpufd->blank_sem);
	}
	if (dpufd->online_play_count < ONLINE_PLAY_LOG_PRINTF)
		DPU_FB_INFO("[online_play_count = %d] end bl_updated = %d, frame_updated = %d.\n",
			dpufd->online_play_count, dpufd->backlight.bl_updated, dpufd->backlight.frame_updated);
}

void dpufb_backlight_update(struct dpu_fb_data_type *dpufd)
{
	struct dpu_fb_panel_data *pdata = NULL;
	unsigned long backlight_duration = 2 * MSEC_PER_FRAME; /* default delay two frame */

	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is null!\n");
	dpu_check_and_no_retval(!dpufd->pdev, ERR, "dpufd->pdev is null!\n");

	pdata = dev_get_platdata(&dpufd->pdev->dev);
	dpu_check_and_no_retval(!pdata, ERR, "pdata is null!\n");

	if (dpufd->online_play_count < ONLINE_PLAY_LOG_PRINTF)
		DPU_FB_INFO("[online_play_count = %d] panel_power_on = %d, bl_updated = %d.\n",
			dpufd->online_play_count, dpufd->panel_power_on, dpufd->backlight.bl_updated);

	if (!dpufd->backlight.bl_updated && dpufd->panel_power_on) {
		dpufd->backlight.frame_updated = 1;
#ifdef CONFIG_DPU_FB_BACKLIGHT_DELAY
		if (dpufd->panel_info.delay_set_bl_thr_support)
			backlight_duration = dpufd->panel_info.delay_set_bl_thr * MSEC_PER_FRAME;

		schedule_delayed_work(&dpufd->backlight.bl_worker, msecs_to_jiffies(backlight_duration));
#else
		dpufb_bl_workqueue_handler(dpufd);
#endif

		DPU_FB_INFO("backlight_duration = %lu\n", backlight_duration);
	}
}

void dpufb_backlight_cancel(struct dpu_fb_data_type *dpufd)
{
	struct dpu_fb_panel_data *pdata = NULL;

	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is null!\n");
	dpu_check_and_no_retval(!dpufd->pdev, ERR, "dpufd->pdev is null!\n");

	pdata = dev_get_platdata(&dpufd->pdev->dev);
	dpu_check_and_no_retval(!pdata, ERR, "pdata is null!\n");

#ifdef CONFIG_DPU_FB_BACKLIGHT_DELAY
	cancel_delayed_work(&dpufd->backlight.bl_worker);
#endif
	dpufd->backlight.bl_updated = 0;
	dpufd->backlight.bl_level_old = 0;
	dpufd->backlight.frame_updated = 0;
	if (dpufd->online_play_count < ONLINE_PLAY_LOG_PRINTF)
		DPU_FB_INFO("[online_play_count = %d] set bl_updated = 0.\n", dpufd->online_play_count);

	if (pdata->set_backlight) {
		dpufd->bl_level = 0;
#ifdef CONFIG_HW_ZEROHUNG
		hung_wp_screen_setbl(dpufd->bl_level);
#endif
		if (dpufd->panel_info.bl_set_type & BL_SET_BY_MIPI)
			dpufb_activate_vsync(dpufd);
		pdata->set_backlight(dpufd->pdev, dpufd->bl_level);
		if (dpufd->panel_info.bl_set_type & BL_SET_BY_MIPI)
			dpufb_deactivate_vsync(dpufd);
	}
}

static void dpu_fb_set_bl_brightness(struct led_classdev *led_cdev,
	enum led_brightness value)
{
	struct dpu_fb_data_type *dpufd = NULL;
	int bl_lvl;

	if (!led_cdev) {
		DPU_FB_ERR("NULL pointer!\n");
		return;
	}

	dpufd = dev_get_drvdata(led_cdev->dev->parent);
	if (!dpufd) {
		DPU_FB_ERR("NULL pointer!\n");
		return;
	}

	if (value > dpufd->panel_info.bl_max)
		value = dpufd->panel_info.bl_max;

	/* This maps android backlight level 0 to 255 into */
	/* driver backlight level 0 to bl_max with rounding */
	bl_lvl = (2 * value * dpufd->panel_info.bl_max + dpufd->panel_info.bl_max) / (2 * dpufd->panel_info.bl_max);
	if (!bl_lvl && value)
		bl_lvl = 1;
	down(&dpufd->brightness_esd_sem);
	dpufb_set_backlight(dpufd, bl_lvl, false);
	up(&dpufd->brightness_esd_sem);
}

static struct led_classdev backlight_led = {
	.name = DEV_NAME_LCD_BKL,
	.brightness_set = dpu_fb_set_bl_brightness,
};

void dpufb_backlight_register(struct platform_device *pdev)
{
	struct dpu_fb_data_type *dpufd = NULL;

	if (!pdev) {
		DPU_FB_ERR("pdev is NULL\n");
		return;
	}

	dpufd = platform_get_drvdata(pdev);
	if (!dpufd) {
		dev_err(&pdev->dev, "dpufd is NULL");
		return;
	}

	dpufd->backlight.bl_updated = 0;
	dpufd->backlight.frame_updated = 0;
	dpufd->backlight.bl_level_old = 0;
	sema_init(&dpufd->backlight.bl_sem, 1);
#ifdef CONFIG_DPU_FB_BACKLIGHT_DELAY
	INIT_DELAYED_WORK(&dpufd->backlight.bl_worker, dpufb_bl_workqueue_handler);
#endif

	if (lcd_backlight_registered)
		return;

	backlight_led.brightness = dpufd->panel_info.bl_default;
	backlight_led.max_brightness = dpufd->panel_info.bl_max;
	/* android supports only one lcd-backlight/lcd for now */

	lcd_backlight_registered = 1;
}

void dpufb_backlight_unregister(struct platform_device *pdev)
{
	struct dpu_fb_data_type *dpufd = NULL;

	if (!pdev) {
		DPU_FB_ERR("pdev is NULL\n");
		return;
	}
	dpufd = platform_get_drvdata(pdev);
	if (!dpufd) {
		dev_err(&pdev->dev, "dpufd is NULL\n");
		return;
	}

	if (lcd_backlight_registered)
		lcd_backlight_registered = 0;
}
