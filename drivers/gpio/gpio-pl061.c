// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2008, 2009 Provigent Ltd.
 *
 * Author: Baruch Siach <baruch@tkos.co.il>
 *
 * Driver for the ARM PrimeCell(tm) General Purpose Input/Output (PL061)
 *
 * Data sheet: ARM DDI 0190B, September 2000
 */
#include <linux/spinlock.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/module.h>
#include <linux/bitops.h>
#include <linux/gpio/driver.h>
#include <linux/device.h>
#include <linux/amba/bus.h>
#include <linux/slab.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pm.h>
#include <linux/arm-smccc.h>

#include <platform_include/basicplatform/linux/vendor_gpio.h>
#include "gpiolib.h"

#define GPIODIR 0x400
#define GPIOIS  0x404
#define GPIOIBE 0x408
#define GPIOIEV 0x40C
#define GPIOIE  0x410
#define GPIORIS 0x414
#define GPIOMIS 0x418
#define GPIOIC  0x41C

#define PL061_GPIO_NR	8

struct hwspinlock       *gpio_hwlock;

#define HISI_SECURE_GPIO_REG_READ   0xc5010004
#define HISI_SECURE_GPIO_REG_WRITE  0xc5010005
unsigned char pl061_readb(struct pl061 *chip, unsigned offset)
{
	struct amba_device *adev = chip->adev;
	unsigned char v;
	struct arm_smccc_res res;

	if (adev->secure_mode) {
		arm_smccc_1_1_smc(HISI_SECURE_GPIO_REG_READ,
			offset, adev->res.start, &res);
		v = (u8)res.a1;
	} else {
		v = readb(chip->base + offset);
	}

	return v;
}

void pl061_writeb(struct pl061 *chip, unsigned char v, unsigned offset)
{
	struct amba_device *adev = chip->adev;
	struct arm_smccc_res res;

	if (adev->secure_mode) {
		arm_smccc_1_1_smc(HISI_SECURE_GPIO_REG_WRITE, v,
			offset, adev->res.start, &res);
	} else {
		writeb(v, chip->base + offset);
	}
	return;
}

static int pl061_get_direction(struct gpio_chip *gc, unsigned offset)
{
	struct pl061 *pl061 = gpiochip_get_data(gc);

	if (pl061_readb(pl061, GPIODIR) & BIT(offset))
		return GPIO_LINE_DIRECTION_OUT;

	return GPIO_LINE_DIRECTION_IN;
}

static int pl061_direction_input(struct gpio_chip *gc, unsigned offset)
{
	struct pl061 *pl061 = gpiochip_get_data(gc);
	unsigned long flags;
	unsigned char gpiodir;

	raw_spin_lock_irqsave(&pl061->lock, flags);
	if (hwspin_lock_timeout(gpio_hwlock, LOCK_TIMEOUT)) {
		raw_spin_unlock_irqrestore(&pl061->lock, flags);
		dev_err(gc->parent, "%s: hwspinlock timeout!\n", __func__);
		dev_err(gc->parent, "hwspinlock status is 0x%x\n",
			get_gpio_hwspinlock_status());
		return -EBUSY;
	}
	gpiodir = pl061_readb(pl061, GPIODIR);
	gpiodir &= ~(BIT(offset));
	pl061_writeb(pl061, gpiodir, GPIODIR);
	hwspin_unlock(gpio_hwlock);
	raw_spin_unlock_irqrestore(&pl061->lock, flags);

	return 0;
}

static int pl061_direction_output(struct gpio_chip *gc, unsigned offset,
		int value)
{
	struct pl061 *pl061 = gpiochip_get_data(gc);
	unsigned long flags;
	unsigned char gpiodir;

	raw_spin_lock_irqsave(&pl061->lock, flags);
	if (hwspin_lock_timeout(gpio_hwlock, LOCK_TIMEOUT)) {
		raw_spin_unlock_irqrestore(&pl061->lock, flags);
		dev_err(gc->parent, "%s: hwspinlock timeout!\n", __func__);
		dev_err(gc->parent, "hwspinlock status is 0x%x\n",
			get_gpio_hwspinlock_status());
		return -EBUSY;
	}
	pl061_writeb(pl061, !!value << offset, (BIT(offset + 2)));
	gpiodir = pl061_readb(pl061, GPIODIR);
	gpiodir |= BIT(offset);
	pl061_writeb(pl061, gpiodir, GPIODIR);

	/*
	 * gpio value is set again, because pl061 doesn't allow to set value of
	 * a gpio pin before configuring it in OUT mode.
	 */
	pl061_writeb(pl061, !!value << offset, (BIT(offset + 2)));
	hwspin_unlock(gpio_hwlock);
	raw_spin_unlock_irqrestore(&pl061->lock, flags);

	return 0;
}

