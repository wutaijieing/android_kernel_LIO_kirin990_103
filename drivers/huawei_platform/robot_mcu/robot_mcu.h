// SPDX-License-Identifier: GPL-2.0
/*
 * robot_mcu.h
 *
 * robot_mcu head file for smurfs mcu control driver.
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

#ifndef _ROBOT_MCU_H
#define _ROBOT_MCU_H

enum robot_mcu_cmd {
	ROBOT_MCU_RESET = 1,
	ROBOT_MCU_BOOT0_SET,
	ROBOT_MCU_BOOT0_RESET,
	ROBOT_MCU_SYNC_UPGRADE_SET,
	ROBOT_MCU_SYNC_UPGRADE_RESET,
	ROBOT_MCU_INVALID,
};

enum robot_mcu_gpio_level {
	ROBOT_MCU_LEVEL_LOW = 0,
	ROBOT_MCU_LEVEL_HIGH,
};

struct robot_mcu_dev_data {
	uint32_t gpio_reset;
	uint32_t gpio_boot0;
	uint32_t gpio_wakeup;
	uint32_t gpio_wakeup_head_touch;
	uint32_t gpio_upgrade_ioctl;
	uint32_t gpio_upgrade_status;
	int irq_wakeup;
	int wakeup_head_touch_irq;
};

#endif
