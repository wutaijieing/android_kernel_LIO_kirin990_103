/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2014-2020. All rights reserved.
 * Description: bl31_log for atf
 * Create: 2014/5/16
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/dma-mapping.h>
#include <linux/printk.h>
#include <linux/gfp.h>
#include <linux/version.h>
#include <linux/arm-smccc.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of_reserved_mem.h>
#include <linux/of.h>
#include <linux/mutex.h>
#include <platform_include/see/bl31_smc.h>
#include <securec.h>

static struct platform_device *g_bl31_log_pdev;

static struct cdev g_bl31_log_cdev;
static struct device *g_bl31_log_device;
static dev_t g_bl31_log_devno;
static struct class *g_bl31_log_class;

static void *g_bl31_log_addr_virts;
static dma_addr_t g_bl31_log_addr_phys;
/*
 * copy 4K data from BL31_LOG_BASE everytimes
 * made g_bl31_log_size as 4k(0x1000)
 */
static size_t g_bl31_log_size = 0x1000;

static DEFINE_MUTEX(g_bl31_log_lock);

noinline int bl31_log_smc(u64 _function_id, u64 _arg0, u64 _arg1, u64 _arg2)
{
	struct arm_smccc_res res = {0};

	arm_smccc_smc(_function_id, _arg0, _arg1, _arg2,
				0, 0, 0, 0, &res);
	return (int)res.a0;
};

