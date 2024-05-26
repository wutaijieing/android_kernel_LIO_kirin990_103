/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/module.h>

#include "hisi_disp_composer.h"
#include "hisi_composer_core.h"
#include "hisi_composer_init.h"
#include "hisi_connector_utils.h"
#include "hisi_chrdev.h"
#include "hisi_disp_gadgets.h"
#include "isr/hisi_isr_offline.h"
#include "hisi_disp_pm.h"
#include "hisi_offline_adaptor.h"
#include "hisi_disp_iommu.h"

#define DTS_COMP_OFFLINE0_OVERLAY     "hisilicon,offline0_overlay"
#define DTS_COMP_OFFLINE1_OVERLAY     "hisilicon,offline1_overlay"

static struct hisi_composer_device *g_hisi_offline_dev;

static ssize_t hisi_offline_debug_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return strlen(buf);
}

static ssize_t hisi_offline_debug_store(struct device *device,
			struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static struct device_attribute offline_attrs[] = {
	__ATTR(offline_debug, S_IRUGO | S_IWUSR, hisi_offline_debug_show, hisi_offline_debug_store),

	/* TODO: other attrs */
};

static int hisi_offline_open(struct inode *inode, struct file *filp)
{
	struct hisi_composer_device *offline_dev;
	struct hisi_composer_priv_ops *priv_ops;

	disp_pr_info("++\n");

	if (!filp->private_data)
		filp->private_data = g_hisi_offline_dev;

	offline_dev = filp->private_data;

	/* offline_dev have been opened */
	if (atomic_read(&offline_dev->ref_cnt) > 0)
		return 0;

	priv_ops = &offline_dev->ov_priv_ops;
	if (priv_ops)
		priv_ops->overlay_on(offline_dev->id, offline_dev->pdev);

	atomic_inc(&offline_dev->ref_cnt);

	disp_pr_info(" enter--\n");
	return 0;
}

static int hisi_offline_release(struct inode *inode, struct file *filp)
{
	struct hisi_composer_device *offline_dev;
	struct hisi_composer_priv_ops *priv_ops;

	offline_dev = filp->private_data;
	priv_ops = &offline_dev->ov_priv_ops;

	/* offline_dev have not been opened, return */
	if (atomic_read(&offline_dev->ref_cnt) == 0)
		return 0;

	/* ref_cnt is not 0 */
	if (!atomic_sub_and_test(1, &offline_dev->ref_cnt))
		return 0;

	if (priv_ops)
		priv_ops->overlay_off(offline_dev->id, offline_dev->pdev);

	return 0;
}

static long hisi_offline_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	return 0;
}

static long hisi_offline_adaptor_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	struct hisi_composer_device *offline_dev;
	int ret = 0;

	if (_IOC_TYPE(cmd) != HISI_DISP_IOCTL_MAGIC) {
		disp_pr_err("cmd is err\n");
		return -EINVAL;
	}

	if (_IOC_NR(cmd) > DISP_IOCTL_CMD_MAX) {
		disp_pr_err("cmd is err\n");
		return -EINVAL;
	}

	offline_dev = f->private_data;
	if (!offline_dev) {
		disp_pr_err("offline_dev is nullptr\n");
		return -EINVAL;
	}

	down(&offline_dev->ov_data.ov_data_sem);

	ret = hisi_offline_adaptor_play(offline_dev, arg);
	/*switch (cmd) {
	case HISI_DISP_OFFLINE_PLAY:
		ret = hisi_offline_adaptor_play(offline_dev, arg);
		break;
	default:
		return 0;
	}*/

	up(&offline_dev->ov_data.ov_data_sem);
	return ret;
}

static struct file_operations offline_fops = {
	.owner = THIS_MODULE,
	.open = hisi_offline_open,
	.release = hisi_offline_release,
#ifdef CONFIG_DKMD_OLD_ARCH
	.unlocked_ioctl = hisi_offline_adaptor_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl =  hisi_offline_adaptor_ioctl,
#endif
#else
	.unlocked_ioctl = hisi_offline_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl =  hisi_offline_ioctl,
#endif
#endif
};

static void hisi_drv_offline_create_chrdev(struct hisi_composer_device *ov_dev)
{
	/* init chrdev info */
	ov_dev->ov_chrdev.name = ov_dev->ov_data.ov_name;
	ov_dev->ov_chrdev.id = ov_dev->ov_data.ov_id;
	ov_dev->ov_chrdev.chrdev_fops = &offline_fops;

	hisi_disp_create_chrdev(&ov_dev->ov_chrdev, ov_dev);
	hisi_disp_create_attrs(ov_dev->ov_chrdev.dte_cdevice, offline_attrs, ARRAY_SIZE(offline_attrs));
}

