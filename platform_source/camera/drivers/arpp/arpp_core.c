/*
 * Copyright (C) 2019-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/fs.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/dma-mapping.h>
#include <linux/dma-buf.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/string.h>

#include "arpp_core.h"
#include "platform.h"
#include "arpp_log.h"
#include "arpp_hwacc.h"
#include "arpp_isr.h"
#include "arpp_smmu.h"
#include "arpp_common_define.h"

struct arpp_core {
	/* char device info */
	struct platform_device *pdev;
	struct device_node *dev_node;
	struct class *dev_class;
	struct device *dev;
	int major;
	int minor;

	/* platform info */
	struct arpp_device arpp_dev;

	/* arpp module */
	struct arpp_hwacc hwacc_info;
	struct arpp_ahc ahc_info;
};

static struct arpp_core *g_arpp_core;
/*open close device */
static atomic_t g_device_ref;
/* arpp power manager */
static atomic_t g_power_ref;
/* hwacc */
static atomic_t g_hwacc_ref;
/* external interface lock */
static struct mutex g_ioctl_lock;

static int hisi_arpp_remove(struct platform_device *pdev);
static int hisi_arpp_hwacc_deinit_internal(void);
static int hisi_arpp_power_down_internal(void);
static int hisi_arpp_wait_for_power_down_internal(void);

static int arpp_open(struct inode *inode, struct file *filp)
{
	struct arpp_core *core_prv = NULL;
	UNUSED(inode);

	mutex_lock(&g_ioctl_lock);

	if (atomic_read(&g_device_ref) > 0) {
		mutex_unlock(&g_ioctl_lock);
		hiar_loge("arpp has been opened!");
		return -EBUSY;
	}

	if (filp == NULL) {
		mutex_unlock(&g_ioctl_lock);
		hiar_loge("device file is NULL");
		return -EINVAL;
	}

	if (filp->private_data == NULL)
		filp->private_data = g_arpp_core;

	core_prv = filp->private_data;
	if (core_prv == NULL) {
		mutex_unlock(&g_ioctl_lock);
		hiar_loge("core_prv is NULL");
		return -EINVAL;
	}

	atomic_inc(&g_device_ref);

	mutex_unlock(&g_ioctl_lock);

	return 0;
}

static int arpp_release(struct inode *inode, struct file *filp)
{
	struct arpp_core *core_prv = NULL;
	UNUSED(inode);

	mutex_lock(&g_ioctl_lock);

	if (atomic_read(&g_device_ref) == 0) {
		mutex_unlock(&g_ioctl_lock);
		hiar_loge("arpp dev has been closed!");
		return -EBUSY;
	}

	if (filp == NULL) {
		mutex_unlock(&g_ioctl_lock);
		hiar_loge("device file is NULL");
		return -EINVAL;
	}

	core_prv = filp->private_data;
	if (core_prv == NULL) {
		mutex_unlock(&g_ioctl_lock);
		hiar_loge("core_prv is NULL");
		return -EINVAL;
	}

	/*
	 * If it is the last reference,
	 * release buffers and power down.
	 */
	if (atomic_read(&g_device_ref) == 1) {
		(void)hisi_arpp_wait_for_power_down_internal();
		/* set cmdlist iova to 0 */
		core_prv->hwacc_info.cmdlist_buf.iova = 0;
		(void)hisi_arpp_power_down_internal();
	}

	atomic_dec(&g_device_ref);

	mutex_unlock(&g_ioctl_lock);

	return 0;
}

