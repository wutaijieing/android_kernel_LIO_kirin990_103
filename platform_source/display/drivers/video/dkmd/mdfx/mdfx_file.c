/*
 * Copyright (c) 2019-2019, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/dirent.h>
#include <linux/syscalls.h>
#include <linux/slab.h>

#include "mdfx_priv.h"
#include "mdfx_utils.h"
#include "mdfx_file.h"

#define FILE_MAX_SIZE_DEFAULT (4 * 1024 * 1024)
#define FILE_MAX_NUM_DEFAULT  10
#define YEAR_1900 1900
static int __mdfx_chown(const char *filename, uid_t user, gid_t group)
{
	mm_segment_t old_fs;
	int chown_ret;
	int chmod_ret;

	if (IS_ERR_OR_NULL(filename))
		return -1;

	old_fs = get_fs();
	set_fs(KERNEL_DS); //lint !e501

	chown_ret = (int)sys_chown((const char __user *)filename, user, group);
	if (chown_ret)
		mdfx_pr_err("chown %s uid [%d] gid [%d] failed err [%d]!\n",
			filename, user, group, chown_ret);

	chmod_ret = (int)sys_chmod((const char __user *)filename, DIR_DEFAULT_MODE);
	if (chmod_ret)
		mdfx_pr_err("chmod %s 0775 failed error [%d]!\n", filename, chmod_ret);

	set_fs(old_fs);

	if (chown_ret || chmod_ret)
		return -1;

	return 0;
}

static int __mdfx_remove_file(const char *fullname)
{
	mm_segment_t old_fs;
	int ret;

	if (IS_ERR_OR_NULL(fullname))
		return -1;

	old_fs = get_fs();
	set_fs(KERNEL_DS); //lint !e501

	ret = (int)sys_access(fullname, 0);
	if (!ret) {
		if (sys_unlink(fullname)) {
			mdfx_pr_err("fail to rm the file %s!\n", fullname);
			set_fs(old_fs);
			return -1;
		}
	}

	set_fs(old_fs);
	return 0;
}

static int __mdfx_create_dir(const char *path)
{
	mm_segment_t old_fs;
	int ret;

	old_fs = get_fs();
	set_fs(KERNEL_DS); //lint !e501

	if (IS_ERR_OR_NULL(path))
		return -1;

	ret = (int)sys_access(path, 0);
	if (ret != 0) {
		ret = sys_mkdir(path, DIR_DEFAULT_MODE);
		if (ret != 0) {
			mdfx_pr_warn("fail to create the dir %s, result is %d\n", path, ret);
			set_fs(old_fs);
			return ret;
		}
	} else {
		set_fs(old_fs);
		return 0;
	}

	set_fs(old_fs);
	return __mdfx_chown(path, ROOT_UID, SYSTEM_GID);
}

int mdfx_create_dir(const char *path)
{
	char cur_path[DIR_PATH_LEN] = {0};
	int index = 0;
	int ret = 0;

	if (IS_ERR_OR_NULL(path)) {
		mdfx_pr_err("dir is null, so fail to make it!\n");
		return -1;
	}

	if (*path != '/')
		return -1;

	cur_path[index++] = *path++;

	while (*path != '\0') {
		if (*path == '/') {
			ret = __mdfx_create_dir(cur_path);
			/* if the dir exist, then continue */
			if (ret && (ret != (-EEXIST)))
				return ret;
		}

		cur_path[index] = *path;
		path++;
		index++;
	}

	return 0;
}

static void mdfx_add_file_to_arry(struct mdfx_redu_file_node *redu_file_arry, int count,
		struct mdfx_redu_file_node *file_node, __kernel_time_t *newest_time, int *newest_index)
{
	struct mdfx_redu_file_node *redu_file = NULL;
	int i;

	if (IS_ERR_OR_NULL(redu_file_arry) || IS_ERR_OR_NULL(file_node) || IS_ERR_OR_NULL(newest_time) || IS_ERR_OR_NULL(newest_index))
		return;

	if (file_node->modify_time > *newest_time)
		return;

