/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2020. All rights reserved.
 * Description: Implement of camera buffer.
 * Create: 2018-11-28
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

#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <securec.h>

#include <cam_buf.h>
#include <cam_log.h>
#include <cam_buf_priv.h>

int hisp_sec_ta_enable(void);
int hisp_sec_ta_disable(void);
int hisp_secboot_info_set(unsigned int type, int sharefd);

extern struct iommu_domain *iommu_get_domain_for_dev(struct device *dev);

struct cam_buf_device {
	struct miscdevice dev;
	struct platform_device *pdev;
	atomic_t ref_count;
	void *private;
};

static struct cam_buf_device *g_cam_buf_dev = NULL;
static int g_cam_secmem_type = 0;

void *cam_buf_map_kernel(int fd)
{
	struct device *dev = NULL;

	if (IS_ERR_OR_NULL(g_cam_buf_dev)) {
		cam_err("%s: g_cam_buf_dev is null", __func__);
		return NULL;
	}

	if (fd < 0) {
		cam_err("%s: fd invalid", __func__);
		return NULL;
	}

	dev = &g_cam_buf_dev->pdev->dev;
	return cam_internal_map_kernel(dev, fd);
}

void cam_buf_unmap_kernel(int fd)
{
	struct device *dev = NULL;

	if (IS_ERR_OR_NULL(g_cam_buf_dev)) {
		cam_err("%s: g_cam_buf_dev is null", __func__);
		return;
	}

	if (fd < 0) {
		cam_err("%s: fd invalid", __func__);
		return;
	}

	dev = &g_cam_buf_dev->pdev->dev;
	cam_internal_unmap_kernel(dev, fd);
}

int cam_buf_map_iommu(int fd, struct iommu_format *fmt)
{
	struct device *dev = NULL;

	if (IS_ERR_OR_NULL(g_cam_buf_dev)) {
		cam_err("%s: check g_cam_buf_dev is error or null", __func__);
		return -ENODEV;
	}

	if (fd < 0 || !fmt) {
		cam_err("%s: fd or fmt invalid", __func__);
		return -EINVAL;
	}

	dev = &g_cam_buf_dev->pdev->dev;
	return cam_internal_map_iommu(dev, fd, fmt);
}

void cam_buf_unmap_iommu(int fd, struct iommu_format *fmt)
{
	struct device *dev = NULL;

	if (IS_ERR_OR_NULL(g_cam_buf_dev)) {
		cam_err("%s: g_cam_buf_dev is null", __func__);
		return;
	}

	if (fd < 0 || !fmt) {
		cam_err("%s: fd or fmt invalid", __func__);
		return;
	}

	dev = &g_cam_buf_dev->pdev->dev;
	cam_internal_unmap_iommu(dev, fd, fmt);
}

void cam_buf_sc_available_size(struct systemcache_format *fmt)
{
	if (!fmt) {
		cam_err("%s: fmt is null", __func__);
		return;
	}
	cam_internal_sc_available_size(fmt);
}

int cam_buf_sc_wakeup(struct systemcache_format *fmt)
{
	if (!fmt) {
		cam_err("%s: fmt is null", __func__);
		return -ENODEV;
	}

	if (fmt->pid <= 0) {
		cam_err("%s: fmt pid invalid", __func__);
		return -ENODEV;
	}

	if (fmt->prio > 1) {
		cam_err("%s: fmt prio invalid", __func__);
		return -ENODEV;
	}
	return cam_internal_sc_wakeup(fmt);
}

int cam_buf_get_phys(int fd, struct phys_format *fmt)
{
	struct device *dev = NULL;

	if (IS_ERR_OR_NULL(g_cam_buf_dev)) {
		cam_err("%s: g_cam_buf_dev is null", __func__);
		return -ENODEV;
	}

	if (fd < 0 || !fmt) {
		cam_err("%s: fd or fmt invalid", __func__);
		return -EINVAL;
	}

	dev = &g_cam_buf_dev->pdev->dev;
	return cam_internal_get_phys(dev, fd, fmt);
}

/* return 0 if pgd_base get failed, caller check please */
phys_addr_t cam_buf_get_pgd_base(void)
{
	struct device *dev = NULL;

	if (IS_ERR_OR_NULL(g_cam_buf_dev)) {
		cam_err("%s: g_cam_buf_dev is null", __func__);
		return 0;
	}

	dev = &g_cam_buf_dev->pdev->dev;
	return cam_internal_get_pgd_base(dev);
}

int cam_buf_sync(int fd, struct sync_format *fmt)
{
	if (fd < 0 || !fmt) {
		cam_err("%s: fd or fmt invalid", __func__);
		return -EINVAL;
	}

	return cam_internal_sync(fd, fmt);
}

