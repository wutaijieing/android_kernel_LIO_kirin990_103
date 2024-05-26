/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: provide gpio access function interfaces.
 * Create: 2019-6-12
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef HIV500_GPIO_H
#define HIV500_GPIO_H
#include <linux/hwspinlock.h>
#include <linux/gpio.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/of_address.h>
#include <linux/version.h>

#define HW_SW_SEL 4
#define HW_SW_SEL_MASK 20
#define GPIO_PER_REG 16
#define HIGH_LEVEL_ENABLE 0x00080008
#define LOW_LEVEL_ENABLE  0x00080000
#define RISING_EDGE_ENABLE  0x00060002
#define FALLING_EDGE_ENABLE  0x00060000
#define BOTH_EDGE_ENABLE  0x00060006
#define LEVEL_TRIGGER_ENABLE 0x00010001
#define EDGE_TRIGGER_ENABLE 0x00010000

#ifdef CONFIG_GPIO_PM_SUPPORT
struct hiv500_context_save_regs {
	u8 gpio_data;
	u8 gpio_dir;
	u8 gpio_is;
	u8 gpio_ibe;
	u8 gpio_iev;
	u8 gpio_ie;
};
#endif

struct hiv500 {
	raw_spinlock_t		lock;

	void __iomem		*base;
	struct gpio_chip	gc;
	struct irq_chip		irq_chip;
	unsigned int		parent_irq;

#ifdef CONFIG_GPIO_PM_SUPPORT
	struct hiv500_context_save_regs csave_regs;
#endif
	struct amba_device *adev;
};
#endif