	redu_file = redu_file_arry + *newest_index;
	redu_file->file_name = file_node->file_name;
	redu_file->modify_time = file_node->modify_time;

	*newest_time = 0;
	for (i = 0; i < count; i++) {
		redu_file = redu_file_arry + i;
		if (redu_file->modify_time > *newest_time) {
			*newest_time = redu_file->modify_time;
			*newest_index = i;
		}
	}

	mdfx_pr_info("*newest_time = %ld, *newest_index=%d", *newest_time, *newest_index);
}

static int mdfx_remove_the_oldest_files(struct list_head *redu_file_list, int file_total,
		int max_file_count, const char *dir_path)
{
	struct mdfx_redu_file_node *redu_file_arry = NULL;
	struct mdfx_redu_file_node *redu_file = NULL;
	struct mdfx_redu_file_node *file_node = NULL;
	struct mdfx_redu_file_node *_node_ = NULL;
	char stat_filename[FILENAME_LEN] = {0};
	int filename_len;
	__kernel_time_t newest_time = 0;
	int newest_index = 0;
	int index = 0;
	int count;

	if (IS_ERR_OR_NULL(redu_file_list) || IS_ERR_OR_NULL(dir_path))
		return file_total;

	if (max_file_count <= 1)
		return file_total;

	if (file_total < max_file_count)
		return file_total;

	count = file_total - max_file_count + 1;
	redu_file_arry = kzalloc(count * sizeof(*redu_file_arry), GFP_KERNEL);
	if (!redu_file_arry)
		return file_total;

	list_for_each_entry_safe(file_node, _node_, redu_file_list, node) {
		if (!file_node->file_name) {
			mdfx_pr_info("file name is null, timestap = %ld", file_node->modify_time);
			continue;
		}

		if (index < count) {
			redu_file = redu_file_arry + index;
			redu_file->file_name = file_node->file_name;
			redu_file->modify_time = file_node->modify_time;
			if (redu_file->modify_time > newest_time) {
				newest_time = redu_file->modify_time;
				newest_index = index;
			}
			index++;
			continue;
		}

		mdfx_add_file_to_arry(redu_file_arry, count, file_node, &newest_time, &newest_index);
	}

	for (index = 0; index < count; index++) {
		redu_file = redu_file_arry + index;
		if (!redu_file->file_name)
			continue;

		filename_len = scnprintf(stat_filename, FILENAME_LEN - 1, "%s%s", dir_path, redu_file->file_name);
		if (filename_len > 0) {
			stat_filename[filename_len] = '\0';
			__mdfx_remove_file(stat_filename);

			mdfx_pr_info("remove file %s, count=%d, max_file_count=%d", stat_filename, count, max_file_count);
		}
	}

	kfree(redu_file_arry);
	return file_total - count;
}

static void mdfx_free_redu_file_list(struct list_head *redu_file_list)
{
	struct mdfx_redu_file_node *file_node = NULL;
	struct mdfx_redu_file_node *_node_ = NULL;

	if (IS_ERR_OR_NULL(redu_file_list))
		return;

	list_for_each_entry_safe(file_node, _node_, redu_file_list, node) {
		list_del(&file_node->node);
		kfree(file_node);
		file_node = NULL;
	}
}