int cam_buf_local_sync(int fd, struct local_sync_format *fmt)
{
	if (fd < 0 || !fmt) {
		cam_err("%s: fd or fmt invalid", __func__);
		return -EINVAL;
	}

	return cam_internal_local_sync(fd, fmt);
}

/* CARE: get sg_table will hold the related buffer,
 * please save ret ptr, and call put with it.
 */
struct sg_table *cam_buf_get_sgtable(int fd)
{
	struct device *dev = NULL;

	if (IS_ERR_OR_NULL(g_cam_buf_dev)) {
		cam_err("%s: g_cam_buf_dev is null", __func__);
		return ERR_PTR(-ENODEV);
	}

	if (fd < 0) {
		cam_err("%s: fd invalid", __func__);
		return ERR_PTR(-EINVAL);
	}

	dev = &g_cam_buf_dev->pdev->dev;
	return cam_internal_get_sgtable(dev, fd);
}

void cam_buf_put_sgtable(struct sg_table *sgt)
{
	struct device *dev = NULL;

	if (IS_ERR_OR_NULL(g_cam_buf_dev)) {
		cam_err("%s: g_cam_buf_dev is invalid", __func__);
		return;
	}

	if (IS_ERR_OR_NULL(sgt)) {
		cam_err("%s: sgt is invalid", __func__);
		return;
	}
	dev = &g_cam_buf_dev->pdev->dev;
	cam_internal_put_sgtable(dev, sgt);
}

int cam_buf_open_security_ta(void)
{
	if (g_cam_secmem_type != 1) {
		cam_err("%s: sec_mem_type = %d", __func__,
				g_cam_secmem_type);
		return -EINVAL;
	}

	return hisp_sec_ta_enable();
}

int cam_buf_close_security_ta(void)
{
	if (g_cam_secmem_type != 1) {
		cam_err("%s: sec_mem_type = %d", __func__,
				g_cam_secmem_type);
		return -EINVAL;
	}

	return hisp_sec_ta_disable();
}

int cam_buf_set_secboot(struct cam_buf_cfg *cfg)
{
	if (g_cam_secmem_type != 1) {
		cam_err("%s: sec_mem_type = %d", __func__,
				g_cam_secmem_type);
		return -EINVAL;
	}

	return hisp_secboot_info_set(cfg->sec_mem_format.type, cfg->fd);
}

int cam_buf_release_secboot(struct cam_buf_cfg *cfg)
{
	(void)cfg;
	if (g_cam_secmem_type != 1) {
		cam_err("%s: sec_mem_type = %d", __func__,
				g_cam_secmem_type);
		return -EINVAL;
	}

	return 0;
}

static void cam_buf_get_secmem_type(struct cam_buf_cfg *cfg)
{
	cfg->secmemtype = (unsigned int)g_cam_secmem_type;
}

int get_secmem_type(void)
{
	return g_cam_secmem_type;
}

struct iommu_domain *cam_buf_get_domain_for_dev(void)
{
	struct device *dev = NULL;

	dev = &g_cam_buf_dev->pdev->dev;
	return iommu_get_domain_for_dev(dev);
}
EXPORT_SYMBOL(cam_buf_get_domain_for_dev);

static long cam_config(struct cam_buf_cfg *cfg)
{
	long ret = 0;

	switch (cfg->type) {
	case CAM_BUF_MAP_IOMMU:
		ret = cam_buf_map_iommu(cfg->fd, &cfg->iommu_format);
		break;
	case CAM_BUF_UNMAP_IOMMU:
		cam_buf_unmap_iommu(cfg->fd, &cfg->iommu_format);
		break;
	case CAM_BUF_SYNC:
		ret = cam_buf_sync(cfg->fd, &cfg->sync_format);
		break;
	case CAM_BUF_LOCAL_SYNC:
		ret = cam_buf_local_sync(cfg->fd, &cfg->local_sync_format);
		break;
	case CAM_BUF_GET_PHYS:
#if (defined(HISP300_CAMERA) || defined(HISP350_CAMERA)) && \
	!defined(DEBUG_KERNEL_CAMERA)
		cam_info("%s: should not get phy addr", __func__);
		ret = -EINVAL;
#else
		ret = cam_buf_get_phys(cfg->fd, &cfg->phys_format);
#endif
		break;
	case CAM_BUF_OPEN_SECURITY_TA:
		ret = cam_buf_open_security_ta();
		break;
	case CAM_BUF_CLOSE_SECURITY_TA:
		ret = cam_buf_close_security_ta();
		break;
	case CAM_BUF_SET_SECBOOT:
		ret = cam_buf_set_secboot(cfg);
		break;
	case CAM_BUF_RELEASE_SECBOOT:
		ret = cam_buf_release_secboot(cfg);
		break;
	case CAM_BUF_GET_SECMEMTYPE:
		cam_buf_get_secmem_type(cfg);
		break;
	case CAM_BUF_SC_AVAILABLE_SIZE:
		cam_buf_sc_available_size(&cfg->systemcache_format);
		break;
	case CAM_BUF_SC_WAKEUP:
		cam_buf_sc_wakeup(&cfg->systemcache_format);
		break;
	default:
		cam_info("%s: invalid command type %d",
				 __func__, cfg->type);
		break;
	}

	return ret;
}

