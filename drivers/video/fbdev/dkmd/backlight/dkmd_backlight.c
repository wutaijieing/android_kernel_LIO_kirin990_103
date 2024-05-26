/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#include "dkmd_backlight.h"
#include "dkmd_log.h"
#include "peri/dkmd_peri.h"
#include "panel_drv.h"
#include "dkmd_blpwm.h"
#include "dkmd_mipi_bl.h"
#include "dkmd_sensorhub_blpwm.h"

#define DEV_NAME_LCD_BL_PREFIX "lcd_backlight"

static int dkmd_bl_set_backlight(struct dkmd_bl_ctrl *bl_ctrl, uint32_t bkl_lvl, bool enforce)
{
	struct dkmd_bl_ops *pbl = NULL;

	if (!bl_ctrl) {
		dpu_pr_err("priv is NULL\n");
		return -EINVAL;
	}

	/* before frame display only update user's value,  bl_level_old is the value before */
	if (!bl_ctrl->bl_updated && !enforce) {
		bl_ctrl->bl_level = bkl_lvl;
		dpu_pr_debug("[backlight] return: only update bl_level %d!", bkl_lvl);
		return 0;
	}

	if (bl_ctrl->bl_level_old == bkl_lvl && (!enforce)) {
		dpu_pr_debug("[backlight] return: bl_level_old %d, bl_level = %d is same to the last, enforce = %d!",
					bl_ctrl->bl_level_old, bkl_lvl, enforce);
		return 0;
	}

	dpu_pr_debug("[backlight] last_bl_level %d, new backlight level = %d\n", bl_ctrl->bl_level_old, bkl_lvl);

	bl_ctrl->bl_level = bkl_lvl;
	pbl = &bl_ctrl->bl_ops;
	if (pbl->set_backlight)
		pbl->set_backlight(bl_ctrl, bkl_lvl);

	bl_ctrl->bl_level_old = bkl_lvl;
	bl_ctrl->bl_timestamp = ktime_get();
	return 0;
}

#ifdef CONFIG_LEDS_CLASS
static void dkmd_led_set_brightness(struct led_classdev *led_cdev,
	enum led_brightness value)
{
	struct panel_drv_private *priv = NULL;
	struct dkmd_bl_ctrl *bl_ctrl = NULL;
	uint32_t bl_lvl = 0;
	uint32_t bl_max = 0;

	if (!led_cdev) {
		dpu_pr_err("[backlight] NULL pointer!\n");
		return;
	}

	priv = dev_get_drvdata(led_cdev->dev->parent);
	if (!priv) {
		dpu_pr_err("[backlight] priv NULL pointer!\n");
		return;
	}

	bl_ctrl = &priv->bl_ctrl;
	bl_max =  priv->bl_info.bl_max;
	if (value > bl_max)
		value = bl_max;

	dpu_pr_debug("[backlight] bl_max %d, user set value %d", bl_max, value);

	/* This maps android backlight level 0 to 255 into
	 * driver backlight level 0 to bl_max with rounding
	 */
	bl_lvl = (2 * value * bl_max + bl_max) / (2 * bl_max);

	if (!bl_lvl && value)
		bl_lvl = 1;

	down(&bl_ctrl->bl_sem);
	dkmd_bl_set_backlight(bl_ctrl, bl_lvl, false);
	up(&bl_ctrl->bl_sem);
}
#endif

// call led_classdev_register to register led device
void dkmd_backlight_register_led(struct platform_device *pdev, struct dkmd_bl_ctrl *bl_ctrl)
{
#ifdef CONFIG_LEDS_CLASS
	struct led_classdev *backlight_led = NULL;
	char *led_name = NULL;
	int len = sizeof(DEV_NAME_LCD_BL_PREFIX) + 2;
	int count = 0;

	if (!pdev || !bl_ctrl) {
		dpu_pr_err("[backlight] pdev is NULL\n");
		return;
	}

	backlight_led = &bl_ctrl->backlight_led;
	if (backlight_led->name != NULL) {
		dpu_pr_err("%s have been register", backlight_led->name);
		return;
	}

	led_name = kzalloc(len, GFP_KERNEL);
	if (!led_name) {
		dpu_pr_err("alloc led name fail, %d", len);
		return;
	}

	count = snprintf(led_name, len - 1, "%s%1u", DEV_NAME_LCD_BL_PREFIX, bl_ctrl->bl_led_index);
	if (count <= 0 || count > len)
		count = len;

	led_name[len - 1] = '\0';
	backlight_led->name = led_name;
	backlight_led->brightness = bl_ctrl->bl_info->bl_default;
	backlight_led->max_brightness = bl_ctrl->bl_info->bl_max;
	backlight_led->brightness_set = dkmd_led_set_brightness;

	if (led_classdev_register(&pdev->dev, backlight_led)) {
		dpu_pr_err("[backlight] led_classdev_register failed!\n");
		kfree(backlight_led->name);
		backlight_led->name = NULL;
		return;
	}
	dpu_pr_info("[backlight] create backlight dev success \n");
#endif
}