#define BUF_SIZE (8192 * 2)
int mdfx_remove_redundant_files(const char *dir_path, const char *include_str, int max_file_count)
{
	struct linux_dirent64 *dirent = NULL;
	char stat_filename[FILENAME_LEN] = {0};
	struct kstat st;
	void *buf = NULL;
	int num;
	int ret;
	int dir;
	int total = 0;
	int filename_len = 0;
	struct list_head redu_file_list;
	struct mdfx_redu_file_node *file_node = NULL;

	if (IS_ERR_OR_NULL(dir_path) || IS_ERR_OR_NULL(include_str))
		return 0;

	buf = kzalloc(BUF_SIZE, GFP_KERNEL);
	if (!buf)
		return 0;

	dir = sys_open(dir_path, O_RDONLY, 0);
	if (dir < 0) {
		mdfx_pr_err("open dir=%s fail\n", dir_path);
		kfree(buf);
		return 0;
	}

	INIT_LIST_HEAD(&redu_file_list);

	dirent = buf;
	num = sys_getdents64(dir, dirent, BUF_SIZE);
	while (num > 0) {
		if ((strcmp(dirent->d_name, ".") == 0) ||
			(strcmp(dirent->d_name, "..") == 0)) {
			num -= dirent->d_reclen;
			dirent = (void *)dirent + dirent->d_reclen;
			continue;
		}

		/* if filename don't include the include_str, continue */
		if (strstr(dirent->d_name, include_str) == NULL) {
			num -= dirent->d_reclen;
			dirent = (void *)dirent + dirent->d_reclen;
			continue;
		}

		filename_len = scnprintf(stat_filename, FILENAME_LEN - 1, "%s%s", dir_path, dirent->d_name);
		stat_filename[filename_len] = '\0';
		ret = vfs_lstat(stat_filename, &st);
		if (ret != 0)
			continue;

		if (S_ISREG(st.mode)) {
			file_node = kzalloc(sizeof(*file_node), GFP_KERNEL);
			if (file_node) {
				file_node->modify_time = st.mtime.tv_sec;
				file_node->file_name = dirent->d_name;
				list_add_tail(&file_node->node, &redu_file_list);
				total++;
			} else {
				mdfx_pr_info("file_node 0x%pK", file_node);
			}
		}

		num -= dirent->d_reclen;
		dirent = (void *)dirent + dirent->d_reclen;
	}

	sys_close(dir);
	kfree(buf);
	buf = NULL;

	mdfx_remove_the_oldest_files(&redu_file_list, total, max_file_count, dir_path);
	mdfx_free_redu_file_list(&redu_file_list);

	return total;
}

int mdfx_file_get_size(const char *filename)
{
	struct kstat file_stat;
	int ret;
	mm_segment_t old_fs;

	if (IS_ERR_OR_NULL(filename) || strlen(filename) == 0)
		return -1;

	old_fs = get_fs();
	set_fs(KERNEL_DS); //lint !e501

	ret = vfs_stat(filename, &file_stat);
	if (ret != 0) {
		mdfx_pr_err("fail to get stat, filenamer %s, result is %d\n",
						filename, ret);
		set_fs(old_fs);
		return -1;
	}

	set_fs(old_fs);

	return file_stat.size;
}

struct file* mdfx_file_open(const char *filename)
{
	struct file *file = NULL;

	if (IS_ERR_OR_NULL(filename)) {
		mdfx_pr_err("filename is NULL");
		return NULL;
	}

	file = filp_open(filename, O_CREAT | O_RDWR | O_APPEND, FILE_DEFAULT_MODE);
	if (IS_ERR_OR_NULL(file)) {
		mdfx_pr_err("filp_open returned:filename %s, error %ld\n",
			filename, PTR_ERR(file));
		return NULL;
	}

	return file;
}

int mdfx_file_write(struct file *file, const char *buf, uint32_t buf_len)
{
	ssize_t write_len;

	if (IS_ERR_OR_NULL(buf) || buf_len == 0 || IS_ERR_OR_NULL(file))
		return -1;

	write_len = vfs_write(file, (char __user *)buf, buf_len, &(file->f_pos));
	if (write_len != buf_len) {
		mdfx_pr_warn("vfs_write filename %s, write_len %ld except len %u\n",
			file->f_path.dentry->d_iname, write_len, buf_len);
		return -1;
	}

	return 0;
}

int mdfx_file_read(struct file *file, const char *buf, uint32_t buf_len)
{
	ssize_t read_len;

	if (IS_ERR_OR_NULL(buf) || buf_len == 0 || IS_ERR_OR_NULL(file))
		return -1;

	read_len = vfs_read(file, (char __user *)buf, buf_len, &(file->f_pos));
	if (read_len < 0) {
		mdfx_pr_warn("vfs_read filename %s, read_len %ld except len %u\n",
			file->f_path.dentry->d_iname, read_len, buf_len);
		return -1;
	}

	return read_len;
}