static long cam_buf_ioctl(struct file *filp, unsigned int cmd,
						  unsigned long arg)
{
	long ret = 0;
	struct cam_buf_cfg data;

	(void)filp;
	if (_IOC_SIZE(cmd) > sizeof(data)) {
		cam_err("%s: cmd size too large", __func__);
		return -EINVAL;
	}
	if (arg == 0) {
		cam_err("%s, arg for buf ioctl is NULL\n", __func__);
		return -EINVAL;
	}

	if (copy_from_user(&data, (void __user *)arg, _IOC_SIZE(cmd))) {
		cam_err("%s: copy in arg failed", __func__);
		return -EFAULT;
	}

	switch (cmd) {
	case CAM_BUF_IOC_CFG:
		ret = cam_config(&data);
		break;
	default:
		cam_info("%s: invalid command %d", __func__, cmd);
		return -EINVAL;
	}

	if (copy_to_user((void __user *)arg, &data, _IOC_SIZE(cmd))) {
		cam_err("%s: copy back arg failed", __func__);
		return -EFAULT;
	}

	return ret;
}

static int cam_buf_release(struct inode *inode, struct file *file)
{
	int ref_count;
	struct cam_buf_device *idev = NULL;
	struct miscdevice *mdev = NULL;

	(void)inode;
	if (!file) {
		cam_info("%s: file is NULL", __func__);
		return 0;
	}

	idev = file->private_data;
	if (idev) {
		ref_count = atomic_dec_return(&idev->ref_count);
		mdev = &idev->dev;
		cam_info("%s: %s device closed, ref:%d",
				 __func__, mdev->name, ref_count);
	}

	return 0;
}

static int cam_buf_open(struct inode *inode, struct file *file)
{
	int ref_count;
	struct miscdevice *mdev = NULL;
	struct cam_buf_device *idev = NULL;

	(void)inode;
	if (!file) {
		cam_info("%s: file is NULL", __func__);
		return -EFAULT;
	}

	mdev = file->private_data;
	if (mdev) {
		idev = container_of(mdev, struct cam_buf_device, dev);
		file->private_data = idev;

		ref_count = atomic_inc_return(&idev->ref_count);
		cam_info("%s: %s device opened, ref:%d", __func__,
				 mdev->name, ref_count);
	}
	return 0;
}

static const struct file_operations g_cam_buf_fops = {
													  .owner = THIS_MODULE,
													  .open = cam_buf_open,
													  .release = cam_buf_release,
													  .unlocked_ioctl = cam_buf_ioctl,
#if CONFIG_COMPAT
													  .compat_ioctl = cam_buf_ioctl,
#endif
};

static int cam_buf_parse_of_node(struct cam_buf_device *idev)
{
	struct device_node *np = idev->pdev->dev.of_node;

	if (!np)
		return -ENODEV;

	return 0;
}

struct cam_buf_device *cam_buf_device_create(struct platform_device *pdev)
{
	int ret;
	struct cam_buf_device *idev;

	idev = kzalloc(sizeof(*idev), GFP_KERNEL);
	if (!idev)
		return ERR_PTR(-ENOMEM);

	atomic_set(&idev->ref_count, 0);
	idev->pdev = pdev;

	ret = cam_buf_parse_of_node(idev);
	if (ret != 0) {
		cam_err("%s: failed to parse device of_node", __func__);
		goto err_out;
	}

	idev->dev.minor = MISC_DYNAMIC_MINOR;
	idev->dev.name = "cam_buf"; /* dev name under /dev */
	idev->dev.fops = &g_cam_buf_fops;
	idev->dev.parent = NULL;
	ret = misc_register(&idev->dev);
	if (ret != 0) {
		cam_err("%s: failed to register misc device", __func__);
		goto err_out;
	}

	return idev;
err_out:
	kfree(idev);
	return ERR_PTR(ret);
}