static int hisi_arpp_power_up_internal(void)
{
	int ret;

	hiar_logi("+");

	if (atomic_read(&g_power_ref) > 0) {
		hiar_logw("arpp has been power up!");
		return 0;
	}

	if (g_arpp_core == NULL) {
		hiar_loge("arpp core is NULL");
		return -EINVAL;
	}

	ret = power_up(&g_arpp_core->arpp_dev);
	if (ret != 0) {
		hiar_loge("power up failed, error=%d", ret);
		return ret;
	}

	init_and_reset(&g_arpp_core->arpp_dev);

	open_auto_lp_mode(&g_arpp_core->arpp_dev);

	mask_and_clear_isr(&g_arpp_core->arpp_dev);

	ret = smmu_enable(&g_arpp_core->arpp_dev);
	if (ret != 0) {
		hiar_loge("smmu enable failed, error=%d", ret);
		goto err_power_down;
	}

	ret = register_irqs(&g_arpp_core->ahc_info);
	if (ret != 0) {
		hiar_loge("register irqs failed, error=%d", ret);
		goto err_smmu_deinit;
	}

	ret = enable_irqs(&g_arpp_core->ahc_info);
	if (ret != 0) {
		hiar_loge("enable irqs failed, error=%d", ret);
		goto err_unregister_irqs;
	}

	atomic_inc(&g_power_ref);

	hiar_logi("-");

	return 0;

err_unregister_irqs:
	unregister_irqs(&g_arpp_core->ahc_info);

err_smmu_deinit:
	smmu_disable(&g_arpp_core->arpp_dev);

err_power_down:
	(void)power_down(&g_arpp_core->arpp_dev);

	return ret;
}

static int hisi_arpp_power_down_internal(void)
{
	int ret;

	hiar_logi("+");

	if (atomic_read(&g_hwacc_ref) > 0) {
		hiar_logw("hwacc need to deinit!");
		return -1;
	}

	if (atomic_read(&g_power_ref) <= 0) {
		hiar_logw("arpp has never been power up!");
		return 0;
	}

	if (g_arpp_core == NULL) {
		hiar_logw("arpp_core is NULL");
		return -EINVAL;
	}

	send_hwacc_init_completed_signal(&g_arpp_core->ahc_info);
	send_lba_calculation_completed_signal(&g_arpp_core->ahc_info);

	disable_irqs(&g_arpp_core->ahc_info);

	unregister_irqs(&g_arpp_core->ahc_info);

	smmu_disable(&g_arpp_core->arpp_dev);

	ret = power_down(&g_arpp_core->arpp_dev);
	if (ret != 0)
		hiar_loge("power down failed, error=%d", ret);

	atomic_dec(&g_power_ref);

	hiar_logi("-");

	return 0;
}

static int hisi_arpp_hwacc_init_internal(void)
{
	int ret;

	hiar_logi("+");

	if (atomic_read(&g_power_ref) <= 0) {
		hiar_loge("arpp has never been power up!");
		return -EBUSY;
	}

	if (atomic_read(&g_hwacc_ref) > 0) {
		hiar_loge("hwacc has been inited!");
		return 0;
	}

	if (g_arpp_core == NULL) {
		hiar_loge("arpp_core is NULL");
		return -EINVAL;
	}

	ret = hwacc_init(&g_arpp_core->hwacc_info, &g_arpp_core->ahc_info);
	if (ret != 0) {
		hiar_loge("hwacc_init failed, error=%d", ret);
		return ret;
	}

	atomic_inc(&g_hwacc_ref);

	hiar_logi("-");

	return 0;
}

static int hisi_arpp_hwacc_deinit_internal(void)
{
	int ret;

	hiar_logi("+");

	if (atomic_read(&g_power_ref) <= 0) {
		hiar_logw("arpp has never been power up!");
		return -1;
	}

	if (atomic_read(&g_hwacc_ref) <= 0) {
		hiar_logw("hwacc has been deinited!");
		return 0;
	}

	if (g_arpp_core == NULL) {
		hiar_loge("arpp_core is NULL");
		return -EINVAL;
	}

	ret = hwacc_deinit(&g_arpp_core->hwacc_info, &g_arpp_core->ahc_info);
	if (ret != 0) {
		hiar_loge("hwacc deinit failed, error=%d", ret);
		return ret;
	}

	atomic_dec(&g_hwacc_ref);

	hiar_logi("-");

	return 0;
}

