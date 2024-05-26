/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#include <linux/module.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/time.h>

#include "hisi_chrdev.h"
#include "hisi_disp_debug.h"
#include "hisi_disp_dpp.h"
#include "test_dpp/hisi_dpp_hiace_test.h"
#include "isr/hisi_isr_dpp.h"

static struct hisi_disp_dpp *g_dpp_device[DISP_DPP_ID_MAX];
static ssize_t hisi_disp_dpp_debug_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return strlen(buf);
}

static ssize_t hisi_disp_dpp_debug_store(struct device *device,
	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static struct device_attribute dpp_attrs[] = {
	__ATTR(dpp_debug, S_IRUGO | S_IWUSR, hisi_disp_dpp_debug_show, hisi_disp_dpp_debug_store),
};

static int hisi_disp_dpp_open(struct inode *inode, struct file *filp)
{
	(void)inode;
	(void)filp;
	return 0;
}

int hisi_disp_dpp_on(void)
{
	struct hisi_disp_dpp *dpp_device = NULL;
	struct hisi_disp_isr_ops *isr_ops = NULL;
	char __iomem *dpu_base = NULL;

	dpp_device = g_dpp_device[DISP_DPP_0];
	disp_pr_info("dpp ++++++, dpp id %d", dpp_device->dpp_data.id);

	/* dpp have been opened */
	if (atomic_read(&dpp_device->dpp_data.ref_cnt) > 0) {
		disp_pr_info("dpp ref_cnt %d", dpp_device->dpp_data.ref_cnt);
		return 0;
	}
	dpu_base = dpp_device->dpp_data.dpu_base;

	isr_ops = dpp_device->dpp_data.isr_ctrl[COMP_ISR_PRE].isr_ops;
	hisi_disp_isr_open();
	if(isr_ops)
		isr_ops->interrupt_mask(dpu_base);
	enable_irq(dpp_device->dpp_data.isr_ctrl[COMP_ISR_PRE].irq_no);
	if(isr_ops) {
		isr_ops->interrupt_clear(dpu_base);
		isr_ops->interrupt_unmask(dpu_base, dpp_device);
	}

	atomic_inc(&dpp_device->dpp_data.ref_cnt);

	disp_pr_info("isr_ops %p------", isr_ops);
	return 0;
}

static void set_somebrightness(char __iomem *nr_base)
{
	uint32_t somebrightness0, somebrightness1, somebrightness2;
	uint32_t somebrightness3, somebrightness4;

	somebrightness0 = 819 & 0x3ff;
	somebrightness1 = 640 & 0x3ff;
	somebrightness2 = 384 & 0x3ff;
	somebrightness3 = 205 & 0x3ff;
	somebrightness4 = 96 & 0x3ff;
	dpu_set_reg(DPU_HIACE_S3_SOME_BRIGHTNESS01_ADDR(nr_base),
		somebrightness0 | (somebrightness1 << 16), 32, 0);
	dpu_set_reg(DPU_HIACE_S3_SOME_BRIGHTNESS23_ADDR(nr_base),
		somebrightness2 | (somebrightness3 << 16), 32, 0);
	dpu_set_reg(DPU_HIACE_S3_SOME_BRIGHTNESS4_ADDR(nr_base), somebrightness4, 32, 0);
}

static void set_color_sigma(char __iomem *nr_base)
{
	uint32_t min_sigma, max_sigma;
	uint32_t color_sigma0, color_sigma1, color_sigma2;
	uint32_t color_sigma3, color_sigma4, color_sigma5;
	uint32_t s3_green_sigma03;
	uint32_t s3_green_sigma45;
	uint32_t s3_red_sigma03;
	uint32_t s3_red_sigma45;
	uint32_t s3_blue_sigma03;
	uint32_t s3_blue_sigma45;
	uint32_t s3_white_sigma03;
	uint32_t s3_white_sigma45;
	uint32_t s3_min_max_sigma;
	min_sigma = 0x16 & 0x1f;
	max_sigma = 0x1f & 0x1f;
	dpu_set_reg(DPU_HIACE_S3_MIN_MAX_SIGMA_ADDR(nr_base), min_sigma | (max_sigma << 16), 32, 0);
	s3_min_max_sigma = min_sigma | (max_sigma << 16);

	color_sigma0 = 0x1f & 0x1f;
	color_sigma1 = 0x1f & 0x1f;
	color_sigma2 = 0x1e & 0x1f;
	color_sigma3 = 0x1e & 0x1f;
	color_sigma4 = 0x1e & 0x1f;
	color_sigma5 = 0x1e & 0x1f;

	s3_green_sigma03 = color_sigma0 | (color_sigma1 << 8) |
		(color_sigma2 << 16) | (color_sigma3 << 24);
	s3_red_sigma03 =  s3_green_sigma03;
	s3_blue_sigma03 = s3_green_sigma03;
	s3_white_sigma03 = s3_green_sigma03;

	s3_green_sigma45 = color_sigma4 | (color_sigma5 << 8);
	s3_red_sigma45 = s3_green_sigma45;
	s3_blue_sigma45 = s3_red_sigma45;
	s3_white_sigma45 = s3_red_sigma45;

	dpu_set_reg(DPU_HIACE_S3_GREEN_SIGMA03_ADDR(nr_base), s3_green_sigma03, 32, 0);
	dpu_set_reg(DPU_HIACE_S3_GREEN_SIGMA45_ADDR(nr_base), s3_green_sigma45, 32, 0);

	dpu_set_reg(DPU_HIACE_S3_RED_SIGMA03_ADDR(nr_base), s3_red_sigma03, 32, 0);
	dpu_set_reg(DPU_HIACE_S3_RED_SIGMA45_ADDR(nr_base), s3_red_sigma45, 32, 0);

	dpu_set_reg(DPU_HIACE_S3_BLUE_SIGMA03_ADDR(nr_base), s3_blue_sigma03, 32, 0);
	dpu_set_reg(DPU_HIACE_S3_BLUE_SIGMA45_ADDR(nr_base), s3_blue_sigma45, 32, 0);

	dpu_set_reg(DPU_HIACE_S3_WHITE_SIGMA03_ADDR(nr_base), s3_white_sigma03, 32, 0);
	dpu_set_reg(DPU_HIACE_S3_WHITE_SIGMA45_ADDR(nr_base), s3_white_sigma45, 32, 0);

	dpu_set_reg(DPU_HIACE_S3_FILTER_LEVEL_ADDR(nr_base), 20, 5, 0);
	dpu_set_reg(DPU_HIACE_S3_SIMILARITY_COEFF_ADDR(nr_base), 296, 10, 0);
	dpu_set_reg(DPU_HIACE_S3_V_FILTER_WEIGHT_ADJ_ADDR(nr_base), 0x1, 2, 0);
}

void init_noisereduction(char __iomem *dpu_base)
{
	char __iomem *nr_base = NULL;
	uint32_t s3_hue;
	uint32_t s3_saturation;
	uint32_t s3_value;
	uint32_t slop;
	uint32_t th_max, th_min;
	disp_pr_info("dpp ++++++, dpu_base %p", dpu_base);

	nr_base =  dpu_base + DSS_HIACE_OFFSET;
	disp_pr_info("dpp init nr");

	/* disable noisereduction */
	set_somebrightness (nr_base);
	set_color_sigma(nr_base);

	slop = 68 & 0xff;
	th_min = 0x0 & 0x1ff;
	th_max = 25 & 0x1ff;
	s3_hue = (slop << 24) | (th_max << 12) | th_min;
	dpu_set_reg(DPU_HIACE_S3_HUE_ADDR(nr_base), s3_hue, 32, 0);

	th_min = 80 & 0xff;
	th_max = 120 & 0xff;
	s3_saturation = (slop << 24) | (th_max << 12) | th_min;
	dpu_set_reg(DPU_HIACE_S3_SATURATION_ADDR(nr_base), s3_saturation, 32, 0);

	th_min = 120 & 0xff;
	th_max = 255 & 0xff;
	s3_value = (slop << 24) | (th_max << 12) | th_min;
	dpu_set_reg(DPU_HIACE_S3_VALUE_ADDR(nr_base), s3_value, 32, 0);

	dpu_set_reg(DPU_HIACE_S3_SKIN_GAIN_ADDR(nr_base), 0x80, 8, 0);
}

void init_hiace(char __iomem *dpu_base)
{
	char __iomem *hiace_base = NULL;
	unsigned long dw_jiffies = 0;
	uint32_t tmp = 0;
	bool is_ready = false;
	uint32_t global_hist_ab_work, global_hist_ab_shadow;
	uint32_t gamma_ab_work, gamma_ab_shadow;
	disp_pr_info("dpp ++++++, dpu_base %p", dpu_base);

	hiace_base =  dpu_base + DSS_HIACE_OFFSET;
	dpu_set_reg(DPU_HIACE_BYPASS_ACE_ADDR(hiace_base), 0x0, 1, 0);

	dpu_set_reg(DPU_HIACE_INIT_GAMMA_ADDR(hiace_base), 0x1, 1, 0);
	dpu_set_reg(DPU_HIACE_UPDATE_LOCAL_ADDR(hiace_base), 0x1, 1, 0); // do not used anymore
	dpu_set_reg(DPU_HIACE_UPDATE_FNA_ADDR(hiace_base), 0x1, 1, 0); // do not used anymore

	/* wait for gamma init finishing */
	dw_jiffies = jiffies + HZ / 2;
	do {
		tmp = inp32(DPU_HIACE_INIT_GAMMA_ADDR(hiace_base));
		if ((tmp & 0x1) != 0x1) {
			is_ready = true;
			break;
		}
	} while (time_after(dw_jiffies, jiffies));

	disp_pr_info("dpp ++++++, init gamma status %d", tmp);

	global_hist_ab_work = inp32(DPU_HIACE_GLOBAL_HIST_AB_WORK_ADDR(hiace_base));
	global_hist_ab_shadow = !global_hist_ab_work;

	gamma_ab_work = inp32(DPU_HIACE_GAMMA_AB_WORK_ADDR(hiace_base));
	gamma_ab_shadow = !gamma_ab_work;

	dpu_set_reg(DPU_HIACE_GAMMA_AB_SHADOW_ADDR(hiace_base), gamma_ab_shadow, 1, 0);
	dpu_set_reg(DPU_HIACE_GLOBAL_HIST_AB_SHADOW_ADDR(hiace_base), global_hist_ab_shadow, 1, 0);

	dpu_set_reg(DPU_HIACE_ACE_INT_STAT_ADDR(hiace_base),  0x1, 1, 0);
	dpu_set_reg(DPU_HIACE_ACE_INT_UNMASK_ADDR(hiace_base),  0x1, 1, 0);

	dpu_set_reg(DPU_HIACE_BYPASS_ACE_ADDR(hiace_base), 0x1, 1, 0);
}

static int hisi_disp_dpp_release(struct inode *inode, struct file *filp)
{
	(void)inode;
	(void)filp;
	return 0;
}

static long hisi_disp_dpp_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	(void)f;
	(void)cmd;
	(void)arg;
	return 0;
}