static int hisi_drv_offline_init_data(struct device_node *np, struct hisi_composer_data *ov_data)
{
	int ret;

	ret = of_property_read_u32(np, "id", &ov_data->ov_id);
	if (ret) {
		disp_pr_err("read id fail");
		return -1;
	}

	ret = of_property_read_string(np, "offline_name", &ov_data->ov_name);
	if (ret) {
		disp_pr_err("read name fail");
		return -1;
	}

	/* TODO: maybe need other clk and regulator */
	ov_data->clk_bits = BIT(PM_CLK_ACLK_GATE) | BIT(PM_CLK_CORE) | BIT(PM_CLK_PCKL_GATE);
	ov_data->regulator_bits = BIT(PM_REGULATOR_SMMU) | BIT(PM_REGULATOR_DPU) | BIT(PM_REGULATOR_VIVOBUS) | BIT(PM_REGULATOR_MEDIA1);
	ov_data->scene_id = DPU_SCENE_OFFLINE_0;
	sema_init(&ov_data->ov_data_sem, 1);

	hisi_offline_init_param_init(ov_data);

	disp_pr_info("name=%s, id=%u, caps=%u", ov_data->ov_name, ov_data->ov_id, ov_data->ov_caps);

	return 0;
}

static void hisi_drv_offline_init_priv_data(struct hisi_composer_device *ov_dev)
{
	memset(&ov_dev->frame, 0, sizeof(struct hisi_display_frame));
	atomic_set(&(ov_dev->ref_cnt), 0);
}

static int hisi_drv_offline_probe(struct platform_device *pdev)
{
	struct hisi_composer_device *ov_dev = NULL;
	const uint32_t *irq_signals;
	uint32_t pre_irq_unmask[IRQ_UNMASK_MAX] = {0};
	int ret;

	disp_pr_info("enter +++++++, pdev=%p \n", pdev);

	ov_dev = devm_kzalloc(&pdev->dev, sizeof(*ov_dev), GFP_KERNEL);
	if (!ov_dev)
		return -1;
	ov_dev->pdev = pdev;

	/* init overlay data */
	ret = hisi_drv_offline_init_data(pdev->dev.of_node, &ov_dev->ov_data);
	if (ret)
		return -1;

	/* init base addr */
	hisi_config_init_ip_base_addr(ov_dev->ov_data.ip_base_addrs, DISP_IP_MAX);

	/* init isr */
	irq_signals = hisi_disp_config_get_irqs();
	memset(pre_irq_unmask, 0, sizeof(pre_irq_unmask));

	pre_irq_unmask[IRQ_UNMASK_OFFLINE0] = BIT(19) | DPU_WCH1_NS_INT;
	hisi_offline_isr_init(&ov_dev->ov_data.isr_ctrl[COMP_ISR_PRE], irq_signals[DISP_IRQ_SIGNAL_NS_L2_M1],
		pre_irq_unmask, IRQ_UNMASK_MAX, hisi_config_get_irq_name(DISP_IRQ_SIGNAL_NS_L2_M1), ov_dev);

	/* init private ops */
	hisi_offline_composer_register_private_ops(&ov_dev->ov_priv_ops);
	platform_set_drvdata(pdev, ov_dev);

	hisi_drv_offline_init_priv_data(ov_dev);

	/* create chrdev */
	hisi_drv_offline_create_chrdev(ov_dev);

	g_hisi_offline_dev = ov_dev;

	disp_pr_info("enter ------------, pdev=%p \n", pdev);

	return 0;
}

static int hisi_drv_offline_remove(struct platform_device *pdev)
{
	/* TODO: remove device */

	return 0;
}

static const struct of_device_id hisi_offine_ov_match_table[] = {
	{
		.compatible = DTS_COMP_OFFLINE0_OVERLAY,
		.data = NULL,
	},
	{},
};
MODULE_DEVICE_TABLE(of, hisi_offine_ov_match_table);

static struct platform_driver g_hisi_offline_composer_driver = {
	.probe = hisi_drv_offline_probe,
	.remove = hisi_drv_offline_remove,
	.suspend = NULL,
	.resume = NULL,
	.shutdown = NULL,
	.driver = {
		.name = HISI_OFFLINE_OVERLAY_DEV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(hisi_offine_ov_match_table),
	},
};

static int __init hisi_drv_offline_composer_init(void)
{
	int ret;

	disp_pr_info("enter +++++++ \n");

	ret = platform_driver_register(&g_hisi_offline_composer_driver);
	if (ret) {
		disp_pr_info("platform_driver_register failed, error=%d!\n", ret);
		return ret;
	}

	disp_pr_info("enter -------- \n");
	return ret;
}

module_init(hisi_drv_offline_composer_init);

MODULE_DESCRIPTION("Hisilicon mipi dsi Driver");
MODULE_LICENSE("GPL v2");

