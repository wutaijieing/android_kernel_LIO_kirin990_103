/** @file
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
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

#ifndef DKMD_BACKLIGHT_COMMON_H
#define DKMD_BACKLIGHT_COMMON_H

#include <linux/types.h>
#include <linux/ktime.h>
#include <linux/semaphore.h>
#include <linux/workqueue.h>
#include <linux/leds.h>

enum bl_set_type {
	BL_SET_BY_NONE = BIT(0),
	BL_SET_BY_PWM = BIT(1),
	BL_SET_BY_BLPWM = BIT(2),
	BL_SET_BY_MIPI = BIT(3),
	BL_SET_BY_SH_BLPWM = BIT(4)
};

enum bl_control_mode {
	REG_ONLY_MODE = 1,
	PWM_ONLY_MODE,
	MUTI_THEN_RAMP_MODE,
	RAMP_THEN_MUTI_MODE,
	I2C_ONLY_MODE = 6,
	BLPWM_AND_CABC_MODE,
	COMMON_IC_MODE = 8,
	AMOLED_NO_BL_IC_MODE = 9,
	BLPWM_MODE = 10,
};

struct dkmd_bl_info {
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
};

struct dkmd_bl_ctrl;
struct dkmd_bl_ops {
	int (*set_backlight)(struct dkmd_bl_ctrl *bl_ctrl, uint32_t bkl_lvl);
	int (*on)(void *data);
	int (*off)(void *data);
	void (*deinit)(struct dkmd_bl_ctrl *bl_ctrl);
};

/*
 * each panel have a bl_ctrl
 */
struct dkmd_bl_ctrl {
	struct delayed_work bl_worker;
	struct semaphore bl_sem;
	uint32_t is_recovery_mode;
	uint32_t bl_led_index;

	/* after the first frame fresh to set backlight then bl_updated set 1 */
	int32_t bl_updated;
	uint32_t bl_level;
	uint32_t bl_level_old;
	ktime_t bl_timestamp;

	struct dkmd_bl_ops bl_ops;
	const struct dkmd_bl_info *bl_info;
	struct led_classdev backlight_led;
	void *connector;
	void *private_data; // for mipi, sh_blpwm, or blpwm 's private data
	const char *bl_dev_name;
};

#endif /* DKMD_BACKLIGHT_COMMON_H */