static struct file_operations dte_dpp_ops = {
	.owner = THIS_MODULE,
	.open = hisi_disp_dpp_open,
	.release = hisi_disp_dpp_release,
	.unlocked_ioctl = hisi_disp_dpp_ioctl,
	.compat_ioctl =  hisi_disp_dpp_ioctl,
};

static int hisi_disp_dpp_init_data(struct device_node *np, struct hisi_dpp_data *dpp_data)
{
	int ret;

	disp_pr_info("np name = %s", np->name);

	if (np->child)
		disp_pr_info("np child name = %s", np->child->name);

	ret = of_property_read_u32(np, "id", &dpp_data->id);
	if (ret) {
		disp_pr_err("read id fail");
		return -1;
	}

	ret = of_property_read_string(np, "dpp_name", &dpp_data->name);
	if (ret) {
		disp_pr_err("read dpp_name fail");
		return -1;
	}

	disp_pr_info("id=%u, name=%s", dpp_data->id, dpp_data->name);

	if(DISP_DPP_0 == dpp_data->id) {
		dpp_data->isr_ctrl[COMP_ISR_PRE].irq_no = irq_of_parse_and_map(np, 0);

		disp_pr_info("irq_no:%d", dpp_data->isr_ctrl[COMP_ISR_PRE].irq_no);
		if (0 == dpp_data->isr_ctrl[COMP_ISR_PRE].irq_no)
			disp_pr_err("get irq no fail 0");

		dpp_data->dpu_base = of_iomap(np, 0);
		disp_pr_info("ip_base_addrs:%p",dpp_data->dpu_base);
	}
	disp_pr_info("-----");

	return 0;
}

