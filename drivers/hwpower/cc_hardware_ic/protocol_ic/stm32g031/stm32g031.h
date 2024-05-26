/* SPDX-License-Identifier: GPL-2.0 */
/*
 * stm32g031.h
 *
 * stm32g031 header file
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

#ifndef _STM32G031_H_
#define _STM32G031_H_

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

struct stm32g031_param {
	int scp_support;
	int fcp_support;
	int ufcs_support;
	int wait_time;
	int hw_config; /* choose different hw configs by different boardid */
	int int_gpio_type; /* 0:int gpio for wakeup; 1:int gpio for interrupt */
};

struct stm32g031_device_info {
	struct i2c_client *client;
	struct device *dev;
	struct notifier_block event_nb;
	struct work_struct irq_work;
	struct stm32g031_param param_dts;
	int gpio_int;
	int irq_int;
	int gpio_enable;
	int gpio_reset;
	int chip_already_init;
	int fw_hw_id;
	int fw_ver_id;
	int fw_size;
	u8 *fw_data;
	bool is_low_power_mode;
	bool ufcs_communicating_flag;
	struct mutex scp_detect_lock;
	struct mutex accp_adapter_reg_lock;
	struct mutex ufcs_detect_lock;
	unsigned int scp_isr_backup[2]; /* scp_isr1,scp_isr2 */
	u8 ufcs_isr[2]; /* ufcs_isr1,ufcs_isr2 */
};

bool stm32g031_is_support_fcp(struct stm32g031_device_info *di);
void stm32g031_hard_reset(struct stm32g031_device_info *di);

#endif /* _STM32G031_H_ */