int bl31_logbuf_read(char *read_buf, u64 buf_size)
{
	u64 total;
	u64 left;
	u64 copy;
	u64 pos;
	s32 ret = 0;
	u64 log_size;
	struct arm_smccc_res res = {0};

	if (!g_bl31_log_pdev) {
		pr_err("[%s] g_bl31_log_pdev not init.\n", __func__);
		return -EFAULT;
	}

	if (!read_buf || !buf_size) {
		pr_err("[%s] invalid argument.\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&g_bl31_log_lock);
	g_bl31_log_addr_virts =
	    dma_alloc_coherent(&g_bl31_log_pdev->dev, g_bl31_log_size,
			       &g_bl31_log_addr_phys, GFP_KERNEL);
	if (!g_bl31_log_addr_virts) {
		pr_err("[%s] dma_alloc_coherent fail.\n", __func__);
		ret = -EFAULT;
		goto alloc_err;
	}

	arm_smccc_smc(ATF_BL31_LOG_INFO, 0, 0, 0,
		      0, 0, 0, 0, &res);
	if (res.a0 != 0){
		pr_err("[%s] copy log to kernel failed\n", __func__);
		ret = -EINVAL;
		goto read_err;
	}

	log_size = res.a2;
	total = (u64)min(buf_size, log_size);

	for (left = total, pos = 0; pos < total;
	     left -= g_bl31_log_size, pos += g_bl31_log_size) {
		/* The min_t function can not be apply at here */
		copy = (u64)min((u64)g_bl31_log_size, left);
		ret = bl31_log_smc(ATF_LOG_FID_VALUE,
				   g_bl31_log_addr_phys, copy, pos);
		if (ret) {
			pr_err("[%s] bl31_log_smc error.\n", __func__);
			goto read_err;
		}
		if (memcpy_s((void *)(read_buf + pos), copy, g_bl31_log_addr_virts, copy) != EOK) {
			pr_err("[%s] memcpy_s error\n", __func__);
			ret =  -EINVAL;
			goto read_err;
		}
	}

read_err:
	dma_free_coherent(&g_bl31_log_pdev->dev, g_bl31_log_size,
			  g_bl31_log_addr_virts, g_bl31_log_addr_phys);
	g_bl31_log_addr_virts = NULL;
alloc_err:
	mutex_unlock(&g_bl31_log_lock);
	return ret;
}

static int bl31_log_file_open(struct inode *inode, struct file *fid)
{
	mutex_lock(&g_bl31_log_lock);
	g_bl31_log_addr_virts =
		dma_alloc_coherent(&g_bl31_log_pdev->dev, g_bl31_log_size,
				   &g_bl31_log_addr_phys, GFP_KERNEL);
	if (!g_bl31_log_addr_virts) {
		pr_err("[%s] dma_alloc_coherent fail.\n", __func__);
		mutex_unlock(&g_bl31_log_lock);
		return -EFAULT;
	}

	pr_err("[%s] open end.\n", __func__);
	return 0; /*lint !e454*/
}

static ssize_t bl31_log_file_read(struct file *fid, char __user *buf,
				  size_t size, loff_t *pos)
{
	size_t copy;
	u64 log_size;
	struct arm_smccc_res res = {0};

	/* to solve a coverity warning */
	if (!buf) {
		pr_err("[%s] buf pointer is null.\n", __func__);
		goto read_err;
	}

	arm_smccc_smc(ATF_BL31_LOG_INFO, 0, 0, 0,
		      0, 0, 0, 0, &res);
	if (res.a0 != 0){
		pr_err("BL31: copy log to kernel failed\n");
		return -EINVAL;
	}

	log_size = res.a2;
	if ((u64)(uintptr_t)*pos >= log_size) {
		dma_free_coherent(&g_bl31_log_pdev->dev, g_bl31_log_size,
				  g_bl31_log_addr_virts, g_bl31_log_addr_phys);
		g_bl31_log_addr_virts = NULL;
		pr_err("[%s] read end.\n", __func__);
		mutex_unlock(&g_bl31_log_lock); /*lint !e455*/
		return 0;
	}

	copy = (ssize_t)min3(size, (size_t)(log_size - *pos), g_bl31_log_size);

	if (bl31_log_smc(ATF_LOG_FID_VALUE,
			 g_bl31_log_addr_phys, copy, *pos)) {
		pr_err("[%s] bl31_log_smc error.\n", __func__);
		goto read_err;
	}

	if (copy_to_user(buf, g_bl31_log_addr_virts, copy)) {
		pr_err("[%s] copy_to_user error.\n", __func__);
		goto read_err;
	}

	*pos += copy;

	return copy;

read_err:
	dma_free_coherent(&g_bl31_log_pdev->dev, g_bl31_log_size,
			  g_bl31_log_addr_virts, g_bl31_log_addr_phys);
	g_bl31_log_addr_virts = NULL;
	mutex_unlock(&g_bl31_log_lock); /*lint !e455*/
	return -EFAULT;
}

static const struct file_operations g_bl31_log_file_ops = {
	.open = bl31_log_file_open,
	.read = bl31_log_file_read,
};

static int bl31_log_add_device(void)
{
	int ret;

	cdev_init(&g_bl31_log_cdev, &g_bl31_log_file_ops);

	ret = alloc_chrdev_region(&g_bl31_log_devno, 0, 1, "bl31_log");
	if (ret) {
		pr_err("[%s] alloc_chrdev_region fail.\n", __func__);
		return -1;
	}

	ret = cdev_add(&g_bl31_log_cdev, g_bl31_log_devno, 1);
	if (ret) {
		pr_err("[%s] cdev_add fail.\n", __func__);
		goto unregister_devno;
	}

	g_bl31_log_class = class_create(THIS_MODULE, "bl31_log");
	if (IS_ERR(g_bl31_log_class)) {
		pr_err("[%s] class_create fail.\n", __func__);
		goto unregister_cdev;
	}

	g_bl31_log_device =
		device_create(g_bl31_log_class, NULL, g_bl31_log_devno,
			      NULL, "bl31_log");
	if (IS_ERR(g_bl31_log_device)) {
		pr_err("[%s] device_create fail.\n", __func__);
		goto unregister_class;
	}

	return 0;

unregister_class:
	class_destroy(g_bl31_log_class);

unregister_cdev:
	cdev_del(&g_bl31_log_cdev);

unregister_devno:
	unregister_chrdev_region(g_bl31_log_devno, 1);

	return -1;
}

static void bl31_log_del_device(void)
{
	device_destroy(g_bl31_log_class, g_bl31_log_devno);
	class_destroy(g_bl31_log_class);
	cdev_del(&g_bl31_log_cdev);
	unregister_chrdev_region(g_bl31_log_devno, 1);
}

static int bl31_log_probe(struct platform_device *pdev)
{
	int ret;

	if (!pdev) {
		pr_err("[%s] pdev is NULL, error.\n", __func__);
		return -EINVAL;
	}

	ret = of_reserved_mem_device_init(&pdev->dev);
	if (ret) {
		pr_err("[%s] of_reserved_mem_device_init fail.\n", __func__);
		return -EINVAL;
	}

	ret = dma_coerce_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));
	if (ret) {
		pr_err("[%s] dma_coerce_mask_and_coherent fail.\n", __func__);
		return -EINVAL;
	}

	g_bl31_log_pdev = pdev;

	ret = bl31_log_add_device();
	if (ret) {
		pr_err("[%s] bl31_log_add_device fail.\n", __func__);
		return -EINVAL;
	}

	pr_err("[%s] probe end.\n", __func__);
	return 0;
}

static int bl31_log_remove(struct platform_device *pdev)
{
	g_bl31_log_pdev = NULL;
	bl31_log_del_device();
	pr_err("[%s] remove end.\n", __func__);
	return 0;
}

static const struct of_device_id bl31_log_of_match[] = {
	{.compatible = "atf,bl31_log"},
	{},
};

static struct platform_driver g_bl31_log_pdrv = {
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "bl31-log",
		   .of_match_table = of_match_ptr(bl31_log_of_match),
	},
	.probe = bl31_log_probe,
	.remove = bl31_log_remove,
};

static int __init bl31_log_init(void)
{
	return platform_driver_register(&g_bl31_log_pdrv);
}

static void __exit bl31_log_exit(void)
{
	platform_driver_unregister(&g_bl31_log_pdrv);
}

MODULE_LICENSE("GPL");
module_init(bl31_log_init);
module_exit(bl31_log_exit);