static void hisi_disp_dpp_create_chrdev(struct hisi_disp_dpp *dpp_dev)
{
	/* init chrdev info */
	dpp_dev->rm_chrdev.name = dpp_dev->dpp_data.name;
	dpp_dev->rm_chrdev.id = dpp_dev->dpp_data.id;
	dpp_dev->rm_chrdev.chrdev_fops = &dte_dpp_ops;

	hisi_disp_create_chrdev(&dpp_dev->rm_chrdev, dpp_dev);
	hisi_disp_create_attrs(dpp_dev->rm_chrdev.dte_cdevice, dpp_attrs, ARRAY_SIZE(dpp_attrs));
}

static int hisi_disp_dpp_probe(struct platform_device *pdev)
{
	struct hisi_disp_dpp *dpp_dev = NULL;
	uint32_t post_irq_unmask[IRQ_UNMASK_MAX] = {0};
	int ret;

	dpp_dev = devm_kzalloc(&pdev->dev, sizeof(*dpp_dev), GFP_KERNEL);
	if (!dpp_dev) {
		disp_pr_err("alloc rm device data fail");
		return -1;
	}

	/* read dtsi and init data */
	ret = hisi_disp_dpp_init_data(pdev->dev.of_node, &dpp_dev->dpp_data);
	if (ret) {
		disp_pr_err("init rm data fail");
		return -1;
	}
	disp_pr_info("dpu base %p, dpp %d", dpp_dev->dpp_data.dpu_base, dpp_dev->dpp_data.id);
	dpp_dev->dpp_data.hiace_end_wq = create_singlethread_workqueue("dpu_hiace_end");
	INIT_WORK(&dpp_dev->dpp_data.hiace_end_work, hiace_end_handle_func);

	if(DISP_DPP_0 == dpp_dev->dpp_data.id) {
		post_irq_unmask[IRQ_UNMASK_GLB]  = BIT_HIACE_IND;
		hisi_dpp_isr_init(&dpp_dev->dpp_data.isr_ctrl[COMP_ISR_PRE], dpp_dev->dpp_data.isr_ctrl[COMP_ISR_PRE].irq_no,
			post_irq_unmask, IRQ_UNMASK_MAX, hisi_config_get_irq_name(DISP_IRQ_SIGNAL_NS_MDP), dpp_dev);
#ifdef CONFIG_DPP_CORE
		init_noisereduction(dpp_dev->dpp_data.dpu_base);
		init_hiace(dpp_dev->dpp_data.dpu_base);
#endif
	}

	dpp_dev->pdev = pdev;
	platform_set_drvdata(pdev, dpp_dev);

	/* create chrdev */
	hisi_disp_dpp_create_chrdev(dpp_dev);

	g_dpp_device[dpp_dev->dpp_data.id % DISP_DPP_ID_MAX] = dpp_dev;
	return 0;
}

