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
#ifndef DPU_FB_ENUM_H
#define DPU_FB_ENUM_H

#include <linux/console.h>
#include <linux/uaccess.h>
#include <linux/leds.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/raid/pq.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/time.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/version.h>
#include <linux/pwm.h>
#include <linux/pm_runtime.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/memblock.h>
#include <linux/syscalls.h>

#include <linux/spi/spi.h>

#include <linux/platform_drivers/mm_ion.h>
#include <linux/gpio.h>

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>

#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/pinctrl/consumer.h>
#include <linux/file.h>
#include <linux/dma-buf.h>
#include <linux/genalloc.h>
#include <linux/atomic.h>

#include "dpu_enum.h"
#include <soc_dss_interface.h>
#include "dpu_fb_debug.h"
#include "dpu_fb_config.h"
#include "overlay/dpu_overlay_utils_struct.h"
#include "overlay/dpu_overlay_utils_dssv510.h"

/* V_FRONT_PORCH has 19 bits in V346
 * V_FRONT_PORCH has 9 bits in other platforms
 */

#define V_FRONT_PORCH_MAX 1023

#define CONFIG_DPU_FB_BACKLIGHT_DELAY

#define CONFIG_BACKLIGHT_2048

#define DPU_COMPOSER_HOLD_TIME (1000UL * 3600 * 24 * 5)

#define DPU_FB0_NUM 3
#define DPU_FB1_NUM 0
#define DPU_FB2_NUM 0
#define DPU_FB3_NUM 0

#define DPU_FB_SYSFS_ATTRS_NUM 64

#define DPU_FB_MAX_DEV_LIST 32
#define DPU_FB_MAX_FBI_LIST 32

#define DPU_OFFLINE_MAX_NUM 2
#define DPU_OFFLINE_MAX_LIST 128
#define ONLINE_PLAY_LOG_PRINTF 10
#define BACKLIGHT_LOG_PRINTF 16
#define ARSR_POST_COLUMN_PADDING_NUM 4
#define MAX_MDC_CHANNEL 3

#define round1(x, y)  (((y) == 0) ? (x) : ((x) / (y) + ((x) % (y) ? 1 : 0)))
#define dss_reduce(x) ((x) > 0 ? ((x) - 1) : (x))

#define _effect_debug_log(message, ...) \
	do { printk("%s: "message, __func__, ## __VA_ARGS__); } while (0)
#define effect_debug_log(feature, message, ...) \
	do { if (g_debug_effect & feature) \
		printk("%s: "message, __func__, ## __VA_ARGS__); } while (0)

/* esd check period-->5000ms */
#define ESD_CHECK_TIME_PERIOD 5000
/* emi protect period-->120000ms */
#define DPU_EMI_PROTECT_CHECK_MAX_COUNT (120000 / ESD_CHECK_TIME_PERIOD)

#define DSM_CREATE_FENCE_FAIL_EXPIRE_COUNT 6

/* UD fingerprint */
#define UD_FP_HBM_LEVEL 993
#define UD_FP_CURRENT_LEVEL 500

/*
 * If vsync delay is enable, the vsync_delay_count can not be set to 0.
 * It is recommended to set to a small value to avoid the abnormality of vsync_delay function
 */
#define VSYNC_DELAY_TIME_DEFAULT 0x01

enum mask_layer_state {
	MASK_LAYER_COMMON_STATE = 0x01, /* all the other scene except screen-on */
	MASK_LAYER_SCREENON_STATE = 0x02, /* when screen on, add mask layer and hbm */
	CIRCLE_LAYER_STATE = 0x04, /* circle layer */
};

enum mask_layer_change_status {
	MASK_LAYER_NO_CHANGE = 0x00, /* mask layer status is no change */
	MASK_LAYER_COMMON_ADDED = 0x01, /* when mask layer is added (not screenon) */
	MASK_LAYER_SCREENON_ADDED = 0x02, /* when mask layer is added (screen-on) */
	MASK_LAYER_REMOVED = 0x03, /* when mask layer is removed */
	CIRCLE_LAYER_ADDED = 0x04, /* when circle layer is added */
	CIRCLE_LAYER_REMOVED = 0x05, /* when circle layer is removed */
};

enum dss_sec_event {
	DSS_SEC_DISABLE = 0,
	DSS_SEC_ENABLE,
};

enum dss_sec_status {
	DSS_SEC_IDLE = 0,
	DSS_SEC_RUNNING,
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

enum ESD_RECOVER_STATE {
	ESD_RECOVER_STATE_NONE = 0,
	ESD_RECOVER_STATE_START = 1,
	ESD_RECOVER_STATE_COMPLETE = 2,
};

enum DPP_BUF_WORK_MODE {
	DPP_BUF_WORK_MODE_ROTATION = 0,
	DPP_BUF_WORK_MODE_BOTH_EFFECT = 1,
};

enum {
	PIPE_SWITCH_CONNECT_DSI0 = 0,
	PIPE_SWITCH_CONNECT_DSI1 = 1,
};

enum {
	FREE = 0,
	HWC_USED = 1,
	MDC_USED = 2,
};

#endif /* DPU_FB_ENUM_H */
