/* SPDX-License-Identifier: GPL-2.0 */
/*
 * rt9758.h
 *
 * charge-pump rt9758 macro, addr etc. (bp: bypass mode, cp: charge-pump mode)
 *
 * Copyright (c) 2021-2021 Huawei Technologies Co., Ltd.
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

#ifndef _RT9758_H_
#define _RT9758_H_

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

/* device id register */
#define RT9758_DEVICE_INFO_REG              0x00

/* mode status register */
#define RT9758_IC_STAT_REG                  0x04
#define RT9758_IC_STAT_BP_VAL               0x03
#define RT9758_IC_STAT_CP_VAL               0x02

/* vout ov register */
#define RT9758_CTRL1_REG                    0x0A
#define RT9758_CTRL1_VOUT_OV_FWD_VAL        0x7

/* vbus ov register */
#define RT9758_CTRL2_REG                    0x0B
#define RT9758_CTRL2_VBUS_OVP_VAL           0xFF

/* current limit register */
#define RT9758_CTRL3_REG                    0x0C
#define RT9758_CTRL3_WRX_IRE_OCP_VAL        0xFF

/* bus ctrl register */
#define RT9758_CTRL4_REG                    0x0D
#define RT9758_CTRL4_Q0_CONTROL_MASK        BIT(0)
#define RT9758_CTRL4_Q0_CONTROL_SHIFT       0
#define RT9758_CTRL4_Q0_CONTROL_DIS         0
#define RT9758_CTRL4_Q0_CONTROL_EN          1

/* mode register */
#define RT9758_CTRL6_REG                    0x0F
#define RT9758_CTRL6_MODE_REV_BP_VAL        0x26
#define RT9758_CTRL6_MODE_REV_CP_VAL        0x27
#define RT9758_CTRL6_MODE_BPCP_MASK         BIT(0)
#define RT9758_CTRL6_MODE_BPCP_SHIFT        0
#define RT9758_CTRL6_MODE_BP_EN             0
#define RT9758_CTRL6_MODE_CP_EN             1
#define RT9758_CTRL6_MODE_DIRECTION_MASK    BIT(1)
#define RT9758_CTRL6_MODE_DIRECTION_SHIFT   1
#define RT9758_CTRL6_MODE_FWD_BPCP_EN       0
#define RT9758_CTRL6_MODE_REV_BPCP_EN       1

struct rt9758_dev_info {
	struct i2c_client *client;
	struct device *dev;
	struct work_struct irq_work;
	u8 chip_id;
	int gpio_int;
	int irq_int;
	bool post_probe_done;
};

#endif /* _RT9758_H_ */
