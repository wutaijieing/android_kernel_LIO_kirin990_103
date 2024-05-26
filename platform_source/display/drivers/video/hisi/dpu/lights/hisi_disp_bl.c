/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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
#include <linux/workqueue.h>
#include <linux/types.h>
#include <linux/leds.h>
#include "hisi_connector_utils.h"
#include "hisi_disp_debug.h"
#include "hisi_disp_bl.h"

static int lcd_backlight_registered;
static unsigned int is_recovery_mode;
static int is_no_fastboot_bl_enable;

#define MSEC_PER_FRAME  17

static bool is_fastboot_display_enable(void)
{
	/* read from dtsi */
	return true;
}

void _hisi_set_backlight(struct hisi_connector_device *connector_dev, uint32_t bkl_lvl, bool enforce)
{
	uint32_t temp = bkl_lvl;
	struct hisi_bl_ops *bl_ops = NULL;
	struct hisi_bl_ctrl *bl_ctrl = NULL;
	struct hisi_panel_info *fix_panel_info = NULL;

	if (!connector_dev) {
		disp_pr_err("connector_dev is NULL\n");
		return;
	}

	bl_ctrl = &connector_dev->bl_ctrl;
	bl_ops = bl_ctrl->bl_ops;
	if (bl_ops->set_backlight) {
		if (bl_ctrl->bl_level == temp && !enforce) {
			disp_pr_info("set_bl return: bl_level = %d is same to the last, enforce = %d!\n",temp,enforce);
			return;
		}

		if (bl_ctrl->bl_level == 0) {
			disp_pr_info("new backlight level = %d \n", bkl_lvl);
		}

		bl_ctrl->bl_level = bkl_lvl;

		fix_panel_info = connector_dev->fix_panel_info;

		if (fix_panel_info->bl_info.bl_type == BL_SET_BY_MIPI) {
			/* todo vsync */
		}

		if (bl_ops->set_backlight) {
			bl_ops->set_backlight(connector_dev->pdev, bkl_lvl);
		}

		connector_dev->bl_ctrl.bl_timestamp = ktime_get();
	}
}
static void hisi_bl_set_brightness(struct led_classdev *led_cdev,
	enum led_brightness value)
{
	struct hisi_connector_device *connector_dev = NULL;
	struct hisi_panel_info *fix_panel_info = NULL;
	int bl_lvl = 0;
	uint32_t bl_max = 0;

	if (!led_cdev) {
		disp_pr_err("NULL pointer!\n");
		return;
	}

	connector_dev = dev_get_drvdata(led_cdev->dev->parent);
	if (!connector_dev) {
		disp_pr_err("connector_dev NULL pointer!\n");
		return;
	}

	fix_panel_info = connector_dev->fix_panel_info;
	bl_max =  fix_panel_info->bl_info.bl_max;

	if (value > bl_max)
		value = bl_max;

	/* This maps android backlight level 0 to 255 into
	   driver backlight level 0 to bl_max with rounding */
	bl_lvl = (2 * value * bl_max + bl_max)/(2 * bl_max);
	if (!bl_lvl && value)
		bl_lvl = 1;
	down(&connector_dev->brightness_esd_sem);
	_hisi_set_backlight(connector_dev, bl_lvl, false);
	up(&connector_dev->brightness_esd_sem);
}


static struct led_classdev backlight_led = {
	.name = DEV_NAME_LCD_BL,
	.brightness_set = hisi_bl_set_brightness,
};

static void hisi_bl_workqueue_handler(struct work_struct *work)
{
	struct hisi_connector_device *connector_dev = NULL;
	struct hisi_panel_info *fix_panel_info = NULL;
	struct hisi_bl_ctrl *pbl_ctrl = NULL;
	pbl_ctrl = container_of(to_delayed_work(work), struct hisi_bl_ctrl, bl_worker); //lint !e666
	if (!pbl_ctrl) {
		disp_pr_err("pbl_ctrl is NULL\n");
		return;
	}

	connector_dev = container_of(pbl_ctrl, struct hisi_connector_device, bl_ctrl);
	if (!connector_dev) {
		disp_pr_err("connector_dev is NULL\n");
		return;
	}

	fix_panel_info = connector_dev->fix_panel_info;

	if (!connector_dev->bl_ctrl.bl_updated) {
		down(&connector_dev->brightness_esd_sem);
		connector_dev->bl_ctrl.bl_updated = 1;
		if (is_recovery_mode) {
			/*
			*fix the bug:recovery interface app can not set backlight successfully
			*if backlight not be set at recovery,driver will set default value
			*otherwise driver not set anymore
			*/
			if(connector_dev->bl_ctrl.bl_level == 0) {
				connector_dev->bl_ctrl.bl_level = fix_panel_info->bl_info.bl_default;
			}
		} else {
			if (!is_fastboot_display_enable() && !is_no_fastboot_bl_enable) {
				is_no_fastboot_bl_enable = 1;
				connector_dev->bl_ctrl.bl_level = fix_panel_info->bl_info.bl_default;
			}
		}

		_hisi_set_backlight(connector_dev, connector_dev->bl_ctrl.bl_level, false);
		up(&connector_dev->brightness_esd_sem);
	}
}