void hisi_disp_destroy_workqueue(struct workqueue_struct *wq)
{
	if(NULL != wq)
		destroy_workqueue(wq);
}

static int hisi_disp_dpp_remove(struct platform_device *pdev)
{
	struct hisi_disp_dpp *dpp_dev = NULL;
	struct hisi_dpp_data *dpp_data = NULL;

	dpp_dev = platform_get_drvdata(pdev);
	if (!dpp_dev) {
		disp_pr_err("NULL Pointer\n");
		return -EINVAL;
	}

	dpp_data = &dpp_dev->dpp_data;
	hisi_disp_destroy_workqueue(dpp_data->hiace_end_wq);
	return 0;
}

static const struct of_device_id hisi_disp_dpp_match_table[] = {
	{
		.compatible = HISI_DTS_COMP_DISP_DPP0,
		.data = NULL,
	},
	{
		.compatible = HISI_DTS_COMP_DISP_DPP1,
		.data = NULL,
	},
	{},
};
MODULE_DEVICE_TABLE(of, hisi_disp_dpp_match_table);

static struct platform_driver g_hisi_disp_dpp_driver = {
	.probe = hisi_disp_dpp_probe,
	.remove = hisi_disp_dpp_remove,
	.suspend = NULL,
	.resume = NULL,
	.shutdown = NULL,
	.driver = {
		.name = HISI_DISP_DPP_DEV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(hisi_disp_dpp_match_table),
	},
};

static int __init hisi_drv_dpp_init(void)
{
	int ret;

	disp_pr_info("enter +++++++ \n");

	ret = platform_driver_register(&g_hisi_disp_dpp_driver);
	if (ret) {
		disp_pr_info("platform_driver_register failed, error=%d!\n", ret);
		return ret;
	}

	disp_pr_info("enter -------- \n");
	return ret;
}

module_init(hisi_drv_dpp_init);
MODULE_DESCRIPTION("Hisilicon Dte Dpp Driver");
MODULE_LICENSE("GPL v2");

