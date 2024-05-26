/* SPDX-License-Identifier: GPL-2.0 */
/*
 * file_local.c
 *
 * Copyright (c) 2020-2021 Huawei Technologies Co., Ltd.
 * Author: weilongping@huawei.com
 * Create: 2020-03-26
 *
 */

#include <linux/file.h>
#include <linux/fs.h>
#include <linux/namei.h>
#include <linux/page-flags.h>
#include <linux/pagemap.h>
#include <linux/sched/signal.h>
#include <linux/slab.h>

#include "hmdfs_client.h"
#include "hmdfs_dentryfile.h"
#include "hmdfs_device_view.h"
#include "hmdfs_merge_view.h"
#include "hmdfs_trace.h"

#ifdef CONFIG_HMDFS_1_0
#include "DFS_1_0/adapter.h"
#endif

int hmdfs_file_open_local(struct inode *inode, struct file *file)
{
	int err = 0;
	struct file *lower_file = NULL;
	struct path lower_path;
	struct super_block *sb = inode->i_sb;
	const struct cred *cred = hmdfs_sb(sb)->cred;
	struct hmdfs_file_info *gfi = kzalloc(sizeof(*gfi), GFP_KERNEL);

	if (!gfi) {
		err = -ENOMEM;
		goto out_err;
	}

	hmdfs_get_lower_path(file->f_path.dentry, &lower_path);
	lower_file = dentry_open(&lower_path, file->f_flags, cred);
	hmdfs_put_lower_path(&lower_path);
	if (IS_ERR(lower_file)) {
		err = PTR_ERR(lower_file);
		kfree(gfi);
	} else {
		gfi->lower_file = lower_file;
		file->private_data = gfi;
	}
out_err:
	return err;
}

int hmdfs_file_release_local(struct inode *inode, struct file *file)
{
	struct hmdfs_file_info *gfi = hmdfs_f(file);

#ifdef CONFIG_HMDFS_1_0
	if (hmdfs_i(inode)->adapter_dentry_flag == ADAPTER_PHOTOKIT_DENTRY_FLAG)
		hmdfs_adapter_update(inode, file);
#endif
	file->private_data = NULL;
	fput(gfi->lower_file);
	kfree(gfi);
	return 0;
}

static void hmdfs_file_accessed(struct file *file)
{
	struct file *lower_file = hmdfs_f(file)->lower_file;
	struct inode *inode = file_inode(file);
	struct inode *lower_inode = file_inode(lower_file);

	if (file->f_flags & O_NOATIME)
		return;

	inode->i_atime = lower_inode->i_atime;
}

ssize_t hmdfs_do_read_iter(struct file *file, struct iov_iter *iter,
			   loff_t *ppos)
{
	ssize_t ret;
	struct file *lower_file = hmdfs_f(file)->lower_file;

	if (!iov_iter_count(iter))
		return 0;

	ret = vfs_iter_read(lower_file, iter, ppos, 0);
	hmdfs_file_accessed(file);

	return ret;
}

static ssize_t hmdfs_local_read_iter(struct kiocb *iocb, struct iov_iter *iter)
{
	return hmdfs_do_read_iter(iocb->ki_filp, iter, &iocb->ki_pos);
}

static void hmdfs_file_modified(struct file *file)
{
	struct inode *inode = file_inode(file);
	struct dentry *dentry = file_dentry(file);
	struct file *lower_file = hmdfs_f(file)->lower_file;
	struct inode *lower_inode = file_inode(lower_file);

	inode->i_atime = lower_inode->i_atime;
	inode->i_ctime = lower_inode->i_ctime;
	inode->i_mtime = lower_inode->i_mtime;
	i_size_write(inode, i_size_read(lower_inode));

	if (!hmdfs_i_merge(hmdfs_i(inode)))
		update_inode_to_dentry(dentry, inode);
}

ssize_t hmdfs_do_write_iter(struct file *file, struct iov_iter *iter,
			    loff_t *ppos)
{
	ssize_t ret;
	struct file *lower_file = hmdfs_f(file)->lower_file;
	struct inode *inode = file_inode(file);

	if (!iov_iter_count(iter))
		return 0;

	inode_lock(inode);

	ret = file_remove_privs(file);
	if (ret)
		goto out_unlock;

	file_start_write(lower_file);
	ret = vfs_iter_write(lower_file, iter, ppos, 0);
	file_end_write(lower_file);

	hmdfs_file_modified(file);

out_unlock:
	inode_unlock(inode);
	return ret;
}

