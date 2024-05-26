/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 *
 * pmic_vm.c
 *
 * Device driver for pmic vmware
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
#include "pmic_vm.h"
#include <linux/device.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/arm-smccc.h>
#include <platform_include/basicplatform/linux/mfd/pmic_platform.h>

#define MAIN_PMIC_SLAVEID 9

static inline unsigned int pmic_hvc(unsigned int pmic_hvc_id, unsigned int x1,
		unsigned int x2, unsigned int x3)
{
	struct arm_smccc_res res = {0};

	arm_smccc_hvc((u64)pmic_hvc_id, (u64)x1, (u64)x2, (u64)x3,
			0, 0, 0, 0, &res);
	return (unsigned int)res.a0;
}

unsigned int pmic_read_reg(int addr)
{
	return pmic_hvc(PMIC_VM_ID | PMIC_READ, MAIN_PMIC_SLAVEID, addr, 0);
}
EXPORT_SYMBOL(pmic_read_reg);

void pmic_write_reg(int addr, int val)
{
	pmic_hvc(PMIC_VM_ID | PMIC_WRITE, MAIN_PMIC_SLAVEID, addr, val);
}
EXPORT_SYMBOL(pmic_write_reg);

static int pmic_vm_read(void *context, unsigned int addr, unsigned int *val)
{
	unsigned long flags;
	struct pmic_vm_dev *di = (struct pmic_vm_dev *)context;

	spin_lock_irqsave(&di->buffer_lock, flags);

	*val = pmic_hvc(PMIC_VM_ID | PMIC_READ, (di->bus_id << 8) | di->slave_id, addr, 0);

	spin_unlock_irqrestore(&di->buffer_lock, flags);

	return 0;
}

static int pmic_vm_write(void *context, unsigned int addr, unsigned int val)
{
	unsigned int ret;
	unsigned long flags;

	struct pmic_vm_dev *di = (struct pmic_vm_dev *)context;

	spin_lock_irqsave(&di->buffer_lock, flags);

	ret = pmic_hvc(PMIC_VM_ID | PMIC_WRITE,
		(di->bus_id << 8) | di->slave_id, addr, val);

	spin_unlock_irqrestore(&di->buffer_lock, flags);

	return ret ? -1 : 0;
}

static const struct regmap_config pmic_vm_regmap_config = {
	.reg_bits = 16,
	.val_bits = 8,
	.reg_read = pmic_vm_read,
	.reg_write = pmic_vm_write,
};

static pmic_vm_parse_dts(struct pmic_vm_dev *di)
{
	int ret;
	struct device_node *np = NULL;

	np = di->dev->of_node;
	if (!np)
		return -1;

	ret = of_property_read_u32(np, "bus_id", (u32 *)&(di->bus_id));
	if (ret != 0) {
		dev_err(di->dev, "no bus_id property set\n");
		return ret;
	}

	ret = of_property_read_u32(np, "slave_id", (u32 *)&(di->slave_id));
	if (ret != 0) {
		dev_err(di->dev, "no slave_id property set\n");
		return ret;
	}

	ret = of_property_read_u32(np, "reg_bits", (u32 *)&(di->reg_bits));
	if (ret != 0)
		dev_err(di->dev, "no reg_bits property set\n");

	ret = of_property_read_u32(np, "val_bits", (u32 *)&(di->val_bits));
	if (ret != 0)
		dev_err(di->dev, "no val_bits property set\n");

	return 0;
}

static int pmic_vm_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct pmic_vm_dev *di = NULL;
	struct device_node *np = NULL;
	int ret;

	di = devm_kzalloc(dev, sizeof(*di), GFP_KERNEL);
	if (!di) {
		dev_err(dev, "RAM error\n");
		ret = -1;
		goto prob_err;
	}

	di->dev = dev;
	platform_set_drvdata(pdev, di);

	ret = pmic_vm_parse_dts(di);
	if (ret != 0) {
		dev_err(di->dev, "dts error\n");
		goto prob_err;
	}

	di->regmap = devm_regmap_init(dev, NULL, di, &pmic_vm_regmap_config);
	if (IS_ERR(di->regmap)) {
		ret = PTR_ERR(di->regmap);
		dev_err(di->dev, "regmap init failed %d\n", ret);
		goto prob_err;
	}

	spin_lock_init(&di->buffer_lock);

	np = di->dev->of_node;
	ret = of_platform_populate(np, NULL, NULL, di->dev);
	if (ret != 0) {
		dev_info(di->dev, "%s populate fail %d\n", __func__, ret);
		goto prob_err;
	}

	pr_info("%s success\n", __func__);
	return 0;

prob_err:
	pr_err("%s ERROR\n", __func__);
	platform_set_drvdata(pdev, NULL);
	return ret;
}

static int pmic_vm_remove(struct platform_device *pdev)
{
	struct pmic_vm_dev *di = platform_get_drvdata(pdev);

	if (!di) {
		pr_err("%s di is NULL\n", __func__);
		return -1;
	}

	platform_set_drvdata(pdev, NULL);
	devm_kfree(&pdev->dev, di);
	return 0;
}

static const struct of_device_id of_pmic_vm_match_tbl[] = {
	{.compatible = "hisilicon,pmic-vm"},
	{ }
};

MODULE_DEVICE_TABLE(of, of_pmic_vm_match_tbl);

static struct platform_driver pmic_vm_driver = {
	.driver = {
		.name = "pmic-vm",
		.owner = THIS_MODULE,
		.of_match_table = of_pmic_vm_match_tbl,
	},
	.probe = pmic_vm_probe,
	.remove = pmic_vm_remove,
};

static int __init pmic_vm_init(void)
{
	return platform_driver_register(&pmic_vm_driver);
}

static void __exit pmic_vm_exit(void)
{
	platform_driver_unregister(&pmic_vm_driver);
}

subsys_initcall_sync(pmic_vm_init)
module_exit(pmic_vm_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("PMIC vm driver");