static int hisi_arpp_wait_for_power_down_internal(void)
{
	int ret;

	hiar_logi("+");

	if (atomic_read(&g_power_ref) <= 0) {
		hiar_logw("arpp has never been power up!");
		return -1;
	}

	if (atomic_read(&g_hwacc_ref) <= 0) {
		hiar_logw("hwacc has been deinited!");
		return 0;
	}

	if (g_arpp_core == NULL) {
		hiar_loge("arpp_core is NULL");
		return -EINVAL;
	}

	ret = hwacc_wait_for_power_down(&g_arpp_core->hwacc_info,
		&g_arpp_core->ahc_info);
	if (ret != 0) {
		hiar_loge("hwacc wait for power down failed, error=%d", ret);
		return ret;
	}

	atomic_dec(&g_hwacc_ref);

	hiar_logi("-");

	return 0;
}

static int hisi_arpp_hwacc_reset_internal(void)
{
	int ret;

	ret = hisi_arpp_wait_for_power_down_internal();
	if (ret != 0)
		hiar_loge("wait for power down failed, error=%d", ret);


	if (atomic_read(&g_power_ref) <= 0) {
		hiar_logw("arpp has never been power up!");
		return 0;
	}

	if (g_arpp_core == NULL) {
		hiar_logw("arpp_core is NULL");
		return -EINVAL;
	}

	send_hwacc_init_completed_signal(&g_arpp_core->ahc_info);
	send_lba_calculation_completed_signal(&g_arpp_core->ahc_info);

	smmu_disable(&g_arpp_core->arpp_dev);

	ret = power_down(&g_arpp_core->arpp_dev);
	if (ret != 0)
		hiar_loge("power down failed, error=%d", ret);

	atomic_dec(&g_power_ref);

	ret = power_up(&g_arpp_core->arpp_dev);
	if (ret != 0) {
		hiar_loge("power up failed, error=%d", ret);
		return ret;
	}

	init_and_reset(&g_arpp_core->arpp_dev);
	open_auto_lp_mode(&g_arpp_core->arpp_dev);
	mask_and_clear_isr(&g_arpp_core->arpp_dev);
	ret = smmu_enable(&g_arpp_core->arpp_dev);
	if (ret != 0) {
		hiar_loge("smmu enable failed, error=%d", ret);
		(void)power_down(&g_arpp_core->arpp_dev);
		return ret;
	}

	atomic_inc(&g_power_ref);

	ret = hisi_arpp_hwacc_init_internal();
	if (ret != 0) {
		hiar_loge("hwacc init failed, error=%d", ret);
		(void)hisi_arpp_power_down_internal();
		return ret;
	}

	return 0;
}

static int hisi_arpp_power_up(void)
{
	int ret;

	mutex_lock(&g_ioctl_lock);

	ret = hisi_arpp_power_up_internal();
	if (ret != 0) {
		hiar_loge("power up failed, error=%d", ret);
		mutex_unlock(&g_ioctl_lock);
		return ret;
	}

	mutex_unlock(&g_ioctl_lock);

	return 0;
}

static int hisi_arpp_power_down(void)
{
	int ret;

	mutex_lock(&g_ioctl_lock);

	ret = hisi_arpp_power_down_internal();
	if (ret != 0) {
		hiar_loge("power down failed, error=%d", ret);
		mutex_unlock(&g_ioctl_lock);
		return ret;
	}

	mutex_unlock(&g_ioctl_lock);

	return ret;
}