static int pl061_get_value(struct gpio_chip *gc, unsigned offset)
{
	struct pl061 *pl061 = gpiochip_get_data(gc);

	return !!pl061_readb(pl061, (BIT(offset + 2)));
}

static void pl061_set_value(struct gpio_chip *gc, unsigned offset, int value)
{
	struct pl061 *pl061 = gpiochip_get_data(gc);

	pl061_writeb(pl061, !!value << offset, (BIT(offset + 2)));
}

static int pl061_irq_type(struct irq_data *d, unsigned trigger)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct pl061 *pl061 = gpiochip_get_data(gc);
	int offset = irqd_to_hwirq(d);
	unsigned long flags;
	u8 gpiois, gpioibe, gpioiev;
	u8 bit = BIT(offset);

	if (offset < 0 || offset >= PL061_GPIO_NR)
		return -EINVAL;

	if ((trigger & (IRQ_TYPE_LEVEL_HIGH | IRQ_TYPE_LEVEL_LOW)) &&
	    (trigger & (IRQ_TYPE_EDGE_RISING | IRQ_TYPE_EDGE_FALLING)))
	{
		dev_err(gc->parent,
			"trying to configure line %d for both level and edge "
			"detection, choose one!\n",
			offset);
		return -EINVAL;
	}


	raw_spin_lock_irqsave(&pl061->lock, flags);
	if (hwspin_lock_timeout(gpio_hwlock, LOCK_TIMEOUT)) {
		raw_spin_unlock_irqrestore(&pl061->lock, flags);
		dev_err(gc->parent, "%s: hwspinlock timeout!\n", __func__);
		dev_err(gc->parent, "hwspinlock status is 0x%x\n",
			get_gpio_hwspinlock_status());
		return -EBUSY;
	}
	gpioiev = pl061_readb(pl061, GPIOIEV);
	gpiois = pl061_readb(pl061, GPIOIS);
	gpioibe = pl061_readb(pl061, GPIOIBE);

	if (trigger & (IRQ_TYPE_LEVEL_HIGH | IRQ_TYPE_LEVEL_LOW)) {
		bool polarity = trigger & IRQ_TYPE_LEVEL_HIGH;

		/* Disable edge detection */
		gpioibe &= ~bit;
		/* Enable level detection */
		gpiois |= bit;
		/* Select polarity */
		if (polarity)
			gpioiev |= bit;
		else
			gpioiev &= ~bit;
		irq_set_handler_locked(d, handle_level_irq);
		dev_dbg(gc->parent, "line %d: IRQ on %s level\n",
			offset,
			polarity ? "HIGH" : "LOW");
	} else if ((trigger & IRQ_TYPE_EDGE_BOTH) == IRQ_TYPE_EDGE_BOTH) {
		/* Disable level detection */
		gpiois &= ~bit;
		/* Select both edges, setting this makes GPIOEV be ignored */
		gpioibe |= bit;
		irq_set_handler_locked(d, handle_edge_irq);
		dev_dbg(gc->parent, "line %d: IRQ on both edges\n", offset);
	} else if ((trigger & IRQ_TYPE_EDGE_RISING) ||
		   (trigger & IRQ_TYPE_EDGE_FALLING)) {
		bool rising = trigger & IRQ_TYPE_EDGE_RISING;

		/* Disable level detection */
		gpiois &= ~bit;
		/* Clear detection on both edges */
		gpioibe &= ~bit;
		/* Select edge */
		if (rising)
			gpioiev |= bit;
		else
			gpioiev &= ~bit;
		irq_set_handler_locked(d, handle_edge_irq);
		dev_dbg(gc->parent, "line %d: IRQ on %s edge\n",
			offset,
			rising ? "RISING" : "FALLING");
	} else {
		/* No trigger: disable everything */
		gpiois &= ~bit;
		gpioibe &= ~bit;
		gpioiev &= ~bit;
		irq_set_handler_locked(d, handle_bad_irq);
		dev_warn(gc->parent, "no trigger selected for line %d\n",
			 offset);
	}

	pl061_writeb(pl061, gpiois, GPIOIS);
	pl061_writeb(pl061, gpioibe, GPIOIBE);
	pl061_writeb(pl061, gpioiev, GPIOIEV);

	hwspin_unlock(gpio_hwlock);
	raw_spin_unlock_irqrestore(&pl061->lock, flags);

	return 0;
}

