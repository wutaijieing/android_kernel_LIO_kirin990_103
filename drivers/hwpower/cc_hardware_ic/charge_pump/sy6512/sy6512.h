/* SPDX-License-Identifier: GPL-2.0 */
/*
 * sy6512.h
 *
 * charge-pump sy6512 macro, addr etc.
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

#ifndef _SY6512_H_
#define _SY6512_H_

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/of_gpio.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/interrupt.h>
#include <linux/irq.h>

/* Enable Register */
#define SY6512_ENABLE_REG                0x00
#define SY6512_SW_FREQ_MASK              (BIT(5) | BIT(6) | BIT(7))
#define SY6512_SW_FREQ_SHIFT             5
#define SY6512_EN_PIN_MASK               BIT(1)
#define SY6512_EN_PIN_SHIFT              1
#define SY6512_EN_SOFT_MASK              BIT(0)
#define SY6512_EN_SOFT_SHIFT             0
#define SY6512_SOFT_ENABLE               1
#define SY6512_SOFT_DISABLE              0

/* Watchdog and BUS OVP Register */
#define SY6512_WD_BUS_OVP_REG            0x01
#define SY6512_WD_EN_MASK                BIT(5)
#define SY6512_WD_EN_SHIFT               5
#define SY6512_WD_DISABLE                0

/* Configuration Register1 */
#define SY6512_CFG_REG1                  0x02
#define SY6512_MODE_CONTROL_MASK         (BIT(3) | BIT(4) | BIT(5))
#define SY6512_MODE_CONTROL_SHIFT        3
#define SY6512_CP_MODE_VAL               0x4
#define SY6512_BP_MODE_VAL               0x5
#define SY6512_REVERSE_BP_MODE_VAL       0x6
#define SY6512_REVERSE_CP_MODE_VAL       0x7

/* Configuration Register2 */
#define SY6512_CFG_REG2                  0x03

/* Status Register */
#define SY6512_STATUS_REG                0x04

/* Device Status Register */
#define SY6512_DEVICE_STATUS_REG         0x05
#define SY6512_POWER_READY_MASK          BIT(4)
#define SY6512_POWER_READY_SHIFT         4

struct sy6512_dev_info {
	struct i2c_client *client;
	struct device *dev;
	struct work_struct irq_work;
	int gpio_int;
	int irq_int;
	bool post_probe_done;
};

#endif /* _SY6512_H_ */
