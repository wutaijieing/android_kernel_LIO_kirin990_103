/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: ATF BL2 log driver
 * Create : 2019/2/14
 */

#include <linux/io.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/arm-smccc.h>
#include <linux/platform_device.h>
#include <platform_include/see/bl31_smc.h>
#include <securec.h>

/* This strut is align with Bl2 */
struct log_header {
	uintptr_t start;
	unsigned long offset;
	size_t size;
	uintptr_t end;
};

static unsigned long *g_log_addr_virs;
static struct dentry *g_dir;

static int bl2_log_open(struct inode *inode, struct file *pfile)
{
	/* do nothing */
	(void)inode;
	(void)pfile;
	return 0;
}

static ssize_t bl2_log_read(struct file *file, char __user *buf,
			    size_t count, loff_t *offp)
{
	struct log_header *bl2_log = NULL;
	struct arm_smccc_res res = {0};
	char *buffer = NULL;
	size_t buf_size;

	if (!g_log_addr_virs) {
		pr_err("BL2 log addr is NULL.\n");
		return -EFAULT;
	}
	bl2_log = (struct log_header *)g_log_addr_virs;
	arm_smccc_smc(ATF_BL2_LOG_INFO, 0, 0, 0,
		      0, 0, 0, 0, &res);
	if (res.a0 != 0){
		pr_err("BL2: copy log to kernel failed\n");
		return -EINVAL;
	}

	buf_size = res.a2;

	buffer = kzalloc(buf_size, GFP_KERNEL);
	if (!buffer) {
		pr_err("BL2: alloc buffer failed.\n");
		return -ENOMEM;
	}
	if (memcpy_s(buffer, buf_size, g_log_addr_virs, buf_size) != EOK) {
		kfree(buffer);
		pr_err("[%s] memove_s error\n", __func__);
		return -EFAULT;
	}

	buf_size = (size_t)simple_read_from_buffer(buf, count, offp,
					   buffer, buf_size);
	kfree(buffer);
	return buf_size;
}
static const struct file_operations g_bl2_log_fops = {
	.owner   = THIS_MODULE,
	.read    = bl2_log_read,
	.open    = bl2_log_open,
};

static int bl2_init_debugfs(void)
{
	struct dentry *node = NULL;

	g_dir = debugfs_create_dir("bl2", NULL);
	if (!g_dir) {
		pr_err("Failed to create /sys/kernel/debug/bl2\n");
		return -EFAULT;
	}

	node = debugfs_create_file("log", S_IWUSR | S_IWGRP, g_dir, NULL,
				   &g_bl2_log_fops);
	if (!node) {
		pr_err("Failed to create /sys/kernel/debug/bl2/log\n");
		debugfs_remove_recursive(g_dir);
		return -EFAULT;
	}
	return 0;
}

static int atf_bl2_probe(struct platform_device *pdev)
{
	int ret;
	u64 addr;
	u64 size;
	struct device *dev = &pdev->dev;
	struct arm_smccc_res res = {0};

	arm_smccc_smc(ATF_BL2_LOG_INFO, 0, 0, 0,
		      0, 0, 0, 0, &res);
	if (res.a0 != 0){
		pr_err("BL2: copy log to kernel failed\n");
		return -EINVAL;
	}

	addr = res.a1;
	size = res.a2;
	g_log_addr_virs = ioremap_cache(addr, size);
	if (!g_log_addr_virs) {
		dev_err(dev, "Failed to remap bl2 log addr.\n");
		return -EINVAL;
	}

	ret = bl2_init_debugfs();
	if (ret != 0) {
		dev_err(dev, "init debugfs for bl2 failed\n");
		iounmap((void __iomem *)g_log_addr_virs);
		return -EINVAL;
	}
	dev_info(dev, "atf bl2 probe done\n");
	return 0;
}

/* This is called when the module is removed. */
void bl2_cleanup_debugfs(void)
{
	debugfs_remove_recursive(g_dir);
}

static int atf_bl2_remove(struct platform_device *pdev)
{
	bl2_cleanup_debugfs();
	return 0;
}

static const struct of_device_id g_atf_bl2_of_match[] = {
	{.compatible = "atf,atf-bl2"},
	{},
};

static struct platform_driver g_atf_bl2_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "atf-bl2",
		.of_match_table = of_match_ptr(g_atf_bl2_of_match),
	},
	.probe = atf_bl2_probe,
	.remove = atf_bl2_remove,
};

static int __init atf_bl2_driver_init(void)
{
	int ret;

	pr_info("atf bl2 driver init\n");
	ret = platform_driver_register(&g_atf_bl2_driver);
	if (ret)
		pr_err("register bl2 driver fail\n");

	return ret;
}

static void __exit atf_bl2_driver_exit(void)
{
	platform_driver_unregister(&g_atf_bl2_driver);
}

MODULE_LICENSE("GPL");
module_init(atf_bl2_driver_init);
module_exit(atf_bl2_driver_exit);