static int hisi_arpp_set_clk_level(unsigned long args)
{
	int ret;
	int clk_level;
	void __user *argp = NULL;

	mutex_lock(&g_ioctl_lock);

	/*
	 * When arpp has been powered down,
	 * HAL can call hisi_arpp_set_clk_level().
	 */
	if (atomic_read(&g_power_ref) > 0) {
		hiar_loge("arpp has been powered up, set clk level failed!");
		mutex_unlock(&g_ioctl_lock);
		return -EBUSY;
	}

	if (g_arpp_core == NULL) {
		hiar_loge("arpp_core is NULL");
		mutex_unlock(&g_ioctl_lock);
		return -EINVAL;
	}

	argp = (void __user *)(uintptr_t)args;
	if (argp == NULL) {
		hiar_loge("argp is invalid");
		mutex_unlock(&g_ioctl_lock);
		return -EINVAL;
	}

	ret = (int)copy_from_user(&clk_level, argp, sizeof(clk_level));
	if (ret != 0) {
		hiar_loge("copy_from_user failed, error=%d", ret);
		mutex_unlock(&g_ioctl_lock);
		return ret;
	}

	if (clk_level < CLK_LEVEL_LOW || clk_level > CLK_LEVEL_TURBO) {
		hiar_loge("clk_level=%d is invalid", clk_level);
		mutex_unlock(&g_ioctl_lock);
		return -EINVAL;
	}

	/* only save clk_level from user */
	g_arpp_core->arpp_dev.clk_level = clk_level;
	hiar_logi("get clk level=%d from HAL", clk_level);

	mutex_unlock(&g_ioctl_lock);

	return 0;
}

static int hisi_arpp_hwacc_init(unsigned long args)
{
	int ret;
	struct hwacc_ini_info ini_info;
	void __user *argp = NULL;
	struct arpp_hwacc *hwacc_info = NULL;
	struct lba_buffer_info *lba_buf_info = NULL;

	mutex_lock(&g_ioctl_lock);

	if (g_arpp_core == NULL) {
		hiar_loge("arpp_core is NULL");
		mutex_unlock(&g_ioctl_lock);
		return -EINVAL;
	}

	argp = (void __user *)(uintptr_t)args;
	if (argp == NULL) {
		hiar_loge("argp is invalid");
		mutex_unlock(&g_ioctl_lock);
		return -EINVAL;
	}

	ret = (int)copy_from_user(&ini_info, argp, sizeof(struct hwacc_ini_info));
	if (ret != 0) {
		hiar_loge("copy_from_user failed, error=%d", ret);
		mutex_unlock(&g_ioctl_lock);
		return ret;
	}

	/* update ctx(hwacc_info)'s cmdlist_buf */
	hwacc_info = &g_arpp_core->hwacc_info;
	lba_buf_info = &hwacc_info->lba_buf_info;
	hwacc_info->cmdlist_buf.base_buf = ini_info.cmdlist_buf;
	lba_buf_info->inout_buf.base_buf = ini_info.inout_buf;
	lba_buf_info->swap_buf.base_buf = ini_info.swap_buf;
	lba_buf_info->free_buf.base_buf = ini_info.free_buf;
	lba_buf_info->out_buf.base_buf = ini_info.out_buf;

	hiar_logi("cmdlist[%d], inout[%d], swap[%d], free[%d], out[%d]",
		ini_info.cmdlist_buf.fd, ini_info.inout_buf.fd, ini_info.swap_buf.fd,
		ini_info.free_buf.fd, ini_info.out_buf.fd);

	ret = hisi_arpp_hwacc_init_internal();
	if (ret != 0) {
		hiar_loge("hwacc init failed, error=%d", ret);
		mutex_unlock(&g_ioctl_lock);
		return ret;
	}

	mutex_unlock(&g_ioctl_lock);

	return 0;
}

static int hisi_arpp_hwacc_deinit(void)
{
	int ret;

	mutex_lock(&g_ioctl_lock);

	ret = hisi_arpp_hwacc_deinit_internal();
	if (ret != 0) {
		hiar_loge("hwacc deinit failed, error=%d", ret);
		mutex_unlock(&g_ioctl_lock);
		return ret;
	}

	mutex_unlock(&g_ioctl_lock);

	return 0;
}