ssize_t hmdfs_local_write_iter(struct kiocb *iocb, struct iov_iter *iter)
{
	return hmdfs_do_write_iter(iocb->ki_filp, iter, &iocb->ki_pos);
}

int hmdfs_fsync_local(struct file *file, loff_t start, loff_t end, int datasync)
{
	int err;
	struct file *lower_file = hmdfs_f(file)->lower_file;

	err = __generic_file_fsync(file, start, end, datasync);
	if (err)
		goto out;

	err = vfs_fsync_range(lower_file, start, end, datasync);
out:
	return err;
}

loff_t hmdfs_file_llseek_local(struct file *file, loff_t offset, int whence)
{
	int err = 0;
	struct file *lower_file = NULL;

	err = generic_file_llseek(file, offset, whence);
	if (err < 0)
		goto out;
	lower_file = hmdfs_f(file)->lower_file;
	err = generic_file_llseek(lower_file, offset, whence);
out:
	return err;
}

int hmdfs_file_mmap_local(struct file *file, struct vm_area_struct *vma)
{
	struct hmdfs_file_info *private_data = file->private_data;
	struct file *realfile = NULL;
	int ret;

	if (!private_data)
		return -EINVAL;

	realfile = private_data->lower_file;
	if (!realfile)
		return -EINVAL;

	if (!realfile->f_op->mmap)
		return -ENODEV;

	if (WARN_ON(file != vma->vm_file))
		return -EIO;

	vma->vm_file = get_file(realfile);
	ret = call_mmap(vma->vm_file, vma);
	if (ret)
		fput(realfile);
	else
		fput(file);

	file_accessed(file);

	return ret;
}

const struct file_operations hmdfs_file_fops_local = {
	.owner = THIS_MODULE,
	.llseek = hmdfs_file_llseek_local,
	.read_iter = hmdfs_local_read_iter,
	.write_iter = hmdfs_local_write_iter,
	.mmap = hmdfs_file_mmap_local,
	.open = hmdfs_file_open_local,
	.release = hmdfs_file_release_local,
	.fsync = hmdfs_fsync_local,
	.splice_read = generic_file_splice_read,
	.splice_write = iter_file_splice_write,
};

static int hmdfs_iterate_local(struct file *file, struct dir_context *ctx)
{
	int err = 0;
	loff_t start_pos = ctx->pos;
	struct file *lower_file = hmdfs_f(file)->lower_file;

	if (ctx->pos == -1)
		return 0;

	lower_file->f_pos = file->f_pos;
	err = iterate_dir(lower_file, ctx);
	file->f_pos = lower_file->f_pos;

	if (err < 0)
		ctx->pos = -1;

	trace_hmdfs_iterate_local(file->f_path.dentry, start_pos, ctx->pos,
				  err);
	return err;
}

int hmdfs_dir_open_local(struct inode *inode, struct file *file)
{
	int err = 0;
	struct file *lower_file = NULL;
	struct dentry *dentry = file->f_path.dentry;
	struct path lower_path;
	struct super_block *sb = inode->i_sb;
	const struct cred *cred = hmdfs_sb(sb)->cred;
	struct hmdfs_file_info *gfi = kzalloc(sizeof(*gfi), GFP_KERNEL);

	if (!gfi)
		return -ENOMEM;

	if (IS_ERR_OR_NULL(cred)) {
		err = -EPERM;
		goto out_err;
	}
	hmdfs_get_lower_path(dentry, &lower_path);
	lower_file = dentry_open(&lower_path, file->f_flags, cred);
	hmdfs_put_lower_path(&lower_path);
	if (IS_ERR(lower_file)) {
		err = PTR_ERR(lower_file);
		goto out_err;
	} else {
		gfi->lower_file = lower_file;
		file->private_data = gfi;
	}
	return err;

out_err:
	kfree(gfi);
	return err;
}

static int hmdfs_dir_release_local(struct inode *inode, struct file *file)
{
	struct hmdfs_file_info *gfi = hmdfs_f(file);

	file->private_data = NULL;
	fput(gfi->lower_file);
	kfree(gfi);
	return 0;
}

const struct file_operations hmdfs_dir_ops_local = {
	.owner = THIS_MODULE,
	.iterate = hmdfs_iterate_local,
	.open = hmdfs_dir_open_local,
	.release = hmdfs_dir_release_local,
	.fsync = hmdfs_fsync_local,
};
