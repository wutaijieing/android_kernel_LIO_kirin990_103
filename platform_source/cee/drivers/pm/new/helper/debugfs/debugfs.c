/*
 * debugfs.c
 *
 * debug information of suspend
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
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

#include "debugfs.h"
#include "helper/log/lowpm_log.h"

#include <linux/debugfs.h>

static int lowpm_debugfs_show(struct seq_file *seq, void *private)
{
	struct lowpm_debugfile *attr = seq->private;

	if (!attr->show)
		return -EPERM;

	return attr->show(seq, NULL);
}


static int lowpm_debugfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, lowpm_debugfs_show, inode->i_private);
}

static ssize_t lowpm_debugfs_write(struct file *file,
		const char __user *buf, size_t count, loff_t *ppos)
{
	struct seq_file *seq = file->private_data;
	struct lowpm_debugfile *attr = seq->private;

	if (!attr->store)
		return -EPERM;

	return attr->store(seq, buf, count, ppos);
}

static const struct file_operations g_lowpm_debugfs_fops = {
	.open		= lowpm_debugfs_open,
	.read		= seq_read,
	.write		= lowpm_debugfs_write,
	.llseek		= seq_lseek,
	.release	= seq_release,
};

int lowpm_create_debugfs(const struct lowpm_debugdir *fs)
{
	struct dentry *pdentry = NULL;
	int i;

	if (fs->dir == NULL)
		return -EINVAL;

	pdentry = debugfs_lookup(fs->dir, NULL);
	if (pdentry == NULL) {
		pdentry = debugfs_create_dir(fs->dir, NULL);
		if (IS_ERR(pdentry)) {
			lp_err("create %s failed", fs->dir);
			return -ENOMEM;
		}
	}

	for (i = 0; fs->files[i].name != NULL; i++) {
		struct dentry *entry = debugfs_create_file(
				fs->files[i].name, fs->files[i].mode,
				pdentry, (void *)&fs->files[i],
				&g_lowpm_debugfs_fops);
		if (IS_ERR(entry)) {
			lp_err("create %s failed", fs->files[i].name);
			return -ENOMEM;
		}
	}

	return 0;
}