static void pl061_irq_handler(struct irq_desc *desc)
{
	unsigned long pending;
	int offset;
	struct gpio_chip *gc = irq_desc_get_handler_data(desc);
	struct pl061 *pl061 = gpiochip_get_data(gc);
	struct irq_chip *irqchip = irq_desc_get_chip(desc);

	chained_irq_enter(irqchip, desc);

	pending = pl061_readb(pl061, GPIOMIS);
	if (pending) {
		for_each_set_bit(offset, &pending, PL061_GPIO_NR)
			generic_handle_irq(irq_find_mapping(gc->irq.domain,
							    offset));
	}

	chained_irq_exit(irqchip, desc);
}

static void pl061_irq_mask(struct irq_data *d)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct pl061 *pl061 = gpiochip_get_data(gc);
	u8 mask = BIT(irqd_to_hwirq(d) % PL061_GPIO_NR);
	u8 gpioie;

	raw_spin_lock(&pl061->lock);
	if (hwspin_lock_timeout(gpio_hwlock, LOCK_TIMEOUT)) {
		raw_spin_unlock(&pl061->lock);
		dev_err(gc->parent, "%s: hwspinlock timeout!\n", __func__);
		dev_err(gc->parent, "hwspinlock status is 0x%x\n",
			get_gpio_hwspinlock_status());
		return ;
	}
	gpioie = pl061_readb(pl061, GPIOIE) & ~mask;
	pl061_writeb(pl061, gpioie, GPIOIE);

	hwspin_unlock(gpio_hwlock);
	raw_spin_unlock(&pl061->lock);
}

static void pl061_irq_unmask(struct irq_data *d)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct pl061 *pl061 = gpiochip_get_data(gc);
	u8 mask = BIT(irqd_to_hwirq(d) % PL061_GPIO_NR);
	u8 gpioie;

	raw_spin_lock(&pl061->lock);
	if (hwspin_lock_timeout(gpio_hwlock, LOCK_TIMEOUT)) {
		raw_spin_unlock(&pl061->lock);
		dev_err(gc->parent, "%s: hwspinlock timeout!\n!", __func__);
		dev_err(gc->parent, "hwspinlock status is 0x%x\n",
			get_gpio_hwspinlock_status());
		return;
	}
	gpioie = pl061_readb(pl061, GPIOIE) | mask;
	pl061_writeb(pl061, gpioie, GPIOIE);
	hwspin_unlock(gpio_hwlock);
	raw_spin_unlock(&pl061->lock);
}

/**
 * pl061_irq_ack() - ACK an edge IRQ
 * @d: IRQ data for this IRQ
 *
 * This gets called from the edge IRQ handler to ACK the edge IRQ
 * in the GPIOIC (interrupt-clear) register. For level IRQs this is
 * not needed: these go away when the level signal goes away.
 */
static void pl061_irq_ack(struct irq_data *d)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct pl061 *pl061 = gpiochip_get_data(gc);
	u8 mask = BIT(irqd_to_hwirq(d) % PL061_GPIO_NR);

	raw_spin_lock(&pl061->lock);
	pl061_writeb(pl061, mask, GPIOIC);
	raw_spin_unlock(&pl061->lock);
}

static int pl061_irq_set_wake(struct irq_data *d, unsigned int state)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct pl061 *pl061 = gpiochip_get_data(gc);

	return irq_set_irq_wake(pl061->parent_irq, state);
}

