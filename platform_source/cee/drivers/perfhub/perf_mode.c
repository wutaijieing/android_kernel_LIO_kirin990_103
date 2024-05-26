/*
 * perf_mode.c
 *
 * perf mode module
 *
 * Copyright (c) 2021-2021 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/notifier.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/platform_drivers/perf_mode.h>


static unsigned int g_mode = PERF_MODE_2;

static BLOCKING_NOTIFIER_HEAD(perf_mode_notifier_list);

int perf_mode_register_notifier(struct notifier_block *nb)
{
	if (nb)
		return blocking_notifier_chain_register(&perf_mode_notifier_list, nb);

	return -EINVAL;
}

int perf_mode_unregister_notifier(struct notifier_block *nb)
{
	if (nb)
		return blocking_notifier_chain_unregister(&perf_mode_notifier_list, nb);

	return -EINVAL;
}

static void update_perf_mode(unsigned int mode)
{
	if (mode >= MAX_PERF_MODE || mode == g_mode)
		return;

	g_mode = mode;
	blocking_notifier_call_chain(&perf_mode_notifier_list,
				     (unsigned long)mode, NULL);
}

static int perf_mode_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int perf_mode_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t
perf_mode_read(struct file *filp, char __user *buf,
	       size_t count, loff_t *f_pos)
{
	return simple_read_from_buffer(buf, count, f_pos, &g_mode, sizeof(unsigned int));
}

static ssize_t
perf_mode_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *f_pos)
{
	unsigned int value;

	if (count == sizeof(unsigned int)) {
		if (copy_from_user(&value, buf, sizeof(unsigned int)))
			return -EFAULT;
	} else {
		int ret;

		ret = kstrtou32_from_user(buf, count, 16, &value);
		if (ret)
			return ret;
	}

	update_perf_mode(value);

	return count;
}

static struct file_operations perf_mode_fops = {
	.write = perf_mode_write,
	.read = perf_mode_read,
	.open = perf_mode_open,
	.release = perf_mode_release,
	.llseek = noop_llseek,
};

static struct miscdevice perf_mode_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "acpuddr_link_governor_level",
	.fops = &perf_mode_fops,
};

static __init int perf_mode_init(void)
{
	return misc_register(&perf_mode_dev);
}
module_init(perf_mode_init);
