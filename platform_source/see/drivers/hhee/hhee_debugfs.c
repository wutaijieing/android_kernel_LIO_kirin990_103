/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2017-2021. All rights reserved.
 * Description: hhee debugfs
 * Create : 2017/12/6
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/debugfs.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <securec.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
#include <asm/compiler.h>
#endif
#include <asm/io.h>
#include <asm/uaccess.h>
#include "hhee.h"

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0))
#include <linux/uaccess.h>
#endif

static struct dentry *hhee_dir = 0;

/* following functions are used for log read */
static int hhee_log_open(struct inode *inode, struct file *pfile)
{
	pfile->private_data = inode->i_private;
	return 0;
}

static ssize_t hhee_log_read(struct file *file, char __user *buf, size_t count,
			     loff_t *offp)
{
	int logtype = (int)(uintptr_t)file->private_data;

	pr_info("hhee log read, logtype is %d\n", logtype);
	return hhee_copy_logs(buf, count, offp, logtype);
}

#define CONST_INFO_LEN (18)
static ssize_t hhee_log_write(struct file *file, const char __user *buf,
			      size_t count, loff_t *offp)
{
	int logtype = (int)(uintptr_t)file->private_data;
	char info[CONST_INFO_LEN];
	char *s = "kernel_crash_test";
	const int len = (int)strlen(s);

	if (!buf)
		return -EINVAL;

	/* if then count cannot be matched with CONST_INFO_LEN,
	 * the input string cannot be kernel_crash_test either */
	if (count != CONST_INFO_LEN)
		return -1;

	if (CRASH_LOG == logtype) {
		if (memset_s(info, CONST_INFO_LEN, 0x0, CONST_INFO_LEN) != EOK) {
			pr_err("[%s] memset_s error\n",  __func__);
			return -EFAULT;
		}
		if (copy_from_user(info, buf, CONST_INFO_LEN - 1))
			return -1;
		info[CONST_INFO_LEN - 1] = '\0';
		if (!strncmp(info, s, len)) {
			pr_err("call AP crash from HHEE for test.\n");
			(void)hhee_fn_hvc((unsigned long)HHEE_HVC_NOTIFY, 0ul, 0ul, 0ul);
		}
	}

	return count;
}

const struct file_operations tzdbg_fops = {
	.owner = THIS_MODULE,
	.read = hhee_log_read,
	.write = hhee_log_write,
	.open = hhee_log_open,
};

int hhee_init_debugfs(void)
{
	struct dentry *junk = NULL;
	int ret;

	hhee_dir = debugfs_create_dir("hhee", NULL);
	if (!hhee_dir) {
		printk(KERN_ALERT "HHEE: failed to create /sys/kernel/debug/hhee\n");
		return -1;
	}

	junk = debugfs_create_file("log", 0220, hhee_dir, (void *)NORMAL_LOG,
				   &tzdbg_fops);
	if (!junk) {
		pr_err("HHEE: failed to create /sys/kernel/debug/hhee/log\n");
		ret = -1;
		goto out;
	}

	junk = debugfs_create_file("crashlog", 0220, hhee_dir,
				   (void *)CRASH_LOG, &tzdbg_fops);
	if (!junk) {
		pr_err("HHEE: failed to create /sys/kernel/debug/hhee/crashlog\n");
		ret = -1;
		goto out;
	}

	junk = debugfs_create_file("monitorlog", 0220, hhee_dir,
				   (void *)MONITOR_LOG, &tzdbg_fops);
	if (!junk) {
		pr_err("HHEE: failed to create /sys/kernel/debug/hhee/monitorlog\n");
		ret = -1;
		goto out;
	}

	return 0;
out:
	debugfs_remove_recursive(hhee_dir);
	return ret;
}

/* This is called when the module is removed */
void hhee_cleanup_debugfs(void)
{
	debugfs_remove_recursive(hhee_dir);
}