void cam_buf_device_destroy(struct cam_buf_device *idev)
{
	misc_deregister(&idev->dev);
	kfree(idev);
}

#ifdef CONFIG_DFX_DEBUG_FS
ssize_t cam_buf_info_show(struct device *dev,
						  struct device_attribute *attr, char *buf)
{
	(void)attr;
	cam_internal_dump_debug_info(dev);
	return snprintf_s(buf, PAGE_SIZE, PAGE_SIZE, "%s", "info dumpped to kmsg...");
}
DEVICE_ATTR_RO(cam_buf_info);
#endif /* CONFIG_DFX_DEBUG_FS */

static int cam_buf_get_dts(struct platform_device *p_dev)
{
	int ret;
	struct device *pdev = NULL;
	struct device_node *np = NULL;
	int tmp = 0;

	if (!p_dev) {
		cam_err("%s: pDev NULL", __func__);
		return -1;
	}

	pdev = &(p_dev->dev);

	np = pdev->of_node;

	if (!np) {
		cam_err("%s: of node NULL", __func__);
		return -1;
	}

	/* get g_cam_secmem_type */
	ret = of_property_read_u32(np, "secmemtype", &tmp);
	if (ret == 0)
		g_cam_secmem_type = tmp;

	return 0;
}

static int32_t cam_buf_probe(struct platform_device *pdev)
{
	int rc;
	int ret = 0;

	if (!pdev) {
		cam_err("%s: null pdev", __func__);
		return -ENODEV;
	}

	/* 64:size */
	ret = dma_set_mask_and_coherent(&pdev->dev,
									DMA_BIT_MASK(64)); /*lint !e598 !e648*/
	if (ret < 0)
		cam_err("dma set mask and coherent failed\n");
	g_cam_buf_dev = cam_buf_device_create(pdev);
	if (IS_ERR(g_cam_buf_dev)) {
		cam_err("%s: fail to create cam ion device", __func__);
		rc = PTR_ERR(g_cam_buf_dev);
		goto err_create_device;
	}

	rc = cam_buf_get_dts(pdev);
	if (rc < 0) {
		cam_err("[%s] Failed: cam_buf_get_dts.%d", __func__, rc);
		goto err_init_internal;
	}

	rc = cam_internal_init(&g_cam_buf_dev->pdev->dev);
	if (rc != 0) {
		cam_err("%s: fail to init internal data", __func__);
		goto err_init_internal;
	}

#ifdef CONFIG_DFX_DEBUG_FS
	rc = device_create_file(&pdev->dev, &dev_attr_cam_buf_info);
	if (rc < 0)
		/* just log it, it's not fatal. */
		cam_err("%s: fail to create cam_buf_info file",
				__func__);
#endif /* CONFIG_DFX_DEBUG_FS */
	return 0;
err_init_internal:
	cam_buf_device_destroy(g_cam_buf_dev);
err_create_device:
	g_cam_buf_dev = NULL;
	return rc;
}

static int32_t cam_buf_remove(struct platform_device *pdev)
{
	if (!g_cam_buf_dev) {
		cam_err("%s: g_cam_buf_dev is not inited", __func__);
		return -EINVAL;
	}
	cam_internal_deinit(&g_cam_buf_dev->pdev->dev);
	cam_buf_device_destroy(g_cam_buf_dev);
#ifdef CONFIG_DFX_DEBUG_FS
	device_remove_file(&pdev->dev, &dev_attr_cam_buf_info);
#endif /* CONFIG_DFX_DEBUG_FS */
	return 0;
}

static const struct of_device_id g_cam_buf_dt_match[] = {
														 {
														  .compatible = "vendor,cam_buf", },
														 {},
};
MODULE_DEVICE_TABLE(of, g_cam_buf_dt_match);

static struct platform_driver g_cam_buf_platform_driver = {
														   .driver = {
																	  .name = "vendor,cam_buf",
																	  .owner = THIS_MODULE,
																	  .of_match_table = g_cam_buf_dt_match,
	},

														   .probe = cam_buf_probe,
														   .remove = cam_buf_remove,
};

static int __init cam_buf_init_module(void)
{
	cam_info("enter %s", __func__);
	return platform_driver_register(&g_cam_buf_platform_driver);
}

static void __exit cam_buf_exit_module(void)
{
	platform_driver_unregister(&g_cam_buf_platform_driver);
}

#ifdef CONFIG_ARM_SMMU_V3
rootfs_initcall(cam_buf_init_module);
#else
subsys_initcall_sync(cam_buf_init_module);
#endif

module_exit(cam_buf_exit_module);

MODULE_DESCRIPTION("cam_buf");
MODULE_LICENSE("GPL v2");
