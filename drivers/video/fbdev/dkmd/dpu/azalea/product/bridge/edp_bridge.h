/*
 * edp_bridge.h
 *
 * edp bridge driver of lt9711a
 *
 * Copyright (c) 2019-2020 Huawei Technologies Co., Ltd.
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

#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <linux/kernel.h>
#include "dpu_fb.h"

#define	LT9711A_I2C_ADDR		0x41
#define	CHIP_ID_REG_0			0x00
#define	CHIP_ID_REG_1			0x01
#define	MAX_CHIP_PROBE_TIMES    5
#define	POWER_DELAY_MS          2

#define edp_bridge_emerg(msg, ...)    \
	do { if (edp_bridge_msg_level > 0)  \
		printk(KERN_EMERG "[edp_bridge]%s: "msg, __func__, ## __VA_ARGS__); } while (0)
#define edp_bridge_alert(msg, ...)    \
	do { if (edp_bridge_msg_level > 1)  \
		printk(KERN_ALERT "[edp_bridge]%s: "msg, __func__, ## __VA_ARGS__); } while (0)
#define edp_bridge_crit(msg, ...)    \
	do { if (edp_bridge_msg_level > 2)  \
		printk(KERN_CRIT "[edp_bridge]%s: "msg, __func__, ## __VA_ARGS__); } while (0)
#define edp_bridge_err(msg, ...)    \
	do { if (edp_bridge_msg_level > 3)  \
		printk(KERN_ERR "[edp_bridge]%s: "msg, __func__, ## __VA_ARGS__); } while (0)
#define edp_bridge_warning(msg, ...)    \
	do { if (edp_bridge_msg_level > 4)  \
		printk(KERN_WARNING "[edp_bridge]%s: "msg, __func__, ## __VA_ARGS__); } while (0)
#define edp_bridge_notice(msg, ...)    \
	do { if (edp_bridge_msg_level > 5)  \
		printk(KERN_NOTICE "[edp_bridge]%s: "msg, __func__, ## __VA_ARGS__); } while (0)
#define edp_bridge_info(msg, ...)    \
	do { if (edp_bridge_msg_level > 6)  \
		printk(KERN_INFO "[edp_bridge]%s: "msg, __func__, ## __VA_ARGS__); } while (0)
#define edp_bridge_debug(msg, ...)    \
	do { if (edp_bridge_msg_level > 7)  \
		printk(KERN_DEBUG "[edp_bridge]%s: "msg, __func__, ## __VA_ARGS__); } while (0)

struct gpio_config {
	int lcd_3v3_gpio;
	int lcd_1v8_gpio;
	int reset_gpio;
	int bl_en_gpio;
	int bl_bst_gpio;
};

struct mipi2edp {
	struct device *dev;
	struct i2c_client *client;
	struct regmap *regmap;
	struct dpu_panel_info panel_info;
	struct regulator *ldo_vdd;
	struct regulator *ldo_vcc;
	int vdd_voltage;
	int vcc_voltage;
	const struct regmap_config *regmap_config;
	struct gpio_config *gpio_set;
};

static const struct regmap_config lt9711a_regmap = {
	.reg_bits = 8,
	.val_bits = 8,
	.reg_stride = 1,
};
