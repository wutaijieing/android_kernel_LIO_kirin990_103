/* SPDX-License-Identifier: GPL-2.0 */
/*
 * hc32l110.h
 *
 * hc32l110 header file
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

#ifndef _HC32L110_H_
#define _HC32L110_H_

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/workqueue.h>
#include <linux/bitops.h>
#include <linux/mutex.h>
#include <linux/notifier.h>

struct hc32l110_param {
	int scp_support;
	int fcp_support;
	int wait_time;
};

struct hc32l110_device_info {
	struct i2c_client *client;
	struct device *dev;
	struct notifier_block event_nb;
	struct work_struct irq_work;
	struct hc32l110_param param_dts;
	int gpio_int;
	int irq_int;
	int gpio_enable;
	int gpio_reset;
	int chip_already_init;
	int fw_dev_id;
	int fw_ver_id;
	int fw_size;
	u8 *fw_data;
	bool is_low_power_mode;
	struct mutex scp_detect_lock;
	struct mutex accp_adapter_reg_lock;
	struct mutex ufcs_detect_lock;
	unsigned int scp_isr_backup[2]; /* scp_isr1,scp_isr2 */
};

bool hc32l110_is_support_fcp(struct hc32l110_device_info *di);

#endif /* _HC32L110_H_ */
