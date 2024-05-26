/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: Implement of camera buffer v3.
 * Create: 2019-07-31
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <cam_buf.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>

#include <cam_log.h>
#include <cam_buf_v3_priv.h>
#include <securec.h>

struct cam_buf_v3_device {
	struct miscdevice dev;
	struct platform_device *pdev;
	atomic_t ref_count;
	enum cam_buf_dev_type devtype;
	int padding_support;
	char filename[CAM_BUF_FILE_MAX_LEN];
	void *private;
};

int cam_buf_v3_map_iommu(struct device *dev, int fd, struct iommu_format *fmt,
						 int padding_support)
{
	if (IS_ERR_OR_NULL(dev)) {
		cam_err("%s: cam_buf_dev is null", __func__);
		return -EINVAL;
	}

	if (fd < 0 || IS_ERR_OR_NULL(fmt)) {
		cam_err("%s: fd or fmt invalid", __func__);
		return -EINVAL;
	}

	return cam_v3_internal_map_iommu(dev, fd, fmt, padding_support);
}

void cam_buf_v3_unmap_iommu(struct device *dev, int fd,
							struct iommu_format *fmt, int padding_support)
{
	if (IS_ERR_OR_NULL(dev)) {
		cam_err("%s: cam_buf_dev is null", __func__);
		return;
	}

	if (fd < 0 || IS_ERR_OR_NULL(fmt)) {
		cam_err("%s: fd or fmt invalid", __func__);
		return;
	}

	cam_v3_internal_unmap_iommu(dev, fd, fmt, padding_support);
}

static long cam_config_v3(struct cam_buf_v3_device *cam_buf_v3_dev,
						  struct cam_buf_cfg *cfg)
{
	long ret = 0;

	if (IS_ERR_OR_NULL(cam_buf_v3_dev)) {
		cam_err("%s: cam_buf_dev is null", __func__);
		return -ENODEV;
	}

	switch (cfg->type) {
	case CAM_BUF_MAP_IOMMU:
		ret = cam_buf_v3_map_iommu(&cam_buf_v3_dev->pdev->dev,
								   cfg->fd, &cfg->iommu_format,
								   cam_buf_v3_dev->padding_support);
		break;
	case CAM_BUF_UNMAP_IOMMU:
		cam_buf_v3_unmap_iommu(&cam_buf_v3_dev->pdev->dev, cfg->fd,
							   &cfg->iommu_format, cam_buf_v3_dev->padding_support);
		break;
	default:
		cam_err("%s: can't handle type[%d]", __func__, cfg->type);
	}

	return ret;
}

static long cam_buf_v3_ioctl(struct file *filp, unsigned int cmd,
							 unsigned long arg)
{
	long ret = 0;
	struct cam_buf_cfg data;
	struct miscdevice *mdev = NULL;
	struct cam_buf_v3_device *idev = NULL;

	if (!filp) {
		cam_err("%s: filp is NULL", __func__);
		return -EFAULT;
	}

	if (_IOC_SIZE(cmd) > sizeof(data)) {
		cam_err("%s: cmd size too large", __func__);
		return -EINVAL;
	}

	if (arg == 0) {
		cam_err("%s, arg for buf v3 ioctl is NULL\n", __func__);
		return -EINVAL;
	}

	if (copy_from_user(&data, (void __user *)arg, _IOC_SIZE(cmd)) != 0) {
		cam_err("%s: copy in arg failed", __func__);
		return -EFAULT;
	}

	mdev = filp->private_data;
	idev = container_of(mdev, struct cam_buf_v3_device, dev);

	switch (cmd) {
	case CAM_BUF_IOC_CFG:
		ret = cam_config_v3(idev, &data);
		break;
	default:
		cam_info("%s: invalid command %d", __func__, cmd);
		return -EINVAL;
	}

	if (copy_to_user((void __user *)arg, &data, _IOC_SIZE(cmd)) != 0) {
		cam_err("%s: copy back arg failed", __func__);
		return -EFAULT;
	}

	return ret;
}

static int cam_buf_v3_release(struct inode *inode, struct file *file)
{
	(void)inode;
	int ref_count;
	struct cam_buf_v3_device *idev = NULL;
	struct miscdevice *mdev = NULL;

	if (!file)
		return 0;
	idev = file->private_data;
	if (!idev)
		return 0;
	mdev = &idev->dev;
	ref_count = atomic_dec_return(&idev->ref_count);
	cam_info("%s: %s device closed, ref:%d",
			 __func__, mdev->name, ref_count);
	return 0;
}

static int cam_buf_v3_open(struct inode *inode, struct file *file)
{
	(void)inode;
	int ref_count;
	struct miscdevice *mdev = NULL;
	struct cam_buf_v3_device *idev = NULL;

	if (!file)
		return -EFAULT;

	mdev = file->private_data;
	idev = container_of(mdev, struct cam_buf_v3_device, dev);
	if (!idev)
		return -EFAULT;

	ref_count = atomic_inc_return(&idev->ref_count);
	cam_info("%s: %s device opened, ref:%d",
			 __func__, mdev->name, ref_count);
	return 0;
}