static int pl061_probe(struct amba_device *adev, const struct amba_id *id)
{
	struct device *dev = &adev->dev;
	struct pl061 *pl061;
	struct gpio_irq_chip *girq;
	int ret, irq;
	struct device_node *np = dev->of_node;

	pl061 = devm_kzalloc(dev, sizeof(*pl061), GFP_KERNEL);
	if (pl061 == NULL)
		return -ENOMEM;

	pl061->adev = adev;
	pl061->base = devm_ioremap_resource(dev, &adev->res);
	if (IS_ERR(pl061->base))
		return PTR_ERR(pl061->base);

	raw_spin_lock_init(&pl061->lock);
	if (of_get_property(np, "gpio,hwspinlock", NULL)) {
		gpio_hwlock = hwspin_lock_request_specific(GPIO_HWLOCK_ID);
		if (gpio_hwlock == NULL)
			return -EBUSY;
	}
	pl061->gc.request = gpiochip_generic_request;
	pl061->gc.free = gpiochip_generic_free;
#ifdef CONFIG_GPIO_PL061_VM
	pl061->gc.base = pl061_get_base_value(np);
#else
	pl061->gc.base = -1;
#endif
	pl061->gc.get_direction = pl061_get_direction;
	pl061->gc.direction_input = pl061_direction_input;
	pl061->gc.direction_output = pl061_direction_output;
	pl061->gc.get = pl061_get_value;
	pl061->gc.set = pl061_set_value;
	pl061->gc.ngpio = PL061_GPIO_NR;
	pl061->gc.label = dev_name(dev);
	pl061->gc.parent = dev;
	pl061->gc.owner = THIS_MODULE;

	/*
	 * irq_chip support
	 */
	pl061->irq_chip.name = dev_name(dev);
	pl061->irq_chip.irq_ack	= pl061_irq_ack;
	pl061->irq_chip.irq_mask = pl061_irq_mask;
	pl061->irq_chip.irq_unmask = pl061_irq_unmask;
	pl061->irq_chip.irq_set_type = pl061_irq_type;
	pl061->irq_chip.irq_set_wake = pl061_irq_set_wake;
#ifdef CONFIG_ARCH_HISI
	pl061->irq_chip.irq_disable = pl061_irq_mask;
	pl061->irq_chip.irq_enable = pl061_irq_unmask;
#endif

	pl061_writeb(pl061, 0xff, GPIOIC);
	pl061_writeb(pl061, 0, GPIOIE); /* disable irqs */
	irq = adev->irq[0];
	if (!irq)
		dev_warn(&adev->dev, "IRQ support disabled\n");
	pl061->parent_irq = irq;

	girq = &pl061->gc.irq;
	girq->chip = &pl061->irq_chip;
	girq->parent_handler = pl061_irq_handler;
	girq->num_parents = 1;
	girq->parents = devm_kcalloc(dev, 1, sizeof(*girq->parents),
				     GFP_KERNEL);
	if (!girq->parents)
		return -ENOMEM;
	girq->parents[0] = irq;
	girq->default_type = IRQ_TYPE_NONE;
	girq->handler = handle_bad_irq;

	ret = devm_gpiochip_add_data(dev, &pl061->gc, pl061);
	if (ret)
		return ret;

	amba_set_drvdata(adev, pl061);
	dev_info(dev, "PL061 GPIO chip registered\n");

	return 0;
}

#ifdef CONFIG_GPIO_PM_SUPPORT
static int pl061_suspend(struct device *dev)
{
	struct pl061 *pl061 = dev_get_drvdata(dev);
	int offset;

	pl061->csave_regs.gpio_data = 0;
	pl061->csave_regs.gpio_dir = pl061_readb(pl061, GPIODIR);
	pl061->csave_regs.gpio_is = pl061_readb(pl061, GPIOIS);
	pl061->csave_regs.gpio_ibe = pl061_readb(pl061, GPIOIBE);
	pl061->csave_regs.gpio_iev = pl061_readb(pl061, GPIOIEV);
	pl061->csave_regs.gpio_ie = pl061_readb(pl061, GPIOIE);

	for (offset = 0; offset < PL061_GPIO_NR; offset++) {
		if (pl061->csave_regs.gpio_dir & (BIT(offset)))
			pl061->csave_regs.gpio_data |=
				pl061_get_value(&pl061->gc, offset) << offset;
	}

	return 0;
}

static int pl061_resume(struct device *dev)
{
	struct pl061 *pl061 = dev_get_drvdata(dev);
	int offset;

	for (offset = 0; offset < PL061_GPIO_NR; offset++) {
		if (pl061->csave_regs.gpio_dir & (BIT(offset)))
			pl061_direction_output(&pl061->gc, offset,
					pl061->csave_regs.gpio_data &
					(BIT(offset)));
		else
			pl061_direction_input(&pl061->gc, offset);
	}

	pl061_writeb(pl061, pl061->csave_regs.gpio_is, GPIOIS);
	pl061_writeb(pl061, pl061->csave_regs.gpio_ibe, GPIOIBE);
	pl061_writeb(pl061, pl061->csave_regs.gpio_iev, GPIOIEV);
	pl061_writeb(pl061, pl061->csave_regs.gpio_ie, GPIOIE);

	return 0;
}

static const struct dev_pm_ops pl061_dev_pm_ops = {
	.suspend = pl061_suspend,
	.resume = pl061_resume,
	.freeze = pl061_suspend,
	.restore = pl061_resume,
};
#endif

static const struct amba_id pl061_ids[] = {
	{
		.id	= 0x00041061,
		.mask	= 0x000fffff,
	},
	{ 0, 0 },
};
MODULE_DEVICE_TABLE(amba, pl061_ids);

static struct amba_driver pl061_gpio_driver = {
	.drv = {
		.name	= "pl061_gpio",
#ifdef CONFIG_GPIO_PM_SUPPORT
		.pm	= &pl061_dev_pm_ops,
#endif
	},
	.id_table	= pl061_ids,
	.probe		= pl061_probe,
};

static int __init pl061_gpio_init(void)
{
	return amba_driver_register(&pl061_gpio_driver);
}
subsys_initcall(pl061_gpio_init);
MODULE_LICENSE("GPL v2");
