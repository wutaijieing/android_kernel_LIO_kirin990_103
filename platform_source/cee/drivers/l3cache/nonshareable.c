/*
 * nonshareable.c
 *
 * nonshareable driver
 *
 * Copyright (c) 2020-2020 Huawei Technologies Co., Ltd.
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
#define pr_fmt(fmt) "nonshareable: " fmt
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/of_platform.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <securec.h>
#include <soc_fcm_namtso_interface.h>
#include <soc_acpu_baseaddr_interface.h>

struct hisi_nonshareable_info {
	int nonshare_irq;
	void __iomem *nonshare_status_addr;
};

static irqreturn_t hisi_nonshare_handler(int irq, void *data)
{
	struct hisi_nonshareable_info *info = (struct hisi_nonshareable_info *)data;
	u64 value;

	value = readq(info->nonshare_status_addr);
	if ((value & BIT(SOC_FCM_NAMTSO_NON_SHARE_STATUS_non_shared_detect_START))) {
		pr_err("[%s] detect non-shareable access, value = 0x%llx\n",
		       __func__, value);
	} else {
		pr_err("[%s] not expected status\n", __func__);
	}

	/* clear int */
	value &= ~BIT(SOC_FCM_NAMTSO_NON_SHARE_STATUS_non_shared_detect_START);
	/* bitmasken */
	value |= BIT(SOC_FCM_NAMTSO_NON_SHARE_STATUS_non_shared_detect_START + 32);
	writeq(value, info->nonshare_status_addr);

	return IRQ_HANDLED;
}

static int hisi_nonshareable_probe(struct platform_device *pdev)
{
	struct hisi_nonshareable_info *info = NULL;
	struct device *dev = &pdev->dev;
	int ret;

	dev_err(dev, "enter\n");

	info = devm_kzalloc(dev, sizeof(*info), GFP_KERNEL);
	if (info == NULL) {
		dev_err(dev, "alloc memory error\n");
		ret = -ENOMEM;
		goto err;
	}
	platform_set_drvdata(pdev, info);

	info->nonshare_status_addr = ioremap(SOC_FCM_NAMTSO_NON_SHARE_STATUS_ADDR(SOC_ACPU_namtso_cfg_BASE_ADDR), 0x8);
	if (info->nonshare_status_addr == NULL) {
		dev_err(dev, "nonshare_status_addr ioremap failed\n");
		ret = -ENOMEM;
		goto err;
	}

	/* cacheable nonshare irq */
	info->nonshare_irq = platform_get_irq_byname(pdev, "nonshare-irq");
	if (info->nonshare_irq < 0) {
		dev_err(dev, "failed to get nonshare-irq %d\n",
			info->nonshare_irq);
		ret = -ENOENT;
		goto err_ioremap;
	}
	ret = devm_request_threaded_irq(dev,
					info->nonshare_irq,
					NULL,
					hisi_nonshare_handler,
					IRQF_TRIGGER_NONE | IRQF_ONESHOT,
					"nonshare_irq",
					info);
	if (ret < 0) {
		dev_err(dev, "failed to request nonshare_irq, ret = %d\n", ret);
		ret = -EIO;
		goto err_ioremap;
	}
	dev_dbg(dev, "nonshare_irq = %d\n", info->nonshare_irq);

	dev_err(dev, "success\n");
	return 0;

err_ioremap:
	iounmap(info->nonshare_status_addr);
err:
	return ret;
}

static int hisi_nonshareable_remove(struct platform_device *pdev)
{
	struct hisi_nonshareable_info *info = platform_get_drvdata(pdev);
	struct device *dev = &pdev->dev;

	iounmap(info->nonshare_status_addr);
	devm_free_irq(dev, info->nonshare_irq, info);

	return 0;
}

#define MODULE_NAME		"hisi,nonshareable"
static const struct of_device_id hisi_nonshareable_match[] = {
	{ .compatible = MODULE_NAME },
	{},
};

static struct platform_driver hisi_nonshareable_drv = {
	.probe		= hisi_nonshareable_probe,
	.remove		= hisi_nonshareable_remove,
	.driver = {
		.name	= MODULE_NAME,
		.of_match_table = of_match_ptr(hisi_nonshareable_match),
	},
};

static int __init hisi_nonshareable_init(void)
{
	return platform_driver_register(&hisi_nonshareable_drv);
}

static void __exit hisi_nonshareable_exit(void)
{
	platform_driver_unregister(&hisi_nonshareable_drv);
}

module_init(hisi_nonshareable_init);
module_exit(hisi_nonshareable_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Hisi nonshareable Driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
