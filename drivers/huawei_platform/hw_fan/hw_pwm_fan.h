// SPDX-License-Identifier: GPL-2.0
/*
 * hw_pwm_fan.h
 *
 * driver for hw_pwm_fan
 *
 * Copyright (c) 2022-2022 Huawei Technologies Co., Ltd.
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

#ifndef _HW_PWM_FAN_H_
#define _HW_PWM_FAN_H_

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/pwm.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/mm.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/pinctrl/consumer.h>
#include <huawei_platform/log/hw_log.h>

#define PERRSTEN2                      (0x078)
#define PERRSTDIS2                     (0x07C)
#define PEREN2                         (0x020)

#define BLPWM_IN_DIV                    0x53
#define BLPWM_IN_PWM_MIN                40
#define BLPWM_IN_PWM_MAX                60
#define BLM_FREQ_DEAULT_RATIO           1

/* BLPWM input address */
#define PWM_IN_CTRL_OFFSET              (0x000)
#define PWM_IN_DIV_OFFSET               (0x004)
#define PWM_IN_NUM_OFFSET               (0x008)
#define PWM_IN_DEBUG_NUM_OFFSET         (0x00C)
#define PWM_IN_MAX_COUNTER_OFFSET       (0x010)
#define PWM_IN_MAX_COUNTER_VAL          (0xFFFF)

/* BLpwm reg address */
#define BLPWM_OUT_CTL                   (0x0100)
#define BLPWM_OUT_DIV                   (0X0104)
#define BLPWM_OUT_CFG                   (0x0108)

/* pwm reg address */
#define PWM_LOCK_OFFSET                 (0x0000)
#define PWM_CTL_OFFSET                  (0X0004)
#define PWM_CFG_OFFSET                  (0x0008)
#define PWM_PR0_OFFSET                  (0x0100)
#define PWM_PR1_OFFSET                  (0x0104)
#define PWM_C0_MR_OFFSET                (0x0300)
#define PWM_C0_MR0_OFFSET               (0x0304)
#define PWM_C1_MR_OFFSET                (0x400)
#define PWM_C1_MR0_OFFSET               (0x0404)

#define PWM_CTRL_LOCK                   0x1acce551
#define PWM_OUT_PRECISION               (800)
#define PWM_OUT_MAX                     100
#define PWM_OUT_MIN                     30
#define PWM_DUTY_MIN                    0
#define PWM_OUT_DUTY_START_WORK	        240
#define PWM_OUT_DIV_PR0                 1
#define PWM_DUTY_PERCENT                100
#define PWM_WORD_OFFSET                 16

/* default pwm clk */
#define PWM_CLK_DEFAULT_RATE            (80 * 1000000L)

/* default blpwm clk */
#define BLPWM_CLK_DEFAULT_RATE          (114 * 1000000L)

#define PAGE_SIZE                       1024

#define RETRY_WORK_TIMEOUT              200
#define RETRY_MAX_COUNT                 3
#define SPEED_CHANGED_WORK_TIMEOUT      2000
#define REGULATING_WORK_TIMEOUT         1000
#define REGULATING_SPEED_TIMEOUT        100
#define REGULATE_SPEED_RANGE            150
#define FINE_TUNING_SPEED_RANGE         100

#define FAN_BOOT_INIT_SPEED             0
#define FAN_STATUS_CHECK_COUNT          200
#define FAN_PWM_DUTY_MIN                1
#define FAN_PWM_DUTY_MAX                100
#define FAN_MIN_SPEED                   2000
#define FAN_BLPWM_TOTAL_CYCLE           1000
#define FAN_WORK_FREQ_DEFAULT           25000
#define FAN_FREQ_10KHZ                  10000
#define FAN_TIME_MINUTE                 600000
#define FAN_SLEEP_TIME                  50
#define FAN_PWM_DUTY_PARA               0
#define FAN_DEST_SPEED_PARA             1
#define FAN_SPEED_PARA_TOTAL            2
#define FAN_SPEED_PARA_LEVEL            20
#define FAN_SWITCH_OFF_CMD              (-1)

#define DTS_COMP_FAN_NAME               "huawei,fan_pwm"

/* pinctrl operation */
enum {
	DTYPE_PINCTRL_GET,
	DTYPE_PINCTRL_STATE_GET,
	DTYPE_PINCTRL_SET,
	DTYPE_PINCTRL_PUT,
};

/* pinctrl state */
enum {
	DTYPE_PINCTRL_STATE_DEFAULT,
	DTYPE_PINCTRL_STATE_IDLE,
};

enum {
	FAN_WORKING_NORMAL = 0,
	FAN_DISCONNCET,
	FAN_ABNORMAL_WORKING,
};

enum {
	PWM_OUT0_MODE = 0,
	PWM_OUT1_MODE,
	BLPWM_OUT_MODE,
};

/* pinctrl data */
struct pinctrl_data {
	struct pinctrl *p;
	struct pinctrl_state *pinctrl_def;
	struct pinctrl_state *pinctrl_idle;
};
struct pinctrl_cmd_desc {
	int dtype;
	struct pinctrl_data *pctrl_data;
	int mode;
};

struct fan_speed_map {
	u8 pwm_duty;
	int dest_speed;
};

struct pwm_fan_data {
	struct clk *pwm_clk;
	struct clk *blpwm_clk;
	struct platform_device *pwm_pdev;
	struct delayed_work speed_regulate_work;
	struct mutex lock;
	struct fan_speed_map speed_map[FAN_SPEED_PARA_LEVEL];
	char __iomem *pwm_base;
	char __iomem *blpwm_base;
	char __iomem *peri_crg_base;
	uint8_t pwmout_speed;
	int speed_map_level;
	int pwm_on;
	int pwmout_duty;
	int gpio_power;
	int cur_speed;
	int dest_speed;
	int last_speed;
	int same_count_val;
	int fan_status;
	int retry;
	int pwm_mode;
	int blpwm_freq_ratio;
	int blpwm_clk_rate;
	bool dest_changed;
	uint32_t fan_work_freq;
	uint32_t freq_out_div;
	bool power_enable;
};

#endif /* _HW_PWM_FAN_H_ */
