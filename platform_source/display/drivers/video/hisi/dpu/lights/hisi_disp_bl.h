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

#ifndef HISI_DISP_BACKLIGHT_H
#define HISI_DISP_BACKLIGHT_H

#include <linux/types.h>
#include <linux/semaphore.h>
#include <linux/platform_device.h>

#define DEV_NAME_LCD_BL "lcd_backlight0"
struct hisi_connector_device;

struct hisi_panel_bl_info {
	/* backlight info get from different lcd */
	uint32_t bl_type;
	uint32_t bl_min;
	uint32_t bl_max;
	uint32_t bl_default;
	/* support delay set backlight threshold */
	uint32_t delay_set_bl_support;
	/* delay set backlight threshold */
	uint32_t delay_set_bl_thr;

	uint32_t blpwm_precision_type;
	uint32_t bl_ic_ctrl_mode;
	/* todo */
};

struct hisi_bl_ops {
	int (*set_backlight)(struct platform_device *pdev, uint32_t level);
};

struct hisi_bl_ctrl {
	/* after the first frame fresh to set backlight then bl_updated set 1*/
	int bl_updated;
	uint32_t bl_level;

	struct delayed_work bl_worker;
	struct semaphore bl_sem;
	struct hisi_bl_ops *bl_ops;
	ktime_t bl_timestamp;
};

extern unsigned int get_boot_into_recovery_flag(void);

void hisi_backlight_register(struct hisi_connector_device *connector_dev);
void hisi_backlight_unregister(struct hisi_connector_device *connector_dev);
void hisi_backlight_update(struct hisi_connector_device *connector_dev, uint32_t level,bool enforce);
void hisi_backlight_cancel(struct hisi_connector_device *connector_dev);

#endif /* HISI_DISP_BACKLIGHT_H */