static int hisi_arpp_calculate_lba(unsigned long args)
{
	int ret;
	void __user *argp;
	struct arpp_lba_info lba_info;

	mutex_lock(&g_ioctl_lock);

	if (atomic_read(&g_power_ref) <= 0) {
		hiar_loge("arpp has never been power up!");
		mutex_unlock(&g_ioctl_lock);
		return -EBUSY;
	}

	if (atomic_read(&g_hwacc_ref) <= 0) {
		hiar_loge("hwacc has never been inited!");
		mutex_unlock(&g_ioctl_lock);
		return -EBUSY;
	}

	if (g_arpp_core == NULL) {
		hiar_loge("g_arpp_core is NULL");
		mutex_unlock(&g_ioctl_lock);
		return -EINVAL;
	}

	argp = (void __user *)(uintptr_t)args;
	if (argp == NULL) {
		hiar_loge("argp is NULL");
		mutex_unlock(&g_ioctl_lock);
		return -EINVAL;
	}

	ret = (int)copy_from_user(&lba_info, argp, sizeof(struct arpp_lba_info));
	if (ret != 0) {
		hiar_loge("copy_from_user failed, error=%d", ret);
		mutex_unlock(&g_ioctl_lock);
		return -EINVAL;
	}

	ret = do_lba_calculation(&g_arpp_core->hwacc_info,
		&g_arpp_core->ahc_info, &lba_info);
	if (ret != 0)
		hiar_loge("do_lba_calculation failed, error=%d", ret);

	mutex_unlock(&g_ioctl_lock);

	return ret;
}

static long arpp_ioctl(struct file *filp, unsigned int cmd, unsigned long args)
{
	long ret;

	hiar_logi("cmd=%u", cmd);

	if (_IOC_TYPE(cmd) != ARPP_CTRL_MAGIC) {
		hiar_loge("verify magic code[%u] failed", _IOC_TYPE(cmd));
		return -EINVAL;
	}

	if (_IOC_NR(cmd) > ARPP_CTRL_MAX_NR) {
		hiar_loge("verify max nr[%u] fail", _IOC_NR(cmd));
		return -EINVAL;
	}

	switch (cmd) {
	case ARPP_IOCTL_POWER_UP:
		ret = hisi_arpp_power_up();
		break;
	case ARPP_IOCTL_POWER_DOWN:
		ret = hisi_arpp_power_down();
		break;
	case ARPP_IOCTL_SET_CLK_LEVEL:
		ret = hisi_arpp_set_clk_level(args);
		break;
	case ARPP_IOCTL_LOAD_ENGINE:
		ret = hisi_arpp_hwacc_init(args);
		break;
	case ARPP_IOCTL_UNLOAD_ENGINE:
		ret = hisi_arpp_hwacc_deinit();
		break;
	case ARPP_IOCTL_DO_LBA:
		ret = hisi_arpp_calculate_lba(args);
		break;
	default:
		ret = -EINVAL;
		hiar_logw("invalid ioctl cmd %u!", cmd);
		break;
	}

	hiar_logi("-");

	return ret;
}

static const struct file_operations hisi_arpp_fops = {
	.owner = THIS_MODULE,
	.open = arpp_open,
	.release = arpp_release,
	.unlocked_ioctl = arpp_ioctl,
	.compat_ioctl = arpp_ioctl
};

static void prepare_arpp_device(struct arpp_core *core_prv)
{
	struct arpp_hwacc *hwacc_info = NULL;
	struct arpp_ahc *ahc_info = NULL;

	if (core_prv == NULL) {
		hiar_loge("arpp dev is NULL");
		return;
	}

	/* prepare hwacc_info */
	hwacc_info = &core_prv->hwacc_info;
	hwacc_info->dev = &core_prv->pdev->dev;
	hwacc_info->arpp_dev = &core_prv->arpp_dev;

	/* prepare ahc_info */
	ahc_info = &core_prv->ahc_info;
	ahc_info->arpp_dev = &core_prv->arpp_dev;
	ahc_info->hwacc_reset_intr = hisi_arpp_hwacc_reset_internal;
}