static const struct file_operations g_cam_buf_v3_fops = {
														 .owner = THIS_MODULE,
														 .open = cam_buf_v3_open,
														 .release = cam_buf_v3_release,
														 .unlocked_ioctl = cam_buf_v3_ioctl,
#if CONFIG_COMPAT
														 .compat_ioctl = cam_buf_v3_ioctl,
#endif
};

static int cam_buf_v3_parse_of_node(struct cam_buf_v3_device *idev)
{
	int ret;
	struct device_node *np = idev->pdev->dev.of_node;

	if (!np)
		return -ENODEV;

	ret = of_property_read_u32(np, "vendor,devtype", &(idev->devtype));
	if (ret < 0)
		cam_err("%s: get devtype failed", __func__);

	idev->padding_support = 0; /* default not support iova padding mode */
	ret = of_property_read_u32(np, "vendor,support-iova-padding",
							   &(idev->padding_support));
	if (ret == 0)
		cam_info("%s: get hwei,support-iova-padding %d", __func__,
				 idev->padding_support);

	return 0;
}

struct cam_buf_v3_device *cam_buf_v3_device_create(
												   struct platform_device *pdev)
{
	int ret;
	struct cam_buf_v3_device *idev = NULL;

	idev = kzalloc(sizeof(*idev), GFP_KERNEL);
	if (!idev)
		return ERR_PTR(-ENOMEM);

	atomic_set(&idev->ref_count, 0);
	idev->pdev = pdev;

	ret = cam_buf_v3_parse_of_node(idev);
	if (ret != 0) {
		cam_err("%s: failed to parse device of_node", __func__);
		goto err_out;
	}
	ret = sprintf_s(idev->filename, CAM_BUF_FILE_MAX_LEN, "cam_buf%d",
					idev->devtype);
	if (ret < 0) {
		cam_err("%s: failed to sprintf_s filename, devtype %d",
				__func__, idev->devtype);
		goto err_out;
	}

	idev->dev.minor = MISC_DYNAMIC_MINOR;
	idev->dev.name = idev->filename; /* dev name under /dev */
	idev->dev.fops = &g_cam_buf_v3_fops;
	idev->dev.parent = NULL;
	ret = misc_register(&idev->dev);
	if (ret != 0) {
		cam_err("%s: failed to register misc device", __func__);
		goto err_out;
	}
	cam_info("%s: create %s success", __func__, idev->dev.name);

	return idev;
err_out:
	kfree(idev);
	return ERR_PTR(ret);
}

void cam_buf_v3_device_destroy(struct cam_buf_v3_device *idev)
{
	misc_deregister(&idev->dev);
	kfree(idev);
}

static int32_t cam_buf_v3_probe(struct platform_device *pdev)
{
	int rc;
	int ret = 0;
	struct cam_buf_v3_device *cam_buf_v3_dev = NULL;

	if (IS_ERR_OR_NULL(pdev)) {
		cam_err("%s: null pdev", __func__);
		return -ENODEV;
	}

	/* 64:size */
	ret = dma_set_mask_and_coherent(&pdev->dev,
									DMA_BIT_MASK(64)); /*lint !e598 !e648*/
	if (ret < 0)
		cam_err("dma set mask and coherent failed\n");
	cam_buf_v3_dev = cam_buf_v3_device_create(pdev);
	if (IS_ERR(cam_buf_v3_dev)) {
		cam_err("%s: fail to create cam ion device", __func__);
		rc = PTR_ERR(cam_buf_v3_dev);
		goto err_create_device;
	}

	platform_set_drvdata(pdev, cam_buf_v3_dev);
	return 0;

err_create_device:
	return rc;
}

static int32_t cam_buf_v3_remove(struct platform_device *pdev)
{
	struct cam_buf_v3_device *cam_buf_v3_dev =
		platform_get_drvdata(pdev);

	if (IS_ERR_OR_NULL(cam_buf_v3_dev)) {
		cam_err("%s: cam_buf_dev is not inited", __func__);
		return -EINVAL;
	}

	cam_buf_v3_device_destroy(cam_buf_v3_dev);

	return 0;
}

static const struct of_device_id g_cam_buf_v3_dt_match[] = {
															{ .compatible = "vendor,cam_buf_v3", },
															{},
};
MODULE_DEVICE_TABLE(of, g_cam_buf_v3_dt_match);

static struct platform_driver g_cam_buf_v3_platform_driver = {
															  .driver = {
															  .name = "vendor,cam_buf_v3",
															  .owner = THIS_MODULE,
															  .of_match_table = g_cam_buf_v3_dt_match,
	},

															  .probe = cam_buf_v3_probe,
															  .remove = cam_buf_v3_remove,
};

static int __init cam_buf_v3_init_module(void)
{
	cam_info("enter %s", __func__);
	return platform_driver_register(&g_cam_buf_v3_platform_driver);
}

static void __exit cam_buf_v3_exit_module(void)
{
	platform_driver_unregister(&g_cam_buf_v3_platform_driver);
}

rootfs_initcall(cam_buf_v3_init_module);
module_exit(cam_buf_v3_exit_module);

MODULE_DESCRIPTION("cam_buf_v3");
MODULE_LICENSE("GPL v2");