int mdfx_file_save(const char *filename, const char *buf, uint32_t buf_len)
{
	struct file *fd = NULL;
	mm_segment_t old_fs;
	int ret;

	if (IS_ERR_OR_NULL(buf) || IS_ERR_OR_NULL(filename)) {
		mdfx_pr_err("buf is NULL");
		return -1;
	}

	fd = mdfx_file_open(filename);
	if (IS_ERR_OR_NULL(fd)) {
		mdfx_pr_err("filp_open returned:filename %s, error %ld\n",
			filename, PTR_ERR(fd));
		return PTR_ERR(fd);
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS); //lint !e501

	ret = mdfx_file_write(fd, buf, buf_len);
	if (ret != 0)
		mdfx_pr_warn("vfs_write filename %s fail, except len %u\n", filename, buf_len);

	set_fs(old_fs);
	filp_close(fd, NULL);

	return ret;
}

static const char* mdfx_get_mdfx_dir_path()
{
	int ret;

	ret = (int)sys_access(DFX_FILEDIR, 0);
	if (!ret)
		return DFX_FILEDIR;

	return DFX_FILEDIR_BACKUP;
}

void mdfx_file_get_name(char *filename, const char *modulename, const char *include_str, uint32_t len)
{
	struct dfx_time dt = mdfx_get_rtc_time();
	const char *path = mdfx_get_mdfx_dir_path();
	char postfix[FILENAME_LEN] = {0};
	char *file_type_name = NULL;
	int ret;

	if (IS_ERR_OR_NULL(filename) || IS_ERR_OR_NULL(modulename) || IS_ERR_OR_NULL(include_str))
		return;

	if (strcmp(modulename, MODULE_NAME_LOGGER) == 0)
		file_type_name = "log";
	else
		file_type_name = "txt";

	sprintf(postfix, ".%s.%s", include_str, file_type_name);
	ret = snprintf(filename, len, "%s/%s/%lu-%.2d%.2d-%.2d%.2d%.2d.%06lu%s", path, modulename,
		YEAR_1900 + dt.tm_rtc.tm_year, dt.tm_rtc.tm_mon + 1, dt.tm_rtc.tm_mday,
		dt.tm_rtc.tm_hour, dt.tm_rtc.tm_min, dt.tm_rtc.tm_sec, dt.tv.tv_usec, postfix);
	if (ret < 0)
		filename[0] = '\0';
	mdfx_pr_info("filename=%s\n", filename);
}

void mdfx_file_get_dir_name(char *dir_name, const char *modulename, uint32_t len)
{
	int ret;
	if (IS_ERR_OR_NULL(dir_name) || IS_ERR_OR_NULL(modulename))
		return;

	ret = snprintf(dir_name, len, "%s/%s/", mdfx_get_mdfx_dir_path(), modulename);
	if (ret < 0)
		dir_name[0] = '\0';
}

void mdfx_file_data_init(struct mdfx_file_spec *file_spec)
{
	if (IS_ERR_OR_NULL(file_spec))
		return;

	file_spec->file_max_num = FILE_MAX_NUM_DEFAULT;
	file_spec->file_max_size = FILE_MAX_SIZE_DEFAULT;
}

// info manager
int mdfx_file_query_spec(struct mdfx_pri_data *data, void __user *argp)
{
	int ret;

	if (IS_ERR_OR_NULL(data)) {
		mdfx_pr_err("file info NULL Pointer\n");
		return -EINVAL;
	}

	if (!argp) {
		mdfx_pr_err("argp NULL Pointer\n");
		return -EINVAL;
	}

	ret = copy_to_user(argp, &data->file_spec, sizeof(data->file_spec));
	if (ret != 0) {
		mdfx_pr_err("copy to user failed!ret=%d", ret);
		return -EFAULT;
	}

	mdfx_pr_info("file_max_num %u, file_max_size %u\n",
			data->file_spec.file_max_num,
			data->file_spec.file_max_size);

	return 0;
}