static int register_chr_device(struct arpp_core *core_prv)
{
	if (core_prv == NULL) {
		hiar_loge("arpp dev is NULL");
		return -EINVAL;
	}

	core_prv->major = register_chrdev(0, DEV_NAME, &hisi_arpp_fops);
	if (core_prv->major < 0) {
		hiar_loge("fail to register driver!");
		return -EFAULT;
	}
	core_prv->minor = 0;

	core_prv->dev_class = class_create(THIS_MODULE, DEV_NAME);
	if (core_prv->dev_class == NULL) {
		hiar_loge("fail to create arpp class!");
		goto err_unregister_chrdev;
	}

	core_prv->dev = device_create(core_prv->dev_class, NULL,
		MKDEV((unsigned int)core_prv->major,
		(unsigned int)core_prv->minor), NULL, DEV_NAME);
	if (core_prv->dev == NULL) {
		hiar_loge("fail to create arpp device!");
		goto err_class_destroy;
	}

	return 0;

err_class_destroy:
	class_destroy(core_prv->dev_class);

err_unregister_chrdev:
	unregister_chrdev(core_prv->major, DEV_NAME);
	core_prv->major = -1;

	return -EFAULT;
}

static void unregister_chr_device(struct arpp_core *core_prv)
{
	if (core_prv == NULL) {
		hiar_loge("arpp core is NULL");
		return;
	}

	if (core_prv->dev_class != NULL) {
		if (core_prv->dev != NULL) {
			device_destroy(core_prv->dev_class,
				MKDEV((unsigned int)core_prv->major,
				(unsigned int)core_prv->minor));
			core_prv->dev = NULL;
		}

		if (core_prv->dev_class != NULL) {
			class_destroy(core_prv->dev_class);
			core_prv->dev_class = NULL;
		}
	}

	if (core_prv->major >= 0) {
		unregister_chrdev(core_prv->major, DEV_NAME);
		core_prv->major = -1;
	}
}

static void init_global_mutex_and_atmoic()
{
	atomic_set(&g_device_ref, 0);
	atomic_set(&g_power_ref, 0);
	atomic_set(&g_hwacc_ref, 0);
	mutex_init(&g_ioctl_lock);
}

static void deinit_global_mutex_and_atmoic()
{
	mutex_destroy(&g_ioctl_lock);
	atomic_set(&g_device_ref, 0);
	atomic_set(&g_power_ref, 0);
	atomic_set(&g_hwacc_ref, 0);
}

static int hisi_arpp_probe(struct platform_device *pdev)
{
	int ret;
	struct arpp_core *core_prv = NULL;
	struct device_node *np = NULL;

	hiar_logi("+");

	ret = is_arpp_available();
	if (ret != 0) {
		hiar_logw("arpp is not available, do not to probe");
		return 0;
	}

	if (pdev == NULL) {
		hiar_loge("platform device is null");
		return -EINVAL;
	}

	np = of_find_compatible_node(NULL, NULL, DTS_COMP_NAME);
	if (np == NULL) {
		hiar_loge("can't find dtsi node");
		return -EINVAL;
	}

	core_prv = kzalloc(sizeof(*core_prv), GFP_KERNEL);
	if (core_prv == NULL) {
		hiar_loge("failed to alloc arpp_core");
		of_node_put(np);
		return -EINVAL;
	}
	(void)memset(core_prv, 0, sizeof(*core_prv));

	core_prv->pdev = pdev;
	core_prv->dev_node = np;

	init_global_mutex_and_atmoic();

	ret = get_all_registers(&core_prv->arpp_dev, core_prv->dev_node);
	if (ret != 0)
		goto err_arpp_remove;

	ret = get_sid_ssid(&core_prv->arpp_dev, core_prv->dev_node);
	if (ret != 0)
		goto err_arpp_remove;

	ret = get_irqs(&core_prv->arpp_dev, core_prv->dev_node);
	if (ret != 0)
		goto err_arpp_remove;

	ret = get_clk(&core_prv->arpp_dev,
		&core_prv->pdev->dev, core_prv->dev_node);
	if (ret != 0)
		goto err_arpp_remove;

	ret = get_regulator(&core_prv->arpp_dev, &core_prv->pdev->dev);
	if (ret != 0)
		goto err_arpp_remove;

	ret = register_chr_device(core_prv);
	if (ret != 0)
		goto err_arpp_remove;

	prepare_arpp_device(core_prv);

	platform_set_drvdata(pdev, core_prv);

	/* for linux kernel 5.10 */
	core_prv->arpp_dev.wake_lock = wakeup_source_register(&core_prv->pdev->dev, "arpp_wakelock");
	if (!core_prv->arpp_dev.wake_lock) {
		hiar_loge("wakeup source register failed");
		goto err_arpp_remove;
	}

	g_arpp_core = core_prv;

	hiar_logi("-");

	return 0;

err_arpp_remove:
	(void)hisi_arpp_remove(pdev);

	if (core_prv != NULL) {
		kfree(core_prv);
		g_arpp_core = NULL;
	}

	return ret;
}

