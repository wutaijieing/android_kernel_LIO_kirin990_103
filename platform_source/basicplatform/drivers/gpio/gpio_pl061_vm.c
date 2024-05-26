/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 * Description: provide gpio access function interfaces.
 * Create: 2021-12-23
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

#include <linux/spinlock.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/bitops.h>
#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/amba/bus.h>
#include <linux/slab.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pm.h>
#include <linux/arm-smccc.h>
#include <platform_include/basicplatform/linux/vendor_gpio.h>
#include "gpiolib.h"

#define GPIO_INPUT 0
#define GPIO_OUTPUT 1
#define PL061_GPIO_NR 8
#define GPIO_VM_ID 0xD00E0000
#define GPIO_GET_VALUE 0xD00E0001
#define GPIO_SET_VALUE 0xD00E0002
#define GPIO_GET_DIRECTION 0xD00E0003
#define GPIO_SET_DIRECTION 0xD00E0004

static inline unsigned int gpio_hvc(unsigned int gpio_hvc_id, unsigned int x1,
	unsigned int x2, unsigned int x3)
{
	struct arm_smccc_res res = {0};
	arm_smccc_hvc((u64)gpio_hvc_id, (u64)x1, (u64)x2, (u64)x3,
			0, 0, 0, 0, &res);
	return (unsigned int)res.a0;
}

static int pl061_get_direction(struct gpio_chip *gc, unsigned offset)
{
	return gpio_hvc(GPIO_VM_ID | GPIO_GET_DIRECTION, gc->base + offset, 0, 0);
}

static int pl061_direction_input(struct gpio_chip *gc, unsigned offset)
{
	return gpio_hvc(GPIO_VM_ID | GPIO_SET_DIRECTION, gc->base + offset, GPIO_INPUT, 0);
}

static int pl061_direction_output(struct gpio_chip *gc, unsigned offset,
		int value)
{
	return gpio_hvc(GPIO_VM_ID | GPIO_SET_DIRECTION, gc->base + offset, GPIO_OUTPUT, value);
}

static int pl061_get_value(struct gpio_chip *gc, unsigned offset)
{
	return gpio_hvc(GPIO_VM_ID | GPIO_GET_VALUE, gc->base + offset, 0, 0);
}

static void pl061_set_value(struct gpio_chip *gc, unsigned offset, int value)
{
	gpio_hvc(GPIO_VM_ID | GPIO_SET_VALUE, gc->base + offset, value, 0);
}

static int pl061_vm_probe(struct amba_device *adev, const struct amba_id *id)
{
	struct device *dev = &adev->dev;
	struct pl061 *pl061;
	int ret;
	struct device_node *np = dev->of_node;
	pl061 = devm_kzalloc(dev, sizeof(struct pl061), GFP_KERNEL);
	if (pl061 == NULL)
		return -ENOMEM;

	pl061->base = (void __iomem *)adev->res.start;

	/* Hook the request()/free() for pinctrl operation */
	if (of_get_property(dev->of_node, "gpio-ranges", NULL)) {
		pl061->gc.request = gpiochip_generic_request;
		pl061->gc.free = gpiochip_generic_free;
	}

	pl061->gc.base = pl061_get_base_value(np);
	pl061->gc.get_direction = pl061_get_direction;
	pl061->gc.direction_input = pl061_direction_input;
	pl061->gc.direction_output = pl061_direction_output;
	pl061->gc.get = pl061_get_value;
	pl061->gc.set = pl061_set_value;
	pl061->gc.ngpio = PL061_GPIO_NR;
	pl061->gc.label = dev_name(dev);
	pl061->gc.parent = dev;
	pl061->gc.owner = THIS_MODULE;
	ret = gpiochip_add_data(&pl061->gc, pl061);
	if (ret)
		return ret;

	amba_set_drvdata(adev, pl061);

	dev_info(&adev->dev, "V-PL061 GPIO chip @%pa registered\n", &adev->res.start);

	return 0;
}

static const struct amba_id pl061_vm_ids[] = {
	{
		.id	= 0x00048061,
		.mask	= 0x000fffff,
	},
	{ 0, 0 },
};

static struct amba_driver pl061_vm_gpio_driver = {
	.drv = {
		.name	= "pl061_vm_gpio",
	},
	.id_table	= pl061_vm_ids,
	.probe		= pl061_vm_probe,
};

static int __init pl061_vm_gpio_init(void)
{
	return amba_driver_register(&pl061_vm_gpio_driver);
}
subsys_initcall(pl061_vm_gpio_init);
