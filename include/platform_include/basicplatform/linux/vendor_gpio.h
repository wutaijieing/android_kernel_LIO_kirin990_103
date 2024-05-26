/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: provide gpio access function interfaces.
 * Create: 2021-01-01
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
#ifndef __VENDOR_GPIO_H__
#define __VENDOR_GPIO_H__

#include <linux/hwspinlock.h>
#include <linux/gpio.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/of_address.h>
#include <linux/version.h>

#define	GPIO_HWLOCK_ID	1
#define	LOCK_TIMEOUT	1000
unsigned int get_gpio_hwspinlock_status(void);
extern int gpiochip_usbphy_request(struct gpio_chip *chip, unsigned int offset);
extern void gpiochip_usbphy_free(struct gpio_chip *chip, unsigned int offset);

#define GPIODATA 0x3fc

#ifdef CONFIG_GPIO_PM_SUPPORT

struct pl061_context_save_regs {
	u8 gpio_data;
	u8 gpio_dir;
	u8 gpio_is;
	u8 gpio_ibe;
	u8 gpio_iev;
	u8 gpio_ie;
};
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
struct pl061 {
	raw_spinlock_t		lock;

	void __iomem		*base;
	struct gpio_chip	gc;
	struct irq_chip		irq_chip;
	int			parent_irq;

#ifdef CONFIG_GPIO_PM_SUPPORT
	struct pl061_context_save_regs csave_regs;
#endif
	struct amba_device *adev;
};
#else
struct pl061 {
	raw_spinlock_t	lock;
	int		sec_status;
	void __iomem	*base;
	struct gpio_chip	gc;
	int                     parent_irq;
	bool		uses_pinctrl;
#ifdef CONFIG_GPIO_PM_SUPPORT
	struct pl061_context_save_regs csave_regs;
#endif
	struct amba_device *adev;
};

int pl061_parse_gpio_base(struct device *dev);
#endif

#endif