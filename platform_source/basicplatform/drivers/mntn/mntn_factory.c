/*
 *
 * License terms: GNU General Public License (GPL) version 2
 *
 * functions:
 * 1. record test points in reserved mem, so make it easy to get
 *    the testcase when system reset abnormally.
 *
 */
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/printk.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <mntn_factory.h>
#include <platform_include/basicplatform/linux/rdr_platform.h>
#include <securec.h>

struct mntn_factory_stru *g_mntn_fac_info;

int mntn_factory_dump(void *dump_addr, unsigned int size)
{
	return 0;
}

static ssize_t mntn_factory_testpoint_store(struct file *file, const char __user *userbuf,
		size_t bytes, loff_t *off)
{
	char *buf;
	ssize_t byte_writen;
	struct mntn_factory_stru *mntn_fac_info = g_mntn_fac_info;
	int ret;

	if (bytes >= MNTN_FACTORY_TESTPOINT_RECORD_SIZE || file == NULL || userbuf == NULL || off == NULL) {
		pr_err("[%s]input size(%ld) is too long\n",
			__func__, bytes);
		return -ENOMEM;
	}

	if (!mntn_fac_info) {
		pr_err("[%s]mntn factory do not init\n", __func__);
		return -EPERM;
	}

	buf = mntn_fac_info->mntn_mem_addr + MNTN_FACTORY_TESTPOINT_RECORD_OFFSET;
	ret = memset_s(buf, MNTN_FACTORY_TESTPOINT_RECORD_SIZE,
			0, MNTN_FACTORY_TESTPOINT_RECORD_SIZE);
	if (ret) {
		pr_err("[%s]dumpmem clean error\n", __func__);
		return -EPERM;
	}
	byte_writen = simple_write_to_buffer(buf,
			MNTN_FACTORY_TESTPOINT_RECORD_SIZE,
			off, userbuf, bytes);
	if (byte_writen <= 0) {
		pr_err("[%s]simple_write_to_buffer error\n", __func__);
		return byte_writen;
	}

	pr_info("[%s]enter-test %s", __func__, buf);
	return byte_writen;
}

static ssize_t mntn_factory_testpoint_show(struct file *file, char __user *buf,
		size_t count, loff_t *pos)
{
	return 0;
}

static const struct file_operations mntn_fac_fops = {
	.write = mntn_factory_testpoint_store,
	.read = mntn_factory_testpoint_show,
};

static int __init mntn_factory_init(void)
{
	struct dentry *root;
	struct dentry *d;
	struct mntn_factory_stru *mntn_fac_info;
	int ret = 0;
	const char head[] = "mntn-factory-dump";
	char *buf;

	if (!rdr_get_ap_init_done()) {
		pr_err("[%s]rdr not init\n", __func__);
		return -EPERM;
	}

	g_mntn_fac_info = kzalloc(sizeof(struct mntn_factory_stru), GFP_KERNEL);
	if (!g_mntn_fac_info) {
		pr_err("[%s]malloc mntn_fac_info failed\n", __func__);
		return -ENOMEM;
	}
	mntn_fac_info = g_mntn_fac_info;

	ret = register_module_dump_mem_func(mntn_factory_dump, "MNTN_FAC", MODU_MNTN_FAC);
	if (ret < 0) {
		pr_err("[%s]register_module_dump_mem_func failed\n", __func__);
		ret = -EFAULT;
		goto end;
	}

	ret = get_module_dump_mem_addr(MODU_MNTN_FAC, &mntn_fac_info->mntn_mem_addr);
	if (ret < 0) {
		pr_err("[%s]get_module_dump_mem_addr failed\n", __func__);
		ret = -EFAULT;
		goto end;
	}

	if (!mntn_fac_info->mntn_mem_addr) {
		pr_err("[%s]mntn_mem_addr is null\n", __func__);
		ret = -EPERM;
		goto end;
	}

	buf = mntn_fac_info->mntn_mem_addr + MNTN_FACTORY_HEAD_OFFSET;
	ret = memcpy_s(buf, MNTN_FACTORY_HEAD_SIZE, head, sizeof(head));
	if (ret) {
		pr_err("[%s]sign mntn-fac head failed\n", __func__);
		ret = -EPERM;
		goto end;
	}

	root = debugfs_create_dir("mntn_fac", NULL);
	if (!root) {
		pr_err("[%s]debugfs_create_dir failed\n", __func__);
		ret = -ENOENT;
		goto end;
	}
	d = debugfs_create_file("testpoint", 0660, root, NULL, &mntn_fac_fops);
	if (!d) {
		pr_err("[%s]debugfs_create_file failed\n", __func__);
		debugfs_remove(root);
		ret = -ENOENT;
		goto end;
	}

	return 0;
end:
	kfree(g_mntn_fac_info);
	g_mntn_fac_info = NULL;
	return ret;
}
late_initcall(mntn_factory_init);

MODULE_DESCRIPTION("MNTN FACTORY MODULE");
MODULE_LICENSE("GPL v2");