void dkmd_backlight_unregister_led(struct dkmd_bl_ctrl *bl_ctrl)
{
#ifdef CONFIG_LEDS_CLASS
	struct led_classdev *backlight_led = NULL;

	if (!bl_ctrl) {
		dpu_pr_err("[backlight] bl_ctrl is NULL\n");
		return;
	}

	backlight_led = &bl_ctrl->backlight_led;
	if (backlight_led->name != NULL) {
		led_classdev_unregister(backlight_led);
		kfree(backlight_led->name);
		backlight_led->name = NULL;
	}
#endif
}

static void dkmd_bl_workqueue_handler(struct work_struct *work)
{
	struct dkmd_bl_ctrl *pbl_ctrl = NULL;

	pbl_ctrl = container_of(to_delayed_work(work), struct dkmd_bl_ctrl, bl_worker); //lint !e666
	if (!pbl_ctrl) {
		dpu_pr_err("[backlight] pbl_ctrl is NULL\n");
		return;
	}

	if (pbl_ctrl->bl_updated)
		return;

	dpu_pr_info("[backlight] bl_level %d, is_recovery_mode %d\n",
				pbl_ctrl->bl_level, pbl_ctrl->is_recovery_mode);

	down(&pbl_ctrl->bl_sem);
	pbl_ctrl->bl_updated = 1;

	/*
	 * fix the bug:recovery interface app can not set backlight successfully
	 * if backlight not be set at recovery, driver will set default value
	 * otherwise driver not set anymore
	*/
	if (pbl_ctrl->is_recovery_mode) {
		if (pbl_ctrl->bl_level == 0)
			pbl_ctrl->bl_level = pbl_ctrl->bl_info->bl_default;
	}

	dkmd_bl_set_backlight(pbl_ctrl, pbl_ctrl->bl_level, false);
	up(&pbl_ctrl->bl_sem);
}

void dkmd_backlight_init_bl_ctrl(uint32_t bl_type, struct dkmd_bl_ctrl *bl_ctrl)
{
	struct dkmd_bl_ops *bl_ops = NULL;

	if (!bl_ctrl) {
		dpu_pr_err("bl_ctrl is null");
		return;
	}
	dpu_pr_info("[backlight] bl_type: %d \n", bl_type);

	bl_ctrl->bl_updated = 0;
	bl_ctrl->bl_level = 0;
	bl_ctrl->bl_level_old = 0;
	bl_ctrl->is_recovery_mode = 0;
	sema_init(&bl_ctrl->bl_sem, 1);
	INIT_DELAYED_WORK(&bl_ctrl->bl_worker, dkmd_bl_workqueue_handler);

	bl_ops = &bl_ctrl->bl_ops;
	switch (bl_type) {
	case BL_SET_BY_BLPWM:
		dkmd_blpwm_init(bl_ctrl);
		break;
	case BL_SET_BY_MIPI:
		dkmd_mipi_bl_init(bl_ctrl);
		break;
	case BL_SET_BY_SH_BLPWM:
		dkmd_sh_blpwm_init(bl_ctrl);
		break;
	default:
		dpu_pr_err("unsupport bl type %u", bl_type);
		return;
	}
}

void dkmd_backlight_deinit_bl_ctrl(struct dkmd_bl_ctrl *bl_ctrl)
{
	if (unlikely(bl_ctrl == NULL)) {
		dpu_pr_err("bl_ctrl is null");
		return;
	}

	if (bl_ctrl->bl_ops.deinit)
		bl_ctrl->bl_ops.deinit(bl_ctrl);
}

#define MSEC_PER_FRAME  17
void dkmd_backlight_update(struct dkmd_bl_ctrl *bl_ctrl, bool enforce)
{
	unsigned long backlight_duration = 0;

	if (!bl_ctrl) {
		dpu_pr_err( "[backlight] bl_ctrl is NULL\n");
		return;
	}

	dpu_pr_debug( "[backlight] bkl_lvl %d, bl_updated %d enforce = %d",
				bl_ctrl->bl_level, bl_ctrl->bl_updated, enforce);

	if (enforce) {
		down(&bl_ctrl->bl_sem);
		dkmd_bl_set_backlight(bl_ctrl, bl_ctrl->bl_level, true);
		up(&bl_ctrl->bl_sem);
		return;
	}

	/* make sure only after the first frame refresh, backlight will be set */
	if (!bl_ctrl->bl_updated) {
		if (bl_ctrl->bl_info->delay_set_bl_support)
			backlight_duration = bl_ctrl->bl_info->delay_set_bl_thr * MSEC_PER_FRAME;

		schedule_delayed_work(&bl_ctrl->bl_worker, msecs_to_jiffies(backlight_duration));
		dpu_pr_info("backlight_duration = %lu", backlight_duration);
	} else {
		down(&bl_ctrl->bl_sem);
		dkmd_bl_set_backlight(bl_ctrl, bl_ctrl->bl_level, false);
		up(&bl_ctrl->bl_sem);
	}
}

void dkmd_backlight_cancel(struct dkmd_bl_ctrl *bl_ctrl)
{
	if (!bl_ctrl) {
		dpu_pr_err("[backlight] bl_ctrl is NULL\n");
		return;
	}
	dpu_pr_info("[backlight] bl_updated 0 and level 0\n");

	cancel_delayed_work(&bl_ctrl->bl_worker);
	bl_ctrl->bl_updated = 0;
	bl_ctrl->bl_level = 0;
	bl_ctrl->bl_level_old = 0;
}

MODULE_LICENSE("GPL");