static int hisi_arpp_remove(struct platform_device *pdev)
{
	int ret;
	struct arpp_core *core_prv = NULL;

	hiar_logi("+");

	ret = is_arpp_available();
	if (ret != 0) {
		hiar_logw("arpp is not available, do not to remove");
		return 0;
	}

	if (pdev == NULL) {
		hiar_loge("platform device is null");
		return -EINVAL;
	}

	core_prv = (struct arpp_core *)platform_get_drvdata(pdev);
	if (core_prv == NULL) {
		hiar_loge("core_prv is NULL!");
		return -EINVAL;
	}

	if (core_prv->pdev == NULL)
		core_prv->pdev = pdev;

	if (core_prv->dev_node != NULL) {
		of_node_put(core_prv->dev_node);
		core_prv->dev_node = NULL;
	}

	wakeup_source_unregister(core_prv->arpp_dev.wake_lock);

	unregister_chr_device(core_prv);

	ret = put_regulator(&core_prv->arpp_dev);
	if (ret != 0)
		hiar_loge("put_regulator failed!");

	ret = put_clk(&core_prv->arpp_dev, &core_prv->pdev->dev);
	if (ret != 0)
		hiar_loge("put_clk failed!");

	put_all_registers(&core_prv->arpp_dev);

	if (core_prv != NULL) {
		kfree(core_prv);
		g_arpp_core = NULL;
	}

	deinit_global_mutex_and_atmoic();

	hiar_logi("-");

	return ret;
}

static const struct of_device_id hisi_arpp_match_table[] = {
	{
		.compatible = DTS_COMP_NAME,
	},
	{},
};

static const struct dev_pm_ops hisi_arpp_dev_pm_ops = {
#ifdef CONFIG_PM_RUNTIME
	.runtime_suspend = NULL,
	.runtime_resume = NULL,
	.runtime_idle = NULL,
#endif
#ifdef CONFIG_PM_SLEEP
	.suspend = NULL,
	.resume = NULL,
#endif
};

static struct platform_driver hisi_arpp_driver = {
	.probe = hisi_arpp_probe,
	.remove = hisi_arpp_remove,
	.driver = {
		.name = DEV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(hisi_arpp_match_table),
		.pm = &hisi_arpp_dev_pm_ops,
	},
};

static int __init hisi_arpp_init(void)
{
	int ret;

	ret = platform_driver_register(&hisi_arpp_driver);
	if (ret != 0) {
		hiar_loge("platform_driver_register failed, error=%d", ret);
		return ret;
	}

	return 0;
}

static void __exit hisi_arpp_exit(void)
{
	platform_driver_unregister(&hisi_arpp_driver);
}

module_init(hisi_arpp_init);
module_exit(hisi_arpp_exit);

MODULE_DESCRIPTION("Hisilicon ARPP Driver");
MODULE_LICENSE("GPL v2");