void hisi_backlight_register(struct hisi_connector_device *connector_dev)
{
	struct platform_device *pdev = NULL;
	struct hisi_panel_info *panel_info = NULL;
	struct hisi_bl_ctrl bl_ctrl;
	disp_pr_err("++++\n");
	if (!connector_dev) {
		disp_pr_err("connector_dev is NULL\n");
		return;
	}

	panel_info = connector_dev->fix_panel_info;
	if (!panel_info) {
		disp_pr_err("panel_info is NULL\n");
		return;
	}

	bl_ctrl = connector_dev->bl_ctrl;

	bl_ctrl.bl_updated = 0;
	bl_ctrl.bl_level = 0;
	sema_init(&connector_dev->bl_ctrl.bl_sem, 1);

	INIT_DELAYED_WORK(&connector_dev->bl_ctrl.bl_worker, hisi_bl_workqueue_handler);

	if (lcd_backlight_registered)
		return;

	is_recovery_mode = get_boot_into_recovery_flag();

	backlight_led.brightness = panel_info->bl_info.bl_default;
	backlight_led.max_brightness = panel_info->bl_info.bl_max;
	pdev = connector_dev->pdev;

#ifdef CONFIG_LEDS_CLASS
	if (led_classdev_register(&pdev->dev, &backlight_led)) {
		disp_pr_err("led_classdev_register failed!\n");
		return;
	}
#endif

	lcd_backlight_registered = 1;
}

void hisi_backlight_unregister(struct hisi_connector_device *connector_dev)
{
	if (!connector_dev) {
		disp_pr_err("connector_dev is NULL\n");
		return;
	}

	if (lcd_backlight_registered) {
		lcd_backlight_registered = 0;

#ifdef CONFIG_LEDS_CLASS
	led_classdev_unregister(&backlight_led);
#endif
	}
}

void hisi_backlight_update(struct hisi_connector_device *connector_dev, uint32_t bkl_lvl, bool enforce)
{
	unsigned long backlight_duration = 2 * MSEC_PER_FRAME;  /* default delay two frame */
	struct hisi_panel_info *fix_panel_info = NULL;

	if (!connector_dev) {
		disp_pr_err( "connector_dev is NULL\n");
		return;
	}

	fix_panel_info = connector_dev->fix_panel_info;

	if (fix_panel_info->bl_info.delay_set_bl_support)
		backlight_duration = fix_panel_info->bl_info.delay_set_bl_thr * MSEC_PER_FRAME;

	if (enforce) {
		 _hisi_set_backlight(connector_dev,bkl_lvl,enforce);
	} else {
		/* make sure after the first frame refresh, it will not call */
		if (!connector_dev->bl_ctrl.bl_updated) {
			connector_dev->bl_ctrl.bl_level = bkl_lvl;
			schedule_delayed_work(&connector_dev->bl_ctrl.bl_worker, msecs_to_jiffies(backlight_duration));
			disp_pr_info("backlight_duration = %lu\n", backlight_duration);
		}
	}
}

void hisi_backlight_cancel(struct hisi_connector_device *connector_dev)
{
	struct hisi_bl_ops *bl_ops = NULL;

	if (!connector_dev) {
		disp_pr_err("connector_dev is NULL\n");
		return;
	}

	cancel_delayed_work(&connector_dev->bl_ctrl.bl_worker);
	connector_dev->bl_ctrl.bl_updated = 0;
	connector_dev->bl_ctrl.bl_level = 0;

	bl_ops = connector_dev->bl_ctrl.bl_ops;
	if(bl_ops->set_backlight) {
		bl_ops->set_backlight(connector_dev->pdev,connector_dev->bl_ctrl.bl_level);
	}

}

