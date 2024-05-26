/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: debugfs for usb.
 * Create: 2019-6-16
 *
 * This software is distributed under the terms of the GNU General
 * Public License ("GPL") as published by the Free Software Foundation,
 * either version 2 of that License or (at your option) any later version.
 */
#include <securec.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>

#include "core.h"
#include "io.h"

static struct dentry *dwc3_symlink;

static int dwc3_maximum_speed_show(struct seq_file *s, void *reserved)
{
	struct dwc3 *dwc = s->private;
	unsigned long flags;

	spin_lock_irqsave(&dwc->lock, flags);
	seq_printf(s, "current maximum_speed %s\n"
		      "Usage:\n"
		      " write \"full\" into this file to force USB full-speed\n"
		      " write \"high\" into this file to force USB high-speed\n",
			usb_speed_string((enum usb_device_speed)dwc->maximum_speed));
	spin_unlock_irqrestore(&dwc->lock, flags);
	return 0;
}

static int dwc3_maximum_speed_open(struct inode *inode, struct file *file)
{
	return single_open(file, dwc3_maximum_speed_show, inode->i_private);
}

static ssize_t dwc3_maximum_speed_write(struct file *file,
		const char __user *ubuf, size_t count, loff_t *ppos)
{
	struct seq_file *s = file->private_data;
	struct dwc3 *dwc = s->private;
	unsigned long flags;
	u32 maximum_speed;
	char buf[8] = {0};

	if (copy_from_user(buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
		return -EFAULT;

	/* as follows copy numbers string to buf */
	if (!strncmp(buf, "super+", 6))
		maximum_speed = USB_SPEED_SUPER_PLUS;
	else if (!strncmp(buf, "super", 5))
		maximum_speed = USB_SPEED_SUPER;
	else if (!strncmp(buf, "high", 4))
		maximum_speed = USB_SPEED_HIGH;
	else if (!strncmp(buf, "full", 4))
		maximum_speed = USB_SPEED_FULL;
	else if (!strncmp(buf, "clear", 5))
		maximum_speed = USB_SPEED_UNKNOWN;
	else
		return -EINVAL;

	spin_lock_irqsave(&dwc->lock, flags);
	dwc->maximum_speed = maximum_speed;
	spin_unlock_irqrestore(&dwc->lock, flags);

	return count;
}

static const struct file_operations dwc3_maximum_speed_fops = {
	.open			= dwc3_maximum_speed_open,
	.write			= dwc3_maximum_speed_write,
	.read			= seq_read,
	.llseek			= seq_lseek,
	.release		= single_release,
};

static int dwc3_cp_test_show(struct seq_file *s, void *reserved)
{
	struct dwc3 *dwc = s->private;
	unsigned long flags;

	spin_lock_irqsave(&dwc->lock, flags);
	seq_printf(s, "[cptest mode: %u]\n", dwc->cp_test);
	spin_unlock_irqrestore(&dwc->lock, flags);
	return 0;
}

static int dwc3_cp_test_open(struct inode *inode, struct file *file)
{
	return single_open(file, dwc3_cp_test_show, inode->i_private);
}

static ssize_t dwc3_cp_test_write(struct file *file,
		const char __user *ubuf, size_t count, loff_t *ppos)
{
	struct seq_file *s = file->private_data;
	struct dwc3 *dwc = s->private;
	unsigned long flags;
	u8 cp_test;
	char buf[8] = {0};

	if (!ubuf)
		return -EFAULT;

	if (copy_from_user(buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
		return -EFAULT;

	if (!strncmp(buf, "1", 1))
		cp_test = 1;
	else if (!strncmp(buf, "0", 1))
		cp_test = 0;
	else
		return -EINVAL;

	spin_lock_irqsave(&dwc->lock, flags);
	dwc->cp_test = cp_test;
	spin_unlock_irqrestore(&dwc->lock, flags);

	return count;
}

static const struct file_operations dwc3_cp_test_fops = {
	.open			= dwc3_cp_test_open,
	.write			= dwc3_cp_test_write,
	.read			= seq_read,
	.llseek			= seq_lseek,
	.release		= single_release,
};

static atomic_t dwc3_noc_err_flag = ATOMIC_INIT(0);
static u32 dwc3_noc_err_addr;

static ssize_t dwc3_noc_err_write(struct file *file,
		const char __user *ubuf, size_t count, loff_t *ppos)
{
	char buf[32] = {0};

	if (!ubuf)
		return -EFAULT;

	if (copy_from_user(buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
		return -EFAULT;

	if (sscanf_s(buf, "0x%x", &dwc3_noc_err_addr) != 1) {
		pr_err("inject addr illegal !\n");
		return -EFAULT;
	}

	pr_err("%s: 0x%x\n", __func__, dwc3_noc_err_addr);
	atomic_set(&dwc3_noc_err_flag, 1);

	return count;
}

static int dwc3_noc_err_show(struct seq_file *s, void *reserved)
{
	seq_printf(s, "[noc activity test mode: %d]\n",
			atomic_read(&dwc3_noc_err_flag));
	return 0;
}

static int dwc3_noc_err_open(struct inode *inode, struct file *file)
{
	return single_open(file, dwc3_noc_err_show, inode->i_private);
}

int dwc3_is_test_noc_err(void)
{
	return atomic_read(&dwc3_noc_err_flag);
}

uint32_t dwc3_get_noc_err_addr(uint32_t addr)
{
	if (!addr)
		pr_err("[USB.NOC.TEST] get input addr:0x%x\n", addr);

	return dwc3_noc_err_addr;
}

static const struct file_operations dwc3_noc_err_fops = {
	.open			= dwc3_noc_err_open,
	.write			= dwc3_noc_err_write,
	.read			= seq_read,
	.llseek			= seq_lseek,
	.release		= single_release,
};

static int dwc3_dis_u1u2_show(struct seq_file *s, void *reserved)
{
	struct dwc3 *dwc = s->private;

	seq_printf(s, "%u\n", dwc->dis_u1u2_quirk);
	return 0;
}

static int dwc3_dis_u1u2_open(struct inode *inode, struct file *file)
{
	return single_open(file, dwc3_dis_u1u2_show, inode->i_private);
}

static ssize_t dwc3_dis_u1u2_write(struct file *file,
		const char __user *ubuf, size_t count, loff_t *ppos)
{
	struct seq_file *s = file->private_data;
	struct dwc3 *dwc = s->private;
	unsigned long flags;
	char buf[2] = {0};
	unsigned int dis_u1u2_quirk;

	if (!ubuf)
		return -EFAULT;

	if (copy_from_user(buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
		return -EFAULT;

	if (!strncmp(buf, "1", 1))
		dis_u1u2_quirk = 1;
	else if (!strncmp(buf, "0", 1))
		dis_u1u2_quirk = 0;
	else
		return -EINVAL;

	spin_lock_irqsave(&dwc->lock, flags);
	dwc->dis_u1u2_quirk = dis_u1u2_quirk;
	pr_info("%s: set dis_u1u2_quirk %u\n", __func__, dwc->dis_u1u2_quirk);
	spin_unlock_irqrestore(&dwc->lock, flags);

	return count;
}

static const struct file_operations dwc3_dis_u1u2_fops = {
	.open			= dwc3_dis_u1u2_open,
	.write			= dwc3_dis_u1u2_write,
	.read			= seq_read,
	.llseek			= seq_lseek,
	.release		= single_release,
};

#ifdef CONFIG_PROC_FS
static int dwc3_show_llinkerrinj(struct seq_file *m, void *v)
{
	struct dwc3 *dwc = m->private;
	unsigned long flags;
	u32 val;

	spin_lock_irqsave(&dwc->lock, flags);
	val = dwc3_readl(dwc->regs, DWC3_LLINKERRINJ(0));
	spin_unlock_irqrestore(&dwc->lock, flags);

	seq_printf(m, "0x%08x", val);

	return 0;
}

static int llinkerrinj_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, dwc3_show_llinkerrinj, PDE_DATA(inode));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops fops_llinkerrinj = {
	.proc_open		= llinkerrinj_proc_open,
	.proc_read		= seq_read,
	.proc_lseek		= seq_lseek,
	.proc_release	= single_release,
};
#else
static const struct file_operations fops_llinkerrinj = {
	.owner		= THIS_MODULE,
	.open		= llinkerrinj_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};
#endif

static void dwc3_create_proc_file(struct dwc3 *dwc)
{
	struct proc_dir_entry *proc_entry = NULL;

	proc_entry = proc_create_data("DWC3_LLINKERRINJ", 0444, NULL, /* file permission 0444 */
				      &fops_llinkerrinj, dwc);
	if (!proc_entry)
		pr_err("%s: create llinkerrinj proc failed\n", __func__);
}

static void dwc3_remove_proc_file(void)
{
	remove_proc_entry("DWC3_LLINKERRINJ", NULL);
}
#else
#define dwc3_create_proc_file(p)
#define dwc3_remove_proc_file()
#endif /* CONFIG_PROC_FS */

void dwc3_chip_debugfs_init(struct dwc3 *dwc, struct dentry *root)
{
	struct dentry *file = NULL;

	if (IS_ENABLED(CONFIG_USB_DWC3_DUAL_ROLE) ||
			IS_ENABLED(CONFIG_USB_DWC3_GADGET)) {
		file = debugfs_create_file("maximum_speed", 0644, root, /* file permission 0644 */
				dwc, &dwc3_maximum_speed_fops);
		if (!file)
			pr_err("%s: create maximum_speed debugfs failed\n", __func__);

		file = debugfs_create_file("cptest", 0644, root, /* file permission 0644 */
				dwc, &dwc3_cp_test_fops);
		if (!file)
			pr_err("%s: create cptest debugfs failed\n", __func__);

		file = debugfs_create_file("noc_err", 0644, root, /* file permission 0644 */
				dwc, &dwc3_noc_err_fops);
		if (!file)
			pr_err("%s: create noc_err debugfs failed\n", __func__);

		file = debugfs_create_file("dis_u1u2", 0644, root, /* file permission 0644 */
				dwc, &dwc3_dis_u1u2_fops);
		if (!file)
			pr_err("%s: create dis_u1u2 debugfs failed\n", __func__);

		dwc3_symlink = debugfs_create_symlink("dwc3",
						      NULL,
						      dev_name(dwc->dev));
		if (IS_ERR_OR_NULL(dwc3_symlink))
			pr_err("%s: create symlink to usb failed\n", __func__);
	}

	dwc3_create_proc_file(dwc);
}

void dwc3_chip_debugfs_exit(void)
{
	dwc3_remove_proc_file();
	debugfs_remove(dwc3_symlink);
	dwc3_symlink = NULL;
}
