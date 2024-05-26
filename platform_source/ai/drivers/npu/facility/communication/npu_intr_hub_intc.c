/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 * Description: npu intr_hub interrupt controller
 */

#include <linux/irqdomain.h>
#include <linux/irq.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/errno.h>

#include "npu_log.h"
#include "npu_common.h"

#define INTR_HUB_L2_INTR_MASKSET        0x4  /* Set Interrupt Enable bits */
#define INTR_HUB_L2_INTR_MASKCLR        0x8  /* Clear Interrupt Enable bits */
#define INTR_HUB_L2_INTR_STATUS         0xC  /* Interrupt Status Register */

#define INTR_HUB_L2_INTR_NUM         32

struct npu_intr_hub_irq_chip {
	int parent_irq;
	void __iomem *base;
	struct irq_domain *root_domain;
};

static void intc_enable(struct irq_data *d)
{
	struct npu_intr_hub_irq_chip *irqc = irq_data_get_irq_chip_data(d);

	npu_drv_info("npu_intr_hub: enable: %ld\n", d->hwirq);
	writel(BIT(d->hwirq), irqc->base + INTR_HUB_L2_INTR_MASKSET);
}

static void intc_disable(struct irq_data *d)
{
	struct npu_intr_hub_irq_chip *irqc = irq_data_get_irq_chip_data(d);

	npu_drv_info("npu_intr_hub: disable: %ld\n", d->hwirq);
	writel(BIT(d->hwirq), irqc->base + INTR_HUB_L2_INTR_MASKCLR);
}

static struct irq_chip intc_dev = {
	.name = "npu_intr_hub",
	.irq_unmask = intc_enable,
	.irq_mask = intc_disable,
};

static int npu_intr_hub_map(struct irq_domain *d, unsigned int irq, irq_hw_number_t hw)
{
	struct npu_intr_hub_irq_chip *irqc = d->host_data;
	unused(hw);

	irq_set_chip_and_handler(irq, &intc_dev, handle_level_irq);
	irq_set_status_flags(irq, IRQ_LEVEL);

	irq_set_chip_data(irq, irqc);
	return 0;
}

static const struct irq_domain_ops npu_intr_hub_irq_domain_ops = {
	.xlate = irq_domain_xlate_onetwocell,
	.map = npu_intr_hub_map,
};

static void npu_intr_hub_intc_irq_handler(struct irq_desc *desc)
{
	struct irq_chip *chip = irq_desc_get_chip(desc);
	struct npu_intr_hub_irq_chip *irqc = irq_data_get_irq_handler_data(&desc->irq_data);
	unsigned long bit, status;

	chained_irq_enter(chip, desc);

	status = readl(irqc->base + INTR_HUB_L2_INTR_STATUS);
	for_each_set_bit(bit, &status, INTR_HUB_L2_INTR_NUM) {
		generic_handle_irq(irq_find_mapping(irqc->root_domain, bit));
	}

	chained_irq_exit(chip, desc);
}

int npu_intr_hub_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct npu_intr_hub_irq_chip *irqc = NULL;
	struct resource *res = NULL;

	irqc = devm_kzalloc(dev, sizeof(struct npu_intr_hub_irq_chip), GFP_KERNEL);
	if (!irqc)
		return -ENOMEM;

	platform_set_drvdata(pdev, irqc);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	irqc->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(irqc->base)) {
		npu_drv_err("npu_intr_hub init: fail to get reg base");
		return PTR_ERR(irqc->base);
	}

	irqc->parent_irq = platform_get_irq(pdev, 0);
	npu_drv_info("npu_intr_hub init: parent_irq: %d\n", irqc->parent_irq);
	if (irqc->parent_irq < 0) {
		npu_drv_err("npu_intr_hub init: fail to get irq");
		return -ENOENT;
	}

	/* Disable all external interrupts until they are explicitly requested. */
	writel(0xffffffff, irqc->base + INTR_HUB_L2_INTR_MASKCLR);

	irqc->root_domain = irq_domain_add_linear(dev->of_node, INTR_HUB_L2_INTR_NUM, &npu_intr_hub_irq_domain_ops, irqc);
	if (!irqc->root_domain) {
		npu_drv_err("npu_intr_hub init: Unable to create IRQ domain");
		return -ENXIO;
	}

	irq_set_chained_handler_and_data(irqc->parent_irq,  npu_intr_hub_intc_irq_handler,  irqc);
	npu_drv_info("npu_intr_hub init:success\n");
	return 0;
}

int npu_intr_hub_remove(struct platform_device *pdev)
{
	struct npu_intr_hub_irq_chip *irqc = platform_get_drvdata(pdev);
	irq_domain_remove(irqc->root_domain);
	return 0;
}

static const struct of_device_id npu_intr_hub_of_match[] = {
	{.compatible = "hisilicon,npu_intr_hub",},
	{},
};
MODULE_DEVICE_TABLE(of, npu_intr_hub_of_match);

static struct platform_driver npu_intr_hub_driver = {
	.probe = npu_intr_hub_probe,
	.remove = npu_intr_hub_remove,
	.driver = {
		.name = "npu_intr_hub",
		.of_match_table = npu_intr_hub_of_match,
	},
};

int npu_intr_hub_init(void)
{
	int ret;

	npu_drv_debug("npu_intr_hub_init started\n");

	ret = platform_driver_register(&npu_intr_hub_driver);
	if (ret) {
		npu_drv_err("register npu intr_hub driver fail\n");
		return ret;
	}
	npu_drv_debug("npu_intr_hub_init succeed\n");

	return ret;
}

void npu_intr_hub_exit(void)
{
	platform_driver_unregister(&npu_intr_hub_driver);
}

