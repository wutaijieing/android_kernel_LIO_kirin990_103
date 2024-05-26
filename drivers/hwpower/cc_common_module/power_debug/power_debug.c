// SPDX-License-Identifier: GPL-2.0
/*
 * power_debug.c
 *
 * debug for power module
 *
 * Copyright (c) 2020-2020 Huawei Technologies Co., Ltd.
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/types.h>
#include <chipset_common/hwpower/common_module/power_debug.h>
#include <chipset_common/hwpower/common_module/power_printk.h>

#define HWLOG_TAG power_dbg
HWLOG_REGIST();

static struct list_head g_power_dbg_list;
static struct dentry *g_power_dbg_dir;
static DEFINE_SPINLOCK(g_power_dbg_list_slock);

static int power_dbg_template_show(struct seq_file *s, void *d)
{
	struct power_dbg_attr *pattr = s->private;
	char *buf = NULL;
	int ret;

	if (!pattr || !pattr->show) {
		hwlog_err("invalid show\n");
		return -EINVAL;
	}

	buf = kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	ret = pattr->show(pattr->dev_data, buf, PAGE_SIZE);
	seq_write(s, buf, ret);
	kfree(buf);

	hwlog_info("show: %s,%s ret=%d\n", pattr->dir, pattr->file, ret);
	return 0;
}

static ssize_t power_dbg_template_write(struct file *file,
	const char __user *data, size_t size, loff_t *ppos)
{
	struct power_dbg_attr *pattr = NULL;
	char *buf = NULL;
	int ret;

	pattr = ((struct seq_file *)file->private_data)->private;
	if (!pattr || !pattr->store) {
		hwlog_err("invalid store\n");
		return -EINVAL;
	}

	buf = kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	if (size >= PAGE_SIZE) {
		hwlog_err("input too long\n");
		kfree(buf);
		return -ENOMEM;
	}

	if (copy_from_user(buf, data, size)) {
		hwlog_err("can not copy data form user to kernel\n");
		kfree(buf);
		return -ENOSPC;
	}
	buf[size] = '\0';

	ret = pattr->store(pattr->dev_data, buf, size);
	kfree(buf);

	hwlog_info("store: %s,%s ret=%d\n", pattr->dir, pattr->file, ret);
	return ret;
}

static int power_dbg_template_open(struct inode *inode, struct file *file)
{
	return single_open(file, power_dbg_template_show, inode->i_private);
}

static const struct file_operations power_dbg_template_fops = {
	.open = power_dbg_template_open,
	.read = seq_read,
	.write = power_dbg_template_write,
	.release = single_release,
};

void power_dbg_ops_register(const char *dir, const char *file,
	void *dev_data, power_dbg_show show, power_dbg_store store)
{
	struct power_dbg_attr *new_attr = NULL;
	unsigned long flags;

	if (!dir || !file) {
		hwlog_err("dir or file is null\n");
		return;
	}

	if (!g_power_dbg_list.next) {
		hwlog_info("list init\n");
		INIT_LIST_HEAD(&g_power_dbg_list);
	}

	new_attr = kzalloc(sizeof(*new_attr), GFP_KERNEL);
	if (!new_attr)
		return;

	spin_lock_irqsave(&g_power_dbg_list_slock, flags);
	strncpy(new_attr->dir, dir, POWER_DBG_FILE_NAME_MAX - 1);
	new_attr->dir[POWER_DBG_FILE_NAME_MAX - 1] = '\0';
	strncpy(new_attr->file, file, POWER_DBG_FILE_NAME_MAX - 1);
	new_attr->file[POWER_DBG_FILE_NAME_MAX - 1] = '\0';
	new_attr->mode = POWER_DBG_FILE_PERMS;
	new_attr->dev_data = dev_data;
	new_attr->show = show;
	new_attr->store = store;
	list_add(&new_attr->list, &g_power_dbg_list);
	spin_unlock_irqrestore(&g_power_dbg_list_slock, flags);

	hwlog_info("%s,%s ops register ok\n", dir, file);
}

static int __init power_dbg_init(void)
{
	int ret;
	struct dentry *dir = NULL;
	struct dentry *file = NULL;
	struct list_head *pos = NULL;
	struct list_head *next = NULL;
	struct power_dbg_attr *pattr = NULL;
	unsigned long flags;

	if (!g_power_dbg_list.next) {
		hwlog_info("list init\n");
		INIT_LIST_HEAD(&g_power_dbg_list);
	}

	g_power_dbg_dir = debugfs_create_dir(POWER_DBG_ROOT_FILE, NULL);
	if (IS_ERR_OR_NULL(g_power_dbg_dir)) {
		hwlog_err("root node %s create failed\n", POWER_DBG_ROOT_FILE);
		ret = -EINVAL;
		goto fail_create_dir;
	}

	list_for_each(pos, &g_power_dbg_list) {
		pattr = list_entry(pos, struct power_dbg_attr, list);

		/* create a specified directory */
		dir = debugfs_lookup(pattr->dir, g_power_dbg_dir);
		if (IS_ERR_OR_NULL(dir))
			dir = debugfs_create_dir(pattr->dir, g_power_dbg_dir);
		if (IS_ERR_OR_NULL(dir)) {
			hwlog_err("dir node %s create failed\n", pattr->dir);
			ret = -ENOMEM;
			goto fail_create_file;
		}

		/* create a file in the specified directory */
		file = debugfs_create_file(pattr->file, pattr->mode,
			dir, pattr, &power_dbg_template_fops);
		if (IS_ERR_OR_NULL(file)) {
			hwlog_err("file node %s create failed\n", pattr->file);
			ret = -ENOMEM;
			goto fail_create_file;
		}

		hwlog_info("%s,%s create ok\n", pattr->dir, pattr->file);
	}

	return 0;

fail_create_file:
	debugfs_remove_recursive(g_power_dbg_dir);
	g_power_dbg_dir = NULL;
fail_create_dir:
	spin_lock_irqsave(&g_power_dbg_list_slock, flags);
	list_for_each_safe(pos, next, &g_power_dbg_list) {
		pattr = list_entry(pos, struct power_dbg_attr, list);
		list_del(&pattr->list);
		kfree(pattr);
	}
	INIT_LIST_HEAD(&g_power_dbg_list);
	spin_unlock_irqrestore(&g_power_dbg_list_slock, flags);

	return ret;
}

static void __exit power_dbg_exit(void)
{
	struct list_head *pos = NULL;
	struct list_head *next = NULL;
	struct power_dbg_attr *pattr = NULL;
	unsigned long flags;

	debugfs_remove_recursive(g_power_dbg_dir);
	g_power_dbg_dir = NULL;

	spin_lock_irqsave(&g_power_dbg_list_slock, flags);
	list_for_each_safe(pos, next, &g_power_dbg_list) {
		pattr = list_entry(pos, struct power_dbg_attr, list);
		list_del(&pattr->list);
		kfree(pattr);
	}
	INIT_LIST_HEAD(&g_power_dbg_list);
	spin_unlock_irqrestore(&g_power_dbg_list_slock, flags);
}

late_initcall_sync(power_dbg_init);
module_exit(power_dbg_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("debug for power module driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
